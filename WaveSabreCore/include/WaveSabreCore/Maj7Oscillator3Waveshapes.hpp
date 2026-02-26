#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "BandSplitter.hpp"
#include "LinkwitzRileyFilter.hpp"
#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Shape.hpp"
#include "Maj7Oscillator4WS.hpp"
#include "SVFilter.hpp"

namespace WaveSabreCore
{
namespace M7
{
  static inline WVSegment SegmentFromEndpoints(double beginPhase01, double endPhaseIncluding1, float beginAmp, float endAmp)
  {
    double dPhase = endPhaseIncluding1 - beginPhase01;
    if (dPhase <= 0)
      dPhase += 1.0;  // wrap around

    float slope = (endAmp - beginAmp) / float(dPhase);
    return WVSegment{beginPhase01, endPhaseIncluding1, beginAmp, slope};
  }
// idleLow01 = amount of wave cycle spent holding at -1 at the beginning of the cycle (0..1)
// rampUp01 = amount of wave cycle spent ramping up from -1 to +1 (0..1)
// idleHigh01 = amount of wave cycle spent holding at +1
// rampDown01 = amount of wave cycle spent ramping down from +1 to -1 (0..1)
// the 4 stages are expected to add up to 1.
static inline WVShape MakeTrapezoidShape1(float idleLow01, float rampUp01, float idleHigh01, float rampDown01)
{ 
  return WVShape{.mSegments = {
                     SegmentFromEndpoints(0.0, idleLow01, -1.0f, -1.0f),
                     SegmentFromEndpoints(idleLow01, idleLow01 + rampUp01, -1.0f, +1.0f),
                     SegmentFromEndpoints(idleLow01 + rampUp01, idleLow01 + rampUp01 + idleHigh01, +1.0f, +1.0f),
                     SegmentFromEndpoints(idleLow01 + rampUp01 + idleHigh01, 1.0, +1.0f, -1.0f),
                 }};
}

// accepts 2 shape 0..1 parameters and maps them to the 4 raw params.
static inline WVShape MakeSawShape(float idleParam01, float rampUpDownParam01)
{
  // when idleParam01 = 0.5 (center), then there should be no idle.
  // idleParam < 0.5 -> more idle at low level; idleParam > 0.5 -> more idle at high level
  // idleParam = 0 -> idleLow01 = 1, idleHigh01 = 0; idleParam = 1 -> idleLow01 = 0, idleHigh01 = 1
  float idleLow01 = std::clamp((0.5f - idleParam01) * 2, 0.0f, 0.995f); // allow 0 duration
  float idleHigh01 = std::clamp((idleParam01 - 0.5f) * 2, 0.0f, 0.995f); // allow 0 duration

  float remaining = 1.0f - idleLow01 - idleHigh01; // remaining cycle portion for the ramps

  // rampUpDownParam01 = saw -> tri map. 0 = saw (full stage = ramp up); 1 = triangle (50% ramp up, 50% ramp down)
  float rampUp01 = rampUpDownParam01 * remaining;
  float rampDown01 = (1.0f - rampUpDownParam01) * remaining;
  return MakeTrapezoidShape1(idleLow01, rampUp01, idleHigh01, rampDown01);
}

static inline WVShape MakeTriangleShape()
{
  return MakeTrapezoidShape1(0.0f, 0.5f, 0.0f, 0.5f);
}
static inline WVShape MakePulseShape(double dutyCycle01)
{
  dutyCycle01 = std::clamp(dutyCycle01, 0.005, 0.995);
  return MakeTrapezoidShape1(dutyCycle01, 0.0f, 1.0f - dutyCycle01, 0.0f);
}

// three state pulse: low, high, 0
// segment 0: low
// segment 1: high
// segment 2: 0
static inline WVShape MakeTriStatePulseShape3(double masterDutyCycle01, double subDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(subDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  auto highDutyPhase = masterDutyCycle01 - lowDutyPhase;

  return WVShape{
      .mSegments = {
          WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
          WVSegment{
              .beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 1.0f, .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 0.f, .slope = 0},
      }};
}


// four state pulse: low, mid, high, mid
// segment 0: low
// segment 1: mid (transition up)
// segment 2: high (@ duty cycle)
// segment 3: mid (transition down)
// one way to think of the segment lengths is that this is a pulse wave within a pulse wave.
static inline WVShape MakeTriStatePulseShape4(double masterDutyCycle01, double lowDuty01, double highDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(lowDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  // scale high duty to the master duty cycle
  auto mainSeg2 = 1.0 - masterDutyCycle01;  // length of the high segment area
  auto highDutyPhase = std::clamp(highDuty01 * mainSeg2, 0.0, mainSeg2 - 0.001);

  return WVShape{
      .mSegments = {
          WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
          WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 0.f, .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01,
                    .endPhaseIncluding1 = masterDutyCycle01 + highDutyPhase,
                    .beginAmp = 1.0f,
                    .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01 + highDutyPhase,
                    .endPhaseIncluding1 = 1.0,
                    .beginAmp = .0f,
                    .slope = 0},
      }};
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// you can use 2 params at a time, but this supports 4.
// - DC asymmetry
// - clipping
// - silence fraction
// - harmonic blend (0..1) between pure sine and windowed sine (1.5*sin*(0.5-0.5*cos))
enum class SineCoreExtVariant
{
  DCClip,
  ClipSilence,
  ClipHarm,
  HarmSilence,
};
template <SineCoreExtVariant Variant>
struct SineCoreExt : public OscillatorCore
{
  // A: dc asym (-1..+1), B: clip (0..1), C: silence frac (0..1), H: harmonic blend (0..1)
  float mA = 0.0f, mB = 0.0f, mC = 0.0f, mH = 0.0f;

  // Derived for clipping
  float mClipThreshold = 1.0f;
  float mClipGain = 1.0f;
  bool mSquareMode = false;
  static constexpr float kSquareEps = 1e-4f;

  SineCoreExt(OscillatorWaveform wf)
      : OscillatorCore(wf)
  {
  }

  void HandleParamsChanged() override
  {
    mA = 0;
    mB = 0;
    mC = 0;
    mH = 0;
    switch (Variant)
    {
      case SineCoreExtVariant::DCClip:
      {
        mA = math::clampN11(mWaveshapeB * 2 - 1);
        mB = math::clamp01(mWaveshapeA);
        break;
      }
      case SineCoreExtVariant::ClipSilence:
      {
        mB = std::abs(mWaveshapeA);
        mC = std::abs(mWaveshapeB);
        break;
      }
      case SineCoreExtVariant::ClipHarm:
      {
        mB = std::abs(mWaveshapeA);
        mH = math::clamp01(mWaveshapeB);
        break;
      }
      case SineCoreExtVariant::HarmSilence:
      {
        mC = math::clamp01(mWaveshapeA);
        mH = math::clamp01(mWaveshapeB);
        break;
      }
    }

    if (mB >= 1.0f - kSquareEps)
    {
      mSquareMode = true;
      mClipThreshold = 1.0f;
      mClipGain = 1.0f;
    }
    else
    {
      mSquareMode = false;
      mClipThreshold = std::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;
    }
  }

  inline float ClipAndNormalize(float x) const
  {
    if (mSquareMode)
      return (x > 0.0f) ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    const float y = std::clamp(x, -mClipThreshold, +mClipThreshold);
    return y * mClipGain;
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample();  // no offset inside accumulator
    const double p = step.phaseBegin01;
    const double off = audioRatePhaseOffset;  // offset is in cycles [0,1) here
    const double actW = 1.0 - (double)mC;     // active window width

    float out = 0.0f;

    if (mC < 1.0f && p < actW)
    {
      // Compress the active part to [0,1), then apply offset and get angle
      const double phi = std::fmod((p / actW) + off, 1.0);
      const float theta = float(phi * math::gPITimes2);

      // Base sine
      float s = std::sin(theta);

      // Harmonic-blended variant:
      //   y_h = 1.5 * s * (0.5 - 0.5*cos(theta))   (your original)
      // which equals 0.75*sin(theta) - 0.375*sin(2*theta)
      float yH;
      {
        // choose whichever is cheaper on your platform:

        // (A) window form (needs cos)
        //float c = std::cos(theta);
        //yH = 1.5f * s * (0.5f - 0.5f * c);

        // // (B) identity form (needs sin 2θ)
        float s2 = std::sin(2.0f * theta);
        yH = 0.75f * s - 0.375f * s2;
      }

      // Blend: H=0 → pure sine, H=1 → windowed/harmonic version
      float yPreClip = s * (1.0f - mH) + yH * mH;

      // DC asymmetry BEFORE clipping (biases clip for asymmetrical distortion)
      yPreClip += mA;

      // Hard clip with makeup gain (or square if B≈1)
      out = ClipAndNormalize(yPreClip);
    }
    // else: in silence tail or full-cycle silence → out stays 0

    return CoreSample{
        .amplitude = out,
        .naive = out,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAHNoiseCore : public OscillatorCore
{
  // Param B: Jitter amount in [0,1]
  float mJitter01 = 0.0f;

  // Jitter range in phase units (fraction of a cycle)
  static constexpr double kJitterMaxPhase = 0.45;  // ±45% of a cycle at B=1

  // Internal clock phasor and current cycle length (in phase units)
  double mClockPhase01 = 0.0;    // advances by dt each sample
  double mCycleThreshold = 1.0;  // = 1.0 + jitter per cycle

  // Output state: held value between steps
  float mHeld = 0.0f;

  // BLEP spill
  M7Osc4::CorrectionSpill mSpill;

  SAHNoiseCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  void HandleParamsChanged() override
  {
    // Map your UI param: A unused (slew removed), B = jitter 0..1
    mJitter01 = std::clamp(mWaveshapeB, 0.0f, 1.0f);
  }


  // Only sets the next cycle length (no target here).
  inline void schedule_next_cycle_length()
  {
    const double J = (double)mJitter01 * kJitterMaxPhase;
    const double jitter = J * math::randN11();  // [-J, +J]
    mCycleThreshold = 1.0 + jitter;
  }


  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    const double dt = step.dt;

    // First-time init
    if (mClockPhase01 == 0.0 && mCycleThreshold == 1.0)
    {
      schedule_next_cycle_length();
      mHeld = math::randN11();  // start value
    }

    // 1) Open BLEP spill for THIS sample
    mSpill.open_sample();

    // 2) SNAPSHOT the held value at the **start** of the sample
    const float heldAtStart = mHeld;

    // 3) March through possible intra-sample events
    double remaining = dt;
    double consumed = 0.0;

    while (true)
    {
      const double toEdge = mCycleThreshold - mClockPhase01;

      if (remaining < toEdge)
      {
        mClockPhase01 += remaining;
        break;
      }

      // Edge happens inside this sample
      const double alphaFull = (dt > 0.0) ? std::clamp((consumed + toEdge) / dt, 0.0, 1.0) : 0.0;

      // Compute new target and true step size from the **current** held value
      const float newTarget = math::randN11();
      const double dAmp = (double)newTarget - (double)mHeld;

      // Band-limit the step at alphaFull
      //add_blep(alphaFull, dAmp, mSpill.now, mSpill.next);
      mSpill.add_edge(alphaFull, dAmp, 0, dt);

      // Commit the step to state (affects later edges in THIS sample and the next sample)
      mHeld = newTarget;

      // Consume up to the edge, reset phasor, schedule next cycle length
      remaining -= toEdge;
      consumed += toEdge;
      mClockPhase01 = 0.0;
      schedule_next_cycle_length();
    }

    // 4) Output = value at sample **start** + now-tap
    const float y = heldAtStart + (float)mSpill.now;

    return CoreSample{
        .amplitude = y,
        .naive = heldAtStart,  // optional: expose pre-correction
        .correction = (float)mSpill.now,
        .phaseAdvance = step,
    };
  }
};


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // Assumes you already have: add_blep(double alpha01, double dAmp, double& now, double& next)
// //
// //struct CorrectionSpill {
// //  double now  = 0.0;  // THIS sample
// //  double next = 0.0;  // NEXT sample
// //  inline void open_sample() { now = next; next = 0.0; }
// //};

// struct GaussianNoise
// {
//   // Gaussian RNG (Box-Muller)
//   uint32_t mRng = 0x853C49E6748FEA9Bull & 0xffffffffu;
//   inline uint32_t rng_u32()
//   {
//     uint32_t x = mRng;
//     x ^= x << 13;
//     x ^= x >> 17;
//     x ^= x << 5;
//     mRng = (x != 0 ? x : 0xA3C59AC3u);
//     return mRng;
//   }
//   inline double rng01()
//   {
//     return (double)rng_u32() * (1.0 / 4294967295.0);  // [0,1]
//   }
//   inline double rng_pm1()
//   {
//     return 2.0 * rng01() - 1.0;  // [-1,1]
//   }
//   inline double rng_gauss()
//   {                                        // Box-Muller (one sample)
//     double u1 = std::max(rng01(), 1e-12);  // avoid log(0)
//     double u2 = rng01();
//     return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * math::gPITimes2 * u2);  // mean 0, var 1
//   }
// };

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// struct ParticleNoiseCore : public OscillatorCore
// {
//   // --- Params (map these from your UI / mWaveshapeA/B/C) ---
//   // A: density per cycle (events/cycle), log-mapped
//   // B: decay time (ms) for each micro-pulse
//   // C: amp scale (0..1), overall particle strength
//   float mDensity01 = 0.5f;  // 0..1 -> ~[0.01 .. 50] events/cycle
//   float mDecay01 = 0.3f;    // 0..1 -> ~[1ms .. 300ms]
//   //float mAmp01     = 0.5f;   // 0..1 gain on particle amplitude

//   // --- Derived (k-rate) ---
//   double mLambdaPhase = 1.0;  // events per phase unit (per cycle)
//   double mTauSec = 0.050;     // seconds
//   double mA_one = 0.0;        // per-sample decay factor a = exp(-Ts/tau)

//   // --- Event scheduler (phase domain) ---
//   double mPhaseToNext = 1.0;  // remaining phase until next event

//   // --- Signal state (value at sample boundaries, pre-correction) ---
//   double mState = 0.0;  // sum of active exponential pulses at sample start

//   // --- BLEP spill ---
//   M7Osc4::CorrectionSpill mSpill;

//   // --- RNG (xorshift32) ---
//   uint32_t mRng = 0x853C49E6748FEA9Bull & 0xffffffffu;
//   inline uint32_t rng_u32()
//   {
//     uint32_t x = mRng;
//     x ^= x << 13;
//     x ^= x >> 17;
//     x ^= x << 5;
//     mRng = (x != 0 ? x : 0xA3C59AC3u);
//     return mRng;
//   }
//   inline double rng01()
//   {
//     return (double)rng_u32() * (1.0 / 4294967295.0);  // [0,1]
//   }
//   inline double rng_pm1()
//   {
//     return 2.0 * rng01() - 1.0;  // [-1,1]
//   }
//   inline double rng_gauss()
//   {                                        // Box-Muller (one sample)
//     double u1 = std::max(rng01(), 1e-12);  // avoid log(0)
//     double u2 = rng01();
//     return std::sqrt(-2.0 * std::log(u1)) * std::cos(2.0 * math::gPITimes2 * u2);  // mean 0, var 1
//   }
//   inline double exp_interarrival_phase(double lambdaPhase)
//   {
//     // Draw exponential in phase units with mean 1/lambda
//     // If lambda ~ 0, return a very large number.
//     if (lambdaPhase <= 1e-9)
//       return 1e9;
//     double u = std::max(rng01(), 1e-12);
//     return -std::log(u) / lambdaPhase;
//   }

//   ParticleNoiseCore(OscillatorWaveform wf)
//       : OscillatorCore(wf)
//   {
//   }

//   void HandleParamsChanged() override
//   {
//     // Map UI -> engine
//     // Density: log sweep ~ [0.01 .. 50] events per cycle
//     const double dMin = 0.01, dMax = 50.0;
//     mDensity01 = std::clamp(mWaveshapeA, 0.0f, 1.0f);
//     mLambdaPhase = dMin * std::pow(dMax / dMin, (double)mDensity01);

//     // Decay time: 1ms .. 300ms (exp mapping is nicer)
//     mDecay01 = std::clamp(mWaveshapeB, 0.0f, 1.0f);
//     const double tMin = 0.001, tMax = 0.300;
//     mTauSec = tMin * std::pow(tMax / tMin, (double)mDecay01);

//     // Amp scale 0..1 maps linearly
//     //mAmp01 = std::clamp(mWaveshapeC, 0.0f, 1.0f);

//     // Per-sample one-pole coefficient for continuous-time decay
//     const double Ts = 1.0 / std::max(1.0f, Helpers::CurrentSampleRateF);
//     mA_one = std::exp(-Ts / std::max(1e-6, mTauSec));
//   }

//   // Re-seed the scheduler if needed (call on note-on)
//   inline void Reset()
//   {
//     mState = 0.0;
//     mSpill.now = mSpill.next = 0.0;
//     mPhaseToNext = exp_interarrival_phase(mLambdaPhase);
//   }

//   CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
//   {
//     const auto step = mPhaseAcc.advanceOneSample();
//     const double dt_phase = step.dt;  // phase advanced this sample (∈ (0,1))

//     // First-run lazy init
//     if (mPhaseToNext > 1e8)
//     {
//       mPhaseToNext = exp_interarrival_phase(mLambdaPhase);
//     }

//     // Open BLEP spill for this sample and snapshot start-state
//     mSpill.open_sample();
//     double yStart = mState;

//     // For physically-correct intra-sample decay, we need fractional powers of 'a'
//     // Over a fraction 'g' of one sample, decay factor is a_one^g
//     auto a_pow = [&](double g) -> double
//     {
//       if (g <= 0.0)
//         return 1.0;
//       return std::exp(std::log(std::max(1e-12, mA_one)) * g);
//     };

//     // March through intra-sample events (could be >1)
//     double remaining = dt_phase;
//     double consumed = 0.0;
//     double ySeg = yStart;  // evolve state continuously within the sample

//     while (remaining >= mPhaseToNext)
//     {
//       // fraction of this sample until the event
//       const double alpha = (dt_phase > 0.0) ? (consumed + mPhaseToNext) / dt_phase : 0.0;

//       // Decay the running state up to the event time
//       const double g_to_edge = (dt_phase > 0.0) ? (mPhaseToNext / dt_phase) : 0.0;  // fraction of one sample
//       ySeg *= a_pow(g_to_edge);

//       // Draw particle amplitude (Gaussian sounds great; scale by mAmp01)
//       const double amp = rng_gauss();

//       // Bandlimit the step at the onset (value discontinuity of size 'amp')
//       //add_blep(alpha, amp, mSpill.now, mSpill.next);
//       mSpill.add_edge(alpha, amp, 0, dt_phase);

//       // Apply the instantaneous step to the continuous state for subsequent segments
//       ySeg += amp;

//       // Consume this event and schedule the next
//       remaining -= mPhaseToNext;
//       consumed += mPhaseToNext;
//       mPhaseToNext = exp_interarrival_phase(mLambdaPhase);
//     }

//     // Decay the remainder of the sample to compute end-of-sample state
//     const double g_tail = (dt_phase > 0.0) ? (remaining / dt_phase) : 0.0;
//     ySeg *= a_pow(g_tail);
//     mState = ySeg;              // next sample starts here
//     mPhaseToNext -= remaining;  // reduce time-to-next by what we just advanced

//     // Output = value at sample start + BLEP tail(s)
//     const float y = (float)(yStart + mSpill.now);

//     return CoreSample{
//         .amplitude = y,
//         .naive = (float)yStart,
//         .correction = (float)mSpill.now,
//         .phaseAdvance = step,
//     };
//   }
// };

// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// struct GrainNoiseCore : public OscillatorCore
// {
//   PhaseAccumulator mRefreshPhase;       // for every cycle of this phase, we "refresh" 1 sample of the noise buffer.
//   size_t mBufferAllocated = 0;          // how many samples mBuffer can hold.
//   size_t mBufferElementsPopulated = 0;  // how many samples in mBuffer have been populated with noise.
//   std::unique_ptr<float[]> mBuffer;
//   GaussianNoise mNoiseGen;

//   GrainNoiseCore(OscillatorWaveform wf)
//       : OscillatorCore(wf)
//   {
//   }

//   void HandleParamsChanged() override
//   {
//     // waveshape A = mWaveshapeA
//     //mVariancePerCycle = math::clamp01(mWaveshapeA);
//     float freqRatio = math::lerp(0.01f, 50.0f, math::clamp01(mWaveshapeA));
//     mRefreshPhase.setFrequencyHz(mMainFrequencyHz * freqRatio);
//   }

//   CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
//   {
//     const auto step = mPhaseAcc.advanceOneSample();
//     if (mMainFrequencyHz < 1e-3f || mMainFrequencyHz > Helpers::CurrentSampleRate * 0.5f)
//     {
//       return CoreSample{
//           .amplitude = 0.0f,
//           .phaseAdvance = step,
//       };
//     }

//     // 1. calculate # of samples in one cycle
//     const double cycleDurationSec = 1.0 / std::max(1.0f, mMainFrequencyHz);
//     const size_t samplesPerCycle = (size_t)std::ceil(cycleDurationSec * Helpers::CurrentSampleRateF);

//     // 2. ensure buffer is big enough, retaining existing data if possible
//     if (mBufferAllocated < samplesPerCycle)
//     {
//       // allocate a new buffer
//       std::unique_ptr<float[]> newBuffer = std::make_unique<float[]>(samplesPerCycle);
//       // copy existing data if possible
//       if (mBuffer)
//       {
//         size_t toCopy = std::min(mBufferElementsPopulated, samplesPerCycle);
//         std::memcpy(newBuffer.get(), mBuffer.get(), toCopy * sizeof(float));
//         mBufferElementsPopulated = toCopy;
//       }
//       mBuffer = std::move(newBuffer);
//       mBufferAllocated = samplesPerCycle;
//     }

//     // 3. popluate uninitialized samples
//     for (size_t i = mBufferElementsPopulated; i < samplesPerCycle; ++i)
//     {
//       mBuffer[i] = (float)mNoiseGen.rng_gauss();
//     }

//     // 4. "refresh" a portion of the buffer (samplesPerCycle * mVariancePerCycle) with new noise
//     // in order to accomplish this, we will shift the buffer left by that amount, and then fill the right side with new noise
//     size_t toRefresh = mRefreshPhase.advanceOneSampleReturningWrapsCrossed();
//     if (toRefresh > 0)
//     {
//       // shift left
//       size_t toKeep = samplesPerCycle - toRefresh;
//       if (toKeep > 0)
//       {
//         std::memmove(mBuffer.get(), mBuffer.get() + toRefresh, toKeep * sizeof(float));
//       }
//       // fill right side with new noise
//       for (size_t i = toKeep; i < samplesPerCycle; ++i)
//       {
//         mBuffer[i] = (float)mNoiseGen.rng_gauss();
//       }
//       mBufferElementsPopulated = samplesPerCycle;
//     }

//     // take the sample at the current phase position
//     const size_t index = (size_t)(step.phaseBegin01 * samplesPerCycle) % samplesPerCycle;
//     const float y = mBuffer[index];

//     return CoreSample{
//         .amplitude = y,
//         .phaseAdvance = step,
//     };
//   }
// };


// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// // multiplies white noise by a shape
// struct ShapeNoisePulseGenerator : public M7Osc4::IShapeGenerator
// {
//   WVShape GetShape(float shapeA, float /*shapeB*/) const override
//   {
//     return MakeTriangleShape();
//     //return MakePulseShape(shapeA);
//   }
// };

// struct ShapeNoiseCore : public OscillatorCore
// {
//   using CorrectionSpill = M7Osc4::CorrectionSpill;
//   using IShapeGenerator = M7Osc4::IShapeGenerator;
//   std::unique_ptr<IShapeGenerator> mShapeGen;
//   WVShape mShape;
//   GaussianNoise mNoiseGen;

//   ShapeNoiseCore(OscillatorWaveform wf)
//       : OscillatorCore(wf)
//       , mShapeGen(new ShapeNoisePulseGenerator)
//   {
//   }

//   void HandleParamsChanged() override
//   {
//     mShape = mShapeGen->GetShape(mWaveshapeA, mWaveshapeB);
//   };

//   CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//   {
//     const PhaseStep step = mPhaseAcc.advanceOneSample();  // no offset here
//     const double dt = step.dt;
//     const double phase = step.phaseBegin01;

//     // naive (evaluation) uses offset
//     const double evalPhase = math::wrap01(phase + audioRatePhaseOffset);
//     const auto [ampNaive, slopeNaive] = mShape.EvalAmpSlopeAt(evalPhase);
//     double y = (double)(ampNaive * .5 + .5) * mNoiseGen.rng_gauss();

//     return CoreSample{
//         .amplitude = (float)y,
//         .phaseAdvance = {/* if you still want to return step info, adapt here */},
//     };
//   }
// };

struct SilenceOsc : public OscillatorCore
{
  SilenceOsc(OscillatorWaveform waveform)
      : OscillatorCore(waveform)
  {
  }

  void HandleParamsChanged() override
  {
  };

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    return CoreSample{
        .amplitude = 0,
        .phaseAdvance = step,
    };
  }
};



struct FoldedSine : public OscillatorCore
{
  FoldedSine(bool triangle, OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
      , mTriangle(triangle)
  {
  }

  float mDrive = 1.0f;  // >= 1.0
  float mBias = 0.0f;   // DC offset before folding
  const float mTriangle;

  void HandleParamsChanged() override
  {
    mDrive = 1.0f + 10.0f * mWaveshapeA;

    // B: symmetry/bias 0..1 -> -1..1; actually in theory -2..2 is usable to catch ALL signal.
    mBias = (mWaveshapeB - 0.5f) * 4.0f;
  }

  static inline float fold_reflect(float x)
  {
    const float threshold = 1;
    const float ax = std::fabs(x);
    const float per = 2 * threshold;
    float m = std::fmod(ax, per);
    if (m < 0.0f)
      m += per;

    const float r = (m <= threshold) ? m : (per - m);
    return std::copysign(r, x);
  }

  inline float sampleWaveform(float phase01)
  {
    if (mTriangle)
    {
      return math::naiveTriangle01(phase01);
    }
    // sine
    return math::sin(math::gPITimes2 * phase01);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    const double phase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const float s = sampleWaveform(phase01);  // math::sin(math::gPITimes2 * float(phase01));

    const float driven = mDrive * s + mBias;
    float y = fold_reflect(driven);

    return CoreSample{
        .amplitude = y,
        .phaseAdvance = step,
    };
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
