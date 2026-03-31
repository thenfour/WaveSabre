#ifndef __WAVESABREVSTLIB_VUMETER_H__
#define __WAVESABREVSTLIB_VUMETER_H__

#include <imgui.h>
#include "Common.h"
#include "Maj7VstUtils.hpp"
#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <cstdio>
#include <string>
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

  VUMeterColors() = default;
  explicit VUMeterColors(const char* htmlColor)
  {
    // Derive a cohesive palette from the base color using HSV variations.
    // - RMS: bright and saturated
    // - Peak: darker and slightly less saturated than RMS
    // - Background: very dark, low saturation of the same hue
    // - Over-unity and indicators: consistent across themes
    ImColor base = ColorFromHTML(htmlColor);

    float h = 0.0f, s = 0.0f, v = 0.0f;
    ImGui::ColorConvertRGBtoHSV(base.Value.x, base.Value.y, base.Value.z, h, s, v);

    auto clamp01 = [](float x) { return x < 0.0f ? 0.0f : (x > 1.0f ? 1.0f : x); };

    // Tuned ranges for a readable meter with the provided hue
    float sStrong = clamp01(std::max(0.60f, s * 1.15f));
    float sMid    = clamp01(std::max(0.35f, s * 0.85f));
    float sBg     = clamp01(std::min(0.35f, s * 0.35f));

    float vBright = clamp01(std::max(0.72f, std::min(0.96f, v * 1.20f)));
    float vPeak   = clamp01(std::max(0.42f, std::min(0.70f, v * 0.75f)));
    float vBg     = clamp01(std::max(0.08f, std::min(0.20f, v * 0.20f)));

    background        = ImColor::HSV(h, sBg,     vBg,     1.0f);
    foregroundRMS     = ImColor::HSV(h, sStrong, vBright, 1.0f);
    foregroundPeak    = ImColor::HSV(h, sMid,    vPeak,   1.0f);

    // Keep critical UI colors consistent for readability and status signaling
    backgroundOverUnity = ColorFromHTML("440000");
    foregroundOverUnity = ColorFromHTML("cccc00");
    clipTick            = ColorFromHTML("ff0000");

    // Text/ticks are semi-transparent to avoid overpowering the meter
    text = ColorFromHTML("ffffff", 0.33f);
    tick = ColorFromHTML("00ffff", 0.33f);

    // Peak marker: same hue as RMS but semi-transparent
    peak = foregroundRMS;
    peak.Value.w = 0.8f;
  }
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

enum class VUMeterRenderStyle
{
  ContinuousFill,
  SteppedSegments,
};

struct VUMeterStepOptions
{
  float segmentPx = 3.0f;  // segment block height in pixels
  float gapPx = 1.0f;      // pixel gap between segments
};

struct VUMeterConfig
{
  ImVec2 size;
  VUMeterLevelMode levelMode;
  VUMeterUnits units;
  float minDB;
  float maxDB;
  std::vector<VUMeterTick> ticks;
  // New optional rendering customization (defaults preserve existing behavior)
  VUMeterRenderStyle renderStyle = VUMeterRenderStyle::ContinuousFill;
  VUMeterStepOptions stepOptions{};
};

struct VUMeterTooltipRow
{
  std::string name;
  bool hasSwatch = false;
  ImColor swatchColor;
  bool separatorBefore = false;
  bool hasPeak = false;
  float peakDB = 0;
  bool hasRMS = false;
  float rmsDB = 0;
  bool hasClip = false;
  float clipDB = 0;
};

static inline void VUMeterResolveColors(const VUMeterConfig& cfg,
                                        const VUMeterColors* colorsOverride,
                                        VUMeterColors& colors)
{
  if (colorsOverride)
  {
    colors = *colorsOverride;
    return;
  }

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
      colors.clipTick = colors.foregroundRMS;
      colors.peak = ColorFromHTML("cccccc", 0);
      break;
    }
  }

  colors.text.Value.w = 0.33f;
  colors.tick.Value.w = 0.33f;
}

static inline float VUMeterValueToDisplayDB(const VUMeterConfig& cfg, double value)
{
  if (cfg.units == VUMeterUnits::Linear)
  {
    return M7::math::LinearToDecibels((float)fabs(value));
  }

  return (float)value;
}

