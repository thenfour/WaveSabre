#ifndef __WAVESABRECORE_CATHEDRAL_H__
#define __WAVESABRECORE_CATHEDRAL_H__

#include "../Basic/DSPMath.hpp"
#include "../WSCore/Device.h"
#include "../Params/Maj7ParamAccessor.hpp"
#include "../Analysis/AnalysisStream.hpp"
#include "../DSP/ReverbCore.hpp"

namespace WaveSabreCore
{
class Cathedral : public Device
{
public:
  enum class ParamIndices
  {
    RoomSize,
    Damp,
    Width,
    LowCutFreq,
    HighCutFreq,
    DryOut,
    WetOut,
    PreDelay,
    NumParams,
  };

  // NB: max 8 chars per string.
#define CATHEDRAL_PARAM_VST_NAMES(symbolName)                                                                          \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Cathedral::ParamIndices::NumParams]              \
  {                                                                                                                    \
    {"RoomSize"}, {"Damp"}, {"Width"}, {"LCF"}, {"HCF"}, {"DryOut"}, {"WetOut"}, {"PreDly"},                           \
  }

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams;

static_assert((int)Cathedral::ParamIndices::NumParams == 8, "param count probably changed and this needs to be regenerated.");
static constexpr int16_t gParamDefaults[(int)Cathedral::ParamIndices::NumParams] = {
  16383, // RoomSize = 0.5
  4915, // Damp = 0.15000000596046447754
  29490, // Width = 0.89999997615814208984
  7255, // LCF = 0.22141247987747192383
  24442, // HCF = 0.74594312906265258789
  16422, // DryOut = 0.50118720531463623047
  11626, // WetOut = 0.35481339693069458008
  0, // PreDly = 0
};

  Cathedral();

  virtual void Run(float** inputs, float** outputs, int numSamples) override;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void OnParamsChanged() override;

private:
  ReverbCore mCore;
};
}  // namespace WaveSabreCore

#endif
