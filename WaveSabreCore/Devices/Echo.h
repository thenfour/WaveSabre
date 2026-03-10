#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "../Basic/DSPMath.hpp"
#include "../WSCore/Device.h"
#include "../Params/Maj7ParamAccessor.hpp"
#include "../Analysis/AnalysisStream.hpp"
#include "../DSP/DelayCore.hpp"

namespace WaveSabreCore::M7
{
struct Echo : public Device
{
  static constexpr M7::IntParamConfig gDelayFineCfg{ 0, 200 };

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

static_assert((int)Echo::ParamIndices::NumParams == 15, "param count probably changed and this needs to be regenerated.");
static constexpr int16_t gDefaults16[(int)Echo::ParamIndices::NumParams] = {
  31, // LdlyC = 0.0009765625
  0, // LdlyF = 0
  16383, // LdlyMS = 0.5
  23, // RdlyC = 0.000732421875
  0, // RdlyF = 0
  16383, // RdlyMS = 0.5
  2221, // LCFreq = 0.06780719757080078125
  8508, // LCQ = 0.25965651869773864746
  26500, // HCFreq = 0.80874627828598022461
  8508, // HCQ = 0.25965651869773864746
  9782, // FbLvl = 0.29853826761245727539
  4902, // FbDrive = 0.14962357282638549805
  8191, // Cross = 0.25
  16422, // DryOut = 0.50118720531463623047
  8230, // WetOut = 0.25118863582611083984
};

  float mParamCache[(int)ParamIndices::NumParams];
  ParamAccessor mParams{mParamCache, 0};

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

      FloatPair dry {inputs[0][i], inputs[1][i]};
      auto outp = FloatPair::Mix(dry, mCore.Run(dry), mDryLin, mWetLin);

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
    mDryLin = mParams.GetLinearVolume(ParamIndices::DryOutput, gVolumeCfg12db);
    mWetLin = mParams.GetLinearVolume(ParamIndices::WetOutput, gVolumeCfg12db);

    mCore.mCrossMix = mParams.Get01Value(ParamIndices::Cross);
    mCore.mFeedbackLin = mParams.GetLinearVolume(ParamIndices::FeedbackLevel, gVolumeCfg6db, 0);
    mCore.mFeedbackDriveLin = mParams.GetLinearVolume(ParamIndices::FeedbackDriveDB, gVolumeCfg36db, 0);

    float leftBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::LeftDelayCoarse);
    float rightBufferLengthMs = DelayCore::CalcDelayMS(mParams, (int)ParamIndices::RightDelayCoarse);

    mCore.OnParamsChanged(leftBufferLengthMs,
                          rightBufferLengthMs,
                          mParams.GetFrequency(ParamIndices::LowCutFreq, gFilterFreqConfig),
                          Decibels{mParams.GetDivCurvedValue(ParamIndices::LowCutQ, gBiquadFilterQCfg)},
                          mParams.GetFrequency(ParamIndices::HighCutFreq, gFilterFreqConfig),
                          Decibels{mParams.GetDivCurvedValue(ParamIndices::HighCutQ, gBiquadFilterQCfg)});
  }
};
}  // namespace WaveSabreCore::M7

namespace WaveSabreCore
{
using Echo = M7::Echo;
}
#endif
