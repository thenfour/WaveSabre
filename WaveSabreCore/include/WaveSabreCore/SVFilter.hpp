#pragma once

#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
#include "Maj7ModMatrix.hpp"
#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <array>

namespace WaveSabreCore
{
namespace M7
{
// Simper/Zavalishin form of state variable filter
struct SVFilter
{
  SVFilter()
  {
    Reset();
  }
  // one pole apf
  float SVFOPapf_temp(float x, float cutoff);

  // Update filter coefficients if cutoff or Q has changed
  M7::FloatPair updateCoefficients(float v0, float cutoff, float Q);
  float SVFlow(float v0, float cutoff, float Q);
  float SVFhigh(float v0, float cutoff, float Q);
  float SVFall(float v0, float cutoff, float Q);
  void Reset();

private:
  float g, k, a1, a2;
  float ic1eq, ic2eq;
  float d0, d1;  // Used for tracking changes in cutoff and Q
  float c, i;    // Additional state variables for SVFOPapf_temp
};  // SVFilter


}  // namespace M7


}  // namespace WaveSabreCore
