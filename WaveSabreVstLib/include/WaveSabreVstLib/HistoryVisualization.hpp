#ifndef __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__
#define __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__

#include "Common.h"
#include "../imgui/imgui.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <deque>
#include <array>

using namespace WaveSabreCore;

namespace WaveSabreVstLib {


struct HistoryViewSeriesConfig {
  ImColor mLineColor;
  float mLineThickness;
};

struct HistoryViewSeries {
  std::deque<float> mHistDecibels;

  HistoryViewSeries() {
    // mFollower.SetParams(0, 100);
  }
};

static constexpr float gHistoryViewMinDB = -50;

template <size_t TSeriesCount, int Twidth, int Theight> struct HistoryView {
  static constexpr ImVec2 gHistViewSize = {Twidth, Theight};
  static constexpr float gPixelsPerSample = 4;
  static constexpr int gSamplesInHist =
      (int)(gHistViewSize.x / gPixelsPerSample);

  HistoryViewSeries mSeries[TSeriesCount];

  void Render(const std::array<HistoryViewSeriesConfig, TSeriesCount> &cfg,
              const std::array<float, TSeriesCount> &linValues) {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = bb.Min + gHistViewSize;

    for (size_t i = 0; i < TSeriesCount; ++i) {
      float lin = linValues[i];

      mSeries[i].mHistDecibels.push_back(M7::math::LinearToDecibels(lin));
      if (mSeries[i].mHistDecibels.size() > gSamplesInHist) {
        mSeries[i].mHistDecibels.pop_front();
      }
    }

    ImColor backgroundColor = ColorFromHTML("222222", 1.0f);

    auto DbToY = [&](float db) {
      // let's show a range of -x to 0 db.
      float x = (db - gHistoryViewMinDB) / -gHistoryViewMinDB;
      x = M7::math::clamp01(x);
      return M7::math::lerp(bb.Max.y, bb.Min.y, x);
    };
    auto SampleToX = [&](int sample) {
      float sx = (float)sample;
      sx /= gSamplesInHist; // 0,1
      return M7::math::lerp(bb.Min.x, bb.Max.x, sx);
    };

    auto *dl = ImGui::GetWindowDrawList();

    ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

    for (size_t iSeries = 0; iSeries < TSeriesCount; ++iSeries) {
      std::vector<ImVec2> points;
      for (int isample = 0;
           isample < ((signed)mSeries[iSeries].mHistDecibels.size());
           ++isample) {
        points.push_back({SampleToX(isample),
                          DbToY(mSeries[iSeries].mHistDecibels[isample])});
      }
      dl->AddPolyline(points.data(), (int)points.size(),
                      cfg[iSeries].mLineColor, 0, cfg[iSeries].mLineThickness);
    }

    ImGui::Dummy(gHistViewSize);
  }
};

} // namespace WaveSabreVstLib

#endif