static inline VUMeterTooltipRow MakeVUMeterTooltipRow(const std::string& name,
                                                      const IAnalysisStream& analysis,
                                                      const VUMeterConfig& cfg,
                                                      bool includeRMS,
                                                      bool includeClip,
                                                      ImColor swatchColor = ImColor(0, 0, 0, 0))
{
  VUMeterTooltipRow row;
  row.name = name;
  row.hasSwatch = swatchColor.Value.w > 0.0f;
  row.swatchColor = swatchColor;
  row.hasPeak = true;
  row.peakDB = VUMeterValueToDisplayDB(cfg, analysis.mCurrentHeldPeak);
  row.hasRMS = includeRMS;
  if (includeRMS)
  {
    row.rmsDB = VUMeterValueToDisplayDB(cfg, analysis.mCurrentRMSValue);
  }

  row.hasClip = includeClip && analysis.mClipIndicator;
  if (row.hasClip)
  {
    row.clipDB = row.peakDB;
  }

  return row;
}

static inline void VUMeterFormatTooltipValue(char* buffer, size_t bufferSize, float value)
{
  if (!std::isfinite(value))
  {
    std::snprintf(buffer, bufferSize, " -inf");
    return;
  }

  std::snprintf(buffer, bufferSize, "%+6.1f", value);
}

static inline void VUMeterTooltipNameCell(const VUMeterTooltipRow& row)
{
  if (row.hasSwatch)
  {
    const float swatchSize = std::floor(ImGui::GetTextLineHeight() * 0.60f);
    ImVec2 swatchMin = ImGui::GetCursorScreenPos();
    swatchMin.y += std::floor((ImGui::GetTextLineHeight() - swatchSize) * 0.5f);
    ImVec2 swatchMax = {swatchMin.x + swatchSize, swatchMin.y + swatchSize};
    ImGui::GetWindowDrawList()->AddRectFilled(swatchMin, swatchMax, row.swatchColor, 1.5f);
    ImGui::Dummy({swatchSize, 0.0f});
    ImGui::SameLine(0, 6.0f);
  }

  ImGui::TextUnformatted(row.name.c_str());
}

static inline void VUMeterTooltipValueCell(const char* value)
{
  const float available = ImGui::GetContentRegionAvail().x;
  const float textWidth = ImGui::CalcTextSize(value).x;
  if (available > textWidth)
  {
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + available - textWidth);
  }
  ImGui::TextUnformatted(value);
}

