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


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// example of simplest core: a sine wave; no band limiting supported
struct SineCore : public OscillatorCore
{
  SineCore()
      : OscillatorCore(OscillatorWaveform::Sine)
  {
  }
  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
    //const double renderPhase = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    float y = math::sin(math::gPITimes2 * (float)step.phaseBegin01);
    return {
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BlepExecutor
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

  // correction spill
  double mNow = 0.0;   // applies to *this* sample
  double mNext = 0.0;  // applies to next sample

  void OpenSample()
  {
    mNow = mNext;
    mNext = 0;
  }

  inline void AddPolyBLEP(float u, float dAmp)
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
                            mNow,
                            mNow + blepBefore * dAmp));
    gOscLog.Log(std::format(" -> blepnext({:.3f} : {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
                            uo,
                            u,
                            blepAfter,
                            dAmp,
                            blepAfter * dAmp,
                            mNext,
                            mNext + blepAfter * dAmp));
#endif  // ENABLE_OSC_LOG

    mNow += dAmp * blepBefore;
    mNext += dAmp * BlepAfter(u);
  }


  // u is whenInSample01 ∈ [0,1)
  void AccumulateEdge(double edgePosInSample01, float dAmp)
  {
    if (dAmp != 0.0f)
    {
      AddPolyBLEP((float)edgePosInSample01, dAmp);

      //
      //
      //
      //  float u = 1.0f - static_cast<float>(edgePosInSample01);
      //mNow += dAmp * BlepBefore(u);
      //mNext += dAmp * BlepAfter(u);
    }
  }

  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude)
  {
    return {mNow, naiveAmplitude + mNow};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct ArtisnalSawCore : public OscillatorCore
{
  ArtisnalSawCore()
      : OscillatorCore(OscillatorWaveform::SawArtisnal)
  {
  }
  BlepExecutor mHelper;

  float EvalNaiveSawAtPhase(double phase01)
  {
    return (float)(-1.0 + 2.0 * phase01);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
    std::vector<EdgeEvent> edgeEvents;

    mHelper.OpenSample();
    const auto naive = EvalNaiveSawAtPhase(step.phaseBegin01);

    // cursor over the sample fraction and evaluation phase
    double fPrev = 0.0;                    // previous event time in sample[0,1)
    double phaseEval = step.phaseBegin01;  // eval phase at fPrev

    for (size_t i = 0; i < step.eventCount; ++i)
    {
      const auto& e = step.events[i];
      const double deltaInSample01 = e.whenInSample01 - fPrev;

      // advance within the current segment (no resets until we hit the event)
      phaseEval = (phaseEval + audioRatePhaseOffset +
                   deltaInSample01 * step.lengthInPhase01);  // do not wrap! we need to catch 1

      if (e.kind == PhaseEventKind::Wrap || e.kind == PhaseEventKind::Reset)
      {
        const float preAmp = EvalNaiveSawAtPhase(phaseEval);  // just BEFORE the edge
        const double postPhase = math::wrap01(
            double(audioRatePhaseOffset));  // AFTER the edge, not the end of sample (0) (phase reset)
        const float postAmp = EvalNaiveSawAtPhase(postPhase);
        const float dAmp = postAmp - preAmp;
        if (!math::FloatEquals(dAmp, -2.f))
        {
          MulDiv(1, 1, 1);
        }

#ifdef ENABLE_OSC_LOG
        gOscLog.Log(std::format("Saw {} @ u={:.3f} phase={:.3f} pre:{:.3f} -> post:{:.3f} dAmp={:.3f}",
                                (e.kind == PhaseEventKind::Wrap) ? "wrap" : "reset",
                                e.whenInSample01,
                                phaseEval,
                                preAmp,
                                postAmp,
                                dAmp));
        edgeEvents.push_back(EdgeEvent{
            .reason = e.kind,
            .atPhase01 = (float)phaseEval,
            .whenInSample01 = (float)e.whenInSample01,
            .preEdgeAmp = preAmp,
            .postEdgeAmp = postAmp,
            .dAmp = dAmp,
            .preEdgeSlope = +2.0f,
            .postEdgeSlope = +2.0f,
            .dSlope = 0.0f,

        });
#endif  // ENABLE_OSC_LOG

        mHelper.AccumulateEdge(e.whenInSample01, dAmp);


        // after the edge, the phase resets to 0 (+ offset), continue from there
        phaseEval = postPhase;
      }

      fPrev = e.whenInSample01;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BasicSawCore : public OscillatorCore
{
  BasicSawCore()
      : OscillatorCore(OscillatorWaveform::SawBasic)
  {
  }

  // t = phase in [0,1), dt = phase increment per sample in [0,1)
  // around the saw discontinuity, this returns the correction value.
  inline double poly_blep_saw(double t, double dt)
  {
    // t in [0,1), dt in (0,1)
    if (t < dt)
    {
      t /= dt;  // 0..1
      // t + t - t^2 - 1  ==  2t - t^2 - 1
      return t + t - t * t - 1.0;
    }
    else if (t > 1.0 - dt)
    {
      t = (t - 1.0) / dt;  // -1..0
      // t^2 + 2t + 1  ==  (t + 1)^2
      return t * t + t + t + 1.0;
    }
    return 0.0;
  }

  inline float bl_saw_sample(double phase, double dt)
  {
    double v = 2.0 * phase - 1.0;   // naive ramp
    v -= poly_blep_saw(phase, dt);  // polyBLEP correction
    return (float)v;
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
    const double dt = step.lengthInPhase01;  // per-sample phase increment
    const double phase = step.phaseBegin01;  // evaluation phase at sample start

    float y = bl_saw_sample(phase, dt);

    for (int i = 0; i < step.eventCount; ++i)
    {
      const auto& e = step.events[i];
      if (e.kind == PhaseEventKind::Reset)
      {
        // Phase at the *start* of this sample when the reset edge belongs to it.
        // For a mid-sample reset we’re in the "tail" branch (t > 1-dt).
        // Using the current-sample evaluation phase here works well in practice:
        y -= (float)poly_blep_saw(phase, dt);
      }
    }
    return CoreSample{
        .amplitude = (float)y,
        .naive = y,
        .correction = (float)0,  //
        .phaseAdvance = step,
#ifdef ENABLE_OSC_LOG
        .log = gOscLog.mBuffer,  //
#endif                           // ENABLE_OSC_LOG
    };
  }
};


}  // namespace M7

}  // namespace WaveSabreCore
