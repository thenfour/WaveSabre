#pragma once

#include "../Analysis/RMS.hpp"
#include "../DSP/DelayBuffer.h"
#include "../DSP/DelayCore.hpp"
#include "../DSP/ReverbCore.hpp"
#include "../Filters/BiquadFilter.h"
#include "../Filters/Maj7Filter.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../WSCore/Device.h"


namespace WaveSabreCore
{
struct Maj7Space : public Device
{
  enum class ParamIndices
  {
    DelayLeftDelayCoarse,  // in 8ths range of 4 beats, 0-4*8
    DelayLeftDelayFine,    // -1 to 1 8th -- NOTE: MUST BE DIRECTLY AFTER COARSE
    DelayLeftDelayMS,      // -200,200 ms -- NOTE: MUST BE DIRECTLY AFTER FINE

    DelayRightDelayCoarse,  // 0-1 => 0-16 inclusive integral
    DelayRightDelayFine,    // -- NOTE: MUST BE DIRECTLY AFTER COARSE
    DelayRightDelayMS,      // -- NOTE: MUST BE DIRECTLY AFTER FINE

    DelayLowCutFreq,  // Helpers::ParamToFrequency(value)
    DelayLowCutQ,
    DelayHighCutFreq,  // Helpers::ParamToFrequency(value)
    DelayHighCutQ,

    DelayFeedbackLevel,  // 0-1
    DelayFeedbackDriveDB,
    DelayCross,  // 0-1

    //
    ReverbRoomSize,
    ReverbDamp,
    ReverbWidth,
    ReverbLowCutFreq,
    ReverbHighCutFreq,
    ReverbPreDelay,

    DelayEnabled,
    ReverbEnabled,

    DryVolume,
    DelayVolume,
    ReverbVolume,

    NumParams,
  };

  // clang-format off
#define MAJ7SPACE_PARAM_VST_NAMES(symbolName)                                                                          \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Space::ParamIndices::NumParams]              \
  {                                                                                                                    \
    {"LdlyC"},                                                                                                         \
    {"LdlyF"},                                                                                                         \
    {"LdlyMS"},                                                                                                        \
    {"RdlyC"},                                                                                                         \
    {"RdlyF"},                                                                                                         \
    {"RdlyMS"},                                                                                                        \
    {"LCFreq"},                                                                                                        \
    {"LCQ"},                                                                                                           \
    {"HCFreq"},                                                                                                        \
    {"HCQ"},                                                                                                           \
    {"FbLvl"},                                                                                                         \
    {"FbDrive"},                                                                                                       \
    {"Cross"},                                                                                                         \
    {"VRoomSz"},                                                                                                         \
    {"VDamp"},                                                                                                         \
    {"VWidth"},                                                                                                         \
    {"VLCFreq"},                                                                                                         \
    {"VHCFreq"},                                                                                                         \
    {"VPreDly"},                                                                                                         \
    {"DlyEn"},                                                                                                        \
    {"VerbEn"},                                                                                                        \
    {"DryOut"},                                                                                                        \
    {"DlyOut"},                                                                                                        \
    {"RevOut"},                                                                                                        \
  }
  // clang-format on


static_assert((int)ParamIndices::NumParams == 24, "param count probably changed and this needs to be regenerated.");
static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
  16398, // LdlyC = 0.50042724609375
  16384, // LdlyF = 0.5
  16384, // LdlyMS = 0.5
  16402, // RdlyC = 0.50054931640625
  16384, // RdlyF = 0.5
  16384, // RdlyMS = 0.5
  2221, // LCFreq = 0.06780719757080078125
  8508, // LCQ = 0.25965651869773864746
  26500, // HCFreq = 0.80874627828598022461
  8508, // HCQ = 0.25965651869773864746
  9782, // FbLvl = 0.29853826761245727539
  4902, // FbDrive = 0.14962357282638549805
  8192, // Cross = 0.25
  16384, // VRoomSz = 0.5
  4915, // VDamp = 0.15000000596046447754
  29491, // VWidth = 0.89999997615814208984
  7255, // VLCFreq = 0.22141247987747192383
  24443, // VHCFreq = 0.74594312906265258789
  65, // VPreDly = 0.0020000000949949026108
  32767, // DlyEn = 1
  32767, // VerbEn = 1
  16422, // DryOut = 0.50118720531463623047
  8230, // DlyOut = 0.25118863582611083984
  8230, // RevOut = 0.25118863582611083984
};

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams{mParamCache, 0};

