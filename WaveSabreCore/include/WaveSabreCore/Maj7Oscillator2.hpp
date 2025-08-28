// ============================================================================
// Oscillator core with clean separation:
//   PhaseAccumulator  ->  WaveformCore (edges)  ->  EdgeSink (band-limit)
// ============================================================================

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"


namespace WaveSabreCore
{
namespace M7
{
// ============================================================================
// 1) Phase engine (pure kinematics)
//    - PHASE IS NORMALIZED IN [0,1) (one cycle)
//    - ASSUMPTION: AT MOST ONE WRAP AND ONE EXTERNAL RESET PER SAMPLE
//      (i.e., phaseDelta01PerSample must be <= 1.0)
//    - Events are reported with their fractional time within the current sample.
// ============================================================================

enum class PhaseEventKind
{
  Wrap,
  Reset
};

struct InSamplePhaseEvent
{
  // Fraction of the current sample when this event happens, [0,1].
  // 0 means "exactly at sample start", 1 means "at the boundary to next sample".
  double whenInSample01 = 0.0f;
  PhaseEventKind kind = PhaseEventKind::Wrap;
};

struct PhaseAdvance
{
  // Phase at start/end of the sample, normalized [0,1).
  double phaseStart01 = 0.0f;
  double phaseEnd01 = 0.0f;

  // Phase delta per sample (normalized cycles per sample, i.e. Hz/sampleRate).
  // ASSUMPTION: 0 < phaseDelta01PerSample <= 1.0
  double phaseDelta01PerSample = 0.0f;

  // In-sample events (at most: one wrap + one external reset).
  InSamplePhaseEvent events[2];
  int eventCount = 0;
};

class PhaseAccumulator
{
public:
  void setPhase01(double phase01)
  {
    mPhase01 = math::wrap01(phase01);
  }
  void setPhaseStepFromFrequencyHz(float hz, float offset = 0)
  {
    mPhaseDelta01 = std::max(hz * Helpers::CurrentSampleRateRecipF, 0.0f) + offset;
  }
  void setPhaseDelta01(double d01)
  {
    mPhaseDelta01 = std::max(d01, 0.0);
  }
  double GetPhaseDelta01() const
  {
    return mPhaseDelta01;
  }
  double GetPhase01() const
  {
    return mPhase01;
  }

  // Schedule a hard sync (phase reset to 0) at a fractional time within THIS sample.
  // If multiple calls occur before advance(), only the last is kept.
  void scheduleResetAtFraction(double whenInSample01)
  {
    mHasPendingReset = true;
    mResetWhen01 = math::clamp01(whenInSample01);
  }

  // Advance one sample and describe the kinematics (including fractional event times).
  PhaseAdvance advanceOneSample()
  {
    PhaseAdvance out{};
    out.phaseStart01 = mPhase01;
    out.phaseDelta01PerSample = mPhaseDelta01;

    // --- Gather potential events (wrap + optional reset) ---
    InSamplePhaseEvent evs[2];
    int n = 0;

    // Wrap?
    // If we cross 1.0 within this sample, record when it happens.
    if ((mPhase01 + mPhaseDelta01) >= 1.0f)
    {
      const double wrapWhen = (1.0f - mPhase01) / (mPhaseDelta01 + FLT_MIN);
      evs[n++] = {math::clamp01(wrapWhen), PhaseEventKind::Wrap};
    }
    // Reset (hard sync)?
    if (mHasPendingReset)
    {
      evs[n++] = {math::clamp01(mResetWhen01), PhaseEventKind::Reset};
      mHasPendingReset = false;
    }

    // Sort events by time (there are 0..2).
    if (n == 2 && evs[0].whenInSample01 > evs[1].whenInSample01)
      std::swap(evs[0], evs[1]);

    // --- Walk the sample, applying events in chronological order ---
    double ph = mPhase01;
    double tCursor = 0.0f;

    for (int i = 0; i < n; ++i)
    {
      const double dt = evs[i].whenInSample01 - tCursor;  // elapsed fraction since last event
      ph += mPhaseDelta01 * dt;                           // drift to event time
      // apply event atomically
      if (evs[i].kind == PhaseEventKind::Wrap)
        ph -= 1.0f;  // wrap to [0,1)
      else
        ph = 0.0f;  // hard reset
      tCursor = evs[i].whenInSample01;
    }
    // Finish remaining fraction
    ph += mPhaseDelta01 * (1.0f - tCursor);

    out.phaseEnd01 = math::wrap01(ph);
    for (int i = 0; i < n; ++i)
      out.events[i] = evs[i];
    out.eventCount = n;

    mPhase01 = out.phaseEnd01;  // persist
    return out;
  }

private:
  double mPhase01 = 0.0;       // [0,1)
  double mPhaseDelta01 = 0.0;  // cycles per sample

