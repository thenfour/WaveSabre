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

}  // namespace M7

}  // namespace WaveSabreCore
