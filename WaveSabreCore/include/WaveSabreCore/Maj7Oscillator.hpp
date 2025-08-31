#pragma once

#include "Maj7OscillatorWaveforms.hpp"
#include <WaveSabreCore/Filters/FilterOnePole.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

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

//
//struct OscillatorNodeX : ISoundSourceDevice::Voice
//{
//  OscillatorDevice* mpOscDevice = nullptr;
//
//  size_t mnSamples = 0;
//
//  // voice-level state
//  double mPhase = 0;
//  double mPhaseIncrement = 0;  // DT
//  float mCurrentSample = 0;
//  float mOutSample = 0;
//  float mPrevSample = 0;
//  float mCurrentFreq = 0;
//
//  float mPhaseModVal = 0;
//
//  IOscillatorWaveform* mpWaveforms[(int)OscillatorWaveform::Count];
//
//  IOscillatorWaveform* mpSlaveWave;  // = &mSawClipWaveform;
//
//  OscillatorNodeX(ModMatrixNode* pModMatrix, OscillatorDevice* pOscDevice, EnvelopeNode* pAmpEnv)
//      : ISoundSourceDevice::Voice(pOscDevice, pModMatrix, pAmpEnv)
//      , mpOscDevice(pOscDevice)
//  {
//    mpSlaveWave = mpWaveforms[(int)OscillatorWaveform::Pulse] = new PulsePWMWaveform;
//    mpWaveforms[(int)OscillatorWaveform::PulseTristate] = new PulseTristateWaveform;
//    mpWaveforms[(int)OscillatorWaveform::SawClip] = new NotchSawWaveform;
//    mpWaveforms[(int)OscillatorWaveform::SineClip] = new SineClipWaveform;
//    mpWaveforms[(int)OscillatorWaveform::SineHarmTrunc] = new SineHarmTruncWaveform;
//    //				mpWaveforms[(int)OscillatorWaveform::TriClip__obsolete] = ;// new SineHarmTruncWaveform;
//    mpWaveforms[(int)OscillatorWaveform::TriSquare] = new TriSquareWaveform;
//    mpWaveforms[(int)OscillatorWaveform::TriTrunc] = new TriTruncWaveform;
//    mpWaveforms[(int)OscillatorWaveform::VarTrapezoid] = new VarTrapezoidWaveform;
//    //mpWaveforms[(int)OscillatorWaveform::VarTriangle] = mpWaveforms[(int)OscillatorWaveform::TriClip__obsolete] = new VarTriWaveform;
//    mpWaveforms[(int)OscillatorWaveform::WhiteNoiseSH] = new WhiteNoiseWaveform;
//    mpWaveforms[(int)OscillatorWaveform::SineRectified] = new RectifiedSineWaveform;
//    mpWaveforms[(int)OscillatorWaveform::SinePhaseDist] = new PhaseDistortedSineWaveform;
//    mpWaveforms[(int)OscillatorWaveform::StaircaseSaw] = new StaircaseSawWaveform;
//    mpWaveforms[(int)OscillatorWaveform::TriFold] = new TriFoldWaveform;
//    mpWaveforms[(int)OscillatorWaveform::DoublePulse] = new DoublePulseWaveform;
//  }
//
//  ~OscillatorNodeX()
//  {
//#ifdef MIN_SIZE_REL
//  #pragma message("OscillatorNode::~OscillatorNode() Leaking memory to save bits.")
//#else
//    // careful: some pointers are reused so care to avoid double delete.
//
//    delete (PulsePWMWaveform*)mpWaveforms[(int)OscillatorWaveform::Pulse];
//    delete (PulseTristateWaveform*)mpWaveforms[(int)OscillatorWaveform::PulseTristate];
//    delete (NotchSawWaveform*)mpWaveforms[(int)OscillatorWaveform::SawClip];
//    delete (SineClipWaveform*)mpWaveforms[(int)OscillatorWaveform::SineClip];
//    delete (SineHarmTruncWaveform*)mpWaveforms[(int)OscillatorWaveform::SineHarmTrunc];
//    delete (TriSquareWaveform*)mpWaveforms[(int)OscillatorWaveform::TriSquare];
//    delete (TriTruncWaveform*)mpWaveforms[(int)OscillatorWaveform::TriTrunc];
//    delete (VarTrapezoidWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTrapezoid];
//    //delete (VarTrapezoidWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTrapezoidSoft];
//    //delete (VarTriWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTriangle];
//    delete (WhiteNoiseWaveform*)mpWaveforms[(int)OscillatorWaveform::WhiteNoiseSH];
//    delete (RectifiedSineWaveform*)mpWaveforms[(int)OscillatorWaveform::SineRectified];
//    delete (PhaseDistortedSineWaveform*)mpWaveforms[(int)OscillatorWaveform::SinePhaseDist];
//    delete (StaircaseSawWaveform*)mpWaveforms[(int)OscillatorWaveform::StaircaseSaw];
//    delete (TriFoldWaveform*)mpWaveforms[(int)OscillatorWaveform::TriFold];
//    delete (DoublePulseWaveform*)mpWaveforms[(int)OscillatorWaveform::DoublePulse];
//
//#endif  // MIN_SIZE_REL
//  }
//
//  float GetLastSample() const
//  {
//    return mOutSample;
//  }
//
//  // used by LFOs to just hard-set the phase. usually NOP
//  void SynchronizePhase(const OscillatorNodeX& src)
//  {
//    mPhase = src.mPhase;
//    mpSlaveWave->mPhase = mPhase;
//  }
//
//
//  virtual void NoteOn(bool legato) override
//  {
//    if (legato)
//      return;
//    if (mpOscDevice->GetPhaseRestart())
//    {
//      mpSlaveWave->OSC_RESTART(0);
//      mPhase = GetPhaseOffset();  // math::fract(f);
//    }
//  }
//
//  float GetPhaseOffset()
//  {
//    int po = mpOscDevice->mIntention == OscillatorIntention::Audio ? (int)OscParamIndexOffsets::PhaseOffset
//                                                                   : (int)LFOParamIndexOffsets::PhaseOffset;
//    float f = mpOscDevice->mParams.GetN11Value(po, mPhaseModVal);
//    return math::fract(f);
//  }
//
//  virtual void NoteOff() override {}
//
//  virtual void BeginBlock() override
//  {
//    if (!this->mpSrcDevice->IsEnabled())
//    {
//      return;
//    }
//    mnSamples = 0;  // ensure reprocessing after setting these params to avoid corrupt state.
//    auto w = mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(mpOscDevice->mIntention == OscillatorIntention::Audio
//                                                                       ? (int)OscParamIndexOffsets::Waveform
//                                                                       : (int)LFOParamIndexOffsets::Waveform);
//    mpSlaveWave = mpWaveforms[math::ClampI((int)w, 0, (int)OscillatorWaveform::Count - 1)];
//  }
//
//  real_t ProcessSampleForAudio(real_t midiNote,
//                               float detuneFreqMul,
//                               float fmScale,
//                               const ParamAccessor& globalParams,
//                               float const* otherSignals,
//                               int iosc,
//                               float ampEnvLin)
//  {
//    const ParamAccessor& params = mpSrcDevice->mParams;
//    if (!this->mpSrcDevice->mEnabledCache)
//    {
//      mOutSample = mCurrentSample = 0;
//      return 0;
//    }
//
//    bool syncEnable = mpOscDevice->mParams.GetBoolValue(OscParamIndexOffsets::SyncEnable);
//
//    float mSyncFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                             (int)OscModParamIndexOffsets::SyncFrequency);
//    float mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                         (int)OscModParamIndexOffsets::FrequencyParam);
//    float mFMFeedbackModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                               (int)OscModParamIndexOffsets::FMFeedback);
//    float mWaveShapeAModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                               (int)OscModParamIndexOffsets::WaveshapeA);
//    float mWaveShapeBModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                               (int)OscModParamIndexOffsets::WaveshapeB);
//    mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                    (int)OscModParamIndexOffsets::Phase);
//    float mPitchFineModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                              (int)OscModParamIndexOffsets::PitchFine);
//    float mFMFeedbackAmt = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::FMFeedback, mFMFeedbackModVal) *
//                           fmScale * 0.5f;
//
//    if (0 == mnSamples)  // NOTE: important that this is designed to be 0 the first run to force initial calculation.
//    {
//      // - osc pitch semis                  note         oscillator
//      // - osc fine (semis)                 note         oscillator
//      // - osc sync freq / kt (Hz)          hz           oscillator
//      // - osc freq / kt (Hz)               hz           oscillator
//      // - osc mul (Hz)                     hz           oscillator
//      // - osc detune (semis)               hz+semis     oscillator
//      // - unisono detune (semis)           hz+semis     oscillator
//      int pitchSemis = params.GetIntValue(OscParamIndexOffsets::PitchSemis, gSourcePitchSemisRange);
//      float pitchFine = params.GetN11Value(OscParamIndexOffsets::PitchFine, mPitchFineModVal) *
//                        gSourcePitchFineRangeSemis;
//      midiNote +=
//          pitchSemis +
//          pitchFine;  // (mpSrcDevice->mPitchFineParam.mCachedVal + mPitchFineModVal)* gSourcePitchFineRangeSemis;
//      float noteHz = math::MIDINoteToFreq(midiNote);
//      float freq = params.GetFrequency(OscParamIndexOffsets::FrequencyParam,
//                                       OscParamIndexOffsets::FrequencyParamKT,
//                                       gSourceFreqConfig,
//                                       noteHz,
//                                       mFreqModVal);  //mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
//      freq *= mpOscDevice->mParams.GetScaledRealValue(OscParamIndexOffsets::FreqMul,
//                                                      0.0f,
//                                                      gFrequencyMulMax,
//                                                      0);  //mpOscDevice->mFrequencyMul.mCachedVal;// .GetRangedValue();
//      freq *= detuneFreqMul;
//      // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
//      freq = std::max(freq, 0.001f);
//      mCurrentFreq = freq;
//
//      double newDT = (double)freq / Helpers::CurrentSampleRate;
//      mPhaseIncrement = newDT;
//
//      float slaveFreq = syncEnable ? mpOscDevice->mParams.GetFrequency(OscParamIndexOffsets::SyncFrequency,
//                                                                       OscParamIndexOffsets::SyncFrequencyKT,
//                                                                       gSyncFreqConfig,
//                                                                       noteHz,
//                                                                       mSyncFreqModVal)
//                                   : freq;  // mpOscDevice->mSyncFrequency.GetFrequency(noteHz, mSyncFreqModVal) : freq;
//      float waveshapeA = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeA, mWaveShapeAModVal);
//      float waveshapeB = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeB, mWaveShapeBModVal);
//      mpSlaveWave->SetParams(
//          slaveFreq, GetPhaseOffset(), waveshapeA, waveshapeB, Helpers::CurrentSampleRate, OscillatorIntention::Audio);
//    }
//
//    mnSamples = (mnSamples + 1) & GetAudioOscillatorRecalcSampleMask();
//
//    mPrevSample = mCurrentSample;  // THIS sample.
//    mCurrentSample = 0;            // a value that gets added to the next sample
//
//    // why do we have 2 phase accumulators? because of hard sync support, and because they have different functions
//    // though when hard sync is not enabled that difference is not very obvious.
//    // mPhase / mPhaseIncrement are the master phase, bound by frequency.
//    // mSlaveWave holds the slave phase, bound by sync frequency.
//
//    // Push master phase forward by full sample.
//    mPhase = math::fract(mPhase + mPhaseIncrement);
//
//    float phaseMod = mPrevSample * mFMFeedbackAmt;
//    int otherIndex = -1;
//    for (int i = 0; i < gOscillatorCount - 1; ++i)
//    {
//      int off = iosc * (gOscillatorCount - 1) + i;
//      ParamIndices amtParam = ParamIndices((int)ParamIndices::FMAmt2to1 + off);
//      ModDestination amtMod = ModDestination((int)ModDestination::FMAmt2to1 + off);
//      float amt = globalParams.Get01Value(amtParam, mpModMatrix->GetDestinationValue(amtMod));
//
//      otherIndex += (i == iosc) ? 2 : 1;
//      phaseMod += otherSignals[otherIndex] * amt * fmScale;
//    }
//
//    mpSlaveWave->mBlepAfter = 0;
//    mpSlaveWave->mBlepBefore = 0;
//
//    if (mPhase >= mPhaseIncrement || !syncEnable)  // did not cross cycle. advance 1 sample
//    {
//      mpSlaveWave->OSC_ADVANCE(1, 0);
//    }
//    else
//    {
//      float x = float(mPhase / mPhaseIncrement);  // sample overshoot, in samples.
//      mpSlaveWave->OSC_ADVANCE(1 - x, x);         // the amount before the cycle boundary
//      mpSlaveWave->OSC_RESTART(x);                // notify of cycle crossing
//      mpSlaveWave->OSC_ADVANCE(x, 0);             // and advance after the cycle begin
//    }
//
//    mPrevSample += mpSlaveWave->mBlepBefore;
//    mCurrentSample += mpSlaveWave->mBlepAfter;
//    // current sample will be used on next sample (this is the 1-sample delay)
//    mCurrentSample += mpSlaveWave->NaiveSample(float(mpSlaveWave->mPhase + phaseMod));
//    mCurrentSample = math::clampN11(mCurrentSample);  // prevent FM from going crazy.
//    mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale * gOscillatorHeadroomScalar * ampEnvLin;
//
//    return mOutSample;
//  }  // process sample for audio
//
//
//  real_t ProcessSampleForLFO(bool forceSilence)
//  {
//    float mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                         (int)LFOModParamIndexOffsets::FrequencyParam);
//    float mWaveShapeAModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                               (int)LFOModParamIndexOffsets::WaveshapeA);
//    float mWaveShapeBModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                               (int)LFOModParamIndexOffsets::WaveshapeB);
//    mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
//                                                    (int)LFOModParamIndexOffsets::Phase);
//    if (!mnSamples)  // NOTE: important that this is designed to be 0 the first run to force initial calculation.
//    {
//      float freq =
//          mpSrcDevice->mParams.GetFrequency(LFOParamIndexOffsets::FrequencyParam, -1, gLFOFreqConfig, 0, mFreqModVal);
//      // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
//      freq = std::max(freq, 0.001f);
//      mCurrentFreq = freq;
//
//      float waveshapeA = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeA, mWaveShapeAModVal);
//      float waveshapeB = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeB, mWaveShapeBModVal);
//      mpSlaveWave->SetParams(
//          freq, GetPhaseOffset(), waveshapeA, waveshapeB, Helpers::CurrentSampleRate, OscillatorIntention::LFO);
//
//      double newDT = (double)freq / Helpers::CurrentSampleRate;
//      mPhaseIncrement = newDT;
//    }
//    mnSamples = (mnSamples + 1) & GetModulationRecalcSampleMask();
//
//    mPhase = math::fract(mPhase + mPhaseIncrement);
//
//    if (forceSilence)
//    {
//      mOutSample = mCurrentSample = 0;
//      return 0;
//    }
//
//    mpSlaveWave->OSC_ADVANCE(1, 0);  // advance phase
//    mOutSample = mCurrentSample = mpSlaveWave->NaiveSample(math::fract(float(mPhase + mPhaseModVal)));
//
//    return mOutSample;
//  }  // process sample for lfo
//};
//
//


}  // namespace M7


}  // namespace WaveSabreCore