  bool mHasPendingReset = false;
  double mResetWhen01 = 0.0;
};

// ============================================================================
// 2) Band-limit policy seam (edge → correction)
//    - Step edges: use polyBLEP
//    - Slope edges: use polyBLAMP
//    - Both use a tiny "spill" to next sample (carry buffer)
// ============================================================================

enum class EdgeKind
{
  Step,
  Slope
};  // Step: value jump; Slope: derivative jump

struct EdgeEvent
{
  double whenInSample01 = 0.0f;  // 0..1, time of discontinuity within THIS sample
  EdgeKind kind = EdgeKind::Step;
  float magnitude = 0.0f;
  // For Step:   magnitude == step height (e.g., ±2 for square/saw).
  // For Slope:  magnitude == change in dY/dPhase at the corner (per cycle).
  //             (Sink will multiply by phaseDelta01PerSample to convert to dY/dSample.)
};

struct EdgeSink
{
  virtual ~EdgeSink() = default;
  virtual void beginSample(double phaseDelta01PerSample) = 0;  // prepare per-sample state
  virtual void addEdge(const EdgeEvent& e) = 0;                // schedule an edge this sample
  virtual double takeCorrectionAndFinishSample() = 0;          // sum tails now; spill the rest
};

// --- No band-limiting (for LFO / debugging) ---
class NoBandlimitSink final : public EdgeSink
{
public:
  void beginSample(double) override
  { /* no-op */
  }
  void addEdge(const EdgeEvent&) override
  { /* no-op */
  }
  double takeCorrectionAndFinishSample() override
  {
    return 0.0;
  }
};

// --- PolyBLEP / PolyBLAMP band-limiting ---
// Two-sample polynomial kernels with spillover into next sample.
// References: widely used forms in open-source softsynths.
//
// Conventions:
//   - dt := phaseDelta01PerSample (width of kernel in sample space).
//   - t  := position of the discontinuity within the current sample [0,1].
//   - We evaluate a portion NOW (t in [0,1]) and precompute the spill that will
//     be needed NEXT sample (by evaluating with t+1).
class PolyBlepBlampSink final : public EdgeSink
{
public:
  void beginSample(double phaseDelta01PerSample) override
  {
    // Pull in spill from previous sample, reset accumulators, and latch dt.
    mSumThisSample = mSpillFromPrevSample;
    mSpillFromPrevSample = 0.0f;
    mSpillToNextSample = 0.0f;
    // Limit dt to [tiny, 1] so polynomial windows behave.
    mDt = std::min(1.0, std::max(phaseDelta01PerSample, 1e-6));
  }

  void addEdge(const EdgeEvent& e) override
  {
    const double t = math::clamp01(e.whenInSample01);
    const double dt = mDt;

    switch (e.kind)
    {
      case EdgeKind::Step:
      {
        const float mag = e.magnitude;

        // Contribution in the current sample:
        mSumThisSample += mag * polyBLEP(t, dt);

        // Spill for the next sample (evaluate with t+1, which maps into [0,1] for next frame):
        mSpillToNextSample += mag * polyBLEP(t + 1.0f, dt);
      }
      break;

      case EdgeKind::Slope:
      {
        // Convert slope jump per-cycle to per-sample by multiplying by dt.
        // (dy/dPhase) * (dPhase/dSample) = (dy/dPhase) * dt
        const double magPerSample = e.magnitude * dt;

        mSumThisSample += magPerSample * polyBLAMP(t, dt);
        mSpillToNextSample += magPerSample * polyBLAMP(t + 1.0f, dt);
      }
      break;
    }
  }

  double takeCorrectionAndFinishSample() override
  {
    const double out = mSumThisSample;
    // Queue spill for the next beginSample()
    mSpillFromPrevSample = mSpillToNextSample;
    // Clear this-sample sum
    mSumThisSample = 0.0f;
    mSpillToNextSample = 0.0f;
    return out;
  }

private:
  // polyBLEP for a step discontinuity
  static inline double polyBLEP(double t, double dt)
  {
    // Map to [0,1] window(s). Two lobes exist: near 0 and near 1.
    if (t < 0.0f || t > 1.0f)
    {
      // when called with (t+1), map back into [0,1]
      t = math::wrap01(t);
    }

    if (t < dt)
    {
      const double x = t / dt;
      // x + x - x^2 - 1  (smoothly cancels a step over width dt)
      return x + x - x * x - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
      const double x = (t - 1.0f) / dt;
      // x^2 + x + 1 (mirror lobe near end of sample)
      return x * x + x + 1.0f;
    }
    return 0.0f;
  }

  // polyBLAMP for a slope discontinuity (integrated polyBLEP)
  static inline double polyBLAMP(double t, double dt)
  {
    static constexpr double oneSixth = 1.0f / 6.0f;
    static constexpr double oneThird = 1.0f / 3.0f;
    if (t < 0.0f || t > 1.0f)
    {
      t = math::wrap01(t);
    }

    if (t < dt)
    {
      const double x = t / dt;
      // x^2 * (x/3 - 1/2)
      return x * x * (x * oneThird - 0.5f);
    }
    else if (t > 1.0f - dt)
    {
      const double x = (t - 1.0f) / dt;
      // x^2 * (x/3 + 1/2) + 1/6
      return x * x * (x * oneThird + 0.5f) + oneSixth;
    }
    return 0.0f;
  }

  double mDt = 0.0f;
  double mSumThisSample = 0.0f;
  double mSpillFromPrevSample = 0.0f;
  double mSpillToNextSample = 0.0f;
};

// ============================================================================
// 3) Waveform cores (pure shape + where their edges are)
//    - Cores render a NAÏVE sample and emit EdgeEvent(s) for discontinuities.
//    - They DO NOT do band-limiting; that’s the EdgeSink’s job.
// ============================================================================

struct CoreSample
{
  float naive = 0.0f;
};

struct IWaveformCore
{
  virtual ~IWaveformCore() = default;
  virtual CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& step,
                                             EdgeSink& edgeSink,
                                             double optionalPhaseOffset01 /*PM, slow*/) = 0;
  virtual void SetParams(float shapeA, float shapeB) = 0;
};

// --- Sine (no edges) ---
class SineCore final : public IWaveformCore
{
public:
  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& s, EdgeSink& edges, double phaseOffset01) override
  {
    (void)edges;                                                        // no edges
    const float p = (float)math::wrap01(s.phaseEnd01 + phaseOffset01);  // sample at end with optional slow PM
    return {std::sinf(2.0f * float(M_PI) * p)};
  }
  void SetParams(float shapeA, float shapeB) override
  {
    (void)shapeA;
    (void)shapeB;  // no params
  }
};

