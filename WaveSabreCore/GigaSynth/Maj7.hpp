// Time sync for LFOs? use Helpers::CurrentTempo; but do i always know the song position?

// size-optimizations:
// (profile w sizebench!)
// - use fixed-point in oscillators. there's absolutely no reason to be using chonky floating point instructions there.

// Pitch params in order of processing:
// x                                  Units        Level
// --------------------------------------------------------
// - midi note                        note         voice @ note on
// - portamento time / curve          note         voice @ block process
// - pitch bend / pitch bend range    note         voice       <-- used for modulation source
// - osc pitch semis                  note         oscillator
// - osc fine (semis)                 note         oscillator
// - osc sync freq / kt (Hz)          hz           oscillator
// - osc freq / kt (Hz)               hz           oscillator
// - osc mul (Hz)                     hz           oscillator
// - osc detune (semis)               hz+semis     oscillator
// - unisono detune (semis)           hz+semis     oscillator

#pragma once

#include "../Analysis/AnalysisStream.hpp"
#include "../Analysis/RMS.hpp"
#include "../Basic/DSPMath.hpp"
#include "../Basic/Helpers.h"
#include "../Devices/Maj7SynthDevice.hpp"
#include "../Params/Maj7ParamAccessor.hpp"


#include "../DSP/Maj7Envelope.hpp"
#include "../Filters/Maj7Filter.hpp"
#include "../Filters/DCFilter.hpp"
#include "../GigaSynth/GigaParams.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "./Maj7Oscillator.hpp"
#include "./Maj7Oscillator3.hpp"
#include "./Maj7Sampler.hpp"


namespace WaveSabreCore
{
namespace M7
{
// even if this doesn't strictly need to have #ifdef, better to make it clear to callers that this depends on built config.
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
enum class OutputStream
{
  Master,
  Osc1,
  Osc2,
  Osc3,
  Osc4,
  LFO1,
  LFO2,
  LFO3,
  LFO4,
  ModMatrix_NormRecalcSample,
  LFO1_NormRecalcSample,
  ModSource_LFO1,
  ModSource_LFO2,
  ModSource_LFO3,
  ModSource_LFO4,
  ModSource_Osc1AmpEnv,
  ModSource_Osc2AmpEnv,
  ModSource_Osc3AmpEnv,
  ModSource_Osc4AmpEnv,
  ModSource_ModEnv1,
  ModSource_ModEnv2,
  ModDest_Osc1PreFMVolume,
  ModDest_Osc2PreFMVolume,
  ModDest_Osc3PreFMVolume,
  ModDest_Osc4PreFMVolume,
  ModDest_FMFeedback1,
  ModDest_FMFeedback2,
  ModDest_FMFeedback3,
  ModDest_FMFeedback4,
  ModDest_Osc1Freq,
  ModDest_Osc2Freq,
  ModDest_Osc3Freq,
  ModDest_Osc4Freq,
  ModDest_Osc1SyncFreq,
  ModDest_Osc2SyncFreq,
  ModDest_Osc3SyncFreq,
  ModDest_Osc4SyncFreq,
  Count,
};
  #define MAJ7_OUTPUT_STREAM_CAPTIONS(symbolName)                                                                      \
    static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OutputStream::Count]{                      \
        "Master",                                                                                                      \
        "Osc1",                                                                                                        \
        "Osc2",                                                                                                        \
        "Osc3",                                                                                                        \
        "Osc4",                                                                                                        \
        "LFO1",                                                                                                        \
        "LFO2",                                                                                                        \
        "LFO3",                                                                                                        \
        "LFO4",                                                                                                        \
        "ModMatrix_NormRecalcSample",                                                                                  \
        "LFO1_NormRecalcSample",                                                                                       \
        "ModSource_LFO1",                                                                                              \
        "ModSource_LFO2",                                                                                              \
        "ModSource_LFO3",                                                                                              \
        "ModSource_LFO4",                                                                                              \
        "ModSource_Osc1AmpEnv",                                                                                        \
        "ModSource_Osc2AmpEnv",                                                                                        \
        "ModSource_Osc3AmpEnv",                                                                                        \
        "ModSource_Osc4AmpEnv",                                                                                        \
        "ModSource_ModEnv1",                                                                                           \
        "ModSource_ModEnv2",                                                                                           \
        "ModDest_Osc1PreFMVolume",                                                                                     \
        "ModDest_Osc2PreFMVolume",                                                                                     \
        "ModDest_Osc3PreFMVolume",                                                                                     \
        "ModDest_Osc4PreFMVolume",                                                                                     \
        "ModDest_FMFeedback1",                                                                                         \
        "ModDest_FMFeedback2",                                                                                         \
        "ModDest_FMFeedback3",                                                                                         \
        "ModDest_FMFeedback4",                                                                                         \
        "ModDest_Osc1Freq",                                                                                            \
        "ModDest_Osc2Freq",                                                                                            \
        "ModDest_Osc3Freq",                                                                                            \
        "ModDest_Osc4Freq",                                                                                            \
        "ModDest_Osc1SyncFreq",                                                                                        \
        "ModDest_Osc2SyncFreq",                                                                                        \
        "ModDest_Osc3SyncFreq",                                                                                        \
        "ModDest_Osc4SyncFreq",                                                                                        \
    };
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

// this does not operate on frequencies, it operates on target midi note value (0-127).
struct PortamentoCalc
{
private:
  float mSourceNote = 0;  // float because you portamento can initiate between notes
  float mTargetNote = 0;
  float mCurrentNote = 0;
  float mSlideCursorSamples = 0;
  bool mEngaged = false;

public:
  ParamAccessor mParams;

