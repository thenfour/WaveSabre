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

// clang-format off
#define CROSSOVER_SLOPE_CAPTIONS(symbolName)                                                                           \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::CrossoverSlope::Count]                       \
  {                                                                                                                    \
    "6dB",\
	"12dB", \
	"24dB",\
	"36dB",\
	"48dB",                                                                             \
  }
// clang-format on

namespace WaveSabreCore
{
namespace M7
{
enum class CrossoverSlope : uint8_t
{
  Slope_6dB, // TODO: don't bother supporting this because it introduces complexity and code bloat.
  Slope_12dB,
  Slope_24dB,
  Slope_36dB,
  Slope_48dB,
  Count,
};


static constexpr int kBandSplitterBands = 3;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// mono signal processing.
struct BandSplitterOutput
{
  float s[kBandSplitterBands];

  float constexpr operator [](size_t index) const
  {
	return s[index];
  }
};
struct BandSplitter
{
private:
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static constexpr int kMagnitudeImpulseTaps = 96;
#endif

  LinkwitzRileyFilter mLR[kBandSplitterBands * 2];

  float mCrossoverFreqA;
  float mCrossoverFreqB;
  CrossoverSlope mCrossoverSlopeA;
  CrossoverSlope mCrossoverSlopeB;

public:

  void SetParams(float crossoverFreqA,
                 CrossoverSlope crossoverSlopeA,
                 float crossoverFreqB,
                 CrossoverSlope crossoverSlopeB)
  {
    // TODO: set LR filter parameters.
  }

  BandSplitterOutput Process(float x)
  {
    // TODO: implement processing based on the desired slope and response.

    //   float low = mLR[0].LR_LPF(x, cutoffHz, slope);//ProcessLow(0,  x, mCrossoverFreqA, mCrossoverSlopeA);
  //   low = mLR[1].LR_LPF(x, cutoffHz, slope);//ProcessLow(1, 2, low, mCrossoverFreqB, mCrossoverSlopeB);

  //   float mid = mLR[2].LR_HPF(x, cutoffHz, slope);//ProcessHigh(2, x, mCrossoverFreqA, mCrossoverSlopeA);
  //   mid = mLR[3].LR_LPF(x, cutoffHz, slope);//ProcessLow(3, out.s[1], mCrossoverFreqB, mCrossoverSlopeB);

	// out.s[2] = mLR[4].APF(x, mCrossoverFreqA, mCrossoverSlopeA);
	// out.s[2] = mLR[5].LR_HPF(out.s[2], mCrossoverFreqB, mCrossoverSlopeB);

    return out;
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  std::array<float, gMBBands> GetMagnitudesAtFrequency(float freqHz) const
  {

    // FrequencySplitter probe = *this;
    // probe.Reset();

    // std::array<float, gMBBands> real{};
    // std::array<float, gMBBands> imag{};
    // const float phaseStep = M7::PITimes2 * freqHz * Helpers::CurrentSampleRateRecipF;

    // for (int i = 0; i < kMagnitudeImpulseTaps; ++i)
    // {
    //   const float input = (i == 0) ? 1.0f : 0.0f;
    //   const auto y = probe.frequency_splitter(input);
    //   const float phase = phaseStep * float(i);
    //   const float c = math::cos(phase);
    //   const float s = math::sin(phase);

    //   for (int band = 0; band < gMBBands; ++band)
    //   {
    //     real[band] += y.s[band] * c;
    //     imag[band] -= y.s[band] * s;
    //   }
    // }

    // std::array<float, gMBBands> ret{};
    // for (int band = 0; band < gMBBands; ++band)
    // {
    //   ret[band] = sqrtf(real[band] * real[band] + imag[band] * imag[band]);
    // }
    // return ret;
  }
#endif
};


}  // namespace M7


}  // namespace WaveSabreCore
