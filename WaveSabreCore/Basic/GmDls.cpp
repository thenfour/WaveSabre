#include "GmDls.h"

#include <Windows.h>

namespace WaveSabreCore
{

uint8_t* GmDls::gpData = nullptr;

void GmDls::EnsureInitialized()
{
  if (gpData)
    return;
  static const char* gmDlsPaths[2] = {
      "drivers/gm.dls",      //
      "drivers/etc/gm.dls",  //
  };
  gpData = new uint8_t[kGmDlsFileSize];
#pragma message("GmDls Leaking memory to save bits.")

  FILE* file = nullptr;
  for (int i = 0; !file; i++)
  {
    file = fopen(gmDlsPaths[i], "rb");
  }
  fread(gpData, 1, kGmDlsFileSize, file);
  fclose(file);
}

void GmDlsSample::LoadGmDlsIndex(int sampleIndex)
{
  GmDls::EnsureInitialized();
  mSampleIndex = sampleIndex;

  // Seek to wave pool chunk's data
  auto ptr = GmDls::gpData + GmDls::kWaveListOffset;

  // Walk wave pool entries
  for (int i = 0; i <= sampleIndex; i++)
  {
    // Walk wave list
    //auto waveListTag = *((unsigned int*)ptr);  // Should be 'LIST'
    ptr += 4;
    auto waveListSize = *((unsigned int*)ptr);
    ptr += 4;

    // Skip entries until we've reached the index we're looking for
    if (i != sampleIndex)
    {
      ptr += waveListSize;
      continue;
    }

    // Walk wave entry
    auto wave = ptr;
    //auto waveTag = *((unsigned int*)wave);  // Should be 'wave'
    wave += 4;

    // Read fmt chunk
    WaveSabreCore::Fmt fmt;
    memcpy(&fmt, wave, sizeof(fmt));
    wave += fmt.size + 8;  // size field doesn't account for tag or length fields

    // Read wsmp chunk
    WaveSabreCore::Wsmp wsmp;
    memcpy(&wsmp, wave, sizeof(wsmp));
    wave += wsmp.size + 8;  // size field doesn't account for tag or length fields

    // Read data chunk
    //auto dataChunkTag = *((unsigned int*)wave);  // Should be 'data'
    wave += 4;
    auto dataChunkSize = *((unsigned int*)wave);
    wave += 4;

    // Data format is assumed to be mono 16-bit signed PCM
    auto sampleLength = int(dataChunkSize / 2);
    mSampleData.resize(sampleLength);
    int16_t* wave16 = (int16_t*)wave;
    for (int j = 0; j < sampleLength; j++)
    {
      auto sample = wave16[j];
      mSampleData[j] = M7::math::Sample16To32Bit(sample);
    }

    if (wsmp.loopCount)
    {
      mSampleLoopStart = wsmp.loopStart;
      mSampleLoopLength = wsmp.loopLength;
    }
    else
    {
      mSampleLoopStart = 0;
      mSampleLoopLength = sampleLength;
    }
  }
}


}  // namespace WaveSabreCore
