#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "DelayCore.hpp"
#include "Device.h"
#include "Maj7Basic.hpp"
#include "RMS.hpp"
#include <WaveSabreCore/AnalysisStream.hpp>

namespace WaveSabreCore
{
struct Echo : public Device
{
  //static constexpr M7::IntParamConfig gDelayFineCfg{ 0, 200 };

  enum class ParamIndices
  {
    LeftDelayCoarse,  // in 8ths range of 4 beats, 0-4*8
    LeftDelayFine,    // -1 to 1 8th
    LeftDelayMS,      // -200,200 ms

    RightDelayCoarse,  // 0-1 => 0-16 inclusive integral
    RightDelayFine,
    RightDelayMS,

    LowCutFreq,  // Helpers::ParamToFrequency(value)
    LowCutQ,
    HighCutFreq,  // Helpers::ParamToFrequency(value)
    HighCutQ,

    FeedbackLevel,  // 0-1
    FeedbackDriveDB,
    Cross,  // 0-1

    DryOutput,  // 0-1
    WetOutput,  // 0-1

    NumParams,
  };

#define ECHO_PARAM_VST_NAMES(symbolName)                                                                               \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Echo::ParamIndices::NumParams]                   \
  {                                                                                                                    \
    {"LdlyC"}, {"LdlyF"}, {"LdlyMS"}, {"RdlyC"}, {"RdlyF"}, {"RdlyMS"}, {"LCFreq"}, {"LCQ"}, {"HCFreq"}, {"HCQ"},      \
        {"FbLvl"}, {"FbDrive"}, {"Cross"}, {"DryOut"}, {"WetOut"},                                                     \
  }

  static_assert((int)Echo::ParamIndices::NumParams == 15,
                "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gDefaults16[(int)Echo::ParamIndices::NumParams] = {
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
      16422,  // DryOut = 0.50118720531463623047
      8230,   // WetOut = 0.25118863582611083984
  };

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams{mParamCache, 0};

  DelayCore mCore;

    float mDryLin;
  float mWetLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Echo()
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

      M7::FloatPair dry {inputs[0][i], inputs[1][i]};
      auto outp = M7::FloatPair::Mix(dry, mCore.Run(dry), mDryLin, mWetLin);

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
    mDryLin = mParams.GetLinearVolume(ParamIndices::DryOutput, M7::gVolumeCfg12db);
    mWetLin = mParams.GetLinearVolume(ParamIndices::WetOutput, M7::gVolumeCfg12db);

    mCore.mCrossMix = mParams.Get01Value(ParamIndices::Cross);
    mCore.mFeedbackLin = mParams.GetLinearVolume(ParamIndices::FeedbackLevel, M7::gVolumeCfg6db, 0);
    mCore.mFeedbackDriveLin = mParams.GetLinearVolume(ParamIndices::FeedbackDriveDB, M7::gVolumeCfg36db, 0);

    float leftBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::LeftDelayCoarse);
    float rightBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::RightDelayCoarse);

    mCore.OnParamsChanged(leftBufferLengthMs,
                          rightBufferLengthMs,
                          mParams.GetFrequency(ParamIndices::LowCutFreq, M7::gFilterFreqConfig),
                          mParams.GetDivCurvedValue(ParamIndices::LowCutQ, M7::gBiquadFilterQCfg),
                          mParams.GetFrequency(ParamIndices::HighCutFreq, M7::gFilterFreqConfig),
                          mParams.GetDivCurvedValue(ParamIndices::HighCutQ, M7::gBiquadFilterQCfg));
  }
};
}  // namespace WaveSabreCore

#endif
