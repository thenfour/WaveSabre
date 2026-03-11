#include "SamplePlayer.h"
#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore
{
SamplePlayer::SamplePlayer()
{
  SampleStart = 0.0f;
  Reverse = false;
  LoopMode = LoopMode::Repeat;
  LoopBoundaryMode = LoopBoundaryMode::FromSample;
  LoopStart = 0.0f;
  LoopLength = 1.0f;
  IsActive = false;
}

void SamplePlayer::SetPlayRate(double ratio)
{
  this->sampleDelta = reverse ? -ratio : ratio;
}

void SamplePlayer::InitPos()
{
  if (!mpSample)
    return;
  reverse = Reverse;
  IsActive = true;
  samplePos = (double)SampleStart * (double)(SampleLength() - 1);
  if (reverse)
  {
    samplePos = 1.0 - samplePos;
  }
}

void SamplePlayer::RunPrep()
{
  if (!mpSample)
    return;
  auto lengthSamplesI = SampleLength();
  auto lengthSamplesF = (float)lengthSamplesI;
  switch (LoopBoundaryMode)
  {
    case LoopBoundaryMode::FromSample:
      roundedLoopStart = mpSample->mSampleLoopStart;
      roundedLoopLength = mpSample->mSampleLoopLength;
      break;

    case LoopBoundaryMode::Manual:
      roundedLoopStart = (int)(lengthSamplesF * LoopStart);
      roundedLoopLength = (int)(lengthSamplesF * LoopLength);
      break;
  }

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

  float leftSample = mpSample->mSampleData[roundedSamplePos];

  // calculate next sample. needs to consider:
  // - reverse playback,
  // - loop mode (which can cause samplepos to jump around, and thus cause the next sample to be non-adjacent to the current sample)
  int rightIndex = roundedSamplePos + 1;
  if (LoopMode == LoopMode::Repeat && rightIndex == roundedLoopEnd)
    rightIndex = roundedLoopStart;
  float rightSample = rightIndex < lengthSamplesI ? mpSample->mSampleData[rightIndex] : 0.0f;
  float sample = M7::math::lerp(leftSample, rightSample, (float)samplePosFract);

  samplePos += sampleDelta;

  switch (LoopMode)
  {
    case LoopMode::Repeat:
      if (sampleDelta > 0.0)
      {
        while (samplePos >= (double)roundedLoopEnd)
          samplePos -= (double)roundedLoopLength;
      }
      else
      {
        while (samplePos < (double)roundedLoopStart)
          samplePos += (double)roundedLoopLength;
      }
      break;

    case LoopMode::PingPong:
      if (sampleDelta > 0.0 && samplePos >= (double)roundedLoopEnd)
      {
        samplePos = (double)(roundedLoopEnd - 1);
        sampleDelta = -sampleDelta;
        reverse = !reverse;
      }
      else if (sampleDelta < 0.0 && samplePos < (double)roundedLoopStart)
      {
        samplePos = (double)roundedLoopStart;
        sampleDelta = -sampleDelta;
        reverse = !reverse;
      }
      break;
  }

  return sample;
}
}  // namespace WaveSabreCore
