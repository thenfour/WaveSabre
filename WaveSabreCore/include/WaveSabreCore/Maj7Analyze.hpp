#pragma once

#include "BiquadFilter.h"
#include "DelayBuffer.h"
#include "Device.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"
#include "RMS.hpp"
#include "StereoImageAnalysis.hpp"

namespace WaveSabreCore
{
struct Maj7Analyze : public Device
{
  enum class ParamIndices
  {
    OutputVolume,
    NumParams,
  };

#define MAJ7ANALYZE_PARAM_VST_NAMES(symbolName)                                                                        \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Analyze::ParamIndices::NumParams]            \
  {                                                                                                                    \
    {"Outp"},                                                                                                          \
  }

  static_assert((int)Maj7Analyze::ParamIndices::NumParams == 1,
                "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gDefaults16[(int)Maj7Analyze::ParamIndices::NumParams] = {
      0,
  };

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams{mParamCache, 0};

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static constexpr double kPeakFalloffMS = 200;
  AnalysisStream mLoudnessAnalysis[2] = {AnalysisStream{kPeakFalloffMS}, AnalysisStream{kPeakFalloffMS}};
  StereoImagingAnalysisStream mInputImagingAnalysis;
  SmoothedStereoFFT mFFT;  // Input display smoother + analyzer
#endif                     // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7Analyze()
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
        mLoudnessAnalysis[0].WriteSample(inputs[0][i]);
        mLoudnessAnalysis[1].WriteSample(inputs[1][i]);
        mInputImagingAnalysis.WriteStereoSample(inputs[0][i], inputs[1][i]);
        mFFT.ProcessSamples(inputs[0][i], inputs[1][i]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

      outputs[0][i] = inputs[0][i];
      outputs[1][i] = inputs[1][i];
    }
  }

  virtual void OnParamsChanged() override {}
};
}  // namespace WaveSabreCore
