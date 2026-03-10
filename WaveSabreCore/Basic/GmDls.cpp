#include "GmDls.h"

#include <Windows.h>

namespace WaveSabreCore
{
GmDls::GmDls()
{
  static const char* gmDlsPaths[2] = {
      "drivers/gm.dls",      //
      "drivers/etc/gm.dls",  //
  };

  HANDLE gmDlsFile = INVALID_HANDLE_VALUE;
  for (int i = 0; gmDlsFile == INVALID_HANDLE_VALUE; i++)
  {
    OFSTRUCT reOpenBuff;
    gmDlsFile = (HANDLE)(UINT_PTR)OpenFile(gmDlsPaths[i], &reOpenBuff, OF_READ);
  }

  auto gmDlsFileSize = GetFileSize(gmDlsFile, NULL);
  auto gmDlsData = new uint8_t[gmDlsFileSize];
  unsigned int bytesRead;
  ReadFile(gmDlsFile, gmDlsData, gmDlsFileSize, (LPDWORD)&bytesRead, NULL);
  CloseHandle(gmDlsFile);

  mpData = gmDlsData;
}

GmDls::~GmDls()
{
  delete[] mpData;
}

void GmDlsSample::LoadGmDlsIndex(int sampleIndex)
{
  if (sampleIndex >= kGmDlsSampleCount)
    return;
  mSampleIndex = sampleIndex;

  // Seek to wave pool chunk's data
  auto ptr = mGmDls.mpData + GmDls::kWaveListOffset;

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
