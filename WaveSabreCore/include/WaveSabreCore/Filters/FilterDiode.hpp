
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct DiodeFilter : IFilter
{
  MoogOnePoleFilter mLPF[4];

  FilterSlope mSlope = (FilterSlope)-1;
  FilterResponse mResponse = (FilterResponse)-1;

  real2 mCutoffHz = 0;
  real2 mReso01 = 0.0f;

  real2 mK = 0.0f;
  real2 mAlpha0 = 1.0f;

  inline void Recalc()
  {
    const real2 cutoff = math::clamp(mCutoffHz, 20.0f, 20000.0f);
    const real2 g = math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
    const real2 oneOver1plusg = real2(1) / (g + 1);
    const real2 G = g * oneOver1plusg;

    const real2 g0 = real2(1);
    const real2 g1 = G;
    const real2 g2 = G;
    const real2 g3 = G;

    const real2 gamma = g0 * g1 * g2 * g3;

    mLPF[3].m_alpha = G;
    mLPF[3].m_beta = oneOver1plusg;

    mLPF[2].m_alpha = G;
    mLPF[2].m_beta = G * oneOver1plusg;

    mLPF[1].m_alpha = G;
    mLPF[1].m_beta = G * G * oneOver1plusg;

    mLPF[0].m_alpha = G;
    mLPF[0].m_beta = G * G * G * oneOver1plusg;

    mK = math::clamp01(mReso01) * 16.0f;
    mAlpha0 = real2(1) / (real2(1) + mK * gamma);
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
    if (response != FilterResponse::Lowpass && response != FilterResponse::Highpass &&
        response != FilterResponse::Bandpass)
      return false;
    return true;
  }

  inline real ProcessCore(real x)
  {
    const real2 s1 = mLPF[0].getFeedbackOutputL();
    const real2 s2 = mLPF[1].getFeedbackOutputL();
    const real2 s3 = mLPF[2].getFeedbackOutputL();
    const real2 s4 = mLPF[3].getFeedbackOutputL();

    const real2 sigma = s1 + s2 + s3 + s4;
    real2 u = (x - mK * sigma) * mAlpha0;
    // if you want to add saturation, do it here and before each filter stage.
    real2 y1 = mLPF[0].ProcessSample((float)u);
    real2 y2 = mLPF[1].ProcessSample((float)y1);
    real2 y3 = mLPF[2].ProcessSample((float)y2);
    real2 y4 = mLPF[3].ProcessSample((float)y3);

    switch (mResponse)
    {
      default:
      case FilterResponse::Lowpass:
        return (real)y4;
      case FilterResponse::Highpass:
        return (real)(x - real2(4) * y1 + real2(6) * y2 - real2(4) * y3 + y4);
      case FilterResponse::Bandpass:
        return (real)(real2(4) * (y1 - real2(3) * y2 + real2(3) * y3 - y4));
    }
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
    for (auto& s : mLPF)
    {
      s.Reset();
    }
  }
}; // class DiodeFilter
}  // namespace M7
}  // namespace WaveSabreCore
