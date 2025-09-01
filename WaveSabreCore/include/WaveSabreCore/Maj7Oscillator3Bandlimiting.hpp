#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Shape.hpp"

namespace WaveSabreCore
{
namespace M7
{

struct IBlepExecutor
{
  // for debugging only
  double mCorrectionAmt = 1;

  // Open a new sample for accumulation.
  virtual void OpenSample(double sampleWindowSizeInPhase01) = 0;

  // Accumulate a discontinuity edge that lies within the current sample.
  // u is whenInSample01 ∈ [0,1)
  virtual void AccumulateEdge(double edgePosInSample01,
                              float dAmp,
                              float dSlopePerPhase
  //double sampleLengthInPhase01,
#ifdef ENABLE_OSC_LOG
                              ,
                              std::string reason
#endif  // ENABLE_OSC_LOG
                              ) = 0;

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
    float uo = u;
    u = 1.0f - static_cast<float>(u);
    auto blepBefore = BlepBefore(u);
    auto blepAfter = BlepAfter(u);
#ifdef ENABLE_OSC_LOG
    gOscLog.Log(std::format(" -> blepnow({:.3f} {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
                            uo,
                            u,
                            blepBefore,
                            dAmp,
                            blepBefore * dAmp,
                            now,
                            now + blepBefore * dAmp));
    gOscLog.Log(std::format(" -> blepnext({:.3f} : {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
                            uo,
                            u,
                            blepAfter,
                            dAmp,
                            blepAfter * dAmp,
                            next,
                            next + blepAfter * dAmp));
#endif  // ENABLE_OSC_LOG

    now += dAmp * blepBefore;
    next += dAmp * BlepAfter(u);
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
    u = 1.0f - static_cast<float>(u);
    auto blampBefore = BlampBefore(u);
    auto blampAfter = BlampAfter(u);
#ifdef ENABLE_OSC_LOG
    gOscLog.Log(std::format(" -> blampnow :{:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
                            blampBefore,
                            dSlope,
                            blampBefore * dSlope,
                            now,
                            now + blampBefore * dSlope));
    gOscLog.Log(std::format(" -> blampnext:{:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
                            blampAfter,
                            dSlope,
                            blampAfter * dSlope,
                            next,
                            next + blampAfter * dSlope));
#endif  // ENABLE_OSC_LOG
    now += dSlope * blampBefore;
    next += dSlope * blampAfter;
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
  void AccumulateEdge(double edgePosInSample01,
                      float dAmp,
                      float dSlopePerPhase
#ifdef ENABLE_OSC_LOG
                      ,
                      std::string reason
#endif  // ENABLE_OSC_LOG
                      ) override
  {
    //const double u = edgePosInSample01;  // when in this sample [0,1)
    if (dAmp != 0.0f)
    {
#ifdef ENABLE_OSC_LOG
      gOscLog.Log(std::format(
          "add BLEP @ u={:.3f} dAmp={:.3f} dSlope={:.3f} ({})", edgePosInSample01, dAmp, dSlopePerPhase, reason));
#endif                                                           // ENABLE_OSC_LOG
      AddPolyBLEP((float)edgePosInSample01, dAmp, mNow, mNext);  // impl todo
    }
    if (dSlopePerPhase != 0.0f)
    {
#ifdef ENABLE_OSC_LOG
      gOscLog.Log(std::format(
          "add BLAMP @ u={:.3f} dAmp={:.3f} dSlope={:.3f} ({})", edgePosInSample01, dAmp, dSlopePerPhase, reason));
#endif  // ENABLE_OSC_LOG
      const double dSlope_perSample = dSlopePerPhase * mSampleWindowSizeInPhase01;
      AddPolyBLAMP((float)edgePosInSample01, (float)dSlope_perSample, mNow, mNext);  // impl todo
    }
  }

  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
  {
    return {mNow, naiveAmplitude + mNow * mCorrectionAmt};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct NullBlepBlampExecutor : IBlepExecutor
{
  void OpenSample(double /*sampleWindowSizeInPhase01*/) override {}
  // u is whenInSample01 ∈ [0,1)
  virtual void AccumulateEdge(double edgePosInSample01,
                              float dAmp,
                              float dSlopePerPhase
#ifdef ENABLE_OSC_LOG
                              ,
                              std::string reason
#endif  // ENABLE_OSC_LOG
                              ) override
  {
  }
  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude) override
  {
    return {0.0, naiveAmplitude};
  }
};

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
  bool mSkipZeroEdgeOnce = false;

  bool mWalkerInitialized = false;
  bool mShapeDirty = true;
  WVShape mShape;
  WVShape::Walker mWalker;

  virtual WVShape GetWaveDesc() = 0;

  virtual void SetCorrectionFactor(float factor) override
  {
    mHelper.mCorrectionAmt = factor * mCorrectionAmt;
  }

