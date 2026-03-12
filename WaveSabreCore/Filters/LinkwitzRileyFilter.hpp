#pragma once

#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "SVFilter.hpp"
#include <stdint.h>

namespace WaveSabreCore
{
namespace M7
{

struct LinkwitzRileyFilter
{
  using real = float;

  // Q values to realize Linkwitz-Riley responses via cascaded 2nd-order SVFs
  // q24 = 1/sqrt(2) (?0.7071) gives a Butterworth 2nd-order section; two in cascade -> LR4 (24 dB/oct)
  static constexpr real q24 = 0.707106781187f;  // sqrt(0.5);
  // The following are Q values for constructing an LR8 (48 dB/oct) via four 2nd-order sections.
  // They are kept for reference but not used in the current build.
  static constexpr real q48_1 = 0.541196100146f;  // 0.5 / cos($pi / 8 * 1);
  static constexpr real q48_2 = 1.30656296488f;   // 0.5 / cos($pi / 8 * 3);

  SVFilter svf[4];
  MoogOnePoleFilter apf1;

  real mFrequency;
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

    // TODO: set parameters for the underlying SVFs based on the desired slope and response.

    // TODO: check correctness of this:
    apf1.SetParams(
        FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Allpass, freq, Param01(0), 0);
  }

  real process(real x)
  {
    // TODO: implement processing based on the desired slope and response.
  }

  // real LR_LPF(real x, real freq, CrossoverSlope slopeCode)
  // {
  //   switch (slopeCode)
  //   {
  //     default:
  //     case 1:
  //       return svf[0].SVFlow(x, freq, 0.5f);
  //     case 2:
  //       x = svf[0].SVFlow(x, freq, q24);
  //       return svf[1].SVFlow(x, freq, q24);
  //     case 3:
  //       x = svf[0].SVFlow(x, freq, 1.0f);
  //       x = svf[1].SVFlow(x, freq, 1.0f);
  //       return svf[2].SVFlow(x, freq, 0.5f);
  //     case 4:
  //       x = svf[0].SVFlow(x, freq, q48_1);
  //       x = svf[1].SVFlow(x, freq, q48_2);
  //       x = svf[2].SVFlow(x, freq, q48_1);
  //       return svf[3].SVFlow(x, freq, q48_2);
  //   }
  // }

  // real LR_HPF(real x, real freq, CrossoverSlope slopeCode)
  // {
  //   if (slopeCode == 1 || slopeCode == 3)
  //   {
  //     x = -x;
  //   }

  //   switch (slopeCode)
  //   {
  //     default:
  //     case 1:
  //       return svf[0].SVFhigh(x, freq, 0.5f);
  //     case 2:
  //       x = svf[0].SVFhigh(x, freq, q24);
  //       return svf[1].SVFhigh(x, freq, q24);
  //     case 3:
  //       x = svf[0].SVFhigh(x, freq, 1.0f);
  //       x = svf[1].SVFhigh(x, freq, 1.0f);
  //       return svf[2].SVFhigh(x, freq, 0.5f);
  //     case 4:
  //       x = svf[0].SVFhigh(x, freq, q48_1);
  //       x = svf[1].SVFhigh(x, freq, q48_2);
  //       x = svf[2].SVFhigh(x, freq, q48_1);
  //       return svf[3].SVFhigh(x, freq, q48_2);
  //   }
  // }

  // real APF(real x, real freq, CrossoverSlope slopeCode)
  // {
  //   switch (slopeCode)
  //   {
  //     default:
  //     case 1:
  //       return apf1.Process(x, freq);
  //     case 2:
  //       return svf[0].SVFall(x, freq, q24);
  //     case 3:
  //       x = svf[0].SVFall(-x, freq, 1.0f);
  //       return apf1.Process(x, freq);
  //     case 4:
  //       x = svf[0].SVFall(x, freq, q48_1);
  //       return svf[1].SVFall(x, freq, q48_2);
  //   }
  // }
};

}  // namespace M7


}  // namespace WaveSabreCore