  PortamentoCalc(float* paramCache)
      : mParams(paramCache, 0)
  {
  }

  // advances cursor; call this at the end of buffer processing. next buffer will call
  // GetCurrentMidiNote() then.
  void Advance(size_t nSamples, float timeMod)
  {
    if (!mEngaged)
    {
      return;
    }
    mSlideCursorSamples += nSamples;
    float durationSamples = math::MillisecondsToSamples(mParams.GetPowCurvedValue(
        GigaSynthParamIndices::PortamentoTime, gEnvTimeCfg, timeMod));  // mTime.GetMilliseconds(timeMod));
    float t = mSlideCursorSamples / durationSamples;
    if (t >= 1)
    {
      NoteOn(mTargetNote, true);  // disengages
      return;
    }
    //t = mParams.ApplyCurveToValue(ParamIndices::PortamentoCurve, t, 0);// mCurve.ApplyToValue(t, 0);
    mCurrentNote = math::lerp(mSourceNote, mTargetNote, t);
  }

  float GetCurrentMidiNote() const
  {
    return mCurrentNote;
  }

  void NoteOn(float targetNote, bool instant)
  {
    if (instant)
    {
      mCurrentNote = mSourceNote = mTargetNote = targetNote;
      mEngaged = false;
      return;
    }
    mEngaged = true;
    mSourceNote = mCurrentNote;
    mTargetNote = targetNote;
    mSlideCursorSamples = 0;
    return;
  }
};

extern const int16_t gDefaultMasterParams[(int)MainParamIndices::Count];
extern const int16_t gDefaultSamplerParams[(int)SamplerParamIndexOffsets::Count];
extern const int16_t gDefaultModSpecParams[(int)ModParamIndexOffsets::Count];
extern const int16_t gDefaultLFOParams[(int)LFOParamIndexOffsets::Count];
extern const int16_t gDefaultEnvelopeParams[(int)EnvParamIndexOffsets::Count];
extern const int16_t gDefaultOscillatorParams[(int)OscParamIndexOffsets::Count];
extern const int16_t gDefaultFilterParams[(int)FilterParamIndexOffsets::Count];

struct Maj7 : public Maj7SynthDevice
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  OutputStream mOutputStreams[2] = {OutputStream::Master, OutputStream::Master};

  AnalysisStream mOutputAnalysis[2]{AnalysisStream{1000}, AnalysisStream { 1000 }};

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  // BASE PARAMS & state
#ifdef ENABLE_PITCHBEND
  real_t mPitchBendN11 = 0;
#endif  // ENABLE_PITCHBEND

  float mUnisonoDetuneAmts[gUnisonoVoiceMax];  // = { 0 };

  float mUnisonoPanAmts[gUnisonoVoiceMax];  // { 0 };

  ModulationSpec* mpModulations[gModulationCount];

  float mParamCache[(int)GigaSynthParamIndices::NumParams];
  ParamAccessor mParams{mParamCache, 0};

  DCFilter mDCFilters[2];

  // these are not REALLY lfos, they are used to track phase at the device level, for when an LFO's phase should be synced between all voices.
  // that would happen only when all of the following are satisfied:
  // 1. the LFO is not triggered by notes.
  // 2. the LFO has no modulations on its phase or frequency

  struct LFODevice
  {
    explicit LFODevice(float* paramCache, size_t ilfo);
    const LFOInfo& mInfo;
    OscillatorDevice mDevice;
    OscillatorNode mPhase{&mDevice, OscillatorIntention::LFO, &mNullModMatrix, nullptr};
    ModMatrixNode mNullModMatrix;
  };

  LFODevice* mpLFOs[gModLFOCount];
  OscillatorDevice* mpOscillatorDevices[gOscillatorCount];

#ifdef ENABLE_SAMPLER_DEVICE
  SamplerDevice* mpSamplerDevices[gSamplerCount];
#endif  // ENABLE_SAMPLER_DEVICE
  ISoundSourceDevice* mSources[gSourceCount];

  Maj7()
      : Maj7SynthDevice((int)GigaSynthParamIndices::NumParams, mParamCache)
  {
    for (int i = 0; i < (int)std::size(mpModulations); ++i)
    {
      mpModulations[i] = new ModulationSpec{mParamCache,
                                            (int)GigaSynthParamIndices::Mod1Enabled +
                                                ((int)ModParamIndexOffsets::Count * i)};
    }

#ifdef ENABLE_SAMPLER_DEVICE
    static_assert(gModLFOCount == gOscillatorCount && gOscillatorCount == gSamplerCount, "meecro optimizutionz");
#else
    static_assert(gModLFOCount == gOscillatorCount, "meecro optimizutionz");
#endif  // ENABLE_SAMPLER_DEVICE
    for (int i = 0; i < gModLFOCount; ++i)
    {
      mpLFOs[i] = new LFODevice(mParamCache, i);
      mSources[i] = mpOscillatorDevices[i] = new OscillatorDevice(mParamCache, mpModulations, gSourceInfo[i]);
#ifdef ENABLE_SAMPLER_DEVICE
      mSources[i + gOscillatorCount] = mpSamplerDevices[i] = new SamplerDevice(mParamCache,
                                                                               mpModulations,
                                                                               gSourceInfo[i + gOscillatorCount]);
#endif  // ENABLE_SAMPLER_DEVICE
    }

    for (size_t i = 0; i < mVoices.Size(); ++i)
    {
      mVoices[i] = mMaj7Voice[i] = new Maj7Voice(this);
    }

    LoadDefaults();
  }

