#pragma once

#include "Maj7Oscillator3Shape.hpp"

namespace WaveSabreCore
{
namespace M7
{
// evaluate the amplitude and slope at the given absolute cycle phase (not relative to segment)
DoublePair WVSegment::EvalAmpSlopeAtPhase(double sampleInPhase01) const
{
  // do not wrap; allow evaluating outside segment
  const double deltaPhase = sampleInPhase01 - beginPhase01;
  const double amp = beginAmp + slope * deltaPhase;
  return {amp, slope};
}

// find segment at phase:
WVSegment WVShape::FindSegment(double sampleInPhase01) const
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

DoublePair WVShape::EvalAmpSlopeAt(double sampleInPhase01) const
{
  auto seg = FindSegment(sampleInPhase01);
  return seg.EvalAmpSlopeAtPhase(sampleInPhase01);
}

SegmentWalker SegmentWalker::Begin(const WVShape& sh, double phaseBegin, double dtSample)
{
  auto p = sh.EvalAmpSlopeAt(phaseBegin);
  return {sh, phaseBegin, dtSample, 1.0, p};
}
void SegmentWalker::ResetSubwindow(double newPhase, double newLen)
{
  phase0 = newPhase;
  winLen = newLen;
  auto p = shape.EvalAmpSlopeAt(newPhase);
  ampSlope0 = p;
}

//template <class F>  // F(alpha, dAmpSlope)
//void SegmentWalker::VisitEdges(EdgeVisitor&& onEdge)
void SegmentWalker::VisitEdges2(double alphaOffset, CorrectionSpill& spill)
{
  double consumed = 0.0;
  double curPhase = phase0;
  //auto curAmpSlope = ampSlope0;
  double curAmp = ampSlope0[0];
  double curSlope = ampSlope0[1];

  while (consumed < winLen)
  {
    const WVSegment& seg = shape.FindSegment(curPhase);
    const double edgePhi = (seg.endPhaseIncluding1 >= 1.0) ? 1.0 : seg.endPhaseIncluding1;

    const double dPhaseToEdge = edgePhi - curPhase;
    const double alpha = dPhaseToEdge / dt;  // portion of the *full* sample; may exceed remaining

    if (alpha <= (winLen - consumed) && alpha >= 0.0)
    {
      const double preAlpha = alpha;
      const double preAmp = curAmp + (preAlpha * dt) * curSlope;
      //const float preSlope = curSlope;
      const DoublePair preAmpSlope = {preAmp, curSlope};

      const double postPhase = (edgePhi >= 1) ? 0 : edgePhi;
      const auto postAmpSlope = shape.EvalAmpSlopeAt(postPhase);

      spill.add_edge(consumed + preAlpha, postAmpSlope - preAmpSlope, dt);

      // step across the edge
      consumed += preAlpha;
      curPhase = postPhase;
      curAmp = postAmpSlope[0];
      curSlope = postAmpSlope[1];
    }
    else
      break;
  }
}


}  // namespace M7

}  // namespace WaveSabreCore
