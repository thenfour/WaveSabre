
#pragma once

#include "FilterOnePole.hpp"
#include "WaveSabreCore/BiquadFilter.h"

namespace WaveSabreCore
{
namespace M7
{
#ifdef ENABLE_BUTTERWORTH_FILTER
struct ButterworthFilter : IFilter
{
  CascadedBiquadFilter mCascade;

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         Param01 reso01, real gainDb) override
  {
  #ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    if (!DoesSupport(circuit, slope, response))
    {
      mCascade.Disable();
      return;
    }
  #endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    mCascade.SetParams(FilterCircuit::Butterworth, slope, response, cutoffHz, reso01, gainDb);
  }

  virtual real ProcessSample(real x) override
  {
    return mCascade.ProcessSample((float)x);
  }

  #ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    auto clone = std::make_unique<ButterworthFilter>();
    clone->mCascade = this->mCascade;
    return clone;
  }
  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    return mCascade.GetMagnitudeAtFrequency(freqHz);
  }

  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    return mCascade.DoesSupport(FilterCircuit::Butterworth, slope, response);
  }
  #endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void Reset() override
  {
    mCascade.Reset();
  }
};  // class ButterworthFilter

#else   // ENABLE_BUTTERWORTH_FILTER
using ButterworthFilter = CascadedBiquadFilter;  // placeholder, to avoid #ifdefs in the code that uses this.
#endif  // ENABLE_BUTTERWORTH_FILTER

}  // namespace M7
}  // namespace WaveSabreCore
