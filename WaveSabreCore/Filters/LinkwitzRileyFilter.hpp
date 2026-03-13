#pragma once

#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "SVFilter.hpp"
#include <stdint.h>

namespace WaveSabreCore
{
namespace M7
{

enum class CrossoverSlope : uint8_t
{
  Slope_12dB,
  Slope_24dB,
  Slope_36dB,
  Slope_48dB,
  Count,
};

// clang-format off
#define CROSSOVER_SLOPE_CAPTIONS(symbolName)                                                                           \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::CrossoverSlope::Count]                       \
  {                                                                                                                    \
	"12dB", \
	"24dB",\
	"36dB",\
	"48dB",                                                                             \
  }
// clang-format on

#ifdef FIXED_SLOPE_CROSSOVER_ONLY
struct LinkwitzRileyFilter
{
  using real = float;

  // Q values to realize Linkwitz-Riley responses via cascaded 2nd-order SVFs
  // q24 = 1/sqrt(2) (?0.7071) gives a Butterworth 2nd-order section; two in cascade -> LR4 (24 dB/oct)
  static constexpr real q24 = 0.707106781187f;  // sqrt(0.5);

  SVFilter svf[2];
  MoogOnePoleFilter apf1;

  real mFrequency = -1.0f;
  FilterResponse mResponse;

  void SetParams(real freq, CrossoverSlope slope, FilterResponse response);

  real Process(real x);
};

#else  // FIXED_SLOPE_CROSSOVER_ONLY

struct LinkwitzRileyFilter
{
  using real = float;

  // Q values to realize Linkwitz-Riley responses via cascaded 2nd-order SVFs
  // q24 = 1/sqrt(2) (?0.7071) gives a Butterworth 2nd-order section; two in cascade -> LR4 (24 dB/oct)
  static constexpr real q24 = 0.707106781187f;  // sqrt(0.5);
  // The following are Q values for constructing an LR8 (48 dB/oct) via four 2nd-order sections.
  static constexpr real q48_1 = 0.541196100146f;  // 0.5 / cos($pi / 8 * 1);
  static constexpr real q48_2 = 1.30656296488f;   // 0.5 / cos($pi / 8 * 3);

  SVFilter svf[4];
  MoogOnePoleFilter apf1;

  real mFrequency = -1.0f;
  CrossoverSlope mSlope;
  FilterResponse mResponse;

  void SetParams(real freq, CrossoverSlope slope, FilterResponse response);

  real Process(real x);
};

#endif  // FIXED_SLOPE_CROSSOVER_ONLY

}  // namespace M7


}  // namespace WaveSabreCore
