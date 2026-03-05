#pragma once

#include <Windows.h>
#undef min
#undef max
#include <algorithm>

#include <type_traits>

#include "../WaveSabreCoreFeatures.hpp"

using std::max;
using std::min;

// https://stackoverflow.com/questions/1505582/determining-32-vs-64-bit-in-c
#if _WIN32 || _WIN64
  #if _WIN64
    #define ENV64BIT
  #else
    #define ENV32BIT
  #endif
#endif

#ifdef _DEBUG
  #include <cstdio>  // for sprintf

  #ifdef _DEBUG
    #define WSASSERT(condition)                                                                                        \
      do                                                                                                               \
      {                                                                                                                \
        if (!(condition))                                                                                              \
        {                                                                                                              \
          ::DebugBreak();                                                                                              \
        }                                                                                                              \
      } while (false)

  #endif

static inline void assert__(bool condition, const char* conditionStr, const char* file, int line)
{
  if (condition)
    return;
  char s[1000];
  ::sprintf(s, "ASSERT failed: %s (%s:%d)", conditionStr, file, line);
  ::OutputDebugStringA(s);
  ::DebugBreak();
}

  #define CCASSERT(x) assert__(x, #x, __FILE__, __LINE__)

#else  // _DEBUG
  #define CCASSERT(x)
#endif  // _DEBUG


#define NOINLINE __declspec(noinline)

#define INLINE inline
#ifdef MIN_SIZE_REL
  #define FORCE_INLINE inline
#else
// for non-size-optimized builds (bloaty), force inline some things when profiling suggests its performant.
  #define FORCE_INLINE __forceinline
#endif


namespace WaveSabreCore::M7
{
using real_t = float;
using real2 =
    float;  // internal filter real type. filters are theoretically more stable. but for the moment it doesn't cause issues.

using real = real_t;

template <typename T>
constexpr real Real(const T x)
{
  return static_cast<real>(x);
  //return {x};
};
constexpr real RealPI = real{3.14159265358979323846264338327950288f};
constexpr real PITimes2 = real{2.0f} * RealPI;
constexpr real Real0 = real{0.0f};
constexpr real Real1 = real{1.0f};
constexpr real Real2 = real{2.0f};


template <typename Tret, typename Tb>
INLINE Tret AddEnum(Tret a, Tb b)
{
  return Tret((int)a + (int)b);
}


}  // namespace WaveSabreCore::M7