// --- Sawtooth (one value step at wrap or reset) ---
class SawCore final : public IWaveformCore
{
public:
  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& s, EdgeSink& edges, double phaseOffset01) override
  {
    edges.beginSample(s.phaseDelta01PerSample);

    // Each wrap/reset produces a step from near +1 to -1 => step magnitude ~ -2
    for (int i = 0; i < s.eventCount; ++i)
    {
      edges.addEdge({s.events[i].whenInSample01, EdgeKind::Step, -2.0f});
    }

    return {sampleSawAt((float)math::wrap01(s.phaseEnd01 + phaseOffset01))};
  }
  void SetParams(float shapeA, float shapeB) override
  {
    (void)shapeA;
    (void)shapeB;  // no params
  }

private:
  static inline float sampleSawAt(float phase01) noexcept
  {
    return 2.0f * phase01 - 1.0f;  // [-1, +1]
  }
};

// --- Square (PWM) with edges at duty crossing and at wrap/reset ---
class SquareCore final : public IWaveformCore
{
public:
  void setDuty(float d)
  {
    mDuty01 = std::clamp(d, 0.001f, 0.999f);
  }
  void SetParams(float shapeA, float shapeB) override
  {
    setDuty(shapeA);  // shapeA = duty cycle [0,1]
    (void)shapeB;     // shapeB unused
  }

  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& s, EdgeSink& edges, double phaseOffset01) override
  {
    edges.beginSample(s.phaseDelta01PerSample);

    const double pStart = math::wrap01(s.phaseStart01 + phaseOffset01);
    const double pEnd = math::wrap01(s.phaseEnd01 + phaseOffset01);

    // 1) Edge at PWM duty crossing inside the sample?
    double tCross = 0.0f;
    if (findPhaseCrossingFraction(pStart, pEnd, mDuty01, tCross))
    {
      const double before = sampleSquareAt(nextToward(mDuty01, 0.0f), mDuty01);
      const double after = sampleSquareAt(mDuty01, mDuty01);
      if (before != after)
      {
        edges.addEdge({tCross, EdgeKind::Step, float(after - before)});  // typically ±2
      }
    }

    // 2) Edges at wrap / reset if they flip the square
    for (int i = 0; i < s.eventCount; ++i)
    {
      const double before = sampleSquareAt(nextToward(1.0f, 0.0f), mDuty01);  // value immediately before event
      const double after = sampleSquareAt(0.0f, mDuty01);                     // immediately after
      if (before != after)
      {
        edges.addEdge({s.events[i].whenInSample01, EdgeKind::Step, float(after - before)});
      }
    }

    return {sampleSquareAt(pEnd, mDuty01)};
  }

private:
  float mDuty01 = 0.5f;

  static inline float sampleSquareAt(double phase01, double duty01) noexcept
  {
    return (phase01 < duty01) ? +1.0f : -1.0f;
  }

  static inline float nextToward(float x, float toward) noexcept
  {
    // small epsilon step toward target (used to sample "before" a discontinuity)
    const float eps = 1e-6f;
    return (x > toward) ? (x - eps) : (x + eps);
  }

  // Detect whether [pStart->pEnd] (possibly wrapping) crosses a target phase,
  // and return the fractional time t in [0,1] if so.
  static bool findPhaseCrossingFraction(double pStart, double pEnd, double target, double& outT)
  {
    double a = pStart, b = pEnd;
    if (b < a)
      b += 1.0f;  // unwrap the interval
    double e = target;
    if (e < a)
      e += 1.0f;
    if (e >= a && e < b)
    {
      outT = (e - a) / (b - a);
      return true;
    }
    return false;
  }
};


// SquareCore that band-limits internally (polyBLEP), for debugging.
// - Does NOT emit edges to EdgeSink.
// - Keeps its own 1-sample spill buffer.
// - Assumes at most one wrap + one reset per sample (same as PhaseAccumulator).
class SquareCoreInlineBLEP final : public IWaveformCore
{
public:
  void setDuty(float d)
  {
    mDuty01 = std::clamp(d, 0.001f, 0.999f);
  }

