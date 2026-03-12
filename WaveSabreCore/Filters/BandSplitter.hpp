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
  static constexpr int kMagnitudeImpulseTaps = 96;
#endif

  LinkwitzRileyFilter mLR[kBandSplitterBands * 2];

public:
  void SetParams(float crossoverFreqA,
                 CrossoverSlope crossoverSlopeA,
                 float crossoverFreqB,
                 CrossoverSlope crossoverSlopeB)
  {
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
    // FrequencySplitter probe = *this;
    // probe.Reset();

    // std::array<float, kBandSplitterBands> real{};
    // std::array<float, kBandSplitterBands> imag{};
    // const float phaseStep = M7::PITimes2 * freqHz * Helpers::CurrentSampleRateRecipF;

    // for (int i = 0; i < kMagnitudeImpulseTaps; ++i)
    // {
    //   const float input = (i == 0) ? 1.0f : 0.0f;
    //   const auto y = probe.frequency_splitter(input);
    //   const float phase = phaseStep * float(i);
    //   const float c = math::cos(phase);
    //   const float s = math::sin(phase);

    //   for (int band = 0; band < kBandSplitterBands; ++band)
    //   {
    //     real[band] += y.s[band] * c;
    //     imag[band] -= y.s[band] * s;
    //   }
    // }

    // std::array<float, kBandSplitterBands> ret{};
    // for (int band = 0; band < kBandSplitterBands; ++band)
    // {
    //   ret[band] = sqrtf(real[band] * real[band] + imag[band] * imag[band]);
    // }
    // return ret;
    (void)freqHz;
    return {};
  }
#endif
};


}  // namespace M7


}  // namespace WaveSabreCore
