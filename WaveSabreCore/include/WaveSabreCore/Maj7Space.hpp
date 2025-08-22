#pragma once

#include "BiquadFilter.h"
#include "DelayBuffer.h"
#include "DelayCore.hpp"
#include "Device.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"
#include "RMS.hpp"
#include "ReverbCore.hpp"

namespace WaveSabreCore
{
struct Maj7Space : public Device
{
  static constexpr M7::IntParamConfig gDelayCoarseCfg{0, 48};
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
    {"DryOut"},                                                                                                        \
    {"DlyOut"},                                                                                                        \
    {"RevOut"},                                                                                                        \
  }
  // clang-format on
  static_assert((int)Maj7Space::ParamIndices::NumParams == 16,
                "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gDefaults16[(int)Maj7Space::ParamIndices::NumParams] = {
      5684,   // LdlyC = 0.17346939444541931152
      16384,  // LdlyF = 0.5
      16384,  // LdlyMS = 0.5
      4346,   // RdlyC = 0.13265305757522583008
      16384,  // RdlyF = 0.5
      16384,  // RdlyMS = 0.5
      2221,   // LCFreq = 0.06780719757080078125
      11459,  // LCQ = 0.34971103072166442871
      26500,  // HCFreq = 0.80874627828598022461
      11459,  // HCQ = 0.34971103072166442871
      9782,   // FbLvl = 0.29853826761245727539
      4902,   // FbDrive = 0.14962357282638549805
      8192,   // Cross = 0.25
      0,
      0,
      0,
  };

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams{mParamCache, 0};

  DelayCore mDelayCore;
  float mDryLin;
  float mDelayLin;
  float mReverbLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7Space()
      : Device((int)ParamIndices::NumParams, mParamCache, gDefaults16)
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
      auto delayWet = mDelayCore.Run(dry);

      auto verbWet = dry; // todo.

      delayWet = delayWet.mul(mDelayLin);
      verbWet = verbWet.mul(mReverbLin);

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

    float leftBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::DelayLeftDelayCoarse);
    float rightBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::DelayRightDelayCoarse);

    mDelayCore.OnParamsChanged(leftBufferLengthMs,
                               rightBufferLengthMs,
                               mParams.GetFrequency(ParamIndices::DelayLowCutFreq, M7::gFilterFreqConfig),
                               mParams.GetDivCurvedValue(ParamIndices::DelayLowCutQ, M7::gBiquadFilterQCfg),
                               mParams.GetFrequency(ParamIndices::DelayHighCutFreq, M7::gFilterFreqConfig),
                               mParams.GetDivCurvedValue(ParamIndices::DelayHighCutQ, M7::gBiquadFilterQCfg));
  }
};
}  // namespace WaveSabreCore
