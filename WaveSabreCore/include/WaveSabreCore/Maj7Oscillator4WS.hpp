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
struct PhaseStep
{
  double phaseBegin01;  // slave phase at sample start (no offset), ∈ [0,1)
  double dt;            // per-sample phase advance in phase units, ∈ [0,1)
  bool hasReset;        // master wrapped inside this sample?
  double resetAlpha01;  // when in this sample the reset occurs, ∈ [0,1) (valid if hasReset)
  double phaseEnd01;    // slave phase at sample end (no offset), ∈ [0,1)
};

// Simple accumulator: no offset, no wrap events
struct PhaseAccumulator
{
  double mPhase01 = 0.0;
  double mDelta = 0.0;

  void setPhase01(double p)
  {
    mPhase01 = math::wrap01(p);
  }
  void setFrequencyHz(double hz)
  {
    mDelta = std::max(hz * Helpers::CurrentSampleRateRecipF, 0.0);
  }
  double getPhase01() const
  {
    return mPhase01;
  }
  double getDelta() const
  {
    return mDelta;
  }

  // advance without resets (used for slave when no hard sync)
  PhaseStep advanceOneSampleNoReset()
  {
    const double begin = mPhase01;
    const double end = math::wrap01(begin + mDelta);
    mPhase01 = end;
    return {begin, mDelta, false, 0.0, end};
  }

  // advance with an externally provided reset alpha (0..1); updates internal phase
  PhaseStep advanceOneSampleWithReset(double alpha01)
  {
    const double begin = mPhase01;
    // pre segment: alpha01 * dt (ignored for state end)
    // post segment after reset: (1 - alpha01) * dt
    const double end = math::wrap01((1.0 - alpha01) * mDelta);
    mPhase01 = end;
    return {begin, mDelta, true, alpha01, end};
  }
};

// Hard-sync pair: master only detects reset; slave just uses alpha (no offset here)
struct HardSyncPhase
{
  PhaseAccumulator master;
  PhaseAccumulator slave;
  bool enabled = false;

  void setPhase01(double p)
  {
    master.setPhase01(p);
    slave.setPhase01(p);
  }

  void setParams(float mainHz, bool hardSyncEnable, float syncHz)
  {
    enabled = hardSyncEnable;
    master.setFrequencyHz(mainHz);
    slave.setFrequencyHz(hardSyncEnable ? syncHz : mainHz);
  }

  // returns slave step; includes hasReset/resetAlpha01 if master wrapped
  PhaseStep advanceOneSample()
  {
    // detect master wrap (at most one, under dt<1)
    const double mBegin = master.getPhase01();
    const double mDt = master.getDelta();
    const double mEnd = mBegin + mDt;

    bool hasReset = false;
    double alpha = 0.0;
    if (enabled && mEnd >= 1.0)  // master wraps this sample
    {
      // when-in-sample = time to reach phase 1.0 divided by dt
      alpha = (1.0 - mBegin) / mDt;  // ∈ (0,1]
      if (alpha >= 1.0)
        alpha = 0.0;  // push to next sample per your policy
      hasReset = (alpha > 0.0);
    }

    // advance master state (always)
    master.setPhase01(math::wrap01(mEnd));

    // advance slave accordingly (no offset)
    if (!hasReset)
      return slave.advanceOneSampleNoReset();
    return slave.advanceOneSampleWithReset(alpha);
  }
};
// -------------- split kernels (current "tail" + next "head")
struct SplitKernels
{
  static inline void add_blep(double alpha, double dAmp, double& now, double& next)
  {
    const double u = 1.0 - alpha;
    now += 0.5 * dAmp * (u * u);
    next += 0.5 * dAmp * (-(alpha * alpha));
  }
  static inline void add_blamp(double alpha, double dSlope, double dt, double& now, double& next)
  {
    const double u = 1.0 - alpha;
    const double tail = dt * (-(1.0 / 3.0) * alpha * alpha * alpha + 0.5 * alpha * alpha + (1.0 / 6.0));
    const double head = dt * ((1.0 / 3.0) * u * u * u - 0.5 * u * u + (1.0 / 6.0));
    now += dSlope * tail;
    next += dSlope * head;
  }
};

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

struct SawGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float /*shapeB*/) const override
  {
    return MakeSawShape();
  }
};
struct TriGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float /*shapeB*/) const override
  {
    return MakeTriangleShape();
  }
};
struct PulseGenerator : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float /*shapeB*/) const override
  {
    return MakePulseShape(shapeA);
  }
};

struct TriPulseGenerator1 : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    return MakeTriStatePulseShape3(shapeA, shapeB);
  }
};

struct TriPulseGenerator2 : public IShapeGenerator
{
  WVShape GetShape(float shapeA, float shapeB) const override
  {
    // shapeA = pulse width (0..1)
    // shapeB defines the low & high duty cycles. when shapeB = 0.5, both are 0.5. when shapeB = 0, low=0, high=1; when shapeB=1, low=1, high=0
    double lowDuty01 = shapeB;
    double highDuty01 = 1.0 - shapeB;
    return MakeTriStatePulseShape4(shapeA, lowDuty01, highDuty01);
  }
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
  virtual void ForcefullySynchronizePhase(const OscillatorCore& src) override
  {
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
