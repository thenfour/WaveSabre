#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "BandSplitter.hpp"
#include "BiquadFilter.h"
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
static inline WVSegment SegmentFromEndpoints(double beginPhase01,
                                             double endPhaseIncluding1,
                                             float beginAmp,
                                             float endAmp)
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
  float idleLow01 = math::clamp((0.5f - idleParam01) * 2, 0.0f, 0.995f);   // allow 0 duration
  float idleHigh01 = math::clamp((idleParam01 - 0.5f) * 2, 0.0f, 0.995f);  // allow 0 duration

  float remaining = 1.0f - idleLow01 - idleHigh01;  // remaining cycle portion for the ramps

  // rampUpDownParam01 = saw -> tri map. 0 = saw (full stage = ramp up); 1 = triangle (50% ramp up, 50% ramp down)
  float rampUp01 = rampUpDownParam01 * remaining;
  float rampDown01 = (1.0f - rampUpDownParam01) * remaining;
  return MakeTrapezoidShape1(idleLow01, rampUp01, idleHigh01, rampDown01);
}

// duty controls the balance between idle & tri/square shape.
// - at duty 0 = , the whole cycle is the tri-square.
// - at duty 1, the whole cycle is idle.
// tri-square shape controls idle time at high state.
// - when shape=0, the shape portion is a triangle (ramp up - ramp down with no idle at high state).
// - when shape=1, it's a square; the whole shape is idle at high state.
static inline WVShape MakeTriSquareShape(float duty01, float triSquare01)
{
  duty01 = math::clamp01(duty01);
  triSquare01 = math::clamp01(triSquare01);

  // duty controls how much of the cycle is idle at low level.
  const float idleLow01 = duty01;
  const float active01 = 1.0f - idleLow01;

  // triSquare controls how much of the active portion is held high:
  // 0 -> triangle (no high hold), 1 -> square (all high hold).
  const float idleHigh01 = active01 * triSquare01;
  const float ramps01Raw = active01 - idleHigh01;
  const float ramps01 = (ramps01Raw > 0.0f) ? ramps01Raw : 0.0f;
  const float rampUp01 = ramps01 * 0.5f;
  const float rampDown01 = ramps01 * 0.5f;

  return MakeTrapezoidShape1(idleLow01, rampUp01, idleHigh01, rampDown01);
}

