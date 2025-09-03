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

//struct SineCore : public OscillatorCore
//{
//  // sine wave core, with shape params (outputs [-1..+1])
//  // shape param 1: dc asymmetry (-1 to +1) -- the point is to shift the waveform so clipping is asymmetrical
//  // shape param 2: clip amount (0-1). clipping the sine also will amplify it to normalize. clip 0 = pure sine, clip 0.99 = almost square (and 1.00 is undefined but we will output a square)
//  // knob2: silence at end of cycle (0-1)
//  // shape param 3: 0-1, squeezes the sinewave horizontally towards the left, adding silence at the end of the cycle. 0 = normal sine, 0.2 = adds .2 cycles of silence at the end, and the sine wave is squeezed left to accommodate. 1 = silence.
//  SineCore()
//      : OscillatorCore(OscillatorWaveform::Sine)
//  {
//  }
//
//  void HandleParamsChanged() override
//  {
//    // k-rate, precompute from mWaveshapeA/B/C
//  }
//
//  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//  {
//    const auto step = mPhaseAcc.advanceOneSample();
//    const double phase = step.phaseBegin01;
//    static constexpr double pi2 = 2 * math::gPI;
//
//    // basic sine
//    float y = math::sin(math::gPITimes2 * (float)phase + audioRatePhaseOffset);
//
//    return {
//        .amplitude = y,
//    };
//  }
//};
//

template <bool Variant>
struct SineCore : public OscillatorCore
{
  // Shape params (k-rate):
  // A: dc asymmetry (-1..+1)
  // B: clip amount  (0..1)   0 = pure sine,  ~1 = almost square,  >=1 => square
  // C: silence frac (0..1)   0 = none,       0.2 => last 20% of cycle is 0,   1 => all silence
  float mA = 0.0f;
  float mB = 0.0f;
  float mC = 0.0f;

  // Derived (k-rate)
  float mClipThreshold = 1.0f;  // T in (0,1], symmetric ±T
  float mClipGain = 1.0f;       // makeup gain = 1/T
  bool mSquareMode = false;     // exact square when true
  static constexpr float kSquareEps = 1e-4f;

  SineCore(OscillatorWaveform wf)
      : OscillatorCore(wf)
  {
  }

  // Call this whenever mWaveshapeA/B/C (host knobs) change
  void HandleParamsChanged() override
  {
    // Pull from your host/engine’s param slots as needed:
    if constexpr (Variant)
    {
      // dc-clip
      mA = std::clamp(mWaveshapeA, -1.0f, 1.0f);  // dc asymmetry
      mB = std::abs(mWaveshapeB);                 // clip amount 0-1; treat -1 to 0 as 1 to 0
      mC = 0;                                     //std::clamp(mWaveshapeC, 0.0f, 1.0f);       // silence fraction
    }
    else
    {
      // clip-silence
      mA = 0;                      //std::clamp(mWaveshapeA, -1.0f, 1.0f);      // dc asymmetry
      mB = std::abs(mWaveshapeA);  // clip amount
      mC = std::abs(mWaveshapeB);  // silence fraction
    }

    // Map clip amount -> threshold:
    // B=0 => T=1 (no clip).  As B→1, T→(small).  B>=1 => square mode.
    if (mB >= 1.0f - kSquareEps)
    {
      mSquareMode = true;
      mClipThreshold = 1.0f;  // unused
      mClipGain = 1.0f;       // unused
    }
    else
    {
      mSquareMode = false;
      mClipThreshold = std::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;  // normalize back to ±1
    }
  }

