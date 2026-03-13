#pragma once

#include <algorithm>  // for std::sort, std::unique

#include "../Basic/Noise.hpp"
#include "../Basic/Vector.hpp"
#include "../Filters/BandSplitter.hpp"
#include "../Filters/BiquadFilter.h"
#include "../Filters/LinkwitzRileyFilter.hpp"
#include "../Filters/SVFilter.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../GigaSynth/Maj7Oscillator3Base.hpp"
#include "./Maj7Oscillator3Shape.hpp"
#include "./Maj7Oscillator4WS.hpp"


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

#ifdef ENABLE_TRIANGLE_FOLD_WAVEFORM
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
    float m = math::fmodf(ax, period);
    if (m < 0.0f)
      m += period;
    const float r = (m <= threshold) ? m : (period - m);
    return math::copysignf(r, x);
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

  PodVector<double> breakpoints;
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
  breakpoints.erase(std::unique(breakpoints.begin(),
                                breakpoints.end(),
                                [](double a, double b)
                                {
                                  return std::abs(a - b) <= 1e-7;
                                }),
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
#endif  // ENABLE_TRIANGLE_FOLD_WAVEFORM

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

#ifdef ENABLE_PULSE4_WAVEFORM

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

#endif  // ENABLE_PULSE4_WAVEFORM


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
        //.phaseAdvance = step,
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
  float mA;
  float mB;
  float mC;
  float mH;

  // Derived for clipping
  float mClipThreshold;
  float mClipGain;
  static constexpr float kLim = 0.998f;
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
    float waveShapeAAsClip = math::lerp(0, kLim, mWaveshapeA);
    switch (mVariant)
    {
      case SineCoreExtVariant::DCClip:
      {
        mA = mWaveshapeB * 2 - 1;
        break;
      }
      case SineCoreExtVariant::ClipSilence:
      {
        mB = waveShapeAAsClip;
        mC = mWaveshapeB;
        break;
      }
      case SineCoreExtVariant::ClipHarm:
      {
        mB = waveShapeAAsClip;
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

      mClipThreshold = math::clamp(1.0f - mB, 1e-6f, 1.0f);
      mClipGain = 1.0f / mClipThreshold;
  }

  inline float ClipAndNormalize(float x) const
  {
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
        //.naive = out,
        //.correction = 0.0f,
        //.phaseAdvance = step,
    };
  }
};

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
    float m = math::fmodf(ax, per);
    if (m < 0.0f)
      m += per;

    const float r = (m <= threshold) ? m : (per - m);
    return math::copysignf(r, x);
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
        //.phaseAdvance = step,
    };
  }
};

struct SAHNoiseCore : public OscillatorCore
{
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

  // Clock phasor and current cycle length (phase units)
  double mClockPhase01 = 0.0;    // advances by dt each sample
  double mCycleThreshold = 0.0;  // <= 0.0 means "uninitialized"

  // Output: held value between steps
  float mHeld = 0.0f;

  CorrectionSpill mSpill;
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

    static constexpr float kFixedQ = 0.7071f;  //db
    const auto fixedReso01 = Param01{gBiquadFilterQCfg.ValueToParam01(kFixedQ)};
    const FilterResponse responseType = (mControlStyle == ControlStyle::HP_Jitter) ? FilterResponse::Highpass
                                                                                   : FilterResponse::Lowpass;

