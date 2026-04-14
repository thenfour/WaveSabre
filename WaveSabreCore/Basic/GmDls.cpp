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

  // NB: can't use fopen or CreateFile, because they don't resolve the relative paths above.
  HANDLE file = INVALID_HANDLE_VALUE;
  for (int i = 0; file == INVALID_HANDLE_VALUE; i++)
  {
    OFSTRUCT reOpenBuff;
    file = (HANDLE)(UINT_PTR)OpenFile(gmDlsPaths[i], &reOpenBuff, OF_READ);
  }
  DWORD bytesRead;
  (void)ReadFile(file, gpData, kGmDlsFileSize, &bytesRead, NULL);
  ::CloseHandle(file);
}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
bool GmDls::TryGetLoopConfig(int sampleIndex, int& sampleLength, int& loopStart, int& loopLength)
{
  if (sampleIndex < 0 || sampleIndex >= kGmDlsSampleCount)
  {
    sampleLength = 0;
    loopStart = 0;
    loopLength = 0;
    return false;
  }

  auto ptr = gpData + kWaveListOffset;
  for (int i = 0; i <= sampleIndex; i++)
  {
    ptr += 4;
    auto waveListSize = *((unsigned int*)ptr);
    ptr += 4;

    if (i != sampleIndex)
    {
      ptr += waveListSize;
      continue;
    }

    auto wave = ptr + 4;
    auto fmtSize = *((unsigned int*)(wave + 4));
    wave += fmtSize + 8;

    WaveSabreCore::Wsmp wsmp;
    memcpy(&wsmp, wave, sizeof(wsmp));
    auto wsmpSize = *((unsigned int*)(wave + 4));
    wave += wsmpSize + 8;

    wave += 4;
    auto dataChunkSize = *((unsigned int*)wave);
    sampleLength = int(dataChunkSize / 2);

    if (wsmp.loopCount)
    {
      loopStart = wsmp.loopStart;
      loopLength = wsmp.loopLength;
    }
    else
    {
      loopStart = 0;
      loopLength = sampleLength;
    }
    return true;
  }

  sampleLength = 0;
  loopStart = 0;
  loopLength = 0;
  return false;
}
#endif

void GmDlsSample::LoadGmDlsIndex(int sampleIndex)
{
  mpSampleData = nullptr;
  mSampleLength = 0;

  if (sampleIndex < 0 || sampleIndex >= GmDls::kGmDlsSampleCount)
    return;

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

    auto fmtSize = *((unsigned int*)(wave + 4));
    wave += fmtSize + 8;

    auto wsmpSize = *((unsigned int*)(wave + 4));
    wave += wsmpSize + 8;

    // Read data chunk
    //auto dataChunkTag = *((unsigned int*)wave);  // Should be 'data'
    wave += 4;
    auto dataChunkSize = *((unsigned int*)wave);
    wave += 4;

    mpSampleData = (const int16_t*)wave;
    mSampleLength = int(dataChunkSize / 2);
    return;
  }
}

float GmDlsSample::GetSampleAt(int sampleOffset) const
{
  if (sampleOffset < 0 || sampleOffset >= mSampleLength)
    return 0.0f;
  return M7::math::Sample16To32Bit(mpSampleData[sampleOffset]);
}


}  // namespace WaveSabreCore
