
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
#ifdef ENABLE_K35_FILTER
struct K35Filter : IFilter
{
  MoogOnePoleFilter mLPF1;
  MoogOnePoleFilter mLPF2;
  MoogOnePoleFilter mHPF1;
  MoogOnePoleFilter mHPF2;

  FilterSlope mSlope = (FilterSlope)-1;
  FilterResponse mResponse = (FilterResponse)-1;

  real2 mCutoffHz = 1000.0f;
  real2 mReso01 = 0.0f;

  real2 mK = 0.01f;
  real2 mAlpha = 1.0f;

  inline void ConfigureOnePolesForCurrentResponse(real2 cutoff)
  {
    mLPF1.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
    mLPF2.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);
    mHPF1.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Highpass, cutoff, 0);
    mHPF2.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Highpass, cutoff, 0);

    auto resetOnePoleScalars = [](MoogOnePoleFilter& f)
    {
      f.m_delta = 0;
      f.m_gamma = 1;
      f.m_epsilon = 0;
      f.m_a_0 = 1;
      f.m_feedbackL = 0;
    };

    resetOnePoleScalars(mLPF1);
    resetOnePoleScalars(mLPF2);
    resetOnePoleScalars(mHPF1);
    resetOnePoleScalars(mHPF2);
  }

  inline void Recalc()
  {
    //const real2 cutoff = math::clamp(mCutoffHz, 20.0f, 20000.0f);
    const real2 g = CalculateFilterG(cutoff);  // math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
    const real2 G = g / (real2(1) + g);

    ConfigureOnePolesForCurrentResponse(cutoff);

    mLPF1.m_alpha = G;
    mLPF2.m_alpha = G;
    mHPF1.m_alpha = G;
    mHPF2.m_alpha = G;

    mK = math::clamp(mReso01 * real2(1.95f) + real2(0.01f), real2(0.01f), real2(1.96f));

    const real2 denom = real2(1) - mK * G + mK * G * G;
    const real2 absDenom = (denom < real2(0)) ? -denom : denom;
    const real2 safeDenom = (absDenom < real2(1e-9f)) ? real2(1e-9f) : denom;
    mAlpha = real2(1) / safeDenom;

    mLPF1.m_beta = 0;
    mLPF2.m_beta = 0;
    mHPF1.m_beta = 0;
    mHPF2.m_beta = 0;

    if (mResponse == FilterResponse::Highpass)
    {
      mHPF2.m_beta = -G / (real2(1) + g);
      mLPF1.m_beta = real2(1) / (real2(1) + g);
    }
    else
    {
      mLPF2.m_beta = (mK - mK * G) / (real2(1) + g);
      mHPF1.m_beta = -real2(1) / (real2(1) + g);
    }
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

  inline real ProcessCore(real x)
  {
    real2 y = 0;
    if (mResponse == FilterResponse::Highpass)
    {
      real2 y1 = mHPF1.ProcessSample(x);
      const real2 s35 = mHPF2.getFeedbackOutputL() + mLPF1.getFeedbackOutputL();
      const real2 u = mAlpha * (y1 + s35);

      y = mK * u;
      const real2 h2 = mHPF2.ProcessSample((float)y);
      mLPF1.ProcessSample((float)h2);
    }
    else
    {
      real2 y1 = mLPF1.ProcessSample(x);
      const real2 s35 = mLPF2.getFeedbackOutputL() + mHPF1.getFeedbackOutputL();
      const real2 u = mAlpha * (y1 + s35);

      y = mK * mLPF2.ProcessSample((float)u);
      mHPF1.ProcessSample((float)y);
    }

    return (real)(y / mK);
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

  #ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    auto clone = std::make_unique<K35Filter>();
    clone->mSlope = this->mSlope;
    clone->mResponse = this->mResponse;
    clone->mCutoffHz = this->mCutoffHz;
    clone->mReso01 = this->mReso01;
    clone->Recalc();
    return clone;
  }
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    if (circuit != FilterCircuit::K35)
      return false;
    if (slope != FilterSlope::Slope12dbOct && slope != FilterSlope::Slope24dbOct)
      return false;
    if (response != FilterResponse::Lowpass && response != FilterResponse::Highpass)
      return false;
    return true;
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
      re += y * math::cos(phase);
      im -= y * math::sin(phase);
    }

    return real(math::sqrt(re * re + im * im));
  }

  #endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void Reset() override
  {
    mLPF1.Reset();
    mLPF2.Reset();
    mHPF1.Reset();
    mHPF2.Reset();
  }
};  // class K35Filter

#else   // ENABLE_K35_FILTER
using K35Filter = NullFilter;
#endif  // ENABLE_K35_FILTER

}  // namespace M7
}  // namespace WaveSabreCore