  void SetParams(float shapeA, float shapeB) override
  {
    setDuty(shapeA);  // shapeA = duty cycle [0,1]
    (void)shapeB;     // shapeB unused
  }

  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& step, EdgeSink& /*unusedSink*/, double phaseOffset01) override
  {
    // Per-sample setup
    const double dt = std::min(1.0, std::max(step.phaseDelta01PerSample, 1e-6));

    // Pull spill from previous sample into the correction sum; clear next spill accumulator
    double correctionNow = mSpillFromPrev;
    mSpillFromPrev = 0.0f;
    double spillToNext = 0.0f;

    // Phase at sample start/end with slow PM applied (sampling offset)
    const double pStart = math::wrap01(step.phaseStart01 + phaseOffset01);
    const double pEnd = math::wrap01(step.phaseEnd01 + phaseOffset01);

    // 1) PWM duty crossing inside this sample? -> step at tCross with magnitude after-before (±2)
    double tCross = 0.0f;
    if (findPhaseCrossingFraction(pStart, pEnd, mDuty01, tCross))
    {
      const double before = sampleSquareAt(nextToward(mDuty01, 0.0f), mDuty01);
      const double after = sampleSquareAt(mDuty01, mDuty01);
      const double stepMag = (after - before);  // typically ±2
      applyPolyBLEP(stepMag, tCross, dt, correctionNow, spillToNext);
    }

    // 2) Wrap/reset events: step occurs if value flips across the event
    for (int i = 0; i < step.eventCount; ++i)
    {
      const double t = step.events[i].whenInSample01;

      // Value just BEFORE the event (phase ~ 1^- with offset), and just AFTER (phase = 0 with offset)
      const double before = sampleSquareAt(nextToward(1.0f, 0.0f), mDuty01);  // p ≈ 1^-
      const double after = sampleSquareAt(0.0f, mDuty01);                     // p = 0
      if (before != after)
      {
        const double stepMag = (after - before);  // ±2 depending on duty
        applyPolyBLEP(stepMag, t, dt, correctionNow, spillToNext);
      }
    }

    // Naïve sample at end, plus our internally computed correction
    const double naive = sampleSquareAt(pEnd, mDuty01);
    const double y = naive + correctionNow;

    // Commit spill for next sample
    mSpillFromPrev = spillToNext;

    return {(float)y};
  }

private:
  double mDuty01 = 0.5f;
  double mSpillFromPrev = 0.0f;  // 1-sample spill buffer

  // --- Helpers (localized to this core) ---

  static inline double sampleSquareAt(double phase01, double duty01) noexcept
  {
    return (phase01 < duty01) ? +1.0f : -1.0f;
  }

  static inline double nextToward(double x, double toward) noexcept
  {
    const double eps = 1e-6f;
    return (x > toward) ? (x - eps) : (x + eps);
  }

  // Detect crossing of target phase within [pStart -> pEnd] accounting for wrap.
  // Returns t in [0,1] if crossed.
  static bool findPhaseCrossingFraction(double pStart, double pEnd, double target, double& outT)
  {
    double a = pStart, b = pEnd;
    if (b < a)
      b += 1.0f;  // unwrap interval
    double e = target;
    if (e < a)
      e += 1.0f;
    if (e >= a && e < b)
    {
      outT = (e - a) / (b - a);
      return true;
    }
    return false;
  }

  // 2-sample polyBLEP window, expressed with (t, dt).
  // Adds "now" portion to correctionNow, and queues "late lobe" into spillToNext.
  static inline void applyPolyBLEP(double magnitude, double t, double dt, double& correctionNow, double& spillToNext)
  {
    // current-sample contribution
    correctionNow += magnitude * polyBLEP(t, dt);
    // next-sample spill (evaluate the mirrored lobe by shifting t -> t+1)
    spillToNext += magnitude * polyBLEP(t + 1.0f, dt);
  }

  static inline double polyBLEP(double t, double dt)
  {
    // Map to [0,1]; handle the "late" lobe case (t+1) by wrapping
    t = math::wrap01(t);

    if (t < dt)
    {
      const double x = t / dt;
      // x + x - x^2 - 1
      return x + x - x * x - 1.0f;
    }
    else if (t > 1.0f - dt)
    {
      const double x = (t - 1.0f) / dt;
      // x^2 + x + 1
      return x * x + x + 1.0f;
    }
    return 0.0f;
  }
};


// SawCore that band-limits internally (polyBLEP), for debugging.
// - Does NOT emit edges to EdgeSink.
// - Owns a 1-sample spill buffer.
// - ASSUMPTION: at most one read-phase wrap and at most one external reset in a sample.
//   (Same constraint as PhaseAccumulator: phaseDelta01PerSample <= 1)

class SawCoreInlineBLEP final : public IWaveformCore
{
public:
  void SetParams(float shapeA, float shapeB) override
  {
    (void)shapeA;
    (void)shapeB;  // shapeB unused
  }

  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& step,
                                     EdgeSink& /*unusedSink*/,
                                     double slowPhaseOffset01) override
  {
    // --- Per-sample setup ----------------------------------------------------
    const double dt = std::min(1.0, std::max(step.phaseDelta01PerSample, 1e-6));

    // Bring in last frame's late lobe; prep next spill bucket.
    double correctionNow = mSpillFromPrevSample;
    mSpillFromPrevSample = 0.0f;
    double spillToNext = 0.0f;

    // Read-phase (sampling) trajectory with slow PM/offset applied.
    // IMPORTANT: We use the read-phase to locate the discontinuity time; placing
    // the BLEP at the *read* discontinuity prevents mis-timed lobes (spikes).
    const double readStart = math::wrap01(step.phaseStart01 + slowPhaseOffset01);
    const double readEnd = math::wrap01(step.phaseEnd01 + slowPhaseOffset01);

    // --- 1) Discontinuity from read-phase wrap (i.e., sampling crosses 1→0) ---
    // If the read-phase decreases modulo 1 (end < start), there was a wrap
    // somewhere inside this sample; compute its fractional time and step size.
    if (readEnd < readStart)
    {
      const double tWrap = (1.0 - readStart) / (dt + 1e-30);         // when read-phase hits 1.0
      const double valueBefore = sampleSawAt(nextToward(1.0, 0.0));  // ≈ +1
      const double valueAfter = sampleSawAt(0.0);                     // = -1
      const double stepMagnitude = (valueAfter - valueBefore);         // typically -2
      applyPolyBLEP(stepMagnitude, tWrap, dt, correctionNow, spillToNext);
    }

    // --- 2) Discontinuities from explicit hard resets (PhaseEventKind::Reset) ---
    // Even if there was no read-phase wrap, a hard reset forces a jump in the read signal.
    for (int i = 0; i < step.eventCount; ++i)
    {
      if (step.events[i].kind != PhaseEventKind::Reset)
        continue;

      const double t = step.events[i].whenInSample01;

      // Phase *just before* reset: advance readStart by dt*t (unwrapped), then wrap for sampling.
      const double readBefore = math::wrap01(readStart + dt * t);
      const double valueBefore = sampleSawAt(nextToward(readBefore, 1.0f));  // safe "just before"
      const double valueAfter = sampleSawAt(math::wrap01(0.0 + slowPhaseOffset01));
      const double stepMagnitude = (valueAfter - valueBefore);

      applyPolyBLEP(stepMagnitude, t, dt, correctionNow, spillToNext);
    }

    // --- Naïve sample plus our internal correction ---------------------------
    const double naive = sampleSawAt(readEnd);
    const double y = naive + correctionNow;

    // Commit spill for next sample
    mSpillFromPrevSample = spillToNext;

    return {(float)y};
  }

