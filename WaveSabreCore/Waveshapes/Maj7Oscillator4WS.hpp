#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Oscillator3Base.hpp>
#include "./Maj7Oscillator3Shape.hpp"
#include "./Blep.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace M7Osc4
{


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
  //std::unique_ptr<IShapeGenerator> mShapeGen;
  IShapeGenerator* mShapeGen;  // not owned
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
    const auto p = mShape.EvalAmpSlopeAt(evalPhase);
    if (mAaOpt == AntiAliasingOption::None) {
      return CoreSample{
          .amplitude = float(p[0]),
      };
    }
    double y = (double)p[0] + mSpill.now;
    double corr = mSpill.now;

    // split the sample into pre/post reset windows (if any)
    double preLen = step.hasReset ? step.resetAlpha01 : 1.0;
    double postLen = step.hasReset ? (1.0 - step.resetAlpha01) : 0.0;

    // pre-reset window: walk shape edges starting at evalPhase
    SegmentWalker w = SegmentWalker::Begin(mShape, evalPhase, dt);
    if (preLen > 0.0)
    {
      w.winLen = preLen;
      w.VisitEdges2(0, mSpill);
      // w.VisitEdges(
      //     [&](double alpha, const DoublePair& dAS)
      //     {
      //       mSpill.add_edge(alpha, dAS, dt);
      //     });
    }

    // dynamic Reset: synthesize edge with exact deltas at the event
    if (step.hasReset)
    {
      const double alpha = step.resetAlpha01;
      const double prePh = evalPhase + alpha * dt;               // just BEFORE reset (no wrap)
      const double postPh = math::wrap01(audioRatePhaseOffset);  // AFTER reset (phase = 0 + offset)

      const auto ppre = /*[ ampPre, slopePre ] =*/ mShape.EvalAmpSlopeAt(prePh);
      const auto  ppost /* [ampPost, slopePost]*/ = mShape.EvalAmpSlopeAt(postPh);

      //mSpill.add_edge(alpha, double(ampPost) - double(ampPre), double(slopePost) - double(slopePre), dt);
      //mSpill.add_edge(alpha, double(ppost[0]) - double(ppre[0]), double(ppost[1]) - double(ppre[1]), dt);
      mSpill.add_edge(alpha, ppost - ppost, dt);

      // post-reset window: continue walking from postPh
      if (postLen > 0.0)
      {
        w.ResetSubwindow(postPh, postLen);
        w.VisitEdges2(alpha, mSpill);
        // w.VisitEdges(
        //     [&](double alphaLocal, const DoublePair& dAS)
        //     {
        //       const double alphaFull = alpha + alphaLocal;
        //       mSpill.add_edge(alphaFull, dAS, dt);
        //     });
      }
    }

    corr = mSpill.now;
    //y = (double)ampNaive + mSpill.now;

    return CoreSample{
        .amplitude = float((double)p[0] + mSpill.now),
        //.naive = (float)ampNaive,
        //.correction = (float)corr,
        //.phaseAdvance = {/* if needed */},
    };
  }
};





}  // namespace M7Osc4

}  // namespace M7

}  // namespace WaveSabreCore