  void HandleParamsChanged() override
  {
    SetCorrectionFactor(mWaveshapeB);
    if (mShapeDirty)
    {
      mShape = GetWaveDesc();
      if (!mWalkerInitialized)
      {
        mWalkerInitialized = true;
        mWalker = mShape.MakeWalker(0);
      }
      mShapeDirty = false;
    }
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    audioRatePhaseOffset = math::wrap01(audioRatePhaseOffset);

    const PhaseAdvance step = mPhaseAcc.advanceOneSample();
    //const WVShape ws = GetWaveDesc();
    mHelper.OpenSample(step.lengthInPhase01);

    const double sampleStartInPhase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    mWalker.JumpToPhase(
        sampleStartInPhase01);  // instead of trying to keep it synchronized with note ons etc; it's safe to do it here. mostly a NOP but will catch externally modified phase changes.
    const auto [amp0, naiveSlope] = mShape.EvalAmpSlopeAt(sampleStartInPhase01);
    const float naive = amp0;

#ifdef ENABLE_OSC_LOG
    gOscLog.Log(std::format("Let's render a sample"));

    gOscLog.Log(std::format("Ph {:.3f} -> {:.3f} (dist {:.3f})",
                            step.phaseBegin01,
                            (step.phaseBegin01 + step.lengthInPhase01),
                            step.lengthInPhase01));
    gOscLog.Log(std::format("Naive amplitude: {:.3f}", naive));
#endif  // ENABLE_OSC_LOG

    auto handleEncounteredEdge = [&](size_t segmentIndex,
                                     const WVSegment& edge,
                                     float preEdgeAmp,
                                     float preEdgeSlope,
                                     float postEdgeAmp,
                                     float postEdgeSlope,
                                     double distFromPathStartToEdgeInPhase01
#ifdef ENABLE_OSC_LOG
                                     ,
                                     std::string reason
#endif  // ENABLE_OSC_LOG
                                 )
    {
      auto edgePosInSample01 = distFromPathStartToEdgeInPhase01 / step.lengthInPhase01;

      // track for calculating deltas. Now, these must be theoretical values, not sampled values.
      // for example a saw's BLEP scale is 2.0f, but if you use running sample values you don't ever actually reach Y values which differ by 2.0f.
      // HOWEVER if you do a sync restart mid-cycle / mid-segment, then we are synthesizing an edge
      // and it can be calculated by sampling
      const float dAmp = postEdgeAmp - preEdgeAmp;
      const float dSlope = postEdgeSlope - preEdgeSlope;

      if (math::FloatEquals(dAmp + dSlope, 0))
        return;

      mHelper.AccumulateEdge(edgePosInSample01,
                             dAmp,
                             dSlope
#ifdef ENABLE_OSC_LOG
                             ,
                             reason
#endif  // ENABLE_OSC_LOG
      );
    };

    const auto [wrapEvent, syncEvent] = step.GetWrapAndSyncEvents();

    const auto sampleWindowSizeInPhase01 = step.lengthInPhase01;
    if (!syncEvent.has_value())
    {
      // Single continuous path: start at phi0Render, advance by delta
      mWalker.Step(sampleWindowSizeInPhase01, handleEncounteredEdge);
    }
    else
    {
      const auto& resetEvent = syncEvent.value();
      // Pre-reset path: [0 .. tReset)
#ifdef ENABLE_OSC_LOG
      M7::gOscLog.Log(std::format("visit pre-sync-reset window"));
#endif  // ENABLE_OSC_LOG
      const double preResetLengthInPhase01 = resetEvent.whenInSample01 *
                                             sampleWindowSizeInPhase01;  // distance from start to reset, in phase units

      mWalker.Step(preResetLengthInPhase01, handleEncounteredEdge);

      mWalker.JumpToPhase(audioRatePhaseOffset);  // reset to phase 0 + offset

      // Post-reset path: [tR .. 1)
#ifdef ENABLE_OSC_LOG
      M7::gOscLog.Log(std::format("visit post-sync-reset window"));
#endif                                                              // ENABLE_OSC_LOG
      const double resetPosInSample01 = resetEvent.whenInSample01;  // when in this sample [0,1)
      const double postResetLengthInPhase01 = (1.0 - resetPosInSample01) * sampleWindowSizeInPhase01;
      mWalker.Step(postResetLengthInPhase01, handleEncounteredEdge);
      //mShape.VisitEdgesAlongPath(audioRatePhaseOffset,      // path begin = 0+ with offset
      //                       postResetLengthInPhase01,  // path length = distance from reset to end of sample
      //                       handleEncounteredEdge);
    }

    const auto [correction, amplitude] = mHelper.CloseSampleAndGetCorrection(naive);

    return CoreSample{
        .amplitude = (float)amplitude,
        .naive = naive,
        .correction = (float)correction,  //
        .phaseAdvance = step,
#ifdef ENABLE_OSC_LOG
        .log = gOscLog.mBuffer,  //
#endif                           // ENABLE_OSC_LOG
    };
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
    Base::mShapeDirty = true;
    Base::HandleParamsChanged();
  }

  virtual WVShape GetWaveDesc() override
  {
    return WVShape{
        .mSegments = {
            WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = mDutyCycle01, .beginAmp = -1.0f, .slope = 0},
            WVSegment{.beginPhase01 = mDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
        }};
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

  virtual WVShape GetWaveDesc() override
  {
    return WVShape{.mSegments = {
                       WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
                   }};
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

  virtual WVShape GetWaveDesc() override
  {
    return WVShape{.mSegments = {
                       WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = 4},
                       WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = -4},
                   }};
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
