
#include "Maj7Sampler.hpp"
#include "../Basic/Helpers.h"

namespace WaveSabreCore
{
namespace M7
{
void SamplerDevice::Deserialize(Deserializer& ds)
{
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
  auto token = mMutex.Enter();
  SampleSource sampleSource = (SampleSource)ds.ReadUByte();
  mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, sampleSource);
  //mSampleSource.SetIntValue(sampleSource);
  if (sampleSource != SampleSource::Embed)
  {
    LoadGmDlsSample(mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsIndex));
    return;
  }
  if (!ds.ReadUByte())
  {  // indicator whether there's a sample serialized or not.
    return;
  }
  auto CompressedSize = ds.ReadUInt32();
  auto UncompressedSize = ds.ReadUInt32();

  WAVEFORMATEX wfx = {0};
  ds.ReadBuffer(&wfx, sizeof(wfx));
  auto waveFormatSize = sizeof(WAVEFORMATEX) + wfx.cbSize;
  // read the data after the WAVEFORMATEX
  auto pwfxComplete = new uint8_t[waveFormatSize];
  memcpy(pwfxComplete, &wfx, sizeof(wfx));
  ds.ReadBuffer(pwfxComplete + sizeof(wfx), wfx.cbSize);

  auto pCompressedData = new uint8_t[CompressedSize];
  ds.ReadBuffer(pCompressedData, CompressedSize);

  LoadSample((char*)pCompressedData, CompressedSize, UncompressedSize, (WAVEFORMATEX*)pwfxComplete, "");

  delete[] pCompressedData;
  delete[] pwfxComplete;
#else   // MAJ7_INCLUDE_GSM_SUPPORT

  // gm.dls is ~4mb. so we require offsets / lengths to be 26 bits (~4mb)
  // 13 bits per half; 2^13 = 8192, which is our serializable scale, so it just works out.
  auto token = mMutex.Enter();

  // this is more reliable, maintainable, and compressible. slightly smaller than below.
  static_assert(IntParamConfig::kSerializableScale >= 8192, "");
  const int offsetHi = mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsOffsetHi);
  const int offsetLo = mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsOffsetLo);
  const int lengthHi = mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsLengthHi);
  const int lengthLo = mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsLengthLo);
  const int offset = offsetHi * 8192 + offsetLo;
  const int length = lengthHi * 8192 + lengthLo;

  // params are 0-1; they need to be scaled up to 0-8192, then combined to get the full 26 bit offset / length.
  // const auto offset = mParams.mParamCache[(int)SamplerParamIndexOffsets::GmDlsOffsetHi] * 8192 * 8192 + mParams.mParamCache[(int)SamplerParamIndexOffsets::GmDlsOffsetLo] * 8192;
  // const auto length = mParams.mParamCache[(int)SamplerParamIndexOffsets::GmDlsLengthHi] * 8192 * 8192 + mParams.mParamCache[(int)SamplerParamIndexOffsets::GmDlsLengthLo] * 8192; 

  mSample.mpSampleData = (const int16_t*)((uintptr_t)GmDls::gpData + (uintptr_t)offset);
  mSample.mSampleLength = (int)length;

#endif  // MAJ7_INCLUDE_GSM_SUPPORT
}

#ifdef MAJ7_INCLUDE_GSM_SUPPORT
void SamplerDevice::Serialize(Serializer& s)
{
  // s.WriteUByte((uint8_t)SampleSideDataFormat::GmDlsSpan); // TODO: support gsm & gmdls.
  // s.WriteUInt32(((const uint8_t*)mSample.mpSampleData) - GmDls::gpData); // sample data offset
  // s.WriteUInt32(mSample.mSampleLength); // sample data

  auto token = mMutex.Enter();
  // params are already serialized. just serialize the non-param stuff (just sample data).
  // indicate sample source
  auto sampleSrc = mParams.GetEnumValue<SampleSource>(SamplerParamIndexOffsets::SampleSource);
  s.WriteUByte((uint8_t)sampleSrc);  // mSampleSource.GetIntValue());
  if (sampleSrc != SampleSource::Embed)
  {
    return;
  }
  if (!mSample)
  {
    s.WriteUByte(0);  // indicate there's no sample serialized.
    return;
  }
  s.WriteUByte(1);  // indicate there's a sample serialized.

  auto pSample = static_cast<GsmSample*>(mSample);

  s.WriteUInt32(pSample->CompressedSize);
  s.WriteUInt32(pSample->UncompressedSize);

  auto waveFormat = reinterpret_cast<const WAVEFORMATEX*>(pSample->WaveFormatData.data());
  auto waveFormatSize = sizeof(WAVEFORMATEX) + waveFormat->cbSize;
  s.WriteBuffer(pSample->WaveFormatData.data(), waveFormatSize);

  // Write compressed data
  s.WriteBuffer(pSample->CompressedData.data(), pSample->CompressedSize);
}
#endif  // MAJ7_INCLUDE_GSM_SUPPORT


