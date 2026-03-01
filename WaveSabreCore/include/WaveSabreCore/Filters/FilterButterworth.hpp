
#pragma once

#include "FilterOnePole.hpp"
#include "WaveSabreCore/BiquadFilter.h"

namespace WaveSabreCore
{
namespace M7
{
struct ButterworthFilter : IFilter
{
  CascadedBiquadFilter mCascade;

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    if (!DoesSupport(circuit, slope, response))
    {
      mCascade.Disable();
      return;
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    mCascade.SetParams(FilterCircuit::Butterworth, slope, response, cutoffHz, reso01);
  }

  virtual real ProcessSample(real x) override
  {
    return mCascade.ProcessSample((float)x);
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
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
}  // namespace M7
}  // namespace WaveSabreCore
