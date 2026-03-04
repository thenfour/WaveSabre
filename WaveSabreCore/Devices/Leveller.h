// note that these are NOT multiband + gains + join. these are 5 cascaded biquad filters; each perform independent peaking.

#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include "../Analysis/AnalysisStream.hpp"
#include "../Analysis/FFTAnalysis.hpp"
#include "../Basic/DSPMath.hpp"
#include "../Filters/DCFilter.hpp"
#include "../Filters/Maj7Filter.hpp"
#include "../Params/Maj7ParamAccessor.hpp"
#include "../WSCore/Device.h"


namespace WaveSabreCore::M7
{
struct Leveller : public Device
{
  static constexpr size_t gBandCount = 5;

  enum class ParamIndices  // : uint8_t
  {
    OutputVolume,
    EnableDCFilter,

    Band1Circuit,
    Band1Slope,
    Band1Response,
    Band1Freq,
    Band1Gain,
    Band1Q,
    Band1Enable,

    Band2Circuit,
    Band2Slope,
    Band2Response,
    Band2Freq,
    Band2Gain,
    Band2Q,
    Band2Enable,

    Band3Circuit,
    Band3Slope,
    Band3Response,
    Band3Freq,
    Band3Gain,
    Band3Q,
    Band3Enable,

    Band4Circuit,
    Band4Slope,
    Band4Response,
    Band4Freq,
    Band4Gain,
    Band4Q,
    Band4Enable,

    Band5Circuit,
    Band5Slope,
    Band5Response,
    Band5Freq,
    Band5Gain,
    Band5Q,
    Band5Enable,

    NumParams,
  };
  // clang-format off
#define LEVELLER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Leveller::ParamIndices::NumParams]{ \
	{"OutpVol"},\
	{"DCEn"},\
		{"ACircuit"}, \
		{"ASlope"}, \
		{"AResp"}, \
		{"AFreq"}, \
		{"AGain"}, \
		{"AQ"}, \
		{"AEn"}, \
		{"BCircuit"}, \
		{"BSlope"}, \
		{"BResp"}, \
		{"BFreq"}, \
		{"BGain"}, \
		{"BQ"}, \
		{"BEn"}, \
		{"CCircuit"}, \
		{"CSlope"}, \
		{"CResp"}, \
		{"CFreq"}, \
		{"CGain"}, \
		{"CQ"}, \
		{"CEn"}, \
		{"DCircuit"}, \
		{"DSlope"}, \
		{"DResp"}, \
		{"DFreq"}, \
		{"DGain"}, \
		{"DQ"}, \
		{"DEn"}, \
		{"ECircuit"}, \
		{"ESlope"}, \
		{"EResp"}, \
		{"EFreq"}, \
		{"EGain"}, \
		{"EQ"}, \
		{"EEn"}, \
}
  // clang-format on

static_assert((int)ParamIndices::NumParams == 37, "param count probably changed and this needs to be regenerated.");
static constexpr int16_t gLevellerDefaults16[(int)ParamIndices::NumParams] = {
  16422, // OutpVol = 0.50115966796875
  0, // DCEn = 0
  16386, // ACircuit = 0.5000762939453125
  16387, // ASlope = 0.5001068115234375
  16385, // AResp = 0.5000457763671875
  5000, // AFreq = 0.152587890625
  16422, // AGain = 0.50115966796875
  10977, // AQ = 0.334991455078125
  32767, // AEn = 1
  16386, // BCircuit = 0.5000762939453125
  16387, // BSlope = 0.5001068115234375
  16389, // BResp = 0.5001678466796875
  9830, // BFreq = 0.29998779296875
  16384, // BGain = 0.5
  16384, // BQ = 0.5
  0, // BEn = 0
  16386, // CCircuit = 0.5000762939453125
  16387, // CSlope = 0.5001068115234375
  16389, // CResp = 0.5001678466796875
  16834, // CFreq = 0.51373291015625
  16384, // CGain = 0.5
  16384, // CQ = 0.5
  32767, // CEn = 1
  16386, // DCircuit = 0.5000762939453125
  16387, // DSlope = 0.5001068115234375
  16389, // DResp = 0.5001678466796875
  21577, // DFreq = 0.658477783203125
  16384, // DGain = 0.5
  16384, // DQ = 0.5
  0, // DEn = 0
  16386, // ECircuit = 0.5000762939453125
  16387, // ESlope = 0.5001068115234375
  16384, // EResp = 0.5000152587890625
  26500, // EFreq = 0.8087158203125
  16422, // EGain = 0.50115966796875
  10977, // EQ = 0.33500000834465026855
  0, // EEn = 0
};

