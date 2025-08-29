#pragma once


#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"

namespace WaveSabreCore
{
namespace M7
{

struct SegBoundary
{
  double phase01;
  float dAmp;
  float dSlope;
};
struct CumShape
{
  float mAmpAt0;
  std::vector<SegBoundary> mEdges;

  // Evaluate naive amplitude and slope at canonical phase p ∈ [0,1)
  inline std::pair<float, float> EvalAmpSlopeAt(double p) const
  {
    float amp = mAmpAt0;  // post-edge value at 0
    float slope = 0.0f;
    double prev = 0.0;

    for (const auto& e : mEdges)
    {
      if (p < e.phase01)
      {
        amp += slope * float(p - prev);
        return {amp, slope};
      }
      amp += slope * float(e.phase01 - prev);
      amp += e.dAmp;
      slope += e.dSlope;
      prev = e.phase01;
    }
    amp += slope * float(p - prev);
    return {amp, slope};
  }

  // todo: what is this?
  static inline double ForwardDistance01(double from, double to)
  {
    // distance along +phase direction on the unit circle
    return (to >= from) ? (to - from) : (1.0 - from + to);
  }

  template <class F>
  inline void VisitEdgesOnPath(double renderOffset01,
                               double phiStartRender01,
                               double pathDelta,
                               double alpha,
                               double deltaTotal,
                               F&& onEdge) const
  {
    if (pathDelta <= 0.0 || deltaTotal <= 0.0)
      return;

    for (const auto& e : mEdges)
    {
      const double eRender = math::wrap01(e.phase01 + renderOffset01);
      const double d = ForwardDistance01(phiStartRender01, eRender);
      if (d >= pathDelta)
        continue;                         // edge lies outside this sub-window
      double u = alpha + d / deltaTotal;  // whenInSample01 in [0,1)
      if (u >= 1.0)
        u = std::nextafter(1.0, 0.0);
      onEdge(u, e.dAmp, e.dSlope);
    }
  }
};


struct PolyBlepBlampExecutor
{
  // spill buffer...
  double mThisSampleCorrection = 0.0;
  double mNextSampleCorrection = 0.0;

  void OpenSample()
  {
    mThisSampleCorrection = mNextSampleCorrection;
    mNextSampleCorrection = 0.0;
  }

  void AccumulateEdge(double u, float dAmp, float dSlope)
  {
    // optimization: put these back in the if blocks. this is to reduce code size.
    const double u2 = u * u;
    const double u3 = u2 * u;
    if (dAmp != 0.0f)
    {
      // calculate polyBLEP contribution
      if (u < 1.0)
      {
        const double blep = (2.0 * u3 - 3.0 * u2 + 1.0);
        mThisSampleCorrection += dAmp * blep;
        mNextSampleCorrection += dAmp * (-(2.0 * u3) + 3.0 * u2);
      }
      else
      {
        // edge happens exactly at end of sample window; all contribution to next sample
        mNextSampleCorrection += dAmp * 1.0;
      }
    }
    if (dSlope != 0.0f)
    {
      // calculate polyBLAMP contribution
      if (u < 1.0)
      {
        const double blamp = (u3 - 1.5 * u2 + 0.5);
        mThisSampleCorrection += dSlope * blamp;
        mNextSampleCorrection += dSlope * (-u3 + 1.5 * u2);
      }
      else
      {
        // edge happens exactly at end of sample window; all contribution to next sample
        mNextSampleCorrection += dSlope * 0.5;
      }
    }
  }

  std::tuple<double, double> CloseSampleAndGetCorrection(double naiveAmplitude)
  {
    return {mThisSampleCorrection, naiveAmplitude + mThisSampleCorrection};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct BandLimitedOscillatorCore : public OscillatorCore
{
  PolyBlepBlampExecutor mHelper;

  virtual CumShape GetCumWaveDesc() = 0;

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseAdvance step = mPhaseAcc.advanceOneSample();
    const double delta = step.phaseDelta01PerSample;

    const CumShape cs = GetCumWaveDesc();

    mHelper.OpenSample();

    // Naive sample: evaluate at render-space begin-phase
    const double phi0Render = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const auto [amp0, /*slope0*/ _] = cs.EvalAmpSlopeAt(phi0Render);
    const float naive = amp0;

    auto addEdge = [&](double u, float dAmp, float dSlope)
    {
      mHelper.AccumulateEdge(u, dAmp, dSlope);
    };

    // Check for an in-sample hard-sync reset
    bool hasReset = false;
    double tReset = 0.0;
    for (int i = 0; i < step.eventCount; ++i)
    {
      if (step.events[i].kind == PhaseEventKind::Reset)
      {
        hasReset = true;
        tReset = step.events[i].whenInSample01;
        break;  // at most one reset per sample by design
      }
    }

    if (!hasReset)
    {
      // Single continuous path: start at phi0Render, advance by delta
      cs.VisitEdgesOnPath(audioRatePhaseOffset, phi0Render, delta, /*alpha*/ 0.0, delta, addEdge);
    }
    else
    {
      // 1) Pre-reset path: [0 .. tReset)
      const double path1 = tReset * delta;
      cs.VisitEdgesOnPath(audioRatePhaseOffset, phi0Render, path1, /*alpha*/ 0.0, delta, addEdge);

      // 2) Insert BLEP/BLAMP at the reset instant for the *teleport* to phase 0
      const double phiAtResetRender = math::wrap01(phi0Render + path1);  // just before reset
      const auto [ampBefore, slopeBefore] = cs.EvalAmpSlopeAt(phiAtResetRender);
      const auto [ampAfter, slopeAfter] = cs.EvalAmpSlopeAt(math::wrap01(audioRatePhaseOffset));  // 0+ with offset

      const float dAmp = ampAfter - ampBefore;
      const float dSlope = slopeAfter - slopeBefore;

      addEdge(tReset, dAmp, dSlope);

      // 3) Post-reset path: [tR .. 1)
      const double path2 = (1.0 - tReset) * delta;
      const double phiAfterResetRender = math::wrap01(audioRatePhaseOffset);  // start at 0+ (render space)
      cs.VisitEdgesOnPath(audioRatePhaseOffset, phiAfterResetRender, path2, /*alpha*/ tReset, delta, addEdge);
    }

    const auto [correction, amplitude] = mHelper.CloseSampleAndGetCorrection(naive);

    return CoreSample{.amplitude = (float)amplitude,
                      .naive = naive,
                      .correction = (float)correction,  //
                      .phaseAdvance = step};
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The idea here is to describe the waveform as a series of segments. The band limited parent class should
// be able to use this description to render the waveform with proper bandlimiting.
struct PWMCore : public BandLimitedOscillatorCore
{
  double mDutyCycle01 = 0.5f;  // 0..1

  void HandleParamsChanged() override
  {
    mDutyCycle01 = std::clamp(mWaveshapeA, 0.001f, 0.999f);
  }

  CumShape GetCumWaveDesc() override
  {
    CumShape cs{.mAmpAt0 = 1.0f};
    cs.mEdges.reserve(2);
    cs.mEdges.push_back(SegBoundary{.phase01 = 0.0, .dAmp = 0.0f, .dSlope = 0.0f});
    cs.mEdges.push_back(SegBoundary{.phase01 = mDutyCycle01, .dAmp = -2.0f, .dSlope = 0.0f});
    return cs;
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