  // Hard clip with makeup gain. Square mode if enabled.
  inline float ClipAndNormalize(float x) const
  {
    if (mSquareMode)
    {
      // exact square; keep 0 at exactly zero to avoid denorm noise toggling
      return (x > 0.0f) ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    }
    const float y = std::clamp(x, -mClipThreshold, +mClipThreshold);
    return y * mClipGain;
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    // Accumulator does NOT see offset; we apply offset only for evaluation.
    const auto step = mPhaseAcc.advanceOneSample();
    double phaseNoOff = step.phaseBegin01;

    // Apply silence window (C): compress phase to [0, 1-C], silence on the tail
    float y = 0.0f;
    if (mC < 1.0f)
    {
      const double activeFrac = 1.0 - (double)mC;  // width of active part
      const double p = phaseNoOff;
      if (p < activeFrac)
      {
        // Compress horizontally into [0,1)
        const double evalPhase = math::wrap01((p / activeFrac) + (double)audioRatePhaseOffset);
        const float angle = float(evalPhase) * math::gPITimes2;
        // Base sine
        float s = std::sin(angle);
        // DC asymmetry (pre-clip)
        s += mA;
        // Clip + makeup gain to normalize peak back to ±1
        y = ClipAndNormalize(s);
      }
      else
      {
        // In the silence tail: exactly zero
        y = 0.0f;
      }
    }
    else
    {
      // Full-cycle silence
      y = 0.0f;
    }

    return CoreSample{
        .amplitude = y,
        .naive = y,  // no bandlimiting here
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};

template <bool Variant>
struct SineHarmCore : public OscillatorCore
{
  // Shape params (k-rate):
  // A: dc asymmetry (-1..+1)
  // B: clip amount  (0..1)   0 = pure sine,  ~1 = almost square,  >=1 => square
  // C: silence frac (0..1)   0 = none,       0.2 => last 20% of cycle is 0,   1 => all silence
  float mA = 0.0f;
  float mB = 0.0f;
  float mC = 0.0f;

  // Derived (k-rate)
  float mClipThreshold = 1.0f;  // T in (0,1], symmetric ±T
  float mClipGain = 1.0f;       // makeup gain = 1/T
  bool mSquareMode = false;     // exact square when true
  static constexpr float kSquareEps = 1e-4f;

  SineHarmCore(OscillatorWaveform wf)
      : OscillatorCore(wf)
  {
  }

  // Call this whenever mWaveshapeA/B/C (host knobs) change
  void HandleParamsChanged() override
  {
    // Pull from your host/engine’s param slots as needed:
    if constexpr (Variant)
    {
      // dc-clip
      mA = std::clamp(mWaveshapeA, -1.0f, 1.0f);  // dc asymmetry
      mB = std::abs(mWaveshapeB);                 // clip amount 0-1; treat -1 to 0 as 1 to 0
      mC = 0;                                     //std::clamp(mWaveshapeC, 0.0f, 1.0f);       // silence fraction
    }
    else
    {
      // clip-silence
      mA = 0;                      //std::clamp(mWaveshapeA, -1.0f, 1.0f);      // dc asymmetry
      mB = std::abs(mWaveshapeA);  // clip amount
      mC = std::abs(mWaveshapeB);  // silence fraction
    }

    // Map clip amount -> threshold:
    // B=0 => T=1 (no clip).  As B→1, T→(small).  B>=1 => square mode.
    if (mB >= 1.0f - kSquareEps)
    {
      mSquareMode = true;
      mClipThreshold = 1.0f;  // unused
      mClipGain = 1.0f;       // unused
    }
    else
    {
      mSquareMode = false;
      mClipThreshold = std::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;  // normalize back to ±1
    }
  }

  // Hard clip with makeup gain. Square mode if enabled.
  inline float ClipAndNormalize(float x) const
  {
    if (mSquareMode)
    {
      // exact square; keep 0 at exactly zero to avoid denorm noise toggling
      return (x > 0.0f) ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    }
    const float y = std::clamp(x, -mClipThreshold, +mClipThreshold);
    return y * mClipGain;
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    // Accumulator does NOT see offset; we apply offset only for evaluation.
    const auto step = mPhaseAcc.advanceOneSample();
    double phaseNoOff = step.phaseBegin01;

    // Apply silence window (C): compress phase to [0, 1-C], silence on the tail
    float y = 0.0f;
    if (mC < 1.0f)
    {
      const double activeFrac = 1.0 - (double)mC;  // width of active part
      const double p = phaseNoOff;
      if (p < activeFrac)
      {
        // Compress horizontally into [0,1)
        const double evalPhase = math::wrap01((p / activeFrac) + (double)audioRatePhaseOffset);
        const float angle = float(evalPhase) * math::gPITimes2;
        // Base sine

        float s = math::sin(angle);
        float b = .5f - math::cos(angle) * .5f;
        s = s * b * 1.5f;

        // DC asymmetry (pre-clip)
        s += mA;
        // Clip + makeup gain to normalize peak back to ±1
        y = ClipAndNormalize(s);
      }
      else
      {
        // In the silence tail: exactly zero
        y = 0.0f;
      }
    }
    else
    {
      // Full-cycle silence
      y = 0.0f;
    }

    return CoreSample{
        .amplitude = y,
        .naive = y,  // no bandlimiting here
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};

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
    switch (Variant)
    {
      case SineCoreExtVariant::DCClip:
      {
        mA = std::clamp(mWaveshapeA, -1.0f, 1.0f);
        mB = std::abs(mWaveshapeB);
        mC = 0;
        mH = 0;
        break;
      }
      case SineCoreExtVariant::ClipSilence:
      {
        mA = 0;
        mB = std::abs(mWaveshapeA);
        mC = std::abs(mWaveshapeB);
        mH = 0;
        break;
      }
      case SineCoreExtVariant::ClipHarm:
      {
        mA = 0;
        mB = std::abs(mWaveshapeA);
        mC = 0;
        mH = std::clamp(mWaveshapeB, 0.0f, 1.0f);
        break;
      }
      case SineCoreExtVariant::HarmSilence:
      {
        mA = 0;
        mB = 0;
        mC = std::clamp(mWaveshapeA, 0.0f, 1.0f);
        mH = std::clamp(mWaveshapeB, 0.0f, 1.0f);
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
        // choose whichever is cheaper on your platform:

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


}  // namespace M7

}  // namespace WaveSabreCore
