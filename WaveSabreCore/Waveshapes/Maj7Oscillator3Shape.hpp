#pragma once

#include "./Blep.hpp"
#include "../Basic/DSPMath.hpp"
#include "../Basic/Pair.hpp"
#include "../Basic/Vector.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct WVSegment
{
  double beginPhase01 = 0;
  double endPhaseIncluding1 = 0;  // exclusive (the idea is end is always > begin to make calculating length simple)
  // y value -1 to +1 nominal
  double beginAmp = 0;
  // slope is dy/dx where x is phase in [0,1) (saw wave slope = 2 because it goes from -1 to +1 in one cycle = 2/1 = 2)
  double slope = 0;

  double LengthInPhase01() const
  {
    return endPhaseIncluding1 - beginPhase01;
  }

  // evaluate the amplitude and slope at the given absolute cycle phase (not relative to segment)
  inline DoublePair EvalAmpSlopeAtPhase(double sampleInPhase01) const;
};

struct WVShape
{
   Vector<WVSegment> mSegments;

  // find segment at phase:
  WVSegment FindSegment(double sampleInPhase01) const;

  inline DoublePair EvalAmpSlopeAt(double sampleInPhase01) const;
};

// -------------- segment walker (wrapless inside each segment; wraps across 1→0 naturally)
//using EdgeVisitor = void(double alpha, const DoublePair& dAmpSlope);
struct SegmentWalker
{
  const WVShape& shape;
  double phase0;  // absolute phase at subwindow start (no wrap)
  double dt;      // full-sample dt
  double winLen;  // subwindow length as fraction of the sample ∈ [0,1]
  //float amp0, slope0;
  DoublePair ampSlope0;

  static SegmentWalker Begin(const WVShape& sh, double phaseBegin, double dtSample);
  void ResetSubwindow(double newPhase, double newLen);
  //template <class F>  // F(alpha, dAmpSlope)
  //void VisitEdges(EdgeVisitor&& onEdge);
  void VisitEdges2(double alphaOffset, CorrectionSpill& spill);
};


}  // namespace M7

}  // namespace WaveSabreCore
