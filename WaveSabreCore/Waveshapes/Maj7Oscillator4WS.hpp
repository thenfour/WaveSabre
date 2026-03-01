#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Oscillator3Base.hpp>
#include "./Maj7Oscillator3Shape.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace M7Osc4
{

// alpha [0,1): time of the edge within the current sample.
// u = 1 - alpha = remaining fraction of the sample after the edge.
// dAmp = postAmp - preAmp, dSlope = postSlope - preSlope (slope is dy/dphase).
namespace SplitKernels
{
// * 0.5 for canonical polyBLEP normalization (dA * .5), to return correction for 1 unit.
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


struct IShapeGenerator
{
  virtual WVShape GetShape(float shapeA, float shapeB) const = 0;
};

enum class AntiAliasingOption
{
  PolyBlep = 0,
  None,
};

struct ShapeCoreStreaming : public OscillatorCore
{
  std::unique_ptr<IShapeGenerator> mShapeGen;
  WVShape mShape;
  CorrectionSpill mSpill;
  AntiAliasingOption mAaOpt;

  ShapeCoreStreaming(OscillatorWaveform w, AntiAliasingOption aaOpt, IShapeGenerator* shapeGen)
      : OscillatorCore(w)
      , mShapeGen(shapeGen)
      , mAaOpt(aaOpt)
  {
  }

  void HandleParamsChanged() override
  {
    mShape = mShapeGen->GetShape(mWaveshapeA, mWaveshapeB);
  };

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();  // no offset here
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

    // pre-reset window: walk shape edges starting at evalPhase
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

    // dynamic Reset: synthesize edge with exact deltas at the event
    if (step.hasReset)
    {
      const double alpha = step.resetAlpha01;
      const double prePh = evalPhase + alpha * dt;               // just BEFORE reset (no wrap)
      const double postPh = math::wrap01(audioRatePhaseOffset);  // AFTER reset (phase = 0 + offset)

      const auto [ampPre, slopePre] = mShape.EvalAmpSlopeAt(prePh);
      const auto [ampPost, slopePost] = mShape.EvalAmpSlopeAt(postPh);

      mSpill.add_edge(alpha, double(ampPost) - double(ampPre), double(slopePost) - double(slopePre), dt);

      // post-reset window: continue walking from postPh
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

    corr = mSpill.now;
    //y = (double)ampNaive + mSpill.now;

    return CoreSample{
        .amplitude = (mAaOpt == AntiAliasingOption::PolyBlep ? float((double)ampNaive + mSpill.now) : ampNaive),
        //.naive = (float)ampNaive,
        //.correction = (float)corr,
        //.phaseAdvance = {/* if needed */},
    };
  }
};





}  // namespace M7Osc4

}  // namespace M7

}  // namespace WaveSabreCore
