
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct MoogLadderFilter : IFilter
{
  // assumes that lpfs are initialized with LP

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    if ((mSlope != slope) || (mResponse != response) || (cutoffHz != m_cutoffHz) || (reso01 != mReso01))
    {
      mSlope = slope;
      mResponse = response;
      m_cutoffHz = cutoffHz;
      mReso01 = reso01;
      m_k = reso01 * real2(3.88);  // this maps dQControl = 0->1 to 0-4 * 0.97 to avoid clippy self oscillation
      Recalc();
    }
  }

  virtual real ProcessSample(real x) override
  {
    return InlineProcessSample(x);
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    if (circuit != FilterCircuit::Moog)
      return false;
    if (slope != FilterSlope::Slope24dbOct && slope != FilterSlope::Slope48dbOct)
      return false;
    if (response != FilterResponse::Lowpass && response != FilterResponse::Highpass &&
        response != FilterResponse::Bandpass)
      return false;
    return true;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    const double clampedFreq = math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
    const double w = math::gPITimes2d * clampedFreq * Helpers::CurrentSampleRateRecipF;

    MoogLadderFilter copy = *this;
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
    for (auto& lpf : m_LPF)
    {
      lpf.Reset();
    }
  }

  inline float InlineProcessSample(float xn)
  {
    real2 dSigma = 0;
    for (auto& lpf : m_LPF)
    {
      dSigma += lpf.getFeedbackOutputL();
    }

    // calculate input to first filter
    real2 dLP[5];
    dLP[0] = (xn - m_k * dSigma) * m_alpha_0;

    static constexpr int8_t letterVals[6][5] = {
        {0, 0, 1, 0, 0},    // lp2
        {0, 0, 0, 0, 1},    // lp4
        {1, -2, 1, 0, 0},   // hp2
        {1, -4, 6, -4, 1},  // hp4
        {0, 2, -2, 0, 0},   // bp2
        {0, 0, 4, -8, 4},   // bp4
    };

    static_assert((size_t)FilterResponse::Lowpass == 0, "filter type enum values must match letterVals table");
    static_assert((size_t)FilterResponse::Highpass == 1, "filter type enum values must match letterVals table");
    static_assert((size_t)FilterResponse::Bandpass == 2, "filter type enum values must match letterVals table");
    size_t filterTypeIndex = (size_t)mResponse * 2 + ((mSlope == FilterSlope::Slope48dbOct) ? 1 : 0);

    // --- cascade of 4 filters
    real2 output = 0;
    for (size_t i = 0; i <= 4; ++i)
    {
      if (i < 4)
        dLP[i + 1] = real2(m_LPF[i].ProcessSample(float(dLP[i])));
      output += dLP[i] * letterVals[filterTypeIndex][i];
    }
    //output += dLP[4] * letterVals[(size_t)m_FilterType][4];

    return float(output);
  }

private:
  void Recalc()
  {
    // prewarp for BZT
    //real wd = PITimes2 * m_cutoffHz;

    // note: measured input to tan function, it seemed limited to (0.005699, 1.282283).
    // input for fasttan shall be limited to (-pi/2, pi/2) according to documentation
    //real wa = (2 * Helpers::CurrentSampleRateF) * math::tan(wd * Helpers::CurrentSampleRateRecipF * Real(0.5));
    //real g = wa * Helpers::CurrentSampleRateRecipF * Real(0.5);

    real2 cutoff = math::clamp(m_cutoffHz, 30, 20000);

    real2 g = math::tan(real2(cutoff) * math::gPI * Helpers::CurrentSampleRateRecipF);

    // G - the feedforward coeff in the VA One Pole
    //     same for LPF, HPF
    const real2 oneOver1plusg = real2(1) / (g + 1);
    real2 G = g * oneOver1plusg;

    m_gamma = 1;
    for (int i = 3; i >= 0; --i)
    {
      m_LPF[i].m_alpha = G;
      m_LPF[i].m_beta = m_gamma * oneOver1plusg;
      m_gamma *= G;
    }

    m_alpha_0 = real2(1) / (m_k * m_gamma + 1);
  }

  MoogOnePoleFilter m_LPF[4];

  FilterSlope mSlope =
      (FilterSlope)-1;  // = FilterSlope::Slope24dbOct; initialize to some invaliid value to force an initial recalc.
  FilterResponse mResponse =
      (FilterResponse)-1;  // = FilterResponse::Lowpass; initialize to some invaliid value to force an initial recalc.

  real2 m_alpha_0;  // = 1; // see block diagram
  real2 m_k;        /// = 0;       // K, set with Q
  real2 m_gamma;    // = 0;       // see block diagram

  // cached params
  real m_cutoffHz;  // = 0;
  real mReso01;     // = 0;// Real(-1); // cached resonance (0..1) for knowing when recalc is not needed.
};
}  // namespace M7
}  // namespace WaveSabreCore
