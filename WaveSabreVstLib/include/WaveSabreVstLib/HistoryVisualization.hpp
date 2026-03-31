#ifndef __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__
#define __WAVESABREVSTLIB_HISTORYVISUALIZATION_H__

#include "Common.h"
#include <imgui.h>
#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <deque>
#include <array>
#include <cmath>
#include <cstdio>

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

enum class HistoryTooltipValueFormat {
  StoredDecibels,
  LinearToDecibels,
  SignedFloat,
  UnsignedFloat,
};

struct HistoryTooltipSeriesConfig {
  const char* mLabel = nullptr;
  ImColor mSwatchColor;
  bool mShowInTooltip = true;
  HistoryTooltipValueFormat mValueFormat = HistoryTooltipValueFormat::StoredDecibels;
  int mPrecision = 1;
  const char* mUnitSuffix = "";

  HistoryTooltipSeriesConfig() = default;

  HistoryTooltipSeriesConfig(const char* label,
                             ImColor swatchColor,
                             bool showInTooltip,
                             HistoryTooltipValueFormat valueFormat = HistoryTooltipValueFormat::StoredDecibels,
                             int precision = 1,
                             const char* unitSuffix = "")
      : mLabel(label)
      , mSwatchColor(swatchColor)
      , mShowInTooltip(showInTooltip)
      , mValueFormat(valueFormat)
      , mPrecision(precision)
      , mUnitSuffix(unitSuffix)
  {
  }
};

static inline void HistoryTooltipFormatValue(char* buffer,
                                             size_t bufferSize,
                                             float value,
                                             const HistoryTooltipSeriesConfig& cfg) {
  float displayValue = value;

  switch (cfg.mValueFormat) {
    case HistoryTooltipValueFormat::StoredDecibels:
      if (!std::isfinite(displayValue)) {
        std::snprintf(buffer, bufferSize, " -inf dB");
        return;
      }
      std::snprintf(buffer, bufferSize, "%+6.1f dB", displayValue);
      return;

    case HistoryTooltipValueFormat::LinearToDecibels:
      displayValue = M7::math::LinearToDecibels(std::fabs(displayValue));
      if (!std::isfinite(displayValue)) {
        std::snprintf(buffer, bufferSize, " -inf dB");
        return;
      }
      std::snprintf(buffer, bufferSize, "%+6.1f dB", displayValue);
      return;

    case HistoryTooltipValueFormat::SignedFloat:
      std::snprintf(buffer,
                    bufferSize,
                    "%+.*f%s%s",
                    cfg.mPrecision,
                    displayValue,
                    (cfg.mUnitSuffix && cfg.mUnitSuffix[0]) ? " " : "",
                    (cfg.mUnitSuffix && cfg.mUnitSuffix[0]) ? cfg.mUnitSuffix : "");
      return;

    case HistoryTooltipValueFormat::UnsignedFloat:
      std::snprintf(buffer,
                    bufferSize,
                    "%.*f%s%s",
                    cfg.mPrecision,
                    displayValue,
                    (cfg.mUnitSuffix && cfg.mUnitSuffix[0]) ? " " : "",
                    (cfg.mUnitSuffix && cfg.mUnitSuffix[0]) ? cfg.mUnitSuffix : "");
      return;
  }
}

static inline void HistoryTooltipNameCell(const HistoryTooltipSeriesConfig& cfg) {
  if (cfg.mSwatchColor.Value.w > 0.0f) {
    const float swatchSize = std::floor(ImGui::GetTextLineHeight() * 0.60f);
    ImVec2 swatchMin = ImGui::GetCursorScreenPos();
    swatchMin.y += std::floor((ImGui::GetTextLineHeight() - swatchSize) * 0.5f);
    ImVec2 swatchMax = {swatchMin.x + swatchSize, swatchMin.y + swatchSize};
    ImGui::GetWindowDrawList()->AddRectFilled(swatchMin, swatchMax, cfg.mSwatchColor, 1.5f);
    ImGui::Dummy({swatchSize, 0.0f});
    ImGui::SameLine(0, 6.0f);
  }

  ImGui::TextUnformatted(cfg.mLabel ? cfg.mLabel : "");
}

static inline void HistoryTooltipValueCell(const char* value) {
  const float available = ImGui::GetContentRegionAvail().x;
  const float textWidth = ImGui::CalcTextSize(value).x;
  if (available > textWidth) {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - textWidth);
  }
  ImGui::TextUnformatted(value);
}

static constexpr float gHistoryViewMinDB = -50;

