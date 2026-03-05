// basically just the stuff needed for the LUTs.
// after that, the normal Math.hpp can use luts.

#pragma once

#include <Windows.h>
#include <cmath>

#include "./Base.hpp"

#ifdef WAVESABRE_CUSTOM_MSVCRT
// MSVC wants to intrinsic many functions, bypassing our own MSVCRT shim and emitting linker errors.
// this #pragma forces it to call our function.
#pragma function(sin, cos, tan, exp, pow, log)
inline double MsvcrtSin(double x)
{
  return ::sin(x);
}
inline double MsvcrtCos(double x)
{
  return ::cos(x);
}
inline double MsvcrtFloor(double x)
{
  return ::floor(x);
}
inline double MsvcrtTan(double x)
{
  return ::tan(x);
}
inline double MsvcrtLog(double x)
{
  return ::log(x);
}
inline double MsvcrtExp(double x)
{
  return ::exp(x);
}
inline double MsvcrtPow(double x, double y)
{
  return ::pow(x, y);
}
#else
inline double MsvcrtSin(double x)
{
  return std::sin(x);
}
inline double MsvcrtCos(double x)
{
  return std::cos(x);
}
inline double MsvcrtFloor(double x)
{
  return std::floor(x);
}
inline double MsvcrtTan(double x)
{
  return std::tan(x);
}
inline double MsvcrtLog(double x)
{
  return std::log(x);
}
inline double MsvcrtExp(double x)
{
  return std::exp(x);
}
inline double MsvcrtPow(double x, double y)
{
  return std::pow(x, y);
}
#endif

namespace WaveSabreCore
{
namespace M7
{
namespace math
{

static constexpr float gPI = 3.14159265358979323846264338327950288f;
static constexpr double gPId = 3.14159265358979323846264338327950288;
static constexpr float gPITimes2 = gPI * 2;
static constexpr double gPITimes2d = gPId * 2;
static constexpr float gPIHalf = gPI * 0.5f;
static constexpr float gSqrt2 = 1.41421356237f;
static constexpr float gSqrt2Recip = 0.70710678118f;
static constexpr float FloatEpsilon = 1e-6f;

INLINE real_t floor(real_t x)
{
  return (real_t)MsvcrtFloor((double)x);
}
INLINE double floord(double x)
{
  return ::MsvcrtFloor(x);
}
INLINE float rand01()
{
  return float(::rand()) / RAND_MAX;
}
INLINE float randN11()
{
  return rand01() * 2 - 1;
}
INLINE float sqrt(float x)
{
  return (float)::sqrt((double)x);
}
INLINE double sqrt(double x)
{
  return ::sqrt(x);
}
NOINLINE constexpr float clamp(float x, float low, float hi)
{
  if (x <= low)
    return low;
  if (x >= hi)
    return hi;
  return x;
}
NOINLINE constexpr double clamp(double x, double low, double hi)
{
  if (x <= low)
    return low;
  if (x >= hi)
    return hi;
  return x;
}
INLINE constexpr float clamp01(float x)
{
  return clamp(x, 0, 1);
}
INLINE constexpr double clamp01(double x)
{
  return clamp(x, 0, 1);
}
INLINE constexpr float clampN11(float x)
{
  return clamp(x, -1, 1);
}
INLINE constexpr double clampN11(double x)
{
  return clamp(x, -1, 1);
}

template <typename T>
INLINE static constexpr T ClampI(T x, T minInclusive, T maxInclusive)
{
  if (x <= minInclusive)
    return minInclusive;
  if (x >= maxInclusive)
    return maxInclusive;
  return x;
}


NOINLINE float lerp(float a, float b, float t);
NOINLINE double lerpD(double a, double b, double t);
NOINLINE float lerp_rev(float v_min,
                      float v_max,
                      float v_val);
INLINE float fract(float x)
{
  // for negative values, fract works like,
  // e.g. -1.3 -> 0.7, because floor(-1.3) is -2.
  return x - math::floor(x);
}
INLINE double fract(double x)
{
  return x - math::floord(x);
}

INLINE float wrap01(float x)
{
  return fract(x);
}
INLINE double wrap01(double x)
{
  return fract(x);
}

INLINE float quantize(float x, float step)
{
  return math::floor(x / step) * step;
}

INLINE double quantize(double x, double step)
{
  return math::floord(x / step) * step;
}

INLINE float map(float inp, float inp_min, float inp_max, float out_min, float out_max)
{
  float t = lerp_rev(inp_min, inp_max, inp);
  return lerp(out_min, out_max, t);
}
INLINE float map_clamped(float inp, float inp_min, float inp_max, float out_min, float out_max)
{
  float t = lerp_rev(inp_min, inp_max, inp);
  return lerp(out_min, out_max, clamp01(t));
}

INLINE double CrtSin(double x)
{
  return ::MsvcrtSin(x);
}

INLINE double CrtCos(double x)
{
  return ::MsvcrtCos(x);
}

INLINE double CrtTan(double x)
{
  return ::MsvcrtTan(x);
}

INLINE double CrtFloor(double x)
{
  return ::MsvcrtFloor(x);
}

INLINE double CrtLog(double x)
{
  return ::MsvcrtLog(x);
}

INLINE double CrtPow(double x, double y)
{
  return ::MsvcrtPow(x, y);
}

INLINE double CrtFmod(double x, double y)
{
  return ::fmod(x, y);
}

INLINE real_t pow(real_t x, real_t y)
{
  return (float)CrtPow((double)x, (double)y);
}
INLINE real_t pow2(real_t y)
{
  return (float)CrtPow(2, (double)y);
}

INLINE float log2(float x)
{
  static constexpr float log2_e = 1.44269504089f;  // 1/log(2)
  return (float)CrtLog((double)x) * log2_e;
}
INLINE float log10(float x)
{
  static constexpr float log10_e = 0.4342944819f;  // 1.0f / LOG(10.0f);
  return (float)CrtLog((double)x) * log10_e;
}
INLINE real_t tan(real_t x)
{
  return (float)CrtTan((double)x);  // fastmath::fastertanfull(x); // less fast to call lib function, but smaller code.
}
INLINE float expf(float x)
{
  return (float)MsvcrtExp((double)x);
}

template<typename T>
INLINE T round(float x)
{
  return (T)(x + 0.5f);
}

INLINE float fmodf(float x, float q) {
    return (float)::fmod((double)x, (double)q);
}

INLINE double fmodd(double x, double q) {
    return ::fmod(x, q);
}

INLINE float copysignf(float x, float y) {
    return (y >= 0) ? std::abs(x) : -std::abs(x);
}

}  // namespace math


}  // namespace M7


}  // namespace WaveSabreCore
