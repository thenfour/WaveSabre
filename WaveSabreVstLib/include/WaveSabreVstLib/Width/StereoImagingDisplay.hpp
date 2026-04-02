#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include <algorithm>
#include <cmath>
#include <string>

#include "CorrelationMeter.hpp"
#include "Goniometer.hpp"
#include "PolarL.hpp"

struct StereoImagingScopeLayerVisibility
{
  bool showPolarL = true;
  bool showGoniometerPoints = true;
  bool showGoniometerLines = false;
  bool showPhaseX = true;
};

struct StereoImagingDisplayStyle
{
  struct SeriesStyle
  {
    const char* label = nullptr;
    StereoImagingColorScheme colors{};
  };

  SeriesStyle primarySeries{};
  SeriesStyle secondarySeries{};
  StereoFieldOverlayStyle fieldOverlay{};
};

inline const StereoImagingDisplayStyle& GetDefaultStereoImagingDisplayStyle()
{
  static const StereoImagingDisplayStyle style{};
  return style;
}

inline ImU32 GetStereoWidthMeterColor(double value, const StereoImagingColorScheme& colors)
{
  const double width = std::max(0.0, value);
  if (width >= 1.0)
  {
    return ApplyAlphaToColor(colors.alertColor, 0.82f);
  }

  if (width >= 0.5)
  {
    return ApplyAlphaToColor(colors.warningColor, 0.82f);
  }

  return ApplyAlphaToColor(colors.okColor, 0.82f);
}

inline ImU32 GetStereoBalanceMeterColor(double value, const StereoImagingColorScheme& colors)
{
  const double magnitude = std::abs(std::clamp(value, -1.0, 1.0));
  if (magnitude >= 0.5)
  {
    return ApplyAlphaToColor(colors.alertColor, 0.82f);
  }

  if (magnitude >= 0.2)
  {
    return ApplyAlphaToColor(colors.warningColor, 0.82f);
  }

  return ApplyAlphaToColor(colors.okColor, 0.82f);
}

inline void RenderStereoImagingLegendEntries(const char* primaryLabel,
                                             ImU32 primaryColor,
                                             const char* secondaryLabel,
                                             ImU32 secondaryColor,
                                             const ImRect& bb)
{
  auto* dl = ImGui::GetWindowDrawList();
  float legendX = bb.Max.x - 8.0f;
  const float legendY = bb.Min.y + 3.0f;

  auto renderLegend = [&](const char* label, ImU32 color) {
    if (!label || !label[0])
    {
      return;
    }

    const ImVec2 labelSize = ImGui::CalcTextSize(label);
    legendX -= labelSize.x;
    dl->AddText({legendX, legendY}, ApplyAlphaToColor(IM_COL32(255, 255, 255, 255), 0.85f), label);
    legendX -= 8.0f;
    dl->AddRectFilled({legendX - 10.0f, legendY + 3.0f}, {legendX - 2.0f, legendY + 11.0f}, color, 2.0f);
    legendX -= 18.0f;
  };

  renderLegend(secondaryLabel, secondaryColor);
  renderLegend(primaryLabel, primaryColor);
}

