#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"

namespace WaveSabreCore
{
namespace M7
{

// line-based waveform segment boundary description
struct SegBoundary
{
  double phase01;
  float dAmp;
  float
      dSlope;  // slope is dy/dx where x is phase in [0,1) (saw wave slope = 2 because it goes from -1 to +1 in one cycle = 2/1 = 2)
};

// line-based waveform shape description expressed in deltas
struct CumShape
{
  float mAmpAt0;    // PRE edge value at 0 (must be consistent with the end of the wave cycle as defined by edges)
  float mSlopeAt0;  // PRE edge slope at 0 (must be consistent with the end of the wave cycle as defined by edges)
  std::vector<SegBoundary> mEdges;

  // todo: precalc absolute edge values for lerp eval?

  // Evaluate naive amplitude and slope at canonical phase p ∈ [0,1)
  inline std::pair<float, float> EvalAmpSlopeAt(double sampleInPhase01) const
  {
    float amp = mAmpAt0;  // post-edge value at 0
    float slope = mSlopeAt0;
    double prev = 0.0;

    for (const auto& e : mEdges)
    {
      if (sampleInPhase01 < e.phase01)
      {
        amp += slope * float(sampleInPhase01 - prev);
        return {amp, slope};
      }
      amp += slope * float(e.phase01 - prev);
      amp += e.dAmp;
      slope += e.dSlope;
      prev = e.phase01;
    }

    amp += slope * float(sampleInPhase01 - prev);
    return {amp, slope};
  }

  // phase distance from "from" to "to", wrapping around 1.0.
  // IOW, how far do you have to go forward from "from" to reach "to";
  // this can never be more than 1.0.
  static inline double ForwardDistance01(double from, double to)
  {
    return (to >= from) ? (to - from) : (1.0 - from + to);
  }

  // Visit all edges that lie on the path starting at phiStartRender01, advancing by pathDelta.
  template <class F>
  inline void VisitEdgesOnPath(double phiStartRender01,  // start of the render window, in phase units [0,1)
                               double pathDelta,         // distance of the path to traverse, in phase units [0,1)
                               double alpha,             // whenInSample01 at start of path [0,1)
                               double deltaTotal,        // total phase delta of the sample [0,1)
                               F&& onEdge                // void(double whenInSample01, float dAmp, float dSlope)
  ) const
  {
    if (pathDelta <= 0.0 || deltaTotal <= 0.0)
      return;

    for (const auto& e : mEdges)
    {
      const double eRender = e.phase01;
      // distance from start of render window to edge, in [0,1)
      const double d = ForwardDistance01(phiStartRender01, eRender);

      static constexpr double kTol = 1e-8;

      if (d + kTol >= pathDelta)
        continue;  // push exact end-of-window edges to next sample

      double u = alpha + d / deltaTotal;  // whenInSample01 in [0,1)
      onEdge(u, e.dAmp, e.dSlope);
    }
  }
};

struct IBlepExecutor
{
  // for debugging only
  double mCorrectionAmt = 1;

  // Open a new sample for accumulation.
  virtual void OpenSample(double sampleWindowSizeInPhase01) = 0;

  // Accumulate a discontinuity edge that lies within the current sample.
  // u is whenInSample01 ∈ [0,1)
  virtual void AccumulateEdge(double u, float dAmp, float dSlopePerPhase) = 0;

  // Close the current sample and get the correction to apply to the naive amplitude.
  // returns {blepCorrection, naiveAmplitude + blepCorrection}
  virtual std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) = 0;
};

struct PolyBlepBlampExecutor1 : IBlepExecutor
{
  template <typename T>
  static inline T BlepBefore(T x)  // before discontinuity
  {
    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
    return x * x;
  }

  template <typename T>
  static inline T BlepAfter(T x)
  {
    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
    x = 1 - x;
    return -x * x;
  }