// Folded triangle built as piecewise-linear WVShape so ShapeCoreStreaming can apply BLEP/BLAMP.
// shapeA controls fold drive (1..16), shapeB controls bias (-2..+2).
static inline WVShape MakeFoldedTriangleShape(float drive, float bias)
{
  drive = (drive < 1.0f) ? 1.0f : drive;

  auto fold_reflect = [](float x)
  {
    const float threshold = 1.0f;
    const float period = threshold * 2.0f;
    const float ax = std::abs(x);
    float m = math::fmod(ax, period);
    if (m < 0.0f)
      m += period;
    const float r = (m <= threshold) ? m : (period - m);
    return std::copysign(r, x);
  };

  struct TriBaseSeg
  {
    double p0 = 0.0;
    double p1 = 0.0;
    float slope = 0.0f;
    float intercept = 0.0f;
  };

  // Tri over [0,1):
  // [0,0.25): y=4p
  // [0.25,0.75): y=2-4p
  // [0.75,1): y=4p-4
  const TriBaseSeg baseSegs[] = {
      {0.0, 0.25, 4.0f, 0.0f},
      {0.25, 0.75, -4.0f, 2.0f},
      {0.75, 1.0, 4.0f, -4.0f},
  };

  std::vector<double> breakpoints;
  breakpoints.reserve(64);
  breakpoints.push_back(0.0);
  breakpoints.push_back(0.25);
  breakpoints.push_back(0.75);
  breakpoints.push_back(1.0);

  static constexpr double eps = 1e-8;

  for (const auto& seg : baseSegs)
  {
    const float xSlope = drive * seg.slope;
    const float xIntercept = drive * seg.intercept + bias;

    const float x0 = xSlope * float(seg.p0) + xIntercept;
    const float x1 = xSlope * float(seg.p1) + xIntercept;
    const float xmin = (x0 < x1) ? x0 : x1;
    const float xmax = (x0 > x1) ? x0 : x1;

    const int nMin = (int)std::floor(xmin);
    const int nMax = (int)std::ceil(xmax);
    for (int n = nMin; n <= nMax; ++n)
    {
      const double p = (double(n) - double(xIntercept)) / double(xSlope);
      if (p > seg.p0 + eps && p < seg.p1 - eps)
      {
        breakpoints.push_back(p);
      }
    }
  }

  std::sort(breakpoints.begin(), breakpoints.end());
  breakpoints.erase(std::unique(breakpoints.begin(), breakpoints.end(),
                                [](double a, double b) { return std::abs(a - b) <= 1e-7; }),
                    breakpoints.end());

  WVShape shape;
  shape.mSegments.reserve(breakpoints.size());

  for (size_t i = 0; i + 1 < breakpoints.size(); ++i)
  {
    const double p0 = breakpoints[i];
    const double p1 = breakpoints[i + 1];
    const double dp = p1 - p0;
    if (dp <= eps)
      continue;

    float tri0 = 0.0f;
    float tri1 = 0.0f;
    if (p0 < 0.25)
      tri0 = float(4.0 * p0);
    else if (p0 < 0.75)
      tri0 = float(2.0 - 4.0 * p0);
    else
      tri0 = float(4.0 * p0 - 4.0);

    if (p1 <= 0.25)
      tri1 = float(4.0 * p1);
    else if (p1 <= 0.75)
      tri1 = float(2.0 - 4.0 * p1);
    else
      tri1 = float(4.0 * p1 - 4.0);

    const float y0 = fold_reflect(drive * tri0 + bias);
    const float y1 = fold_reflect(drive * tri1 + bias);
    const float slope = (y1 - y0) / float(dp);

    shape.mSegments.push_back(WVSegment{p0, p1, y0, slope});
  }

  if (shape.mSegments.empty())
  {
    shape.mSegments.push_back(WVSegment{0.0, 1.0, 0.0f, 0.0f});
  }

  return shape;
}
static inline WVShape MakePulseShape(double dutyCycle01)
{
  dutyCycle01 = math::clamp(dutyCycle01, 0.005, 0.995);
  return MakeTrapezoidShape1(float(dutyCycle01), 0.0f, 1.0f - float(dutyCycle01), 0.0f);
}