private:
  double mSpillFromPrevSample = 0.0f;  // one-sample late-lobe storage

  // Saw mapping [-1,+1] from normalized phase
  static inline double sampleSawAt(double phase01) noexcept
  {
    return 2.0f * phase01 - 1.0f;
  }

  static inline double nextToward(double x, double toward) noexcept
  {
    // small epsilon to safely evaluate "just before" or "just after"
    const double eps = 1e-6;
    return (x > toward) ? (x - eps) : (x + eps);
  }

  // Apply 2-sample polyBLEP around discontinuity at time t in [0,1].
  // Adds "now" portion to correctionNow and queues "late lobe" to spillToNext.
  static inline void applyPolyBLEP(double stepMagnitude, double t, double dt, double& correctionNow, double& spillToNext)
  {
    // Current-sample portion (early lobe)
    correctionNow += stepMagnitude * polyBLEP(t, dt);
    // Next-sample portion (late lobe). Using t+1 maps the window into next frame.
    spillToNext += stepMagnitude * polyBLEP(t + 1, dt);
  }

  // Standard 2-lobe polyBLEP (piecewise polynomial), parameterized by event time t and width dt.
  static inline double polyBLEP(double t, double dt)
  {
    // Wrap t to [0,1] so that (t+1) also lands in-range for the late lobe.
    t = math::wrap01(t);

    if (t < dt)
    {
      const double x = t / dt;
      // early lobe: x + x - x^2 - 1
      return x + x - x * x - 1;
    }
    else if (t > 1.0f - dt)
    {
      const double x = (t - 1) / dt;
      // late lobe: x^2 + x + 1
      return x * x + x + 1;
    }
    return 0.0f;
  }
};


// --- Triangle (slope changes → BLAMP) ---
class TriangleCore final : public IWaveformCore
{
public:
  void SetParams(float shapeA, float shapeB) override
  {
    (void)shapeA;
    (void)shapeB;  // no params
  }

  CoreSample renderNaiveAndEmitEdges(const PhaseAdvance& s, EdgeSink& edges, double phaseOffset01) override
  {
    edges.beginSample(s.phaseDelta01PerSample);

    // Slope wrt phase is +4 for phase<0.5, -4 for phase>=0.5.
    // Corners at 0.5 and at wrap/reset: slope jump magnitude = ±8 (per cycle).
    const double pStart = math::wrap01(s.phaseStart01 + phaseOffset01);
    const double pEnd = math::wrap01(s.phaseEnd01 + phaseOffset01);

    double tCorner = 0.0f;
    if (crossesHalf(pStart, pEnd, tCorner))
    {
      edges.addEdge({tCorner, EdgeKind::Slope, -8.0f});  // +4 -> -4 jump
    }
    for (int i = 0; i < s.eventCount; ++i)
    {
      edges.addEdge({s.events[i].whenInSample01, EdgeKind::Slope, +8.0f});  // -4 -> +4 at wrap/reset
    }

    return {sampleTriangleAt(pEnd)};
  }

private:
  static inline float sampleTriangleAt(double phase01) noexcept
  {
    // 4*|p-0.5|-1 maps [0,1) → [-1,1] with corners at 0,0.5
    return 4.0f * (float)std::fabs(phase01 - 0.5) - 1.0f;
  }
  static bool crossesHalf(double pStart, double pEnd, double& outT)
  {
    double a = pStart, b = pEnd;
    if (b < a)
      b += 1.0f;
    const double e = (0.5f < a) ? (0.5f + 1.0f) : 0.5f;
    if (e >= a && e < b)
    {
      outT = (e - a) / (b - a);
      return true;
    }
    return false;
  }
};

// ============================================================================
// 4) Oscillator wrapper (musical node)
//    - Owns phase, a waveform core, and an EdgeSink.
//    - Handles audio-rate FM feedback and optional slow PM (phase offset).
//    - LFO mode: swap EdgeSink to NoBandlimitSink and keep full-scale outputs.
// ============================================================================
//
//enum class OscMode
//{
//  Audio,
//  LFO
//};

