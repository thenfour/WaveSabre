#ifndef __WAVESABRECORE_BIQUADFILTER_H__
#define __WAVESABRECORE_BIQUADFILTER_H__

#include "WaveSabreCore/Filters/FilterBase.hpp"
#include "WaveSabreCore/Maj7Basic.hpp"
#include "WaveSabreCore/Maj7ParamAccessor.hpp"
#include "LUTs.hpp"

namespace WaveSabreCore
{
enum class BiquadFilterType
{
  Lowpass,
  Highpass,
  Bandpass,
  Notch,
  Allpass,
  Peak,
  HighShelf,
  LowShelf,
};

class BiquadFilter : public M7::IFilter
{
public:
  BiquadFilter();

  //float Next(float input);

  // q is ~0 to ~12
  void SetParams(BiquadFilterType type, float freq, float q, float gain);

  //bool thru;

  BiquadFilterType type;
  float freq;
  float q;
  float gain;

  //float ma0, ma1, ma2, mb0, mb1, mb2; // store a0 so we can calculate the coeffs from c12345
  //float c1, c2, c3, c4, c5;
  float coeffs[6];  // a0, a1, a2, b0, b1, b2

  // map to normalized against a0:
  // a0, a1, a2, b0, b1, b2 but all divided by a0. so normCoeffs[0] is just 1.
  // 0   1   2   3   4   5
  float normCoeffs[6];

  float lastInput, lastLastInput;
  float lastOutput, lastLastOutput;

  // IFilter
  virtual void SetParams(M7::FilterType type, float cutoffHz, float reso01) override
  {
    BiquadFilterType bt;
    switch (type)
    {
      case M7::FilterType::HP2:
      case M7::FilterType::HP4:
        bt = BiquadFilterType::Highpass;
        break;
      default:
      case M7::FilterType::LP2:
      case M7::FilterType::LP4:
        bt = BiquadFilterType::Lowpass;
        break;
    }

    M7::ParamAccessor pa{&reso01, 0};
    float q = pa.GetDivCurvedValue(0, M7::gBiquadFilterQCfg, 0);

    SetParams(bt, cutoffHz, q, q);
  }
  // IFilter
  virtual float ProcessSample(float x) override;

  // IFilter
  virtual void Reset() override
  {
    lastInput = lastLastInput = 0.0f;
    lastOutput = lastLastOutput = 0.0f;
  }
  
  static float GetCompensationGainLinearBP(float cutoffHz, float q)
  {
    const float fRef = 1000.0f;  // reference frequency where gain is ~1
    const float qRef = 0.707f;
    // clamping is already done upstream. don't sanitize/clamp values!
    float g = M7::math::sqrt(fRef / cutoffHz) * M7::math::sqrt(q / qRef);
    g = M7::math::clamp(g, 0.25f, 8.0f);
    return g;
  }

  static float GetCompensationGainLinearLP(float cutoffHz)
  {
    const float fRef = 1000.0f;  // reference frequency where gain is ~1
    // clamping is already done upstream. don't sanitize/clamp values!
    float g = M7::math::sqrt(fRef / cutoffHz);
    g = M7::math::clamp(g, 0.25f, 8.0f);
    return g;
  }

  static float GetCompensationGainLinearHP(float cutoffHz)
  {
    const float nyquistHz = 0.5f * Helpers::CurrentSampleRateF;
    const float fRef = 1000.0f;
    const float refRemaining = std::max(10.0f, nyquistHz - fRef);
    const float remaining = std::max(10.0f, nyquistHz - cutoffHz);
    float g = M7::math::sqrt(refRemaining / remaining);
    g = M7::math::clamp(g, 0.25f, 8.0f);
    return g;
  }

  float GetCompensationGainLinear() const
  {
	switch (type)
	{
	  case BiquadFilterType::Lowpass:
		return GetCompensationGainLinearLP(freq);
	  case BiquadFilterType::Highpass:
		return GetCompensationGainLinearHP(freq);
	  case BiquadFilterType::Bandpass:
		return GetCompensationGainLinearBP(freq, q);
	  default:
		return 1.0f;
	}
  }

  // Returns linear magnitude at a frequency in Hz using current normalized coefficients
  float GetMagnitudeAtFrequency(float freqHz) const;
};


class CascadedBiquadFilter
{
  // 1 stage's slope = 12db/oct.
  // 2 stage = 24db/oct.
  // 3 stage = 36db/oct
  // 4 stage = 48db/oct.
  BiquadFilter mFilters[4];
  size_t mNStages;

public:
  CascadedBiquadFilter(size_t nStages)
      : mNStages(nStages)
  {
    CCASSERT(nStages >= 1 && nStages <= 4);
  }

  void SetParams(BiquadFilterType type, float freq, float q, float gain)
  {
    for (size_t i = 0; i < mNStages; ++i)
    {
      mFilters[i].SetParams(type, freq, q, gain);
    }
  }
  float ProcessSample(float x)
  {
    float y = x;
    for (size_t i = 0; i < mNStages; ++i)
    {
      y = mFilters[i].ProcessSample(y);
    }
    return y;
  }
  void Reset()
  {
    for (size_t i = 0; i < mNStages; ++i)
    {
      mFilters[i].Reset();
    }
  }
  float GetCompensationGainLinear() const
  {
	// this is a bit of a hack but it works well enough in practice. we just multiply the compensation gains of each stage together, which is close enough to the actual compensation gain of the cascaded filter.
	float g = 1.0f;
	for (size_t i = 0; i < mNStages; ++i)
	{
	  g *= mFilters[i].GetCompensationGainLinear();
	}
	return g;
  }
}; // class CascadedBiquadFilter
}  // namespace WaveSabreCore


#endif