inline void RenderStereoImagingMeter(const char* id,
                                     ImVec2 size,
                                     const char* label,
                                     double minValue,
                                     double maxValue,
                                     double centerValue,
                                     double primaryValue,
                                     ImU32 primaryColor,
                                     const char* primarySeriesLabel,
                                     const double* secondaryValue = nullptr,
                                     ImU32 secondaryColor = 0,
                                     const char* secondarySeriesLabel = nullptr)
{
  ImGui::PushID(id);
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
  ImGui::Dummy(size);

  auto* dl = ImGui::GetWindowDrawList();
  dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 40, 40, 255));
  dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

  const double range = maxValue - minValue;
  auto valueToX = [&](double value) -> float {
    if (range <= 0.0001)
    {
      return bb.Min.x;
    }

    const double clampedValue = std::clamp(value, minValue, maxValue);
    return bb.Min.x + static_cast<float>((clampedValue - minValue) / range * size.x);
  };

  const float minX = valueToX(minValue);
  const float centerX = valueToX(centerValue);
  const float maxX = valueToX(maxValue);
  dl->AddLine({minX, bb.Min.y}, {minX, bb.Max.y}, IM_COL32(120, 120, 120, 60), 1.0f);
  dl->AddLine({centerX, bb.Min.y}, {centerX, bb.Max.y}, IM_COL32(180, 180, 180, 90), 1.0f);
  dl->AddLine({maxX, bb.Min.y}, {maxX, bb.Max.y}, IM_COL32(120, 120, 120, 60), 1.0f);

  dl->AddText({bb.Min.x + 4.0f, bb.Min.y + 3.0f}, IM_COL32(255, 255, 255, 150), label);
  RenderStereoImagingLegendEntries(primarySeriesLabel, primaryColor, secondarySeriesLabel, secondaryColor, bb);

  const bool hasSecondary = secondaryValue != nullptr;
  const float contentTop = bb.Min.y + 16.0f;
  const float contentBottom = bb.Max.y - 18.0f;
  const float gap = hasSecondary ? 4.0f : 0.0f;
  const int laneCount = hasSecondary ? 2 : 1;
  const float laneHeight = std::max(4.0f, (contentBottom - contentTop - gap) / static_cast<float>(laneCount));

  auto renderLane = [&](int laneIndex, double value, ImU32 color) {
    const float laneY = contentTop + laneIndex * (laneHeight + gap);
    const float valueX = valueToX(value);
    dl->AddRectFilled({std::min(centerX, valueX), laneY}, {std::max(centerX, valueX), laneY + laneHeight}, color, 2.0f);
  };

  renderLane(0, primaryValue, primaryColor);
  if (hasSecondary)
  {
    renderLane(1, *secondaryValue, secondaryColor);
  }

  char primaryText[64];
  if (hasSecondary && primarySeriesLabel && primarySeriesLabel[0])
  {
    sprintf_s(primaryText, "%s %.3f", primarySeriesLabel, static_cast<float>(primaryValue));
  }
  else
  {
    sprintf_s(primaryText, "%.3f", static_cast<float>(primaryValue));
  }
  dl->AddText({bb.Min.x + 4.0f, bb.Max.y - 14.0f}, primaryColor, primaryText);

  if (hasSecondary)
  {
    char secondaryText[64];
    if (secondarySeriesLabel && secondarySeriesLabel[0])
    {
      sprintf_s(secondaryText, "%s %.3f", secondarySeriesLabel, static_cast<float>(*secondaryValue));
    }
    else
    {
      sprintf_s(secondaryText, "%.3f", static_cast<float>(*secondaryValue));
    }

    const ImVec2 secondaryTextSize = ImGui::CalcTextSize(secondaryText);
    dl->AddText({bb.Max.x - secondaryTextSize.x - 4.0f, bb.Max.y - 14.0f}, secondaryColor, secondaryText);
  }

  ImGui::PopID();
}