template <size_t TSeriesCount, int Twidth, int Theight> struct HistoryView {
  static constexpr ImVec2 gHistViewSize = {Twidth, Theight};
  static constexpr float gPixelsPerSample = 4;
  static constexpr int gSamplesInHist =
      (int)(gHistViewSize.x / gPixelsPerSample);
  static constexpr ImVec2 gOverlayButtonPadding = {6.0f, 6.0f};

  HistoryViewSeries mSeries[TSeriesCount];
  std::deque<double> mTimestamps;
  bool mPaused = false;

  float SampleToX(int sample, const ImRect& bb) const {
    float sx = (float)sample;
    sx /= gSamplesInHist;
    return M7::math::lerp(bb.Min.x, bb.Max.x, sx);
  }

  template<typename TValueArray>
  void PushSamples(const TValueArray& values) {
    mTimestamps.push_back(ImGui::GetTime());
    if (mTimestamps.size() > gSamplesInHist) {
      mTimestamps.pop_front();
    }

    for (size_t i = 0; i < TSeriesCount; ++i) {
      mSeries[i].mHistDecibels.push_back(values[i]);
      if (mSeries[i].mHistDecibels.size() > gSamplesInHist) {
        mSeries[i].mHistDecibels.pop_front();
      }
    }
  }

  int FindClosestSampleIndex(const ImRect& bb, float mouseX) const {
    if (mTimestamps.empty()) {
      return -1;
    }

    int bestIndex = 0;
    float bestDistance = std::fabs(mouseX - SampleToX(0, bb));
    for (int i = 1; i < (int)mTimestamps.size(); ++i) {
      const float distance = std::fabs(mouseX - SampleToX(i, bb));
      if (distance < bestDistance) {
        bestDistance = distance;
        bestIndex = i;
      }
    }

    return bestIndex;
  }

  bool TryGetHoveredSample(const ImRect& bb, int& hoveredIndex, float& hoveredX, float& hoveredY) const {
    if (mTimestamps.empty() || !ImGui::IsMouseHoveringRect(bb.Min, bb.Max, true)) {
      return false;
    }

    hoveredIndex = FindClosestSampleIndex(bb, ImGui::GetIO().MousePos.x);
    if (hoveredIndex < 0 || hoveredIndex >= (int)mTimestamps.size()) {
      return false;
    }

    hoveredX = SampleToX(hoveredIndex, bb);
    hoveredY = ImGui::GetIO().MousePos.y;
    hoveredY = hoveredY < bb.Min.y ? bb.Min.y : hoveredY;
    hoveredY = hoveredY > bb.Max.y ? bb.Max.y : hoveredY;
    return true;
  }

  void RenderPauseButton(const ImRect& bb) {
    const ImVec2 savedCursorPos = ImGui::GetCursorPos();
    ImGui::PushID(this);
    ImGui::SetCursorScreenPos({bb.Min.x + gOverlayButtonPadding.x, bb.Min.y + gOverlayButtonPadding.y});
    if (ImGui::SmallButton(mPaused ? "Resume" : "Pause")) {
      mPaused = !mPaused;
    }
    ImGui::PopID();
    ImGui::SetCursorPos(savedCursorPos);
  }

  template<typename TValueToY>
  void RenderHoverOverlay(const ImRect& bb,
                          const std::array<HistoryViewSeriesConfig, TSeriesCount>& renderCfg,
                          const TValueToY& valueToY) {
    int hoveredIndex = -1;
    float hoveredX = 0.0f;
    float hoveredY = 0.0f;
    if (!TryGetHoveredSample(bb, hoveredIndex, hoveredX, hoveredY)) {
      return;
    }

    auto* dl = ImGui::GetWindowDrawList();
    const ImU32 verticalGuideColor = ColorFromHTML("ffffff", 0.35f);
    const ImU32 horizontalGuideColor = ColorFromHTML("ffffff", 0.18f);
    const ImU32 markerOutlineColor = ColorFromHTML("111111", 0.95f);

    dl->PushClipRect(bb.Min, bb.Max, true);
    dl->AddLine({hoveredX, bb.Min.y}, {hoveredX, bb.Max.y}, verticalGuideColor, 1.0f);
    dl->AddLine({bb.Min.x, hoveredY}, {bb.Max.x, hoveredY}, horizontalGuideColor, 1.0f);

    for (size_t iSeries = 0; iSeries < TSeriesCount; ++iSeries) {
      if (renderCfg[iSeries].mLineColor.Value.w <= 0.0f) {
        continue;
      }
      if (hoveredIndex >= (int)mSeries[iSeries].mHistDecibels.size()) {
        continue;
      }

      const float markerY = valueToY(mSeries[iSeries].mHistDecibels[hoveredIndex], iSeries);
      const float markerRadius = std::max(3.0f, renderCfg[iSeries].mLineThickness + 1.5f);
      dl->AddCircleFilled({hoveredX, markerY}, markerRadius + 1.0f, markerOutlineColor, 16);
      dl->AddCircleFilled({hoveredX, markerY}, markerRadius, renderCfg[iSeries].mLineColor, 16);
    }

    dl->PopClipRect();
  }

  void RenderTooltip(const ImRect& bb,
                     const std::array<HistoryViewSeriesConfig, TSeriesCount>& renderCfg,
                     const std::array<HistoryTooltipSeriesConfig, TSeriesCount>& tooltipCfg) {
    int hoveredIndex = -1;
    float hoveredX = 0.0f;
    float hoveredY = 0.0f;
    if (!TryGetHoveredSample(bb, hoveredIndex, hoveredX, hoveredY)) {
      return;
    }

    double elapsedSeconds = mTimestamps[hoveredIndex] - mTimestamps.back();
    if (std::fabs(elapsedSeconds) < 0.0005) {
      elapsedSeconds = 0.0;
    }
    const double elapsedMilliseconds = elapsedSeconds * 1000.0;
    const double elapsedSamples = elapsedSeconds * Helpers::CurrentSampleRateF;

    char headerBuffer[128];
    std::snprintf(headerBuffer,
                  sizeof(headerBuffer),
                  "Hovered: %+.3fs (%+.0f ms, %+.0f samples)",
                  elapsedSeconds,
                  elapsedMilliseconds,
                  elapsedSamples);

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(headerBuffer);
    ImGui::Spacing();

    if (ImGui::BeginTable("##history_tooltip", 2, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV)) {
      ImGui::TableSetupColumn("Series");
      ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed);

      for (size_t i = 0; i < TSeriesCount; ++i) {
        const auto& ttCfg = tooltipCfg[i];
        if (!ttCfg.mShowInTooltip || !ttCfg.mLabel || !ttCfg.mLabel[0]) {
          continue;
        }
        if (renderCfg[i].mLineColor.Value.w <= 0.0f) {
          continue;
        }
        if (hoveredIndex >= (int)mSeries[i].mHistDecibels.size()) {
          continue;
        }

        char valueBuffer[64];
        HistoryTooltipFormatValue(valueBuffer, sizeof(valueBuffer), mSeries[i].mHistDecibels[hoveredIndex], ttCfg);

        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        HistoryTooltipNameCell(ttCfg);
        ImGui::TableSetColumnIndex(1);
        HistoryTooltipValueCell(valueBuffer);
      }

      ImGui::EndTable();
    }

    ImGui::EndTooltip();
  }

  void Render(const std::array<HistoryViewSeriesConfig, TSeriesCount> &cfg,
              const std::array<float, TSeriesCount> &linValues,
              const std::array<HistoryTooltipSeriesConfig, TSeriesCount>* tooltipCfg = nullptr) {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = {bb.Min.x + gHistViewSize.x, bb.Min.y + gHistViewSize.y};

    std::array<float, TSeriesCount> dbValues{};
    for (size_t i = 0; i < TSeriesCount; ++i) {
      dbValues[i] = M7::math::LinearToDecibels(linValues[i]);
    }
    if (!mPaused) {
      PushSamples(dbValues);
    }

    ImColor backgroundColor = ColorFromHTML("222222", 1.0f);

    auto DbToY = [&](float db) {
      // let's show a range of -x to 0 db.
      float x = (db - gHistoryViewMinDB) / -gHistoryViewMinDB;
      x = M7::math::clamp01(x);
      return M7::math::lerp(bb.Max.y, bb.Min.y, x);
    };
    auto *dl = ImGui::GetWindowDrawList();

    ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

    for (size_t iSeries = 0; iSeries < TSeriesCount; ++iSeries) {
      std::vector<ImVec2> points;
      for (int isample = 0;
           isample < ((signed)mSeries[iSeries].mHistDecibels.size());
           ++isample) {
        points.push_back({SampleToX(isample, bb),
                          DbToY(mSeries[iSeries].mHistDecibels[isample])});
      }
      dl->AddPolyline(points.data(), (int)points.size(),
                      cfg[iSeries].mLineColor, 0, cfg[iSeries].mLineThickness);
    }

    ImGui::Dummy(gHistViewSize);
    RenderHoverOverlay(bb, cfg, [&](float value, size_t) { return DbToY(value); });
    RenderPauseButton(bb);
    if (tooltipCfg) {
      RenderTooltip(bb, cfg, *tooltipCfg);
    }
  }

  // Custom render method for non-dB values (like stereo phase correlation, width, balance)
  void RenderCustom(const std::array<HistoryViewSeriesConfig, TSeriesCount> &cfg,
                    const std::array<float, TSeriesCount> &values,
                    float globalMinValue = -1.0f,
                    float globalMaxValue = 2.0f,
                    const std::array<HistoryTooltipSeriesConfig, TSeriesCount>* tooltipCfg = nullptr) {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = {bb.Min.x + gHistViewSize.x, bb.Min.y + gHistViewSize.y};

    if (!mPaused) {
      PushSamples(values);
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
        points.push_back({SampleToX(isample, bb),
                          ValueToYForSeries(mSeries[iSeries].mHistDecibels[isample], iSeries)});
      }
      if (points.size() > 1) {
        dl->AddPolyline(points.data(), (int)points.size(),
                        cfg[iSeries].mLineColor, 0, cfg[iSeries].mLineThickness);
      }
    }

    ImGui::Dummy(gHistViewSize);
    RenderHoverOverlay(bb, cfg, [&](float value, size_t seriesIndex) { return ValueToYForSeries(value, seriesIndex); });
    RenderPauseButton(bb);
    if (tooltipCfg) {
      RenderTooltip(bb, cfg, *tooltipCfg);
    }
  }
};

} // namespace WaveSabreVstLib

#endif
