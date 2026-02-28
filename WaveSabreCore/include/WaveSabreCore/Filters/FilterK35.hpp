
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct K35Filter : IFilter
{
  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    //
  }
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    return false;
  }
  virtual real ProcessSample(real x) override
  {
    return x;
  }
  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    return 1.0f;
  }
  virtual void Reset() override
  {
    //
  }
}; // class K35Filter
}  // namespace M7
}  // namespace WaveSabreCore
