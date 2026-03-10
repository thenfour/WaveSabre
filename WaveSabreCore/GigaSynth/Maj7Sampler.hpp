#pragma once

#include <Windows.h>
// correction for windows.h macros.
#undef min
#undef max

//#include <WaveSabreCore/Maj7Basic.hpp>
//#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include "../Basic/CriticalSection.hpp"
#include "../Basic/GmDls.h"
#include "../Basic/PodVector.hpp"
#include "../Basic/Serializer.hpp"

#ifdef MAJ7_INCLUDE_GSM_SUPPORT
  #include "GsmSample.h"
#endif  // MAJ7_INCLUDE_GSM_SUPPORT
#include "../DSP/SamplePlayer.h"
#include "../GigaSynth/SampleSource.hpp"
#include "Maj7Oscillator.hpp"


namespace WaveSabreCore
{
namespace M7
{

////////////////////////////////////////////////////////////////////////////////////////////////
struct SamplerDevice : ISoundSourceDevice
{
  bool mEnabledCached;

  float mSampleRateCorrectionFactor = 0;
  WaveSabreCore::CriticalSection mMutex;

  GmDlsSample mSample;

  void Deserialize(Deserializer& ds);
  
  explicit SamplerDevice(float* paramCache, ModulationList modulations, const SourceInfo& srcInfo);

#ifdef MAJ7_INCLUDE_GSM_SUPPORT
  char mSamplePath[MAX_PATH] = {0};
  void Serialize(Serializer& s);

  // called when loading chunk, or by VST
  void LoadSample(char* compressedDataPtr,
                  int compressedSize,
                  int uncompressedSize,
                  WAVEFORMATEX* waveFormatPtr,
                  const char* path);
#endif  // MAJ7_INCLUDE_GSM_SUPPORT

  void LoadGmDlsSample(int sampleIndex);

  virtual void BeginBlock() override;

  virtual void EndBlock() override;

  virtual bool IsEnabled() const override
  {
    return mParams.GetBoolValue(OscParamIndexOffsets::Enabled);
  }
  virtual bool MatchesKeyRange(int midiNote) const override
  {
    if (mParams.GetIntValue(SamplerParamIndexOffsets::KeyRangeMin) > midiNote)
      return false;
    if (mParams.GetIntValue(SamplerParamIndexOffsets::KeyRangeMax) < midiNote)
      return false;
    return true;
  }

};  // struct SamplerDevice

////////////////////////////////////////////////////////////////////////////////////////////////
struct SamplerVoice : ISoundSourceDevice::Voice
{
  SamplerDevice* mpSamplerDevice;
  SamplePlayer mSamplePlayer;
  bool mNoteIsOn = false;

  float mDelayPos01 = 0;
  float mDelayStep = 0;  // per sample, how much to advance the delay stage. meaningless outside of delay stage.

  SamplerVoice(ModMatrixNode& modMatrix, SamplerDevice* pDevice, EnvelopeNode* pAmpEnv);
  void ConfigPlayer();

  virtual void NoteOn(bool legato) override;
  virtual void NoteOff() override;

  virtual void BeginBlock() override;

  float ProcessSample(real_t midiNote, float detuneFreqMul, float fmScale, float ampEnvLin);

};  // struct SamplerVoice

}  // namespace M7


}  // namespace WaveSabreCore
