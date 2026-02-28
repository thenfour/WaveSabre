#ifndef __WAVESABRECORE_BIQUADFILTER_H__
#define __WAVESABRECORE_BIQUADFILTER_H__

#include "LUTs.hpp"
#include "WaveSabreCore/Filters/FilterBase.hpp"
#include "WaveSabreCore/Maj7Basic.hpp"
#include "WaveSabreCore/Maj7ParamAccessor.hpp"


namespace WaveSabreCore
{
namespace M7
{

// POD.
struct BiquadConfig
{
  // q is linear Q (typically ~0.2 to ~12)
  void SetBiquadParams(FilterResponse type, float freq, float q, float gain);

  const float& normA0() const
  {
    static_assert(std::is_trivially_copyable<BiquadConfig>::value, "BiquadConfig must be a POD struct");
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

  float Q() const
  {
    return this->q;
  }
  float FreqHz() const
  {
    return this->freq;
  }

private:
  FilterResponse response;
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

class BiquadFilter  // : public IFilter
{
  BiquadConfig mConfig;
  float lastInput, lastLastInput;
  float lastOutput, lastLastOutput;

public:
  BiquadFilter();

  void SetBiquadParams(FilterResponse response, float freq, float q, float gain)
  {
    mConfig.SetBiquadParams(response, freq, q, gain);
  }

  float Q() const
  {
    return mConfig.Q();
  }
  float freqHz() const
  {
    return mConfig.FreqHz();
  }

  //   void SetParams(FilterCircuit circuit,
  //                          FilterSlope slope,
  //                          FilterResponse response,
  //                          float cutoffHz,
  //                          float reso01) override
  //   {
  //     // M7::ParamAccessor pa{&reso01, 0};
  //     // float q = pa.GetDivCurvedValue(0, M7::gBiquadFilterQCfg, 0);

  // 	//

  //     SetBiquadParams(response, cutoffHz, q, 0);
  //   }

  // IFilter
  //   virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  //   {
  //     if (circuit != FilterCircuit::Biquad)
  //       return false;
  //     if (slope != FilterSlope::Slope12dbOct)
  //       return false;
  //     // supports all responses.
  //     return true;
  //   }

  float ProcessSample(float x);

  void Reset()
  {
    lastInput = lastLastInput = 0.0f;
    lastOutput = lastLastOutput = 0.0f;
  }

  // Copies filter type/params/coefficients from another instance, but preserves this instance's state.
  void CopyParamsAndCoeffsFrom(const BiquadFilter& src)
  {
    this->mConfig = src.mConfig;
  }

  const BiquadConfig& GetConfig() const
  {
    return mConfig;
  }

  // Returns linear magnitude at a frequency in Hz using current normalized coefficients
  float GetMagnitudeAtFrequency(float freqHz) const;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
class CascadedBiquadFilter : public IFilter
{
  enum class QStrategy
  {
    UserResonance,
    Butterworth,
  };

  static constexpr size_t kMaxStages = 8;
  // 0 stage = bypass
  // 1 stage's slope = 12db/oct.
  // 2 stage = 24db/oct.
  // 3 stage = 36db/oct
  // 4 stage = 48db/oct.
  BiquadFilter mFilters[kMaxStages];
  size_t mNStages;
  float mGainCompensationLinear;
  float mEnableCompensationGain = false;

  static inline float ButterworthQForSection(int sectionIndex, int nStages)
  {
    const int N = nStages * 2;  // filter order
    const float k = (float)(sectionIndex + 1);
    const float angle = ((2.0f * k) - 1.0f) * math::gPI / (2.0f * (float)N);
    const float c = math::cos(angle);
    const float denom = (2.0f * c > 1e-6f) ? (2.0f * c) : 1e-6f;
    return 1.0f / denom;
  }

public:
  explicit CascadedBiquadFilter()
      : mNStages(0)
  {
  }

  void SetCompensationEnabled(bool enabled)
  {
	mEnableCompensationGain = enabled;
  }

  void Disable()
  {
    mNStages = 0;
    mGainCompensationLinear = 1;
  }

  void SetBiquadParams(int nStages,
                       FilterResponse response,
                       float cutoffHz,
                       float q,
                       float gain,
                       QStrategy qStrategy)
  {
    CCASSERT(nStages >= 0 && nStages <= kMaxStages);

    // if nStages has increased since last call, we need to reset the new stages.
    if (nStages > mNStages)
    {
      for (size_t i = mNStages; i < nStages; ++i)
      {
        mFilters[i].Reset();
      }
    }

    mNStages = nStages;

    if (nStages == 0)
      return;

    if (qStrategy == QStrategy::UserResonance)
    {
      // Compute coefficients once, then copy to remaining stages.
      mFilters[0].SetBiquadParams(response, cutoffHz, q, q);
      for (size_t i = 1; i < nStages; ++i)
      {
        mFilters[i].CopyParamsAndCoeffsFrom(mFilters[0]);
      }
    }
    else
    {
      for (int i = 0; i < nStages; ++i)
      {
        const float sectionQ = ButterworthQForSection(i, nStages);
        mFilters[i].SetBiquadParams(response, cutoffHz, sectionQ, 0.0f);
      }
    }

    mGainCompensationLinear = mEnableCompensationGain ? CalculateCompensationGainLinear() : 1.0f;
  }

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    // convert slope to n stages.
    int nStages = (int)slope - 1;
    static_assert(((int)(FilterSlope::Slope12dbOct)-1) == 1, "filter slope enum values must match n stages + 1");
    static_assert(((int)(FilterSlope::Slope24dbOct)-1) == 2, "filter slope enum values must match n stages + 1");
    static_assert(((int)(FilterSlope::Slope96dbOct)-1) == 8, "filter slope enum values must match n stages + 1");

    if (circuit == FilterCircuit::Butterworth)
    {
      SetBiquadParams(nStages, response, cutoffHz, 0, 0, QStrategy::Butterworth);
    }
    else
    {
      const float q = gBiquadFilterQCfg.Param01ToValue(reso01);
      SetBiquadParams(nStages, response, cutoffHz, q, 0, QStrategy::UserResonance);
    }
  }

  // IFilter
  virtual float ProcessSample(float x) override
  {
    float y = x;
    for (size_t i = 0; i < mNStages; ++i)
    {
      y = mFilters[i].ProcessSample(y);
    }
    return y * mGainCompensationLinear;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    if (mNStages == 0)
      return 1.0f;

    double mag = 1.0;
    for (size_t i = 0; i < mNStages; ++i)
    {
      mag *= double(mFilters[i].GetMagnitudeAtFrequency((float)freqHz));
    }
    mag *= double(mGainCompensationLinear);
    return (real)((mag > 0.0) ? mag : 0.0);
  }

  // IFilter
  virtual void Reset() override
  {
    for (size_t i = 0; i < mNStages; ++i)
    {
      mFilters[i].Reset();
    }
  }


  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response)
  {
    if (circuit != FilterCircuit::Biquad && circuit != FilterCircuit::Butterworth)
      return false;
    if (slope < FilterSlope::Slope12dbOct || slope > FilterSlope::Slope96dbOct)
      return false;
    if (circuit == FilterCircuit::Butterworth)
      return response == FilterResponse::Lowpass || response == FilterResponse::Highpass;
    return true;
  }


  float CalculateCompensationGainLinear() const
  {
    if (mNStages == 0)
      return 1.0f;

    constexpr float kEpsilon = 1e-10f;
    constexpr int kNFreqBins = 32;
    constexpr int kImpulseTaps = 256;

    float sumMag2 = 0.0;
    for (int k = 0; k < kNFreqBins; ++k)
    {
      const float w = M7::math::gPI * (k + 0.5f) / kNFreqBins;
      const float c1 = M7::math::cos(w);
      const float s1 = M7::math::sin(w);
      const float c2 = M7::math::cos(2 * w);
      const float s2 = M7::math::sin(2 * w);

      float cascadeMag2 = 1;
      for (size_t i = 0; i < mNStages; ++i)
      {
        const auto& cfg = mFilters[i].GetConfig();
        const float b0 = cfg.normB0();
        const float b1 = cfg.normB1();
        const float b2 = cfg.normB2();
        const float a1 = cfg.normA1();
        const float a2 = cfg.normA2();

        const float numRe = b0 + b1 * c1 + b2 * c2;
        const float numIm = -b1 * s1 - b2 * s2;
        const float denRe = 1 + a1 * c1 + a2 * c2;
        const float denIm = -a1 * s1 - a2 * s2;
        const float numMag2 = numRe * numRe + numIm * numIm;
        const float denMag2 = denRe * denRe + denIm * denIm;
        cascadeMag2 *= (denMag2 > kEpsilon) ? (numMag2 / denMag2) : 0.0f;
      }
      sumMag2 += cascadeMag2;
    }

    const float avgMag2 = sumMag2 / kNFreqBins;
    const float inputVariance = 1.0f / 3.0f;
    const float sigmaOut = M7::math::sqrt((inputVariance * avgMag2 > kEpsilon) ? (inputVariance * avgMag2) : kEpsilon);
    //const float peakFactor = M7::math::sqrt(2.0f * (float)M7::math::CrtLog(1024.0));
    // empirically determined factor to get close to -0.1dBFS peaks with white noise input. not exact because the noise is not perfectly white, and the filter response is not perfectly flat in passband.
    constexpr float peakFactor = 3.0f;
    const float gPeak = 1.0f / (peakFactor * sigmaOut);

    float x1[kMaxStages] = {0};
    float x2[kMaxStages] = {0};
    float y1[kMaxStages] = {0};
    float y2[kMaxStages] = {0};
    float l1 = 0.0f;
    int tinyTail = 0;

    const auto& cfg = mFilters[0].GetConfig();
    const float b0 = cfg.normB0();
    const float b1 = cfg.normB1();
    const float b2 = cfg.normB2();
    const float a1 = cfg.normA1();
    const float a2 = cfg.normA2();

    for (int n = 0; n < kImpulseTaps; ++n)
    {
      float stageIn = (n == 0) ? 1.0f : 0.0f;
      for (size_t i = 0; i < mNStages; ++i)
      {
        const float y = b0 * stageIn + b1 * x1[i] + b2 * x2[i] - a1 * y1[i] - a2 * y2[i];
        x2[i] = x1[i];
        x1[i] = stageIn;
        y2[i] = y1[i];
        y1[i] = y;
        stageIn = y;
      }

      l1 += std::abs(stageIn);

      if (std::abs(stageIn) < kEpsilon)
      {
        tinyTail++;
        if (tinyTail > 128)
          break;
      }
      else
      {
        tinyTail = 0;
      }
    }

    const float gSafe = 1.0f / ((l1 > kEpsilon) ? l1 : kEpsilon);
    const float g = (gPeak < gSafe) ? gPeak : gSafe;
    return M7::math::clamp(g, 0.0f, 16.0f);
  }
};  // class CascadedBiquadFilter
}  // namespace M7
}  // namespace WaveSabreCore


#endif
