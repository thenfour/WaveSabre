#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Bandlimiting.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Waveshapes.hpp"
#include "Maj7Oscillator4WS.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace M7Osc4
{
struct SawGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    return MakeSawShape(shapeA, shapeB);
  }
};
struct TriGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    return MakeTriSquareShape(shapeA, shapeB);
  }
};
struct PulseGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float /*shapeB*/) const override
  {
    return MakePulseShape(shapeA);
  }
};

struct TriPulseGenerator1 : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    return MakeTriStatePulseShape3(shapeA, shapeB);
  }
};

struct TriPulseGenerator2 : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    // shapeA = pulse width (0..1)
    // shapeB defines the low & high duty cycles. when shapeB = 0.5, both are 0.5. when shapeB = 0, low=0, high=1; when shapeB=1, low=1, high=0
    double lowDuty01 = shapeA;
    double highDuty01 = 1.0 - shapeA;
    return MakeTriStatePulseShape4(shapeB, lowDuty01, highDuty01);
  }
};

struct FoldedTriGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    const float drive = 1.0f + 15.0f * math::clamp01(shapeA);
    const float bias = (math::clamp01(shapeB) - 0.5f) * 4.0f;
    return MakeFoldedTriangleShape(drive, bias);
  }
};

}  // namespace M7Osc4

inline OscillatorCore* InstantiateWaveformCore(OscillatorWaveform w, OscillatorIntention intention)
{
  const M7Osc4::AntiAliasingOption aaOpt = (intention == OscillatorIntention::LFO) 
    ? M7Osc4::AntiAliasingOption::None 
    : M7Osc4::AntiAliasingOption::PolyBlep;

  switch (w)
  {
    default:
    case OscillatorWaveform::SineDCClip:
      return new SineCoreExt(OscillatorWaveform::SineDCClip, SineCoreExtVariant::DCClip);
    case OscillatorWaveform::SineClipSqueeze:
      return new SineCoreExt(OscillatorWaveform::SineClipSqueeze, SineCoreExtVariant::ClipSilence);

    case OscillatorWaveform::SineHarmDCClip:
      return new SineCoreExt(OscillatorWaveform::SineHarmDCClip,  SineCoreExtVariant::ClipHarm);
    case OscillatorWaveform::SineHarmClipSqueeze:
      return new SineCoreExt(OscillatorWaveform::SineHarmClipSqueeze, SineCoreExtVariant::HarmSilence);

    case OscillatorWaveform::ShapeCoreSawTri:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::SawGenerator);
    case OscillatorWaveform::ShapeCoreSawPulse2:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::PulseGenerator);
    case OscillatorWaveform::ShapeCoreSawPulse3:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::TriPulseGenerator1);
    case OscillatorWaveform::ShapeCoreSawPulse4:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::TriPulseGenerator2);
    case OscillatorWaveform::ShapeCoreSawTriSquare:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::TriGenerator);

    case OscillatorWaveform::FoldedSine:
      return new FoldedCore(OscillatorWaveform::FoldedSine, FoldedCore::Style::Sine_Fold_Bias);
    case OscillatorWaveform::FoldedTriangle:
      return new FoldedCore(OscillatorWaveform::FoldedTriangle, FoldedCore::Style::Tri_Fold_Bias);
    case OscillatorWaveform::FoldedTriBL:
      return new M7Osc4::ShapeCoreStreaming(w, aaOpt, new M7Osc4::FoldedTriGenerator);

      case OscillatorWaveform::EvolvingGrainNoise:
      return new EvolvingGrainNoiseCore(OscillatorWaveform::EvolvingGrainNoise);

    case OscillatorWaveform::Noise_SaH_LP4:
      return new SAHNoiseCore(OscillatorWaveform::Noise_SaH_LP4, SAHNoiseCore::ControlStyle::LP_Jitter);
    case OscillatorWaveform::Noise_SaH_HP4:
      return new SAHNoiseCore(OscillatorWaveform::Noise_SaH_HP4, SAHNoiseCore::ControlStyle::HP_Jitter);

    case OscillatorWaveform::Noise_White_ProbDuty:
      return new WhiteNoiseCore2(OscillatorWaveform::Noise_White_ProbDuty, WhiteNoiseCore2::ControlStyle::Prob_Duty);
    case OscillatorWaveform::Noise_White_ProbLP:
      return new WhiteNoiseCore2(OscillatorWaveform::Noise_White_ProbLP, WhiteNoiseCore2::ControlStyle::Prob_LP);
    case OscillatorWaveform::Noise_White_ProbBP:
      return new WhiteNoiseCore2(OscillatorWaveform::Noise_White_ProbBP, WhiteNoiseCore2::ControlStyle::Prob_BP);

      case OscillatorWaveform::Noise_White_DutyLP:
      return new WhiteNoiseCore2(OscillatorWaveform::Noise_White_DutyLP, WhiteNoiseCore2::ControlStyle::Duty_LP);
    case OscillatorWaveform::Noise_White_DutyBP:
      return new WhiteNoiseCore2(OscillatorWaveform::Noise_White_DutyBP, WhiteNoiseCore2::ControlStyle::Duty_BP);
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct KRateRecalculator
{
  // when 0, run.
  size_t mNSamplesElapsed = 0;
  void Invalidate()
  {
    mNSamplesElapsed = 0;
  }
  template <typename T>
  void visit(T run)
  {
    if (mNSamplesElapsed == 0)
    {
      run();
    }
    mNSamplesElapsed = (mNSamplesElapsed + 1) & GetModulationRecalcSampleMask();
  }
};