  M7::DelayCore mDelayCore;
  ReverbCore mReverbCore;
  float mDryLin;
  float mDelayLin;
  float mReverbLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mDelayAnalysis[2];
  AnalysisStream mReverbAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7Space()
      : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
  {
    LoadDefaults();
  }

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    for (int i = 0; i < numSamples; i++)
    {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mInputAnalysis[0].WriteSample(inputs[0][i]);
        mInputAnalysis[1].WriteSample(inputs[1][i]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

      M7::FloatPair dry{inputs[0][i], inputs[1][i]};

      M7::FloatPair delayWet{};
      if (mParams.GetBoolValue(ParamIndices::DelayEnabled))
      {
        delayWet = mDelayCore.Run(dry);
        delayWet = delayWet.mul(mDelayLin);
      }

      M7::FloatPair verbWet{};
      if (mParams.GetBoolValue(ParamIndices::ReverbEnabled))
      {
        verbWet = mReverbCore.ProcessSample(dry + delayWet);
        verbWet = verbWet.mul(mReverbLin);
      }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mDelayAnalysis[0].WriteSample(delayWet.Left());
        mDelayAnalysis[1].WriteSample(delayWet.Right());
        mReverbAnalysis[0].WriteSample(verbWet.Left());
        mReverbAnalysis[1].WriteSample(verbWet.Right());
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


      auto outp = dry * mDryLin + delayWet + verbWet;

      outputs[0][i] = outp.Left();
      outputs[1][i] = outp.Right();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mOutputAnalysis[0].WriteSample(outputs[0][i]);
        mOutputAnalysis[1].WriteSample(outputs[1][i]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    }
  }


  virtual void OnParamsChanged() override
  {
    // own params
    mDryLin = mParams.GetLinearVolume(ParamIndices::DryVolume, M7::gVolumeCfg12db);
    mDelayLin = mParams.GetLinearVolume(ParamIndices::DelayVolume, M7::gVolumeCfg12db);
    mReverbLin = mParams.GetLinearVolume(ParamIndices::ReverbVolume, M7::gVolumeCfg12db);

    // Delay params.
    mDelayCore.mCrossMix = mParams.Get01Value(ParamIndices::DelayCross);
    mDelayCore.mFeedbackLin = mParams.GetLinearVolume(ParamIndices::DelayFeedbackLevel, M7::gVolumeCfg6db, 0);
    mDelayCore.mFeedbackDriveLin = mParams.GetLinearVolume(ParamIndices::DelayFeedbackDriveDB, M7::gVolumeCfg36db, 0);

    float leftBufferLengthMs = M7::DelayCore::CalcDelayMS(mParams, (int)ParamIndices::DelayLeftDelayCoarse);
    float rightBufferLengthMs = M7::DelayCore::CalcDelayMS(mParams, (int)ParamIndices::DelayRightDelayCoarse);

    mDelayCore.OnParamsChanged(
        leftBufferLengthMs,
        rightBufferLengthMs,
        mParams.GetFrequency(ParamIndices::DelayLowCutFreq, M7::gFilterFreqConfig),
        M7::Decibels{mParams.GetDivCurvedValue(ParamIndices::DelayLowCutQ, M7::gBiquadFilterQCfg)},
        mParams.GetFrequency(ParamIndices::DelayHighCutFreq, M7::gFilterFreqConfig),
        M7::Decibels{mParams.GetDivCurvedValue(ParamIndices::DelayHighCutQ, M7::gBiquadFilterQCfg)});

    // reverb params.
    mReverbCore.preDelayMS = mParams.GetScaledRealValue(ParamIndices::ReverbPreDelay,
                                                        0,
                                                        500);  //mParams.Get01Value(ParamIndices::PreDelay) * 500.0f;
    mReverbCore.damp = mParams.Get01Value(ParamIndices::ReverbDamp);
    mReverbCore.width = mParams.Get01Value(ParamIndices::ReverbWidth);
    mReverbCore.lowCutFreq = mParams.GetFrequency(ParamIndices::ReverbLowCutFreq, M7::gFilterFreqConfig);
    mReverbCore.highCutFreq = mParams.GetFrequency(ParamIndices::ReverbHighCutFreq, M7::gFilterFreqConfig);

    // roomsize is not a linear param. it's also not a div-curved param; it's inverted.
    // gotta flip -> map -> flip.
    // auto roomSize = mParams.Get01Value(ParamIndices::ReverbRoomSize);
    // roomSize = 1.0f - roomSize;
    // M7::ParamAccessor pa{&roomSize, 0};
    // float t = pa.GetDivCurvedValue(0, M7::gRoomSizeParamCfg, 0);
    // roomSize = 1.0f - t;
    //mReverbCore.roomSize = M7::math::clamp01(roomSize);

    mReverbCore.roomSize = mParams.GetInvDivCurvedValue(ParamIndices::ReverbRoomSize, M7::gRoomSizeParamCfg);

    mReverbCore.Update();
  }
};
}  // namespace WaveSabreCore
