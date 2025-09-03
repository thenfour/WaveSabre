#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3BlepExec.hpp"
#include "Maj7Oscillator3Shape.hpp"
//
//namespace WaveSabreCore
//{
//namespace M7
//{
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//template <typename TBlepExecutor, OscillatorWaveform waveformType>
//struct BandLimitedOscillatorCore : public OscillatorCore
//{
//protected:
//  BandLimitedOscillatorCore(float correctionAmt /* for debugging */)
//      : OscillatorCore(waveformType)
//      , mCorrectionAmt(correctionAmt)
//  {
//  }
//
//public:
//  static_assert(std::is_base_of<IBlepExecutor, TBlepExecutor>::value, "TBlepExecutor must implement IBlepExecutor");
//  TBlepExecutor mHelper;
//  float mCorrectionAmt;  // debugging only
//  bool mSkipZeroEdgeOnce = false;
//
//  bool mWalkerInitialized = false;
//  bool mShapeDirty = true;
//  WVShape mShape;
//  WVShape::Walker mWalker;
//
//  virtual WVShape GetWaveDesc() = 0;
//
//  virtual void SetCorrectionFactor(float factor) override
//  {
//    mHelper.mCorrectionAmt = factor * mCorrectionAmt;
//  }
//
//  void HandleParamsChanged() override
//  {
//    SetCorrectionFactor(mWaveshapeB);
//    if (mShapeDirty)
//    {
//      mShape = GetWaveDesc();
//      if (!mWalkerInitialized)
//      {
//        mWalkerInitialized = true;
//        mWalker = mShape.MakeWalker(0);
//      }
//      mShapeDirty = false;
//    }
//  }
//
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    //audioRatePhaseOffset = math::wrap01(audioRatePhaseOffset);
//
//    std::vector<EdgeEvent> edgeEventsThisSample;
//
//    const PhaseAdvance step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
//    mHelper.OpenSample(step.lengthInPhase01);
//
//    //const double sampleStartInPhase01 = math::wrap01(step.phaseBegin01);
//    const auto [naive, naiveSlope] = mShape.EvalAmpSlopeAt(step.phaseBegin01);
//
//#ifdef ENABLE_OSC_LOG
//    gOscLog.Log(std::format("Let's render a sample"));
//
//    gOscLog.Log(std::format("Ph {:.3f} -> {:.3f} (dist {:.3f})",
//                            step.phaseBegin01,
//                            (step.phaseBegin01 + step.lengthInPhase01),
//                            step.lengthInPhase01));
//    gOscLog.Log(std::format("Naive amplitude: {:.3f}", naive));
//#endif  // ENABLE_OSC_LOG
//
//    auto handleEncounteredEdge = [&](PhaseEventKind phaseEventReason,
//                                     size_t segmentIndex,
//                                     const WVSegment& edge,
//                                     float preEdgeAmp,
//                                     float preEdgeSlope,
//                                     float postEdgeAmp,
//                                     float postEdgeSlope,
//                                     double distFromPathStartToEdgeInPhase01
//#ifdef ENABLE_OSC_LOG
//                                     ,
//                                     std::string reason
//#endif  // ENABLE_OSC_LOG
//                                 )
//    {
//      auto edgePosInSample01 = distFromPathStartToEdgeInPhase01 / step.lengthInPhase01;
//
//      // track for calculating deltas. Now, these must be theoretical values, not sampled values.
//      // for example a saw's BLEP scale is 2.0f, but if you use running sample values you don't ever actually reach Y values which differ by 2.0f.
//      // HOWEVER if you do a sync restart mid-cycle / mid-segment, then we are synthesizing an edge
//      // and it can be calculated by sampling
//      const float dAmp = postEdgeAmp - preEdgeAmp;
//      const float dSlope = postEdgeSlope - preEdgeSlope;
//
//      edgeEventsThisSample.push_back(EdgeEvent{
//          .reason = phaseEventReason,
//          .atPhase01 = (float)edge.endPhaseIncluding1,
//          .whenInSample01 = (float)edgePosInSample01,
//          .preEdgeAmp = preEdgeAmp,
//          .postEdgeAmp = postEdgeAmp,
//          .dAmp = dAmp,
//          .preEdgeSlope = preEdgeSlope,
//          .postEdgeSlope = postEdgeSlope,
//          .dSlope = dSlope,
//      });
//
//      if (math::FloatEquals(dAmp + dSlope, 0.f))
//        return;
//
//      mHelper.AccumulateEdge(edgePosInSample01,
//                             dAmp,
//                             dSlope
//#ifdef ENABLE_OSC_LOG
//                             ,
//                             reason
//#endif  // ENABLE_OSC_LOG
//      );
//    };
//
//    // instead of trying to keep it synchronized with note ons etc; it's safe to do it here. mostly a NOP but will catch externally modified phase changes.
//    mWalker.JumpToPhase(audioRatePhaseOffset, handleEncounteredEdge, 0);
////
////    const auto [wrapEvent, syncEvent] = step.GetWrapAndSyncEvents();
////
////    //const auto sampleWindowSizeInPhase01 = step.lengthInPhase01;
////    if (!syncEvent.has_value())
////    {
////      // Single continuous path: start at phi0Render, advance by delta
////      mWalker.Step(step.lengthInPhase01, handleEncounteredEdge);
////    }
////    else
////    {
////      const auto& resetEvent = syncEvent.value();
////      // Pre-reset path: [0 .. tReset)
////#ifdef ENABLE_OSC_LOG
////      M7::gOscLog.Log(std::format("visit pre-sync-reset window"));
////#endif  // ENABLE_OSC_LOG
////      const double preResetLengthInPhase01 = resetEvent.whenInSample01 *
////                                             step.lengthInPhase01;  // distance from start to reset, in phase units
////
////      mWalker.Step(preResetLengthInPhase01, handleEncounteredEdge);
////
////      mWalker.JumpToPhase(audioRatePhaseOffset, handleEncounteredEdge, resetEvent.whenInSample01);  // reset to phase 0 + offset
////
////      // Post-reset path: [tR .. 1)
////#ifdef ENABLE_OSC_LOG
////      M7::gOscLog.Log(std::format("visit post-sync-reset window"));
////#endif                                                              // ENABLE_OSC_LOG
////      //const double resetPosInSample01 = resetEvent.whenInSample01;  // when in this sample [0,1)
////      const double postResetLengthInPhase01 = (1.0 - resetEvent.whenInSample01) * step.lengthInPhase01;
////      mWalker.Step(postResetLengthInPhase01, handleEncounteredEdge);
////    }
//
//    const auto [correction, amplitude] = mHelper.CloseSampleAndGetCorrection(naive);
//
//    return CoreSample{
//        .amplitude = (float)amplitude,
//        .naive = naive,
//        .correction = (float)correction,  //
//        .phaseAdvance = step,
//#ifdef ENABLE_OSC_LOG
//        .edgeEvents = edgeEventsThisSample,
//        .log = gOscLog.mBuffer,  //
//#endif                           // ENABLE_OSC_LOG
//    };
//  }
//};
//
//}  // namespace M7
//
//}  // namespace WaveSabreCore