static inline void VUMeterRenderTooltipRows(const std::vector<VUMeterTooltipRow>& rows)
{
  if (rows.empty())
  {
    return;
  }

  if (ImGui::BeginTable("##vu_tooltip", 4, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
  {
    ImGui::TableSetupColumn("Meter");
    ImGui::TableSetupColumn("Peak(dB)", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("RMS(dB)", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableSetupColumn("Clip(dB)", ImGuiTableColumnFlags_WidthFixed);
    ImGui::TableHeadersRow();

    for (const auto& row : rows)
    {
      if (row.separatorBefore)
      {
        ImGui::TableNextRow(ImGuiTableRowFlags_None, 6.0f);
        ImGui::TableSetColumnIndex(0);
        ImVec2 lineMin = ImGui::GetCursorScreenPos();
        const float lineY = lineMin.y + 2.0f;
        const float lineRight = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
        ImGui::GetWindowDrawList()->AddLine({lineMin.x, lineY},
                                            {lineRight, lineY},
                                            ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.08f)));
        ImGui::Dummy({0.0f, 3.0f});
      }

      char peakBuffer[32] = "";
      char rmsBuffer[32] = "";
      char clipBuffer[32] = "";

      if (row.hasPeak)
      {
        VUMeterFormatTooltipValue(peakBuffer, sizeof(peakBuffer), row.peakDB);
      }
      if (row.hasRMS)
      {
        VUMeterFormatTooltipValue(rmsBuffer, sizeof(rmsBuffer), row.rmsDB);
      }
      if (row.hasClip)
      {
        VUMeterFormatTooltipValue(clipBuffer, sizeof(clipBuffer), row.clipDB);
      }

      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
  VUMeterTooltipNameCell(row);
      ImGui::TableSetColumnIndex(1);
      VUMeterTooltipValueCell(peakBuffer);
      ImGui::TableSetColumnIndex(2);
      VUMeterTooltipValueCell(row.hasRMS ? rmsBuffer : "");
      ImGui::TableSetColumnIndex(3);
      VUMeterTooltipValueCell(row.hasClip ? clipBuffer : "");
    }

    ImGui::EndTable();
  }
}

struct VUMeterTooltipStripScope
{
  bool mIdSet = false;
  bool mPendingSubBankSeparator = false;
  std::vector<VUMeterTooltipRow> mRows;

  explicit VUMeterTooltipStripScope(const char* id)
      : mIdSet(true)
  {
    ImGui::BeginGroup();
    ImGui::PushID(id);
  }

  VUMeterTooltipStripScope()
  {
    ImGui::BeginGroup();
  }

  ~VUMeterTooltipStripScope()
  {
    if (mIdSet)
    {
      ImGui::PopID();
    }

    ImGui::EndGroup();
    const ImVec2 groupMin = ImGui::GetItemRectMin();
    const ImVec2 groupMax = ImGui::GetItemRectMax();
    if (!mRows.empty() && ImGui::IsMouseHoveringRect(groupMin, groupMax, true))
    {
      ImGui::BeginTooltip();
      VUMeterRenderTooltipRows(mRows);
      ImGui::EndTooltip();
    }
  }

  void AddRow(const VUMeterTooltipRow& row)
  {
    VUMeterTooltipRow resolvedRow = row;
    resolvedRow.separatorBefore = resolvedRow.separatorBefore || mPendingSubBankSeparator;
    mPendingSubBankSeparator = false;
    mRows.push_back(resolvedRow);
  }

  void BeginSubBank()
  {
    if (!mRows.empty())
    {
      mPendingSubBankSeparator = true;
    }
  }

  void AddAnalysisRow(const std::string& name,
                      const IAnalysisStream& analysis,
                      const VUMeterConfig& cfg,
                      bool includeRMS,
                      bool includeClip = true,
                      ImColor swatchColor = ImColor(0, 0, 0, 0))
  {
    AddRow(MakeVUMeterTooltipRow(name, analysis, cfg, includeRMS, includeClip, swatchColor));
  }
};

// Map a dB value to a Y coordinate inside the meter bounding box.
static inline float VUMeterDbToY(const ImRect& bb, const VUMeterConfig& cfg, float db)
{
  float x = M7::math::lerp_rev(cfg.minDB, cfg.maxDB, db);
  x = M7::math::clamp01(x);
  return M7::math::lerp(bb.Max.y, bb.Min.y, x);
}

// Fill the range [dB1, dB2] with colors, splitting at 0 dB to use under/over-unity colors.
static inline void VUMeterRenderRegionContinuous(ImDrawList* dl,
                                                 const ImRect& bb,
                                                 const VUMeterConfig& cfg,
                                                 float dB1,
                                                 float dB2,
                                                 ImColor colUnderUnity,
                                                 ImColor colOverUnity)
{
  if (dB1 > dB2)
    std::swap(dB1, dB2);  // ensure order

  // intersection with renderable region
  float cmin = std::max(dB1, cfg.minDB);
  float cmax = std::min(dB2, cfg.maxDB);
  if (cmin >= cmax)
    return;  // nothing to render

  // Render depending on relation to 0 dB
  if (cmax <= 0)
  {
    // Entire range is <= 0 dB
    dl->AddRectFilled({bb.Min.x, VUMeterDbToY(bb, cfg, cmin)}, {bb.Max.x, VUMeterDbToY(bb, cfg, cmax)}, colUnderUnity);
  }
  else if (cmin >= 0)
  {
    // Entire range is >= 0 dB
    dl->AddRectFilled({bb.Min.x, VUMeterDbToY(bb, cfg, cmin)}, {bb.Max.x, VUMeterDbToY(bb, cfg, cmax)}, colOverUnity);
  }
  else
  {
    // Range spans across 0, split into two regions
    dl->AddRectFilled({bb.Min.x, VUMeterDbToY(bb, cfg, cmin)}, {bb.Max.x, VUMeterDbToY(bb, cfg, 0)}, colUnderUnity);
    dl->AddRectFilled({bb.Min.x, VUMeterDbToY(bb, cfg, 0)}, {bb.Max.x, VUMeterDbToY(bb, cfg, cmax)}, colOverUnity);
  }
}

// this kinda looks OK -- the idea is to avoid pixel-based vertical rendering which can look jumpy and "raw".
// stepped meters feel more calm and don't distract with insignificant changes in signal. however, it just kinda
// looks messy at the moment. something to refine later maybe. the reality is that it's not really important.
// - consider rendering in gaps
// - avoid overlaying on stepped meters (put ticks & text outside)
static inline void VUMeterRenderRegionSteppedRange(ImDrawList* dl,
                                                   const ImRect& bb,
                                                   const VUMeterConfig& cfg,
                                                   float loDb,
                                                   float hiDb,
                                                   ImColor color,
                                                   const VUMeterStepOptions& opt)
{
  // Compute target fill region in pixel Y (top smaller, bottom larger)
  float yA = std::round(VUMeterDbToY(bb, cfg, loDb));
  float yB = std::round(VUMeterDbToY(bb, cfg, hiDb));
  float yMin = std::min(yA, yB);
  float yMax = std::max(yA, yB);

  // Segment grid anchored to the bottom of the meter
  const float seg = (opt.segmentPx > 0.0f) ? opt.segmentPx : 1.0f;
  const float gap = std::max(0.0f, opt.gapPx);
  const float step = seg + gap;

  const float yBottom = std::round(bb.Max.y);
  const float yTopLimit = std::round(bb.Min.y);

  for (float ySegBot = yBottom; ySegBot > yTopLimit; ySegBot -= step)
  {
    float ySegTop = ySegBot - seg;

    // Clamp segment to the meter bounds
    float ySegTopClamped = std::max(ySegTop, yTopLimit);
    float ySegBotClamped = std::min(ySegBot, yBottom);
    if (ySegBotClamped <= ySegTopClamped)
      continue;

    // Decide to fill this segment based on its midpoint being within target range
    float yMid = (ySegTopClamped + ySegBotClamped) * 0.5f;
    if (yMid >= yMin && yMid <= yMax)
    {
      dl->AddRectFilled({bb.Min.x, ySegTopClamped}, {bb.Max.x, ySegBotClamped}, color);
    }
  }
}

static inline void VUMeterRenderRegionStepped(ImDrawList* dl,
                                              const ImRect& bb,
                                              const VUMeterConfig& cfg,
                                              float dB1,
                                              float dB2,
                                              ImColor colUnderUnity,
                                              ImColor colOverUnity,
                                              const VUMeterStepOptions& opt)
{
  if (dB1 > dB2)
    std::swap(dB1, dB2);

  float cmin = std::max(dB1, cfg.minDB);
  float cmax = std::min(dB2, cfg.maxDB);
  if (cmin >= cmax)
    return;

  if (cmax <= 0)
  {
    // entirely under unity
    VUMeterRenderRegionSteppedRange(dl, bb, cfg, cmin, cmax, colUnderUnity, opt);
  }
  else if (cmin >= 0)
  {
    // entirely over unity
    VUMeterRenderRegionSteppedRange(dl, bb, cfg, cmin, cmax, colOverUnity, opt);
  }
  else
  {
    // split at 0 dB
    VUMeterRenderRegionSteppedRange(dl, bb, cfg, cmin, 0.0f, colUnderUnity, opt);
    VUMeterRenderRegionSteppedRange(dl, bb, cfg, 0.0f, cmax, colOverUnity, opt);
  }
}

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
  VUMeterResolveColors(cfg, colorsOverride, colors);


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
  bb.Max = {bb.Min.x + cfg.size.x, bb.Min.y + cfg.size.y};

  auto* dl = ImGui::GetWindowDrawList();

  assert(cfg.minDB < cfg.maxDB);  // simplifies logic.

  // background (keep continuous for readability)
  VUMeterRenderRegionContinuous(dl, bb, cfg, cfg.minDB, cfg.maxDB, colors.background, colors.backgroundOverUnity);

  // draw the peak bar
  if (peakLevel)
  {
    float fgBaseDB = cfg.minDB;
    if (cfg.levelMode == VUMeterLevelMode::Attenuation)
      fgBaseDB = 0;

    if (cfg.renderStyle == VUMeterRenderStyle::SteppedSegments)
      VUMeterRenderRegionStepped(dl, bb, cfg, peakDb, fgBaseDB, colors.foregroundPeak, colors.foregroundOverUnity, cfg.stepOptions);
    else
      VUMeterRenderRegionContinuous(dl, bb, cfg, peakDb, fgBaseDB, colors.foregroundPeak, colors.foregroundOverUnity);
  }

  // draw the RMS bar
  if (rmsLevel)
  {
    float fgBaseDB = cfg.minDB;
    if (cfg.levelMode == VUMeterLevelMode::Attenuation)
      fgBaseDB = 0;

    if (cfg.renderStyle == VUMeterRenderStyle::SteppedSegments)
      VUMeterRenderRegionStepped(dl, bb, cfg, rmsDB, fgBaseDB, colors.foregroundRMS, colors.foregroundOverUnity, cfg.stepOptions);
    else
      VUMeterRenderRegionContinuous(dl, bb, cfg, rmsDB, fgBaseDB, colors.foregroundRMS, colors.foregroundOverUnity);
  }

  // draw clip
  bool allowClipIndicator = (cfg.levelMode != VUMeterLevelMode::Attenuation);
  if (clipIndicator && *clipIndicator && allowClipIndicator)
  {
    dl->AddRectFilled(bb.Min, {bb.Max.x, bb.Min.y + 8}, colors.clipTick);
  }

  // draw held peak
  if (heldPeakLevel != nullptr)
  {
    float peakY = VUMeterDbToY(bb, cfg, heldPeakDb);
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
    tickbb.Min.y = std::round(VUMeterDbToY(bb, cfg, tickdb));  // make crisp lines plz.
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
                    const std::string& tooltipRight = "",
          const char* htmlColor = nullptr,
          VUMeterTooltipStripScope* tooltipGroup = nullptr)
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

  const VUMeterConfig cfg = {
      size,
      VUMeterLevelMode::Audio,
      VUMeterUnits::Linear,
      -50, 6, smallTickSet,
  };

  VUMeterColors* pColorOverride = nullptr;
  VUMeterColors colorOverride;
  if (htmlColor)
  {
    colorOverride = VUMeterColors{htmlColor};
    pColorOverride = &colorOverride;
  }

  VUMeterColors resolvedColors;
  VUMeterResolveColors(cfg, pColorOverride, resolvedColors);

  ImGui::BeginGroup();
  ImGui::PushID(id);
  if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg, pColorOverride))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg, pColorOverride))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::EndGroup();

  if (tooltipGroup)
  {
    tooltipGroup->BeginSubBank();
    if (!tooltipLeft.empty()) tooltipGroup->AddAnalysisRow(tooltipLeft, a0, cfg, true, true, resolvedColors.foregroundRMS);
    if (!tooltipRight.empty()) tooltipGroup->AddAnalysisRow(tooltipRight, a1, cfg, true, true, resolvedColors.foregroundRMS);
  }
  else if ((!tooltipLeft.empty() || !tooltipRight.empty()) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
  {
    std::vector<VUMeterTooltipRow> rows;
    if (!tooltipLeft.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipLeft, a0, cfg, true, true, resolvedColors.foregroundRMS));
    if (!tooltipRight.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipRight, a1, cfg, true, true, resolvedColors.foregroundRMS));
    if (!rows.empty())
    {
      ImGui::BeginTooltip();
      VUMeterRenderTooltipRows(rows);
      ImGui::EndTooltip();
    }
  }
}

