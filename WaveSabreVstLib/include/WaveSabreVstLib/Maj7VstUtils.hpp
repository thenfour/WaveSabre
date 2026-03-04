#ifndef __WAVESABREVSTLIB_MAJ7VSTUTILS_H__
#define __WAVESABREVSTLIB_MAJ7VSTUTILS_H__

#include "Common.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <vector>
#include <string>
#include <algorithm>

using namespace WaveSabreCore;

namespace WaveSabreVstLib {
    static constexpr double gNormalKnobSpeed = 0.0015;
    static constexpr double gSlowKnobSpeed = 0.000003;

    static constexpr double gNormalKnobSpeedInt = 0.0030;
    static constexpr double gSlowKnobSpeedInt = 0.00001;

    // Utility function used by tab system
    inline ImColor ColorFromHTML(const char* buf, float alpha = 1.0f) {
        int i[3] = { 0 };
        const char* p = buf;
        while (*p == '#' || ImCharIsBlankA(*p))
            p++;
        i[0] = i[1] = i[2] = 0;
        int r = sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0],
            (unsigned int*)&i[1], (unsigned int*)&i[2]);
        float f[3] = { 0 };
        for (int n = 0; n < 3; n++)
            f[n] = i[n] / 255.0f;
        return ImColor{ f[0], f[1], f[2], alpha };
    }

    inline ImVec2 CalcListBoxSize(float items) {
        return { 0.0f, ImGui::GetTextLineHeightWithSpacing() * items +
                          ImGui::GetStyle().FramePadding.y * 2.0f };
    }


inline std::vector<std::pair<std::string, int>>
autocomplete(std::string input,
             const std::vector<std::pair<std::string, int>> &entries) {
  std::vector<std::pair<std::string, int>> suggestions;
  std::transform(input.begin(), input.end(), input.begin(),
                 ::tolower); // convert input to lowercase
  for (auto entry : entries) {
    std::string lowercaseEntry = entry.first;
    std::transform(lowercaseEntry.begin(), lowercaseEntry.end(),
                   lowercaseEntry.begin(),
                   ::tolower); // convert entry to lowercase
    int inputIndex = 0, entryIndex = 0;
    while ((size_t)inputIndex < input.size() &&
           (size_t)entryIndex < lowercaseEntry.size()) {
      if (input[inputIndex] == lowercaseEntry[entryIndex]) {
        inputIndex++;
      }
      entryIndex++;
    }
    if (inputIndex == input.size()) {
      suggestions.push_back(entry);
    }
  }
  return suggestions;
}

static inline std::vector<std::pair<std::string, int>> LoadGmDlsOptions() {
  std::vector<std::pair<std::string, int>> mOptions;
  mOptions.push_back({"(no sample)", -1});

  // Read gm.dls file
  auto gmDls = GmDls::Load();

  // Seek to wave pool chunk's data
  auto ptr = gmDls + GmDls::WaveListOffset;

  // Walk wave pool entries
  for (int i = 0; i < M7::gGmDlsSampleCount; i++) {
    // Walk wave list
    auto waveListTag = *((unsigned int *)ptr); // Should be 'LIST'
    ptr += 4;
    auto waveListSize = *((unsigned int *)ptr);
    ptr += 4;

    // Walk wave entry
    auto wave = ptr;
    auto waveTag = *((unsigned int *)wave); // Should be 'wave'
    wave += 4;

    // Skip fmt chunk
    auto fmtChunkTag = *((unsigned int *)wave); // Should be 'fmt '
    wave += 4;
    auto fmtChunkSize = *((unsigned int *)wave);
    wave += 4;
    wave += fmtChunkSize;

    // Skip wsmp chunk
    auto wsmpChunkTag = *((unsigned int *)wave); // Should be 'wsmp'
    wave += 4;
    auto wsmpChunkSize = *((unsigned int *)wave);
    wave += 4;
    wave += wsmpChunkSize;

    // Skip data chunk
    auto dataChunkTag = *((unsigned int *)wave); // Should be 'data'
    wave += 4;
    auto dataChunkSize = *((unsigned int *)wave);
    wave += 4;
    wave += dataChunkSize;

    // Walk info list
    auto infoList = wave;
    auto infoListTag = *((unsigned int *)infoList); // Should be 'LIST'
    infoList += 4;
    auto infoListSize = *((unsigned int *)infoList);
    infoList += 4;

    // Walk info entry
    auto info = infoList;
    auto infoTag = *((unsigned int *)info); // Should be 'INFO'
    info += 4;

    // Skip copyright chunk
    auto icopChunkTag = *((unsigned int *)info); // Should be 'ICOP'
    info += 4;
    auto icopChunkSize = *((unsigned int *)info);
    info += 4;
    // This size appears to be the size minus null terminator, yet each entry
    // has a null terminator anyways, so they all seem to end in 00 00. Not sure why.
    info += icopChunkSize + 1;

    // Read name (finally :D)
    auto nameChunkTag = *((unsigned int *)info); // Should be 'INAM'
    info += 4;
    auto nameChunkSize = *((unsigned int *)info);
    info += 4;

    // Insert name into appropriate group
    auto name = std::string((char *)info);
    mOptions.push_back({name, i});

    ptr += waveListSize;
  }

  delete[] gmDls;
  return mOptions;
}


} // namespace WaveSabreVstLib

#endif
