#include "WaveSabreCore/Maj7Basic.hpp"
#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Helpers.h>



// this is an impl of the RBJ cookbook filters. (search term "Cookbook formulae for audio EQ biquad filter coefficients")

namespace WaveSabreCore
{
void BiquadConfig::SetParams(BiquadFilterType type, float freq, float q, float gain)
{
  if (type == this->type && freq == this->freq && this->q == q && this->gain == gain)
    return;
  this->type = type;
  this->freq = freq;
  this->q = q;
  this->gain = gain;

  // these ranges can cause instability and are effectively out of usable range
  // NB: frequency params typically go down to like 31.25hz or so. catch that to give the user the option of bypassing.
  freq = M7::math::clamp(freq, 32, 20000);

  const float A = M7::math::DecibelsToLinear(gain);

  mW0 = M7::math::gPITimes2 * freq * Helpers::CurrentSampleRateRecipF;
  const float alpha = M7::math::sin(mW0) / (q * 2);
  float cosw0 = M7::math::cos(mW0);

  float& a0 = this->coeffs[0];
  float& a1 = this->coeffs[1];
  float& a2 = this->coeffs[2];
  float& b0 = this->coeffs[3];
  float& b1 = this->coeffs[4];
  float& b2 = this->coeffs[5];

  a1 = cosw0 * -2;
  a0 = 1 + alpha;
  a2 = 1 - alpha;
  switch (type)
  {
    default:
    // you may be tempted to try and unify these the same way as lowshelf / highshelf. but you don't gain anything (measured.)
    case BiquadFilterType::Lowpass:
      b1 = 1.0f - cosw0;
      b0 = b2 = b1 * 0.5f;
      break;
    case BiquadFilterType::Highpass:
      b1 = -(1.0f + cosw0);
      b0 = b2 = b1 * -0.5f;
      break;
    case BiquadFilterType::Bandpass:
      b0 = alpha;
      b1 = 0.0f;
      b2 = -alpha;
      break;
    case BiquadFilterType::Notch:
      b0 = 1.0f;
      b1 = -2.0f * cosw0;
      b2 = 1.0f;
      break;
    case BiquadFilterType::Allpass:
      b0 = 1.0f - alpha;
      b1 = -2.0f * cosw0;
      b2 = 1.0f + alpha;
      break;
    case BiquadFilterType::Peak:
    {
      a0 = 1.0f + (alpha / A);
      a2 = 1.0f - (alpha / A);
      b0 = 1.0f + alpha * A;
      b1 = -2.0f * cosw0;
      b2 = 1.0f - alpha * A;
      break;
    }
    case BiquadFilterType::LowShelf:
    case BiquadFilterType::HighShelf:
    {
      float sqrtAtimesAlphaTimes2 = M7::math::sqrt(A) * 2 * alpha;
      float Ap1 = A + 1;
      float Am1 = A - 1;
      float two = 2;
      if (type == BiquadFilterType::HighShelf)
      {
        two = -2;
        cosw0 = -cosw0;
      }
      float Am1CosW0 = Am1 * cosw0;

      b0 = A * (Ap1 - Am1CosW0 + sqrtAtimesAlphaTimes2);
      b2 = A * (Ap1 - Am1CosW0 - sqrtAtimesAlphaTimes2);
      a0 = Ap1 + Am1CosW0 + sqrtAtimesAlphaTimes2;
      a2 = Ap1 + Am1CosW0 - sqrtAtimesAlphaTimes2;
      b1 = two * A * (Am1 - Ap1 * cosw0);
      a1 = -two * (Am1 + Ap1 * cosw0);

      break;
    }
  }

  for (int i = 1; i < 6; ++i)
  {
    normCoeffs[i] = this->coeffs[i] / a0;
  }
}

BiquadFilter::BiquadFilter()
{
  Reset();
}

float BiquadFilter::ProcessSample(float input)
{
  const float& c1 = mConfig.normB0();
  const float& c2 = mConfig.normB1();
  const float& c3 = mConfig.normB2();
  const float& c4 = mConfig.normA1();
  const float& c5 = mConfig.normA2();

  float output = c1 * input              // c1 = b0/a0
                 + c2 * lastInput        // c2 = b1/a0
                 + c3 * lastLastInput    // c3 = b2 / a0;
                 - c4 * lastOutput       // c4 = a1 / a0
                 - c5 * lastLastOutput;  // c5 = a2 / a0

  lastLastInput = lastInput;
  lastInput = input;
  lastLastOutput = lastOutput;
  lastOutput = output;

  return output;
}

float BiquadFilter::GetMagnitudeAtFrequency(float freqHz) const
{
  // Use normalized coefficients (a0 == 1)
  const float& a0 = mConfig.normA0();
  const float& a1 = mConfig.normA1();
  const float& a2 = mConfig.normA2();
  const float& b0 = mConfig.normB0();
  const float& b1 = mConfig.normB1();
  const float& b2 = mConfig.normB2();
  const auto& w = mConfig.w0();

  const float cw = M7::math::cos(w);
  const float c2w = M7::math::cos(2.0 * w);

  const float num = b0 * b0 + b1 * b1 + b2 * b2 + 2.0 * (b0 * b1 + b1 * b2) * cw + 2.0 * b0 * b2 * c2w;
  const float den = 1.0 + a1 * a1 + a2 * a2 + 2.0 * (a1 + a1 * a2) * cw + 2.0 * a2 * c2w;

  return (M7::math::sqrt(num / den));
}
}  // namespace WaveSabreCore
