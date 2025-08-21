#ifndef __WAVESABREVSTLIB_VUMETER_H__
#define __WAVESABREVSTLIB_VUMETER_H__

#include "../imgui/imgui.h"
#include "Common.h"
#include "Maj7VstUtils.hpp"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <vector>

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{


struct VUMeterColors
{
  ImColor background;
  ImColor foregroundRMS;
  ImColor foregroundPeak;
  ImColor backgroundOverUnity;
  ImColor foregroundOverUnity;
  ImColor text;
  ImColor tick;
  ImColor clipTick;
  ImColor peak;
};

struct VUMeterTick
{
  float dBvalue;
  const char* caption;  // may be nullptr to not display text.
};

enum VUMeterLevelMode
{
  Audio,
  Attenuation,
  Disabled,
};

enum VUMeterUnits
{
  dB,
  Linear,
};

inline VUMeterColors GetVUMeterColorsForMidSideLevel()
{
  // Mid-side level visualization using a blue-based color scheme (distinct from audio's green scheme)
  VUMeterColors colors;
  colors.background = ColorFromHTML("1c2b32");      // Dark bluish background (inverted from audio's 2b321c)
  colors.foregroundRMS = ColorFromHTML("0a6c9b");   // Bright blue RMS (inverted from audio's 6c9b0a)
  colors.foregroundPeak = ColorFromHTML("324a63");  // Darker blue peak (inverted from audio's 4a6332)

  colors.backgroundOverUnity = ColorFromHTML("440000");  // Keep same red for clipping
  colors.foregroundOverUnity = ColorFromHTML("cccc00");  // Keep same yellow for over-unity

  colors.text = ColorFromHTML("ffffff", 0.2f);  // Keep same text
  colors.tick = ColorFromHTML("00ffff");        // Keep same cyan ticks
  colors.clipTick = ColorFromHTML("ff0000");    // Keep same red clip indicator
  colors.peak = ColorFromHTML("0a6c9b", 0.8f);  // Blue peak marker (matching RMS blue)
  return colors;
}

struct VUMeterConfig
{
  ImVec2 size;
  VUMeterLevelMode levelMode;
  VUMeterUnits units;
  float minDB;
  float maxDB;
  std::vector<VUMeterTick> ticks;
};

// rms level may be nullptr to not graph it.
// peak level may be nullptr to not graph it.
// clipindicator same.
// return true if clicked.
inline bool VUMeter(const char* id,
                    const double* rmsLevel,
                    const double* peakLevel,
                    const double* heldPeakLevel,
                    const bool* clipIndicator,
                    bool allowTickText,
                    const VUMeterConfig& cfg,
                    const VUMeterColors* colorsOverride = nullptr)
{
  VUMeterColors colors;
  if (colorsOverride)
  {
    colors = *colorsOverride;
  }
  else
  {
    switch (cfg.levelMode)
    {
      case VUMeterLevelMode::Audio:
      {
        colors.background = ColorFromHTML("2b321c");
        colors.foregroundRMS = ColorFromHTML("6c9b0a");
        colors.foregroundPeak = ColorFromHTML("4a6332");

        colors.backgroundOverUnity = ColorFromHTML("440000");
        colors.foregroundOverUnity = ColorFromHTML("cccc00");

        colors.text = ColorFromHTML("ffffff");
        colors.tick = ColorFromHTML("00ffff");
        colors.clipTick = ColorFromHTML("ff0000");
        colors.peak = ColorFromHTML("6c9b0a", 0.8f);
        break;
      }
      case VUMeterLevelMode::Attenuation:
      {
        colors.background = ColorFromHTML("402e2e");
        colors.foregroundRMS = ColorFromHTML("900000");
        colors.foregroundPeak = ColorFromHTML("600000");

        colors.backgroundOverUnity = ColorFromHTML("440000");
        colors.foregroundOverUnity = ColorFromHTML("cccc00");

        colors.text = ColorFromHTML("ffffff");
        colors.tick = ColorFromHTML("00ffff");
        colors.clipTick = ColorFromHTML("ff0000");
        colors.peak = ColorFromHTML("6c9b0a", 0.8f);
        break;
      }
      default:
      case VUMeterLevelMode::Disabled:
      {
        colors.background = ColorFromHTML("222222");
        colors.foregroundRMS = ColorFromHTML("eeeeee", 0);
        colors.foregroundPeak = ColorFromHTML("666666", 0);

        colors.backgroundOverUnity = ColorFromHTML("666666", 0);
        colors.foregroundOverUnity = ColorFromHTML("777777", 0);

        colors.text = ColorFromHTML("555555", .5);
        colors.tick = ColorFromHTML("666666", 0);
        colors.clipTick = colors.foregroundRMS;  // don't bother with clipping for attenuation
        colors.peak = ColorFromHTML("cccccc", 0);
        break;
      }
    }
    colors.text.Value.w = 0.33f;
    colors.tick.Value.w = 0.33f;
  }


  float rmsDB = 0;
  if (rmsLevel)
  {
    if (cfg.units == VUMeterUnits::Linear)
    {
      rmsDB = M7::math::LinearToDecibels(::fabsf(float(*rmsLevel)));
    }
    else
    {
      rmsDB = float(*rmsLevel);
    }
  }

  float peakDb = 0;
  if (peakLevel)
  {
    if (cfg.units == VUMeterUnits::Linear)
    {
      peakDb = M7::math::LinearToDecibels(::fabsf(float(*peakLevel)));
    }
    else
    {
      peakDb = float(*peakLevel);
    }
  }

  float heldPeakDb = 0;
  if (heldPeakLevel)
  {
    if (cfg.units == VUMeterUnits::Linear)
    {
      heldPeakDb = M7::math::LinearToDecibels(::fabsf(float(*heldPeakLevel)));
    }
    else
    {
      heldPeakDb = float(*heldPeakLevel);
    }
  }

  ImRect bb;
  bb.Min = ImGui::GetCursorScreenPos();
  bb.Max = bb.Min + cfg.size;

  auto DbToY = [&](float db)
  {
    float x = M7::math::lerp_rev(cfg.minDB, cfg.maxDB, db);
    x = M7::math::clamp01(x);
    return M7::math::lerp(bb.Max.y, bb.Min.y, x);
  };

  auto* dl = ImGui::GetWindowDrawList();

  assert(cfg.minDB < cfg.maxDB);  // simplifies logic.

  auto RenderDBRegion = [&](float dB1, float dB2, ImColor bgUnderUnity, ImColor bgOverUnity)
  {
    if (dB1 > dB2)
      std::swap(dB1, dB2);  // make sure in order to simplify logic

    // find the intersection of the desired region and the renderable region.
    float cmin = std::max(dB1, cfg.minDB);
    float cmax = std::min(dB2, cfg.maxDB);
    if (cmin >= cmax)
    {
      return;  // nothing to render.
    }

    // break render rgn into positive & negative dB regions.
    if (cmax <= 0)
    {  // Entire range is negative; render cmin,cmax in under-unity style
      dl->AddRectFilled({bb.Min.x, DbToY(cmin)}, {bb.Max.x, DbToY(cmax)}, bgUnderUnity);
    }
    else if (cmin >= 0)
    {  // Entire range is non-negative, render cmin,cmax in
       // over-unity style
      dl->AddRectFilled({bb.Min.x, DbToY(cmin)}, {bb.Max.x, DbToY(cmax)}, bgOverUnity);
    }
    else
    {
      // Range spans across 0, split into two regions
      dl->AddRectFilled({bb.Min.x, DbToY(cmin)}, {bb.Max.x, DbToY(0)}, bgUnderUnity);
      dl->AddRectFilled({bb.Min.x, DbToY(0)}, {bb.Max.x, DbToY(cmax)}, bgOverUnity);
    }
  };

  RenderDBRegion(cfg.minDB, cfg.maxDB, colors.background, colors.backgroundOverUnity);

  float levelY = DbToY(rmsDB);

  // draw the peak bar
  if (peakLevel)
  {
    float fgBaseDB = cfg.minDB;
    if (cfg.levelMode == VUMeterLevelMode::Attenuation)
      fgBaseDB = 0;
    RenderDBRegion(peakDb, fgBaseDB, colors.foregroundPeak, colors.foregroundOverUnity);
  }

  // draw the RMS bar
  if (rmsLevel)
  {
    float fgBaseDB = cfg.minDB;
    if (cfg.levelMode == VUMeterLevelMode::Attenuation)
      fgBaseDB = 0;
    RenderDBRegion(rmsDB, fgBaseDB, colors.foregroundRMS, colors.foregroundOverUnity);
  }

  // draw clip
  bool allowClipIndicator = true;  // (cfg.levelMode != VUMeterLevelMode::Attenuation);
  if (clipIndicator && *clipIndicator && allowClipIndicator)
  {
    dl->AddRectFilled(bb.Min, {bb.Max.x, bb.Min.y + 8}, colors.clipTick);
  }

  // draw held peak
  if (heldPeakLevel != nullptr)
  {
    float peakY = DbToY(heldPeakDb);
    ImRect threshbb = bb;
    threshbb.Min.y = threshbb.Max.y = peakY;
    float tickHeight = 2.0f;
    threshbb.Max.y += tickHeight;
    dl->AddRectFilled(threshbb.Min, threshbb.Max, colors.peak);
  }

  // draw plot lines
  auto drawTick = [&](float tickdb, const char* txt)
  {
    ImRect tickbb = bb;
    tickbb.Min.y = std::round(DbToY(tickdb));  // make crisp lines plz.
    tickbb.Max.y = tickbb.Min.y;
    dl->AddLine(tickbb.Min, tickbb.Max, colors.text);
    if (txt && allowTickText)
    {
      dl->AddText({tickbb.Min.x, tickbb.Min.y}, colors.text, txt);
    }
  };

  for (auto& t : cfg.ticks)
  {
    drawTick(t.dBvalue, t.caption);
  }

  return ImGui::InvisibleButton(id, cfg.size);
}

inline void VUMeter(const char* id,
                    AnalysisStream& a0,
                    AnalysisStream& a1,
                    ImVec2 size = {30, 300},
                    const std::string& tooltipLeft = "",
                    const std::string& tooltipRight = "")
{
  static const std::vector<VUMeterTick> standardTickSet = {
      {-3.0f, "3db"},
      {-6.0f, "6db"},
      {-12.0f, "12db"},
      {-18.0f, "18db"},
      {-24.0f, "24db"},
      {-30.0f, "30db"},
      {-40.0f, nullptr},
  };

  static const std::vector<VUMeterTick> smallTickSet = {
      {-10.0f, "10db"},
      {-20.0f, nullptr},
      {-30.0f, "30db"},
      {-40.0f, nullptr},
  };

  const VUMeterConfig cfg = {size, VUMeterLevelMode::Audio, VUMeterUnits::Linear, -50, 6, smallTickSet};

  ImGui::PushID(id);
  if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  if (!tooltipLeft.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipLeft.c_str());
    ImGui::EndTooltip();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  if (!tooltipRight.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipRight.c_str());
    ImGui::EndTooltip();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
}

inline void VUMeter(const char* id,
                    IAnalysisStream& a0,
                    IAnalysisStream& a1,
                    const VUMeterConfig& cfg,
                    const std::string& tooltipLeft = "",
                    const std::string& tooltipRight = "")
{
  ImGui::PushID(id);
  if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  if (!tooltipLeft.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipLeft.c_str());
    ImGui::EndTooltip();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  if (!tooltipRight.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipRight.c_str());
    ImGui::EndTooltip();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
}

// Mid-Side VU Meter with blue color scheme
inline void VUMeterMS(const char* id,
                      IAnalysisStream& mid,
                      IAnalysisStream& side,
                      const VUMeterConfig& cfg,
                      const std::string& tooltipMid,
                      const std::string& tooltipSide)
{
  VUMeterColors msColors = GetVUMeterColorsForMidSideLevel();

  ImGui::PushID(id);
  if (VUMeter("VU M",
              &mid.mCurrentRMSValue,
              &mid.mCurrentPeak,
              &mid.mCurrentHeldPeak,
              &mid.mClipIndicator,
              true,
              cfg,
              &msColors))
  {
    mid.Reset();
    side.Reset();
  }
  if (!tooltipMid.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipMid.c_str());
    ImGui::EndTooltip();
  }

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU S",
              &side.mCurrentRMSValue,
              &side.mCurrentPeak,
              &side.mCurrentHeldPeak,
              &side.mClipIndicator,
              false,
              cfg,
              &msColors))
  {
    mid.Reset();
    side.Reset();
  }
  if (!tooltipSide.empty() && ImGui::IsItemHovered())
  {
    ImGui::BeginTooltip();
    ImGui::Text(tooltipSide.c_str());
    ImGui::EndTooltip();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
}

inline void VUMeterAtten(const char* id, IAnalysisStream& a0, IAnalysisStream& a1, ImVec2 size = {30, 300})
{
  static const std::vector<VUMeterTick> smallTickSet = {
      {-1, nullptr},
      {-2, nullptr},
      {-3, "-3"},
      {-6, "-6"},
      {-9, "-9"},
      {-12, "-12"},
  };

  const VUMeterConfig cfg = {size, VUMeterLevelMode::Attenuation, VUMeterUnits::Linear, -12.0f, 0.3f, smallTickSet};

  ImGui::PushID(id);
  if (VUMeter("VU L", nullptr, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", nullptr, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
}

}  // namespace WaveSabreVstLib

#endif
