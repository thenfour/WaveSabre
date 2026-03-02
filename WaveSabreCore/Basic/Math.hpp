
#pragma once

#include <Windows.h>

#include "./Base.hpp"
#include "./LUTs.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace math
{

INLINE float pow2_N16_16(float n)
{
  return gLuts->gPow2_N16_16_LUT.Invoke(n);
}
INLINE real_t sin(float x)
{
  return gLuts->gSinLUT.Invoke(x);
}
INLINE real_t cos(float x)
{
  return gLuts->gCosLUT.Invoke(x);
}
INLINE real_t sin(double x)
{
  return gLuts->gSinLUT.Invoke(float(x));
}
INLINE real_t cos(double x)
{
  return gLuts->gCosLUT.Invoke(float(x));
}

// optimized LUT works for 0<=x<=1
INLINE real_t sqrt01(real_t x)
{
  return gLuts->gSqrt01LUT.Invoke(x);
}
INLINE real_t tanh(real_t x)
{
  return gLuts->gTanhLUT.Invoke(x);
}
INLINE real_t modCurve_xN11_kN11(real_t x, real_t k)
{
  return gLuts->gCurveLUT.Invoke(x, k);
}

template <typename T>
inline bool FloatEquals(T f1, T f2, T eps = T{FloatEpsilon})
{
  //return f1 == f2 || std::abs(f1 - f2) < eps;
  return std::abs(f1 - f2) < eps;
}

}  // namespace math


}  // namespace M7


}  // namespace WaveSabreCore
