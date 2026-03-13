#include "LinkwitzRileyFilter.hpp"

namespace WaveSabreCore::M7
{
#ifdef FIXED_SLOPE_CROSSOVER_ONLY
void LinkwitzRileyFilter::SetParams(real freq, CrossoverSlope slope, FilterResponse response)
{
  if (freq == mFrequency && response == mResponse)
  {
    return;
  }
  mResponse = response;
  mFrequency = freq;

  apf1.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Allpass, freq, Param01(0), 0);

  // for lowpass and highpass, the same SVF settings can be used; the only difference is whether the output is taken from the LP or HP output of the SVF.
  for (auto& s : svf)
  {
    s.SetParams(freq, q24, response);
  }
}

real LinkwitzRileyFilter::Process(real x)
{
  x = svf[0].Process(x);
  switch (mResponse)
  {
    case FilterResponse::Lowpass:
    case FilterResponse::Highpass:
      return svf[1].Process(x);
    default:
      return x;
  }
}

#else  // FIXED_SLOPE_CROSSOVER_ONLY


void LinkwitzRileyFilter::SetParams(real freq, CrossoverSlope slope, FilterResponse response)
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
  #ifdef ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_12dB:
          svf[0].SetParams(freq, 0.5f, mResponse);
          break;
  #endif  // ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_24dB:
          svf[0].SetParams(freq, q24, mResponse);
          svf[1].SetParams(freq, q24, mResponse);
          break;
  #ifdef ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_36dB:
          svf[0].SetParams(freq, 1.0f, mResponse);
          svf[1].SetParams(freq, 1.0f, mResponse);
          svf[2].SetParams(freq, 0.5f, mResponse);
          break;
  #endif  // ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_48dB:
          svf[0].SetParams(freq, q48_1, mResponse);
          svf[2].SetParams(freq, q48_1, mResponse);
          svf[1].SetParams(freq, q48_2, mResponse);
          svf[3].SetParams(freq, q48_2, mResponse);
          break;
      }
      break;
    case FilterResponse::Allpass:
      switch (mSlope)
      {
        default:
  #ifdef ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_12dB:
          break;
  #endif  // ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_24dB:
          svf[0].SetParams(freq, q24, FilterResponse::Allpass);
          break;
  #ifdef ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_36dB:
          svf[0].SetParams(freq, 1.0f, FilterResponse::Allpass);
          break;
  #endif  // ENABLE_36db_oct_CROSSOVER

        case CrossoverSlope::Slope_48dB:
          svf[0].SetParams(freq, q48_1, FilterResponse::Allpass);
          svf[1].SetParams(freq, q48_2, FilterResponse::Allpass);
          break;
      }
      break;
    default:
      break;
  }

  apf1.SetParams(FilterCircuit::OnePole, FilterSlope::Slope6dbOct, FilterResponse::Allpass, freq, Param01(0), 0);
}

real LinkwitzRileyFilter::Process(real x)
{
  switch (mResponse)
  {
    case FilterResponse::Lowpass:
      switch (mSlope)
      {
        default:
  #ifdef ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_12dB:
          return svf[0].Process(x);
  #endif  // ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_24dB:
          x = svf[0].Process(x);
          return svf[1].Process(x);
  #ifdef ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_36dB:
          x = svf[0].Process(x);
          x = svf[1].Process(x);
          return svf[2].Process(x);
  #endif  // ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_48dB:
          x = svf[0].Process(x);
          x = svf[1].Process(x);
          x = svf[2].Process(x);
          return svf[3].Process(x);
      }
    case FilterResponse::Highpass:
      switch (mSlope)
      {
        default:
  #ifdef ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_12dB:
          return svf[0].Process(-x);
  #endif  // ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_24dB:
          x = svf[0].Process(x);
          return svf[1].Process(x);
  #ifdef ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_36dB:
          x = svf[0].Process(-x);
          x = svf[1].Process(x);
          return svf[2].Process(x);
  #endif  // ENABLE_36db_oct_CROSSOVER
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
  #ifdef ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_12dB:
          return apf1.ProcessSample(x);
  #endif  // ENABLE_12db_oct_CROSSOVER
        case CrossoverSlope::Slope_24dB:
          return svf[0].Process(x);
  #ifdef ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_36dB:
          x = svf[0].Process(x);
          return apf1.ProcessSample(x);
  #endif  // ENABLE_36db_oct_CROSSOVER
        case CrossoverSlope::Slope_48dB:
          x = svf[0].Process(x);
          return svf[1].Process(x);
      }
    default:
      return x;
  }
}

#endif  // FIXED_SLOPE_CROSSOVER_ONLY

}  // namespace WaveSabreCore::M7
