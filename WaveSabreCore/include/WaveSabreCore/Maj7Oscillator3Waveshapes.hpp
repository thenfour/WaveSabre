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

static inline WVShape MakeSawShape()
{
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
                 }};
}
static inline WVShape MakeTriangleShape()
{
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = +4.0f},
                     WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = +1.0f, .slope = -4.0f},
                 }};
}
static inline WVShape MakePulseShape(double dutyCycle01)
{
  dutyCycle01 = std::clamp(dutyCycle01, 0.001, 0.999);
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = dutyCycle01, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = dutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
                 }};
}

// three state pulse: low, high, 0
// segment 0: low
// segment 1: high
// segment 2: 0
static inline WVShape MakeTriStatePulseShape3(double masterDutyCycle01, double subDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(subDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  auto highDutyPhase = masterDutyCycle01 - lowDutyPhase;

  return WVShape{.mSegments = {
      WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 1.0f, .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 0.f, .slope = 0},
                 }};
}


// four state pulse: low, mid, high, mid
// segment 0: low
// segment 1: mid (transition up)
// segment 2: high (@ duty cycle)
// segment 3: mid (transition down)
// one way to think of the segment lengths is that this is a pulse wave within a pulse wave.
static inline WVShape MakeTriStatePulseShape4(double masterDutyCycle01, double lowDuty01, double highDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(lowDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  // scale high duty to the master duty cycle
  auto mainSeg2 = 1.0 - masterDutyCycle01; // length of the high segment area
  auto highDutyPhase = std::clamp(highDuty01 * mainSeg2, 0.0, mainSeg2 - 0.001);

  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 0.f, .slope = 0},
                     WVSegment{.beginPhase01 = masterDutyCycle01, .endPhaseIncluding1 = masterDutyCycle01 + highDutyPhase, .beginAmp = 1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = masterDutyCycle01 + highDutyPhase, .endPhaseIncluding1 = 1.0, .beginAmp = .0f, .slope = 0},
                 }};
}
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// The idea here is to describe the waveform as a series of segments. The band limited parent class should
//// be able to use this description to render the waveform with proper bandlimiting.
//template <typename TBlepExecutor, OscillatorWaveform Twf>
//struct PWMCoreT : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
//{
//  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
//  PWMCoreT()
//      : Base(1)
//  {
//  }
//
//  double mDutyCycle01 = 0.5f;  // 0..1
//
//  void HandleParamsChanged() override
//  {
//    mDutyCycle01 = std::clamp(this->mWaveshapeA, 0.001f, 0.999f);
//    Base::mShapeDirty = true;
//    Base::HandleParamsChanged();
//  }
//
//  virtual WVShape GetWaveDesc() override
//  {
//    return WVShape{
//        .mSegments = {
//            WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = mDutyCycle01, .beginAmp = -1.0f, .slope = 0},
//            WVSegment{.beginPhase01 = mDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
//        }};
//  }
//};
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//template <typename TBlepExecutor, OscillatorWaveform Twf>
//struct SawCore : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
//{
//  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
//  SawCore()
//      : Base(1)
//  {
//  }
//
//  virtual WVShape GetWaveDesc() override
//  {
//    return WVShape{.mSegments = {
//                       WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
//                   }};
//  }
//};
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//template <typename TBlepExecutor, OscillatorWaveform Twf>
//struct TriTruncCore : public BandLimitedOscillatorCore<TBlepExecutor, Twf>
//{
//  using Base = BandLimitedOscillatorCore<TBlepExecutor, Twf>;
//  TriTruncCore()
//      : Base(1)
//  {
//  }
//
//  virtual WVShape GetWaveDesc() override
//  {
//    return WVShape{.mSegments = {
//                       WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = 4},
//                       WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = -4},
//                   }};
//  }
//};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
// example of simplest core: a sine wave; no band limiting supported
struct SineCore : public OscillatorCore
{
  SineCore()
      : OscillatorCore(OscillatorWaveform::Sine)
  {
  }
  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
    float y = math::sin(math::gPITimes2 * (float)step.phaseBegin01);
    return {
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct BlepExecutor
//{
//  template <typename T>
//  static inline T BlepBefore(T x)  // before discontinuity
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    return x * x;
//  }
//
//  template <typename T>
//  static inline T BlepAfter(T x)
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    x = 1 - x;
//    return -x * x;
//  }
//
//  // correction spill
//  double mNow = 0.0;   // applies to *this* sample
//  double mNext = 0.0;  // applies to next sample
//
//  void OpenSample()
//  {
//    mNow = mNext;
//    mNext = 0;
//  }
//
//  inline void AddPolyBLEP(float u, float dAmp)
//  {
//    float uo = u;
//    u = 1.0f - static_cast<float>(u);
//    auto blepBefore = BlepBefore(u);
//    auto blepAfter = BlepAfter(u);
//#ifdef ENABLE_OSC_LOG
//    gOscLog.Log(std::format(" -> blepnow({:.3f} {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            uo,
//                            u,
//                            blepBefore,
//                            dAmp,
//                            blepBefore * dAmp,
//                            mNow,
//                            mNow + blepBefore * dAmp));
//    gOscLog.Log(std::format(" -> blepnext({:.3f} : {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            uo,
//                            u,
//                            blepAfter,
//                            dAmp,
//                            blepAfter * dAmp,
//                            mNext,
//                            mNext + blepAfter * dAmp));
//#endif  // ENABLE_OSC_LOG
//
//    mNow += dAmp * blepBefore;
//    mNext += dAmp * BlepAfter(u);
//  }
//
//
//  // u is whenInSample01 ∈ [0,1)
//  void AccumulateEdge(double edgePosInSample01, float dAmp)
//  {
//    if (dAmp != 0.0f)
//    {
//      AddPolyBLEP((float)edgePosInSample01, dAmp);
//
//      //
//      //
//      //
//      //  float u = 1.0f - static_cast<float>(edgePosInSample01);
//      //mNow += dAmp * BlepBefore(u);
//      //mNext += dAmp * BlepAfter(u);
//    }
//  }
//
//  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude)
//  {
//    return {mNow, naiveAmplitude + mNow};
//  }
//};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct ArtisnalSawCore : public OscillatorCore
//{
//  ArtisnalSawCore()
//      : OscillatorCore(OscillatorWaveform::SawArtisnal)
//  {
//  }
//  BlepExecutor mHelper;
//
//  float EvalNaiveSawAtPhase(double phase01)
//  {
//    return (float)(-1.0 + 2.0 * phase01);
//  }
//
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
//    std::vector<EdgeEvent> edgeEvents;
//
//    mHelper.OpenSample();
//    const auto naive = EvalNaiveSawAtPhase(step.phaseBegin01);
//
//    // cursor over the sample fraction and evaluation phase
//    double fPrev = 0.0;                    // previous event time in sample[0,1)
//    double phaseEval = step.phaseBegin01;  // eval phase at fPrev
//
//    for (size_t i = 0; i < step.eventCount; ++i)
//    {
//      const auto& e = step.events[i];
//      const double deltaInSample01 = e.whenInSample01 - fPrev;
//
//      // advance within the current segment (no resets until we hit the event)
//      phaseEval = (phaseEval + audioRatePhaseOffset +
//                   deltaInSample01 * step.lengthInPhase01);  // do not wrap! we need to catch 1
//
//      if (e.kind == PhaseEventKind::Wrap || e.kind == PhaseEventKind::Reset)
//      {
//        const float preAmp = EvalNaiveSawAtPhase(phaseEval);  // just BEFORE the edge
//        const double postPhase = math::wrap01(
//            double(audioRatePhaseOffset));  // AFTER the edge, not the end of sample (0) (phase reset)
//        const float postAmp = EvalNaiveSawAtPhase(postPhase);
//        const float dAmp = postAmp - preAmp;
//        if (!math::FloatEquals(dAmp, -2.f))
//        {
//          MulDiv(1, 1, 1);
//        }
//
//#ifdef ENABLE_OSC_LOG
//        gOscLog.Log(std::format("Saw {} @ u={:.3f} phase={:.3f} pre:{:.3f} -> post:{:.3f} dAmp={:.3f}",
//                                (e.kind == PhaseEventKind::Wrap) ? "wrap" : "reset",
//                                e.whenInSample01,
//                                phaseEval,
//                                preAmp,
//                                postAmp,
//                                dAmp));
//        edgeEvents.push_back(EdgeEvent{
//            .reason = e.kind,
//            .atPhase01 = (float)phaseEval,
//            .whenInSample01 = (float)e.whenInSample01,
//            .preEdgeAmp = preAmp,
//            .postEdgeAmp = postAmp,
//            .dAmp = dAmp,
//            .preEdgeSlope = +2.0f,
//            .postEdgeSlope = +2.0f,
//            .dSlope = 0.0f,
//
//        });
//#endif  // ENABLE_OSC_LOG
//
//        mHelper.AccumulateEdge(e.whenInSample01, dAmp);
//
//
//        // after the edge, the phase resets to 0 (+ offset), continue from there
//        phaseEval = postPhase;
//      }
//
//      fPrev = e.whenInSample01;
//    }
//
//    const auto [correction, amplitude] = mHelper.CloseSampleAndGetCorrection(naive);
//
//    return CoreSample{
//        .amplitude = (float)amplitude,
//        .naive = naive,
//        .correction = (float)correction,  //
//        .phaseAdvance = step,
//#ifdef ENABLE_OSC_LOG
//        .log = gOscLog.mBuffer,  //
//#endif                           // ENABLE_OSC_LOG
//    };
//  }
//};
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct BasicSawCore : public OscillatorCore
//{
//  BasicSawCore()
//      : OscillatorCore(OscillatorWaveform::SawBasic)
//  {
//  }
//
//  // t = phase in [0,1), dt = phase increment per sample in [0,1)
//  // around the saw discontinuity, this returns the correction value.
//  inline double poly_blep_saw(double t, double dt)
//  {
//    // t in [0,1), dt in (0,1)
//    if (t < dt)
//    {
//      t /= dt;  // 0..1
//      // t + t - t^2 - 1  ==  2t - t^2 - 1
//      return t + t - t * t - 1.0;
//    }
//    else if (t > 1.0 - dt)
//    {
//      t = (t - 1.0) / dt;  // -1..0
//      // t^2 + 2t + 1  ==  (t + 1)^2
//      return t * t + t + t + 1.0;
//    }
//    return 0.0;
//  }
//
//  inline float bl_saw_sample(double phase, double dt)
//  {
//    double v = 2.0 * phase - 1.0;   // naive ramp
//    v -= poly_blep_saw(phase, dt);  // polyBLEP correction
//    return (float)v;
//  }
//
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    const auto step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
//    const double dt = step.lengthInPhase01;  // per-sample phase increment
//    const double phase = step.phaseBegin01;  // evaluation phase at sample start
//
//    float y = bl_saw_sample(phase, dt);
//
//    for (int i = 0; i < step.eventCount; ++i)
//    {
//      const auto& e = step.events[i];
//      if (e.kind == PhaseEventKind::Reset)
//      {
//        // Phase at the *start* of this sample when the reset edge belongs to it.
//        // For a mid-sample reset we’re in the "tail" branch (t > 1-dt).
//        // Using the current-sample evaluation phase here works well in practice:
//        y -= (float)poly_blep_saw(phase, dt);
//      }
//    }
//    return CoreSample{
//        .amplitude = (float)y,
//        .naive = y,
//        .correction = (float)0,  //
//        .phaseAdvance = step,
//#ifdef ENABLE_OSC_LOG
//        .log = gOscLog.mBuffer,  //
//#endif                           // ENABLE_OSC_LOG
//    };
//  }
//};
//
//
//// A precomputed waveform edge: junction from seg[i] (left) to seg[i+1] (right),
//// including the wrap i = last → 0 at phase 0.
//struct WVEdge
//{
//  double phase01;  // absolute edge phase in [0,1), wrap edge is 0
//  float dAmp;      // post - pre  (amplitude step)
//  float dSlope;    // slope_after - slope_before  (slope jump)
//};
//
//// Build edges (including wrap)
//inline std::vector<WVEdge> BuildEdges(const WVShape& shape)
//{
//  std::vector<WVEdge> out;
//  const size_t n = shape.mSegments.size();
//  if (n == 0)
//    return out;
//
//  out.reserve(n);
//  for (size_t i = 0; i < n; ++i)
//  {
//    const auto& left = shape.mSegments[i];
//    const auto& right = shape.mSegments[(i + 1) % n];
//
//    // Edge phase is the *begin* of the right segment; wrap edge is 0
//    const double edgePhase = (i + 1 < n) ? right.beginPhase01 : 0.0;
//
//    // Amp just before edge (end of left), and just after edge (start of right)
//    const double leftEndPhase = left.endPhaseIncluding1;  // no wrapping here
//    const float ampL = left.beginAmp + left.slope * float(leftEndPhase - left.beginPhase01);
//    const float ampR = right.beginAmp;  // at begin of right
//
//    const float dA = ampR - ampL;
//    const float dS = right.slope - left.slope;
//
//    out.push_back(WVEdge{edgePhase, dA, dS});
//  }
//  return out;
//}
//
//
//struct ShapeCore : public OscillatorCore
//{
//  WVShape mShape;
//  std::vector<WVEdge> mEdges;  // precomputed from mShape
//
//  // t = phase in [0,1), dt = per-sample phase increment in (0,1)
//  // Returns zero unless t is within dt of an edge.
//  inline double poly_blep(double t, double dt)
//  {
//    if (t < dt)
//    {
//      t /= dt;                     // 0..1
//      return t + t - t * t - 1.0;  // 2t - t^2 - 1
//    }
//    else if (t > 1.0 - dt)
//    {
//      t = (t - 1.0) / dt;          // -1..0
//      return t * t + t + t + 1.0;  // (t+1)^2
//    }
//    return 0.0;
//  }
//
//  // polyBLAMP: kernel for slope-discontinuity (kink) anti-aliasing.
//  // This is the integrated form used for triangles, etc.
//  // It is also zero unless t is within dt of an edge.
//  inline double poly_blamp(double t, double dt)
//  {
//    if (t < dt)
//    {
//      t /= dt;  // 0..1
//      // t^3/3 - t^2/2 + 1/6   (written in a numerically-friendly grouped form)
//      return t * t * (t * (1.0 / 3.0) - 0.5) + (1.0 / 6.0);
//    }
//    else if (t > 1.0 - dt)
//    {
//      t = (t - 1.0) / dt;  // -1..0
//      // t^3/3 + t^2/2 + 1/6
//      return t * t * (t * (1.0 / 3.0) + 0.5) + (1.0 / 6.0);
//    }
//    return 0.0;
//  }
//
//  ShapeCore()
//      : OscillatorCore(OscillatorWaveform::SawShape4)
//  {
//    // saw
//    //mShape = WVShape{.mSegments = {
//    //                 WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
//    //             }};
//
//    // triangle
//    mShape = WVShape{.mSegments = {
//                         WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = 4},
//                         WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = -4},
//                     }};
//
//    // pwm
//    //mShape = WVShape{
//    //    .mSegments = {
//    //        WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = mDutyCycle01, .beginAmp = -1.0f, .slope = 0},
//    //        WVSegment{.beginPhase01 = mDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
//    //    }};
//
//    mEdges = BuildEdges(mShape);
//  }
//
//  double mDutyCycle01 = 0.5f;  // 0..1
//
//  void HandleParamsChanged() override
//  {
//    //mDutyCycle01 = std::clamp(this->mWaveshapeA, 0.001f, 0.999f);
//
//    //mShape = WVShape{
//    //    .mSegments = {
//    //        WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = mDutyCycle01, .beginAmp = -1.0f, .slope = 0},
//    //        WVSegment{.beginPhase01 = mDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
//    //    }};
//
//    //mEdges = BuildEdges(mShape);
//  }
//
//  struct PendingEdge
//  {
//    double phase01;  // absolute edge phase in [0,1), e.g. post-reset phase = wrap(offset)
//    double dAmp;     // post - pre at the reset instant
//    double dSlope;   // slope_after - slope_before at the reset instant
//    int ttl = 2;     // apply tail now and head next sample; then expire
//  };
//
//  std::vector<PendingEdge> mPending;  // in your ShapeCore/OscillatorCore
//  inline double apply_edge_corr(double phase, double dt, double edgePhi, double dAmp, double dSlope)
//  {
//    // relative position of current sample start vs. the edge center
//    const double tRel = math::wrap01(phase - edgePhi);  // ∈ [0,1)
//    // step + slope-kink kernels
//    double c = 0.0;
//    if (dAmp != 0.0)
//      c += 0.5 * dAmp * poly_blep(tRel, dt);
//    if (dSlope != 0.0)
//      c += dSlope * dt * poly_blamp(tRel, dt);
//    return c;
//  }
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    const PhaseAdvance step = mPhaseAcc.advanceOneSample(audioRatePhaseOffset);
//    const double dt = step.lengthInPhase01;
//    const double phase = step.phaseBegin01;
//
//    // 1) naive sample
//    const auto [ampNaive, slopeNaive] = mShape.EvalAmpSlopeAt(phase);
//    double y = (double)ampNaive;
//    double corr = 0.0;
//
//    // 2) apply any PENDING dynamic edges (from previous sample’s resets)
//    //    (this gives you the "head" portion automatically)
//    for (auto& pe : mPending)
//    {
//      const double c = apply_edge_corr(phase, dt, pe.phase01, pe.dAmp, pe.dSlope);
//      y += c;
//      corr += c;
//    }
//
//    // 3) STATIC edges from the shape (junctions + natural wrap at φ=0)
//    //    BUT: suppress any static edge whose φ matches a pending dynamic edge φ,
//    //    to avoid using pre-baked deltas when a reset replaced that edge this cycle.
//    auto is_suppressed = [&](double staticPhi)
//    {
//      for (const auto& pe : mPending)
//        if (std::abs(staticPhi - pe.phase01) < 1e-12)
//          return true;  // tiny tol, no epsilon games elsewhere
//      return false;
//    };
//
//    for (const WVEdge& e : mEdges)
//    {
//      if (is_suppressed(e.phase01))
//        continue;  // dynamic edge owns this φ this cycle
//      const double c = apply_edge_corr(phase, dt, e.phase01, (double)e.dAmp, (double)e.dSlope);
//      y += c;
//      corr += c;
//    }
//
//    // 4) Handle RESET events occurring inside THIS sample:
//    //    create a dynamic edge with the TRUE (dAmp,dSlope) at the reset,
//    //    and apply its "tail" immediately by calling apply_edge_corr once now.
//    for (int i = 0; i < step.eventCount; ++i)
//    {
//      const auto& ev = step.events[i];
//      if (ev.kind != PhaseEventKind::Reset)
//        continue;
//
//      const double alpha = ev.whenInSample01;                    // 0..1 (fraction into this sample)
//      const double prePh = phase + alpha * dt;                   // just BEFORE reset (no wrap)
//      const double postPh = math::wrap01(audioRatePhaseOffset);  // AFTER reset (phase = 0+offset)
//
//      const auto [ampPre, slopePre] = mShape.EvalAmpSlopeAt(prePh);
//      const auto [ampPost, slopePost] = mShape.EvalAmpSlopeAt(postPh);
//
//      PendingEdge pe{
//          .phase01 = postPh,                               // edge is centered at the post-reset phase
//          .dAmp = (double)ampPost - (double)ampPre,        // true step
//          .dSlope = (double)slopePost - (double)slopePre,  // true slope jump
//          .ttl = 2,
//      };
//
//      // Tail correction belongs to THIS sample as well:
//      const double cTail = apply_edge_corr(phase, dt, pe.phase01, pe.dAmp, pe.dSlope);
//      y += cTail;
//      corr += cTail;
//
//      // Queue edge so that the HEAD fires next sample with the same (dAmp,dSlope)
//      mPending.push_back(pe);
//    }
//
//    // 5) Age out pending edges (after two samples they’re guaranteed zero)
//    //    Do it AFTER we computed this sample so they persist into the next.
//    for (auto& pe : mPending)
//      --pe.ttl;
//    mPending.erase(std::remove_if(mPending.begin(),
//                                  mPending.end(),
//                                  [](const PendingEdge& p)
//                                  {
//                                    return p.ttl <= 0;
//                                  }),
//                   mPending.end());
//
//    return CoreSample{
//        .amplitude = (float)y,
//        .naive = (float)ampNaive,
//        .correction = (float)corr,
//        .phaseAdvance = step,
//    };
//  }
//};


/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//struct SplitKernels
//{
//  // BLEP (step) split. alpha = event time in current sample, ∈ [0,1)
//  // u = remaining fraction after the edge within this sample = 1 - alpha
//  // Coefficient uses 0.5 * dAmp to match the standard per-sample polyBLEP normalization.
//  static inline void add_blep(double alpha, double dAmp, double& now, double& next)
//  {
//    const double u = 1.0 - alpha;
//    dAmp *= 0.5;
//    now += dAmp * (u * u);
//    next += dAmp * (-(alpha * alpha));  // -(1-u)^2 == -alpha^2
//  }
//
//  // BLAMP (slope kink) split. Scales with dt (kernel width).
//  // Tail (current) is a polynomial in alpha; head (next) in u = 1 - alpha.
//  static inline void add_blamp(double alpha, double dSlope, double dt, double& now, double& next)
//  {
//    const double u = 1.0 - alpha;
//
//    // Tail (current sample):  dt * ( -1/3 α^3 + 1/2 α^2 + 1/6 )
//    const double tail = dt * (-(1.0 / 3.0) * alpha * alpha * alpha + 0.5 * alpha * alpha + (1.0 / 6.0));
//
//    // Head (next sample):     dt * ( +1/3 u^3    - 1/2 u^2    + 1/6 )
//    const double head = dt * ((1.0 / 3.0) * u * u * u - 0.5 * u * u + (1.0 / 6.0));
//
//    now += dSlope * tail;
//    next += dSlope * head;
//  }
//};
//
////
//struct CorrectionSpill
//{
//  double mNow = 0.0;   // added to this sample
//  double mNext = 0.0;  // carried to next sample
//
//  inline void OpenSample(double dt)
//  {
//    mNow = mNext;
//    mNext = 0.0;
//  }
//  inline void AccumulateEdge(double alpha, double dAmp, double dSlope, double dt)
//  {
//    if (dAmp != 0.0)
//      SplitKernels::add_blep(alpha, dAmp, mNow, mNext);
//    if (dSlope != 0.0)
//      SplitKernels::add_blamp(alpha, dSlope, dt, mNow, mNext);
//  }
//};
//
//
//// i am pretty sure this is the closest yet, but it's still not perfect.
//// 1. phase offsets reveal an issue
//// 2. blamps are broken entirely but likely just need a small adjustment
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//struct BlepExecutor2
//{
//  template <typename T>
//  static inline T BlepBefore(T x)  // before discontinuity
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    return x * x;
//  }
//
//  template <typename T>
//  static inline T BlepAfter(T x)
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    x = 1 - x;
//    return -x * x;
//  }
//
//  inline void AddPolyBLEP(float u, float dAmp, double& now, double& next)
//  {
//    float uo = u;
//    u = 1.0f - static_cast<float>(u);
//    u *= (float)mSampleWindowSizeInPhase01;
//    auto blepBefore = BlepBefore(u);
//    auto blepAfter = BlepAfter(u);
//#ifdef ENABLE_OSC_LOG
//    gOscLog.Log(std::format(" -> blepnow({:.3f} {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            uo,
//                            u,
//                            blepBefore,
//                            dAmp,
//                            blepBefore * dAmp,
//                            now,
//                            now + blepBefore * dAmp));
//    gOscLog.Log(std::format(" -> blepnext({:.3f} : {:.3f}) = {:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            uo,
//                            u,
//                            blepAfter,
//                            dAmp,
//                            blepAfter * dAmp,
//                            next,
//                            next + blepAfter * dAmp));
//#endif  // ENABLE_OSC_LOG
//
//    now += dAmp * blepBefore;
//    next += dAmp * blepAfter;
//  }
//
//  template <typename T>
//  static inline T BlampBefore(T x)  // before discontinuity
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    static constexpr T OneThird = T{1} / T{3};
//    return x * x * x * OneThird;
//  }
//
//  template <typename T>
//  static inline T BlampAfter(T x)
//  {
//    static_assert(std::is_floating_point<T>::value, "requires a floating point type");
//    static constexpr T NegOneThird = T{-1} / T{3};
//    x = x - 1;
//    return NegOneThird * x * x * x;
//  }
//
//  inline void AddPolyBLAMP(float u, float dSlope, double& now, double& next)
//  {
//    u = 1.0f - static_cast<float>(u);
//    //u /= mSampleWindowSizeInPhase01;
//    auto blampBefore = BlampBefore(u);
//    auto blampAfter = BlampAfter(u);
//#ifdef ENABLE_OSC_LOG
//    gOscLog.Log(std::format(" -> blampnow :{:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            blampBefore,
//                            dSlope,
//                            blampBefore * dSlope,
//                            now,
//                            now + blampBefore * dSlope));
//    gOscLog.Log(std::format(" -> blampnext:{:.3f} * scale:{:.3f} = {:.3f} + now{:.3f} = {:.3f}",
//                            blampAfter,
//                            dSlope,
//                            blampAfter * dSlope,
//                            next,
//                            next + blampAfter * dSlope));
//#endif  // ENABLE_OSC_LOG
//    now += dSlope * blampBefore;
//    next += dSlope * blampAfter;
//  }
//
//  // Spill buffers (carry-over tails)
//  double mNow = 0.0;   // applies to *this* sample
//  double mNext = 0.0;  // applies to next sample
//
//  double mSampleWindowSizeInPhase01 = 0.0;  // == step.phaseDelta01PerSample
//
//  void OpenSample(double sampleWindowSizeInPhase01)
//  {
//    mSampleWindowSizeInPhase01 = sampleWindowSizeInPhase01;
//
//    // Bring down tails
//    mNow = mNext;
//    mNext = 0;
//  }
//
//  // u is whenInSample01 ∈ [0,1)
//  void AccumulateEdge(double edgePosInSample01,
//                      float dAmp,
//                      float dSlopePerPhase
//#ifdef ENABLE_OSC_LOG
//                      ,
//                      std::string reason
//#endif  // ENABLE_OSC_LOG
//  )
//  {
//    //const double u = edgePosInSample01;  // when in this sample [0,1)
//    if (dAmp != 0.0f)
//    {
//#ifdef ENABLE_OSC_LOG
//      gOscLog.Log(std::format(
//          "add BLEP @ u={:.3f} dAmp={:.3f} dSlope={:.3f} ({})", edgePosInSample01, dAmp, dSlopePerPhase, reason));
//#endif                                                           // ENABLE_OSC_LOG
//      AddPolyBLEP((float)edgePosInSample01, dAmp, mNow, mNext);  // impl todo
//    }
//    if (dSlopePerPhase != 0.0f)
//    {
//#ifdef ENABLE_OSC_LOG
//      gOscLog.Log(std::format(
//          "add BLAMP @ u={:.3f} dAmp={:.3f} dSlope={:.3f} ({})", edgePosInSample01, dAmp, dSlopePerPhase, reason));
//#endif  // ENABLE_OSC_LOG
//      const double dSlope_perSample = dSlopePerPhase * mSampleWindowSizeInPhase01;
//      AddPolyBLAMP((float)edgePosInSample01, (float)dSlope_perSample, mNow, mNext);  // impl todo
//    }
//  }
//
//  //std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude)
//  //{
//  //  return {mNow, naiveAmplitude + mNow * mCorrectionAmt};
//  //}
//};
//
//
////
//struct SegmentWalker
//{
//  const WVShape& shape;
//  // Cursor state at the beginning of the current subwindow:
//  double phase0;     // absolute phase at subwindow start
//  double dt;         // per-sample phase advance (width of full sample)
//  double windowLen;  // subwindow length as fraction of the sample ∈ [0,1]
//  float amp0;        // amplitude at phase0
//  float slope0;      // slope at phase0
//
//  // Initialize at sample start
//  static SegmentWalker Begin(const WVShape& sh, double phaseBegin, double dtSample)
//  {
//    auto [amp, slope] = sh.EvalAmpSlopeAt(phaseBegin);
//    return SegmentWalker{sh, phaseBegin, dtSample, 1.0, amp, slope};
//  }
//
//  // Re-initialize for post-reset subwindow
//  void ResetSubwindow(double newPhase, double newWindowLen)
//  {
//    phase0 = newPhase;
//    windowLen = newWindowLen;
//    auto ps = shape.EvalAmpSlopeAt(newPhase);
//    amp0 = ps.first;
//    slope0 = ps.second;
//  }
//
//  // Emit all internal SHAPE boundaries crossed in this subwindow.
//  // Calls F(alpha, dAmp, dSlope) with alpha ∈ (0, windowLen].
//  template <class F>
//  void VisitShapeEdges(F&& onEdge)
//  {
//    double consumed = 0.0;
//    double cursorPhase = phase0;
//    float cursorAmp = amp0;
//    float cursorSlope = slope0;
//
//    while (consumed < windowLen)
//    {
//      // Locate end of current segment (absolute phase)
//      const WVSegment& seg = *shape.GetSegmentForPhase(cursorPhase).leftEdge;
//      const double edgePhi = (seg.endPhaseIncluding1 >= 1.0) ? 1.0 : seg.endPhaseIncluding1;
//
//      // Distance to that edge in phase units (mod-1 not needed within a segment)
//      const double dPhaseToEdge = edgePhi - cursorPhase;
//      // Convert to fraction of the *sample window*
//      const double alpha = dPhaseToEdge / dt;  // may be > remaining window
//
//      if (alpha <= (windowLen - consumed))
//      {
//        // Advance to just BEFORE the edge inside this subwindow
//        const double preAlpha = std::max(0.0, alpha);  // guard numerical jitter
//        const double preAmp = cursorAmp + float(preAlpha * dt) * cursorSlope;
//        const float preSlope = cursorSlope;
//
//        // POST values are taken from the right-hand side of the edge
//        // Wrap to 0 when hitting the cycle end.
//        const double postPhase = (edgePhi >= 1.0) ? 0.0 : edgePhi;
//        const auto [postAmp, postSlope] = shape.EvalAmpSlopeAt(postPhase);
//
//        const double dAmp = double(postAmp) - double(preAmp);
//        const double dSlope = double(postSlope) - double(preSlope);
//
//        onEdge(consumed + preAlpha, dAmp, dSlope);  // alpha within current subwindow
//
//        // Step over the edge: update cursor for remaining window
//        consumed += preAlpha;
//        cursorPhase = postPhase;
//        cursorAmp = postAmp;
//        cursorSlope = postSlope;
//      }
//      else
//      {
//        // No more edges within the remaining subwindow
//        break;
//      }
//    }
//  }
//};
//
//// no baked edges, fully dynamic, facilitating morphing and modulation
//struct ShapeCoreStreaming : public OscillatorCore
//{
//  WVShape mShape;
//  CorrectionSpill mBLHelper;
//  //BlepExecutor2 mBLHelper;
//
//  ShapeCoreStreaming(OscillatorWaveform wf, const WVShape& shape)
//      : OscillatorCore(wf)
//      , mShape(shape)
//  {
//  }
//
//  //CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  //{
//  //  // don't pass offset into advanceOneSample. this offset is a "rendering offset",
//  //  // but phase advance is about advancing a sample and getting an execution plan. that includes resets & wraps within the sample.
//  //  // rendering the waveform is OUR concern which is where the offset matters.
//  //  const PhaseAdvance step = mPhaseAcc.advanceOneSample(0);
//  //  const double dt = step.lengthInPhase01;  // phase width per sample
//  //  const double phase = step.phaseBegin01;  // eval phase at sample start
//
//  //  //mSpill.open_sample();
//  //  mBLHelper.OpenSample(dt);
//
//  //  // alpha = event time in current sample, ∈ [0,1)
//  //  const auto accumulateEdge = [&](double alpha)
//  //  {
//  //    // the "phase before wrap" is tricky. if phase offset is 0, it's just 1.0.
//  //    // but with phase offset, it's 1.0 + offset, which may wrap around. but we still want to catch 1.0 exactly.
//  //    // so with pre-reset-phase, it's [0,1], but with post-reset-phase it's [0,1). weird.
//  //    //
//  //    // also important: pre and post phase NEED to both account for the offset in the same way.
//  //    // `phase` bakes the offset in, but the reset
//  //    const double prePh = phase + alpha * dt;                   // just before wrap
//  //    const double postPh = math::wrap01(audioRatePhaseOffset);  // 0 + offset
//  //    const auto [ampPre, slopePre] = mShape.EvalAmpSlopeAt(prePh);
//  //    const auto [ampPost, slopePost] = mShape.EvalAmpSlopeAt(postPh);
//  //    mBLHelper.AccumulateEdge(alpha, double(ampPost) - double(ampPre), double(slopePost) - double(slopePre), dt);
//  //  };
//
//  //  // Naive sample from current phase
//  //  const auto [ampNaive, slopeNaive] = mShape.EvalAmpSlopeAt(phase);
//  //  double y = (double)ampNaive + mBLHelper.mNow;  // add spills from prior edges
//
//  //  // ---- Pre-reset subwindow (or full window if no reset)
//  //  double preLen = 1.0;  // default full window
//  //  double postLen = 0.0;
//  //  if (step.eventCount && step.events[0].kind == PhaseEventKind::Reset)
//  //  {
//  //    preLen = step.events[0].whenInSample01;
//  //    postLen = 1.0 - preLen;
//  //  }
//
//  //  SegmentWalker w = SegmentWalker::Begin(mShape, phase + audioRatePhaseOffset, dt);
//
//  //  // Visit shape edges before reset (kinks/steps inside the window)
//  //  if (preLen > 0.0)
//  //  {
//  //    w.windowLen = preLen;
//  //    w.VisitShapeEdges(
//  //        [&](double alpha, double dA, double dS)
//  //        {
//  //          mBLHelper.AccumulateEdge(alpha, dA, dS, dt);
//  //        });
//
//  //    // Handle WRAP if it occurs in this pre window (from step events)
//  //    for (int i = 0; i < step.eventCount; ++i)
//  //      if (step.events[i].kind == PhaseEventKind::Wrap && step.events[i].whenInSample01 <= preLen)
//  //      {
//  //        accumulateEdge(step.events[i].whenInSample01);
//  //      }
//  //  }
//
//  //  // ---- Reset event (if any): treat as dynamic edge at its alpha
//  //  if (postLen > 0.0)
//  //  {
//  //    const double alpha = step.events[0].whenInSample01;
//  //    accumulateEdge(alpha);
//  //    const double postPh = math::wrap01(audioRatePhaseOffset);  // 0 + offset
//
//  //    // ---- Post-reset subwindow: continue walking shape from postPh
//  //    w.ResetSubwindow(postPh, postLen);
//  //    w.VisitShapeEdges(
//  //        [&](double alphaLocal, double dA, double dS)
//  //        {
//  //          // alpha for the full sample window:
//  //          const double alphaFull = alpha + alphaLocal;  // alpha was the reset time
//  //          mBLHelper.AccumulateEdge(alphaFull, dA, dS, dt);
//  //        });
//
//  //    // Also consider WRAP inside post window, if present
//  //    for (int i = 0; i < step.eventCount; ++i)
//  //      if (step.events[i].kind == PhaseEventKind::Wrap && step.events[i].whenInSample01 > alpha)
//  //      {
//  //        const double alphaFull = step.events[i].whenInSample01;
//  //        const double prePh2 = postPh + (alphaFull - alpha) * dt;
//  //        const double postPh2 = math::wrap01(audioRatePhaseOffset);  // 0 + offset
//  //        const auto [a0, s0] = mShape.EvalAmpSlopeAt(prePh2);
//  //        const auto [a1, s1] = mShape.EvalAmpSlopeAt(postPh2);
//  //        mBLHelper.AccumulateEdge(alphaFull, double(a1) - double(a0), double(s1) - double(s0), dt);
//  //      }
//  //  }
//
//  //  // Final corrected output for this sample
//  //  y = (double)ampNaive + mBLHelper.mNow;
//
//  //  return CoreSample{
//  //      .amplitude = (float)y,
//  //      .naive = (float)ampNaive,
//  //      .correction = (float)mBLHelper.mNow,
//  //      .phaseAdvance = step,
//  //  };
//  //}
//
//
//  // concepts:
//  // - phase advance should not account for offset.
//  // - totally ignore phase wrap events; wrapping is handled by the shape walker.
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    // don't pass offset into advanceOneSample. this offset is a "rendering offset",
//    // but phase advance is about advancing a sample and getting an execution plan. that includes resets & wraps within the sample.
//    // rendering the waveform is OUR concern which is where the offset matters.
//    const PhaseAdvance step = mPhaseAcc.advanceOneSample(0);
//    const double dt = step.lengthInPhase01;  // phase width per sample
//    const double phase = step.phaseBegin01;  // eval phase at sample start
//
//    //mSpill.open_sample();
//    mBLHelper.OpenSample(dt);
//
//    // alpha = event time in current sample, ∈ [0,1)
//    const auto accumulateEdge = [&](double alpha)
//    {
//      // the "phase before wrap" is tricky. if phase offset is 0, it's just 1.0.
//      // but with phase offset, it's 1.0 + offset, which may wrap around. but we still want to catch 1.0 exactly.
//      // so with pre-reset-phase, it's [0,1], but with post-reset-phase it's [0,1). weird.
//      //
//      // also important: pre and post phase NEED to both account for the offset in the same way.
//      // `phase` bakes the offset in, but the reset
//      const double prePh = math::wrap01(audioRatePhaseOffset + phase + alpha * dt);  // just before wrap
//      const double postPh = math::wrap01(audioRatePhaseOffset);                      // 0 + offset
//      const auto [ampPre, slopePre] = mShape.EvalAmpSlopeAt(prePh);
//      const auto [ampPost, slopePost] = mShape.EvalAmpSlopeAt(postPh);
//      mBLHelper.AccumulateEdge(alpha, double(ampPost) - double(ampPre), double(slopePost) - double(slopePre), dt);
//    };
//
//    // Naive sample from current phase
//    const auto [ampNaive, slopeNaive] = mShape.EvalAmpSlopeAt(phase);
//    double y = (double)ampNaive + mBLHelper.mNow;  // add spills from prior edges
//
//    // ---- Pre-reset subwindow (or full window if no reset)
//    double preLen = 1.0;  // default full window
//    double postLen = 0.0;
//    if (step.eventCount && step.events[0].kind == PhaseEventKind::Reset)
//    {
//      preLen = step.events[0].whenInSample01;
//      postLen = 1.0 - preLen;
//    }
//
//    SegmentWalker w = SegmentWalker::Begin(mShape, phase + audioRatePhaseOffset, dt);
//
//    // Visit shape edges before reset (kinks/steps inside the window)
//    if (preLen > 0.0)
//    {
//      w.windowLen = preLen;
//      w.VisitShapeEdges(
//          [&](double alpha, double dA, double dS)
//          {
//            mBLHelper.AccumulateEdge(alpha, dA, dS, dt);
//          });
//
//      // Handle WRAP if it occurs in this pre window (from step events)
//      for (int i = 0; i < step.eventCount; ++i)
//        if (step.events[i].kind == PhaseEventKind::Wrap && step.events[i].whenInSample01 <= preLen)
//        {
//          accumulateEdge(step.events[i].whenInSample01);
//        }
//    }
//
//    // ---- Reset event (if any): treat as dynamic edge at its alpha
//    if (postLen > 0.0)
//    {
//      const double alpha = step.events[0].whenInSample01;
//      accumulateEdge(alpha);
//      const double postPh = math::wrap01(audioRatePhaseOffset);  // 0 + offset
//
//      // ---- Post-reset subwindow: continue walking shape from postPh
//      w.ResetSubwindow(postPh, postLen);
//      w.VisitShapeEdges(
//          [&](double alphaLocal, double dA, double dS)
//          {
//            // alpha for the full sample window:
//            const double alphaFull = alpha + alphaLocal;  // alpha was the reset time
//            mBLHelper.AccumulateEdge(alphaFull, dA, dS, dt);
//          });
//
//      // Also consider WRAP inside post window, if present
//      for (int i = 0; i < step.eventCount; ++i)
//        if (step.events[i].kind == PhaseEventKind::Wrap && step.events[i].whenInSample01 > alpha)
//        {
//          const double alphaFull = step.events[i].whenInSample01;
//          const double prePh2 = math::wrap01(audioRatePhaseOffset + postPh + (alphaFull - alpha) * dt);
//          const double postPh2 = math::wrap01(audioRatePhaseOffset);  // 0 + offset
//          const auto [a0, s0] = mShape.EvalAmpSlopeAt(prePh2);
//          const auto [a1, s1] = mShape.EvalAmpSlopeAt(postPh2);
//          mBLHelper.AccumulateEdge(alphaFull, double(a1) - double(a0), double(s1) - double(s0), dt);
//        }
//    }
//
//    // Final corrected output for this sample
//    y = (double)ampNaive + mBLHelper.mNow;
//
//    return CoreSample{
//        .amplitude = (float)y,
//        .naive = (float)ampNaive,
//        .correction = (float)mBLHelper.mNow,
//        .phaseAdvance = step,
//    };
//  }
//};


}  // namespace M7

}  // namespace WaveSabreCore
