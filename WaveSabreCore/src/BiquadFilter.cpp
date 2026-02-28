#include "WaveSabreCore/Maj7Basic.hpp"
#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Helpers.h>


// this is an impl of the RBJ cookbook filters. (search term "Cookbook formulae for audio EQ biquad filter coefficients")

namespace WaveSabreCore
{
namespace M7
{
void BiquadConfig::SetBiquadParams(FilterResponse response, float freq, float q, float gain)
{
  if (response == this->response && freq == this->freq && this->q == q && this->gain == gain)
    return;
  this->response = response;
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
  switch (response)
  {
    default:
    // you may be tempted to try and unify these the same way as lowshelf / highshelf. but you don't gain anything (measured.)
    case FilterResponse::Lowpass:
      b1 = 1.0f - cosw0;
      b0 = b2 = b1 * 0.5f;
      break;
    case FilterResponse::Highpass:
      b1 = -(1.0f + cosw0);
      b0 = b2 = b1 * -0.5f;
      break;
    case FilterResponse::Bandpass:
      b0 = alpha;
      b1 = 0.0f;
      b2 = -alpha;
      break;
    case FilterResponse::Notch:
      b0 = 1.0f;
      b1 = -2.0f * cosw0;
      b2 = 1.0f;
      break;
    case FilterResponse::Allpass:
      b0 = 1.0f - alpha;
      b1 = -2.0f * cosw0;
      b2 = 1.0f + alpha;
      break;
    case FilterResponse::Peak:
    {
      a0 = 1.0f + (alpha / A);
      a2 = 1.0f - (alpha / A);
      b0 = 1.0f + alpha * A;
      b1 = -2.0f * cosw0;
      b2 = 1.0f - alpha * A;
      break;
    }
    case FilterResponse::LowShelf:
    case FilterResponse::HighShelf:
    {
      float sqrtAtimesAlphaTimes2 = M7::math::sqrt(A) * 2 * alpha;
      float Ap1 = A + 1;
      float Am1 = A - 1;
      float two = 2;
      if (response == FilterResponse::HighShelf)
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
    // it is actually necessary to do this using `double`. otherwise the response curve looks awful.
  // Use normalized coefficients (a0 == 1)
  const double a1 = mConfig.normA1();
  const double a2 = mConfig.normA2();
  const double b0 = mConfig.normB0();
  const double b1 = mConfig.normB1();
  const double b2 = mConfig.normB2();

  const double clampedFreqHz = M7::math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
  const double w = M7::math::gPITimes2d * clampedFreqHz * Helpers::CurrentSampleRateRecipF;

  const double cw = M7::math::CrtCos(w);
  const double c2w = M7::math::CrtCos(2 * w);

  const double num = b0 * b0 + b1 * b1 + b2 * b2 + 2.0 * (b0 * b1 + b1 * b2) * cw + 2.0 * b0 * b2 * c2w;
  const double den = 1.0 + a1 * a1 + a2 * a2 + 2.0 * (a1 + a1 * a2) * cw + 2.0 * a2 * c2w;

  const double safeDen = (den > 1e-20) ? den : 1e-20;
  const double ratio = num / safeDen;
  return M7::math::sqrt(float((ratio > 0) ? ratio : 0));
}
}  // namespace M7
}  // namespace WaveSabreCore
