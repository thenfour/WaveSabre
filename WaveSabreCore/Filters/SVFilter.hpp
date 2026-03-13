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

  float g;
  float k;
  float a1;
  float a2;
  float ic1eq;
  float ic2eq;
};  // SVFilter


}  // namespace M7


}  // namespace WaveSabreCore
