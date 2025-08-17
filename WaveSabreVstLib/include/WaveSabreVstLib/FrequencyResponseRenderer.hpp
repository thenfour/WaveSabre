#pragma once

#include "Common.h"
#include <d3d9.h>
#include <functional>
#include <queue>
#include <string>
#include <vector>


#define IMGUI_DEFINE_MATH_OPERATORS

#include "../imgui/imgui-knobs.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "VstPlug.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib {
///////////////////////////////////////////////////////////////////////////////////////////////////
struct FrequencyResponseRendererFilter {
  const ImColor thumbColor;
  const BiquadFilter *filter;
};

template <size_t TFilterCount, size_t TParamCount>
struct FrequencyResponseRendererConfig {
  const ImColor backgroundColor;
  const ImColor lineColor;
  const float thumbRadius;
  const std::array<FrequencyResponseRendererFilter, TFilterCount> filters;
  float mParamCacheCopy[TParamCount];
  // Optional: explicit frequency grid configuration (in Hz). If empty, built-in defaults are used.
  std::vector<float> majorFreqTicks{}; // labeled and drawn with major style
  std::vector<float> minorFreqTicks{}; // unlabeled and drawn with minor style
};

template <int Twidth, int Theight, int TsegmentCount, size_t TFilterCount,
          size_t TParamCount>
struct FrequencyResponseRenderer {
  struct ThumbRenderInfo {
    ImColor color;
    ImVec2 point;
  };
  // the response graph is extremely crude; todo:
  // - add the user-selected points to the points list explicitly, to give
  // better looking peaks. then you could reduce # of points.
  // - respect 'thru' member.
  // - display info about freq range and amplitude
  // - adjust amplitude

  static constexpr ImVec2 gSize = {Twidth, Theight};
  static constexpr int gSegmentCount = TsegmentCount;

  const float gMinDB = -12;
  const float gMaxDB = 12;

  // because params change as a result of this immediate gui, we need at least 1
  // full frame of lag to catch param changes correctly. so keep processing
  // multiple frames until things settle. in the meantime, force recalculating.
  int mAdditionalForceCalcFrames = 0;
  ImVec2 mPoints[gSegmentCount]; // actual pixel values.
  std::vector<ThumbRenderInfo> mThumbs;

  // the param cache that points have been calculated on. this way we can only
  // recalc the freq response when params change.
  float mParamCacheCache[TParamCount] = {0};

  int renderSerial = 0;

  // https://forum.juce.com/t/draw-frequency-response-of-filter-from-transfer-function/20669
  // https://www.musicdsp.org/en/latest/Analysis/186-frequency-response-from-biquad-coefficients.html
  // https://dsp.stackexchange.com/questions/3091/plotting-the-magnitude-response-of-a-biquad-filter
  static float
  BiquadMagnitudeForFrequency(const WaveSabreCore::BiquadFilter &bq,
                              double freq) {
    static constexpr double tau = 6.283185307179586476925286766559;
    auto w = tau * freq / Helpers::CurrentSampleRate;

    double ma1 = double(bq.coeffs[1]) / bq.coeffs[0];
    double ma2 = double(bq.coeffs[2]) / bq.coeffs[0];
    double mb0 = double(bq.coeffs[3]) / bq.coeffs[0];
    double mb1 = double(bq.coeffs[4]) / bq.coeffs[0];
    double mb2 = double(bq.coeffs[5]) / bq.coeffs[0];

    double numerator = mb0 * mb0 + mb1 * mb1 + mb2 * mb2 +
                       2 * (mb0 * mb1 + mb1 * mb2) * ::cos(w) +
                       2 * mb0 * mb2 * ::cos(2 * w);
    double denominator = 1 + ma1 * ma1 + ma2 * ma2 +
                         2 * (ma1 + ma1 * ma2) * ::cos(w) +
                         2 * ma2 * ::cos(2 * w);
    double magnitude = ::sqrt(numerator / denominator);
    return (float)magnitude;
  }

  float FreqToX(float hz, ImRect &bb) {
    float underlyingValue = 0;
    // float ktdummy = 0;
    M7::ParamAccessor p{&underlyingValue, 0};
    p.SetFrequencyAssumingNoKeytracking(0, M7::gFilterFreqConfig, hz);
    return M7::math::lerp(bb.Min.x, bb.Max.x, underlyingValue);
    // M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig
    // }; return 0;
  }

  float DBToY(float dB, ImRect &bb) {
    float t01 = M7::math::lerp_rev(gMinDB, gMaxDB, dB);
    t01 = M7::math::clamp01(t01);
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
    // return bb.Max.y - bb.GetHeight() * M7::math::clamp01(magLin);
  }