// three state pulse: low, high, 0
// segment 0: low
// segment 1: high
// segment 2: 0
static inline WVShape MakeTriStatePulseShape3(double masterDutyCycle01, double subDuty01)
{
  masterDutyCycle01 = math::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = math::clamp(subDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
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
  masterDutyCycle01 = math::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = math::clamp(lowDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  // scale high duty to the master duty cycle
  auto mainSeg2 = 1.0 - masterDutyCycle01;  // length of the high segment area
  auto highDutyPhase = math::clamp(highDuty01 * mainSeg2, 0.0, mainSeg2 - 0.001);

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

  void HandleParamsChanged() override {};

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
//template <SineCoreExtVariant Variant>
struct SineCoreExt : public OscillatorCore
{
  // A: dc asym (-1..+1), B: clip (0..1), C: silence frac (0..1), H: harmonic blend (0..1)
  float mA = 0.0f;
  float mB = 0.0f;
  float mC = 0.0f;
  float mH = 0.0f;

  // Derived for clipping
  float mClipThreshold = 1.0f;
  float mClipGain = 1.0f;
  bool mSquareMode = false;
  static constexpr float kSquareEps = 1e-4f;
  const SineCoreExtVariant mVariant;

  SineCoreExt(OscillatorWaveform wf, SineCoreExtVariant variant)
      : OscillatorCore(wf)
      , mVariant(variant)
  {
  }

  void HandleParamsChanged() override
  {
    mA = 0;
    mB = 0;
    mC = 0;
    mH = 0;
    switch (mVariant)
    {
      case SineCoreExtVariant::DCClip:
      {
        mA = mWaveshapeB * 2 - 1;
        mB = mWaveshapeA;
        break;
      }
      case SineCoreExtVariant::ClipSilence:
      {
        mB = mWaveshapeA;
        mC = mWaveshapeB;
        break;
      }
      case SineCoreExtVariant::ClipHarm:
      {
        mB = mWaveshapeA;
        mH = mWaveshapeB;
        break;
      }
      case SineCoreExtVariant::HarmSilence:
      {
        mC = mWaveshapeA;
        mH = mWaveshapeB;
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
      mClipThreshold = math::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;
    }
  }

  inline float ClipAndNormalize(float x) const
  {
    if (mSquareMode)
      return (x > 0.0f) ? 1.0f : (x < 0.0f ? -1.0f : 0.0f);
    const float y = math::clamp(x, -mClipThreshold, +mClipThreshold);
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
      const double phi = math::fmodd((p / actW) + off, 1.0);
      const float theta = float(phi * math::gPITimes2);

      // Base sine
      float s = math::sin(theta);

      float s2 = math::sin(2.0f * theta);
      float yH = 0.75f * s - 0.375f * s2;

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


// struct FoldedCore : public OscillatorCore
// {
//   enum class Style
//   {
//     Sine_Fold_Bias,  // A = fold amount; B = bias
//     Tri_Fold_Bias,   // A = fold amount; B = bias
//     //TriSaw_Fold_Shape,  // A = fold amount; B = shape (tri-saw)
//   };

//   FoldedCore(OscillatorWaveform waveformType, Style style)
//       : OscillatorCore(waveformType)
//       , mStyle(style)
//   {
//   }

//   float mDrive = 1.0f;  // >= 1.0
//   float mBias = 0.0f;   // DC offset before folding
//   //float mTriSaw = 0;
//   Style mStyle;

//   void HandleParamsChanged() override
//   {
//     mDrive = 1.0f + 15.0f * mWaveshapeA;  // 1 - 16x

//     // B: symmetry/bias 0..1 -> -1..1; actually in theory -2..2 is usable to catch ALL signal.
//     mBias = (mWaveshapeB - 0.5f) * 4.0f;
//   }

//   static inline float fold_reflect(float x)
//   {
//     const float threshold = 1;
//     const float ax = std::abs(x);
//     const float per = 2 * threshold;
//     float m = math::fmod(ax, per);
//     if (m < 0.0f)
//       m += per;

//     const float r = (m <= threshold) ? m : (per - m);
//     return std::copysign(r, x);
//   }

//   // naive triangle - saw shape, outputting [-1,1]; period over [0,1).
//   real_t naiveTri01(real_t x01)
//   {
//     x01 = math::fract(x01);
//     if (x01 < 0.25f)
//       return x01 * 4;  // over 0,.25, *4 gives 0..1 (ramp up)
//     if (x01 < 0.75f)
//       return 2 - x01 * 4;  // over .25,.75, *4 gives 1..-1, and 2- that gives 1..-1 (ramp down)
//     return x01 * 4 - 4;    // over .75,1, *4 gives -1..0 (ramp up)
//   }

//   // // triangle->saw morph, output [-1,+1], period over [0,1).
//   // real_t naiveTriSaw01(real_t x01, real_t triSawShape01)
//   // {
//   //   x01 = math::fract(x01);

//   //   // Clamp shape to [0,1]
//   //   const real_t s = math::clamp(triSawShape01, (real_t)0, (real_t)1);

//   //   // Rise portion length: 0.5 (triangle) -> 1.0 (pure rising saw)
//   //   const real_t riseLen = (real_t)0.5 + (real_t)0.5 * s;
//   //   const real_t fallLen = (real_t)1.0 - riseLen;

//   //   // Phase shift so that s=0 matches your existing waveform's phase:
//   //   // peak at 0.25, trough at 0.75, y(0)=0.
//   //   const real_t x = math::fract(x01 + (real_t)0.25);

//   //   // Canonical asymmetric triangle that goes -1 -> +1 over riseLen, then +1 -> -1 over fallLen.
//   //   if (x < riseLen)
//   //   {
//   //     // -1 .. +1
//   //     return (real_t)-1.0 + (real_t)2.0 * (x / riseLen);
//   //   }
//   //   else
//   //   {
//   //     // When fallLen goes to 0 (s -> 1), this branch is never taken (since riseLen -> 1).
//   //     // Still, guard against tiny fallLen for numerical safety.
//   //     const real_t denom = std::max(fallLen, (real_t)1e-12);
//   //     const real_t t = (x - riseLen) / denom;  // 0..1
//   //     return (real_t)1.0 - (real_t)2.0 * t;    // +1 .. -1
//   //   }
//   // }

//   inline float sampleWaveform(float phase01)
//   {
//     if (mStyle == Style::Sine_Fold_Bias)
//     {
//       return math::sin(math::gPITimes2 * phase01);
//     }
//     return naiveTri01(phase01);
//   }

//   CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
//   {
//     const PhaseStep step = mPhaseAcc.advanceOneSample();

//     const double phase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
//     const float s = sampleWaveform((float)phase01);  // math::sin(math::gPITimes2 * float(phase01));

//     const float driven = mDrive * s + mBias;
//     float y = fold_reflect(driven);

//     return CoreSample{
//         .amplitude = y,
//         .phaseAdvance = step,
//     };
//   }
// };



struct FoldedSineCore : public OscillatorCore
{
  FoldedSineCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  float mDrive = 1.0f;  // >= 1.0
  float mBias = 0.0f;   // DC offset before folding

  void HandleParamsChanged() override
  {
    mDrive = 1.0f + 15.0f * mWaveshapeA;  // 1 - 16x

    // B: symmetry/bias 0..1 -> -1..1; actually in theory -2..2 is usable to catch ALL signal.
    mBias = (mWaveshapeB - 0.5f) * 4.0f;
  }

  static inline float fold_reflect(float x)
  {
    const float threshold = 1;
    const float ax = std::abs(x);
    const float per = 2 * threshold;
    float m = math::fmod(ax, per);
    if (m < 0.0f)
      m += per;

    const float r = (m <= threshold) ? m : (per - m);
    return std::copysign(r, x);
  }

  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const PhaseStep step = mPhaseAcc.advanceOneSample();

    const double phase01 = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    const float s = math::sin(math::gPITimes2 * float(phase01));

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
  // how waveshape params map to sound control.
  enum class ControlStyle
  {
    HP_Jitter,  // A = high pass freq; B = jitter amount
    LP_Jitter,  // A = low pass freq; B = jitter amount
  };

  // Param B: Jitter amount in [0,1]
  float mJitter01 = 0.0f;
  ControlStyle mControlStyle;

  // Jitter range in phase units (fraction of a cycle)
  static constexpr double kJitterMaxPhase = 0.45;  // +-45% of a cycle at B=1

  // Internal clock phasor and current cycle length (in phase units)
  double mClockPhase01 = 0.0;    // advances by dt each sample
  double mCycleThreshold = 1.0;  // = 1.0 + jitter per cycle

  // Output state: held value between steps
  float mHeld = 0.0f;

  M7Osc4::CorrectionSpill mSpill;

  CascadedBiquadFilter mFilter;

  SAHNoiseCore(OscillatorWaveform waveformType, ControlStyle controlStyle)
      : OscillatorCore(waveformType)
      , mControlStyle(controlStyle)
      , mFilter()
  {
  }

  void HandleParamsChanged() override
  {
    mJitter01 = math::clamp01(mWaveshapeB);
    static constexpr float kFixedQ = 0.7071f;

    mFilter.SetParams(FilterCircuit::Biquad,
                      FilterSlope::Slope24dbOct, // 2 stage.
                      (mControlStyle == ControlStyle::HP_Jitter) ? FilterResponse::Highpass
                                                                 : FilterResponse::Lowpass,
                      this->GetFrequency(mWaveshapeA),
                      kFixedQ);
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

    //  Open BLEP spill for THIS sample
    mSpill.open_sample();

    //  SNAPSHOT the held value at the **start** of the sample
    const float heldAtStart = mHeld;

    //  March through possible intra-sample events
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
      const double alphaFull = (dt > 0.0) ? math::clamp01((consumed + toEdge) / dt) : 0.0;

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

    //  Output = value at sample **start** + now-tap
    float y = heldAtStart + (float)mSpill.now;
    y = mFilter.ProcessSample(y);

    return CoreSample{
        .amplitude = y,
        .naive = heldAtStart,  // optional: expose pre-correction
        .correction = (float)mSpill.now,
        .phaseAdvance = step,
    };
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EvolvingGrainNoiseCore : public OscillatorCore
{
  // UI-ish params (0..1)
  float mGrainSize01 = 0.5f;     // shapeA
  float mMutationRate01 = 0.0f;  // shapeB

  // Grain size in samples (actual used length)
  size_t mGrainSizeSamples = 256;

  // Fixed-size buffer instead of std::vector
  static constexpr size_t kMinGrainSizeSamples = 8;
  static constexpr size_t kMaxGrainSizeSamples = 4096;
  float mGrain[kMaxGrainSizeSamples]{};

  bool mGrainValid = false;  // whether mGrain[..mGrainSizeSamples) is initialized
  bool mInitialized = false;

  EvolvingGrainNoiseCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  inline size_t ComputeGrainSizeSamples() const
  {
    // Map 0..1 → [minSize, maxSize] exponentially
    const float minSize = float(kMinGrainSizeSamples);
    const float maxSize = float(kMaxGrainSizeSamples);
    const float sizeF = minSize * math::pow(maxSize / minSize, mGrainSize01);
    const int sizeI = math::round<int>(sizeF);
    return (size_t)math::ClampI(sizeI, (int)kMinGrainSizeSamples, (int)kMaxGrainSizeSamples);
  }

  inline void EnsureGrainAllocated()
  {
    const size_t targetSize = ComputeGrainSizeSamples();

    // If size unchanged and we already have valid content, nothing to do
    if (targetSize == mGrainSizeSamples && mGrainValid)
      return;

    mGrainSizeSamples = targetSize;

    // Fill the active portion of the fixed buffer with noise
    for (size_t i = 0; i < mGrainSizeSamples; ++i)
    {
      mGrain[i] = math::randN11();  // [-1,1]
    }

    mGrainValid = true;
  }

  inline void MutateGrainCycle()
  {
    if (!mGrainValid || mGrainSizeSamples == 0)
      return;

    const float targetCountF = mMutationRate01 * float(mGrainSizeSamples);
    const int targetCountI = math::round<int>(targetCountF);

    const size_t mutateCount = (size_t)math::ClampI(targetCountI, 0, (int)mGrainSizeSamples);

    for (size_t i = 0; i < mutateCount; ++i)
    {
      // Random index in [0, mGrainSizeSamples)
      const size_t index = (size_t)(math::rand01() * double(mGrainSizeSamples)) % mGrainSizeSamples;

      mGrain[index] = math::randN11();
    }
  }

  void HandleParamsChanged() override
  {
    mGrainSize01 = mWaveshapeA;
    mMutationRate01 = mWaveshapeB;

    EnsureGrainAllocated();
  }

  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
  {
    const auto step = mPhaseAcc.advanceOneSample();

    if (!mInitialized)
    {
      mInitialized = true;
      mGrainValid = false;  // force re-init
      EnsureGrainAllocated();
      MutateGrainCycle();
    }

    // assert grain valid & has samples.

    const size_t index = (size_t)(step.phaseBegin01 * mGrainSizeSamples) % mGrainSizeSamples;

    const float y = mGrain[index];

    const bool completedCycle = step.hasReset || (step.phaseBegin01 + step.dt >= 1.0f);
    if (completedCycle)
    {
      MutateGrainCycle();
    }

    return CoreSample{
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }

  void RestartDueToNoteOn() override
  {
    OscillatorCore::RestartDueToNoteOn();
    mInitialized = false;
    // for fresh random grain per note
    // mGrainValid = false;
  }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct WhiteNoiseCore2 : public OscillatorCore
{
  //static constexpr float kFilterQ = 0.707f;
  // note: impulses are always 1 sample. And impulses are allowed to be adjascent.
  // the only thing that makes this "grain" is that
  float mProbability01 = 0;
  // it's tempting to do gain compensation. when heavy filtering and sweeping probability, you're just changing the signal level.
  // but cant do this in a generalized way; it will only cause clipping.
  float mDutyCycle01 =
      0.5f;  // how much of the cycle is spent on noise vs. silence. after this point in the cycle, output forced to 0.
  CascadedBiquadFilter mFilter;

  enum class ControlStyle
  {
    Prob_Duty,  // A = probability, B = duty cycle
    Prob_LP,    // A = probability, B = Q (note frequency is the cutoff)
    //Prob_HP,    // A = probability, B = Q (note frequency is the cutoff)
    Prob_BP,  // A = probability, B = Q (note frequency is the cutoff)
    Duty_LP,
    //Duty_HP,
    Duty_BP,
  };

  ControlStyle mControlStyle;

  WhiteNoiseCore2(OscillatorWaveform waveformType, ControlStyle controlStyle)
      : OscillatorCore(waveformType)
      , mFilter()
      , mControlStyle(controlStyle)
  {
  }

  void HandleParamsChanged() override
  {
    mWaveshapeA = math::clamp01(mWaveshapeA);
    mProbability01 = mWaveshapeA * mWaveshapeA;  // curve for better control at low vals.

    mWaveshapeB = math::clamp01(mWaveshapeB);
    mDutyCycle01 = 1.0f;
    //mAmpExtent01 = 1.0f;

    switch (mControlStyle)
    {
      case ControlStyle::Prob_Duty:
        mDutyCycle01 = mWaveshapeB;
        break;

      default:
      case ControlStyle::Prob_LP:
      //case ControlStyle::Prob_HP:
      case ControlStyle::Prob_BP:
        break;
      case ControlStyle::Duty_LP:
      //case ControlStyle::Duty_HP:
      case ControlStyle::Duty_BP:
      {
        mProbability01 = 0.5f;  // fixed default.
        mDutyCycle01 = mWaveshapeA;
        break;
      }
    }

    auto filterType = FilterResponse::Lowpass;  // default, may be overridden below

    switch (mControlStyle)
    {
      default:
        mFilter.Disable();
        break;
      case ControlStyle::Prob_BP:
      case ControlStyle::Duty_BP:
        filterType = FilterResponse::Bandpass;
      case ControlStyle::Prob_LP:
      //case ControlStyle::Prob_HP:
      case ControlStyle::Duty_LP:
        //case ControlStyle::Duty_HP:
        {
          ParamAccessor pa{&mWaveshapeB, 0};
          float q = pa.GetDivCurvedValue(0, gBiquadFilterQCfg);
          mFilter.SetParams(FilterCircuit::Biquad, FilterSlope::Slope24dbOct, filterType, this->mMainFrequencyHz, q);
          break;
        }
    }
  }

  CoreSample renderSampleAndAdvance(float) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    float y = 0.0f;

    const bool inDutyWindow = step.phaseBegin01 < mDutyCycle01;
    if (inDutyWindow && math::rand01() < mProbability01)
    {
      const float sign = (math::rand01() < 0.5f) ? -1.0f : 1.0f;
      y = sign;
    }

    y = mFilter.ProcessSample(y);
    y = math::clampN11(y); // filters make this unpredictable so clamp to conform to oscillator's expected output

    return CoreSample{
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }

  void RestartDueToNoteOn() override
  {
    OscillatorCore::RestartDueToNoteOn();
    mFilter.Reset();
  }
};


}  // namespace M7

}  // namespace WaveSabreCore
