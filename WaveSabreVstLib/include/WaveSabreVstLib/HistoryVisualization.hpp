#ifndef __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__
#define __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__

#include "Common.h"
#include <imgui.h>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <deque>
#include <array>

using namespace WaveSabreCore;

namespace WaveSabreVstLib {


struct HistoryViewSeriesConfig {
  ImColor mLineColor;
  float mLineThickness;
  
  // Per-series Y scale configuration
  float mMinValue = -1.0f;  // Minimum value for this series
  float mMaxValue = 2.0f;   // Maximum value for this series
  bool mUseCustomScale = false; // If false, uses global scale from RenderCustom parameters
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

  // Custom render method for non-dB values (like stereo phase correlation, width, balance)
  void RenderCustom(const std::array<HistoryViewSeriesConfig, TSeriesCount> &cfg,
                    const std::array<float, TSeriesCount> &values,
                    float globalMinValue = -1.0f, float globalMaxValue = 2.0f) {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = bb.Min + gHistViewSize;

    for (size_t i = 0; i < TSeriesCount; ++i) {
      float val = values[i];
      // Store raw values instead of converting to dB
      mSeries[i].mHistDecibels.push_back(val);
      if (mSeries[i].mHistDecibels.size() > gSamplesInHist) {
        mSeries[i].mHistDecibels.pop_front();
      }
    }

    ImColor backgroundColor = ColorFromHTML("222222", 1.0f);

    // Lambda to get per-series value-to-Y mapping
    auto ValueToYForSeries = [&](float value, size_t seriesIndex) {
      // Choose scale for this series
      float minVal = cfg[seriesIndex].mUseCustomScale ? cfg[seriesIndex].mMinValue : globalMinValue;
      float maxVal = cfg[seriesIndex].mUseCustomScale ? cfg[seriesIndex].mMaxValue : globalMaxValue;
      
      float range = maxVal - minVal;
      if (range <= 0.0001f) return bb.Min.y; // Avoid division by zero
      float normalized = (value - minVal) / range;
      normalized = M7::math::clamp01(normalized);
      return M7::math::lerp(bb.Max.y, bb.Min.y, normalized);
    };

    // Global value-to-Y mapping for reference lines (uses global scale)
    auto GlobalValueToY = [&](float value) {
      float range = globalMaxValue - globalMinValue;
      if (range <= 0.0001f) return bb.Min.y; // Avoid division by zero
      float normalized = (value - globalMinValue) / range;
      normalized = M7::math::clamp01(normalized);
      return M7::math::lerp(bb.Max.y, bb.Min.y, normalized);
    };

    auto SampleToX = [&](int sample) {
      float sx = (float)sample;
      sx /= gSamplesInHist; // 0,1
      return M7::math::lerp(bb.Min.x, bb.Max.x, sx);
    };

    auto *dl = ImGui::GetWindowDrawList();

    ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

    // Enhanced reference lines for stereo parameters (using global scale)
    //ImU32 subtleLineColor = ColorFromHTML("333333", 0.4f);
    //ImU32 centerLineColor = ColorFromHTML("555555", 0.7f);  // More prominent center line
    
    // Center/zero line (most important reference)
    //if (globalMinValue <= 0.0f && globalMaxValue >= 0.0f) {
    //  float centerY = GlobalValueToY(0.0f);
    //  dl->AddLine({bb.Min.x, centerY}, {bb.Max.x, centerY}, centerLineColor, 1.5f);
    //  
    //  // Add small center indicator text
    //  if (gHistViewSize.x > 100) {  // Only show if there's enough space
    //    dl->AddText({bb.Min.x + 4, centerY - 8}, centerLineColor, "0");
    //  }
    //}
    //
    //// Additional reference lines for common stereo ranges
    //if (globalMinValue <= -1.0f && globalMaxValue >= 1.0f) {
    //  // -1 and +1 lines for correlation and balance ranges
    //  float minLineY = GlobalValueToY(-1.0f);
    //  float maxLineY = GlobalValueToY(1.0f);
    //  dl->AddLine({bb.Min.x, minLineY}, {bb.Max.x, minLineY}, subtleLineColor, 1.0f);
    //  dl->AddLine({bb.Min.x, maxLineY}, {bb.Max.x, maxLineY}, subtleLineColor, 1.0f);
    //  
    //  // Add range labels
    //  if (gHistViewSize.x > 100) {
    //    dl->AddText({bb.Min.x + 4, minLineY - 8}, subtleLineColor, "-1");
    //    dl->AddText({bb.Min.x + 4, maxLineY - 8}, subtleLineColor, "+1");
    //  }
    //}
    //
    //// Width reference line at 1.0 (normal stereo width)
    //if (globalMinValue <= 1.0f && globalMaxValue >= 1.0f) {
    //  float normalWidthY = GlobalValueToY(1.0f);
    //  dl->AddLine({bb.Min.x, normalWidthY}, {bb.Max.x, normalWidthY}, ColorFromHTML("444444", 0.5f), 1.0f);
    //  
    //  if (gHistViewSize.x > 100) {
    //    dl->AddText({bb.Min.x + 4, normalWidthY - 8}, ColorFromHTML("444444", 0.8f), "1");
    //  }
    //}

    // Render series lines (each using their own scale)
    for (size_t iSeries = 0; iSeries < TSeriesCount; ++iSeries) {
      std::vector<ImVec2> points;
      for (int isample = 0;
           isample < ((signed)mSeries[iSeries].mHistDecibels.size());
           ++isample) {
        points.push_back({SampleToX(isample),
                          ValueToYForSeries(mSeries[iSeries].mHistDecibels[isample], iSeries)});
      }
      if (points.size() > 1) {
        dl->AddPolyline(points.data(), (int)points.size(),
                        cfg[iSeries].mLineColor, 0, cfg[iSeries].mLineThickness);
      }
    }

    ImGui::Dummy(gHistViewSize);
  }
};

} // namespace WaveSabreVstLib

#endif
