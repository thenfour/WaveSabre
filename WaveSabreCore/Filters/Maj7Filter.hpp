#pragma once
//
#include "../Filters/FilterButterworth.hpp"
#include "../Filters/FilterDiode.hpp"
#include "../Filters/FilterK35.hpp"
#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "../Filters/BiquadFilter.h"

#include "../GigaSynth/GigaParams.hpp"
#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "../GigaSynth/Maj7Basic.hpp"

namespace WaveSabreCore
{
namespace M7
{

struct FilterNode  : IFilter
{
  BiquadFilter mFilter;

  virtual void SetParams(FilterResponse response,
                 float cutoffHz,
                 Param01 reso01,
                 float gainDb) override;

  virtual void Reset() override
  {
    mFilter.Reset();
  }
  virtual float ProcessSample(float inputSample) override
  {
    return mFilter.ProcessSample(inputSample);
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual bool DoesSupport(FilterResponse response)  override {
      return true;
  }
  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    return mFilter.GetMagnitudeAtFrequency(freqHz);
  }
  // needed in order for VSTs to be able to clone filters for the response graph without interrupting realtime audio processing.
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    return std::make_unique<FilterNode>(*this);
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

};  // FilterNode


struct FilterAuxNode  // : IAuxEffect
{
  FilterNode mFilter;

  ParamAccessor mParams;

  FilterResponse mFilterResponse = (FilterResponse)0;

  //ModDestination mModDestBase;
  float mNoteHz = 0;
  size_t mnSampleCount = 0;
  ModMatrixNode* mModMatrix = nullptr;
  bool mEnabledCached = false;

    explicit FilterAuxNode(float* paramCache, GigaSynthParamIndices baseParamID)
      : mParams(paramCache, baseParamID)
  {
  }

  void AuxBeginBlock(float noteHz, ModMatrixNode& modMatrix)
  {
    mModMatrix = &modMatrix;
    mnSampleCount = 0;  // ensure reprocessing after setting these params to avoid corrupt state.
    mNoteHz = noteHz;
    //mFilterCircuit = mParams.GetEnumValue<FilterCircuit>(FilterParamIndexOffsets::FilterCircuit);
    //mFilterSlope = mParams.GetEnumValue<FilterSlope>(FilterParamIndexOffsets::FilterSlope);
    mFilterResponse = mParams.GetEnumValue<FilterResponse>(FilterParamIndexOffsets::FilterResponse);
    mEnabledCached = mParams.GetBoolValue(FilterParamIndexOffsets::Enabled);
  }

  float AuxProcessSample(float inputSample)
  {
    if (!mEnabledCached)
      return inputSample;
    auto recalcMask = GetModulationRecalcSampleMask();
    bool calc = (mnSampleCount == 0);
    mnSampleCount = (mnSampleCount + 1) & recalcMask;
    if (calc)
    {
      auto reso01 = Param01{
          mParams.Get01Value(FilterParamIndexOffsets::Q,
                   mModMatrix->GetDestinationValue(ModDestination::Filter1Q))};

        auto freq = mParams.GetFrequency(FilterParamIndexOffsets::Freq,
                         FilterParamIndexOffsets::FreqKT,
                         gFilterFreqConfig,
                         mNoteHz,
                         mModMatrix->GetDestinationValue(ModDestination::Filter1Freq));

      //const auto q = Decibels {gBiquadFilterQCfg.Param01ToValue(reso01.value)};

      // mFilter
      //     .SetBiquadParams(mFilterResponse, freq, q, 0);

      mFilter.SetParams(
                       mFilterResponse,
                       freq,
                       reso01,
                       0 /* no gain here; it's only for filter types we don't support */);
    }

    return mFilter.ProcessSample(inputSample);
  }
};

}  // namespace M7


}  // namespace WaveSabreCore
