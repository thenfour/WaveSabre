#include "SamplePlayer.h"
#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore
{
SamplePlayer::SamplePlayer()
{
  SampleStart = 0.0f;
  mLoopEnabled = false;
  LoopStart = 0.0f;
  LoopLength = 1.0f;
  IsActive = false;
}

void SamplePlayer::SetPlayRate(double ratio)
{
  this->sampleDelta = ratio;
}

void SamplePlayer::InitPos()
{
  if (!mpSample)
    return;
  IsActive = true;
  samplePos = (double)SampleStart * (double)(SampleLength() - 1);
}

void SamplePlayer::RunPrep()
{
  if (!mpSample)
    return;
  auto lengthSamplesI = SampleLength();
  auto lengthSamplesF = (float)lengthSamplesI;
  roundedLoopStart = (int)(lengthSamplesF * LoopStart);
  roundedLoopLength = (int)(lengthSamplesF * LoopLength);

  if (roundedLoopLength < 1)
    roundedLoopLength = 1;

  if (roundedLoopStart >= lengthSamplesI)
    roundedLoopStart = lengthSamplesI - 1;

  roundedLoopEnd = roundedLoopStart + roundedLoopLength;
  if (roundedLoopEnd > lengthSamplesI)
  {
    roundedLoopEnd = lengthSamplesI;
    roundedLoopLength = roundedLoopEnd - roundedLoopStart;
  }
}

float SamplePlayer::Next()
{
  if (!mpSample)
    return 0;
  double samplePosFloor = M7::math::floord(samplePos);
  double samplePosFract = samplePos - samplePosFloor;

  auto lengthSamplesI = SampleLength();
  int roundedSamplePos = (int)samplePosFloor;
  if (roundedSamplePos < 0 || roundedSamplePos >= (int)lengthSamplesI)
  {
    IsActive = false;
    return 0.0f;
  }

  // calculate next sample. needs to consider:
  // - reverse playback,
  // - loop mode (which can cause samplepos to jump around, and thus cause the next sample to be non-adjacent to the current sample)
  int rightIndex = roundedSamplePos + 1;
  if (mLoopEnabled && rightIndex == roundedLoopEnd)
    rightIndex = roundedLoopStart;
  float leftSample = mpSample->GetSampleAt(roundedSamplePos);
  float rightSample = mpSample->GetSampleAt(rightIndex);
  float sample = M7::math::lerp(leftSample, rightSample, (float)samplePosFract);

  samplePos += sampleDelta;

  if (mLoopEnabled)
  {
    while (samplePos >= (double)roundedLoopEnd)
      samplePos -= (double)roundedLoopLength;
  }

  return sample;
}
}  // namespace WaveSabreCore
