// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com
// this is an effective one-pass filter but also supports some features
// used by diode/moog/et al filters,
// like feedback, ...

#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Basic/Helpers.h"
#include "FilterBase.hpp"


namespace WaveSabreCore
{
namespace M7
{
struct MoogOnePoleFilter : public IFilter
{
  virtual void SetParams(
    // FilterCircuit circuit,
    //                      FilterSlope slope,
                         FilterResponse response,
                         float cutoffHz,
                         Param01 reso01,
                         real gainDb) override;

  virtual void Reset() override;
  real2 getFeedbackOutputL();

  virtual float ProcessSample(float xn__) override;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    auto clone = std::make_unique<MoogOnePoleFilter>();
    clone->mResponse = this->mResponse;
    clone->m_cutoffHz = this->m_cutoffHz;
    clone->m_alpha = this->m_alpha;
    clone->m_beta = this->m_beta;
    clone->m_gamma = this->m_gamma;
    clone->m_delta = this->m_delta;
    clone->m_epsilon = this->m_epsilon;
    return clone;
  }
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    if (circuit != FilterCircuit::OnePole)
      return false;
    if (slope != FilterSlope::Slope6dbOct)
      return false;
    switch (response)
    {
      case FilterResponse::Lowpass:
      case FilterResponse::Highpass:
      case FilterResponse::Allpass:
        return true;
    }
    return false;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    if (mResponse == FilterResponse::Allpass)
    {
      return 1.0f;
    }

    const double clampedFreq = math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
    const double w = math::gPITimes2d * clampedFreq * Helpers::CurrentSampleRateRecipF;
    const double cw = math::cos(w);
    const double sw = math::sin(w);

    const double alpha = double(m_alpha);
    const double p = 1.0 - alpha;
    const double den2 = (1.0 - p * cw) * (1.0 - p * cw) + (p * sw) * (p * sw);
    const double safeDen2 = (den2 > 1e-20) ? den2 : 1e-20;

    const double hlpRe = alpha * (1.0 - p * cw) / safeDen2;
    const double hlpIm = -alpha * p * sw / safeDen2;

    if (mResponse == FilterResponse::Lowpass)
    {
      return real(math::sqrt(hlpRe * hlpRe + hlpIm * hlpIm));
    }

    const double hhpRe = 1.0 - hlpRe;
    const double hhpIm = -hlpIm;
    return real(math::sqrt(hhpRe * hhpRe + hhpIm * hhpIm));
  }

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  // all get written to directly by other filters; so yea.
  real2 m_alpha = 1;  // Feed Forward coeff
  real2 m_beta = 0;
  real2 m_gamma = 1;      // Pre-Gain
  real2 m_delta = 0;      // FB_IN Coeff
  real2 m_epsilon = 0;    // FB_OUT scalar
  real2 m_a_0 = 1;        // input gain
  real2 m_feedbackL = 0;  // our own feedback coeff from S ..... this is written to by DiodeFilter

private:
  FilterResponse mResponse = FilterResponse::Lowpass;
  float m_cutoffHz = 10000;

  real2 m_z_1L = 0;  // z-1 storage location, left/mono

  void Recalc();
};


class VanillaOnePoleFilter : public IFilter
{
public:
  virtual void SetParams(
                         FilterResponse response,
                         float cutoffHz,
                         Param01 reso01, // ignored,
                         float gainDb // ignored
                        ) override
    {
      if (response == mResponse && cutoffHz == m_cutoffHz)
        return;
      mResponse = response;
      m_cutoffHz = cutoffHz;
      float g = CalculateFilterG(cutoffHz);
      m_a0 = 1.0f - g;
      m_b1 = g;
    }

  virtual void Reset() override
  {
    m_a0 = 1.0f;
    m_b1 = 0.0f;
    m_z_1 = 0.0f;
  }

virtual float ProcessSample(float xn) override
{
   float lp = m_a0 * xn + m_b1 * m_z_1;
   m_z_1 = lp;

   switch (mResponse)
   {
      default:
      case FilterResponse::Lowpass:
         return lp;
      case FilterResponse::Highpass:
         return xn - lp;
   }
}
  private:
  FilterResponse mResponse;
  float m_cutoffHz;
  real2 m_a0 = 1;
  real2 m_b1 = 0;
  real2 m_z_1 = 0;  // z-1 storage location
};


}  // namespace M7
}  // namespace WaveSabreCore
