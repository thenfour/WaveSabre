#pragma once

#include <WaveSabreCore/Filters/FilterOnePole.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include "Maj7OscillatorBase.hpp"

namespace WaveSabreCore
{
namespace M7
{
// helps select constructors.
// Some differences in LFO and Audio behaviors:
// - LFOs don't have any anti-aliasing measures.
// - LFOs have fewer vst-controlled params; so many of their params have backing values as members
// - LFO modulations are K-Rate rather than A-Rate.
//struct OscillatorIntentionLFO {};
//struct OscillatorIntentionAudio {};

// nothing additional to add
struct OscillatorDevice : ISoundSourceDevice
{
  OscillatorIntention mIntention;

  virtual bool IsEnabled() const override
  {
    return (mIntention == OscillatorIntention::LFO) || mParams.GetBoolValue(OscParamIndexOffsets::Enabled);
  }
  virtual bool MatchesKeyRange(int midiNote) const override
  {
    if (mIntention == OscillatorIntention::LFO)
      return true;
    if (mParams.GetIntValue(OscParamIndexOffsets::KeyRangeMin, gKeyRangeCfg) > midiNote)
      return false;
    if (mParams.GetIntValue(OscParamIndexOffsets::KeyRangeMax, gKeyRangeCfg) < midiNote)
      return false;
    return true;
  }


  //virtual float GetLinearVolume(float mod) const override
  //{
  //	return mParams.GetLinearVolume(OscParamIndexOffsets::Volume, gUnityVolumeCfg, mod);
  //}

  // for Audio
  explicit OscillatorDevice(/*OscillatorIntentionAudio, */ float* paramCache,
                            ModulationList modulations,
                            const SourceInfo& srcinfo /* ModulationSpec* pAmpModulation, size_t isrc*/);

  // for LFO, we internalize many params
  explicit OscillatorDevice(/*OscillatorIntentionLFO, */ float* paramCache, size_t ilfo);

  bool GetPhaseRestart() const
  {
    return mParams.GetBoolValue((mIntention == OscillatorIntention::Audio) ? (int)OscParamIndexOffsets::PhaseRestart
                                                                           : (int)LFOParamIndexOffsets::Restart);
  }
  //// only for LFO
  //float GetLPFFrequency() const {
  //	return mParams.GetFrequency(LFOParamIndexOffsets::Sharpness, -1, gLFOLPFreqConfig, 0, );
  //}

  virtual void BeginBlock() override
  {
    ISoundSourceDevice::BeginBlock();
  }
  virtual void EndBlock() override {}
};


}  // namespace M7


}  // namespace WaveSabreCore
