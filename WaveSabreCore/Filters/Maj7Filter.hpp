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

struct FilterNode
{
  BiquadFilter mFilter;

  void SetParams(FilterResponse response,
                 float cutoffHz,
                 Param01 reso01,
                 float gainDb);

  void ResetState()
  {
    mFilter.Reset();
  }
  float ProcessSample(float inputSample)
  {
    return mFilter.ProcessSample(inputSample);
  }

};  // FilterNode


struct FilterAuxNode  // : IAuxEffect
{
  FilterNode mFilter;

  ParamAccessor mParams;

  //FilterCircuit mFilterCircuit;
  //FilterSlope mFilterSlope;
  FilterResponse mFilterResponse;

  ModDestination mModDestBase;
  float mNoteHz = 0;
  size_t mnSampleCount = 0;
  ModMatrixNode* mModMatrix = nullptr;
  bool mEnabledCached = false;

  FilterAuxNode(float* paramCache, GigaSynthParamIndices baseParamID, ModDestination modDestBase)
      : mParams(paramCache, baseParamID)
      , mModDestBase(modDestBase)
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
                             mModMatrix->GetDestinationValue((int)mModDestBase + (int)FilterAuxModDestOffsets::Q))};

      auto freq = mParams.GetFrequency(FilterParamIndexOffsets::Freq,
                                   FilterParamIndexOffsets::FreqKT,
                                   gFilterFreqConfig,
                                   mNoteHz,
                                   mModMatrix->GetDestinationValue((int)mModDestBase +
                                                                       (int)FilterAuxModDestOffsets::Freq));

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
