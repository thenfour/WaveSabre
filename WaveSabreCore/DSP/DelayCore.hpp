#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Filters/Maj7Filter.hpp"
#include "../DSP/DelayBuffer.h"

namespace WaveSabreCore::M7
{
struct DelayCore
{
private:
  DelayBuffer mBuffers[2];
  BiquadFilter mLowCutFilter[2];
  BiquadFilter mHighCutFilter[2];
  float mFeedbackDriveGainCompensationFact;

public:
  static constexpr IntParamConfig gDelayCoarseCfg{0, 48};

  float mFeedbackLin;
  float mFeedbackDriveLin;

  float mCrossMix;

  FloatPair Run(const FloatPair& dry)
  {
    // read buffer, apply processing (filtering, cross mix, drive)
    FloatPair delayBufferSignal = {mBuffers[0].ReadSample(), mBuffers[1].ReadSample()};

    delayBufferSignal[0] = mHighCutFilter[0].ProcessSample(delayBufferSignal[0]);
    delayBufferSignal[1] = mHighCutFilter[1].ProcessSample(delayBufferSignal[1]);

    delayBufferSignal[0] = mLowCutFilter[0].ProcessSample(delayBufferSignal[0]);
    delayBufferSignal[1] = mLowCutFilter[1].ProcessSample(delayBufferSignal[1]);

    // apply drive
    delayBufferSignal[0] = math::tanh(delayBufferSignal[0] * mFeedbackDriveLin) *
                           mFeedbackDriveGainCompensationFact;
    delayBufferSignal[1] = math::tanh(delayBufferSignal[1] * mFeedbackDriveLin) *
                           mFeedbackDriveGainCompensationFact;

    // cross mix
    delayBufferSignal = {math::lerp(delayBufferSignal[0], delayBufferSignal[1], mCrossMix),
                         math::lerp(delayBufferSignal[1], delayBufferSignal[0], mCrossMix)};

    mBuffers[0].WriteSample(dry[0] + delayBufferSignal[0] * mFeedbackLin);
    mBuffers[1].WriteSample(dry[1] + delayBufferSignal[1] * mFeedbackLin);

    return {delayBufferSignal[0], delayBufferSignal[1]};
  }

private:
  static float CalcDelayMS(int eighthsI, float eighths01, float msParam)
  {
    // 60000/bpm = milliseconds per beat. but we are going to be in 8 divisions per beat.
    // 60000/8 = 7500
    float eighths = eighths01 + eighthsI;
    float ms = 7500.0f / Helpers::CurrentTempo * eighths;
    ms += msParam;
    return std::max(0.0f, ms);
  }

public:
  static float CalcDelayMS(const ParamAccessor& params,
                           int eighthsIntParamOffset)
  {
    return DelayCore::CalcDelayMS(params.GetIntValue(eighthsIntParamOffset),
                                  params.GetN11Value(eighthsIntParamOffset + 1, 0),
                                  params.GetBipolarPowCurvedValue(eighthsIntParamOffset + 2, gEnvTimeCfg, 0));
  }

  //virtual void SetParam(int index, float value) override
  void OnParamsChanged(float leftBufferLengthMs,
                       float rightBufferLengthMs,
                       float lowCutFreqHz,
                       Decibels lowCutQ,
                       float highCutFreqHz,
                       Decibels highCutQ)
  {
    mFeedbackDriveGainCompensationFact = math::CalcTanhGainCompensation(mFeedbackDriveLin);

    mBuffers[0].SetLength(leftBufferLengthMs);

    mBuffers[1].SetLength(rightBufferLengthMs);

    for (int i = 0; i < 2; i++)
    {
      mLowCutFilter[i].SetBiquadParams(FilterResponse::Highpass,
                                 lowCutFreqHz,
                                 lowCutQ,
                                 0  // gain
      );
      mHighCutFilter[i].SetBiquadParams(FilterResponse::Lowpass,
                                  highCutFreqHz,
                                  highCutQ,
                                  0  // gain
      );
    }
  }
};
}  // namespace WaveSabreCore::M7
