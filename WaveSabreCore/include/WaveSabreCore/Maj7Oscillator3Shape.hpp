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

  struct HitTestResult
  {
    const WVSegment* leftEdge;
    size_t segmentIndex;
    double distanceFromLeftEdgeToX;
  };

  // tracks the cursor and current segment
  struct Walker
  {
    const WVShape* mpParent = nullptr;

    // the cursor is the location in phase where the next step begins.
    // it means that the point @ mCursorPhase01 has not yet been visited (the effective "end" of whatever has been walked).
    // this way we can advance the cursor to a segment boundary, where remaining=0, and not evaluate the new segment.
    // then the next step checks for segment remaining = 0 to visit the new segment.
    double mCursorPhase01 = 0;

    // represents the last evaluated segment. moves at a different time than mCursorPhase01.
    // cursor phase is moved first; when a new segment is entered, this is updated.
    double mSegmentRemainingPhase = 0;  // distance from cursor to the end of the current segment, in phase units
    int mSegIndex = -1;
    //WVSegment mCurrentSegment;

    Walker() = default;

    Walker(WVShape& parent, double cursorPhase01)
        : mpParent(&parent)
        , mCursorPhase01(math::wrap01(cursorPhase01))
    {
      mSegIndex = -1;
    }

    void JumpToPhase(double newPhase01)
    {
      // jump to phase as gracefully as possible.
      // this can happen for any reason, including just correcting for drift, so don't reset any state.
      // if the new cursor is very far out of the current segment though, then record the current amp/slope as pre-edge values,
      // and reset the segment so it gets re-evaluated and  the appropriate edge is visited.
      mCursorPhase01 = math::wrap01(newPhase01);
    }

    // visitEdge = void(
    //   size_t segmentIndex,
    //   WVSegment segmentsLeftEdge,
    //   float preEdgeAmp,
    //   float preEdgeSlope,
    //   float postEdgeAmp,
    //   float postEdgeSlope,
    //   double distFromPathStartToEdgeInPhase01,
    //   reason)
    template <class F>
    inline void Step(double stepSizePhase01,  // distance of the path to traverse, in phase units [0,1)
                     F&& visitEdge)
    {
      double remainingInPath = stepSizePhase01;
      double travelled = 0.0;

      // enters the given segment index; assumes it's within range but range+1 is ok (wraps to 0).
      // visits the left edge of that segment.
      auto enterSegment = [&](int segIndex, double segmentPhaseAlreadyTraversed, const std::string& reason)
      {
        // assumes current phase is set.

        // when entering a new segment, we need to know pre-left-edge amp & slope.
        // if we have completed the previous segment, we could use its end values.
        // if not, eval it at the current cursor phase.
        float preA = 0;
        float preS = 0;
        if (mSegIndex >= 0)
        {
          const auto& currentSegment = mpParent->mSegments[mSegIndex];
          double phaseAtEndOfPrevSegment = currentSegment.endPhaseIncluding1 - mSegmentRemainingPhase;
          const auto [a, s] = currentSegment.EvalAmpSlopeAtPhase(phaseAtEndOfPrevSegment);
          preA = a;
          preS = s;
        }

        mCursorPhase01 = math::wrap01(
            mCursorPhase01);  // now that we have taken our "post previous segment" sample, wrap if necessary.

        if (segIndex >= mpParent->mSegments.size())
          segIndex = 0;
        mSegIndex = segIndex;
        const auto& currentSegment = mpParent->mSegments[mSegIndex];
        //mCurrentSegment = mpParent->mSegments[segIndex];
        mSegmentRemainingPhase = currentSegment.endPhaseIncluding1 - currentSegment.beginPhase01 -
                                 segmentPhaseAlreadyTraversed;
        const auto [newAmp, newSlope] = currentSegment.EvalAmpSlopeAtPhase(mCursorPhase01);

#ifdef ENABLE_OSC_LOG
        gOscLog.Log(std::format("enterSegment({}, cursorPhase {:.3f}, reason={})", segIndex, mCursorPhase01, reason));
        gOscLog.Log(std::format("  [{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]", preA, preS, newAmp, newSlope));
        visitEdge(segIndex, currentSegment, preA, preS, newAmp, newSlope, travelled, reason);
#else
        visitEdge(segIndex, currentSegment, preA, preS, newAmp, newSlope, travelled);
#endif  // ENABLE_OSC_LOG
      };

      if (mSegIndex == -1)
      {
        // have to "hard" initialize the walker
        auto ht = mpParent->GetSegmentForPhase(mCursorPhase01);
        enterSegment((int)ht.segmentIndex, ht.distanceFromLeftEdgeToX, "hard init");
      }

      if (mSegmentRemainingPhase < 1e-6)
      {
        enterSegment(mSegIndex + 1, 0, "step starts exactly on edge");
      }

      while (remainingInPath > 0)
      {
        const auto& currentSegment = mpParent->mSegments[mSegIndex];
        // distance to this segment's right boundary (which is the next segment's left edge)
        const double remainingInSegment = currentSegment.endPhaseIncluding1 - mCursorPhase01;  // > 0 for valid segments

        if (remainingInPath < (remainingInSegment + 1e-6))
        {
          // We do not evaluate into the next segment (landing directly on the edge does not count)
          mCursorPhase01 = mCursorPhase01 + remainingInPath;  // do not wrap here; +1 gets wrapped on next run.
          mSegmentRemainingPhase = mSegmentRemainingPhase - remainingInPath;
          return;
        }

        // We WILL cross into the next segment. Visit that next segment's left edge once.
        travelled += remainingInSegment;
        remainingInPath -= remainingInSegment;
        mSegmentRemainingPhase = 0;

        // Move cursor exactly onto the edge (wrap if == 1.0)
        mCursorPhase01 = math::wrap01(currentSegment.endPhaseIncluding1);

        // Advance segment
        enterSegment(mSegIndex + 1, 0, "edge:cross");
      }
    }
  };

  Walker MakeWalker(double cursorPhase01)
  {
    return Walker{*this, cursorPhase01};
  }

  // segments are defined by their starting edge.
  HitTestResult GetSegmentForPhase(double xPhase01) const
  {
    xPhase01 = math::wrap01(xPhase01);
    // assumes segments are sorted by phase ascending
    // assumes >= 1 segment.
    for (size_t i = 0; i < mSegments.size(); ++i)
    {
      const auto& seg = mSegments[i];
      if (xPhase01 < seg.endPhaseIncluding1)
      {
        return HitTestResult{&seg, i, xPhase01 - seg.beginPhase01};
      }
    }
    // unreachable
    return {};
  }

  inline std::pair<float, float> EvalAmpSlopeAt(double sampleInPhase01) const
  {
    sampleInPhase01 = math::wrap01(
        sampleInPhase01);  // make sure the segment we evaluate on is the segment that contains this phase.
    auto segHit = GetSegmentForPhase(sampleInPhase01);
    return segHit.leftEdge->EvalAmpSlopeAtPhase(sampleInPhase01);
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