    mFilter.SetParams(FilterCircuit::Biquad,
                      FilterSlope::Slope24dbOct,  // 2 stage.
                      responseType,
                      this->GetFrequency(mWaveshapeA),
                      fixedReso01,
                      0 /*gaindb*/);
  }

  inline void schedule_next_cycle_length()
  {
    // J in [0, kJitterMaxPhase]
    const double J = double(mJitter01) * kJitterMaxPhase;
    const double jitter = J * math::randN11();  // [-J, +J]
    mCycleThreshold = 1.0 + jitter;             // in [0.55, 1.45]
  }

  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    const double dt = step.dt;

    // First-time init (or explicit reset) when threshold <= 0
    if (mCycleThreshold <= 0.0)
    {
      schedule_next_cycle_length();
      mHeld = math::randN11();  // start value
    }

    mSpill.open_sample();

    // Snapshot held value at start of sample
    const float heldAtStart = mHeld;

    const double toEdge = mCycleThreshold - mClockPhase01;

    // With jitter in [-0.45, 0.45], threshold in [0.55, 1.45],
    // and dt <= 0.5 (Nyquist), we can hit at most ONE edge in a sample.
    if (dt >= toEdge)
    {
      // Edge occurs inside this sample
      const double alpha = toEdge / dt;  // guaranteed in (0, 1] when dt > 0

      const float newTarget = math::randN11();
      const double dAmp = double(newTarget) - double(mHeld);

      // Band-limit the step at alpha
      mSpill.add_edge(alpha, {dAmp, 0}, dt);

      // Commit the step
      mHeld = newTarget;

      // Advance phasor after edge
      mClockPhase01 = dt - toEdge;

      // Schedule next cycle
      schedule_next_cycle_length();
    }
    else
    {
      // No edge this sample
      mClockPhase01 += dt;
    }

    float y = heldAtStart + float(mSpill.now);
    y = mFilter.ProcessSample(y);

    return CoreSample{
        .amplitude = y,
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
  static constexpr size_t kMinGrainSizeSamples = 1;
  static constexpr size_t kMaxGrainSizeSamples = 1024;
  float mGrain[kMaxGrainSizeSamples]{};

  bool mGrainValid = false;  // whether mGrain[..mGrainSizeSamples) is initialized
  bool mInitialized = false;

  EvolvingGrainNoiseCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  size_t ComputeGrainSizeSamples() const;
  void EnsureGrainAllocated();

  void MutateGrainCycle();

  void HandleParamsChanged() override;

  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override;

  void RestartDueToNoteOn() override;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// here's a pitched noise that uses another evolving wave cycle.
// this one uses a 2D fbm noise field. over time drifts through the field in a big circle.
// a big circle allows infinite movement over the field without any dimension exploding, causing precision issues (#144)
// 1. small enough that the precision scale is under control (so modulations & knobs are always consistent and well-behaved and predictable)
// 2. big enough that you never hear any repetitions
// the wave cycle is another circle. allows for smooth cyclical waveform, continuous movement and drifting, modulation-friendly.
struct ContinuousNoiseCore : public OscillatorCore
{
  static int gInstanceCount;

  ContinuousNoiseCore(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
      , mFieldScaleCurrent(1.0f)
      , mFieldScaleTarget(1.0f)
      , mBaseCenter(0.0f, 10.0f * float(++gInstanceCount))
  {
  }

  double mFieldScaleCurrent;
  double mFieldScaleTarget;
  double mMovementSpeed = 0;

  double mTravelPhase = 0.0;
  static constexpr int kTravelRadius = 1024;
  static constexpr float kOneOverTravelRadius = 1.0f / kTravelRadius;
  DoublePair mBaseCenter;

  void HandleParamsChanged() override
  {
    mFieldScaleTarget = math::lerp(0.5f, 7.0f, mWaveshapeA);
    // want enough speed to feel like really NOISE; that doesn't happen until after 150 or so. but below that you get a lot of variation.
    // so go for high max, but use a curve to allow good control at low speeds.
    mMovementSpeed = math::lerp(0,
                                400.0f * kOneOverTravelRadius * Helpers::CurrentSampleRateRecipF,
                                mWaveshapeB * mWaveshapeB);
  }

  CoreSample renderSampleAndAdvance(float /*audioRatePhaseOffset*/) override
  {
    const auto step = mPhaseAcc.advanceOneSample();

    const auto angle = step.phaseBegin01 * math::gPITimes2d;
    const auto orbit = SinCosD(angle);

    // Geometric continuity correction for field scale changes:
    // preserve the currently sampled field-space point.
    if (mFieldScaleCurrent != mFieldScaleTarget)
    {
      const double ds = mFieldScaleCurrent - mFieldScaleTarget;
      mBaseCenter.Accumulate(orbit * ds);
      mFieldScaleCurrent = mFieldScaleTarget;
    }

    mTravelPhase += mMovementSpeed;
    const auto drift = SinCosD(mTravelPhase) * kTravelRadius;
    const auto center = mBaseCenter + drift;

    const double scale = mFieldScaleCurrent;

    const double v = fbm2D(center + orbit * scale);

    return CoreSample{
        .amplitude = static_cast<float>(v),
    };
  }
};

#ifdef ENABLE_FILTERED_WHITENOISE_WAVEFORMS
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
      0;  // how much of the cycle is spent on noise vs. silence. after this point in the cycle, output forced to 0.

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

  CascadedBiquadFilter mFilter;

  WhiteNoiseCore2(OscillatorWaveform waveformType, ControlStyle controlStyle)
      : OscillatorCore(waveformType)
      , mFilter()
      , mControlStyle(controlStyle)
  {
    //mFilter.SetCompensationEnabled(true);
  }

  void HandleParamsChanged() override
  {
    mProbability01 = mWaveshapeA * mWaveshapeA;  // curve for better control at low vals.
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
          mFilter.SetParams(FilterCircuit::Biquad,
                            FilterSlope::Slope24dbOct,
                            filterType,
                            this->mMainFrequencyHz,
                            Param01{mWaveshapeB},
                            0 /*gaindb*/);
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
    y = math::clampN11(y);  // filters make this unpredictable so clamp to conform to oscillator's expected output

    return CoreSample{
        .amplitude = y,
        //.naive = y,
        //.correction = 0.0f,
        //.phaseAdvance = step,
    };
  }

  void RestartDueToNoteOn() override
  {
    OscillatorCore::RestartDueToNoteOn();
    mFilter.Reset();
  }
};
#else   //  ENABLE_FILTERED_WHITENOISE_WAVEFORMS

// simplified: only 1 control style: prob_duty.
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
      0;  // how much of the cycle is spent on noise vs. silence. after this point in the cycle, output forced to 0.

  WhiteNoiseCore2(OscillatorWaveform waveformType)
      : OscillatorCore(waveformType)
  {
  }

  void HandleParamsChanged() override
  {
    mProbability01 = mWaveshapeA * mWaveshapeA;  // curve for better control at low vals.
    mDutyCycle01 = mWaveshapeB;

    auto filterType = FilterResponse::Lowpass;  // default, may be overridden below
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

    return CoreSample{
        .amplitude = y,
    };
  }
};
#endif  //  ENABLE_FILTERED_WHITENOISE_WAVEFORMS


}  // namespace M7

}  // namespace WaveSabreCore
