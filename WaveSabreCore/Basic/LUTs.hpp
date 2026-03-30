
#pragma once

#include "MathBase.hpp"

namespace WaveSabreCore
{
namespace M7
{
namespace math
{
//extern void InitLUTs();

static constexpr size_t gLutSize1D = 4096;
static constexpr size_t gLutSize2D = 512;

#ifdef FORCE_USE_INLINE_LUTS
#define USE_INLINE_LUTS
#endif

// LUTs are hot paths; inline as much as possible. But if perf is an issue, we want to inline for non-size-optimized builds
#ifndef MIN_SIZE_REL
#define USE_INLINE_LUTS
#endif

#ifndef USE_INLINE_LUTS

// lookup table for 0-1 input values, linear interpolation upon lookup.
// non-periodic.
struct LUT01
{
  //const size_t mNSamples;
  float mpTable[gLutSize1D];

  LUT01(/*size_t nSamples, */ float (*fn)(float));
  //#ifdef MIN_SIZE_REL
  //#else
  //                virtual ~LUT01();
  //#endif // MIN_SIZE_REL
  virtual float Invoke(float x) const;
};

struct SinCosLUT : public LUT01
{
  SinCosLUT(/*size_t nSamples,*/ float (*fn)(float));

  virtual float Invoke(float x) const override;
};

// tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
struct TanHLUT : public LUT01
{
  TanHLUT(/*size_t nSamples*/);
  virtual float Invoke(float x) const override;
};

struct LUT2D
{
  //size_t mNSamplesX;
  //size_t mNSamplesY;
  //float* mpTable;
  float mpTable[gLutSize2D * gLutSize2D];

  LUT2D(float (*fn)(float, float));
  //#ifdef MIN_SIZE_REL
  //#else
  //                virtual ~LUT2D();
  //#endif // MIN_SIZE_REL

  virtual float Invoke(float x, float y) const;
};

struct CurveLUT : public LUT2D
{
  // valid for 0<k<1 and 0<x<1
  static real_t modCurve_x01_k01_RT(real_t x, real_t k);

  // extends range to support -1<x<0 and -1<k<0
  // outputs -1 to 1
  static real_t modCurve_xN11_kN11_RT(real_t x, real_t k);

  // incoming values should support -1,1 and output -1,1
  CurveLUT();

  // user passes in n11 values; need to map N11 to 01
  virtual float Invoke(float xN11, float yN11) const override;
};

// pow(2,n) is a quite hot path, used by MidiNoteToFrequency(), as well as all other frequency calculations.
// the range of `n` is [-15,+15] but depends on frequency param scale. so let's extend a bit and make a huge lut.
struct Pow2_N16_16_LUT : public LUT01
{
  Pow2_N16_16_LUT(/*size_t nSamples*/);
  virtual float Invoke(float x) const override;
};

struct LUTs
{
  SinCosLUT gSinLUT;
  SinCosLUT gCosLUT;
  TanHLUT gTanhLUT;
  LUT01 gSqrt01LUT;

  CurveLUT gCurveLUT;
  Pow2_N16_16_LUT gPow2_N16_16_LUT;
  // pow10 could be useful too.

  LUTs();
};

#else

template <size_t N>
INLINE float LookupLUT1D(const float (&mpTable)[N], float x)
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

struct SinLUT
{
  float mpTable[gLutSize1D];

  INLINE SinLUT()
  {
    for (size_t i = 0; i < gLutSize1D; ++i)
    {
      float x = float(i) / (gLutSize1D - 1);
      mpTable[i] = (float)math::CrtSin((double)(x * gPITimes2));
    }
  }

  INLINE float Invoke(float x) const
  {
    static constexpr float gPeriodCorrection = 1 / gPITimes2;
    float scaledX = x * gPeriodCorrection;
    return LookupLUT1D(mpTable, fract(scaledX));
  }
};

struct CosLUT
{
  float mpTable[gLutSize1D];

  INLINE CosLUT()
  {
    for (size_t i = 0; i < gLutSize1D; ++i)
    {
      float x = float(i) / (gLutSize1D - 1);
      mpTable[i] = (float)math::CrtCos((double)(x * gPITimes2));
    }
  }