  static inline void AddPolyBLEP(float u, float dAmp, double& now, double& next)
  {
    const float t = 1.0f - static_cast<float>(u);
    now += dAmp * BlepBefore(t);
    next += dAmp * BlepAfter(t);
  }

  template <typename T>
  static inline T BlampBefore(T x)  // before discontinuity
  {
    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
    static constexpr T OneThird = T{1} / T{3};
    return x * x * x * OneThird;
  }

  template <typename T>
  static inline T BlampAfter(T x)
  {
    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
    static constexpr T NegOneThird = T{-1} / T{3};
    x = x - 1;
    return NegOneThird * x * x * x;
  }

  static inline void AddPolyBLAMP(float u, float dSlope, double& now, double& next)
  {
    float t = 1.0f - static_cast<float>(u);
    now += dSlope * BlampBefore(t);
    next += dSlope * BlampAfter(t);
  }

  // Spill buffers (carry-over tails)
  double mNow = 0.0;   // applies to *this* sample
  double mNext = 0.0;  // applies to next sample

  double mSampleWindowSizeInPhase01 = 0.0;  // == step.phaseDelta01PerSample

  void OpenSample(double sampleWindowSizeInPhase01) override
  {
    mSampleWindowSizeInPhase01 = sampleWindowSizeInPhase01;

    // Bring down tails
    mNow = mNext;
    mNext = 0;
  }