  enum class BandParamOffsets : uint8_t
  {
    Circuit,
    Slope,
    Response,
    Freq,
    Gain,
    Q,
    Enable,
    Count__,
  };

  struct Band
  {
    Band(float* paramCache, ParamIndices baseParamID)
        : mParams(paramCache, baseParamID)
    {
    }

    void RecalcFilters()
    {
      const auto circuit = mParams.GetEnumValue<FilterCircuit>(BandParamOffsets::Circuit);
      const auto slope = mParams.GetEnumValue<FilterSlope>(BandParamOffsets::Slope);
      const auto response = mParams.GetEnumValue<FilterResponse>(BandParamOffsets::Response);
      const auto cutoffHz = mParams.GetFrequency(BandParamOffsets::Freq, gFilterFreqConfig);
      const auto reso01 = Param01{mParams.Get01Value(BandParamOffsets::Q)};
      const auto gain = mParams.GetScaledRealValue(BandParamOffsets::Gain, gEqBandGainMin, gEqBandGainMax, 0);

      for (size_t i = 0; i < 2; ++i)
      {
        mFilters[i].SetParams(circuit, slope, response, cutoffHz, reso01, gain);
      }
    }

    ParamAccessor mParams;

    //BiquadFilter mFilters[2];
    FilterNode mFilters[2];
  };

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
  SmoothedStereoFFT mInputSpectrumSmoother;   // Input display smoother + analyzer
  SmoothedStereoFFT mOutputSpectrumSmoother;  // Output display smoother + analyzer
#endif                                        // SELECTABLE_OUTPUT_STREAM_SUPPORT


  Leveller()
      : Device((int)ParamIndices::NumParams, mParamCache, gLevellerDefaults16)
  {
    LoadDefaults();
  }

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    float masterGain = mParams.GetLinearVolume(ParamIndices::OutputVolume, gVolumeCfg12db);
    bool enableDC = mParams.GetBoolValue(ParamIndices::EnableDCFilter);

    for (int iSample = 0; iSample < numSamples; iSample++)
    {
      float s1 = inputs[0][iSample];
      float s2 = inputs[1][iSample];

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mInputAnalysis[0].WriteSample(inputs[0][iSample]);
        mInputAnalysis[1].WriteSample(inputs[1][iSample]);
        mInputSpectrumSmoother.ProcessSamples(s1, s2);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

      if (enableDC)
      {
        s1 = mDCFilters[0].ProcessSample(s1);
        s2 = mDCFilters[1].ProcessSample(s2);
      }

      for (int iBand = 0; iBand < gBandCount; ++iBand)
      {
        auto& b = mBands[iBand];
        if (b.mParams.GetBoolValue(BandParamOffsets::Enable))
        {
          s1 = b.mFilters[0].ProcessSample(s1);
          s2 = b.mFilters[1].ProcessSample(s2);
        }
      }

      outputs[0][iSample] = masterGain * s1;
      outputs[1][iSample] = masterGain * s2;


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mOutputSpectrumSmoother.ProcessSamples(outputs[0][iSample], outputs[1][iSample]);
        mOutputAnalysis[0].WriteSample(outputs[0][iSample]);
        mOutputAnalysis[1].WriteSample(outputs[1][iSample]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    }
  }

  virtual void OnParamsChanged() override
  {
    for (int iBand = 0; iBand < gBandCount; ++iBand)
    {
      auto& b = mBands[iBand];
      b.RecalcFilters();
    }
  }

  float mParamCache[(size_t)ParamIndices::NumParams];
  ParamAccessor mParams{mParamCache, 0};

  Band mBands[gBandCount] = {
      {mParamCache, ParamIndices::Band1Circuit /*, 0*/},
      {mParamCache, ParamIndices::Band2Circuit /*, 650 */},
      {mParamCache, ParamIndices::Band3Circuit /*, 2000 */},
      {mParamCache, ParamIndices::Band4Circuit /*, 7000 */},
      {mParamCache, ParamIndices::Band5Circuit /*, 22050 */},
  };

  DCFilter mDCFilters[2];
};
}  // namespace WaveSabreCore::M7

namespace WaveSabreCore
{
using Leveller = M7::Leveller;
}

#endif
