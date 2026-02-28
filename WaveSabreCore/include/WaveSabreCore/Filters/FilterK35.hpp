
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct K35Filter : IFilter
{
  MoogOnePoleFilter mLPF[2];

  FilterSlope mSlope = (FilterSlope)-1;
  FilterResponse mResponse = (FilterResponse)-1;

  real2 mCutoffHz = 1000.0f;
  real2 mReso01 = 0.0f;

  real2 mK = 0.0f;
  real2 mAlpha0 = 1.0f;
  real2 mGamma = 0.0f;

  inline void Recalc()
  {
    const real2 cutoff = math::clamp(mCutoffHz, 20.0f, 20000.0f);
    const real2 g = math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
    const real2 oneOver1plusg = real2(1) / (g + 1);
    const real2 G = g * oneOver1plusg;

    mGamma = G * G;

    mLPF[1].m_alpha = G;
    mLPF[1].m_beta = oneOver1plusg;

    mLPF[0].m_alpha = G;
    mLPF[0].m_beta = G * oneOver1plusg;

    mK = math::clamp01(mReso01) * 3.6f;
    mAlpha0 = real2(1) / (real2(1) + mK * mGamma);
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
    if (circuit != FilterCircuit::K35)
      return false;
    if (slope != FilterSlope::Slope12dbOct && slope != FilterSlope::Slope24dbOct)
      return false;
    if (response != FilterResponse::Lowpass && response != FilterResponse::Highpass &&
        response != FilterResponse::Bandpass)
      return false;
    return true;
  }

  inline real ProcessCore(real x)
  {
    real2 sigma = mLPF[0].getFeedbackOutputL() + mLPF[1].getFeedbackOutputL();
    real2 u = (x - mK * sigma) * mAlpha0;

    real2 s1 = mLPF[0].ProcessSample((float)u);
    real2 s2 = mLPF[1].ProcessSample((float)s1);

    switch (mResponse)
    {
      default:
      case FilterResponse::Lowpass:
        return (real)s2;
      case FilterResponse::Highpass:
        return (real)(x - real2(2) * s1 + s2);
      case FilterResponse::Bandpass:
        return (real)(real2(2) * (s1 - s2));
    }
  }

  virtual real ProcessSample(real x) override
  {
    real y = ProcessCore(x);
    if (mSlope == FilterSlope::Slope24dbOct)
    {
      y = ProcessCore(y);
    }
    return y;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    const double clampedFreq = math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
    const double w = math::gPITimes2d * clampedFreq * Helpers::CurrentSampleRateRecipF;

    K35Filter copy = *this;
    copy.Reset();

    static constexpr int kImpulseTaps = 192;
    double re = 0.0;
    double im = 0.0;
    for (int n = 0; n < kImpulseTaps; ++n)
    {
      const float x = (n == 0) ? 1.0f : 0.0f;
      const double y = copy.ProcessSample(x);
      const double phase = w * double(n);
      re += y * std::cos(phase);
      im -= y * std::sin(phase);
    }

    return real(std::sqrt(re * re + im * im));
  }

  virtual void Reset() override
  {
    for (auto& s : mLPF)
    {
      s.Reset();
    }
  }
}; // class K35Filter
}  // namespace M7
}  // namespace WaveSabreCore
