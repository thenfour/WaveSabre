#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Oscillator3Base.hpp>
#include <WaveSabreCore/Vector.hpp>

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
   Vector<WVSegment> mSegments;

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

// -------------- segment walker (wrapless inside each segment; wraps across 1→0 naturally)
struct SegmentWalker
{
  const WVShape& shape;
  double phase0;  // absolute phase at subwindow start (no wrap)
  double dt;      // full-sample dt
  double winLen;  // subwindow length as fraction of the sample ∈ [0,1]
  float amp0, slope0;

  static SegmentWalker Begin(const WVShape& sh, double phaseBegin, double dtSample)
  {
    auto [a, s] = sh.EvalAmpSlopeAt(phaseBegin);
    return {sh, phaseBegin, dtSample, 1.0, a, s};
  }
  void ResetSubwindow(double newPhase, double newLen)
  {
    phase0 = newPhase;
    winLen = newLen;
    auto [a, s] = shape.EvalAmpSlopeAt(newPhase);
    amp0 = a;
    slope0 = s;
  }

  template <class F>  // F(alpha, dAmp, dSlope)
  void VisitEdges(F&& onEdge)
  {
    double consumed = 0.0;
    double curPhase = phase0;
    float curAmp = amp0;
    float curSlope = slope0;

    while (consumed < winLen)
    {
      const WVSegment& seg = shape.FindSegment(curPhase);
      const double edgePhi = (seg.endPhaseIncluding1 >= 1.0) ? 1.0 : seg.endPhaseIncluding1;

      const double dPhaseToEdge = edgePhi - curPhase;
      const double alpha = dPhaseToEdge / dt;  // portion of the *full* sample; may exceed remaining

      if (alpha <= (winLen - consumed) && alpha >= 0.0)
      {
        const double preAlpha = alpha;
        const double preAmp = curAmp + float(preAlpha * dt) * curSlope;
        const float preSlope = curSlope;

        const double postPhase = (edgePhi >= 1.0) ? 0.0 : edgePhi;
        const auto [postAmp, postSlope] = shape.EvalAmpSlopeAt(postPhase);

        onEdge(consumed + preAlpha, double(postAmp) - double(preAmp), double(postSlope) - double(preSlope));

        // step across the edge
        consumed += preAlpha;
        curPhase = postPhase;
        curAmp = postAmp;
        curSlope = postSlope;
      }
      else
        break;
    }
  }
};


}  // namespace M7

}  // namespace WaveSabreCore
