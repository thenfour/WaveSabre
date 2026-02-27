#ifndef __WAVESABRECORE_BIQUADFILTER_H__
#define __WAVESABRECORE_BIQUADFILTER_H__

#include "LUTs.hpp"
#include "WaveSabreCore/Filters/FilterBase.hpp"
#include "WaveSabreCore/Maj7Basic.hpp"
#include "WaveSabreCore/Maj7ParamAccessor.hpp"


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

// POD.
struct BiquadConfig
{
  // q is ~0 to ~12 (or higher if you want; in db)
  void SetParams(BiquadFilterType type, float freq, float q, float gain);

//   float& normA0()
//   {
//     return normCoeffs[0];
//   }
//   float& normA1()
//   {
//     return normCoeffs[1];
//   }
//   float& normA2()
//   {
//     return normCoeffs[2];
//   }
//   float& normB0()
//   {
//     return normCoeffs[3];
//   }
//   float& normB1()
//   {
//     return normCoeffs[4];
//   }
//   float& normB2()
//   {
//     return normCoeffs[5];
//   }

  const float& normA0() const
  {
    return normCoeffs[0];
  }
  const float& normA1() const
  {
    return normCoeffs[1];
  }
  const float& normA2() const
  {
    return normCoeffs[2];
  }
  const float& normB0() const
  {
    return normCoeffs[3];
  }
  const float& normB1() const
  {
    return normCoeffs[4];
  }
  const float& normB2() const
  {
    return normCoeffs[5];
  }

  const float& w0() const
  {
	return mW0;
  }

private:
  BiquadFilterType type;
  float freq;
  float q;
  float gain;

  //float ma0, ma1, ma2, mb0, mb1, mb2; // store a0 so we can calculate the coeffs from c12345
  //float c1, c2, c3, c4, c5;
  float coeffs[6];  // a0, a1, a2, b0, b1, b2
  float mW0;

  // map to normalized against a0:
  // a0, a1, a2, b0, b1, b2 but all divided by a0. so normCoeffs[0] is just 1.
  // 0   1   2   3   4   5
  float normCoeffs[6];
};

class BiquadFilter : public M7::IFilter
{
  BiquadConfig mConfig;
  float lastInput, lastLastInput;
  float lastOutput, lastLastOutput;

public:
  BiquadFilter();

  void SetParams(BiquadFilterType type, float freq, float q, float gain)
  {
    mConfig.SetParams(type, freq, q, gain);
  }

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

  // Copies filter type/params/coefficients from another instance, but preserves this instance's state.
  void CopyParamsAndCoeffsFrom(const BiquadFilter& src)
  {
    this->mConfig = src.mConfig;
  }

  // Compute average |H(e^jw)|^2 across frequency for a biquad
  // H(z) = (b0 + b1 z^-1 + b2 z^-2) / (1 + a1 z^-1 + a2 z^-2)
  //
  // Note: uses linear frequency bins. For �perceptual� matching you could use log bins,
  // but for *energy* in a discrete-time system, linear bins are the right default.
  float AvgMag2() const
  {
    // constexpr int N = 128;  // 64..256 is typical; cost is tiny at param-change rate.
    // double sum = 0.0;

    // for (int k = 0; k < N; ++k)
    // {
    //   // Midpoint sampling avoids w=0 and w=pi exactly
    //   double w = M_PI * (k + 0.5) / N;
    //   double c1 = math::cos(w), s1 = math::sin(w);
    //   double c2 = math::cos(2 * w), s2 = math::sin(2 * w);

    //   // z^-1 = e^{-jw} = c1 - j s1
    //   // z^-2 = e^{-j2w} = c2 - j s2
    //   // num = b0 + b1 z^-1 + b2 z^-2
    //   double num_re = b0 + b1 * c1 + b2 * c2;
    //   double num_im = -b1 * s1 - b2 * s2;

    //   // den = 1 + a1 z^-1 + a2 z^-2
    //   double den_re = 1.0 + a1 * c1 + a2 * c2;
    //   double den_im = -a1 * s1 - a2 * s2;

    //   double num_mag2 = num_re * num_re + num_im * num_im;
    //   double den_mag2 = den_re * den_re + den_im * den_im;

    //   // avoid divide-by-zero in pathological cases
    //   double mag2 = (den_mag2 > 1e-18) ? (num_mag2 / den_mag2) : 0.0;
    //   sum += mag2;
    // }

    // return (float)(sum / N);
  }

  // Gain to achieve a target RMS for a given input noise distribution.
  // If your white noise is uniform in [-1,1], var = 1/3.
  // If it�s normal(0,1), var = 1.
  // If it�s uniform in [-0.5,0.5], var = 1/12, etc.
  // inputVariance:
  //      uniform u ∈ [-1,1]: var = 1/3
  //uniform u ∈ [-0.5,0.5]: var = 1/12
  //Gaussian N(0,1): var = 1
  float GetCompensationGainLinear(float inputVariance = float(1.0 / 3.0), float targetRms = 1.0f) const
  {
    float avgMag2 = AvgMag2();
    float outVar = inputVariance * avgMag2;
    float targetVar = targetRms * targetRms;

    float g = (outVar > 1e-20f) ? std::sqrt(targetVar / outVar) : 0.0f;
    return g;
  }

  // Returns linear magnitude at a frequency in Hz using current normalized coefficients
  float GetMagnitudeAtFrequency(float freqHz) const;
};


class CascadedBiquadFilter
{
  // 0 stage = bypass
  // 1 stage's slope = 12db/oct.
  // 2 stage = 24db/oct.
  // 3 stage = 36db/oct
  // 4 stage = 48db/oct.
  BiquadFilter mFilters[4];
  size_t mNStages;

public:
  explicit CascadedBiquadFilter(size_t nStages = 0)
      : mNStages(nStages)
  {
    CCASSERT(nStages >= 0 && nStages <= 4);
  }

  void Disable()
  {
    mNStages = 0;
  }

  void SetParams(int nStages, BiquadFilterType type, float freq, float q, float gain)
  {
    CCASSERT(nStages >= 0 && nStages <= 4);
    mNStages = nStages;

    if (nStages == 0)
      return;

    // Compute coefficients once, then copy to remaining stages.
    mFilters[0].SetParams(type, freq, q, gain);
    for (size_t i = 1; i < nStages; ++i)
    {
      mFilters[i].CopyParamsAndCoeffsFrom(mFilters[0]);
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
    // float g = 1.0f;
    // for (size_t i = 0; i < mNStages; ++i)
    // {
    //   g *= mFilters[i].GetCompensationGainLinear();
    // }
    // return g;
	return 1;
  }
};  // class CascadedBiquadFilter
}  // namespace WaveSabreCore


#endif