  ~Maj7()
  {
#ifdef MIN_SIZE_REL
  #pragma message("Maj7::~Maj7() Leaking memory to save bits.")
#else
    for (size_t i = 0; i < std::size(mpModulations); ++i)
    {
      delete mpModulations[i];
    }

    for (size_t i = 0; i < gModLFOCount; ++i)
    {
      delete mpLFOs[i];
      delete mpOscillatorDevices[i];
#ifdef ENABLE_SAMPLER_DEVICE
      delete mpSamplerDevices[i];
#endif  // ENABLE_SAMPLER_DEVICE
    }

    for (size_t i = 0; i < mVoices.Size(); ++i)
    {
      delete mMaj7Voice[i];
    }
#endif  // MIN_SIZE_REL
  }

  virtual void LoadDefaults() override
  {
    // samplers reset
    //for (auto& s : mpSamplerDevices)
    //{
    //  s->Reset();
    //}

    // now load in all default params
    ImportDefaultsArray(std::size(gDefaultMasterParams), gDefaultMasterParams, mParamCache);

    for (auto& m : mpLFOs)
    {
      ImportDefaultsArray(std::size(gDefaultLFOParams), gDefaultLFOParams, m->mDevice.mParams.GetOffsetParamCache());
    }
    for (auto& m : mpOscillatorDevices)
    {
      ImportDefaultsArray(std::size(gDefaultOscillatorParams),
                          gDefaultOscillatorParams,
                          m->mParams.GetOffsetParamCache());  // mParamCache + (int)m.mBaseParamID);
    }
#ifdef ENABLE_SAMPLER_DEVICE
    for (auto& s : mpSamplerDevices)
    {
      ImportDefaultsArray(std::size(gDefaultSamplerParams),
                          gDefaultSamplerParams,
                          s->mParams.GetOffsetParamCache());  // mParamCache + (int)s.mBaseParamID);
    }
#endif  // ENABLE_SAMPLER_DEVICE
    for (int i = 0; i < (int)std::size(mpModulations); ++i)
    {
      ImportDefaultsArray(std::size(gDefaultModSpecParams),
                          gDefaultModSpecParams,
                          mpModulations[i]->mParams.GetOffsetParamCache());  // mParamCache + (int)m.mBaseParamID);
    }
    //for (auto* m : mMaj7Voice[0]->mFilter)
    //{
    //  ImportDefaultsArray(std::size(gDefaultFilterParams), gDefaultFilterParams, m[0]->mParams.GetOffsetParamCache());
    //}
    ImportDefaultsArray(std::size(gDefaultFilterParams),
                        gDefaultFilterParams,
                        mMaj7Voice[0]->mFilter[0].mParams.GetOffsetParamCache());
    for (auto& m : mMaj7Voice[0]->mpEnvelopes)
    {
      ImportDefaultsArray(std::size(gDefaultEnvelopeParams),
                          gDefaultEnvelopeParams,
                          m->mParams.GetOffsetParamCache());  // mParamCache + (int)p.mParams.mBaseParamID);
    }
    for (auto* p : mSources)
    {
      p->InitDevice();
    }

    mParamCache[(int)GigaSynthParamIndices::Osc1Enabled] = 1.0f;

    // Apply dynamic state
    this->SetVoiceMode(
        mParams.GetEnumValue<VoiceMode>(GigaSynthParamIndices::VoicingMode));     // mVoicingModeParam.GetEnumValue());
    this->SetUnisonoVoices(mParams.GetIntValue(GigaSynthParamIndices::Unisono));  // mUnisonoVoicesParam.GetIntValue());

#ifdef _DEBUG
    // validate values.
    if (mVoicesUnisono < 1 || mVoicesUnisono > gUnisonoVoiceMax)
    {
      // clamp and allow execution in order to access the UI for investigation / generating defaults.
      mVoicesUnisono = 1;
      //throw std::runtime_error("Invalid unisono voice count loaded from defaults.");
    }
#endif

    // NOTE: samplers will always be empty here

    SetVoiceInitialStates();
  }

  // when you load params or defaults, you need to seed voice initial states so that
  // when for example a NoteOn happens, the voice will have correct values in its mod matrix.
  void SetVoiceInitialStates();

//  virtual void HandlePitchBend(float pbN11) override
//  {
//#ifdef ENABLE_PITCHBEND
//    mPitchBendN11 = pbN11;
//#endif  // ENABLE_PITCHBEND
//  }
//
//  virtual void HandleMidiCC(int ccN, int val) override
//  {
//    // we don't care about this for the moment.
//  }

  // minified
  virtual void SetBinary16DiffChunk(M7::Deserializer& ds) override
  //virtual void SetBinary16DiffChunk(void* data, int size) override
  {
    //Deserializer ds{ (const uint8_t*)data };
    Device::SetBinary16DiffChunk(ds);
    //SetMaj7StyleChunk(ds);
#ifdef ENABLE_SAMPLER_DEVICE
    for (auto& s : mpSamplerDevices)
    {
      s->Deserialize(ds);
    }
#endif  // ENABLE_SAMPLER_DEVICE
    SetVoiceInitialStates();
  }

