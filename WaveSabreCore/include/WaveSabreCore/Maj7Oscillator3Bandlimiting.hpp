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
struct PhaseUnits
{
  double mValue;
};
struct SampleUnits
{
  double mValue;
};

// line-based waveform segment boundary description
struct SegBoundary
{
  double phase01;
  float dAmp;
  float dSlope;
};

// line-based waveform shape description expressed in deltas
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

  // phase distance from "from" to "to", wrapping around 1.0.
  // IOW, how far do you have to go forward from "from" to reach "to";
  // this can never be more than 1.0.
  static inline double ForwardDistance01(double from, double to)
  {
    return (to >= from) ? (to - from) : (1.0 - from + to);
  }

  // Visit all edges that lie on the path starting at phiStartRender01, advancing by pathDelta.
  template <class F>
  inline void VisitEdgesOnPath(double renderOffset01,    // a-rate phase offset, applied to all edges
                               double phiStartRender01,  // start of the render window, in phase units [0,1)
                               double pathDelta,         // distance of the path to traverse, in phase units [0,1)
                               double alpha,             // whenInSample01 at start of path [0,1)
                               double deltaTotal,        // total phase delta of the sample [0,1)
                               F&& onEdge                // void(double whenInSample01, float dAmp, float dSlope)
  ) const
  {
    if (pathDelta <= 0.0 || deltaTotal <= 0.0)
      return;

    for (const auto& e : mEdges)
    {
      const double eRender = e.phase01;  // + renderOffset01; wrap not needed; offset is baked into phiStartRender01
      // distance from start of render window to edge, in [0,1)
      const double d = ForwardDistance01(phiStartRender01, eRender);
      if (d >= pathDelta)
        continue;  // edge lies outside the specified window

      double u = alpha + d / deltaTotal;  // whenInSample01 in [0,1)
      if (u >= 1.0)
        u = std::nextafter(1.0, 0.0);  // avoid hitting exactly 1.0; this should maybe never happen?
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

  // u is 0-1 samples after the discontinuity.
  void AccumulateEdge(double edgePosInSampleInPhase01, float dAmp, float dSlope)
  {
      // apply corrections...
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
    const CumShape cs = GetCumWaveDesc();
    const auto sampleWindowSizeInPhase01 = step.lengthInPhase01;

    mHelper.OpenSample();

    // Naive sample: evaluate at render-space begin-phase
    const double sampleStartInPhase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const auto [amp0, /*slope0*/ _] = cs.EvalAmpSlopeAt(sampleStartInPhase01);
    const float naive = amp0;

    auto addEdge = [&](double u, float dAmp, float dSlope)
    {
      // convert u (in [0,1) sample units) to edgePosInSampleInPhase01 (in [0,1) phase units)
      double edgePosInSampleInPhase01 = u * sampleWindowSizeInPhase01;
      mHelper.AccumulateEdge(edgePosInSampleInPhase01, dAmp, dSlope);
    };

    // Check for an in-sample hard-sync reset
    bool hasReset = false;
    double resetPosInSample01 = 0.0;  // when in this sample a reset happens [0,1)
    for (int i = 0; i < step.eventCount; ++i)
    {
      if (step.events[i].kind == PhaseEventKind::Reset)
      {
        hasReset = true;
        resetPosInSample01 = step.events[i].whenInSample01;
        break;  // at most one reset per sample by design
      }
    }

    if (!hasReset)
    {
      // Single continuous path: start at phi0Render, advance by delta
      cs.VisitEdgesOnPath(audioRatePhaseOffset,
                          sampleStartInPhase01,       // path begin
                          sampleWindowSizeInPhase01,  // path length
                          0.0,                        // whenInSample01 at start of path = beginning of sample
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);
    }
    else
    {
      // 1) Pre-reset path: [0 .. tReset)
      const double preResetLengthInPhase01 = resetPosInSample01 *
                                             sampleWindowSizeInPhase01;  // distance from start to reset, in phase units
      cs.VisitEdgesOnPath(audioRatePhaseOffset,
                          sampleStartInPhase01,       // path begin
                          preResetLengthInPhase01,    // path length = distance from start to reset
                          0.0,                        // whenInSample01 at start of path = beginning of sample
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);

      // 2) Insert BLEP/BLAMP at the reset instant for the *teleport* to phase 0
      const double resetPosInPhase01 = math::wrap01(sampleStartInPhase01 +
                                                    preResetLengthInPhase01);  // just before reset
      const auto [ampBefore, slopeBefore] = cs.EvalAmpSlopeAt(resetPosInPhase01);

      // after reset, we're at phase 0 + phase offset.
      const auto [ampAfter, slopeAfter] = cs.EvalAmpSlopeAt(math::wrap01(audioRatePhaseOffset));  // 0+ with offset

      const float dAmp = ampAfter - ampBefore;
      const float dSlope = slopeAfter - slopeBefore;

      addEdge(resetPosInSample01, dAmp, dSlope);

      // 3) Post-reset path: [tR .. 1)
      const double postResetLengthInPhase01 = (1.0 - resetPosInSample01) * sampleWindowSizeInPhase01;
      const double phiAfterResetRender = math::wrap01(audioRatePhaseOffset);  // start at 0+ (render space)
      cs.VisitEdgesOnPath(audioRatePhaseOffset,
                          phiAfterResetRender,        // path begin = 0+ with offset
                          postResetLengthInPhase01,   // path length = distance from reset to end of sample
                          resetPosInSample01,         // whenInSample01 at start of path = reset time
                          sampleWindowSizeInPhase01,  // total delta = full sample
                          addEdge);
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