//class OscillatorNode
//{
//public:
//  void setSampleRate(float sr)
//  {
//    mPhase.setSampleRate(sr);
//  }
//  void setMode(OscMode m)
//  {
//    mMode = m;
//  }
//  void setCore(std::unique_ptr<IWaveformCore> c)
//  {
//    mCore = std::move(c);
//  }
//  void setEdgeSink(std::unique_ptr<EdgeSink> s)
//  {
//    mSink = std::move(s);
//  }
//  void setPhase01(float p)
//  {
//    mPhase.setPhase01(p);
//  }
//  float previousOutput() const
//  {
//    return mPrevOut;
//  }
//
//  // Render one sample.
//  // Inputs:
//  //   - baseHz: frequency after all pitch math (>= ~0.001 Hz)
//  //   - fmFeedbackAmt: scales 'previous output' injected as FM (phase increment perturbation)
//  //   - slowPhaseOffset01: a slow PM/phase offset applied at sampling (not re-timing edges)
//  //   - optional hard sync: set doHardSync + whenInSample01
//  float renderOneSample(float baseHz,
//                        float fmFeedbackAmt,
//                        float slowPhaseOffset01,
//                        bool doHardSync,
//                        float syncWhenInSample01)
//  {
//    if (!mCore || !mSink)
//      return 0.0f;
//
//    // Convert base frequency to normalized phase delta per sample.
//    const float sr = std::max(1.0f, mSampleRateCache);
//    const float baseDelta01 = std::max(baseHz / sr, 0.001f / sr);
//
//    // Simple FM feedback: perturb the increment by previous output.
//    float dPhase01 = baseDelta01 + fmFeedbackAmt * mPrevOut;
//
//    // If desired, clamp to [0,1] to honor the engine's "one wrap max" assumption.
//    dPhase01 = std::min(dPhase01, 1.0f);
//
//    mPhase.setPhaseDelta01(dPhase01);
//    if (doHardSync)
//      mPhase.scheduleResetAtFraction(clamp01(syncWhenInSample01));
//
//    const PhaseAdvance adv = mPhase.advanceOneSample();
//
//    // Let the core emit edges and the sink turn them into correction
//    mSink->beginSample(adv.phaseDelta01PerSample);
//    const CoreSample cs = mCore->renderNaiveAndEmitEdges(adv, *mSink, slowPhaseOffset01);
//
//    float y = cs.naive;
//    if (mMode == OscMode::Audio)
//    {
//      y += mSink->takeCorrectionAndFinishSample();
//    }
//    else
//    {
//      (void)mSink->takeCorrectionAndFinishSample();  // discard
//      // LFOs should already be full-scale by construction of cores.
//    }
//
//    mPrevOut = y;
//    return y;
//  }
//
//  void setInternalSampleRateCache(float sr)
//  {
//    mSampleRateCache = sr;
//  }
//
//private:
//  PhaseAccumulator mPhase;
//  std::unique_ptr<IWaveformCore> mCore;
//  std::unique_ptr<EdgeSink> mSink;
//  OscMode mMode = OscMode::Audio;
//
//  float mPrevOut = 0.0f;
//  float mSampleRateCache = 48000.0f;
//};
//
//// ============================================================================
//// 5) Example of integrating with your renderer
////    (Sketch: show how the node slots into your ProcessSampleForAudio.
////     Keep your param plumbing; only replace the "generate one sample" bit.)
//// ============================================================================
//
//// Example glue (names adapted to your snippet):
//struct MyOscGlue
//{
//  OscillatorNode node;  // owns phase/core/sink
//  float currentFreqHz = 440.0f;
//  float fmPrevOut = 0.0f;
//
//  // call once at init:
//  void init(float sampleRate, bool forAudio, bool useTriangle)
//  {
//    node.setInternalSampleRateCache(sampleRate);
//    node.setSampleRate(sampleRate);
//    node.setMode(forAudio ? OscMode::Audio : OscMode::LFO);
//
//    if (!useTriangle)
//    {
//      node.setCore(std::make_unique<SawCore>());  // try SquareCore, SineCore, TriangleCore
//    }
//    else
//    {
//      node.setCore(std::make_unique<TriangleCore>());
//    }
//
//    if (forAudio)
//    {
//      node.setEdgeSink(std::make_unique<PolyBlepBlampSink>());
//    }
//    else
//    {
//      node.setEdgeSink(std::make_unique<NoBandlimitSink>());
//    }
//  }
//
//  // one-sample render (replace your "RenderAudioSampleAndAdvance" body)
//  float render(float freqHz, float fmFeedbackAmt, float slowPhaseOffset01, bool hardSyncNow, float hardSyncWhen01)
//  {
//    node.setInternalSampleRateCache(/* your SR */ 48000.0f);
//    const float y = node.renderOneSample(freqHz, fmFeedbackAmt, slowPhaseOffset01, hardSyncNow, hardSyncWhen01);
//    fmPrevOut = y;
//    return y;
//  }
//};

/////////////////////////////////////////////////////////////////////////////

struct KRateRecalculator
{
  // when 0, run.
  size_t mNSamplesElapsed = 0;
  void Invalidate()
  {
    mNSamplesElapsed = 0;
  }
  template <typename T>
  void visit(T run)
  {
    if (mNSamplesElapsed == 0)
    {
      run();
    }
    mNSamplesElapsed = (mNSamplesElapsed + 1) & GetModulationRecalcSampleMask();
  }
};

struct OscillatorNode : ISoundSourceDevice::Voice
{
private:
  OscillatorDevice* mpOscDevice = nullptr;
  OscillatorIntention mIntention = OscillatorIntention::Audio;
  PhaseAccumulator mPhase;
  PhaseAccumulator mSyncPhase;

