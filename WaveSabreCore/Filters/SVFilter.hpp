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
  float mCutoff;
  float mQ;
  FilterResponse mResponse;

  float g, k, a1, a2;
  float ic1eq, ic2eq;
};  // SVFilter


}  // namespace M7


}  // namespace WaveSabreCore
