
#include "SVFilter.hpp"
#include "../Basic/Helpers.h"

namespace WaveSabreCore
{
namespace M7
{

void SVFilter::SetParams(float cutoff, float Q, FilterResponse response)
{
  if (cutoff == mCutoff && Q == mQ && response == mResponse)
  {
    return;
  }
  mCutoff = cutoff;
  mQ = Q;
  mResponse = response;

  //g = M7::math::tan(M7::math::gPI * cutoff * Helpers::CurrentSampleRateRecipF);
  g = CalculateFilterG(cutoff);
  k = 1.0f / Q;  // do NOT try to normalize this; caller is responsible for usable values. (size-optimization)
  a1 = 1.0f / (1.0f + g * (g + k));
  a2 = g * a1;
}

// Update filter coefficients if cutoff or Q has changed
float SVFilter::Process(float v0)
{
  float v1 = a1 * ic1eq + a2 * (v0 - ic2eq);
  float v2 = ic2eq + g * v1;
  ic1eq = 2 * v1 - ic1eq;
  ic2eq = 2 * v2 - ic2eq;

  switch (mResponse)
  {
    default:
    case FilterResponse::Lowpass:
      return v2;
    case FilterResponse::Highpass:
      return v0 - k * v1 - v2;
    case FilterResponse::Bandpass:
      return v1;
    case FilterResponse::Allpass:
      return v0 - 2 * k * v1;
      // case FilterResponse::Notch:
      //   return v0 - k * v1;
  }
}

}  // namespace M7


}  // namespace WaveSabreCore
