
#include "BandSplitter.hpp"

namespace WaveSabreCore::M7
{
void BandSplitter::SetParams(float crossoverFreqA,
                             CrossoverSlope crossoverSlopeA,
                             float crossoverFreqB,
                             CrossoverSlope crossoverSlopeB)
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  mCrossoverFreqA = crossoverFreqA;
  mCrossoverFreqB = crossoverFreqB;
  mCrossoverSlopeA = crossoverSlopeA;
  mCrossoverSlopeB = crossoverSlopeB;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  mLR[1].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Lowpass);
  mLR[3].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Lowpass);

  mLR[0].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Lowpass);
  mLR[2].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Highpass);
  mLR[4].SetParams(crossoverFreqA, crossoverSlopeA, FilterResponse::Allpass);

  mLR[5].SetParams(crossoverFreqB, crossoverSlopeB, FilterResponse::Highpass);
}

BandSplitterOutput BandSplitter::Process(float x)
{
  float a = mLR[0].Process(x);
  a = mLR[1].Process(a);

  float b = mLR[2].Process(x);
  b = mLR[3].Process(b);

  float c = mLR[4].Process(x);
  c = mLR[5].Process(c);

  return {a, b, c};
}

}  // namespace WaveSabreCore::M7