inline void VUMeter(const char* id,
                    IAnalysisStream& a0,
                    IAnalysisStream& a1,
                    const VUMeterConfig& cfg,
                    const std::string& tooltipLeft = "",
                    const std::string& tooltipRight = "",
                    VUMeterTooltipStripScope* tooltipGroup = nullptr)
{
  VUMeterColors resolvedColors;
  VUMeterResolveColors(cfg, nullptr, resolvedColors);

  ImGui::BeginGroup();
  ImGui::PushID(id);
  if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::EndGroup();

  if (tooltipGroup)
  {
    tooltipGroup->BeginSubBank();
    if (!tooltipLeft.empty()) tooltipGroup->AddAnalysisRow(tooltipLeft, a0, cfg, true, true, resolvedColors.foregroundRMS);
    if (!tooltipRight.empty()) tooltipGroup->AddAnalysisRow(tooltipRight, a1, cfg, true, true, resolvedColors.foregroundRMS);
  }
  else if ((!tooltipLeft.empty() || !tooltipRight.empty()) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
  {
    std::vector<VUMeterTooltipRow> rows;
    if (!tooltipLeft.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipLeft, a0, cfg, true, true, resolvedColors.foregroundRMS));
    if (!tooltipRight.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipRight, a1, cfg, true, true, resolvedColors.foregroundRMS));
    if (!rows.empty())
    {
      ImGui::BeginTooltip();
      VUMeterRenderTooltipRows(rows);
      ImGui::EndTooltip();
    }
  }
}