//SamplerDevice::SamplerDevice(float* paramCache, ModulationSpec* ampEnvModulation,
//	ParamIndices baseParamID, ParamIndices ampEnvBaseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
//) :
SamplerDevice::SamplerDevice(float* paramCache, ModulationList modulations, const SourceInfo& srcInfo)
    : ISoundSourceDevice(paramCache,
                         modulations[srcInfo.mModulationIndex],
                         srcInfo.mParamBase,  //ampEnvBaseParamID,
                         srcInfo.mAmpModSource,
                         srcInfo.mModDestBase)  //,
                                                //mParams(paramCache, sourceInfo.mParamBase)
{
}

#ifdef MAJ7_INCLUDE_GSM_SUPPORT
// called when loading chunk, or by VST
void SamplerDevice::LoadSample(char* compressedDataPtr,
                               int compressedSize,
                               int uncompressedSize,
                               WAVEFORMATEX* waveFormatPtr,
                               const char* path)
{
  auto token = mMutex.Enter();
  //if (mSample)
  //{
  //  delete mSample;
  //  mSample = nullptr;
  //}
  mSampleSource.SetEnumValue(SampleSource::Embed);
  mSampleLoadSequence++;
  mSample = new GsmSample(compressedDataPtr, compressedSize, uncompressedSize, waveFormatPtr);
  strcpy(mSamplePath, path);
}
#endif  // MAJ7_INCLUDE_GSM_SUPPORT

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
void SamplerDevice::LoadGmDlsSample(int sampleIndex)
{
  auto token = mMutex.Enter();
  if (sampleIndex < 0 || sampleIndex >= gGmDlsSampleCount)
  {
    return;
  }

  //mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, SampleSource::GmDls);
  mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsIndex, sampleIndex);
  
  mSample.LoadGmDlsIndex(sampleIndex);

  // store mSample.mpSampleData and mSample.mSampleLength in params so they can be serialized.
  const int offset = (uintptr_t)mSample.mpSampleData - (uintptr_t)GmDls::gpData;
  const int offsetHi = offset >> 13;
  const int offsetLo = offset & ((1 << 13) - 1);
  mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsOffsetHi, offsetHi);
  mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsOffsetLo, offsetLo);

  const int length = mSample.mSampleLength;
  const int lengthHi = length >> 13;
  const int lengthLo = length & ((1 << 13) - 1);
  mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsLengthHi, lengthHi);
  mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsLengthLo, lengthLo);
}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

void SamplerDevice::BeginBlock()
{
  mMutex.ManualEnter();

  mEnabledCached = mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled);

  ISoundSourceDevice::BeginBlock();

  // base note + original samplerate gives us a "native frequency" of the sample.
  // so let's say the sample is 22.05khz, base midi note 60 (261hz).
  // and you are requested to play it back at "1000hz" (or, midi note 88.7)
  // and imagine native sample rate is 44.1khz.
  // the base frequency is 261hz, and you're requesting to play 1000hz
  // so that's (play_hz / base_hz) = a rate of 3.83. plus factor the base samplerate, and it's
  // (base_sr / play_sr) = another rate of 0.5.
  // put together that's (play_hz / base_hz) * (base_sr / play_sr)
  // or, play_hz * (base_sr / (base_hz * play_sr))
  //               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  // i don't know play_hz yet at this stage, let's simplify & precalculate the rest
  //if (!mSample)
  //{
  //  return;
  //}
  float base_hz = math::MIDINoteToFreq((float)mParams.GetIntValue(SamplerParamIndexOffsets::BaseNote));
  mSampleRateCorrectionFactor =
      GmDlsSample::kSampleRate /
      (2 * base_hz *
       Helpers::CurrentSampleRateF);  // WHY * 2? because it corresponds more naturally to other synth octave ranges.
}

void SamplerDevice::EndBlock()
{
  mMutex.ManualLeave();
}


SamplerVoice::SamplerVoice(ModMatrixNode& modMatrix, SamplerDevice* pDevice, EnvelopeNode* pAmpEnv)
    : ISoundSourceDevice::Voice(pDevice, &modMatrix, pAmpEnv)
    , mpSamplerDevice(pDevice)
{
}

void SamplerVoice::ClearState()
{
  mSamplePlayer = SamplePlayer{};
  mNoteIsOn = false;
  // mDelayPos01 = 0;
  // mDelayStep = 0;
}

