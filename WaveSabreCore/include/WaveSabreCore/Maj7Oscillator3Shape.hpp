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
    WVSegment mLastRunningSegment;

    bool mQueuedEdgeOnNextStep = false;
    float mQueuedEdgePreAmp = 0;
    float mQueuedEdgePreSlope = 0;

    Walker() = default;

    Walker(WVShape& parent, double cursorPhase01)
        : mpParent(&parent)
        , mCursorPhase01(math::wrap01(cursorPhase01))
    {
      mSegIndex = -1;
    }

    // jumps to the given phase, and emits an edge event if this is a discontinuity.
    template <class F>
    void JumpToPhase(double newPhase01, F&& visitEdge, double distFromPathStartToEdgeInPhase01)
    {
      if (math::FloatEquals(newPhase01, mCursorPhase01))
        return;

      // note: you can jump to the same segment you're already in.
      if (mSegIndex < 0)
        return;

      const auto [preA, preS] = mLastRunningSegment.EvalAmpSlopeAtPhase(mCursorPhase01);

      mCursorPhase01 = math::wrap01(newPhase01);
      auto ht = mpParent->GetSegmentForPhase(newPhase01);

      // if you jump to exactly the left edge of a segment, don't emit edge; it will be emitted on the next step.
      // normally that's detected via mSegmentRemainingPhase, but we can't just set that because it would falsely act
      // as if the segment completed.
      if (ht.distanceFromLeftEdgeToX < 1e-6)
      {
        mQueuedEdgeOnNextStep = true;
        mQueuedEdgePreAmp = preA;
        mQueuedEdgePreSlope = preS;
        return;
      }

      // jumping to the middle of a segment.
      mSegIndex = (int)ht.segmentIndex;
      mLastRunningSegment = mpParent->mSegments[mSegIndex];
      mSegmentRemainingPhase = mLastRunningSegment.LengthInPhase01() -
                               ht.distanceFromLeftEdgeToX;
      const auto [newAmp, newSlope] = mLastRunningSegment.EvalAmpSlopeAtPhase(mCursorPhase01);

#ifdef ENABLE_OSC_LOG
      gOscLog.Log(std::format("JumpToPhase({}, tosegmentid {}, cursorPhase {:.3f})", newPhase01, mSegIndex, mCursorPhase01));
      gOscLog.Log(std::format("  [{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]", preA, preS, newAmp, newSlope));

      // distFromPathStartToEdgeInPhase01 is meaningless here; there's no path.
      visitEdge(PhaseEventKind::Unknown, mSegIndex, mLastRunningSegment, preA, preS, newAmp, newSlope, distFromPathStartToEdgeInPhase01, "JumpToPhase");
#else
      visitEdge(PhaseEventKind::Unknown, mSegIndex, mLastRunningSegment, preA, preS, newAmp, newSlope, distFromPathStartToEdgeInPhase01);
#endif  // ENABLE_OSC_LOG
    }

    // visitEdge = void(
    //   PhaseEventKind phaseEventReason,
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

      // enters the given segment index, at the beginning of the segment; assumes it's within range but range+1 is ok (wraps to 0).
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
          double phaseAtEndOfPrevSegment = mLastRunningSegment.endPhaseIncluding1 - mSegmentRemainingPhase;
          const auto [a, s] = mLastRunningSegment.EvalAmpSlopeAtPhase(phaseAtEndOfPrevSegment);
          preA = a;
          preS = s;
        }

        mCursorPhase01 = math::wrap01(
            mCursorPhase01);  // now that we have taken our "post previous segment" sample, wrap if necessary.

        if (segIndex >= mpParent->mSegments.size())
          segIndex = 0;
        mSegIndex = segIndex;
        mLastRunningSegment = mpParent->mSegments[mSegIndex];
        mSegmentRemainingPhase = mLastRunningSegment.LengthInPhase01() -
                                 segmentPhaseAlreadyTraversed;
        const auto [newAmp, newSlope] = mLastRunningSegment.EvalAmpSlopeAtPhase(mCursorPhase01);

#ifdef ENABLE_OSC_LOG
        gOscLog.Log(std::format("enterSegment({}, cursorPhase {:.3f}, reason={})", segIndex, mCursorPhase01, reason));
        gOscLog.Log(std::format("  [{:.3f}, {:.3f}] -> [{:.3f}, {:.3f}]", preA, preS, newAmp, newSlope));
        visitEdge(PhaseEventKind::Unknown, segIndex, mLastRunningSegment, preA, preS, newAmp, newSlope, travelled, reason);
#else
        visitEdge(PhaseEventKind::Unknown, segIndex, mLastRunningSegment, preA, preS, newAmp, newSlope, travelled);
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

  // find segment at phase:
  WVSegment FindSegment(double sampleInPhase01) const
  {
    sampleInPhase01 = math::wrap01(sampleInPhase01);
    auto segHit = GetSegmentForPhase(sampleInPhase01);
    return *segHit.leftEdge;
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
