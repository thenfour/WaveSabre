#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Filters/FilterBase.hpp"

namespace WaveSabreCore
{
namespace M7
{
// Simper/Zavalishin form of state variable filter
struct SVFilter
{
  void SetParams(float cutoff, float Q, FilterResponse response);
  float Process(float v0);

private:
  float mCutoff = -1.0f;
  float mQ = -1.0f;
  FilterResponse mResponse = FilterResponse::Lowpass;

  float g = 0.0f;
  float k = 0.0f;
  float a1 = 0.0f;
  float a2 = 0.0f;
  float ic1eq = 0.0f;
  float ic2eq = 0.0f;
};  // SVFilter


}  // namespace M7


}  // namespace WaveSabreCore
