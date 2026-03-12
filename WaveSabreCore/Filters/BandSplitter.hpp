#pragma once

//#include <WaveSabreCore/Maj7Basic.hpp>
//#include <WaveSabreCore/BiquadFilter.h>
#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "SVFilter.hpp"
#include <array>
#include <math.h>


#include "LinkwitzRileyFilter.hpp"

namespace WaveSabreCore
{
namespace M7
{
static constexpr int kBandSplitterBands = 3;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mono signal processing.
struct BandSplitterOutput
{
  float s[kBandSplitterBands];

  float constexpr operator[](size_t index) const
  {
    return s[index];
  }
};
struct BandSplitter
{
private:
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    float mCrossoverFreqA = 200.0f;
    float mCrossoverFreqB = 2000.0f;
    CrossoverSlope mCrossoverSlopeA = CrossoverSlope::Slope_24dB;
    CrossoverSlope mCrossoverSlopeB = CrossoverSlope::Slope_24dB;
#endif

  LinkwitzRileyFilter mLR[kBandSplitterBands * 2];
public:
  void SetParams(float crossoverFreqA,
                 CrossoverSlope crossoverSlopeA,
                 float crossoverFreqB,
                 CrossoverSlope crossoverSlopeB)
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    mCrossoverFreqA = crossoverFreqA;
    mCrossoverFreqB = crossoverFreqB;
    mCrossoverSlopeA = crossoverSlopeA;
    mCrossoverSlopeB = crossoverSlopeB;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

    mLR[1].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Lowpass);
    mLR[3].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Lowpass);

    mLR[0].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Lowpass);
    mLR[2].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Highpass);
    mLR[4].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Allpass);

    mLR[5].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Highpass);
  }

  BandSplitterOutput Process(float x)
  {
    float a = mLR[0].Process(x);
    a = mLR[1].Process(a);

    float b = mLR[2].Process(x);
    b = mLR[3].Process(b);

    float c = mLR[4].Process(x);
    c = mLR[5].Process(c);

    return { a, b, c};
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  std::array<float, kBandSplitterBands> GetMagnitudesAtFrequency(float freqHz) const
  {
    auto getLRIdealLowpassMagnitude = [](float freq, float cutoff, CrossoverSlope slope)
    {
      if (cutoff <= 0.0f)
      {
        return 0.0f;
      }

      const float ratio = freq / cutoff;
      const float ratio2 = ratio * ratio;

      float power;
      switch (slope)
      {
        default:
        case CrossoverSlope::Slope_12dB:
          power = ratio2;
          break;
        case CrossoverSlope::Slope_24dB:
          power = ratio2 * ratio2;
          break;
        case CrossoverSlope::Slope_36dB:
          power = ratio2 * ratio2 * ratio2;
          break;
        case CrossoverSlope::Slope_48dB:
          power = ratio2 * ratio2 * ratio2 * ratio2;
          break;
      }

      return 1.0f / (1.0f + power);
    };

    const float lowA = getLRIdealLowpassMagnitude(freqHz, mCrossoverFreqA, mCrossoverSlopeA);
    const float highA = 1.0f - lowA;
    const float lowB = getLRIdealLowpassMagnitude(freqHz, mCrossoverFreqB, mCrossoverSlopeB);
    const float highB = 1.0f - lowB;

    return {lowA * lowB, highA * lowB, highB};
  }
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
};


}  // namespace M7


}  // namespace WaveSabreCore