  void SetParam(int index, float value)
  {
    if (index < 0)
      return;
    if (index >= (int)GigaSynthParamIndices::NumParams)
      return;

    mParamCache[index] = value;

    switch (index)
    {
      case (int)GigaSynthParamIndices::VoicingMode:
      {
        this->SetVoiceMode(mParams.GetEnumValue<VoiceMode>(GigaSynthParamIndices::VoicingMode));
        break;
      }
      case (int)GigaSynthParamIndices::Unisono:
      {
        this->SetUnisonoVoices(mParams.GetIntValue(GigaSynthParamIndices::Unisono));
        break;
      }
      case (int)GigaSynthParamIndices::MaxVoices:
      {
        this->SetMaxVoices(mParams.GetIntValue(GigaSynthParamIndices::MaxVoices));
        break;
      }
    }
  }

  virtual void ProcessBlock(float* const* const outputs, int numSamples) override
  {
    ProcessBlock(outputs, numSamples, false);
  }

  void ProcessBlock(float* const* const outputs, int numSamples, bool forceAllVoicesToProcess)
  {
    bool sourceEnabled[gSourceCount];

    for (size_t i = 0; i < gSourceCount; ++i)
    {
      auto* src = mSources[i];
      bool enabled = src->IsEnabled();
      sourceEnabled[i] = enabled;
      src->BeginBlock();
    }

    for (int i = 0; i < (int)std::size(mpModulations); ++i)
    {
      mpModulations[i]->BeginBlock();
    }

    //float sourceModDistribution[gSourceCount];
    //BipolarDistribute(gSourceCount, sourceEnabled, sourceModDistribution);

    bool unisonoEnabled[gUnisonoVoiceMax] = {false};
    for (size_t i = 0; i < (size_t)mVoicesUnisono; ++i)
    {
      unisonoEnabled[i] = true;
    }
    BipolarDistribute(gUnisonoVoiceMax, unisonoEnabled, mUnisonoDetuneAmts);

    // scale down per param & mod
    for (size_t i = 0; i < (size_t)mVoicesUnisono; ++i)
    {
      mUnisonoPanAmts[i] = mUnisonoDetuneAmts[i] *
                           (mParamCache[(int)GigaSynthParamIndices::UnisonoStereoSpread] /*+ mUnisonoStereoSpreadMod*/);
      mUnisonoDetuneAmts[i] *= mParamCache[(int)GigaSynthParamIndices::UnisonoDetune] /*+ mUnisonoDetuneMod*/;
    }

    for (size_t i = 0; i < gModLFOCount; ++i)
    {
      auto& lfo = mpLFOs[i];
      lfo->mDevice.BeginBlock();
      lfo->mPhase.BeginBlock();
    }

    for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
    {
      mMaj7Voice[iv]->BeginBlock(forceAllVoicesToProcess);
    }

    const float masterGain = mParams.GetLinearVolume(GigaSynthParamIndices::MasterVolume, gMasterVolumeCfg, 0);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    bool isGuiVisible = IsGuiVisible();
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
    {
      float s[2] = {0};

      for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
      {
        mMaj7Voice[iv]->ProcessAndMix(s, forceAllVoicesToProcess);
      }

      for (size_t ioutput = 0; ioutput < 2; ++ioutput)
      {
        float o = s[ioutput];
        o *= masterGain;
        o = mDCFilters[ioutput].ProcessSample(o);
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        outputs[ioutput][iSample] = SelectStreamValue(mOutputStreams[ioutput], o);
        if (isGuiVisible)
        {
          mOutputAnalysis[ioutput].WriteSample(o);
        }
#else
        outputs[ioutput][iSample] = o;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
      }

      // advance phase of master LFOs
      for (size_t i = 0; i < gModLFOCount; ++i)
      {
        auto& lfo = *mpLFOs[i];
        lfo.mPhase.RenderSampleForLFOAndAdvancePhase(true);
      }
    }

    for (size_t i = 0; i < gSourceCount; ++i)
    {
      auto* src = mSources[i];
      src->EndBlock();
    }
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  float SelectStreamValue(OutputStream s, float masterValue)
  {
    switch (s)
    {
      default:
      case OutputStream::Master:
        return masterValue;

      case OutputStream::LFO1:
        return this->mMaj7Voice[0]->mpLFOs[0]->mNode.GetLastSample();
      case OutputStream::LFO2:
        return this->mMaj7Voice[0]->mpLFOs[1]->mNode.GetLastSample();
      case OutputStream::LFO3:
        return this->mMaj7Voice[0]->mpLFOs[2]->mNode.GetLastSample();
      case OutputStream::LFO4:
        return this->mMaj7Voice[0]->mpLFOs[3]->mNode.GetLastSample();
      case OutputStream::ModMatrix_NormRecalcSample:
        return float(this->mMaj7Voice[0]->mModMatrix.mnSampleCount) / GetModulationRecalcSampleMask();
      case OutputStream::LFO1_NormRecalcSample:
        return float(this->mMaj7Voice[0]->mpLFOs[0]->mNode.GetSamplesSinceRecalc()) /
               GetAudioOscillatorRecalcSampleMask();
      case OutputStream::ModSource_LFO1:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO1);
      case OutputStream::ModSource_LFO2:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO2);
      case OutputStream::ModSource_LFO3:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO3);
      case OutputStream::ModSource_LFO4:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO4);
      case OutputStream::ModDest_Osc1PreFMVolume:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1PreFMVolume);
      case OutputStream::ModDest_Osc2PreFMVolume:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2PreFMVolume);
      case OutputStream::ModDest_Osc3PreFMVolume:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3PreFMVolume);
      case OutputStream::ModDest_Osc4PreFMVolume:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4PreFMVolume);
      case OutputStream::ModDest_FMFeedback1:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FMFeedback);
      case OutputStream::ModDest_FMFeedback2:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2FMFeedback);
      case OutputStream::ModDest_FMFeedback3:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3FMFeedback);
      case OutputStream::ModDest_FMFeedback4:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4FMFeedback);
      case OutputStream::ModDest_Osc1Freq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FrequencyParam);
      case OutputStream::ModDest_Osc2Freq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FrequencyParam);
      case OutputStream::ModDest_Osc3Freq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3FrequencyParam);
      case OutputStream::ModDest_Osc4Freq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4FrequencyParam);
      case OutputStream::ModDest_Osc1SyncFreq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1SyncFrequency);
      case OutputStream::ModDest_Osc2SyncFreq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2SyncFrequency);
      case OutputStream::ModDest_Osc3SyncFreq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3SyncFrequency);
      case OutputStream::ModDest_Osc4SyncFreq:
        return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4SyncFrequency);

      case OutputStream::ModSource_Osc1AmpEnv:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc1AmpEnv);
      case OutputStream::ModSource_Osc2AmpEnv:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc2AmpEnv);
      case OutputStream::ModSource_Osc3AmpEnv:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc3AmpEnv);
      case OutputStream::ModSource_Osc4AmpEnv:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc4AmpEnv);
      case OutputStream::ModSource_ModEnv1:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::ModEnv1);
      case OutputStream::ModSource_ModEnv2:
        return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::ModEnv2);
      case OutputStream::Osc1:
        return this->mMaj7Voice[0]->mpOscillatorNodes[0]->GetLastSample();
      case OutputStream::Osc2:
        return this->mMaj7Voice[0]->mpOscillatorNodes[1]->GetLastSample();
      case OutputStream::Osc3:
        return this->mMaj7Voice[0]->mpOscillatorNodes[2]->GetLastSample();
      case OutputStream::Osc4:
        return this->mMaj7Voice[0]->mpOscillatorNodes[3]->GetLastSample();
    }
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


  struct Maj7Voice : public Voice
  {
    Maj7Voice(Maj7* owner)
        : mpOwner(owner)
        , mPortamento(owner->mParamCache)
      , mFilter{FilterAuxNode(owner->mParamCache, GigaSynthParamIndices::Filter1Enabled),
            FilterAuxNode(owner->mParamCache, GigaSynthParamIndices::Filter1Enabled)}
    {
      //for (int ich = 0; ich < 2; ++ich)
      //{
      //  for (int ifilt = 0; ifilt < 2; ++ifilt)
      //  {
      //    mpFilters[ifilt][ich] = new FilterAuxNode(
      //        owner->mParamCache,
      //        (GigaSynthParamIndices)((int)GigaSynthParamIndices::Filter1Enabled +
      //                                (int)FilterParamIndexOffsets::Count * ifilt),
      //        (ModDestination)((int)ModDestination::Filter1Freq + ((int)FilterAuxModDestOffsets::Count * ifilt)));
      //  }
      //}

      for (int i = 0; i < (gSourceCount + gModEnvCount); ++i)
      {
        mpEnvelopes[i] = new EnvelopeNode{mModMatrix, owner->mParamCache, gEnvelopeInfo[i]};
      }

#ifdef ENABLE_SAMPLER_DEVICE
      static_assert(gModLFOCount == gOscillatorCount && gOscillatorCount == gSamplerCount, "meecro optimizyshunz");
#else
      static_assert(gModLFOCount == gOscillatorCount, "meecro optimizyshunz");
#endif // ENABLE_SAMPLER_DEVICE
      for (int i = 0; i < gModLFOCount; ++i)
      {
        mpLFOs[i] = new LFOVoice{*mpOwner->mpLFOs[i], mModMatrix};

        mSourceVoices[i] = mpOscillatorNodes[i] =
            new OscillatorNode(owner->mpOscillatorDevices[i], OscillatorIntention::Audio, &mModMatrix, mpEnvelopes[i]);
#ifdef ENABLE_SAMPLER_DEVICE
        mSourceVoices[i + gOscillatorCount] = mpSamplerVoices[i] = new SamplerVoice(mModMatrix,
                                                                                    owner->mpSamplerDevices[i],
                                                                                    mpEnvelopes[i + gOscillatorCount]);
#endif  // ENABLE_SAMPLER_DEVICE
      }
    }
    ~Maj7Voice()
    {
#ifdef MIN_SIZE_REL
  #pragma message("Maj7Voice::~Maj7Voice() Leaking memory to save bits.")
#else
      for (int i = 0; i < (gSourceCount + gModEnvCount); ++i)
      {
        delete mpEnvelopes[i];
      }

      for (int i = 0; i < gModLFOCount; ++i)
      {
        delete mpLFOs[i];
        delete mpOscillatorNodes[i];
#ifdef ENABLE_SAMPLER_DEVICE
        delete mpSamplerVoices[i];
#endif  // ENABLE_SAMPLER_DEVICE
      }
#endif  // MIN_SIZE_REL
    }

    Maj7* mpOwner;
    ModMatrixNode mModMatrix;

    real_t mVelocity01 = 0;
    real_t mTriggerRandom01 = 0;
    real_t mTrigger01 = 0;
    float mMidiNote = 0;

    PortamentoCalc mPortamento;

    struct LFOVoice
    {
      explicit LFOVoice(LFODevice& device, ModMatrixNode& modMatrix);
      LFODevice& mDevice;
      OscillatorNode mNode;
      VanillaOnePoleFilter mFilter;
    };

    FilterAuxNode mFilter[2];
    LFOVoice* mpLFOs[gModLFOCount];

    // first source envs, then mod envs.
    EnvelopeNode* mpEnvelopes[gSourceCount + gModEnvCount];
    OscillatorNode* mpOscillatorNodes[gOscillatorCount];
#ifdef ENABLE_SAMPLER_DEVICE
    SamplerVoice* mpSamplerVoices[gSamplerCount];
#endif  // ENABLE_SAMPLER_DEVICE

    ISoundSourceDevice::Voice* mSourceVoices[gSourceCount];

    // K-rate cache for per-source output gains (pan * volume) including global
    FloatPair mOutputGainsCached[gSourceCount];
    bool mOutputGainsInitialized = false;

    // K-rate cache for LFO samples
    float mLFOSampleCached[gModLFOCount]{};
    bool mLFOInitialized = false;
    // Per-block usage/enabled caches
    bool mSourceEnabledCache[gSourceCount]{};  // mirrors device enabled state
    bool mLFOUsedCache[gModLFOCount]{};        // true if any enabled modulation references this LFO


    virtual void Kill(VoiceNoteOnFlags flags) override
    {
      if (!HasFlag(flags, VoiceNoteOnFlags::VoiceSteal))
      {
        for (auto& p : mpEnvelopes)
        {
          p->KillEnvelope();
        }
      }

      mModMatrix.Recalc();

      for (auto& p : mpLFOs)
      {
        p->mNode.ClearState();
        p->mFilter.Reset();
      }

      for (auto* p : mpOscillatorNodes)
      {
        p->ClearState();
      }

#ifdef ENABLE_SAMPLER_DEVICE
      for (auto* p : mpSamplerVoices)
      {
        p->ClearState();
      }
#endif  // ENABLE_SAMPLER_DEVICE

      // Don't reset filters to avoid clicks. Maybe this could be added
      // for panic, but not general.
      //for (auto* a : mpFilters)
      //{
      //  for (int ich = 0; ich < 2; ++ich)
      //  {
      //    a[ich]->mFilter.ResetState();
      //  }
      //}

      mMidiNote = 0;
      mOutputGainsInitialized = false;
      mLFOInitialized = false;
    }

    void BeginBlock(bool forceProcessing)
    {
      for (size_t i = 0; i < gSourceCount; ++i)
      {
        auto* srcVoice = mSourceVoices[i];
        bool enabled = srcVoice->mpSrcDevice->IsEnabled();
        mSourceEnabledCache[i] = enabled;
        if (!enabled)
        {
          srcVoice->mpAmpEnv->KillEnvelope();
          continue;
        }
      }

      for (auto& p : mpEnvelopes)
      {
        p->BeginBlock();
      }

      if (!forceProcessing && !this->IsPlaying())
      {
        return;
      }

      // we know we are a valid and playing voice; set master LFO to use this mod matrix.
      for (auto& lfo : this->mpOwner->mpLFOs)
      {
        lfo->mPhase.SetModMatrix(&this->mModMatrix);
      }

#ifdef ENABLE_PITCHBEND
      mMidiNote = mPortamento.GetCurrentMidiNote() +
                  mpOwner->mParams.GetIntValue(GigaSynthParamIndices::PitchBendRange) * mpOwner->mPitchBendN11;
#else
      mMidiNote = mPortamento.GetCurrentMidiNote();
#endif  // ENABLE_PITCHBEND

      real_t noteHz = math::MIDINoteToFreq(mMidiNote);

      mModMatrix.BeginBlock();
      mModMatrix.SetSourceValue(ModSource::Velocity, mVelocity01);
      mModMatrix.SetSourceValue(ModSource::NoteValue, mMidiNote / 127.0f);
      mModMatrix.SetSourceValue(ModSource::RandomTrigger, mTriggerRandom01);
      mModMatrix.SetSourceValue(ModSource::Trigger01, mTrigger01);
      float iuv = 1;
      if (mpOwner->mVoicesUnisono > 1)
      {
        iuv = float(mUnisonVoice) / (mpOwner->mVoicesUnisono - 1);
      }
      mModMatrix.SetSourceValue(ModSource::UnisonoVoice, iuv);

      for (size_t iMacro = 0; iMacro < gMacroCount; ++iMacro)
      {
        mModMatrix.SetSourceValue((int)ModSource::Macro1 + iMacro,
                                  mpOwner->mParams.Get01Value((int)GigaSynthParamIndices::Macro1 + iMacro,
                                                              0));  // krate, 01
      }

      for (size_t i = 0; i < gModLFOCount; ++i)
      {
        auto& lfo = *mpLFOs[i];
        if (!lfo.mDevice.mDevice.GetPhaseRestart())
        {
          // phase restart means per-voice phases.
          // but otherwise, this lets the voice-level LFOs appear to be one unified LFO. Almost always this would be a NOP.
          // sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
          //lfo.mNode.SetPhase(lfo.mDevice.mPhase.mPhase);
          lfo.mNode.SynchronizePhase(lfo.mDevice.mPhase);
        }
        lfo.mNode.BeginBlock();

        auto freq = lfo.mDevice.mDevice.mParams.GetFrequency(LFOParamIndexOffsets::Sharpness,
                                                             -1,
                                                             gLFOLPFreqConfig,
                                                             0,
                                                             mModMatrix.GetDestinationValue(
                                                                 (int)lfo.mDevice.mDevice.mModDestBaseID +
                                                                 (int)LFOModParamIndexOffsets::Sharpness));

        lfo.mFilter.SetParams(
                              FilterResponse::Lowpass,
                              freq,
                              Param01{0} /*reso*/,
                              0 /*gaindb*/);
      }

      //for (auto* a : mpFilters)
      {
        for (int ich = 0; ich < 2; ++ich)
        {
          //a[ich]->AuxBeginBlock(noteHz, mModMatrix);
          mFilter[ich].AuxBeginBlock(noteHz, mModMatrix);
        }
      }

      //float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];

      for (size_t i = 0; i < gSourceCount; ++i)
      {
        auto* srcVoice = mSourceVoices[i];

        srcVoice->BeginBlock();
      }

      // Ensure output/master pan gains get recomputed on the first sample of processing
      mOutputGainsInitialized = false;
      mLFOInitialized = false;

      // Recompute per-block usage caches (LFO usage)
      for (size_t i = 0; i < gModLFOCount; ++i)
        mLFOUsedCache[i] = false;
      for (int imod = 0; imod < (int)std::size(mpOwner->mpModulations); ++imod)
      {
        auto* m = mpOwner->mpModulations[imod];
        if (!m->mParams.GetBoolValue(ModParamIndexOffsets::Enabled))
          continue;
        auto srcMain = m->mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::Source);
        bool auxEnabled = m->mParams.GetBoolValue(ModParamIndexOffsets::AuxEnabled);
        auto srcAux = auxEnabled ? m->mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::AuxSource)
                                 : ModSource::None;
        for (size_t i = 0; i < gModLFOCount; ++i)
        {
          auto lfoSrc = mpOwner->mpLFOs[i]->mInfo.mModSource;
          if (srcMain == lfoSrc || srcAux == lfoSrc)
          {
            mLFOUsedCache[i] = true;
          }
        }
      }
    }

    inline bool IsKRateBoundary() const
    {
      return (mModMatrix.mnSampleCount & GetModulationRecalcSampleMask()) == 0;
    }

    // Recompute per-source output gains (pan * volume) at K-rate
    inline void UpdateOutputGainsIfNeeded(float myUnisonoPan)
    {
      if (!mOutputGainsInitialized || IsKRateBoundary())
      {
        for (size_t i = 0; i < gSourceCount; ++i)
        {
          auto* const srcVoice = mSourceVoices[i];
          auto* const dev = srcVoice->mpSrcDevice;

          float volumeMod = mModMatrix.GetDestinationValue(
              AddEnum(dev->mModDestBaseID, SourceModParamIndexOffsets::Volume));
          float volumeLin = dev->mParams.GetLinearVolume(SourceParamIndexOffsets::Volume, gUnityVolumeCfg, volumeMod);
          //float compGainLin = dev->mParams.GetLinearVolume(SourceParamIndexOffsets::CompensationGain, gVolumeCfg12db);
          //volumeLin *= compGainLin;

          float masterPanN11 = 2 * mpOwner->mParams.GetN11Value(GigaSynthParamIndices::Pan,
                                                            mModMatrix.GetDestinationValue(ModDestination::Pan));

          float panMod = mModMatrix.GetDestinationValue(AddEnum(dev->mModDestBaseID, SourceModParamIndexOffsets::Pan));
          float panN11 = dev->mParams.GetN11Value(SourceParamIndexOffsets::Pan, panMod + myUnisonoPan + masterPanN11);

          auto panLin = M7::math::PanToFactor(panN11);
          mOutputGainsCached[i] = panLin.mul(volumeLin);
        }
        mOutputGainsInitialized = true;
      }
    }

    // K-rate LFO update: compute on boundary, otherwise advance phase cheaply and reuse cached value
    inline void UpdateLFOsIfNeeded()
    {
      if (!mLFOInitialized || IsKRateBoundary())
      {
        for (size_t i = 0; i < gModLFOCount; ++i)
        {
          if (!mLFOUsedCache[i])
          {
            // If not used, ensure published value is zero.
            mLFOSampleCached[i] = 0.0f;
            continue;
          }
          auto& lfo = *mpLFOs[i];
          float lfoSample = lfo.mNode.RenderSampleForLFOAndAdvancePhase(false);
          // filter at K-rate; acceptable for modulation smoothing
          lfoSample = lfo.mFilter.ProcessSample(lfoSample);
          mLFOSampleCached[i] = lfoSample;
        }
        mLFOInitialized = true;
      }
      else
      {
        // advance phase without producing a new sample value for used LFOs only
        for (size_t i = 0; i < gModLFOCount; ++i)
        {
          if (!mLFOUsedCache[i])
            continue;
          auto& lfo = *mpLFOs[i];
          lfo.mNode.RenderSampleForLFOAndAdvancePhase(true);
        }
      }

      // publish cached samples into mod matrix (0 for unused LFOs)
      for (size_t i = 0; i < gModLFOCount; ++i)
      {
        auto& lfo = *mpLFOs[i];
        mModMatrix.SetSourceValue(lfo.mDevice.mInfo.mModSource, mLFOSampleCached[i]);
      }
    }

    void ProcessAndMix(float* s, bool forceProcessing)
    {
      // NB: process envelopes before short-circuiting due to being not playing.
      // this is for issue#31; mod envelopes need to be able to release down to 0 even when the source envs are not playing.
      // if mod envs get suspended rudely, then they'll "wake up" at the wrong value.
      for (auto& env : mpEnvelopes)
      {
#ifndef MIN_SIZE_REL  // optimization
        if (!env->IsPlaying())
          continue;
#endif
        float l = env->ProcessSample();
        mModMatrix.SetSourceValue(env->mMyModSource, l);
      }

      if (!forceProcessing && !this->IsPlaying())
      {
        return;
      }

      // LFOs at K-rate
      UpdateLFOsIfNeeded();

      // process modulations here. sources have just been set, and past here we're getting many destination values.
      // processing here ensures fairly up-to-date accurate values.
      // one area where this is quite sensitive is envelopes with instant attacks/releases
      mModMatrix.ProcessSample(mpOwner->mpModulations);  // this sets dest values to 0.

      float globalFMScale = 3 *
                            mpOwner->mParams.Get01Value(GigaSynthParamIndices::FMBrightness,
                                                        mModMatrix.GetDestinationValue(ModDestination::FMBrightness));

      float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
      float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];

      UpdateOutputGainsIfNeeded(myUnisonoPan);

      float sourceValues[gOscillatorCount];  // required for FM to hold all source values
      //float detuneMul[gSourceCount];         // = { 0 };
      float det = math::SemisToFrequencyMul(myUnisonoDetune);
      float ampEnvGains[gSourceCount];

      FloatPair mixedSources{0, 0};
      static constexpr float kAmpSilent = 1e-6f;

      for (size_t i = 0; i < gSourceCount; ++i)
      {
        auto* const srcVoice = mSourceVoices[i];
        auto* const dev = srcVoice->mpSrcDevice;

        // the amp envelope gain...
        float hiddenVolumeBacking = mModMatrix.GetDestinationValue(
            AddEnum(dev->mModDestBaseID, SourceModParamIndexOffsets::HiddenVolume));
        ParamAccessor hiddenAmpParam{&hiddenVolumeBacking, 0};
        float ampEnvGain = ampEnvGains[i] = hiddenAmpParam.GetLinearVolume(0, gUnityVolumeCfg);

        if (!mSourceEnabledCache[i] || ampEnvGain <= kAmpSilent)
        {
          if (i < gOscillatorCount)
          {
            // ensure FM reads zero from disabled/quiet oscillators
            sourceValues[i] = 0.0f;
          }
          // skip sampler processing and mixing when disabled/quiet
          continue;
        }

#ifdef ENABLE_SAMPLER_DEVICE
        if (i >= gOscillatorCount)  // if sampler, process sample here while it's fresh
        {
          auto ps = static_cast<SamplerVoice*>(srcVoice);
          float s = ps->ProcessSample(mMidiNote, det, 0, ampEnvGain);
          mixedSources.Accumulate(mOutputGainsCached[i].mul(s));
        }
        else
#endif  // ENABLE_SAMPLER_DEVICE
        {
          auto po = static_cast<OscillatorNode*>(srcVoice);
          // #127 must apply amp gain to last samples.
          sourceValues[i] = po->GetLastSample() * ampEnvGain;
        }
      }

      // cannot do this in previous loop because we need all the source values in advance.
      for (int i = 0; i < gOscillatorCount; ++i)
      {
        if (!mSourceEnabledCache[i] || ampEnvGains[i] <= kAmpSilent)
        {
          continue;
        }
        auto* po = mpOscillatorNodes[i];
        float s = po->RenderSampleForAudioAndAdvancePhase(
            mMidiNote, det, globalFMScale, mpOwner->mParams, sourceValues, i, ampEnvGains[i]);
        mixedSources.Accumulate(mOutputGainsCached[i].mul(s));
      }

      for (size_t ich = 0; ich < 2; ++ich)
      {
        //for (size_t ifilter = 0; ifilter < gFilterCount; ++ifilter)
        {
          //mixedSources.x[ich] = mpFilters[ifilter][ich]->AuxProcessSample(mixedSources.x[ich]);
          mixedSources.x[ich] = mFilter[ich].AuxProcessSample(mixedSources.x[ich]);
        }
        s[ich] += mixedSources[ich];
      }

      mPortamento.Advance(1, mModMatrix.GetDestinationValue(ModDestination::PortamentoTime));
    }

    virtual void NoteOn(VoiceNoteOnFlags flags) override
    {
      const auto legato = HasFlag(flags, VoiceNoteOnFlags::Legato);
      if (!legato)
      {
        Kill(flags);
      }

      mVelocity01 = mNoteInfo.Velocity / 127.0f;
      mTriggerRandom01 = math::rand01();
      mTrigger01 = mNoteInfo.mSequence % 2 == 0 ? 1.0f : 0.0f;

      // don't process all envelopes because some have keyranges to respect.
      // only process mod envs.
      for (int i = gSourceCount; i < (gSourceCount + gModEnvCount); ++i)
      {
        mpEnvelopes[i]->EnvelopeNoteOn(flags);
      }

      for (auto& p : mpLFOs)
      {
        p->mNode.NoteOn(legato);
      }

      for (auto& srcVoice : mSourceVoices)
      {
        if (!srcVoice->mpSrcDevice->MatchesKeyRange(mNoteInfo.MidiNoteValue))
          continue;
        srcVoice->NoteOn(legato);
        srcVoice->mpAmpEnv->EnvelopeNoteOn(flags);
      }
      mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !legato);
    }

    virtual void NoteOff() override
    {
      for (auto& srcVoice : mSourceVoices)
      {
        srcVoice->NoteOff();
      }

      for (auto& p : mpEnvelopes)
      {
        p->EnvelopeNoteOff();
      }
    }

    virtual bool IsPlaying() override
    {
      for (auto& srcVoice : mSourceVoices)
      {
        // if the voice is not enabled, its env won't be playing.
        if (srcVoice->mpAmpEnv->IsPlaying())
          return true;
      }
      return false;
    }
  };  // Maj7Voice

  struct Maj7Voice* mMaj7Voice[gMaxMaxVoices];  // = { 0 };
};
}  // namespace M7

using Maj7 = M7::Maj7;

}  // namespace WaveSabreCore