// Mid-Side VU Meter with blue color scheme
inline void VUMeterMS(const char* id,
                      IAnalysisStream& mid,
                      IAnalysisStream& side,
                      const VUMeterConfig& cfg,
                      const std::string& tooltipMid,
                      const std::string& tooltipSide,
                      VUMeterTooltipStripScope* tooltipGroup = nullptr)
{
  VUMeterColors msColors = GetVUMeterColorsForMidSideLevel();

  ImGui::BeginGroup();
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
  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::EndGroup();

  if (tooltipGroup)
  {
    tooltipGroup->BeginSubBank();
    if (!tooltipMid.empty()) tooltipGroup->AddAnalysisRow(tooltipMid, mid, cfg, true, true, msColors.foregroundRMS);
    if (!tooltipSide.empty()) tooltipGroup->AddAnalysisRow(tooltipSide, side, cfg, true, true, msColors.foregroundRMS);
  }
  else if ((!tooltipMid.empty() || !tooltipSide.empty()) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
  {
    std::vector<VUMeterTooltipRow> rows;
    if (!tooltipMid.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipMid, mid, cfg, true, true, msColors.foregroundRMS));
    if (!tooltipSide.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipSide, side, cfg, true, true, msColors.foregroundRMS));
    if (!rows.empty())
    {
      ImGui::BeginTooltip();
      VUMeterRenderTooltipRows(rows);
      ImGui::EndTooltip();
    }
  }
}

