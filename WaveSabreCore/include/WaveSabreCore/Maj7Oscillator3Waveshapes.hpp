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
struct ArtisnalSawCore : public OscillatorCore
{
  ArtisnalSawCore()
      : OscillatorCore(OscillatorWaveform::SawArtisnal)
  {
    mShape = WVShape{.mSegments = {
                         WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
                     }};
  }
  PolyBlepBlampExecutor1 mHelper;
  WVShape mShape;
  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
    mHelper.OpenSample(step.lengthInPhase01);
    const auto [naive, naiveSlope] = mShape.EvalAmpSlopeAt(step.phaseBegin01);

    // walk intra-sample segments and handle discontinuities (edges) encountered
    float preEdgeAmp = naive;
    float preEdgeSlope = naiveSlope;
    for (size_t iEvent = 0; iEvent < step.eventCount; ++iEvent)
    {
      auto& e = step.events[iEvent];
      // for sawtooth, wrap and reset are similar. with the arate phase offset, the jump is not always exactly -2.0f
      // and must be calculated always.
      if (e.kind == PhaseEventKind::Wrap || e.kind == PhaseEventKind::Reset)
      {
        const double postEventPhase01 = math::wrap01(audioRatePhaseOffset);
        const auto [postEventAmp, postEventSlope] = mShape.EvalAmpSlopeAt(postEventPhase01);
        mHelper.AccumulateEdge(
            e.whenInSample01,
            postEventAmp - preEdgeAmp, postEventSlope - preEdgeSlope);
        preEdgeAmp = postEventAmp;
        preEdgeSlope = postEventSlope;
      }
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


}  // namespace M7

}  // namespace WaveSabreCore
