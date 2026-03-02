
#include <WaveSabreCore/SVFilter.hpp>

namespace WaveSabreCore
{
namespace M7
{
// Update filter coefficients if cutoff or Q has changed
M7::FloatPair SVFilter::updateCoefficients(float v0, float cutoff, float Q)
{
  float new_d0 = cutoff + Q;
  if (d1 != new_d0)
  {
    d1 = d0;
    d0 = new_d0;
    g = M7::math::tan(M7::math::gPI * cutoff * Helpers::CurrentSampleRateRecipF);
    k = 1.0f / Q;
    a1 = 1.0f / (1.0f + g * (g + k));
    a2 = g * a1;
  }
  float v1 = a1 * ic1eq + a2 * (v0 - ic2eq);
  float v2 = ic2eq + g * v1;
  ic1eq = 2 * v1 - ic1eq;
  ic2eq = 2 * v2 - ic2eq;
  return {v1, v2};
}

float SVFilter::SVFlow(float v0, float cutoff, float Q)
{
  auto x = updateCoefficients(v0, cutoff, Q);
  return x.x[1];
}

float SVFilter::SVFhigh(float v0, float cutoff, float Q)
{
  auto x = updateCoefficients(v0, cutoff, Q);
  return v0 - k * x.x[0] - x.x[1];
}

float SVFilter::SVFall(float v0, float cutoff, float Q)
{
  auto x = updateCoefficients(v0, cutoff, Q);
  return v0 - 2 * k * x.x[0];
}

void SVFilter::Reset()
{
  g = 0, k = 0, a1 = 0, a2 = 0;
  ic1eq = 0, ic2eq = 0;
  d0 = 0, d1 = 0;  // Used for tracking changes in cutoff and Q
}

}  // namespace M7


}  // namespace WaveSabreCore
