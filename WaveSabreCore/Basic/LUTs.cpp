
#include "LUTs.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace math
{

#ifdef MIN_SIZE_REL
LUTs::LUTs()
    : gSinLUT{[](float x)
              {
                return (float)math::CrtSin((double)(x * gPITimes2));
              }}
    , gCosLUT{[](float x)
              {
                return (float)math::CrtCos((double)(x * gPITimes2));
              }}
    , gTanhLUT{}
    , gSqrt01LUT{[](float x)
                 {
                   return (float)math::CrtPow((double)x, 0.5);
                 }}
    ,

    gCurveLUT{}
    , gPow2_N16_16_LUT{}

{
}

LUT01::LUT01(float (*fn)(float))
{
  // populate table.
  for (size_t i = 0; i < gLutSize1D; ++i)
  {
    float x = float(i) / (gLutSize1D - 1);
    mpTable[i] = fn(x);
  }
}

float LUT01::Invoke(float x) const
{
  if (x <= 0)
    return mpTable[0];
  if (x >= 1)
    return mpTable[gLutSize1D - 1];
  float index = x * (gLutSize1D - 1);
  int lower_index = static_cast<int>(index);
  int upper_index = lower_index + 1;
  float t = index - lower_index;
  return (1 - t) * mpTable[lower_index] + t * mpTable[upper_index];
}

SinCosLUT::SinCosLUT(float (*fn)(float))
    : LUT01(fn)
{
}

float SinCosLUT::Invoke(float x) const
{
  static constexpr float gPeriodCorrection = 1 / gPITimes2;
  float scaledX = x * gPeriodCorrection;
  return LUT01::Invoke(fract(scaledX));
}


Pow2_N16_16_LUT::Pow2_N16_16_LUT()
    : LUT01(
          [](float n)
          {
            return (float)math::CrtPow(2.0, ((double)n - .5) * 32);  // return (float)::sin((double)x * 2 * M_PI); }};
          })
{
}

float Pow2_N16_16_LUT::Invoke(float x) const
{  // passed in value is -16,16
  static constexpr float gScaleCorrection = 1.0f / 32;
  x *= gScaleCorrection;
  x += 0.5f;
  return LUT01::Invoke(x);
}

// tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
TanHLUT::TanHLUT()
    : LUT01(
          [](float x)
          {
            return (float)::tanh(((double)x - .5) * gPITimes2d);
          })
{
}

float TanHLUT::Invoke(float x) const
{
  static constexpr float gOneOver2pi = 1.0f / gPITimes2;
  x *= gOneOver2pi;
  x += 0.5f;
  return LUT01::Invoke(x);
}

LUT2D::LUT2D(float (*fn)(float, float))
{
  // Populate table.
  for (size_t y = 0; y < gLutSize2D; ++y)
  {
    for (size_t x = 0; x < gLutSize2D; ++x)
    {
      float u = float(x) / (gLutSize2D - 1);
      float v = float(y) / (gLutSize2D - 1);
      mpTable[y * gLutSize2D + x] = fn(u, v);
    }
  }
}

float LUT2D::Invoke(float x, float y) const
{
  if (x <= 0)
    x = 0;
  if (y <= 0)
    y = 0;
  if (x >= 1)
    x = 0.9999f;  // this ensures it will not OOB
  if (y >= 1)
    y = 0.9999f;

  float indexX = x * (gLutSize2D - 1);
  float indexY = y * (gLutSize2D - 1);

  int lowerIndexX = static_cast<int>(indexX);
  int lowerIndexY = static_cast<int>(indexY);
  int upperIndexX = lowerIndexX + 1;
  int upperIndexY = lowerIndexY + 1;

  float tx = indexX - lowerIndexX;
  float ty = indexY - lowerIndexY;

  float f00 = mpTable[lowerIndexY * gLutSize2D + lowerIndexX];
  float f10 = mpTable[lowerIndexY * gLutSize2D + upperIndexX];
  float f0 = lerp(f00, f10, tx);

  float f01 = mpTable[upperIndexY * gLutSize2D + lowerIndexX];
  float f11 = mpTable[upperIndexY * gLutSize2D + upperIndexX];
  float f1 = lerp(f01, f11, tx);

  return lerp(f0, f1, ty);
}

// valid for 0<k<1 and 0<x<1
real_t CurveLUT::modCurve_x01_k01_RT(real_t x, real_t k)
{
  real_t ret = 1 - math::pow(x, k);
  return math::pow(ret, 1 / k);
}

// extends range to support -1<x<0 and -1<k<0
// outputs -1 to 1
real_t CurveLUT::modCurve_xN11_kN11_RT(real_t x, real_t k)
{
  static constexpr float CornerMargin = 0.6f;  // .77 is quite sharp, 0.5 is mild and usable but maybe should be sharper
  k *= CornerMargin;
  k = clamp(k, -CornerMargin, CornerMargin);
  x = clampN11(x);
  if (k >= 0)
  {
    if (x > 0)
    {
      return 1 - modCurve_x01_k01_RT(x, 1 - k);
    }
    return modCurve_x01_k01_RT(-x, 1 - k) - 1;
  }
  if (x > 0)
  {
    return modCurve_x01_k01_RT(1 - x, 1 + k);
  }
  return -modCurve_x01_k01_RT(x + 1, 1 + k);
}

// incoming values should support -1,1 and output -1,1
CurveLUT::CurveLUT()
    : LUT2D(
          [](float x01, float y01)
          {
            return modCurve_xN11_kN11_RT(x01 * 2 - 1, y01 * 2 - 1);
          })
{
}

// user passes in n11 values; need to map N11 to 01
float CurveLUT::Invoke(float xN11, float yN11) const
{
  return LUT2D::Invoke(xN11 * .5f + .5f, yN11 * .5f + .5f);
}

#endif

float lerp(float a, float b, float t)
{
  return a * (1.0f - t) + b * t;
}
double lerpD(double a, double b, double t)
{
  return a * (1.0 - t) + b * t;
}
float lerp_rev(float v_min, float v_max,
               float v_val)  // inverse of lerp; returns 0-1 where t lies between a and b.
{
  return (v_val - v_min) / (v_max - v_min);
}

// in minsizerel, static initialization is disabled.
// everything must use gigasynth so we init from there.
LUTs* gLuts = nullptr;


}  // namespace math

}  // namespace M7


}  // namespace WaveSabreCore