  std::unique_ptr<IWaveformCore> mCore;
  std::unique_ptr<EdgeSink> mSink;

  KRateRecalculator mKRateRecalc;
  float mPreviousSample = 0;
  float mCurrentFrequencyHz = 0;
  float mFMFeedbackAmt = 0.0f;

  static float CalculateImmediateFrequency()
  {
    //
  }

public:
  OscillatorNode(OscillatorDevice* pOscDevice,
                 OscillatorIntention intention,
                 ModMatrixNode* pModMatrix,
                 EnvelopeNode* pAmpEnv)
      : ISoundSourceDevice::Voice(pOscDevice, pModMatrix, pAmpEnv)
      , mpOscDevice(pOscDevice)
      , mIntention(intention)
  {
    if (mIntention == OscillatorIntention::Audio)
    {
      //mCore = std::make_unique<SawCoreInlineBLEP>();
      mCore = std::make_unique<SawCore>();
      //mCore = std::make_unique<SquareCoreInlineBLEP>();
      //mCore = std::make_unique<SquareCore>();

      mSink = std::make_unique<PolyBlepBlampSink>();
      //mSink = std::make_unique<NoBandlimitSink>();
    }
    else
    {
      mCore = std::make_unique<SineCore>();
      mSink = std::make_unique<NoBandlimitSink>();
    }
  }

  // return phase in [0,1)
  double GetPhase01() const
  {
    return mPhase.GetPhase01();
  }

  // used by LFOs to just hard-set the phase. usually NOP
  void SynchronizePhase(const OscillatorNode& src)
  {
    mPhase.setPhase01(src.GetPhase01());
    //mpSlaveWave->mPhase = mPhase;
  }