  void CalculatePoints(
      const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg,
      ImRect &bb) {
    // float underlyingValue = 0;
    float ktdummy = 0;
    // M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig
    // };
    M7::QuickParam param{0, M7::gFilterFreqConfig};

    renderSerial++;

    mThumbs.clear();

    for (auto &f : cfg.filters) {
      if (!f.filter)
        continue; // nullptr values are valid and used when a filter is
                  // bypassed.
      // if (f.filter->thru) continue;
      float freq = f.filter->freq;
      float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
      float magdB = M7::math::LinearToDecibels(magLin);
      mThumbs.push_back({f.thumbColor, {FreqToX(freq, bb), DBToY(magdB, bb)}});
    }

    for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
      param.SetRawValue(float(iseg) / gSegmentCount);
      float freq = param.GetFrequency();
      float magdB = 0;

      for (auto &f : cfg.filters) {
        if (!f.filter)
          continue; // nullptr values are valid and used when a filter is
                    // bypassed.
        // if (f.filter->thru) continue;
        float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
        magdB += M7::math::LinearToDecibels(magLin);
      }

      // float magLin = M7::math::DecibelsToLinear(magdB) / 4; // /4 to
      // basically give a 12db range of display.

      mPoints[iseg] = ImVec2(
          FreqToX(freq,
                  bb), // (bb.Min.x + iseg * bb.GetWidth() / gSegmentCount),
          DBToY(magdB, bb));
    }
  }

  void EnsurePointsPopulated(
      const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg,
      ImRect &bb) {
    bool areEqual = memcmp(cfg.mParamCacheCopy, mParamCacheCache,
                           sizeof(cfg.mParamCacheCopy)) == 0;

    if (areEqual && (mAdditionalForceCalcFrames == 0))
      return;

    mAdditionalForceCalcFrames = areEqual ? mAdditionalForceCalcFrames - 1 : 2;

    memcpy(mParamCacheCache, cfg.mParamCacheCopy, sizeof(cfg.mParamCacheCopy));

    CalculatePoints(cfg, bb);
  }

  // even though you pass in the filters here, you're not allowed to change them
  // due to how things are cached.
  void OnRender(
      const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg) {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = bb.Min + gSize;

    auto *dl = ImGui::GetWindowDrawList();

    ImGui::RenderFrame(bb.Min, bb.Max, cfg.backgroundColor);
    // ImGui::RenderFrame(bb.Min, bb.Max, (renderSerial & 1) ? 0 :
    // cfg.backgroundColor);

    EnsurePointsPopulated(cfg, bb);

    // draw background grid (frequency verticals and dB horizontals)
    ImColor gridMinor = ColorFromHTML("333333", 0.5f);
    ImColor gridMajor = ColorFromHTML("555555", 0.8f);
    ImColor labelColor = ColorFromHTML("AAAAAA", 0.65f);
    const float labelPad = 2.0f;

    // frequency grid (Hz)
    {
      // caller-provided ticks if any; else fallback defaults
      const std::vector<float> &maj = cfg.majorFreqTicks.empty()
                                          ? (const std::vector<float>&)std::vector<float>{100.0f, 1000.0f, 10000.0f, 20000}
                                          : cfg.majorFreqTicks;
      const std::vector<float> &min = cfg.minorFreqTicks.empty()
                                          ? (const std::vector<float>&)std::vector<float>{
          50.0f, 60, 70,80,90,
              200.0f, 300, 400, 500.0f, 600,700,800,900,
              2e3, 3e3, 4e3, 5e3, 6e3, 7e3, 8e3, 9e3,
			  11e3, 12e3, 13e3, 14e3, 15e3, 16e3, 17e3, 18e3, 19e3,
          }
                                          : cfg.minorFreqTicks;

      auto drawTick = [&](float f, bool major) {
        float x = std::round(FreqToX(f, bb));
        if (x < bb.Min.x || x > bb.Max.x) return;
        dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, major ? gridMajor : gridMinor, 1.0f);
        if (major) {
          char txt[16] = {0};
          if (f >= 1000.0f)
            snprintf(txt, sizeof(txt), "%dk", (int)(f / 1000.0f));
          else
            snprintf(txt, sizeof(txt), "%d", (int)f);
          ImVec2 ts = ImGui::CalcTextSize(txt);
          ImVec2 pos = { x - ts.x * 0.5f, bb.Max.y - ts.y - labelPad };
          if (pos.x < bb.Min.x + labelPad) pos.x = bb.Min.x + labelPad;
          if (pos.x + ts.x > bb.Max.x - labelPad) pos.x = bb.Max.x - labelPad - ts.x;
          dl->AddText(pos, labelColor, txt);
        }
      };

      for (float f : min) drawTick(f, false);
      for (float f : maj) drawTick(f, true);
    }

    // dB grid (horizontal)
    {
      const float dbTicks[] = {-12.0f, -6.0f, 0.0f, 6.0f, 12.0f};
      for (float dB : dbTicks) {
        float y = std::round(DBToY(dB, bb));
        if (y >= bb.Min.y && y <= bb.Max.y) {
          bool major = (dB == -12.0f) || (dB == -6.0f) || (dB == 6.0f) ||
                       (dB == 12.0f) || (dB == 0.0f);
          dl->AddLine({bb.Min.x, y}, {bb.Max.x, y},
                      major ? gridMajor : gridMinor, 1.0f);
          // label major ticks (including 0 dB)
          char txt[16] = {0};
          if (dB > 0.0f)
            snprintf(txt, sizeof(txt), "+%gdB", dB);
          else if (dB < 0.0f)
            snprintf(txt, sizeof(txt), "%gdB", dB);
          else
            snprintf(txt, sizeof(txt), "0dB");
          ImVec2 ts = ImGui::CalcTextSize(txt);
          ImVec2 pos = { bb.Min.x + labelPad, y - ts.y - 0.5f * labelPad };
          // keep inside rect
          if (pos.y < bb.Min.y + labelPad) pos.y = bb.Min.y + labelPad;
          if (pos.y + ts.y > bb.Max.y - labelPad) pos.y = bb.Max.y - labelPad - ts.y;
          dl->AddText(pos, labelColor, txt);
        }
      }
    }

    // unity line (0 dB)
    float unityY = std::round(DBToY(0, bb)); // round for crisp line.
    dl->AddLine({bb.Min.x, unityY}, {bb.Max.x, unityY}, ColorFromHTML("444444"),
                1.0f);

    dl->AddPolyline(mPoints, gSegmentCount, cfg.lineColor, 0, 2.0f);

    for (auto &th : mThumbs) {
      dl->AddCircleFilled(th.point, cfg.thumbRadius, th.color);
    }

    ImGui::Dummy(gSize);
  }
};
} // namespace WaveSabreVstLib
