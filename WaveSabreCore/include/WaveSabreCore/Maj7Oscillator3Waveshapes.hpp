#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "BandSplitter.hpp"
#include "LinkwitzRileyFilter.hpp"
#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Shape.hpp"
#include "Maj7Oscillator4WS.hpp"
#include "SVFilter.hpp"

namespace WaveSabreCore
{
namespace M7
{
  static inline WVSegment SegmentFromEndpoints(double beginPhase01, double endPhaseIncluding1, float beginAmp, float endAmp)
  {
    double dPhase = endPhaseIncluding1 - beginPhase01;
    if (dPhase <= 0)
      dPhase += 1.0;  // wrap around

    float slope = (endAmp - beginAmp) / float(dPhase);
    return WVSegment{beginPhase01, endPhaseIncluding1, beginAmp, slope};
  }
// idleLow01 = amount of wave cycle spent holding at -1 at the beginning of the cycle (0..1)
// rampUp01 = amount of wave cycle spent ramping up from -1 to +1 (0..1)
// idleHigh01 = amount of wave cycle spent holding at +1
// rampDown01 = amount of wave cycle spent ramping down from +1 to -1 (0..1)
// the 4 stages are expected to add up to 1.
static inline WVShape MakeTrapezoidShape1(float idleLow01, float rampUp01, float idleHigh01, float rampDown01)
{ 
  return WVShape{.mSegments = {
                     SegmentFromEndpoints(0.0, idleLow01, -1.0f, -1.0f),
                     SegmentFromEndpoints(idleLow01, idleLow01 + rampUp01, -1.0f, +1.0f),
                     SegmentFromEndpoints(idleLow01 + rampUp01, idleLow01 + rampUp01 + idleHigh01, +1.0f, +1.0f),
                     SegmentFromEndpoints(idleLow01 + rampUp01 + idleHigh01, 1.0, +1.0f, -1.0f),
                 }};
}

// accepts 2 shape 0..1 parameters and maps them to the 4 raw params.
static inline WVShape MakeSawShape(float idleParam01, float rampUpDownParam01)
{
  // when idleParam01 = 0.5 (center), then there should be no idle.
  // idleParam < 0.5 -> more idle at low level; idleParam > 0.5 -> more idle at high level
  // idleParam = 0 -> idleLow01 = 1, idleHigh01 = 0; idleParam = 1 -> idleLow01 = 0, idleHigh01 = 1
  float idleLow01 = std::clamp((0.5f - idleParam01) * 2, 0.0f, 0.995f); // allow 0 duration
  float idleHigh01 = std::clamp((idleParam01 - 0.5f) * 2, 0.0f, 0.995f); // allow 0 duration

  float remaining = 1.0f - idleLow01 - idleHigh01; // remaining cycle portion for the ramps

  // rampUpDownParam01 = saw -> tri map. 0 = saw (full stage = ramp up); 1 = triangle (50% ramp up, 50% ramp down)
  float rampUp01 = rampUpDownParam01 * remaining;
  float rampDown01 = (1.0f - rampUpDownParam01) * remaining;
  return MakeTrapezoidShape1(idleLow01, rampUp01, idleHigh01, rampDown01);
}

static inline WVShape MakeTriangleShape()
{
  return MakeTrapezoidShape1(0.0f, 0.5f, 0.0f, 0.5f);
}
static inline WVShape MakePulseShape(double dutyCycle01)
{
  dutyCycle01 = std::clamp(dutyCycle01, 0.005, 0.995);
  return MakeTrapezoidShape1(dutyCycle01, 0.0f, 1.0f - dutyCycle01, 0.0f);
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

  return WVShape{
      .mSegments = {
          WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
          WVSegment{
              .beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 1.0f, .slope = 0},
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
  auto mainSeg2 = 1.0 - masterDutyCycle01;  // length of the high segment area
  auto highDutyPhase = std::clamp(highDuty01 * mainSeg2, 0.0, mainSeg2 - 0.001);

  return WVShape{
      .mSegments = {
          WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
          WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 0.f, .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01,
                    .endPhaseIncluding1 = masterDutyCycle01 + highDutyPhase,
                    .beginAmp = 1.0f,
                    .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01 + highDutyPhase,
                    .endPhaseIncluding1 = 1.0,
                    .beginAmp = .0f,
                    .slope = 0},
      }};
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SilenceOsc : public OscillatorCore
{
  SilenceOsc(OscillatorWaveform waveform)
      : OscillatorCore(waveform)
  {
  }

  void HandleParamsChanged() override
  {
  };

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    return CoreSample{
        .amplitude = 0,
        .phaseAdvance = step,
    };
  }
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// you can use 2 params at a time, but this supports 4.
// - DC asymmetry
// - clipping
// - silence fraction
// - harmonic blend (0..1) between pure sine and windowed sine (1.5*sin*(0.5-0.5*cos))
enum class SineCoreExtVariant
{
  DCClip,
  ClipSilence,
  ClipHarm,
  HarmSilence,
};
template <SineCoreExtVariant Variant>
struct SineCoreExt : public OscillatorCore
{
  // A: dc asym (-1..+1), B: clip (0..1), C: silence frac (0..1), H: harmonic blend (0..1)
  float mA = 0.0f, mB = 0.0f, mC = 0.0f, mH = 0.0f;

  // Derived for clipping
  float mClipThreshold = 1.0f;
  float mClipGain = 1.0f;
  bool mSquareMode = false;
  static constexpr float kSquareEps = 1e-4f;

  SineCoreExt(OscillatorWaveform wf)
      : OscillatorCore(wf)
  {
  }

  void HandleParamsChanged() override
  {
    mA = 0;
    mB = 0;
    mC = 0;
    mH = 0;
    switch (Variant)
    {
      case SineCoreExtVariant::DCClip:
      {
        mA = math::clampN11(mWaveshapeB * 2 - 1);
        mB = math::clamp01(mWaveshapeA);
        break;
      }
      case SineCoreExtVariant::ClipSilence:
      {
        mB = std::abs(mWaveshapeA);
        mC = std::abs(mWaveshapeB);
        break;
      }
      case SineCoreExtVariant::ClipHarm:
      {
        mB = std::abs(mWaveshapeA);
        mH = math::clamp01(mWaveshapeB);
        break;
      }
      case SineCoreExtVariant::HarmSilence:
      {
        mC = math::clamp01(mWaveshapeA);
        mH = math::clamp01(mWaveshapeB);
        break;
      }
    }

    if (mB >= 1.0f - kSquareEps)
    {
      mSquareMode = true;
      mClipThreshold = 1.0f;
      mClipGain = 1.0f;
    }
    else
    {
      mSquareMode = false;
      mClipThreshold = std::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;
    }
  }

  inline float ClipAndNormalize(float x) const
  {
    if (mSquareMode)
      return (x > 0.0f) ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    const float y = std::clamp(x, -mClipThreshold, +mClipThreshold);
    return y * mClipGain;
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample();  // no offset inside accumulator
    const double p = step.phaseBegin01;
    const double off = audioRatePhaseOffset;  // offset is in cycles [0,1) here
    const double actW = 1.0 - (double)mC;     // active window width

    float out = 0.0f;

    if (mC < 1.0f && p < actW)
    {
      // Compress the active part to [0,1), then apply offset and get angle
      const double phi = std::fmod((p / actW) + off, 1.0);
      const float theta = float(phi * math::gPITimes2);

      // Base sine
      float s = std::sin(theta);

      // Harmonic-blended variant:
      //   y_h = 1.5 * s * (0.5 - 0.5*cos(theta))   (your original)
      // which equals 0.75*sin(theta) - 0.375*sin(2*theta)
      float yH;
      {
        // (A) window form (needs cos)
        //float c = std::cos(theta);
        //yH = 1.5f * s * (0.5f - 0.5f * c);

        // // (B) identity form (needs sin 2θ)
        float s2 = std::sin(2.0f * theta);
        yH = 0.75f * s - 0.375f * s2;
      }

      // Blend: H=0 → pure sine, H=1 → windowed/harmonic version
      float yPreClip = s * (1.0f - mH) + yH * mH;

      // DC asymmetry BEFORE clipping (biases clip for asymmetrical distortion)
      yPreClip += mA;

      // Hard clip with makeup gain (or square if B≈1)
      out = ClipAndNormalize(yPreClip);
    }
    // else: in silence tail or full-cycle silence → out stays 0

    return CoreSample{
        .amplitude = out,
        .naive = out,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};


struct FoldedSine : public OscillatorCore
{
  FoldedSine(bool triangle, OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
      , mTriangle(triangle)
  {
  }

  float mDrive = 1.0f;  // >= 1.0
  float mBias = 0.0f;   // DC offset before folding
  const float mTriangle;

  void HandleParamsChanged() override
  {
    mDrive = 1.0f + 10.0f * mWaveshapeA;

    // B: symmetry/bias 0..1 -> -1..1; actually in theory -2..2 is usable to catch ALL signal.
    mBias = (mWaveshapeB - 0.5f) * 4.0f;
  }

  static inline float fold_reflect(float x)
  {
    const float threshold = 1;
    const float ax = std::fabs(x);
    const float per = 2 * threshold;
    float m = std::fmod(ax, per);
    if (m < 0.0f)
      m += per;

    const float r = (m <= threshold) ? m : (per - m);
    return std::copysign(r, x);
  }

  inline float sampleWaveform(float phase01)
  {
    if (mTriangle)
    {
      return math::naiveTriangle01(phase01);
    }
    // sine
    return math::sin(math::gPITimes2 * phase01);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    const double phase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const float s = sampleWaveform(phase01);  // math::sin(math::gPITimes2 * float(phase01));

    const float driven = mDrive * s + mBias;
    float y = fold_reflect(driven);

    return CoreSample{
        .amplitude = y,
        .phaseAdvance = step,
    };
  }
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct SAHNoiseCore : public OscillatorCore
{
  // Param B: Jitter amount in [0,1]
  float mJitter01 = 0.0f;

  // Jitter range in phase units (fraction of a cycle)
  static constexpr double kJitterMaxPhase = 0.45;  // ±45% of a cycle at B=1

  // Internal clock phasor and current cycle length (in phase units)
  double mClockPhase01 = 0.0;    // advances by dt each sample
  double mCycleThreshold = 1.0;  // = 1.0 + jitter per cycle

  // Output state: held value between steps
  float mHeld = 0.0f;

  // BLEP spill
  M7Osc4::CorrectionSpill mSpill;

  SAHNoiseCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  void HandleParamsChanged() override
  {
    // Map your UI param: A unused (slew removed), B = jitter 0..1
    mJitter01 = std::clamp(mWaveshapeB, 0.0f, 1.0f);
  }


  // Only sets the next cycle length (no target here).
  inline void schedule_next_cycle_length()
  {
    const double J = (double)mJitter01 * kJitterMaxPhase;
    const double jitter = J * math::randN11();  // [-J, +J]
    mCycleThreshold = 1.0 + jitter;
  }


  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    const double dt = step.dt;

    // First-time init
    if (mClockPhase01 == 0.0 && mCycleThreshold == 1.0)
    {
      schedule_next_cycle_length();
      mHeld = math::randN11();  // start value
    }

    // 1) Open BLEP spill for THIS sample
    mSpill.open_sample();

    // 2) SNAPSHOT the held value at the **start** of the sample
    const float heldAtStart = mHeld;

    // 3) March through possible intra-sample events
    double remaining = dt;
    double consumed = 0.0;

    while (true)
    {
      const double toEdge = mCycleThreshold - mClockPhase01;

      if (remaining < toEdge)
      {
        mClockPhase01 += remaining;
        break;
      }

      // Edge happens inside this sample
      const double alphaFull = (dt > 0.0) ? std::clamp((consumed + toEdge) / dt, 0.0, 1.0) : 0.0;

      // Compute new target and true step size from the **current** held value
      const float newTarget = math::randN11();
      const double dAmp = (double)newTarget - (double)mHeld;

      // Band-limit the step at alphaFull
      //add_blep(alphaFull, dAmp, mSpill.now, mSpill.next);
      mSpill.add_edge(alphaFull, dAmp, 0, dt);

      // Commit the step to state (affects later edges in THIS sample and the next sample)
      mHeld = newTarget;

      // Consume up to the edge, reset phasor, schedule next cycle length
      remaining -= toEdge;
      consumed += toEdge;
      mClockPhase01 = 0.0;
      schedule_next_cycle_length();
    }

    // 4) Output = value at sample **start** + now-tap
    const float y = heldAtStart + (float)mSpill.now;

    return CoreSample{
        .amplitude = y,
        .naive = heldAtStart,  // optional: expose pre-correction
        .correction = (float)mSpill.now,
        .phaseAdvance = step,
    };
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