struct OscillatorNode : ISoundSourceDevice::Voice
{
private:
  OscillatorDevice* mpOscDevice = nullptr;
  OscillatorIntention mIntention = OscillatorIntention::Audio;

  std::unique_ptr<OscillatorCore> mCore;

  KRateRecalculator mKRateRecalc;
  float mPreviousSample = 0;
  float mCurrentFrequencyHz = 0;
  float mFMFeedbackAmt = 0.0f;

  float mCorrectionFactor = 1;

public:
  //float mInv = 1;

  OscillatorNode(OscillatorDevice* pOscDevice,
                 OscillatorIntention intention,
                 ModMatrixNode* pModMatrix,
                 EnvelopeNode* pAmpEnv)
      : ISoundSourceDevice::Voice(pOscDevice, pModMatrix, pAmpEnv)
      , mpOscDevice(pOscDevice)
      , mIntention(intention)
  {
    mCore = std::make_unique<SineCoreExt>(OscillatorWaveform::SineDCClip, SineCoreExtVariant::DCClip);
  }

  // used by LFOs to just hard-set the phase. usually NOP
  void SynchronizePhase(const OscillatorNode& src)
  {
    mCore->ForcefullySynchronizePhase(*src.mCore);
  }

  // used only by vst editor.
  float GetPhase01() const
  {
    return (float)mCore->mPhaseAcc.slave.getPhase01();
  }
  // used by debugging
  void SetCorrectionFactor(float factor)
  {
    mCorrectionFactor = factor;
    mCore->SetCorrectionFactor(factor);
  }

