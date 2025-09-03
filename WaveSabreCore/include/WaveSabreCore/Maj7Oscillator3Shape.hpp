#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Shape.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct WVSegment
{
  double beginPhase01 = 0;
  double endPhaseIncluding1 = 0;  // exclusive (the idea is end is always > begin to make calculating length simple)
  // y value -1 to +1 nominal
  float beginAmp = 0;
  // slope is dy/dx where x is phase in [0,1) (saw wave slope = 2 because it goes from -1 to +1 in one cycle = 2/1 = 2)
  float slope = 0;

  double LengthInPhase01() const
  {
    return endPhaseIncluding1 - beginPhase01;
  }

  // evaluate the amplitude and slope at the given absolute cycle phase (not relative to segment)
  inline std::pair<float, float> EvalAmpSlopeAtPhase(double sampleInPhase01) const
  {
    // do not wrap; allow evaluating outside segment
    const double deltaPhase = sampleInPhase01 - beginPhase01;
    const float amp = beginAmp + slope * (float)deltaPhase;
    return {amp, slope};
  }
};
struct WVShape
{
  std::vector<WVSegment> mSegments;

  // find segment at phase:
  WVSegment FindSegment(double sampleInPhase01) const
  {
    sampleInPhase01 = math::wrap01(sampleInPhase01);
    // assumes segments are sorted by phase ascending
    // assumes >= 1 segment.
    for (size_t i = 0; i < mSegments.size(); ++i)
    {
      const auto& seg = mSegments[i];
      if (sampleInPhase01 < seg.endPhaseIncluding1)
      {
        return seg;
      }
    }
    // unreachable
    return {};
  }

  inline std::pair<float, float> EvalAmpSlopeAt(double sampleInPhase01) const
  {
    auto seg = FindSegment(sampleInPhase01);
    return seg.EvalAmpSlopeAtPhase(sampleInPhase01);
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
