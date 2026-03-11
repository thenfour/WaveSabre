
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

  GmDlsSample* mpSample = nullptr;

  // used by editor
  double samplePos;

private:
  int SampleLength() const
  {
    if (!mpSample) return 0;
    return int(mpSample->mSampleData.size());
  }

  bool IsActive;

  double sampleDelta;
  int roundedLoopStart;
  int roundedLoopLength;
  int roundedLoopEnd;
  bool reverse;
};
}  // namespace WaveSabreCore
