#pragma once

#include <d3d9.h>
#include <tchar.h>
#include <windows.h>

#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include <WaveSabreCore.h>
#include <WaveSabreCore/../../Basic/Helpers.h>
//#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreVstLib/ImGuiUtils.hpp>
#include <WaveSabreVstLib/Maj7ParamConverters.hpp>
#include <WaveSabreVstLib/Maj7VstUtils.hpp>

namespace WaveSabreVstLib
{

// Internal config for the waveform renderer
struct WaveformViewConfig
{
  const std::vector<float>* referenceYValues =
      nullptr;  // optional list of Y values (in data space) to draw subtle reference lines for
  ImU32 referenceLineColor = ColorFromHTML("333333");
  float referenceLineThickness = 1.0f;

  bool showLollipops = true;  // whether to draw circular markers at each sample point
  bool showStems = false;     // whether to draw vertical lines from baseline to each sample point
  bool showLines = true;      // whether to draw a connected line through the sample points
  bool showExtDots = true;
  bool showEdgeEventLines = true;

  bool autoRangeY = true;  // whether to auto-range the Y axis to fit the data (overrides yMin/yMax if true)
  float yMin = -1.0f;
  float yMax = 1.0f;
};

struct WFVSample
{
  M7::CoreSample sample;
  ImVec2 point;  // used internally
};

// Internal implementation shared by public overloads
static inline std::optional<WFVSample> WaveformViewImpl(const char* id,
                                                        ImVec2 size,
                                                        std::vector<WFVSample>& samples,
                                                        const WaveformViewConfig& cfg)
{
  using namespace WaveSabreCore;

  ImGuiIdScope idScope{id};

  // Early out placeholder when no data
  if (samples.empty())
  {
    ImGui::Dummy(size);
    return {};
  }

  // Compute bounds
  float minVal = samples[0].sample.amplitude;
  float maxVal = samples[0].sample.amplitude;
  for (size_t i = 1; i < samples.size(); ++i)
  {
    minVal = std::min(minVal, samples[i].sample.amplitude);
    maxVal = std::max(maxVal, samples[i].sample.amplitude);
  }
  // Fallback to a sensible range if flat
  if (M7::math::FloatEquals(minVal, maxVal))
  {
    minVal -= 1.0f;
    maxVal += 1.0f;
  }

  if (cfg.autoRangeY)
  {
    // Expand range slightly for better visibility
    float range = maxVal - minVal;
    float padding = range * 0.05f;  // 5% padding
    minVal -= padding;
    maxVal += padding;
  }
  else
  {
    // Use configured fixed range
    minVal = cfg.yMin;
    maxVal = cfg.yMax;
  }

  const float range = std::max(1e-6f, maxVal - minVal);

  // Layout
  ImVec2 pos = ImGui::GetCursorScreenPos();
  ImRect bb{pos, pos + size};
  auto* dl = ImGui::GetWindowDrawList();

  // Background and border
  ImGui::RenderFrame(bb.Min, bb.Max, ColorFromHTML("222222"));

  // Helpers for coordinate mapping
  auto XForIndex = [&](size_t i)
  {
    float t = samples.size() > 1 ? (float)i / (float)(samples.size() - 1) : 0.0f;
    return M7::math::lerp(bb.Min.x, bb.Max.x, t);
  };
  auto YForValue = [&](float v)
  {
    float t = (v - minVal) / range;  // 0..1
    t = M7::math::clamp01(t);
    return M7::math::lerp(bb.Max.y, bb.Min.y, t);
  };

  // Optional reference lines (behind waveform)
  if (cfg.referenceYValues && !cfg.referenceYValues->empty())
  {
    for (float v : *cfg.referenceYValues)
    {
      // Skip if equals baseline 0 to avoid duplicating baseline (within epsilon)
      if (M7::math::FloatEquals(v, 0.0f))
        continue;
      float y = YForValue(v);
      if (y >= bb.Min.y && y <= bb.Max.y)
      {
        dl->AddLine(ImVec2(bb.Min.x, y), ImVec2(bb.Max.x, y), cfg.referenceLineColor, cfg.referenceLineThickness);
      }
    }
  }

  // draw "out of range" background rects at the top (above +1 and below -1).
  {
    const float y1 = YForValue(1.0f);
    if (y1 > bb.Min.y)
    {
      dl->AddRectFilled(ImVec2(bb.Min.x, bb.Min.y), ImVec2(bb.Max.x, y1), ColorFromHTML("443333", 0.5f));
    }
    const float yNeg1 = YForValue(-1.0f);
    if (yNeg1 < bb.Max.y)
    {
      dl->AddRectFilled(ImVec2(bb.Min.x, yNeg1), ImVec2(bb.Max.x, bb.Max.y), ColorFromHTML("443333", 0.5f));
    }
  }

  // Baseline at 0 amplitude (may be outside if range doesn't include 0)
  const float baselineY = YForValue(0.0f);
  const bool baselineVisible = baselineY >= bb.Min.y && baselineY <= bb.Max.y;
  if (baselineVisible)
  {
    dl->AddLine(ImVec2(bb.Min.x, baselineY), ImVec2(bb.Max.x, baselineY), ColorFromHTML("444444"));
  }

  // Build point list for connected line
  //std::vector<WFVSample> points;
  //points.reserve(samples.size());
  for (size_t i = 0; i < samples.size(); ++i)
  {
    samples[i].point = ImVec2(XForIndex(i), YForValue(samples[i].sample.amplitude));
  }

  // Hover detection within bounding box (before Dummy so we can render highlight behind markers)
  int hoveredIdx = -1;
  if (ImGui::IsMouseHoveringRect(bb.Min, bb.Max))
  {
    ImVec2 mouse = ImGui::GetIO().MousePos;
    const float offset =
        1;  // highlight from THIS sample to the next (in the sample window, the sample emitted is at the beginning of the window)
    float xNorm = (mouse.x - bb.Min.x) / std::max(1.0f, (bb.Max.x - bb.Min.x));
    xNorm = M7::math::clamp01(xNorm);
    size_t i = (size_t)std::round(xNorm * (samples.size() - 1));
    i = (size_t)std::min<size_t>(i, samples.size() - 1);
    hoveredIdx = (int)i;
  }

  // Subtle background highlight for hovered sample (vertical band)
  if (hoveredIdx >= 0)
  {
    const int i = hoveredIdx;
    const float offset =
        1;  // highlight from THIS sample to the next (in the sample window, the sample emitted is at the beginning of the window)
    const float x0 = XForIndex((size_t)i);
    const float x1 = XForIndex(std::min((size_t)i + (size_t)offset, samples.size() - 1));
    dl->AddRectFilled(ImVec2(x0, bb.Min.y), ImVec2(x1, bb.Max.y), ColorFromHTML("444466", 0.3f));
  }

  // Connected base line
  const ImU32 baseLineColor = ColorFromHTML("88ccff");
  const float baseLineThickness = 1.5f;
  if (samples.size() > 1 && cfg.showLines)
  {
    std::vector<ImVec2> points;
    points.reserve(samples.size());
    for (size_t i = 0; i < samples.size(); ++i)
    {
      points.push_back(samples[i].point);
    }

    dl->AddPolyline(points.data(), (int)points.size(), baseLineColor, 0, baseLineThickness);
  }

  // Lollipop stems and markers
  const float stemThickness = 1.0f;
  const float markerRadius = 5.0f;
  const ImU32 stemColor = ColorFromHTML("888888");
  const ImU32 markerFill = ColorFromHTML("ffffff");
  const ImU32 markerOutline = ColorFromHTML("000000");

  // Highlight styles
  const ImU32 hlLineColor = ColorFromHTML("ffcc66");
  const float hlLineThickness = 2.5f;
  const ImU32 hlStemColor = ColorFromHTML("ffcc66");
  const float hlStemThickness = 1.5f;
  const ImU32 hlMarkerFill = ColorFromHTML("ffcc66");
  const ImU32 hlMarkerOutline = ColorFromHTML("000000");

  // Overlay: highlight line segments around hovered sample
  if (hoveredIdx >= 0 && samples.size() > 1 && cfg.showLines)
  {
    const int i = hoveredIdx;
    //if (i > 0)
    //  dl->AddLine(samples[(size_t)i - 1].point, samples[(size_t)i].point, hlLineColor, hlLineThickness);
    if (i < (int)samples.size() - 1)
      dl->AddLine(samples[(size_t)i].point, samples[(size_t)i + 1].point, hlLineColor, hlLineThickness);
  }

  if (baselineVisible)
  {
    for (size_t i = 0; i < samples.size(); ++i)
    {
      const auto& s = samples[i];
      const ImVec2 p = s.point;
      const bool isHovered = ((int)i == hoveredIdx);
      // stem to baseline
      if (cfg.showStems)
      {
        dl->AddLine(ImVec2(p.x, p.y),
                    ImVec2(p.x, baselineY),
                    isHovered ? hlStemColor : stemColor,
                    isHovered ? hlStemThickness : stemThickness);
      }

#if 0
      if (cfg.showEdgeEventLines)
      {
        for (size_t iEdgeEvent = 0; iEdgeEvent < s.sample.edgeEvents.size(); ++iEdgeEvent)
        {
          const auto& e = s.sample.edgeEvents[iEdgeEvent];
          // draw a dot at the event location. e.whenInSample01 is [0,1) within the sample window.
          const float ex = M7::math::lerp(XForIndex(i), XForIndex(i + 1), e.whenInSample01);
          const float ey = YForValue(s.sample.naive);
          ImU32 ec = ColorFromHTML("aa44aa", 0.5f);
          // vertical line as well.
          dl->AddLine(ImVec2(ex, baselineY), ImVec2(ex, ey), ec, 1.0f);
          dl->AddCircleFilled(ImVec2(ex, ey), 3, ec, 8);
          dl->AddCircleFilled(ImVec2(ex, baselineY), 3, ec, 8);
        }
      }
#endif
      // marker
      if (cfg.showLollipops)
      {
        dl->AddCircleFilled(p, markerRadius, isHovered ? hlMarkerFill : markerFill, 8);
        dl->AddCircle(p, markerRadius, isHovered ? hlMarkerOutline : markerOutline, 8, 1.0f);
      }

      if (cfg.showExtDots)
      {
        std::string str;
        if (s.sample.correction > 1e-6)
        {
          str += ".corr";
          dl->AddCircle(p, markerRadius + 2, ColorFromHTML("44ff44"), 8, 2.0f);
        }
#if 0
        for (size_t iEvent = 0; iEvent < s.sample.phaseAdvance.eventCount; ++iEvent)
        {
          const auto& e = s.sample.phaseAdvance.events[iEvent];
          if (e.kind == M7::PhaseEventKind::Wrap)
          {
            str += ".wrap";
            dl->AddCircle(p, markerRadius + 4, ColorFromHTML("00ffff"), 8, 2.0f);
          }
          else if (e.kind == M7::PhaseEventKind::Reset)
          {
            str += ".sync";
            dl->AddCircle(p, markerRadius + 6, ColorFromHTML("ff8800"), 8, 2.0f);
          }
        }
#endif

        // draw the text above or below the circular marker.
        // if the amplitude is positive, draw the indicator below the marker. If negative, draw above.
        if (!str.empty())
        {
          ImVec2 textSize = ImGui::CalcTextSize(str.c_str());
          ImVec2 textPos;
          if (s.sample.amplitude >= 0.0f)
          {
            // below
            textPos = ImVec2(p.x - textSize.x * 0.5f, p.y + markerRadius + 4);
            if (textPos.y + textSize.y > bb.Max.y)
              textPos.y = bb.Max.y - textSize.y;
          }
          else
          {
            // above
            textPos = ImVec2(p.x - textSize.x * 0.5f, p.y - markerRadius - 4 - textSize.y);
            if (textPos.y < bb.Min.y)
              textPos.y = bb.Min.y;
          }
          dl->AddText(textPos, ColorFromHTML("ffffff"), str.c_str());
        }
      }
    }
  }
  else
  {
    // If baseline is off-screen, still draw markers
    if (cfg.showStems)
    {
      for (size_t i = 0; i < samples.size(); ++i)
      {
        const auto& s = samples[i];
        const ImVec2& p = s.point;
        const bool isHovered = ((int)i == hoveredIdx);
        dl->AddCircleFilled(p, markerRadius, isHovered ? hlMarkerFill : markerFill, 8);
        dl->AddCircle(p, markerRadius, isHovered ? hlMarkerOutline : markerOutline, 8, 1.0f);
      }
    }
  }

  // Make the drawn area interactive (for tooltip)
  ImGui::Dummy(size);

  // Tooltip with XY details
  //if (ImGui::IsItemHovered())
  if (hoveredIdx >= 0)
  {
    //ImVec2 mouse = ImGui::GetIO().MousePos;
    //// Nearest sample index from mouse X
    //float xNorm = (mouse.x - bb.Min.x) / std::max(1.0f, (bb.Max.x - bb.Min.x));
    //xNorm = M7::math::clamp01(xNorm);
    //size_t i = (size_t)std::round(xNorm * (samples.size() - 1));
    //i = (size_t)std::min<size_t>(i, samples.size() - 1);

    const auto& s = samples[hoveredIdx];
    const float vLin = s.sample.amplitude;
    const float vDb = M7::math::LinearToDecibels(std::max(std::abs(vLin), M7::gMinGainLinear));
    const float sampleRate = Helpers::CurrentSampleRateF;
    //const float tSec = sampleRate > 0.0f ? (float)hoveredIdx / sampleRate : 0.0f;
    //const float tMs = tSec * 1000.0f;
    // Phase 0..1 (avoid hitting exactly 1.0 at the end)
    //const float phase = (float)hoveredIdx / (float)samples.size();

    ImGui::BeginTooltip();
    ImGui::Text("Sample: %u", (unsigned)hoveredIdx);
    //ImGui::Text("Time: %.4f s (%.2f ms)", tSec, tMs);
    ImGui::Text("Amplitude: %.6f lin", vLin);
    //ImGui::Text("Phase: %.4f", phase);
    //    ImGui::Separator();
    //ImGui::Text("Amplitude: %.2f dB", vDb);
    ImGui::Separator();
    ImGui::Text("Naive: %.3f", s.sample.naive);
    ImGui::Text("Correction: %.3f", s.sample.correction);
    //ImGui::Text("Frequency: %.3f Hz", s.sample.phaseAdvance.ComputeFrequencyHz());
    ImGui::Separator();
#if 0
    for (int i = 0; i < s.sample.phaseAdvance.eventCount; ++i)
    {
      const auto& e = s.sample.phaseAdvance.events[i];
      const char* kindStr = (e.kind == M7::PhaseEventKind::Wrap) ? "Wrap" : "Reset";
      ImGui::Text("Event %d: %s at %.3f", i, kindStr, e.whenInSample01);
    }
#endif
    ImGui::EndTooltip();

    return s;
  }

  return {};
}

//// renders a bordered lollipop plot, with connected lines.
//// tooltips show XY values
////   X = sample#, time, and cycle phase 0-1 (0 = first sample of the cycle, 0.99?? = last).
////   Y = amplitude in dB and linear.
//// always displays all samples
//// Y scale is normalized to the input data
//inline void WaveformView(const char* id, ImVec2 size, std::vector<WFVSample>& samples)
//{
//  WaveformViewConfig cfg;  // defaults, no reference lines
//  WaveformViewImpl(id, size, samples, cfg);
//}
//
//// Overload with support for reference Y lines
//inline void WaveformView(const char* id,
//                         ImVec2 size,
//                         std::vector<WFVSample>& samples,
//                         const std::vector<float>& referenceYValues,
//                         ImU32 referenceLineColor = ColorFromHTML("999999", 0.35f),
//                         float referenceLineThickness = 1.0f)
//{
//  WaveformViewConfig cfg;
//  cfg.referenceYValues = &referenceYValues;
//  cfg.referenceLineColor = referenceLineColor;
//  cfg.referenceLineThickness = referenceLineThickness;
//  WaveformViewImpl(id, size, samples, cfg);
//}


}  // namespace WaveSabreVstLib
