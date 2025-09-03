#pragma once
#include "Maj7OscillatorBase.hpp"

#pragma once

#include <WaveSabreCore/Filters/FilterOnePole.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
//
//namespace WaveSabreCore
//{
//namespace M7
//{
//
///////////////////////////////////////////////////////////////////////////////
//struct SineClipWaveform : IOscillatorWaveform
//{
//  // returns Y value at specified phase. instance / stateless.
//  virtual float NaiveSample(float phase01) override
//  {
//    return math::sin(math::gPITimes2 * phase01);
//  }
//
//  // this is not improved by returing correct slope. blepping curves is too hard 4 me.
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    return 1;  // math::cos(math::gPITimes2 * phase01);
//  }
//
//  virtual void AfterSetParams() override
//  {
//    //mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
//
//    //mEdge1 = (0.75f - mShape * .25f);
//    //mEdge2 = (0.75f + mShape * .25f);
//    //mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained
//
//    //mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
//    //mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    //float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
//    //OSC_ACCUMULATE_BLAMP(newPhase, mEdge1, scale, samples, samplesTillNextSample);
//    //OSC_ACCUMULATE_BLAMP(newPhase, mEdge2, scale, samples, samplesTillNextSample);
//  }
//};
//
///////////////////////////////////////////////////////////////////////////////
//struct SineHarmTruncWaveform : IOscillatorWaveform
//{
//  // returns Y value at specified phase. instance / stateless.
//  virtual float NaiveSample(float phase01) override
//  {
//    if (phase01 < this->mShapeA)
//    {
//      float rad = phase01 / mShapeA * math::gPITimes2;
//      float s = math::sin(rad);
//      float b = .5f - math::cos(rad) * .5f;
//      s = s * b * 1.5f;
//      return s;
//    }
//    else
//    {
//      return 0;
//    }
//  }
//
//  // this is not improved by returing correct slope. blepping curves is too hard 4 me.
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    return 0;
//  }
//
//  virtual void AfterSetParams() override
//  {
//    float waveshape = std::abs(mShapeA * 2 - 1);  // create reflection to make bipolar
//    mShapeA = math::lerp(1, 0.1f, waveshape);
//  }
//};
//
///////////////////////////////////////////////////////////////////////////////
//// frequency = sample and hold (a sort of sample-crush, in the spirit of a low pass)
//// shape = sigle pole high pass
//struct WhiteNoiseWaveform : IOscillatorWaveform
//{
//  OnePoleFilter mHPFilter;
//  float mCurrentLevel = 0;
//  float mCurrentSample = 0;
//
//  // returns Y value at specified phase. instance / stateless.
//  virtual float NaiveSample(float phase01) override
//  {
//    return mCurrentSample;
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    return 0;
//  }
//
//  virtual void OSC_ADVANCE(float samples, float samplesTillNextSample)
//  {
//    // assume we're always advancing by 1 exact sample. it theoretically breaks hard sync but like, who the f is hard-syncing a white noise channel?
//
//    mPhase += mPhaseIncrement;
//    if (mPhase > 1)
//    {
//      mCurrentLevel = math::randN11();
//      mPhase = math::fract(mPhase);
//    }
//
//    mCurrentSample = mHPFilter.ProcessSample(mCurrentLevel);
//    //mCurrentSample = mCurrentLevel;
//  }
//
//
//  virtual void AfterSetParams() override
//  {
//    // shape determines the high pass frequency
//    float cutoff;
//
//    if (mIntention == OscillatorIntention::LFO)
//    {
//      cutoff = mFrequency * mShapeA * mShapeA;
//    }
//    else
//    {
//      ParamAccessor pa{&mShapeA, 0};
//      cutoff = pa.GetFrequency(0, FreqParamConfig{mFrequency, 6, 0 /*assume never used*/});
//    }
//
//
//    mHPFilter.SetParams(FilterType::HP, cutoff, 0);
//  }
//};
//
//
/////////////////////////////////////////////////////////////////////////////////
////struct VarTriWaveform :IOscillatorWaveform
////{
////	virtual void AfterSetParams() override
////	{
////		mShape = math::lerp(0.99f, .01f, mShape); // just prevent div0
////	}
//
////	// returns Y value at specified phase. instance / stateless.
////	virtual float NaiveSample(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return phase01 / mShape * 2 - 1;
////		}
////		return (1 - phase01) / (1 - mShape) * 2 - 1;
////	}
//
////	// this is not improved by returing correct slope. blepping curves is too hard 4 me.
////	virtual float NaiveSampleSlope(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return 2 * mShape;
////		}
////		return -2 * (1 - mShape);
////	}
//
////	virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
////	{
////		//       1-|            ,-',        |
////		//         |         ,-'  | ',      |
////		//       0-|      ,-'         ',    |
////		//         |   ,-'        |     ',  |
////		//      -1-|,-'                   ',|
////		// pw       0-------------|---------1
////		// slope:
////		//         |--------------          | = 1/pw
////		//         |                        | = 0
////		//         |              ----------| = -1/(1-pw)
////		float pw = mShape;
////		// this is
////		// dt / (1/pw - 1/(1-pw))
////		float scale = (float)(mPhaseIncrement / (pw - pw * pw));
////		OSC_ACCUMULATE_BLAMP(newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
////		OSC_ACCUMULATE_BLAMP(newPhase, pw/*edge*/, -scale, samples, samplesTillNextSample);
////	}
////};
//
///////////////////////////////////////////////////////////////////////////////
//struct PulseTristateWaveform : IOscillatorWaveform
//{
//  float mT2 = 0;
//  static constexpr float mT3 = 0.5f;
//  float mT4 = 0;
//
// edges:
// shapeA is -1 to 1
// 0
// mt2: abs(shapeA) * .5 (so shape determines width of high state)
// mt3 is always 0.5
// mt4 is 0.5 + mt2
// so main duty cycle is always 50%
// and shape param determines the width of the non-0 state.
// this is actually 4 states (high, 0, low, 0)
//  virtual void AfterSetParams() override
//  {
//    mShapeA = std::abs(mShapeA * 2 - 1);  // create reflection to make bipolar
//    this->mShapeA = math::lerp(0.95f, .05f, mShapeA);
//    mT2 = mShapeA / 2;
//    //mT3 = .5f;
//    mT4 = .5f + mT2;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    if (phase01 < mT2)
//      return 1;
//    if (phase01 < mT3)
//      return 0;
//    if (phase01 < mT4)
//      return -1;
//    return 0;
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    return 0;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    OSC_ACCUMULATE_BLEP(newPhase, 0, .5f, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLEP(newPhase, mT2, -.5f, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLEP(newPhase, mT3, -.5f, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLEP(newPhase, mT4, .5f, samples, samplesTillNextSample);
//  }
//};
//
/////////////////////////////////////////////////////////////////////////////////
////struct SineAsymWaveform :IOscillatorWaveform
////{
////	virtual void SetParams(float freq, float phaseOffset, float waveshape, double sampleRate) override
////	{
////		IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate);
////		mShape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
////		mShape = math::lerp(.5f, 0.03f, mShape * mShape); // prevent div0
////	}
//
////	virtual float NaiveSample(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return math::cos(math::gPI * phase01 / mShape);
////		}
////		return -math::cos(math::gPI * (phase01 - mShape) / (1 - mShape));
////	}
//
////	virtual float NaiveSampleSlope(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return math::sin(math::gPI * phase01 / mShape);
////		}
////		return -math::sin(math::gPI * (phase01 - mShape) / (1 - mShape));
////	}
////};
//
/////////////////////////////////////////////////////////////////////////////////
////struct SineTruncWaveform :IOscillatorWaveform
////{
////	virtual void SetParams(float freq, float phaseOffset, float waveshape, double sampleRate) override
////	{
////		IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate);
////		waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
////		mShape = math::lerp(0.97f, 0.03f, waveshape); // prevent div0
////	}
//
////	virtual float NaiveSample(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return math::sin(math::gPITimes2 * phase01 / mShape);
////		}
////		return 0;
////	}
//
////	virtual float NaiveSampleSlope(float phase01) override
////	{
////		if (phase01 < mShape) {
////			return math::gPITimes2 * math::cos(math::gPITimes2 * phase01 / mShape) / mShape;
////		}
////		return 0;
////	}
//
////	virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
////	{
////		float scale = float(math::gPI * mPhaseIncrement / mShape);
////		OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
////		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, -scale, samples, samplesTillNextSample);
////	}
////};
//
//
/////////////////////////////////////////////////////////////////////////////////
////struct TriClipWaveform :IOscillatorWaveform
////{
////	virtual void AfterSetParams() override
////	{
////		mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
////		mShape = math::lerp(0.97f, 0.03f, mShape);
////		mDCOffset = -.5;//-.5 * this.shape;
////		mScale = 2;//1/(.5+this.DCOffset);
////		//mScale *= gOscillatorHeadroomScalar;
////	}
//
////	virtual float NaiveSample(float phase01) override
////	{
////		if (phase01 >= mShape) {
////			return 0;
////		}
////		float y = phase01 / (mShape * 0.5f);
////		if (y < 1) {
////			return y;
////		}
////		return 2 - y;
////	}
//
////	virtual float NaiveSampleSlope(float phase01) override
////	{
////		if (phase01 >= mShape) {
////			return 0;
////		}
////		float y = phase01 / (mShape * 0.5f);
////		if (y < 1) {
////			return 1 / mShape;
////		}
////		return -1 / mShape;
////	}
//
////	virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
////	{
////		float scale = float(mPhaseIncrement / mShape);
////		OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
////		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .5f/*edge*/, -2 * scale, samples, samplesTillNextSample);
////		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, scale, samples, samplesTillNextSample);
////	}
////};
//
//
/////////////////////////////////////////////////////////////////////////////////
//struct TriTruncWaveform : IOscillatorWaveform
//{
//  virtual void AfterSetParams() override
//  {
//    mShapeA = std::abs(mShapeA * 2 - 1);  // create reflection to make bipolar
//    mShapeA = math::lerp(0.97f, .03f, mShapeA);
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    if (phase01 >= mShapeA)
//    {
//      return 0;
//    }
//    float tx = (phase01 - mShapeA) / mShapeA;
//    tx -= 0.25f;
//    tx = math::fract(tx);
//    tx -= 0.5f;
//    tx = std::abs(tx) * 4 - 1;
//    return tx;
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    if (phase01 < mShapeA * 0.25f)
//    {
//      return 2 / mShapeA;
//    }
//    if (phase01 < mShapeA * 0.75f)
//    {
//      return -2 / mShapeA;
//    }
//    if (phase01 < mShapeA)
//    {
//      return 2 / mShapeA;
//    }
//    return 0;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    float scale = float(mPhaseIncrement * 2 / mShapeA);  //OSC_GENERAL_SLOPE(this.shape);
//    OSC_ACCUMULATE_BLAMP(newPhase, 0 /*edge*/, scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mShapeA * .25f /*edge*/, -2 * scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mShapeA * .75f /*edge*/, 2 * scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mShapeA /*edge*/, -scale, samples, samplesTillNextSample);
//  }
//};
//
///////////////////////////////////////////////////////////////////////////////
//struct VarTrapezoidWaveform : IOscillatorWaveform
//{
//  float mT1 = 0;
//  float mT2 = 0;
//  float mT3 = 0;
//  float mSlope = 0.1f;  // 0, .5 range., avoid div0!
//  float mWidth = 0;
//
//  virtual void AfterSetParams() override
//  {
//    float remainingSpace = 1 - 2 * mSlope;
//    mWidth = mShapeA * remainingSpace;
//    mSlope = math::lerp(0.02f, 0.47f, mShapeB * 0.5f);
//
//    mT1 = mWidth;
//    mT2 = mWidth + mSlope;
//    mT3 = 1 - mSlope;
//  }
//
//  virtual float NaiveSample(float t) override
//  {
//    /*
//				  //--------------------------------------------------------
//				  // edge    0   t1  t2        t3  1
//				  //         |    |  |          |  |
//				  //         -----,                , +1
//				  //               \              /
//				  //                \            /
//				  //                 `----------`    -1
//				  // width   |----|
//				  // slope        |--|          |--|
//
//				  // derivative
//				  // edge    0   t1  t2        t3  1
//				  //         |    |  |          |  |
//				  //                            ---- ((slope*2))
//				  //         -----   -----------     0
//				  //              ---                -((slope*2))
//				*/
//    if (t < mT1)
//    {
//      return 1;
//    }
//    if (t < mT2)
//    {
//      return 1 - 2 * (t - mT1) / mSlope;
//    }
//    if (t < mT3)
//    {
//      return -1;
//    }
//    return -1 + 2 * (t - mT3) / mSlope;
//  }
//
//  virtual float NaiveSampleSlope(float t) override
//  {
//    if (t < mT1)
//    {
//      return 0;
//    }
//    if (t < mT2)
//    {
//      return -1 / mSlope;
//    }
//    if (t < mT3)
//    {
//      return 0;
//    }
//    return 1 / mSlope;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    float scale = (float)(mPhaseIncrement / (mSlope));
//    OSC_ACCUMULATE_BLAMP(newPhase, 0 /*edge*/, -scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mT1 /*edge*/, -scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mT2 /*edge*/, scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, mT3 /*edge*/, scale, samples, samplesTillNextSample);
//  }
//};
//
//struct TriSquareWaveform : VarTrapezoidWaveform
//{
//  virtual void SetParams(float freq,
//                         float phaseOffset,
//                         float slopeA,
//                         float shapeB,
//                         double sampleRate,
//                         OscillatorIntention intention) override
//  {
//    slopeA = std::abs(slopeA * 2 - 1);  // create reflection to make bipolar
//    mSlope = math::lerp(0.5f, 0.015f, math::sqrt01(slopeA));
//    VarTrapezoidWaveform::SetParams(freq, phaseOffset, 0.5f, 0.5f, sampleRate, intention);
//  }
//};
//
//
//struct NotchSawWaveform : IOscillatorWaveform
//{
//  // fixed notch at 0.5 for simplicity; mShape controls depth [0..1]
//  static constexpr float kNotchPos = 0.5f;
//  float mDepth = 0.0f;  // actual notch depth [0..1]
//
//  virtual void AfterSetParams() override
//  {
//    // gentle curve for depth feel, and avoid extremes
//    mDepth = math::clamp01(mShapeA);
//    // DC: step is active for half the cycle -> avg shift = -depth * 0.5
//    mDCOffset = +0.5f * mDepth;
//    mScale = 1.0f;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    // base saw plus a negative step at the notch
//    float y = phase01 * 2.0f - 1.0f;
//    if (phase01 >= kNotchPos)
//      y -= mDepth;
//    return y;
//  }
//
//  virtual float NaiveSampleSlope(float /*phase01*/) override
//  {
//    // piecewise linear everywhere except discontinuities (handled by BLEP)
//    return 2.0f;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // One value-step at wrap (0), one at notch.
//    // For a value step of delta, blepScale should be delta/2.
//    // Base saw has delta = -2 -> blepScale = -1 at 0.
//    // The notch adds: step -mDepth at kNotchPos, and +mDepth at wrap (when step resets).
//    OSC_ACCUMULATE_BLEP(newPhase, 0.0f, -1.0f, samples, samplesTillNextSample);                // saw wrap
//    OSC_ACCUMULATE_BLEP(newPhase, 0.0f, +0.5f * mDepth, samples, samplesTillNextSample);       // step resets at wrap
//    OSC_ACCUMULATE_BLEP(newPhase, kNotchPos, -0.5f * mDepth, samples, samplesTillNextSample);  // notch step
//  }
//};
//
//struct RectifiedSineWaveform : IOscillatorWaveform
//{
//  // mShape is the morph amount 0 (pure sine) .. 1 (full rectified)
//  virtual void AfterSetParams() override
//  {
//    // DC of (2|sin|-1) over a cycle is (4/pi - 1). We lerp toward it by mShape.
//    float dcRect = 4.0f / math::gPI - 1.0f;
//    mDCOffset = -dcRect * mShapeA;
//    mScale = 1.0f;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    float s = math::sin(math::gPITimes2 * phase01);
//    float rect = 2.0f * std::abs(s) - 1.0f;
//    return math::lerp(s, rect, mShapeA);
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    // derivative (not used for Visit kinks, but useful for sync)
//    float s = math::sin(math::gPITimes2 * phase01);
//    float c = math::cos(math::gPITimes2 * phase01);
//    float ds = math::gPITimes2 * c;                                     // sine derivative
//    float drect = 4.0f * math::gPI * ((s >= 0.0f) ? 1.0f : -1.0f) * c;  // derivative of 2|sin|-1
//    return math::lerp(ds, drect, mShapeA);
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // Kinks at phase 0 and 0.5. Slope jump magnitude = 8*pi*mShape.
//    if (mIntention != OscillatorIntention::Audio || mShapeA <= 1e-5f)
//      return;
//    float scale = float(mPhaseIncrement * (8.0 * math::gPI) * mShapeA);
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, scale, samples, samplesTillNextSample);
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.5f, scale, samples, samplesTillNextSample);
//  }
//};
//
//struct PhaseDistortedSineWaveform : IOscillatorWaveform
//{
//  float mBreak = 0.5f;  // breakpoint in [0,1], avoid extremes
//
//  virtual void AfterSetParams() override
//  {
//    // map mShape -> breakpoint (keep away from 0/1 to prevent crazy slopes)
//    float x = math::clamp01(mShapeA);
//    mBreak = math::lerp(0.08f, 0.92f, x);
//    mDCOffset = 0.0f;
//    mScale = 1.0f;
//  }
//
//  inline float WarpPhase(float t) const
//  {
//    // compress [0..mBreak] into [0..0.5], [mBreak..1] into [0.5..1]
//    if (t < mBreak)
//      return (t / mBreak) * 0.5f;
//    return 0.5f + (t - mBreak) / (1.0f - mBreak) * 0.5f;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    float phi = WarpPhase(phase01);
//    return math::sin(math::gPITimes2 * phi);
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    // dy/dt = 2π cos(2π phi) * dphi/dt
//    float dphidt = (phase01 < mBreak) ? (0.5f / mBreak) : (0.5f / (1.0f - mBreak));
//    float phi = WarpPhase(phase01);
//    return math::gPITimes2 * math::cos(math::gPITimes2 * phi) * dphidt;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // BLAMP at breakpoint (phi=0.5, cos(pi)=-1) and at wrap (0).
//    float dphidtBefore = 0.5f / mBreak;
//    float dphidtAfter = 0.5f / (1.0f - mBreak);
//
//    // slope jump at mBreak: -2π*(after - before)
//    float deltaMid = -2.0f * float(math::gPI) * (dphidtAfter - dphidtBefore);
//    OSC_ACCUMULATE_BLAMP(newPhase, mBreak, float(mPhaseIncrement) * deltaMid, samples, samplesTillNextSample);
//
//    // slope jump at wrap (end -> start): 2π*(before - after)
//    float deltaWrap = 2.0f * float(math::gPI) * (dphidtBefore - dphidtAfter);
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, float(mPhaseIncrement) * deltaWrap, samples, samplesTillNextSample);
//  }
//};
//
//// New: Staircase saw (quantized steps per cycle)
//struct StaircaseSawWaveform : IOscillatorWaveform
//{
//  static constexpr int kMinSteps = 2;
//  static constexpr int kMaxSteps = 32;
//  int mSteps = 8;
//  float mStepHeight = 0.0f;  // 2/steps
//
//  virtual void AfterSetParams() override
//  {
//    // map shape^2 -> steps in [2..32]
//    float x = mShapeA;
//    x = x * x;
//    int steps = (int)(math::lerp((float)kMinSteps, (float)kMaxSteps, x) + 0.5f);
//    mSteps = math::ClampI(steps, kMinSteps, kMaxSteps);
//    mStepHeight = 2.0f / (float)mSteps;
//    // Zero mean for our definition where top step is +1
//    mDCOffset = -1.0f / (float)mSteps;
//    mScale = 1.0f;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    float idx = std::floor(phase01 * (float)mSteps);
//    float y = -1.0f + mStepHeight * (idx + 1.0f);
//    return y;
//  }
//
//  virtual float NaiveSampleSlope(float /*phase01*/) override
//  {
//    return 0.0f;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // Wrap step at 0 has delta = -2 + 2/steps => blepScale = -(1 - 1/steps)
//    float wrapScale = -(1.0f - 1.0f / (float)mSteps);
//    OSC_ACCUMULATE_BLEP(newPhase, 0.0f, wrapScale, samples, samplesTillNextSample);
//    // interior rising steps at n/steps with delta = +2/steps => blepScale = +1/steps
//    float interiorScale = 1.0f / (float)mSteps;
//    for (int n = 1; n < mSteps; ++n)
//    {
//      float edge = (float)n / (float)mSteps;
//      OSC_ACCUMULATE_BLEP(newPhase, edge, interiorScale, samples, samplesTillNextSample);
//    }
//  }
//};
//
//// New: Folded triangle (single fold threshold)
//struct TriFoldWaveform : IOscillatorWaveform
//{
//  float mThreshold = 1.0f;  // 0..1
//
//  virtual void AfterSetParams() override
//  {
//    // map shape^2 to threshold from 1 -> 0.05 (avoid 0)
//    float x = mShapeA;
//    x = x * x;
//    mThreshold = math::lerp(1.0f, 0.05f, x);
//    mDCOffset = 0.0f;
//    mScale = 1.0f;
//  }
//
//  inline static float Tri(float t)
//  {
//    // triangle, range [-1,1], tri(0)=0
//    float a = math::fract(t + 0.25f) - 0.5f;
//    return (std::abs(a) * 4.0f - 1.0f);
//  }
//
//  inline static float TriSlope(float t)
//  {
//    // sign based on position inside the triangle cycle
//    float b = math::fract(t + 0.25f) - 0.5f;
//    return (b < 0.0f) ? -4.0f : 4.0f;
//  }
//
//  virtual float NaiveSample(float phase01) override
//  {
//    float x = Tri(phase01);
//    float ax = std::abs(x);
//    if (ax <= mThreshold)
//      return x;  // no fold region
//    float y = (2.0f * mThreshold - ax);
//    return (x >= 0.0f) ? y : -y;
//  }
//
//  virtual float NaiveSampleSlope(float phase01) override
//  {
//    float x = Tri(phase01);
//    float s = TriSlope(phase01);
//    // inside fold, dy/dt = -s, else dy/dt = s
//    return (std::abs(x) <= mThreshold) ? s : -s;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // Always handle base triangle corners and wrap
//    float k = float(mPhaseIncrement) * 8.0f;                                    // slope jump magnitude (±8)
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, +k, samples, samplesTillNextSample);   // wrap: -4 -> +4
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.25f, -k, samples, samplesTillNextSample);  // corner: +4 -> -4 (after folding sign)
//    OSC_ACCUMULATE_BLAMP(newPhase, 0.75f, +k, samples, samplesTillNextSample);  // corner: -4 -> +4 (after folding sign)
//
//    // Additional knees introduced by folding where |tri| == threshold
//    if (mThreshold < 0.999f)
//    {
//      float th = mThreshold;
//      float t1 = th * 0.25f;
//      float t2 = 0.5f - th * 0.25f;
//      float t3 = 0.5f + th * 0.25f;
//      float t4 = 1.0f - th * 0.25f;
//      // Correct signs for slope jumps at knees:
//      // t1: inside -> folded: -4 -> +4 => +k
//      // t2: folded -> inside: -4 -> +4 => +k
//      // t3: inside -> folded: +4 -> -4 => -k
//      // t4: folded -> inside: +4 -> -4 => -k
//      OSC_ACCUMULATE_BLAMP(newPhase, t1, +k, samples, samplesTillNextSample);
//      OSC_ACCUMULATE_BLAMP(newPhase, t2, +k, samples, samplesTillNextSample);
//      OSC_ACCUMULATE_BLAMP(newPhase, t3, -k, samples, samplesTillNextSample);
//      OSC_ACCUMULATE_BLAMP(newPhase, t4, -k, samples, samplesTillNextSample);
//    }
//  }
//};
//
//// New: Double pulse (two pulses per cycle)
//struct DoublePulseWaveform : IOscillatorWaveform
//{
//  float mCenter1 = 0.35f;
//  float mCenter2 = 0.65f;
//  float mWidth = 0.08f;  // fixed width per pulse
//
//  virtual void AfterSetParams() override
//  {
//    // shape controls separation between the two pulse centers, keep symmetric around 0.5
//    float sep = math::lerp(0.10f, 0.45f, mShapeA);  // half-distance from 0.5
//    mCenter1 = 0.5f - sep * 0.5f;
//    mCenter2 = 0.5f + sep * 0.5f;
//    mWidth = 0.08f;  // fixed for now
//    float duty = math::clamp01(2.0f * mWidth);
//    float mean = -1.0f + 2.0f * duty;
//    mDCOffset = -mean;  // center around zero
//    mScale = 1.0f;
//  }
//
//  virtual float NaiveSample(float t) override
//  {
//    auto inPulse = [&](float c)
//    {
//      float d = t - c;
//      // wrap shortest distance in 0..1 domain
//      d -= std::floor(d + 0.5f);
//      return std::abs(d) < (mWidth * 0.5f);
//    };
//    bool on = inPulse(mCenter1) || inPulse(mCenter2);
//    return on ? 1.0f : -1.0f;
//  }
//
//  virtual float NaiveSampleSlope(float /*t*/) override
//  {
//    return 0.0f;
//  }
//
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
//  {
//    // Four edges per cycle (two pulses). Handle wrap by duplicating edges +/- 1.
//    auto emitPulseEdges = [&](float c)
//    {
//      float a = c - mWidth * 0.5f;
//      float b = c + mWidth * 0.5f;
//      // rising at a, falling at b
//      float edges[4] = {a, b, a - 1.0f, b - 1.0f};
//      for (int i = 0; i < 4; ++i)
//      {
//        float e = edges[i];
//        if (e < 0.0f)
//          e += 1.0f;
//        // Decide sign: at 'a' we go -1 -> +1 => +1 blepScale; at 'b' we go +1 -> -1 => -1
//        bool isRising = (i % 2) == 0;  // a and a-1
//        float s = isRising ? +1.0f : -1.0f;
//        OSC_ACCUMULATE_BLEP(newPhase, e, s, samples, samplesTillNextSample);
//      }
//    };
//    emitPulseEdges(mCenter1);
//    emitPulseEdges(mCenter2);
//  }
//};
//
//}  // namespace M7
//
//
//}  // namespace WaveSabreCore
