#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

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

inline void RenderStereoImagingScope(const char* id,
                                     const StereoImagingAnalysisStream& analysis,
                                     ImVec2 size,
                                     const StereoImagingScopeLayerVisibility& visibility)
{
  auto* dl = ImGui::GetWindowDrawList();
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImRect bb(pos, {pos.x + size.x, pos.y + size.y});

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

  if (visibility.showPolarL)
  {
    RenderPolarLLayer(id, analysis, size, center, radius);
  }

  if (visibility.showGoniometerPoints)
  {
    RenderGoniometerLayer(id, analysis, size, center, radius, true);
  }

  if (visibility.showGoniometerLines)
  {
    RenderGoniometerLayer(id, analysis, size, center, radius, false);
  }

  if (visibility.showPhaseX)
  {
    RenderPhaseCorrelationOverlay(id, analysis, size, center, radius);
  }

  ImGui::Dummy(size);
}

inline void RenderStereoImagingDisplay(const char* id,
                                       const StereoImagingAnalysisStream& analysis,
                                       const StereoImagingScopeLayerVisibility& visibility,
                                       int dimension = 250)
{
  ImGuiGroupScope _scope(id);

  const ImVec2 meterSize(static_cast<float>(dimension), 30.0f);
  const ImVec2 scopeSize(static_cast<float>(dimension), static_cast<float>(dimension));
  const std::string correlationId = std::string(id) + "_correlation";

  RenderCorrelationMeter(correlationId.c_str(), analysis.mPhaseCorrelation, meterSize);
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

  RenderGeneralMeter(
      analysis.mStereoWidth, 0, 1, meterSize, "Stereo width", 0, {"008800", 0.5, "ffff00", 1.0, "ff0000"});
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

  RenderGeneralMeter(analysis.mStereoBalance,
                     -1,
                     1,
                     meterSize,
                     "Left-right balance",
                     0,
                     {
                         "ff0000",
                         -0.5,
                         "ffff00",
                         -0.2,
                         "00ff00",
                         +0.2,
                         "ffff00",
                         +0.5,
                         "ff0000",
                     });

  if (ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text("Left-right balance");
    ImGui::Separator();
    ImGui::Text("Indicates the balance between left and right channels:");
    ImGui::BulletText("-1.0: Full left (right channel silent)");
    ImGui::BulletText(" 0.0: Centered (equal left/right)");
    ImGui::BulletText("+1.0: Full right (left channel silent)");
    ImGui::EndTooltip();
  }

  const std::string scopeId = std::string(id) + "_scope";
  RenderStereoImagingScope(scopeId.c_str(), analysis, scopeSize, visibility);
}