  // u is whenInSample01 ∈ [0,1)
  void AccumulateEdge(double u, float dAmp, float dSlopePerPhase) override
  {
    if (dAmp != 0.0f)
    {
      AddPolyBLEP((float)u, dAmp, mNow, mNext);  // impl todo
    }
    if (dSlopePerPhase != 0.0f)
    {
      const double dSlope_perSample = dSlopePerPhase * mSampleWindowSizeInPhase01;
      AddPolyBLAMP((float)u, (float)dSlope_perSample, mNow, mNext);  // impl todo
    }
  }

  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
  {
    return {mNow, naiveAmplitude + mNow * mCorrectionAmt};
  }
};

//
//struct PolyBlepBlampExecutor2 : IBlepExecutor
//{
//  // t = fraction AFTER the edge within the current sample, t = 1 - u, u in [0,1)
//  // BLEP: current + next
//  static inline float BlepAfter(float t)
//  {  // current sample
//    const float t2 = t * t;
//    return t * t2 - 0.5f * t2 * t2;  // t^3 - 1/2 t^4
//  }
//  static inline float BlepBefore(float t)
//  {  // next sample
//    const float mt = 1.0f - t;
//    const float mt2 = mt * mt;
//    return -(mt * mt2) + 0.5f * mt2 * mt2;  // -(1-t)^3 + 1/2 (1-t)^4
//  }
//
//  static inline void AddPolyBLEP(float u, float dAmp, double& now, double& next)
//  {
//    const float t = 1.0f - static_cast<float>(u);
//    now += dAmp * BlepAfter(t);
//    next += dAmp * BlepBefore(t);
//  }
//
//  // BLAMP: current + next + next+1 (for slope jumps)
//  static inline float BlampAfter(float t)
//  {  // current sample
//    const float d = t, d2 = d * d, d3 = d2 * d, d4 = d3 * d, d5 = d4 * d;
//    return (-d5) * (1.0f / 40.0f) + d4 * (1.0f / 24.0f) + d3 * (1.0f / 12.0f) + d2 * (1.0f / 12.0f) +
//           d * (1.0f / 24.0f) + (1.0f / 120.0f);
//  }
//  static inline float BlampBefore_1(float t)
//  {  // next sample
//    const float d = t, d2 = d * d, d3 = d2 * d, d4 = d3 * d, d5 = d4 * d;
//    (void)d3;
//    return d5 * (1.0f / 40.0f) - d4 * (1.0f / 12.0f) + d2 * (1.0f / 3.0f) - d * (1.0f / 2.0f) + (7.0f / 30.0f);
//  }
//  static inline float BlampBefore_2(float t)
//  {  // next+1 sample
//    const float d = t, d2 = d * d, d3 = d2 * d, d4 = d3 * d, d5 = d4 * d;
//    return (-d5) * (1.0f / 120.0f) + d4 * (1.0f / 24.0f) - d3 * (1.0f / 12.0f) + d2 * (1.0f / 12.0f) -
//           d * (1.0f / 24.0f) + (1.0f / 120.0f);
//  }
//
//  static inline void AddPolyBLAMP(float u, float dSlope, double& now, double& next, double& next2)
//  {
//    float t = 1.0f - static_cast<float>(u);
//    now += dSlope * BlampAfter(t);
//    next += dSlope * BlampBefore_1(t);
//    next2 += dSlope * BlampBefore_2(t);
//  }
//
//  // Spill buffers (carry-over tails)
//  double mNow = 0.0;    // applies to *this* sample
//  double mNext = 0.0;   // applies to next sample
//  double mNext2 = 0.0;  // applies to next+1 sample (BLAMP only)
//
//  double mSampleWindowSizeInPhase01 = 0.0;  // == step.phaseDelta01PerSample
//
//  // for debugging only
//  double mCorrectionAmt = 1;
//
//  void OpenSample(double sampleWindowSizeInPhase01) override
//  {
//    mSampleWindowSizeInPhase01 = sampleWindowSizeInPhase01;
//
//    // Bring down tails
//    mNow = mNext;
//    mNext = mNext2;
//    mNext2 = 0.0;
//  }
//
//  // u is whenInSample01 ∈ [0,1)
//  void AccumulateEdge(double u, float dAmp, float dSlopePerPhase) override
//  {
//    if (dAmp != 0.0f)
//    {
//      AddPolyBLEP((float)u, dAmp, mNow, mNext);
//    }
//    if (dSlopePerPhase != 0.0f)
//    {
//      const double dSlope_perSample = dSlopePerPhase * mSampleWindowSizeInPhase01;
//      AddPolyBLAMP((float)u, (float)dSlope_perSample, mNow, mNext, mNext2);
//    }
//  }
//
//  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
//  {
//    return {mNow, naiveAmplitude + mNow * mCorrectionAmt};
//  }
//};
//
//
//struct PolyBlepBlampExecutor3 : IBlepExecutor
//{
//  static inline void AddPolyBLEP(float u, float dAmp, double& now, double& next)
//  {
//    //u = 1.0f - u;
//    const float u2 = u * u;
//    // current sample
//    now += dAmp * (u - 0.5f * u2 - 0.5f);
//    // next sample
//    next += dAmp * (0.5f * u2);
//  }
//
//  static inline void AddPolyBLAMP(float u, float dSlopePerSample, double& now, double& next, double& next2)
//  {
//    //u = 1.0f - u;
//    const float u2 = u * u;
//    const float u3 = u2 * u;
//    // current sample
//    now += dSlopePerSample * ((1.0f / 6.0f) * u3 - 0.5f * u2 + 0.5f * u - (1.0f / 6.0f));
//    // next sample
//    next += dSlopePerSample * (0.5f * u2 - 0.5f * u + (1.0f / 6.0f));
//    // next+1 sample
//    next2 += dSlopePerSample * (-(1.0f / 6.0f) * u3);
//  }
//
//  // Spill buffers (carry-over tails)
//  double mNow = 0.0;    // applies to *this* sample
//  double mNext = 0.0;   // applies to next sample
//  double mNext2 = 0.0;  // applies to next+1 sample (BLAMP only)
//
//  double mSampleWindowSizeInPhase01 = 0.0;  // == step.phaseDelta01PerSample
//
//  // for debugging only
//  double mCorrectionAmt = 1;
//
//  void OpenSample(double sampleWindowSizeInPhase01) override
//  {
//    mSampleWindowSizeInPhase01 = sampleWindowSizeInPhase01;
//
//    // Bring down tails
//    mNow = mNext;
//    mNext = mNext2;
//    mNext2 = 0.0;
//  }
//
//  // u is whenInSample01 ∈ [0,1)
//  void AccumulateEdge(double u, float dAmp, float dSlopePerPhase) override
//  {
//    if (dAmp != 0.0f)
//    {
//      AddPolyBLEP((float)u, dAmp, mNow, mNext);
//    }
//    if (dSlopePerPhase != 0.0f)
//    {
//      const double dSlope_perSample = dSlopePerPhase * mSampleWindowSizeInPhase01;
//      AddPolyBLAMP((float)u, (float)dSlope_perSample, mNow, mNext, mNext2);
//    }
//  }
//
//  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
//  {
//    return {mNow, naiveAmplitude + mNow * mCorrectionAmt};
//  }
//};
//

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NullBlepBlampExecutor : IBlepExecutor
{
  void OpenSample(double /*sampleWindowSizeInPhase01*/) override {}
  // u is whenInSample01 ∈ [0,1)
  void AccumulateEdge(double /*u*/, float /*dAmp*/, float /*dSlopePerPhase*/) override {}
  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
  {
    return {0.0, naiveAmplitude};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TBlepExecutor, OscillatorWaveform waveformType>
struct BandLimitedOscillatorCore : public OscillatorCore
{
protected:
  BandLimitedOscillatorCore(float correctionAmt /* for debugging */)
      : OscillatorCore(waveformType)
      , mCorrectionAmt(correctionAmt)
  {
  }

public:
  static_assert(std::is_base_of<IBlepExecutor, TBlepExecutor>::value, "TBlepExecutor must implement IBlepExecutor");
  TBlepExecutor mHelper;
  float mCorrectionAmt;  // debugging only

  virtual CumShape GetCumWaveDesc() = 0;

  virtual void SetCorrectionFactor(float factor) override
  {
    mHelper.mCorrectionAmt = factor * mCorrectionAmt;
  }

  void HandleParamsChanged() override
  {
    SetCorrectionFactor(mWaveshapeB);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseAdvance step = mPhaseAcc.advanceOneSample();
    const CumShape cs = GetCumWaveDesc();

    mHelper.OpenSample(step.lengthInPhase01);

    // Naive sample: evaluate at render-space begin-phase
    const double sampleStartInPhase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const auto [amp0, /*slope0*/ _] = cs.EvalAmpSlopeAt(sampleStartInPhase01);
    const float naive = amp0;

    auto addEdge = [&](double u, float dAmp, float dSlope)
    {
      mHelper.AccumulateEdge(u, dAmp, dSlope);
    };

    // Check for an in-sample hard-sync reset
    bool hasReset = false;
    double resetPosInSample01 = 0.0;  // when in this sample a reset happens [0,1)
    for (int i = 0; i < step.eventCount; ++i)
    {
      if (step.events[i].kind == PhaseEventKind::Reset)
      {
        hasReset = true;
        resetPosInSample01 = step.events[i].whenInSample01;
        break;  // at most one reset per sample by design
      }
    }

    const auto sampleWindowSizeInPhase01 = step.lengthInPhase01;

    if (!hasReset)
    {
      // Single continuous path: start at phi0Render, advance by delta
      cs.VisitEdgesOnPath(sampleStartInPhase01,       // path begin
                          sampleWindowSizeInPhase01,  // path length
                          0.0,                        // whenInSample01 at start of path = beginning of sample
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);
    }
    else
    {
      // 1) Pre-reset path: [0 .. tReset)
      const double preResetLengthInPhase01 = resetPosInSample01 *
                                             sampleWindowSizeInPhase01;  // distance from start to reset, in phase units
      cs.VisitEdgesOnPath(sampleStartInPhase01,                          // path begin
                          preResetLengthInPhase01,                       // path length = distance from start to reset
                          0.0,                        // whenInSample01 at start of path = beginning of sample
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);

      // 2) Insert BLEP/BLAMP at the reset instant for the *teleport* to phase 0
      const double resetPosInPhase01 = math::wrap01(sampleStartInPhase01 +
                                                    preResetLengthInPhase01);  // just before reset
      const auto [ampBefore, slopeBefore] = cs.EvalAmpSlopeAt(resetPosInPhase01);

      // after reset, we're at phase 0 + phase offset.
      const auto [ampAfter, slopeAfter] = cs.EvalAmpSlopeAt(math::wrap01(audioRatePhaseOffset));  // 0+ with offset

      const float dAmp = ampAfter - ampBefore;
      const float dSlope = slopeAfter - slopeBefore;

      addEdge(resetPosInSample01, dAmp, dSlope);

      // 3) Post-reset path: [tR .. 1)
      const double postResetLengthInPhase01 = (1.0 - resetPosInSample01) * sampleWindowSizeInPhase01;
      const double phiAfterResetRender = math::wrap01(audioRatePhaseOffset);  // start at 0+ (render space)
      cs.VisitEdgesOnPath(phiAfterResetRender,                                // path begin = 0+ with offset
                          postResetLengthInPhase01,   // path length = distance from reset to end of sample
                          resetPosInSample01,         // whenInSample01 at start of path = reset time
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);
    }

    const auto [correction, amplitude] = mHelper.CloseSampleAndGetCorrection(naive);

    return CoreSample{.amplitude = (float)amplitude,
                      .naive = naive,
                      .correction = (float)correction,  //
                      .phaseAdvance = step};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The idea here is to describe the waveform as a series of segments. The band limited parent class should
// be able to use this description to render the waveform with proper bandlimiting.
template <typename TBlepExecutor, OscillatorWaveform Twf>
struct PWMCoreT : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
{
  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
  PWMCoreT()
      : Base(1)
  {
  }

  double mDutyCycle01 = 0.5f;  // 0..1

  void HandleParamsChanged() override
  {
    mDutyCycle01 = std::clamp(this->mWaveshapeA, 0.001f, 0.999f);
    Base::HandleParamsChanged();
  }

  CumShape GetCumWaveDesc() override
  {
    CumShape cs{.mAmpAt0 = -1, .mSlopeAt0 = 0};
    cs.mEdges.reserve(2);
    cs.mEdges.push_back(SegBoundary{.phase01 = 0.0, .dAmp = +2.0f, .dSlope = 0.0f});
    cs.mEdges.push_back(SegBoundary{.phase01 = mDutyCycle01, .dAmp = -2.0f, .dSlope = 0.0f});
    return cs;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TBlepExecutor, OscillatorWaveform Twf>
struct SawCore : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
{
  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
  SawCore()
      : Base(1)
  {
  }


  CumShape GetCumWaveDesc() override
  {
    CumShape cs{.mAmpAt0 = 1, .mSlopeAt0 = 2};
    cs.mEdges.reserve(1);
    cs.mEdges.push_back(SegBoundary{.phase01 = 0.0, .dAmp = -2.0f, .dSlope = 0});
    return cs;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
template <typename TBlepExecutor, OscillatorWaveform Twf>
struct TriTruncCore : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
{
  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
  TriTruncCore()
      : Base(1)
  {
  }
  CumShape GetCumWaveDesc() override
  {
    CumShape cs{.mAmpAt0 = -1, .mSlopeAt0 = -4};
    cs.mEdges.reserve(1);
    cs.mEdges.push_back(SegBoundary{.phase01 = 0.0, .dAmp = 0, .dSlope = +8});
    cs.mEdges.push_back(SegBoundary{.phase01 = 0.5, .dAmp = 0, .dSlope = -8});
    return cs;
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