inline void VUMeterAtten(const char* id,
                        IAnalysisStream& a0,
                        IAnalysisStream& a1,
                        ImVec2 size = {30, 300},
                        const std::string& tooltipLeft = "",
                        const std::string& tooltipRight = "",
                        VUMeterTooltipStripScope* tooltipGroup = nullptr)
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
  VUMeterColors resolvedColors;
  VUMeterResolveColors(cfg, nullptr, resolvedColors);

  ImGui::BeginGroup();
  ImGui::PushID(id);
  if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {1, 0});
  ImGui::SameLine();
  if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
  {
    a0.Reset();
    a1.Reset();
  }
  ImGui::PopID();
  ImGui::PopStyleVar();
  ImGui::EndGroup();

  if (tooltipGroup)
  {
    tooltipGroup->BeginSubBank();
    if (!tooltipLeft.empty()) tooltipGroup->AddAnalysisRow(tooltipLeft, a0, cfg, true, false, resolvedColors.foregroundRMS);
    if (!tooltipRight.empty()) tooltipGroup->AddAnalysisRow(tooltipRight, a1, cfg, true, false, resolvedColors.foregroundRMS);
  }
  else if ((!tooltipLeft.empty() || !tooltipRight.empty()) && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
  {
    std::vector<VUMeterTooltipRow> rows;
    if (!tooltipLeft.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipLeft, a0, cfg, true, false, resolvedColors.foregroundRMS));
    if (!tooltipRight.empty()) rows.push_back(MakeVUMeterTooltipRow(tooltipRight, a1, cfg, true, false, resolvedColors.foregroundRMS));
    if (!rows.empty())
    {
      ImGui::BeginTooltip();
      VUMeterRenderTooltipRows(rows);
      ImGui::EndTooltip();
    }
  }
}

}  // namespace WaveSabreVstLib

#endif
