#pragma once

#include "../GigaSynth/Maj7Oscillator3Base.hpp"
#include "./Blep.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace M7Osc4
{

struct TrapezoidShapeParams
{
  float idleLow01 = 0.0f;
  float rampUp01 = 0.5f;
  float idleHigh01 = 0.0f;
  float rampDown01 = 0.5f;
};

static inline TrapezoidShapeParams MakeTrapezoidShapeParams(OscillatorWaveform waveform, float shapeA, float shapeB)
{
  switch (waveform)
  {
    case OscillatorWaveform::ShapeCoreSawTri:
    {
      const float idleLow01 = math::clamp((0.5f - shapeA) * 2.0f, 0.0f, 0.995f);
      const float idleHigh01 = math::clamp((shapeA - 0.5f) * 2.0f, 0.0f, 0.995f);
      const float remaining01 = 1.0f - idleLow01 - idleHigh01;
      return {
          idleLow01,
          shapeB * remaining01,
          idleHigh01,
          (1.0f - shapeB) * remaining01,
      };
    }
    case OscillatorWaveform::ShapeCoreSawPulse2:
    {
      const float duty01 = math::clamp(shapeA, 0.005f, 0.995f);
      return {
          duty01,
          0.0f,
          1.0f - duty01,
          0.0f,
      };
    }
    case OscillatorWaveform::ShapeCoreSawTriSquare:
    {
      const float idleLow01 = math::clamp01(shapeA);
      const float active01 = 1.0f - idleLow01;
      const float idleHigh01 = active01 * math::clamp01(shapeB);
      const float rampLen01 = (active01 > idleHigh01) ? (active01 - idleHigh01) * 0.5f : 0.0f;
      return {
          idleLow01,
          rampLen01,
          idleHigh01,
          rampLen01,
      };
    }
    default:
      return {};
  }
}

struct TrapezoidCoreNaive : public OscillatorCore
{
  TrapezoidShapeParams mShape;

  TrapezoidCoreNaive(OscillatorWaveform w)
      : OscillatorCore(w)
  {
  }

  void HandleParamsChanged() override
  {
    mShape = MakeTrapezoidShapeParams(mWaveformType, mWaveshapeA, mWaveshapeB);
  }

  float Evaluate(double phase01) const
  {
    const double wrappedPhase01 = math::wrap01(phase01);
    const double riseBegin01 = mShape.idleLow01;
    const double highBegin01 = riseBegin01 + mShape.rampUp01;
    const double fallBegin01 = highBegin01 + mShape.idleHigh01;

    if (wrappedPhase01 < riseBegin01)
    {
      return -1.0f;
    }
    if (wrappedPhase01 < highBegin01)
    {
      if (mShape.rampUp01 <= 0.0f)
      {
        return 1.0f;
      }
      return -1.0f + float(((wrappedPhase01 - riseBegin01) * 2.0) / mShape.rampUp01);
    }
    if (wrappedPhase01 < fallBegin01)
    {
      return 1.0f;
    }
    if (mShape.rampDown01 <= 0.0f)
    {
      return -1.0f;
    }
    return 1.0f - float(((wrappedPhase01 - fallBegin01) * 2.0) / mShape.rampDown01);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    CoreSample result;
    result.amplitude = Evaluate(step.phaseBegin01 + audioRatePhaseOffset);
    return result;
  }
};

struct TrapezoidCoreBandLimited : public OscillatorCore
{
  struct Segment
  {
    double beginPhase01 = 0.0;
    double endPhaseIncluding1 = 1.0;
    double beginAmp = 0.0;
    double slope = 0.0;

    DoublePair EvalAmpSlopeAtPhase(double phase01) const
    {
      return {beginAmp + slope * (phase01 - beginPhase01), slope};
    }
  };

  TrapezoidShapeParams mShape;
  Segment mSegments[4];
  size_t mSegmentCount = 0;
  CorrectionSpill mSpill;

  TrapezoidCoreBandLimited(OscillatorWaveform w)
      : OscillatorCore(w)
  {
  }

  void HandleParamsChanged() override
  {
    mShape = MakeTrapezoidShapeParams(mWaveformType, mWaveshapeA, mWaveshapeB);
    RebuildSegments();
  }

  void AppendSegment(double beginPhase01, double length01, double beginAmp, double slope)
  {
    if (length01 <= 0.0)
    {
      return;
    }

    mSegments[mSegmentCount++] = {beginPhase01, beginPhase01 + length01, beginAmp, slope};
  }

  void RebuildSegments()
  {
    mSegmentCount = 0;

    double phase01 = 0.0;
    AppendSegment(phase01, mShape.idleLow01, -1.0, 0.0);
    phase01 += mShape.idleLow01;

    if (mShape.rampUp01 > 0.0f)
    {
      AppendSegment(phase01, mShape.rampUp01, -1.0, 2.0 / mShape.rampUp01);
      phase01 += mShape.rampUp01;
    }

    AppendSegment(phase01, mShape.idleHigh01, 1.0, 0.0);
    phase01 += mShape.idleHigh01;

    if (mShape.rampDown01 > 0.0f)
    {
      AppendSegment(phase01, mShape.rampDown01, 1.0, -2.0 / mShape.rampDown01);
      phase01 += mShape.rampDown01;
    }

    if (mSegmentCount == 0)
    {
      mSegments[0] = {0.0, 1.0, 0.0, 0.0};
      mSegmentCount = 1;
      return;
    }

    mSegments[mSegmentCount - 1].endPhaseIncluding1 = 1.0;
  }

  size_t FindSegmentIndex(double phase01) const
  {
    for (size_t i = 0; i < mSegmentCount; ++i)
    {
      if (phase01 < mSegments[i].endPhaseIncluding1)
      {
        return i;
      }
    }

    return mSegmentCount - 1;
  }

  DoublePair EvalAmpSlopeAt(double phase01) const
  {
    if (mSegmentCount == 0)
    {
      return {};
    }

    const double wrappedPhase01 = math::wrap01(phase01);
    return mSegments[FindSegmentIndex(wrappedPhase01)].EvalAmpSlopeAtPhase(wrappedPhase01);
  }

  void VisitEdges(double phaseBegin01,
                  double dt,
                  double winLen,
                  double alphaOffset,
                  CorrectionSpill& spill) const
  {
    if (mSegmentCount == 0 || dt <= 0.0 || winLen <= 0.0)
    {
      return;
    }

    double consumed = 0.0;
    double currentPhase01 = math::wrap01(phaseBegin01);

    while (consumed < winLen)
    {
      const size_t segmentIndex = FindSegmentIndex(currentPhase01);
      const auto& segment = mSegments[segmentIndex];
      const double edgePhase01 = (segment.endPhaseIncluding1 >= 1.0) ? 1.0 : segment.endPhaseIncluding1;
      const double dPhaseToEdge = edgePhase01 - currentPhase01;
      const double alpha = dPhaseToEdge / dt;

      if (alpha < 0.0 || alpha > (winLen - consumed))
      {
        break;
      }

      const DoublePair preAmpSlope = segment.EvalAmpSlopeAtPhase(edgePhase01);
      const double postPhase01 = (edgePhase01 >= 1.0) ? 0.0 : edgePhase01;
      const auto& postSegment = mSegments[(segmentIndex + 1) % mSegmentCount];
      const DoublePair postAmpSlope = postSegment.EvalAmpSlopeAtPhase(postPhase01);

      spill.add_edge(alphaOffset + consumed + alpha, postAmpSlope - preAmpSlope, dt);

      consumed += alpha;
      currentPhase01 = postPhase01;
    }
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();
    const double dt = step.dt;
    const double evalPhase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);

    mSpill.open_sample();

    const auto ampSlope = EvalAmpSlopeAt(evalPhase01);
    const double naive = ampSlope[0];

    const double preLen = step.hasReset ? step.resetAlpha01 : 1.0;
    const double postLen = step.hasReset ? (1.0 - step.resetAlpha01) : 0.0;

    if (preLen > 0.0)
    {
      VisitEdges(evalPhase01, dt, preLen, 0.0, mSpill);
    }

    if (step.hasReset)
    {
      const double alpha = step.resetAlpha01;
      const double prePhase01 = evalPhase01 + alpha * dt;
      const double postPhase01 = math::wrap01(audioRatePhaseOffset);

      const auto preAmpSlope = EvalAmpSlopeAt(prePhase01);
      const auto postAmpSlope = EvalAmpSlopeAt(postPhase01);
      mSpill.add_edge(alpha, postAmpSlope - preAmpSlope, dt);

      if (postLen > 0.0)
      {
        VisitEdges(postPhase01, dt, postLen, alpha, mSpill);
      }
    }

    CoreSample result;
    result.amplitude = float(naive + mSpill.now);
    return result;
  }
};

using TrapezoidCore = TrapezoidCoreNaive;
//using TrapezoidCore = TrapezoidCoreBandLimited;

}  // namespace M7Osc4

}  // namespace M7

}  // namespace WaveSabreCore
