
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct DiodeFilter : IFilter
{
  MoogOnePoleFilter mLPF1;
  MoogOnePoleFilter mLPF2;
  MoogOnePoleFilter mLPF3;
  MoogOnePoleFilter mLPF4;

  FilterSlope mSlope = (FilterSlope)-1;
  FilterResponse mResponse = (FilterResponse)-1;

  real2 mCutoffHz = 0;
  real2 mReso01 = 0.0f;

  real2 mK = 0.0f;
  real2 mGamma = 0.0f;
  real2 mSg1 = 0.0f;
  real2 mSg2 = 0.0f;
  real2 mSg3 = 0.0f;
  real2 mSg4 = 0.0f;

  inline void ConfigureOnePolesForLowpass(real2 cutoff)
  {
    mLPF1.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
    mLPF2.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
    mLPF3.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
    mLPF4.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
  }

  inline void Recalc()
  {
    const real2 cutoff = math::clamp(mCutoffHz, 20.0f, 20000.0f);
    const real2 g = math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
    const real2 G4 = real2(0.5f) * g / (real2(1) + g);
    const real2 G3 = real2(0.5f) * g / (real2(1) + g - real2(0.5f) * g * G4);
    const real2 G2 = real2(0.5f) * g / (real2(1) + g - real2(0.5f) * g * G3);
    const real2 G1 = g / (real2(1) + g - g * G2);

    mGamma = G4 * G3 * G2 * G1;
    mSg1 = G4 * G3 * G2;
    mSg2 = G4 * G3;
    mSg3 = G4;
    mSg4 = real2(1);

    const real2 G = g / (real2(1) + g);

    ConfigureOnePolesForLowpass(cutoff);

    mLPF1.m_alpha = G;
    mLPF2.m_alpha = G;
    mLPF3.m_alpha = G;
    mLPF4.m_alpha = G;

    mLPF1.m_beta = real2(1) / (real2(1) + g - g * G2);
    mLPF2.m_beta = real2(1) / (real2(1) + g - real2(0.5f) * g * G3);
    mLPF3.m_beta = real2(1) / (real2(1) + g - real2(0.5f) * g * G4);
    mLPF4.m_beta = real2(1) / (real2(1) + g);

    mLPF1.m_delta = g;
    mLPF2.m_delta = real2(0.5f) * g;
    mLPF3.m_delta = real2(0.5f) * g;
    mLPF4.m_delta = 0;

    mLPF1.m_gamma = real2(1) + G1 * G2;
    mLPF2.m_gamma = real2(1) + G2 * G3;
    mLPF3.m_gamma = real2(1) + G3 * G4;
    mLPF4.m_gamma = real2(1);

    mLPF1.m_epsilon = G2;
    mLPF2.m_epsilon = G3;
    mLPF3.m_epsilon = G4;
    mLPF4.m_epsilon = 0;

    mLPF1.m_a_0 = real2(1);
    mLPF2.m_a_0 = real2(0.5f);
    mLPF3.m_a_0 = real2(0.5f);
    mLPF4.m_a_0 = real2(0.5f);

    mK = math::clamp01(mReso01) * 16.0f;
  }

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    const real2 clampedReso = math::clamp01(reso01);
    if ((mSlope != slope) || (mResponse != response) || (mCutoffHz != cutoffHz) || (mReso01 != clampedReso))
    {
      mSlope = slope;
      mResponse = response;
      mCutoffHz = cutoffHz;
      mReso01 = clampedReso;
      Recalc();
    }
  }

  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    if (circuit != FilterCircuit::Diode)
      return false;
    if (slope != FilterSlope::Slope24dbOct && slope != FilterSlope::Slope48dbOct)
      return false;
    if (response != FilterResponse::Lowpass)
      return false;
    return true;
  }

  inline real ProcessCore(real x)
  {
    mLPF4.m_feedbackL = 0;
    mLPF3.m_feedbackL = mLPF4.getFeedbackOutputL();
    mLPF2.m_feedbackL = mLPF3.getFeedbackOutputL();
    mLPF1.m_feedbackL = mLPF2.getFeedbackOutputL();

    const real2 sigma = mSg1 * mLPF1.getFeedbackOutputL() + mSg2 * mLPF2.getFeedbackOutputL() +
                        mSg3 * mLPF3.getFeedbackOutputL() + mSg4 * mLPF4.getFeedbackOutputL();

    const real2 kModded = math::clamp(mK, real2(0), real2(16));

    x *= real2(1) + real2(0.3f) * kModded;
    x = (x - kModded * sigma) / (real2(1) + kModded * mGamma);

    x = mLPF1.ProcessSample((float)x);
    x = mLPF2.ProcessSample((float)x);
    x = mLPF3.ProcessSample((float)x);
    x = mLPF4.ProcessSample((float)x);

    return x;
  }

  virtual real ProcessSample(real x) override
  {
    real y = ProcessCore(x);
    if (mSlope == FilterSlope::Slope48dbOct)
    {
      y = ProcessCore(y);
    }
    return y;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    const double clampedFreq = math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
    const double w = math::gPITimes2d * clampedFreq * Helpers::CurrentSampleRateRecipF;

    DiodeFilter copy = *this;
    copy.Reset();

    static constexpr int kImpulseTaps = 256;
    double re = 0.0;
    double im = 0.0;
    for (int n = 0; n < kImpulseTaps; ++n)
    {
      const float x = (n == 0) ? 1.0f : 0.0f;
      const double y = copy.ProcessSample(x);
      const double phase = w * double(n);
      re += y * math::CrtCos(phase);
      im -= y * math::CrtSin(phase);
    }

    return math::sqrt(float(re * re + im * im));
  }

  virtual void Reset() override
  {
    mLPF1.Reset();
    mLPF2.Reset();
    mLPF3.Reset();
    mLPF4.Reset();
  }
}; // class DiodeFilter
}  // namespace M7
}  // namespace WaveSabreCore
