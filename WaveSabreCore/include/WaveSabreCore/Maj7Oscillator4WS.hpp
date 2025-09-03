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
namespace M7Osc4
{

// alpha ∈ [0,1): time of the edge within the current sample.
// u = 1 - alpha = remaining fraction of the sample after the edge.
// dAmp = postAmp - preAmp, dSlope = postSlope - preSlope (slope is dy/dphase).
namespace SplitKernels
{
// * 0.5 for canonical polyBLEP normalization (ΔA * .5), to return correction for 1 unit.
// since delta Y is max 2 (-1 to +1), poly_blep has to halve it for the function to work.
static inline void add_blep(double alpha, double dAmp, double& now, double& next)
{
  const double u = 1.0 - alpha;
  const double halfDAmp = 0.5 * dAmp;
  now += halfDAmp * (u * u);
  next += halfDAmp * (-(alpha * alpha));
}

static inline void add_blamp(double alpha, double dSlope, double dt, double& now, double& next)
{
  static constexpr double OneThird = 1.0 / 3.0;
  const double u = 1.0 - alpha;
  const double outputScale = dSlope * dt * 0.5;
  now += outputScale * u * u * u * OneThird;
  const double um1 = u - 1.0;
  next += outputScale * -OneThird * um1 * um1 * um1;
}
};  // namespace SplitKernels

struct CorrectionSpill
{
  double now = 0.0;
  double next = 0.0;
  inline void open_sample()
  {
    now = next;
    next = 0.0;
  }
  inline void add_edge(double alpha, double dAmp, double dSlope, double dt)
  {
    if (dAmp != 0.0)
      SplitKernels::add_blep(alpha, dAmp, now, next);
    if (dSlope != 0.0)
      SplitKernels::add_blamp(alpha, dSlope, dt, now, next);
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

struct IShapeGenerator
{
  virtual WVShape GetShape(float shapeA, float shapeB) const = 0;
};

// -------------- the core
struct ShapeCoreStreaming : public OscillatorCore
{
  std::unique_ptr<IShapeGenerator> mShapeGen;
  WVShape mShape;
  HardSyncPhase mPh;  // accumulators; no offset inside
  CorrectionSpill mSpill;

  ShapeCoreStreaming(OscillatorWaveform w, IShapeGenerator* shapeGen)
      : OscillatorCore(w)
      , mShapeGen(shapeGen)
  {
  }

  void SetKRateParams(float shapeA, float shapeB, float mainFreqHz, bool enableHardSync, float syncFreqHz) override
  {
    mWaveshapeA = shapeA;
    mWaveshapeB = shapeB;
    mPh.setParams(mainFreqHz, enableHardSync, syncFreqHz);
    mShape = mShapeGen->GetShape(mWaveshapeA, mWaveshapeB);
    HandleParamsChanged();
  };

  // used by LFOs to just hard-set the phase. LFO phase, when "note restart" is disabled, is global, so
  // all individual voice LFOs should be in sync and act as if they're the same.
  // Everything after the 1st call will effectively be a NOP, so no special bandlimiting or processing necessary.
  virtual void ForcefullySynchronizePhase(const OscillatorCore& src) override {
    //mPh.SynchronizeWith(src.mPh);
  };

  virtual void RestartDueToNoteOn() override
  {
    // set phase to 0 (because the oscillator's "phase offset" is performed via audioRatePhaseOffset in renderSampleAndAdvance.
    mPh.setPhase01(0);
  };


  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPh.advanceOneSample();  // no offset here
    const double dt = step.dt;
    const double phase = step.phaseBegin01;

    mSpill.open_sample();

    // naive (evaluation) uses offset
    const double evalPhase = math::wrap01(phase + audioRatePhaseOffset);
    const auto [ampNaive, slopeNaive] = mShape.EvalAmpSlopeAt(evalPhase);
    double y = (double)ampNaive + mSpill.now;
    double corr = mSpill.now;

    // split the sample into pre/post reset windows (if any)
    double preLen = step.hasReset ? step.resetAlpha01 : 1.0;
    double postLen = step.hasReset ? (1.0 - step.resetAlpha01) : 0.0;

    // ---- pre-reset window: walk shape edges starting at evalPhase
    SegmentWalker w = SegmentWalker::Begin(mShape, evalPhase, dt);
    if (preLen > 0.0)
    {
      w.winLen = preLen;
      w.VisitEdges(
          [&](double alpha, double dA, double dS)
          {
            mSpill.add_edge(alpha, dA, dS, dt);
          });
    }

    // ---- dynamic Reset (if any): synthesize edge with exact deltas at the event
    if (step.hasReset)
    {
      const double alpha = step.resetAlpha01;
      const double prePh = evalPhase + alpha * dt;               // just BEFORE reset (no wrap)
      const double postPh = math::wrap01(audioRatePhaseOffset);  // AFTER reset (phase = 0 + offset)

      const auto [ampPre, slopePre] = mShape.EvalAmpSlopeAt(prePh);
      const auto [ampPost, slopePost] = mShape.EvalAmpSlopeAt(postPh);

      mSpill.add_edge(alpha, double(ampPost) - double(ampPre), double(slopePost) - double(slopePre), dt);

      // ---- post-reset window: continue walking from postPh
      if (postLen > 0.0)
      {
        w.ResetSubwindow(postPh, postLen);
        w.VisitEdges(
            [&](double alphaLocal, double dA, double dS)
            {
              const double alphaFull = alpha + alphaLocal;
              mSpill.add_edge(alphaFull, dA, dS, dt);
            });
      }
    }

    y = (double)ampNaive + mSpill.now;
    corr = mSpill.now;

    return CoreSample{
        .amplitude = (float)y,
        .naive = (float)ampNaive,
        .correction = (float)corr,
        .phaseAdvance = {/* if you still want to return step info, adapt here */},
    };
  }
};


}  // namespace M7Osc4

}  // namespace M7

}  // namespace WaveSabreCore
