
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
                         Param01 reso01, real gainDb) override;

  virtual real ProcessSample(real x) override;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    auto clone = std::make_unique<MoogLadderFilter>();
    clone->mSlope = this->mSlope;
    clone->mResponse = this->mResponse;
    clone->m_cutoffHz = this->m_cutoffHz;
    clone->mReso01 = this->mReso01;
    clone->m_k = this->m_k;
    clone->Recalc();
    return clone;
  }
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

  virtual void Reset() override;
private:
  void Recalc();

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