  // Returns the theoretical phase offset (not a live cursor).
  // not 100% certain if this is the best way to express this.
  float GetPhaseOffset() const
  {
    float phaseModVal = 0.0f;
    size_t phaseOffsetParamOffset = 0;
    switch (mpOscDevice->mIntention)
    {
      case OscillatorIntention::Audio:
        phaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                       (int)OscModParamIndexOffsets::Phase);
        phaseOffsetParamOffset = (size_t)OscParamIndexOffsets::PhaseOffset;
        break;
      case OscillatorIntention::LFO:
        phaseOffsetParamOffset = (size_t)LFOParamIndexOffsets::PhaseOffset;
      default:
        return 0.0f;
    }
    float f = mpOscDevice->mParams.GetN11Value(phaseOffsetParamOffset, phaseModVal);
    return math::fract(f);
  }

  // Required for
  // - outputting to alternate streams
  // - making the FM source values available to other oscillators
  float GetLastSample() const
  {
    return mPreviousSample;
  }

  // for debug only, alternate streams, expose how many samples since last recalculation of k-rate params.
  size_t GetSamplesSinceRecalc() const
  {
    return mKRateRecalc.mNSamplesElapsed;
  }

  virtual void NoteOn(bool legato) override
  {
    if (legato)
      return;
    if (mpOscDevice->GetPhaseRestart())
    {
      mPhase.setPhase01(0);
    }
  }
  virtual void NoteOff() override {}

  virtual void BeginBlock() override
  {
    if (!mpSrcDevice->IsEnabled())
    {
      return;
    }
    mKRateRecalc.Invalidate();
    auto w = mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(mpOscDevice->mIntention == OscillatorIntention::Audio
                                                                       ? (int)OscParamIndexOffsets::Waveform
                                                                       : (int)LFOParamIndexOffsets::Waveform);
    //mpSlaveWave = mpWaveforms[math::ClampI((int)w, 0, (int)OscillatorWaveform::Count - 1)];
  }

  float RenderSampleForAudioAndAdvancePhase(real_t midiNote,
                                            float detuneFreqMul,
                                            float fmScale,
                                            const ParamAccessor& globalParams,
                                            float const* otherSignals,
                                            int iosc,
                                            float ampEnvLin)
  {
    const ParamAccessor& params = mpSrcDevice->mParams;
    auto modDestBaseId = mpSrcDevice->mModDestBaseID;
    if (!mpSrcDevice->mEnabledCache)
    {
      mPreviousSample = 0.0f;
      return 0;
    }

    mKRateRecalc.visit(
        [&]()
        {
          // Cache some k-rate modulated params
          // establish:
          // - frequency
          // - sync frequency
          // - waveshapes
          bool syncEnable = mpOscDevice->mParams.GetBoolValue(OscParamIndexOffsets::SyncEnable);

          float syncFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                  (int)OscModParamIndexOffsets::SyncFrequency);
          float freqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                              (int)OscModParamIndexOffsets::FrequencyParam);
          float waveShapeAModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::WaveshapeA);
          float waveShapeBModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::WaveshapeB);
          //float phaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
          //                                                      (int)OscModParamIndexOffsets::Phase);
          float pitchFineModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                   (int)OscModParamIndexOffsets::PitchFine);
          float FMFeedbackModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::FMFeedback);
          mFMFeedbackAmt = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::FMFeedback, FMFeedbackModVal) *
                           fmScale * 0.5f;

          float waveshapeA = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeA, waveShapeAModVal);
          float waveshapeB = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeB, waveShapeBModVal);

          mCore->SetParams(waveshapeA, waveshapeB);

          // - osc pitch semis                  note         oscillator
          // - osc fine (semis)                 note         oscillator
          // - osc sync freq / kt (Hz)          hz           oscillator
          // - osc freq / kt (Hz)               hz           oscillator
          // - osc mul (Hz)                     hz           oscillator
          // - osc detune (semis)               hz+semis     oscillator
          // - unisono detune (semis)           hz+semis     oscillator
          int pitchSemis = params.GetIntValue(OscParamIndexOffsets::PitchSemis, gSourcePitchSemisRange);
          float pitchFine = params.GetN11Value(OscParamIndexOffsets::PitchFine, pitchFineModVal) *
                            gSourcePitchFineRangeSemis;
          midiNote +=
              pitchSemis +
              pitchFine;  // (mpSrcDevice->mPitchFineParam.mCachedVal + mPitchFineModVal)* gSourcePitchFineRangeSemis;
          float noteHz = math::MIDINoteToFreq(midiNote);
          float freq =
              params.GetFrequency(OscParamIndexOffsets::FrequencyParam,
                                  OscParamIndexOffsets::FrequencyParamKT,
                                  gSourceFreqConfig,
                                  noteHz,
                                  freqModVal);  //mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
          freq *=
              mpOscDevice->mParams.GetScaledRealValue(OscParamIndexOffsets::FreqMul,
                                                      0.0f,
                                                      gFrequencyMulMax,
                                                      0);  //mpOscDevice->mFrequencyMul.mCachedVal;// .GetRangedValue();
          freq *= detuneFreqMul;
          // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
          freq = std::max(freq, 0.0001f);
          mCurrentFrequencyHz = freq;

          if (syncEnable)
          {
            float syncFreq = params.GetFrequency(OscParamIndexOffsets::SyncFrequency,
                                                 OscParamIndexOffsets::SyncFrequencyKT,
                                                 gSyncFreqConfig,
                                                 noteHz,
                                                 syncFreqModVal);
            syncFreq = std::max(syncFreq, 0.0001f);
            mSyncPhase.setPhaseStepFromFrequencyHz(syncFreq);
            mPhase.setPhaseStepFromFrequencyHz(freq);
          }
          else
          {
            mPhase.setPhaseStepFromFrequencyHz(freq);
          }
        });

    // Calculate immediate phase modulation from feedback and other oscillators.
    // immediatePhaseMod = mPrevSample * FMFeedbackAmt + sum(otherOscs * theirAmt)
    auto calcImmediatePhaseMod = [&]()
    {
      float previousSample = mPreviousSample;
      float immediatePhaseMod = mPreviousSample * mFMFeedbackAmt;
      int otherIndex = -1;
      for (int i = 0; i < gOscillatorCount - 1; ++i)
      {
        int off = iosc * (gOscillatorCount - 1) + i;
        ParamIndices amtParam = ParamIndices((int)ParamIndices::FMAmt2to1 + off);
        ModDestination amtMod = ModDestination((int)ModDestination::FMAmt2to1 + off);
        float amt = globalParams.Get01Value(amtParam, mpModMatrix->GetDestinationValue(amtMod));

        otherIndex += (i == iosc) ? 2 : 1;
        immediatePhaseMod += otherSignals[otherIndex] * amt * fmScale;
      }
      return immediatePhaseMod;
    };

    auto immediatePhaseMod = calcImmediatePhaseMod();

    mPhase.setPhaseStepFromFrequencyHz(mCurrentFrequencyHz, immediatePhaseMod);

    const PhaseAdvance adv = mPhase.advanceOneSample();

    // Let the core emit edges and the sink turn them into correction
    mSink->beginSample(adv.phaseDelta01PerSample);
    const CoreSample cs = mCore->renderNaiveAndEmitEdges(adv, *mSink, GetPhaseOffset());

    float y = cs.naive;
    y -= (float)mSink->takeCorrectionAndFinishSample();

    //y = math::clampN11(y);  // prevent FM from going crazy.
    mPreviousSample = y;
    return y * ampEnvLin;
  }

  // render one sample for LFO, advance phase.
  // forceSilence skips rendering the wave but still advances phase
  float RenderSampleForLFOAndAdvancePhase(bool forceSilence)
  {
    auto modDestBaseId = mpSrcDevice->mModDestBaseID;
    //float phaseModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::Phase);

    mKRateRecalc.visit(
        [&]()
        {
          float freqModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::FrequencyParam);
          float waveShapeAModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::WaveshapeA);
          float waveShapeBModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::WaveshapeB);

          float freq = mpSrcDevice->mParams.GetFrequency(LFOParamIndexOffsets::FrequencyParam,
                                                         -1,  // no keytracking for lfo
                                                         gLFOFreqConfig,
                                                         0,  // note hz (no keytracking)
                                                         freqModVal);
          // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
          freq = std::max(freq, 0.001f);

          float waveshapeA = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeA, waveShapeAModVal);
          float waveshapeB = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeB, waveShapeBModVal);
          mCore->SetParams(waveshapeA, waveshapeB);

          mPhase.setPhaseStepFromFrequencyHz(freq);
        });

    const PhaseAdvance adv = mPhase.advanceOneSample();
    if (forceSilence)
    {
      // still need to advance phase, but skip rendering the wave.
      mPreviousSample = 0.0f;
      return 0.0f;
    }
    const CoreSample cs = mCore->renderNaiveAndEmitEdges(adv, *mSink, GetPhaseOffset());
    float y = cs.naive;
    mPreviousSample = y;
    return y;
  }

};  // OscillatorNode


}  // namespace M7


}  // namespace WaveSabreCore
