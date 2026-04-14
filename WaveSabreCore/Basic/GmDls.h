#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Basic/PodVector.hpp"


namespace WaveSabreCore
{
struct GmDls
{
  GmDls() = delete;
  static constexpr int kGmDlsSampleCount = M7::gGmDlsSampleCount;
  static constexpr int kGmDlsFileSize = 3440660;
  static constexpr int kWaveListOffset = 0x00044602;
  static uint8_t *gpData;
  static void EnsureInitialized();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static bool TryGetLoopConfig(int sampleIndex, int& sampleLength, int& loopStart, int& loopLength);
#endif
  // GmDls();
  // ~GmDls();
};

struct GmDlsSample
{
  static constexpr int kSampleRate = 44100;
  int mSampleIndex = 0;

  M7::PodVector<float> mSampleData;

  void LoadGmDlsIndex(int index);
};


typedef struct
{
  char tag[4];
  unsigned int size;
  short wChannels;
  int dwSamplesPerSec;
  int dwAvgBytesPerSec;
  short wBlockAlign;
} Fmt;

typedef struct
{
  char tag[4];
  unsigned int size;
  unsigned short unityNote;
  short fineTune;
  int gain;
  int attenuation;
  unsigned int fulOptions;
  unsigned int loopCount;
  unsigned int loopSize;
  unsigned int loopType;
  unsigned int loopStart;
  unsigned int loopLength;
} Wsmp;


}  // namespace WaveSabreCore
