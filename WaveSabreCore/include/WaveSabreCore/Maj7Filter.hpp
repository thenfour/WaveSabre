#pragma once

#include "Filters/FilterDiode.hpp"
#include "Filters/FilterK35.hpp"
#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
#include "Maj7ModMatrix.hpp"
#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <array>

namespace WaveSabreCore
{
namespace M7
{

struct FilterNode
{
  NullFilter mNullFilter;
  MoogOnePoleFilter mOnePole;
  CascadedBiquadFilter mBiquad;
  CascadedBiquadFilter mButterworth;  // placeholder.
  DiodeFilter mDiode;
  K35Filter mK35;
  MoogLadderFilter mMoog;

  IFilter* mSelectedFilter = &mNullFilter;

  void SetParams(FilterCircuit circuit, FilterSlope slope, FilterResponse response, float cutoffHz, float resoParam01)
  {
    // select filter & set type
    IFilter* nextFilter = nullptr;
    switch (circuit)
    {
      default:
      case FilterCircuit::Disabled:
        nextFilter = &mNullFilter;
        break;
      case FilterCircuit::OnePole:
        nextFilter = &mOnePole;
        break;
      case FilterCircuit::Biquad:
        nextFilter = &mBiquad;
        break;
      case FilterCircuit::Butterworth:
        nextFilter = &mButterworth;
        break;
      case FilterCircuit::Diode:
        nextFilter = &mDiode;
        break;
      case FilterCircuit::K35:
        nextFilter = &mK35;
        break;
      case FilterCircuit::Moog:
        nextFilter = &mMoog;
        break;
    }
    mSelectedFilter->SetParams(circuit, slope, response, cutoffHz, resoParam01);
    if (mSelectedFilter != nextFilter)
    {
      mSelectedFilter->Reset();
    }
    mSelectedFilter = nextFilter;
  }

  float ProcessSample(float inputSample)
  {
    return mSelectedFilter->ProcessSample(inputSample);
  }

};  // FilterNode


struct FilterAuxNode  // : IAuxEffect
{
  FilterNode
      mFilter;  // stereo. cpu optimization: combine into a stereo filter to eliminate double recalc. but it's more code size.

  ParamAccessor mParams;

  FilterCircuit mFilterCircuit;
  FilterSlope mFilterSlope;
  FilterResponse mFilterResponse;

  ModDestination mModDestBase;
  float mNoteHz = 0;
  size_t mnSampleCount = 0;
  ModMatrixNode* mModMatrix = nullptr;
  bool mEnabledCached = false;

  FilterAuxNode(float* paramCache, ParamIndices baseParamID, ModDestination modDestBase)
      : mParams(paramCache, baseParamID)
      , mModDestBase(modDestBase)
  {
  }

  void AuxBeginBlock(float noteHz, ModMatrixNode& modMatrix)
  {
    mModMatrix = &modMatrix;
    mnSampleCount = 0;  // ensure reprocessing after setting these params to avoid corrupt state.
    mNoteHz = noteHz;
    mFilterCircuit = mParams.GetEnumValue<FilterCircuit>(FilterParamIndexOffsets::FilterCircuit);
    mFilterSlope = mParams.GetEnumValue<FilterSlope>(FilterParamIndexOffsets::FilterSlope);
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
      //M7::ParamAccessor pa{&reso01, 0};
      //float q = pa.GetDivCurvedValue(0, M7::gBiquadFilterQCfg, 0);
      auto Q = mParams.GetDivCurvedValue(FilterParamIndexOffsets::Q,
                                         gBiquadFilterQCfg,
                                         mModMatrix->GetDestinationValue((int)mModDestBase +
                                                                         (int)FilterAuxModDestOffsets::Q));

      mFilter.SetParams(
          mFilterCircuit,
          mFilterSlope,
          mFilterResponse,
          mParams.GetFrequency(FilterParamIndexOffsets::Freq,
                               FilterParamIndexOffsets::FreqKT,
                               gFilterFreqConfig,
                               mNoteHz,
                               mModMatrix->GetDestinationValue((int)mModDestBase + (int)FilterAuxModDestOffsets::Freq)),
          Q);
    }

    return mFilter.ProcessSample(inputSample);
  }
};

}  // namespace M7


}  // namespace WaveSabreCore