inline void RenderStereoImagingScope(const char* id,
                                     StereoImagingAnalysisStream& primaryAnalysis,
                                     StereoImagingAnalysisStream* secondaryAnalysis,
                                     ImVec2 size,
                                     const StereoImagingScopeLayerVisibility& visibility,
                                     const StereoImagingDisplayStyle& style = GetDefaultStereoImagingDisplayStyle())
{
  ImGui::PushID(id);
  ImGui::InvisibleButton("##stereoScope", size);
  const bool scopeHovered = ImGui::IsItemHovered();
  const bool openPopup = ImGui::IsItemClicked(ImGuiMouseButton_Left);

  auto* dl = ImGui::GetWindowDrawList();
  ImRect bb(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

  dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
  dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

  ImVec2 center = {bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f};
  const float radius = std::min(size.x, size.y) * 0.45f;

  for (int i = 1; i <= 4; ++i)
  {
    float r = radius * i * 0.25f;
    dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
  }

  for (int angle = 0; angle < 360; angle += 30)
  {
    float rad = angle * 3.14159f / 180.0f;
    ImVec2 lineEnd = {center.x + cos(rad) * radius, center.y + sin(rad) * radius};
    ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
    dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
  }

  dl->AddText({center.x - 4, bb.Min.y + 2}, IM_COL32(100, 255, 100, 150), "M");
  dl->AddText({bb.Max.x - 15, center.y - 8}, IM_COL32(150, 150, 255, 150), "R");
  dl->AddText({center.x - 20, bb.Max.y - 16}, IM_COL32(255, 100, 100, 150), "inv");
  dl->AddText({bb.Min.x + 2, center.y - 8}, IM_COL32(150, 150, 255, 150), "L");

  RenderStereoImagingLegendEntries(style.primarySeries.label,
                                   ApplyAlphaToColor(style.primarySeries.colors.okColor, 0.9f),
                                   secondaryAnalysis ? style.secondarySeries.label : nullptr,
                                   ApplyAlphaToColor(style.secondarySeries.colors.okColor, 0.9f),
                                   bb);

  if (visibility.showPolarL)
  {
    const std::string primaryPolarId = std::string(id) + "_primary";
    RenderPolarLLayer(primaryPolarId.c_str(), primaryAnalysis, size, center, radius, style.primarySeries.colors);
    if (secondaryAnalysis)
    {
      const std::string secondaryPolarId = std::string(id) + "_secondary";
      RenderPolarLLayer(secondaryPolarId.c_str(),
                        *secondaryAnalysis,
                        size,
                        center,
                        radius,
                        style.secondarySeries.colors);
    }
  }

  if (visibility.showGoniometerPoints)
  {
    RenderGoniometerLayer(id, primaryAnalysis, size, center, radius, true, style.primarySeries.colors);
    if (secondaryAnalysis)
    {
      RenderGoniometerLayer(id, *secondaryAnalysis, size, center, radius, true, style.secondarySeries.colors);
    }
  }

  if (visibility.showGoniometerLines)
  {
    RenderGoniometerLayer(id, primaryAnalysis, size, center, radius, false, style.primarySeries.colors);
    if (secondaryAnalysis)
    {
      RenderGoniometerLayer(id, *secondaryAnalysis, size, center, radius, false, style.secondarySeries.colors);
    }
  }

  if (visibility.showPhaseX)
  {
    RenderPhaseCorrelationOverlay(
        id, primaryAnalysis, size, center, radius, style.fieldOverlay, style.primarySeries.colors);
    if (secondaryAnalysis)
    {
      RenderPhaseCorrelationOverlay(
          id, *secondaryAnalysis, size, center, radius, style.fieldOverlay, style.secondarySeries.colors);
    }
  }

  if (openPopup)
  {
    ImGui::OpenPopup("##stereoVisSpeedPopup");
    ImGui::SetNextWindowPos(ImGui::GetIO().MouseClickedPos[ImGuiMouseButton_Left], ImGuiCond_Appearing);
  }

  if (ImGui::BeginPopup("##stereoVisSpeedPopup"))
  {
    ImGui::Text("Stereo visualization speed");
    ImGui::Separator();

    using BalanceSpeed = StereoImagingAnalysisStream::BalanceBallisticsSpeed;
    const BalanceSpeed speedOptions[] = {
        BalanceSpeed::Momentary,
        BalanceSpeed::Fast,
        BalanceSpeed::Medium,
        BalanceSpeed::Slow,
        BalanceSpeed::Section,
    };

    for (auto speed : speedOptions)
    {
      const bool selected = primaryAnalysis.GetBalanceBallisticsSpeed() == speed;
      if (ImGui::Selectable(StereoImagingAnalysisStream::GetBalanceBallisticsSpeedLabel(speed), selected))
      {
        primaryAnalysis.SetBalanceBallisticsSpeed(speed);
        if (secondaryAnalysis)
        {
          secondaryAnalysis->SetBalanceBallisticsSpeed(speed);
        }
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();
      ImGui::TextDisabled("%s", StereoImagingAnalysisStream::GetBalanceBallisticsSpeedDescription(speed));
    }

    ImGui::EndPopup();
  }

  if (scopeHovered && !ImGui::IsPopupOpen("##stereoVisSpeedPopup"))
  {
    ImGui::BeginTooltip();
    ImGui::Text("Stereo field");
    ImGui::Separator();
    ImGui::Text("Click to change stereo visualization speed.");
    const auto currentSpeed = primaryAnalysis.GetBalanceBallisticsSpeed();
    ImGui::Text("Current: %s", StereoImagingAnalysisStream::GetBalanceBallisticsSpeedLabel(currentSpeed));
    ImGui::TextDisabled("%s", StereoImagingAnalysisStream::GetBalanceBallisticsSpeedDescription(currentSpeed));
    ImGui::EndTooltip();
  }

  ImGui::PopID();
}

inline void RenderStereoImagingDisplay(const char* id,
                                       StereoImagingAnalysisStream& primaryAnalysis,
                                       StereoImagingAnalysisStream* secondaryAnalysis,
                                       const StereoImagingScopeLayerVisibility& visibility,
                                       int dimension = 250,
                                       const StereoImagingDisplayStyle& style = GetDefaultStereoImagingDisplayStyle())
{
  ImGuiGroupScope _scope(id);

  const bool hasSecondary = secondaryAnalysis != nullptr;
  const ImVec2 meterSize(static_cast<float>(dimension), hasSecondary ? 42.0f : 30.0f);
  const ImVec2 scopeSize(static_cast<float>(dimension), static_cast<float>(dimension));
  const std::string correlationId = std::string(id) + "_correlation";

  const double secondaryCorrelation = hasSecondary ? secondaryAnalysis->mPhaseCorrelation : 0.0;
  RenderStereoImagingMeter(correlationId.c_str(),
                           meterSize,
                           "Correlation",
                           -1.0,
                           1.0,
                           0.0,
                           primaryAnalysis.mPhaseCorrelation,
                           GetCorrellationColor(
                               static_cast<float>(primaryAnalysis.mPhaseCorrelation), 0.82f, style.primarySeries.colors),
                           style.primarySeries.label,
                           hasSecondary ? &secondaryCorrelation : nullptr,
                           hasSecondary ? GetCorrellationColor(static_cast<float>(secondaryAnalysis->mPhaseCorrelation),
                                                               0.82f,
                                                               style.secondarySeries.colors)
                                        : 0,
                           hasSecondary ? style.secondarySeries.label : nullptr);
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text("Phase correlation");
    ImGui::Separator();
    ImGui::Text("Indicates stereo phase correlation:");
    ImGui::BulletText("-1.0: out of phase (180 deg)");
    ImGui::BulletText(" 0.0: No correlation (random phase)");
    ImGui::BulletText("+1.0: Perfectly in phase (0 deg)");
    ImGui::EndTooltip();
  }

  const std::string widthId = std::string(id) + "_width";
  const double secondaryWidth = hasSecondary ? secondaryAnalysis->mStereoWidth : 0.0;
  RenderStereoImagingMeter(widthId.c_str(),
                           meterSize,
                           "Stereo width",
                           0.0,
                           1.0,
                           0.0,
                           primaryAnalysis.mStereoWidth,
                           GetStereoWidthMeterColor(primaryAnalysis.mStereoWidth, style.primarySeries.colors),
                           style.primarySeries.label,
                           hasSecondary ? &secondaryWidth : nullptr,
                           hasSecondary
                               ? GetStereoWidthMeterColor(secondaryAnalysis->mStereoWidth, style.secondarySeries.colors)
                               : 0,
                           hasSecondary ? style.secondarySeries.label : nullptr);
  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text("Stereo width");
    ImGui::Separator();
    ImGui::Text("Indicates how much of the total signal energy is the side channel.");
    ImGui::BulletText("0.0: Mono (no side channel)");
    ImGui::BulletText("1.0: Widest possible (no mid signal)");
    ImGui::EndTooltip();
  }

  const std::string balanceId = std::string(id) + "_balance";
  const double secondaryBalance = hasSecondary ? secondaryAnalysis->mStereoBalance : 0.0;
  RenderStereoImagingMeter(balanceId.c_str(),
                           meterSize,
                           "Left-right balance",
                           -1.0,
                           1.0,
                           0.0,
                           primaryAnalysis.mStereoBalance,
                           GetStereoBalanceMeterColor(primaryAnalysis.mStereoBalance, style.primarySeries.colors),
                           style.primarySeries.label,
                           hasSecondary ? &secondaryBalance : nullptr,
                           hasSecondary ? GetStereoBalanceMeterColor(secondaryAnalysis->mStereoBalance,
                                                                    style.secondarySeries.colors)
                                        : 0,
                           hasSecondary ? style.secondarySeries.label : nullptr);

  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text("Left-right balance");
    ImGui::Separator();
    ImGui::Text("Indicates the balance between left and right channels:");
    ImGui::BulletText("-1.0: Full left (right channel silent)");
    ImGui::BulletText(" 0.0: Centered (equal left/right)");
    ImGui::BulletText("+1.0: Full right (left channel silent)");
    ImGui::Separator();
    const auto currentSpeed = primaryAnalysis.GetBalanceBallisticsSpeed();
    ImGui::Text("Speed: %s", StereoImagingAnalysisStream::GetBalanceBallisticsSpeedLabel(currentSpeed));
    ImGui::TextDisabled("%s", StereoImagingAnalysisStream::GetBalanceBallisticsSpeedDescription(currentSpeed));
    ImGui::EndTooltip();
  }

  const std::string scopeId = std::string(id) + "_scope";
  RenderStereoImagingScope(scopeId.c_str(), primaryAnalysis, secondaryAnalysis, scopeSize, visibility, style);
}

inline void RenderStereoImagingScope(const char* id,
                                     StereoImagingAnalysisStream& analysis,
                                     ImVec2 size,
                                     const StereoImagingScopeLayerVisibility& visibility,
                                     const StereoImagingDisplayStyle& style = GetDefaultStereoImagingDisplayStyle())
{
  RenderStereoImagingScope(id, analysis, nullptr, size, visibility, style);
}

inline void RenderStereoImagingDisplay(const char* id,
                                       StereoImagingAnalysisStream& analysis,
                                       const StereoImagingScopeLayerVisibility& visibility,
                                       int dimension = 250,
                                       const StereoImagingDisplayStyle& style = GetDefaultStereoImagingDisplayStyle())
{
  RenderStereoImagingDisplay(id, analysis, nullptr, visibility, dimension, style);
}
