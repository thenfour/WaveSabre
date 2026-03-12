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

  void SetParams(real freq, CrossoverSlope slope, FilterResponse response)
  {
    if (freq == mFrequency && slope == mSlope && response == mResponse)
    {
      return;
    }
    mResponse = response;
    mFrequency = freq;
    mSlope = slope;

    switch (mResponse)
    {
      case FilterResponse::Lowpass:
      case FilterResponse::Highpass:
        switch (mSlope)
        {
          default:
          case CrossoverSlope::Slope_12dB:
            svf[0].SetParams(freq, 0.5f, mResponse);
            break;
          case CrossoverSlope::Slope_24dB:
            svf[0].SetParams(freq, q24, mResponse);
            svf[1].SetParams(freq, q24, mResponse);
            break;
          case CrossoverSlope::Slope_36dB:
            svf[0].SetParams(freq, 1.0f, mResponse);
            svf[1].SetParams(freq, 1.0f, mResponse);
            svf[2].SetParams(freq, 0.5f, mResponse);
            break;
          case CrossoverSlope::Slope_48dB:
            svf[0].SetParams(freq, q48_1, mResponse);
            svf[1].SetParams(freq, q48_2, mResponse);
            svf[2].SetParams(freq, q48_1, mResponse);
            svf[3].SetParams(freq, q48_2, mResponse);
            break;
        }
        break;
      case FilterResponse::Allpass:
        switch (mSlope)
        {
          default:
          case CrossoverSlope::Slope_12dB:
            break;
          case CrossoverSlope::Slope_24dB:
            svf[0].SetParams(freq, q24, FilterResponse::Allpass);
            break;
          case CrossoverSlope::Slope_36dB:
            svf[0].SetParams(freq, 1.0f, FilterResponse::Allpass);
            break;
          case CrossoverSlope::Slope_48dB:
            svf[0].SetParams(freq, q48_1, FilterResponse::Allpass);
            svf[1].SetParams(freq, q48_2, FilterResponse::Allpass);
            break;
        }
        break;
      default:
        break;
    }

    apf1.SetParams(
        FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Allpass, freq, Param01(0), 0);
  }

  real Process(real x)
  {
    switch (mResponse)
    {
      case FilterResponse::Lowpass:
        switch (mSlope)
        {
          default:
          case CrossoverSlope::Slope_12dB:
            return svf[0].Process(x);
          case CrossoverSlope::Slope_24dB:
            x = svf[0].Process(x);
            return svf[1].Process(x);
          case CrossoverSlope::Slope_36dB:
            x = svf[0].Process(x);
            x = svf[1].Process(x);
            return svf[2].Process(x);
          case CrossoverSlope::Slope_48dB:
            x = svf[0].Process(x);
            x = svf[1].Process(x);
            x = svf[2].Process(x);
            return svf[3].Process(x);
        }
      case FilterResponse::Highpass:
        if (mSlope == CrossoverSlope::Slope_12dB || mSlope == CrossoverSlope::Slope_36dB)
        {
          x = -x;
        }

        switch (mSlope)
        {
          default:
          case CrossoverSlope::Slope_12dB:
            return svf[0].Process(x);
          case CrossoverSlope::Slope_24dB:
            x = svf[0].Process(x);
            return svf[1].Process(x);
          case CrossoverSlope::Slope_36dB:
            x = svf[0].Process(x);
            x = svf[1].Process(x);
            return svf[2].Process(x);
          case CrossoverSlope::Slope_48dB:
            x = svf[0].Process(x);
            x = svf[1].Process(x);
            x = svf[2].Process(x);
            return svf[3].Process(x);
        }
      case FilterResponse::Allpass:
        switch (mSlope)
        {
          default:
          case CrossoverSlope::Slope_12dB:
            return apf1.ProcessSample(x);
          case CrossoverSlope::Slope_24dB:
            return svf[0].Process(x);
          case CrossoverSlope::Slope_36dB:
            x = svf[0].Process(x);
            return apf1.ProcessSample(x);
          case CrossoverSlope::Slope_48dB:
            x = svf[0].Process(x);
            return svf[1].Process(x);
        }
      default:
        return x;
    }
  }
};

}  // namespace M7


}  // namespace WaveSabreCore