  INLINE float Invoke(float x) const
  {
    static constexpr float gPeriodCorrection = 1 / gPITimes2;
    float scaledX = x * gPeriodCorrection;
    return LookupLUT1D(mpTable, fract(scaledX));
  }
};

struct Sqrt01LUT
{
  float mpTable[gLutSize1D];

  INLINE Sqrt01LUT()
  {
    for (size_t i = 0; i < gLutSize1D; ++i)
    {
      float x = float(i) / (gLutSize1D - 1);
      mpTable[i] = (float)math::CrtPow((double)x, 0.5);
    }
  }

  INLINE float Invoke(float x) const
  {
    return LookupLUT1D(mpTable, x);
  }
};

// tanh approaches -1 before -PI, and +1 after +PI. so squish the range and do a 0,1 LUT mapping from -PI,PI
struct TanHLUT
{
  float mpTable[gLutSize1D];

  INLINE TanHLUT()
  {
    for (size_t i = 0; i < gLutSize1D; ++i)
    {
      float x = float(i) / (gLutSize1D - 1);
      mpTable[i] = (float)::tanh(((double)x - .5) * gPITimes2);
    }
  }
  INLINE float Invoke(float x) const
  {
    static constexpr float gOneOver2pi = 1.0f / gPITimes2;
    x *= gOneOver2pi;
    x += 0.5f;
    return LookupLUT1D(mpTable, x);
  }
};

// pow(2,n) is a quite hot path, used by MidiNoteToFrequency(), as well as all other frequency calculations.
// the range of `n` is [-15,+15] but depends on frequency param scale. so let's extend a bit and make a huge lut.
struct Pow2_N16_16_LUT
{
  float mpTable[gLutSize1D];

  INLINE Pow2_N16_16_LUT()
  {
    for (size_t i = 0; i < gLutSize1D; ++i)
    {
      float x = float(i) / (gLutSize1D - 1);
      mpTable[i] = (float)math::CrtPow(2.0, ((double)x - .5) * 32);
    }
  }
  INLINE float Invoke(float x) const
  {
    static constexpr float gScaleCorrection = 1.0f / 32;
    x *= gScaleCorrection;
    x += 0.5f;
    return LookupLUT1D(mpTable, x);
  }
};

// inline because only called once.
INLINE float LookupLUT2D(const float (&mpTable)[gLutSize2D * gLutSize2D], float x, float y)
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

struct CurveLUT  // : public LUT2D
{
  float mpTable[gLutSize2D * gLutSize2D];

  // incoming values should support -1,1 and output -1,1
  INLINE CurveLUT()
  {
    // Populate table.
    for (size_t y = 0; y < gLutSize2D; ++y)
    {
      for (size_t x = 0; x < gLutSize2D; ++x)
      {
        float x01 = float(x) / (gLutSize2D - 1);
        float y01 = float(y) / (gLutSize2D - 1);
        mpTable[y * gLutSize2D + x] = modCurve_xN11_kN11_RT(x01 * 2 - 1, y01 * 2 - 1);
      }
    }
  }

  // valid for 0<k<1 and 0<x<1
  INLINE static real_t modCurve_x01_k01_RT(real_t x, real_t k)
  {
    real_t ret = 1 - math::pow(x, k);
    return math::pow(ret, 1 / k);
  }

  // extends range to support -1<x<0 and -1<k<0
  // outputs -1 to 1
  INLINE static real_t modCurve_xN11_kN11_RT(real_t x, real_t k)
  {
    static constexpr float CornerMargin =
        0.6f;  // .77 is quite sharp, 0.5 is mild and usable but maybe should be sharper
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

  // user passes in n11 values; need to map N11 to 01
  NOINLINE float Invoke(float xN11, float yN11) const
  {
    return LookupLUT2D(mpTable, xN11 * .5f + .5f, yN11 * .5f + .5f);
  }
};

struct LUTs
{
  SinLUT gSinLUT;
  CosLUT gCosLUT;
  TanHLUT gTanhLUT;
  Sqrt01LUT gSqrt01LUT;
  Pow2_N16_16_LUT gPow2_N16_16_LUT;

  CurveLUT gCurveLUT;
  // pow10 could be useful too.
};

#endif  // MIN_SIZE_REL

extern LUTs* gLuts;

}  // namespace math


}  // namespace M7


}  // namespace WaveSabreCore