void SamplerVoice::ConfigPlayer()
{
  mSamplePlayer.SampleStart = mpSamplerDevice->mParams.Get01Value(
      SamplerParamIndexOffsets::SampleStart,
      mpModMatrix->GetDestinationValue(
          (int)mpSrcDevice->mModDestBaseID +
          (int)SamplerModParamIndexOffsets::SampleStart));  // mpSamplerDevice->mSampleStart.Get01Value();
  mSamplePlayer.LoopStart = mpSamplerDevice->mParams.Get01Value(
      SamplerParamIndexOffsets::LoopStart);  //mpSamplerDevice->mLoopStart.Get01Value();
  mSamplePlayer.LoopLength = mpSamplerDevice->mParams.Get01Value(
      SamplerParamIndexOffsets::LoopLength);  //mpSamplerDevice->mLoopLength.Get01Value();
  if (!mNoteIsOn && mpSamplerDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::ReleaseExitsLoop))
  {  // mReleaseExitsLoop.GetBoolValue()) {
     //if (!mNoteIsOn && mpSamplerDevice->mReleaseExitsLoop.GetBoolValue()) {
    mSamplePlayer.LoopMode = LoopMode::Disabled;
  }
  else
  {
    //mSamplePlayer.LoopMode = mpSamplerDevice->mLoopMode.GetEnumValue();
    mSamplePlayer.LoopMode = mpSamplerDevice->mParams.GetEnumValue<LoopMode>(SamplerParamIndexOffsets::LoopMode);
  }
  //mSamplePlayer.InterpolationMode = mpSamplerDevice->mParams.GetEnumValue<InterpolationMode>(SamplerParamIndexOffsets::InterpolationType); //mpSamplerDevice->mInterpolationMode.GetEnumValue();
  mSamplePlayer.Reverse = mpSamplerDevice->mParams.GetBoolValue(
      SamplerParamIndexOffsets::Reverse);  //mpSamplerDevice->mReverse.GetBoolValue();
}

void SamplerVoice::NoteOn(bool legato)
{
  //if (!mpSamplerDevice->mSample)
  //{
  //  return;
  //}
  if (!mpSrcDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled))
  {
    return;
  }
  if (legato && !mpSamplerDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::LegatoTrig))
  {
    return;
  }

  auto token = mpSamplerDevice->mMutex.Enter();

  mSamplePlayer.mpSample = &mpSamplerDevice->mSample;
  //mSamplePlayer.mpSampleBuffer = &mpSamplerDevice->mSample->GetSampleBuffer();
  //mSamplePlayer.SampleLength = mpSamplerDevice->mSample->GetSampleLength();
  //mSamplePlayer.SampleLoopStart = mpSamplerDevice->mSample->GetSampleLoopStart();  // used for boundary mode from sample
  //mSamplePlayer.SampleLoopLength =
  //    mpSamplerDevice->mSample->GetSampleLoopLength();  // used for boundary mode from sample
  mNoteIsOn = true;
  // this is a subtle but important thing. DO NOT calculate the delay step yet here. on the 1st note (see #35)
  // the mod matrix hasn't run yet and will cause the delay stage to always be skipped. so we have to:
  // 1. set step to 0 to ensure a sample is always processed (and with it, the mod matrix)
  // 2. make sure we calculate delaystep *after* handling delay complete
  mDelayStep = 0;
  mDelayPos01 = 0;
  ConfigPlayer();
}

void SamplerVoice::NoteOff()
{
  mNoteIsOn = false;
}

void SamplerVoice::BeginBlock(/*real_t midiNote, float detuneFreqMul, float fmScale,*/)
{
  if (!mpSamplerDevice->mEnabledCached)  // >mEnabledParam.mCachedVal)
    return;
  //if (!mpSamplerDevice->mSample)
  //{
  //  return;
  //}

  ConfigPlayer();
  mSamplePlayer.RunPrep();
}

float SamplerVoice::ProcessSample(real_t midiNote, float detuneFreqMul, float fmScale, float ampEnvLin)
{
  if (!mpSrcDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled))
    return 0;

  // see above for subtle info about handling modulated delay
  if (mDelayPos01 < 1)
  {
    mDelayPos01 += mDelayStep;
    if (mDelayPos01 >= 1)
    {
      mSamplePlayer.InitPos();  // play.
    }
  }
  auto ms = mpSamplerDevice->mParams.GetPowCurvedValue(
      SamplerParamIndexOffsets::Delay,
      gEnvTimeCfg,
      mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::Delay));
  mDelayStep = math::CalculateInc01PerSampleForMS(ms);

  // todo: unify freq calculation with oscillator.
  // but it would mean changing mod dest offsets so they are the same
  // and changing param offsets to match as well. for maybe a savings of like 60 bytes of squished code.
  float pitchFineMod = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                        (int)SamplerModParamIndexOffsets::PitchFine);
  float freqMod = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                   (int)SamplerModParamIndexOffsets::FrequencyParam);

  int pitchSemis = mpSamplerDevice->mParams.GetIntValue(SamplerParamIndexOffsets::TuneSemis);
  float pitchFine = mpSamplerDevice->mParams.GetN11Value(SamplerParamIndexOffsets::TuneFine, pitchFineMod) *
                    gSourcePitchFineRangeSemis;
  midiNote += pitchSemis + pitchFine;

  float noteHz = math::MIDINoteToFreq(midiNote);
  float freq = mpSamplerDevice->mParams.GetFrequency(
      SamplerParamIndexOffsets::FreqParam, SamplerParamIndexOffsets::FreqKT, gSourceFreqConfig, noteHz, freqMod);
  freq *= detuneFreqMul;

  float rate = freq * mpSamplerDevice->mSampleRateCorrectionFactor;

  mSamplePlayer.SetPlayRate(rate);

  return math::clampN11(mSamplePlayer.Next() * ampEnvLin);  // clamp addresses craz glitch when changing samples.
}


}  // namespace M7


}  // namespace WaveSabreCore