  // Returns the theoretical phase offset (not a live cursor).
  // considers phase offset + modulation.
  // not 100% certain if this is the best way to express this.
  float GetPhaseOffset() const
  {
    float phaseModVal = 0.0f;
    size_t phaseOffsetParamOffset = 0;
    switch (mpOscDevice->mIntention)
    {
      case OscillatorIntention::Audio:
        phaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                       (int)OscModParamIndexOffsets::Phase);
        phaseOffsetParamOffset = (size_t)OscParamIndexOffsets::PhaseOffset;
        break;
      case OscillatorIntention::LFO:
        phaseOffsetParamOffset = (size_t)LFOParamIndexOffsets::PhaseOffset;
      default:
        return 0.0f;
    }
    float f = mpOscDevice->mParams.GetN11Value(phaseOffsetParamOffset, phaseModVal);
    return math::fract(f);
  }

  // Required for
  // - outputting to alternate streams
  // - making the FM source values available to other oscillators
  float GetLastSample() const
  {
    return mPreviousSample;
  }

  // for debug only, alternate streams, expose how many samples since last recalculation of k-rate params.
  size_t GetSamplesSinceRecalc() const
  {
    return mKRateRecalc.mNSamplesElapsed;
  }

  virtual void NoteOn(bool legato) override
  {
    if (legato)
      return;
    if (mpOscDevice->GetPhaseRestart())
    {
      mCore->RestartDueToNoteOn();
    }
  }
  virtual void NoteOff() override {}

  virtual void BeginBlock() override
  {
    if (!mpSrcDevice->IsEnabled())
    {
      return;
    }
    mKRateRecalc.Invalidate();
    auto w = mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(mpOscDevice->mIntention == OscillatorIntention::Audio
                                                                       ? (int)OscParamIndexOffsets::Waveform
                                                                       : (int)LFOParamIndexOffsets::Waveform);
    //mpSlaveWave = mpWaveforms[math::ClampI((int)w, 0, (int)OscillatorWaveform::Count - 1)];
  }

  void SetWaveformShape(OscillatorWaveform w)
  {
    if (mCore->mWaveformType == w)
    {
      return;
    }

    auto* p = InstantiateWaveformCore(w, mIntention);

    mCore.reset(p);
    mCore->SetCorrectionFactor(mCorrectionFactor);
  }

  float RenderSampleForAudioAndAdvancePhase(real_t midiNote,
                                            float detuneFreqMul,
                                            float fmScale,
                                            const ParamAccessor& globalParams,
                                            float const* otherSignals,
                                            int iosc,
                                            float ampEnvLin)
  {
    const ParamAccessor& params = mpSrcDevice->mParams;
    auto modDestBaseId = mpSrcDevice->mModDestBaseID;
    if (!mpSrcDevice->mEnabledCache)
    {
      mPreviousSample = 0.0f;
      return 0;
    }

    mKRateRecalc.visit(
        [&]()
        {
          // Cache some k-rate modulated params
          // establish:
          // - frequency
          // - sync frequency
          // - waveshapes
          bool syncEnable = mpOscDevice->mParams.GetBoolValue(OscParamIndexOffsets::SyncEnable);

          float syncFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                  (int)OscModParamIndexOffsets::SyncFrequency);
          float freqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                              (int)OscModParamIndexOffsets::FrequencyParam);
          float waveShapeAModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::WaveshapeA);
          float waveShapeBModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::WaveshapeB);
          float pitchFineModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                   (int)OscModParamIndexOffsets::PitchFine);
          float FMFeedbackModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID +
                                                                    (int)OscModParamIndexOffsets::FMFeedback);
          mFMFeedbackAmt = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::FMFeedback, FMFeedbackModVal) *
                           fmScale * 0.5f;

          float waveshapeA = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeA, waveShapeAModVal);
          float waveshapeB = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::WaveshapeB, waveShapeBModVal);

          // - osc pitch semis                  note         oscillator
          // - osc fine (semis)                 note         oscillator
          // - osc sync freq / kt (Hz)          hz           oscillator
          // - osc freq / kt (Hz)               hz           oscillator
          // - osc mul (Hz)                     hz           oscillator
          // - osc detune (semis)               hz+semis     oscillator
          // - unisono detune (semis)           hz+semis     oscillator
          int pitchSemis = params.GetIntValue(OscParamIndexOffsets::PitchSemis, gSourcePitchSemisRange);
          float pitchFine = params.GetN11Value(OscParamIndexOffsets::PitchFine, pitchFineModVal) *
                            gSourcePitchFineRangeSemis;
          midiNote +=
              pitchSemis +
              pitchFine;  // (mpSrcDevice->mPitchFineParam.mCachedVal + mPitchFineModVal)* gSourcePitchFineRangeSemis;
          float noteHz = math::MIDINoteToFreq(midiNote);
          float freq =
              params.GetFrequency(OscParamIndexOffsets::FrequencyParam,
                                  OscParamIndexOffsets::FrequencyParamKT,
                                  gSourceFreqConfig,
                                  noteHz,
                                  freqModVal);  //mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
          freq *=
              mpOscDevice->mParams.GetScaledRealValue(OscParamIndexOffsets::FreqMul,
                                                      0.0f,
                                                      gFrequencyMulMax,
                                                      0);  //mpOscDevice->mFrequencyMul.mCachedVal;// .GetRangedValue();
          freq *= detuneFreqMul;
          // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
          freq = std::max(freq, 0.0001f);
          mCurrentFrequencyHz = freq;

          float syncFreq = 1;
          if (syncEnable)
          {
            syncFreq = params.GetFrequency(OscParamIndexOffsets::SyncFrequency,
                                           OscParamIndexOffsets::SyncFrequencyKT,
                                           gSyncFreqConfig,
                                           noteHz,
                                           syncFreqModVal);
            syncFreq = std::max(syncFreq, 0.0001f);
          }

          SetWaveformShape(params.GetEnumValue<OscillatorWaveform>(OscParamIndexOffsets::Waveform));

          mCore->SetKRateParams(waveshapeA, waveshapeB, freq, syncEnable, syncFreq);
        });

    // Calculate immediate phase modulation from feedback and other oscillators.
    // immediatePhaseMod = mPrevSample * FMFeedbackAmt + sum(otherOscs * theirAmt)
    auto calcImmediatePhaseMod = [&]()
    {
      float previousSample = mPreviousSample;
      float immediatePhaseMod = mPreviousSample * mFMFeedbackAmt;
      int otherIndex = -1;
      for (int i = 0; i < gOscillatorCount - 1; ++i)
      {
        int off = iosc * (gOscillatorCount - 1) + i;
        ParamIndices amtParam = ParamIndices((int)ParamIndices::FMAmt2to1 + off);
        ModDestination amtMod = ModDestination((int)ModDestination::FMAmt2to1 + off);
        float amt = globalParams.Get01Value(amtParam, mpModMatrix->GetDestinationValue(amtMod));

        otherIndex += (i == iosc) ? 2 : 1;
        immediatePhaseMod += otherSignals[otherIndex] * amt * fmScale;
      }
      return immediatePhaseMod;
    };

    auto immediatePhaseMod = calcImmediatePhaseMod();
    auto outp = mCore->renderSampleAndAdvance(immediatePhaseMod + GetPhaseOffset());
    mLastSample = outp;
    mPreviousSample = outp.amplitude;
    return outp.amplitude * ampEnvLin;
  }

  CoreSample mLastSample;

  // render one sample for LFO, advance phase.
  // forceSilence skips rendering the wave but still advances phase
  float RenderSampleForLFOAndAdvancePhase(bool forceSilence)
  {
#ifdef ENABLE_OSC_LOG
    auto ens = gOscLog.EnabledBlock(false);
#endif  // ENABLE_OSC_LOG
    auto modDestBaseId = mpSrcDevice->mModDestBaseID;

    mKRateRecalc.visit(
        [&]()
        {
          float freqModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::FrequencyParam);
          float waveShapeAModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::WaveshapeA);
          float waveShapeBModVal = mpModMatrix->GetDestinationValue(modDestBaseId, LFOModParamIndexOffsets::WaveshapeB);

          float freq = mpSrcDevice->mParams.GetFrequency(LFOParamIndexOffsets::FrequencyParam,
                                                         -1,  // no keytracking for lfo
                                                         gLFOFreqConfig,
                                                         0,  // note hz (no keytracking)
                                                         freqModVal);
          // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
          freq = std::max(freq, 0.001f);

          SetWaveformShape(mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(LFOParamIndexOffsets::Waveform));

          float waveshapeA = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeA, waveShapeAModVal);
          float waveshapeB = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::WaveshapeB, waveShapeBModVal);
          mCore->SetKRateParams(waveshapeA, waveshapeB, freq, false, 1);
        });

    // uncomment for prod
    const auto outp = mCore->renderSampleAndAdvance(GetPhaseOffset());
    mLastSample = outp;

    if (forceSilence)
    {
      mPreviousSample = 0.0f;
      return 0.0f;
    }

    mPreviousSample = outp.amplitude;
    return outp.amplitude;
  }

};  // OscillatorNode


}  // namespace M7


}  // namespace WaveSabreCore
