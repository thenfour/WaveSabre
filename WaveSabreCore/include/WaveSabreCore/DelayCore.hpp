#pragma once

#include "BiquadFilter.h"
#include "DelayBuffer.h"
#include "Device.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"
#include "RMS.hpp"

namespace WaveSabreCore
{
struct DelayCore
{
private:
  DelayBuffer mBuffers[2];
  M7::BiquadFilter mLowCutFilter[2];
  M7::BiquadFilter mHighCutFilter[2];
  float mFeedbackDriveGainCompensationFact;

public:
  static constexpr M7::IntParamConfig gDelayCoarseCfg{0, 48};

  float mFeedbackLin;
  float mFeedbackDriveLin;

  //float mDryLin;
  //float mWetLin;
  float mCrossMix;

  M7::FloatPair Run(const M7::FloatPair& dry)
  {
    // read buffer, apply processing (filtering, cross mix, drive)
    M7::FloatPair delayBufferSignal = {mBuffers[0].ReadSample(), mBuffers[1].ReadSample()};

    delayBufferSignal[0] = mHighCutFilter[0].ProcessSample(delayBufferSignal[0]);
    delayBufferSignal[1] = mHighCutFilter[1].ProcessSample(delayBufferSignal[1]);

    delayBufferSignal[0] = mLowCutFilter[0].ProcessSample(delayBufferSignal[0]);
    delayBufferSignal[1] = mLowCutFilter[1].ProcessSample(delayBufferSignal[1]);

    // apply drive
    delayBufferSignal[0] = M7::math::tanh(delayBufferSignal[0] * mFeedbackDriveLin) *
                           mFeedbackDriveGainCompensationFact;
    delayBufferSignal[1] = M7::math::tanh(delayBufferSignal[1] * mFeedbackDriveLin) *
                           mFeedbackDriveGainCompensationFact;

    // cross mix
    delayBufferSignal = {M7::math::lerp(delayBufferSignal[0], delayBufferSignal[1], mCrossMix),
                         M7::math::lerp(delayBufferSignal[1], delayBufferSignal[0], mCrossMix)};

    //M7::FloatPair dry{inputs[0][i], inputs[1][i]};

    mBuffers[0].WriteSample(dry[0] + delayBufferSignal[0] * mFeedbackLin);
    mBuffers[1].WriteSample(dry[1] + delayBufferSignal[1] * mFeedbackLin);

    return {delayBufferSignal[0], delayBufferSignal[1]};

    //return {dry[0] * mDryLin + delayBufferSignal[0] * mWetLin, dry[1] * mDryLin + delayBufferSignal[1] * mWetLin};
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
  static float CalcDelayMS(const M7::ParamAccessor& params,
                           int eighthsIntParamOffset)
  {
    return DelayCore::CalcDelayMS(params.GetIntValue(eighthsIntParamOffset, gDelayCoarseCfg),
                                  params.GetN11Value(eighthsIntParamOffset + 1, 0),
                                  params.GetBipolarPowCurvedValue(eighthsIntParamOffset + 2, M7::gEnvTimeCfg, 0));
  }

  //virtual void SetParam(int index, float value) override
  void OnParamsChanged(float leftBufferLengthMs,
                       float rightBufferLengthMs,
                       float lowCutFreqHz,
                       M7::Decibels lowCutQ,
                       float highCutFreqHz,
                       M7::Decibels highCutQ)
  {
    mFeedbackDriveGainCompensationFact = M7::math::CalcTanhGainCompensation(mFeedbackDriveLin);

    mBuffers[0].SetLength(leftBufferLengthMs);

    mBuffers[1].SetLength(rightBufferLengthMs);

    for (int i = 0; i < 2; i++)
    {
      mLowCutFilter[i].SetBiquadParams(M7::FilterResponse::Highpass,
                                 lowCutFreqHz,
                                 lowCutQ,
                                 0  // gain
      );
      mHighCutFilter[i].SetBiquadParams(M7::FilterResponse::Lowpass,
                                  highCutFreqHz,
                                  highCutQ,
                                  0  // gain
      );
    }
  }
};
}  // namespace WaveSabreCore
