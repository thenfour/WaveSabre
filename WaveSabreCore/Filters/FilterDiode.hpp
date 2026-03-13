
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
#ifdef ENABLE_DIODE_FILTER

struct DiodeFilter : IFilter
{
  static constexpr size_t kStages = 4;
  MoogOnePoleFilter mLPF[4];

  FilterSlope mSlope = (FilterSlope)-1;
  FilterResponse mResponse = (FilterResponse)-1;

  real2 mCutoffHz = 0;
  real2 mReso01 = 0.0f;

  real2 mK = 0;
  real2 mGamma = 0;
  real2 mSg[kStages];  // sigma weights instead of mSg1..4

  inline void Recalc()
  {
    //const real2 cutoff = math::clamp(mCutoffHz, 20.0f, 20000.0f);
    const real2 g = CalculateFilterG(cutoff);  // math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
    const real2 halfG = real2(0.5f) * g;
    const real2 gPlus1 = real2(1) + g;

    // G[3] = G4, G[2] = G3, G[1] = G2, G[0] = G1
    real2 G[5];
    G[4] = 0;  // used later when applying epsilon.
    G[3] = halfG / gPlus1;
    G[2] = halfG / (gPlus1 - halfG * G[3]);
    G[1] = halfG / (gPlus1 - halfG * G[2]);
    G[0] = g / (gPlus1 - g * G[1]);

    mSg[3] = real2(1);       // 1
    mSg[2] = G[3];           // G4
    mSg[1] = G[3] * G[2];    // G4 * G3
    mSg[0] = mSg[1] * G[1];  // G4 * G3 * G2

    mGamma = G[0] * mSg[0];

    const real2 GonePole = g / gPlus1;

    for (size_t i = 0; i < kStages; i++)
    {
      auto& s = mLPF[i];
      s.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Lowpass, cutoff, 0);

      s.m_alpha = GonePole;
      s.m_delta = halfG;
      s.m_a_0 = real2(0.5);
      s.m_epsilon = G[i + 1];
    }

    // Stage-specific tweaks
    mLPF[0].m_beta = real2(1) / (gPlus1 - g * G[1]);
    mLPF[1].m_beta = real2(1) / (gPlus1 - halfG * G[2]);
    mLPF[2].m_beta = real2(1) / (gPlus1 - halfG * G[3]);
    mLPF[3].m_beta = real2(1) / gPlus1;

    mLPF[0].m_delta = g;
    // [1],[2] set above
    mLPF[3].m_delta = real2(0);

    mLPF[0].m_gamma = real2(1) + G[0] * G[1];
    mLPF[1].m_gamma = real2(1) + G[1] * G[2];
    mLPF[2].m_gamma = real2(1) + G[2] * G[3];
    mLPF[3].m_gamma = real2(1);

    mLPF[0].m_a_0 = real2(1);

    // mReso01 is already clamped in SetParams
    mK = mReso01 * 16.0f;
  }

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    const real2 clampedReso = math::clamp01(reso01);

    if (mSlope == slope && mResponse == response && mCutoffHz == cutoffHz && mReso01 == clampedReso)
    {
      return;
    }

    mSlope = slope;
    mResponse = response;
    mCutoffHz = cutoffHz;
    mReso01 = clampedReso;

    Recalc();
  }

  inline real ProcessCore(real x)
  {
    mLPF[kStages - 1].m_feedbackL = 0;
    for (int i = kStages - 1; i > 0; --i)
    {
      mLPF[i - 1].m_feedbackL = mLPF[i].getFeedbackOutputL();
    }

    real2 sigma = real2(0);
    for (int i = 0; i < kStages; ++i)
    {
      sigma += mSg[i] * mLPF[i].getFeedbackOutputL();
    }

    const real2 kModded = mK;

    x *= real2(1) + real2(0.3) * kModded;
    x = (x - kModded * sigma) / (real2(1) + kModded * mGamma);

    for (int i = 0; i < kStages; ++i)
    {
      x = mLPF[i].ProcessSample((float)x);
    }

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

  #ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    auto clone = std::make_unique<DiodeFilter>();
    clone->mSlope = this->mSlope;
    clone->mResponse = this->mResponse;
    clone->mCutoffHz = this->mCutoffHz;
    clone->mReso01 = this->mReso01;
    clone->Recalc();
    return clone;
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

  #endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void Reset() override
  {
    for (size_t i = 0; i < kStages; i++)
    {
      mLPF[i].Reset();
    }
  }
};  // class DiodeFilter

#else   // ENABLE_DIODE_FILTER
using DiodeFilter = NullFilter;
#endif  // ENABLE_DIODE_FILTER

}  // namespace M7
}  // namespace WaveSabreCore
