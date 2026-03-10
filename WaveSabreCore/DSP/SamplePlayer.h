
#pragma once

#include <cstdint>
#include "../Basic/PodVector.hpp"
#include "../Basic/GmDls.h"

namespace WaveSabreCore
{

enum class LoopMode  // : uint8_t
{
  Disabled,
  Repeat,
  PingPong,

  NumLoopModes,
};

enum class LoopBoundaryMode  // : uint8_t
{
  FromSample,
  Manual,

  NumLoopBoundaryModes,
};

class SamplePlayer
{
public:
  SamplePlayer();

  void SetPlayRate(double ratio);
  void InitPos();
  void RunPrep();
  float Next();

  bool Reverse;
  WaveSabreCore::LoopMode LoopMode;
  WaveSabreCore::LoopBoundaryMode LoopBoundaryMode;

  float SampleStart;
  float LoopStart;
  float LoopLength;

  GmDlsSample* mpSample;

  // used by editor
  double samplePos;

private:
  size_t SampleLength() const
  {
    return mpSample->mSampleData.size();
  }

  bool IsActive;

  // for linear interpolation, use a 1-sample delay and store previous sample value.
  float mPrevSample = 0;

  double sampleDelta;
  size_t roundedLoopStart;
  size_t roundedLoopLength;
  size_t roundedLoopEnd;
  bool reverse;
};
}  // namespace WaveSabreCore
