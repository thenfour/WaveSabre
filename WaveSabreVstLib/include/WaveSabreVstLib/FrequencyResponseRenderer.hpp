#pragma once

#include "Common.h"
#include <d3d9.h>
#include <functional>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>  // for std::sort


#define IMGUI_DEFINE_MATH_OPERATORS

#include "../imgui/imgui-knobs.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "VstPlug.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib {
///////////////////////////////////////////////////////////////////////////////////////////////////
struct FrequencyResponseRendererFilter {
  const char *thumbColor;
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
  
  // Multiple FFT spectrum overlays with independent scaling (Professional approach)
  struct FFTAnalysisOverlay {
    const IFrequencyAnalysis* frequencyAnalysis = nullptr;  // FFT data source
    ImColor fftColor = ColorFromHTML("4488FF", 0.6f);       // spectrum line color
    ImColor fftFillColor = ColorFromHTML("4488FF", 0.2f);   // spectrum fill color
    bool useRightChannel = false;                           // which channel to display
    bool enableFftFill = true;                              // enable filled area under curve
    const char* label = nullptr;                            // optional label for legend/display
  };
  
  std::vector<FFTAnalysisOverlay> fftOverlays{};            // Multiple FFT overlays
  
  // Independent FFT Y-axis scaling (shared by all overlays)
  float fftDisplayMinDB = -60.0f;  // Noise floor (iZotope style: -60dB to 0dB)
  float fftDisplayMaxDB = 0.0f;    // Digital maximum (0dB)
  bool useIndependentFFTScale = true;  // Use separate scale for FFT vs EQ response
  
  // Legacy single FFT support (for backward compatibility)
  const IFrequencyAnalysis* frequencyAnalysis = nullptr;
  ImColor fftColor = ColorFromHTML("4488FF", 0.6f);
  ImColor fftFillColor = ColorFromHTML("4488FF", 0.2f);
  bool useRightChannel = false;
  bool enableFftFill = true;
};

template <int Twidth, int Theight, int TsegmentCount, size_t TFilterCount,
          size_t TParamCount, bool TshowGridLabels>
struct FrequencyResponseRenderer {
  struct ThumbRenderInfo {
    const char * color;
    ImVec2 point;
  };
  // the response graph is extremely crude; todo:
  // - add the user-selected points to the points list explicitly, to give
  // better looking peaks. then you could reduce # of points.
  // - respect 'thru' member.
  // - display info about freq range and amplitude
  // - adjust amplitude

  static constexpr ImVec2 gSize = {Twidth, Theight};
  static constexpr int gPixelsPerSegment = 2;
  static constexpr int gSegmentCount = std::max(
      5, Twidth / gPixelsPerSegment); // TsegmentCount; -- just go for smooth 4-px per segment

  // dynamic display range (defaults to +-12 dB)
  float mCurrentHalfRangeDB = 12.0f;
  float mDisplayMinDB = -12.0f;
  float mDisplayMaxDB = 12.0f;
  int mShrinkCountdown = 0; // hysteresis for shrinking range

  // because params change as a result of this immediate gui, we need at least 1
  // full frame of lag to catch param changes correctly. so keep processing
  // multiple frames until things settle. in the meantime, force recalculating.
  int mAdditionalForceCalcFrames = 0;

  // cached per-segment values
  float mMagdB[gSegmentCount] = {0};      // Filter magnitude response
  float mX[gSegmentCount] = {0};          // Screen X coordinates
  
  // Screen-space FFT response cache (unified approach)
  struct FFTSegmentCache {
    float magnitudeDB;
    bool valid;
    FFTSegmentCache() : magnitudeDB(-100.0f), valid(false) {}
  };
  
  // Cache for each FFT overlay at screen resolution
  std::vector<std::array<FFTSegmentCache, gSegmentCount>> mFFTCache;

  // computed visible window excluding deep stopband edges
  int mVisibleLeftIndex = 0;
  int mVisibleRightIndex = gSegmentCount - 1;

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
    M7::ParamAccessor p{&underlyingValue, 0};
    p.SetFrequencyAssumingNoKeytracking(0, M7::gFilterFreqConfig, hz);
    return M7::math::lerp(bb.Min.x, bb.Max.x, underlyingValue);
  }

  float DBToY(float dB, ImRect &bb) {
    float t01 = M7::math::lerp_rev(mDisplayMinDB, mDisplayMaxDB, dB);
    t01 = M7::math::clamp01(t01);
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
  }

  // Separate scaling function for FFT spectrum (Professional approach)
  float FFTDBToY(float dB, ImRect &bb, const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg) {
    if (cfg.useIndependentFFTScale) {
      // Use independent FFT scale (noise floor to 0dB)
      float t01 = M7::math::lerp_rev(cfg.fftDisplayMinDB, cfg.fftDisplayMaxDB, dB);
      t01 = M7::math::clamp01(t01);
      return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
    } else {
      // Fallback to shared scale with EQ response
      return DBToY(dB, bb);
    }
  }

  void CalculatePoints(
      const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg,
      ImRect &bb) {
    M7::QuickParam param{0, M7::gFilterFreqConfig};

    renderSerial++;

    // Don't compute thumbs here; do it after dynamic range is set
    mThumbs.clear();

    // Calculate filter response at screen resolution (parameter-dependent only)
    for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
      param.SetRawValue(float(iseg) / gSegmentCount);
      float freq = param.GetFrequency();
      
      // Calculate filter magnitude response (existing logic)
      float filterMagdB = 0;
      for (auto &f : cfg.filters) {
        if (!f.filter)
          continue; // nullptr values are valid and used when a filter is bypassed.
        float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
        filterMagdB += M7::math::LinearToDecibels(magLin);
      }

      mX[iseg] = FreqToX(freq, bb);
      mMagdB[iseg] = filterMagdB;
    }
  }

  // Separate function: Always calculate FFT data (independent of parameters)
  void CalculateFFTPoints(
      const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg,
      ImRect &bb) {
    M7::QuickParam param{0, M7::gFilterFreqConfig};

    // Ensure FFT cache is properly sized for all overlays
    if (mFFTCache.size() != cfg.fftOverlays.size()) {
      mFFTCache.resize(cfg.fftOverlays.size());
    }

    // Always recalculate FFT data every frame (dynamic data)
    for (size_t overlayIndex = 0; overlayIndex < cfg.fftOverlays.size(); ++overlayIndex) {
      const auto& overlay = cfg.fftOverlays[overlayIndex];
      auto& overlayCache = mFFTCache[overlayIndex];
      
      for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
        param.SetRawValue(float(iseg) / gSegmentCount);
        float freq = param.GetFrequency();
        
        auto& cache = overlayCache[iseg];
        
        if (overlay.frequencyAnalysis) {
          // Screen-space sampling: get magnitude at exact frequency
          cache.magnitudeDB = overlay.frequencyAnalysis->GetMagnitudeAtFrequency(freq, overlay.useRightChannel);
          cache.valid = true;
        } else {
          cache.magnitudeDB = -100.0f; // Silence
          cache.valid = false;
        }
      }
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

    // Calculate filter response (parameter-dependent, cached)
    EnsurePointsPopulated(cfg, bb);
    
    // Calculate FFT response (always updated, independent of parameters)
    CalculateFFTPoints(cfg, bb);

    // Determine visible in-band window (exclude deep stopband)
    const float cutThreshold = -24.0f;                  // dB
    const int margin = std::max(1, gSegmentCount / 50); // ~2%

    int left = 0;
    for (int i = 0; i < gSegmentCount; ++i) {
      if (mMagdB[i] > cutThreshold) {
        left = std::max(0, i - margin);
        break;
      }
      if (i == gSegmentCount - 1)
        left = 0; // none found
    }

    int right = gSegmentCount - 1;
    for (int i = gSegmentCount - 1; i >= 0; --i) {
      if (mMagdB[i] > cutThreshold) {
        right = std::min(gSegmentCount - 1, i + margin);
        break;
      }
      if (i == 0)
        right = gSegmentCount - 1;
    }

    if (right < left) {
      left = 0;
      right = gSegmentCount - 1;
    }

    mVisibleLeftIndex = left;
    mVisibleRightIndex = right;

    // Compute required scale using 6dB steps with hysteresis
    float posPeak = -1e9f;
    float negPeak = 1e9f;
    for (int i = left; i <= right; ++i) {
      posPeak = std::max(posPeak, mMagdB[i]);
      negPeak = std::min(negPeak, mMagdB[i]);
    }

    auto stepUp6 = [](float v) { return 6.0f * std::ceil(v / 6.0f); };

    const float baseHalf = 12.0f;
    const float maxHalf = 48.0f;
    float posNeed = stepUp6(std::max(baseHalf, std::max(0.0f, posPeak)));
    float negNeed = stepUp6(std::max(baseHalf, std::max(0.0f, -negPeak)));
    float suggestedHalf = std::min(maxHalf, std::max(posNeed, negNeed));

    // hysteresis: expand immediately, shrink after N frames stable
    const int shrinkFrames = 6;
    if (suggestedHalf > mCurrentHalfRangeDB) {
      mCurrentHalfRangeDB = suggestedHalf;
      mShrinkCountdown = shrinkFrames;
    } else if (suggestedHalf < mCurrentHalfRangeDB) {
      if (mShrinkCountdown > 0) {
        --mShrinkCountdown;
      } else {
        mCurrentHalfRangeDB = suggestedHalf;
      }
    } else {
      mShrinkCountdown = shrinkFrames;
    }

    mDisplayMinDB = -mCurrentHalfRangeDB;
    mDisplayMaxDB = +mCurrentHalfRangeDB;

    // draw background grid (frequency verticals and dynamic dB horizontals)
    ImColor gridMinor = ColorFromHTML("333333", 0.5f);
    ImColor gridMajor = ColorFromHTML("555555", 0.8f);
    ImColor labelColor = ColorFromHTML("AAAAAA", 0.65f);
    const float labelPad = 2.0f;

    // frequency grid (Hz)
    {
      // caller-provided ticks if any; else fallback defaults
      const std::vector<float> &maj =
          cfg.majorFreqTicks.empty()
              ? (const std::vector<float> &)std::vector<float>{100.0f, 1000.0f,
                                                               10000.0f,
                                                               20000.0f}
              : cfg.majorFreqTicks;
      const std::vector<float> &min = cfg.minorFreqTicks.empty()
                                          ? (const std::vector<float>&)std::vector<float>{
          50.0f, 60, 70,80,90,
              200.0f, 300, 400, 500.0f, 600,700,800,900,
              2000.0f, 3000.0f, 4000.0f, 5000.0f, 6000.0f, 7000.0f, 8000.0f, 9000.0f,
              11000.0f, 12000.0f, 13000.0f, 14000.0f, 15000.0f, 16000.0f, 17000.0f, 18000.0f, 19000.0f,
          }
                                          : cfg.minorFreqTicks;

      auto drawTick = [&](float f, bool major) {
        float x = std::round(FreqToX(f, bb));
        if (x < bb.Min.x || x > bb.Max.x)
          return;
        dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, major ? gridMajor : gridMinor,
                    1.0f);
        if (major && TshowGridLabels) {
          char txt[16] = {0};
          if (f >= 1000.0f)
            snprintf(txt, sizeof(txt), "%dk", (int)(f / 1000.0f));
          else
            snprintf(txt, sizeof(txt), "%d", (int)f);
          ImVec2 ts = ImGui::CalcTextSize(txt);
          ImVec2 pos = {x - ts.x * 0.5f, bb.Max.y - ts.y - labelPad};
          if (pos.x < bb.Min.x + labelPad)
            pos.x = bb.Min.x + labelPad;
          if (pos.x + ts.x > bb.Max.x - labelPad)
            pos.x = bb.Max.x - labelPad - ts.x;
          dl->AddText(pos, labelColor, txt);
        }
      };

      for (float f : min)
        drawTick(f, false);
      for (float f : maj)
        drawTick(f, true);
    }

    // dB grid (horizontal): multiples of 6 dB within [mDisplayMinDB,
    // mDisplayMaxDB]
    {
      int half = (int)std::ceil(mCurrentHalfRangeDB / 6.0f) * 6;
      for (int v = -half; v <= half; v += 6) {
        float dB = (float)v;
        float y = std::round(DBToY(dB, bb));
        if (y < bb.Min.y || y > bb.Max.y)
          continue;
        bool major = (v == 0) || (v % 12 == 0);
        dl->AddLine({bb.Min.x, y}, {bb.Max.x, y}, major ? gridMajor : gridMinor,
                    1.0f);
        if (TshowGridLabels) {
          char txt[16] = {0};
          if (v > 0)
            snprintf(txt, sizeof(txt), "+%ddB", v);
          else if (v < 0)
            snprintf(txt, sizeof(txt), "%ddB", v);
          else
            snprintf(txt, sizeof(txt), "0dB");
          ImVec2 ts = ImGui::CalcTextSize(txt);
          ImVec2 pos = {bb.Min.x + labelPad, y - ts.y - 0.5f * labelPad};
          if (pos.y < bb.Min.y + labelPad)
            pos.y = bb.Min.y + labelPad;
          if (pos.y + ts.y > bb.Max.y - labelPad)
            pos.y = bb.Max.y - labelPad - ts.y;
          dl->AddText(pos, labelColor, txt);
        }
      }
    }

    // unity line (0 dB) over grid for visibility
    float unityY = std::round(DBToY(0, bb)); // round for crisp line.
    dl->AddLine({bb.Min.x, unityY}, {bb.Max.x, unityY}, ColorFromHTML("444444"),
                1.0f);

    // Render FFT spectrum overlays using unified screen-space sampling
    for (size_t overlayIndex = 0; overlayIndex < cfg.fftOverlays.size(); ++overlayIndex) {
      const auto& overlay = cfg.fftOverlays[overlayIndex];
      if (!overlay.frequencyAnalysis || overlayIndex >= mFFTCache.size()) continue;
      
      const auto& fftCache = mFFTCache[overlayIndex];
      
      // Build screen-space points from cache (same approach as filter response)
      std::vector<ImVec2> fftPoints;
      fftPoints.reserve(gSegmentCount);
      
      // FFT should show full spectrum regardless of filter response visibility
      // (FFT represents actual audio signal, not filter response)
      for (int i = 0; i < gSegmentCount; ++i) {
        const auto& cache = fftCache[i];
        if (!cache.valid) continue;
        
        // Apply independent FFT scaling
        float displayMin = cfg.useIndependentFFTScale ? cfg.fftDisplayMinDB : mDisplayMinDB;
        float displayMax = cfg.useIndependentFFTScale ? cfg.fftDisplayMaxDB : mDisplayMaxDB;
        
        // Skip points outside display range (same logic as filter response)
        if (cache.magnitudeDB < displayMin || cache.magnitudeDB > displayMax) continue;
        
        float y = FFTDBToY(cache.magnitudeDB, bb, cfg);
        fftPoints.push_back({mX[i], y});
      }
      
      if (fftPoints.size() >= 2) {
        // Render filled area under curve first (behind line)
        if (overlay.enableFftFill) {
          const float baseline = bb.Max.y;
          
          for (size_t i = 0; i < fftPoints.size() - 1; ++i) {
            const ImVec2& p1 = fftPoints[i];
            const ImVec2& p2 = fftPoints[i + 1];
            
            // Create quad for each segment
            ImVec2 quad[4] = {
              {p1.x, baseline},  // bottom-left
              {p1.x, p1.y},      // top-left  
              {p2.x, p2.y},      // top-right
              {p2.x, baseline}   // bottom-right
            };
            
            dl->AddConvexPolyFilled(quad, 4, overlay.fftFillColor);
          }
        }
        
        // Render spectrum line (same simple approach as filter response)
        dl->AddPolyline(fftPoints.data(), static_cast<int>(fftPoints.size()), 
                       overlay.fftColor, 0, 1.5f);
      }
    }
    
    // Legacy single FFT support (backward compatibility) - use same unified approach
    if (cfg.frequencyAnalysis && cfg.fftOverlays.empty()) {
      // Calculate legacy FFT data using unified screen-space sampling
      std::vector<ImVec2> legacyPoints;
      legacyPoints.reserve(gSegmentCount);
      
      // FFT should show full spectrum regardless of filter response visibility
      for (int i = 0; i < gSegmentCount; ++i) {
        M7::QuickParam param{0, M7::gFilterFreqConfig};
        param.SetRawValue(float(i) / gSegmentCount);
        float freq = param.GetFrequency();
        
        float magnitudeDB = cfg.frequencyAnalysis->GetMagnitudeAtFrequency(freq, cfg.useRightChannel);
        
        // Apply independent FFT scaling
        float displayMin = cfg.useIndependentFFTScale ? cfg.fftDisplayMinDB : mDisplayMinDB;
        float displayMax = cfg.useIndependentFFTScale ? cfg.fftDisplayMaxDB : mDisplayMaxDB;
        
        if (magnitudeDB >= displayMin && magnitudeDB <= displayMax) {
          float y = FFTDBToY(magnitudeDB, bb, cfg);
          legacyPoints.push_back({mX[i], y});
        }
      }
      
      // Render legacy FFT with same approach
      if (legacyPoints.size() >= 2) {
        if (cfg.enableFftFill) {
          const float baseline = bb.Max.y;
          for (size_t i = 0; i < legacyPoints.size() - 1; ++i) {
            const ImVec2& p1 = legacyPoints[i];
            const ImVec2& p2 = legacyPoints[i + 1];
            ImVec2 quad[4] = {{p1.x, baseline}, {p1.x, p1.y}, {p2.x, p2.y}, {p2.x, baseline}};
            dl->AddConvexPolyFilled(quad, 4, cfg.fftFillColor);
          }
        }
        dl->AddPolyline(legacyPoints.data(), static_cast<int>(legacyPoints.size()), 
                       cfg.fftColor, 0, 1.5f);
      }
    }
    
    // Draw FFT scale indicator and legend
    if (cfg.useIndependentFFTScale && TshowGridLabels && (!cfg.fftOverlays.empty() || cfg.frequencyAnalysis)) {
      // Draw scale indicator in top-right corner
      char scaleText[32];
      snprintf(scaleText, sizeof(scaleText), "FFT: %.0f to %.0fdB", cfg.fftDisplayMinDB, cfg.fftDisplayMaxDB);
      ImVec2 textSize = ImGui::CalcTextSize(scaleText);
      ImVec2 textPos = {bb.Max.x - textSize.x - 4, bb.Min.y + 2};
      dl->AddText(textPos, ColorFromHTML("888888", 0.7f), scaleText);
      
      // Draw legend for multiple overlays
      if (cfg.fftOverlays.size() > 1) {
        float legendY = bb.Min.y + 20;
        for (size_t i = 0; i < cfg.fftOverlays.size(); ++i) {
          const auto& overlay = cfg.fftOverlays[i];
          if (!overlay.frequencyAnalysis || !overlay.label) continue;
          
          float legendX = bb.Max.x - 80;
          // Draw color swatch
          dl->AddRectFilled({legendX, legendY}, {legendX + 12, legendY + 8}, overlay.fftColor);
          // Draw label
          ImVec2 labelPos = {legendX + 16, legendY - 2};
          dl->AddText(labelPos, ColorFromHTML("CCCCCC", 0.8f), overlay.label);
          legendY += 14;
        }
      }
    }

    // Render response as visible segments only
    std::vector<ImVec2> segment;
    segment.reserve(gSegmentCount);

    auto flushSegment = [&]() {
      if (segment.size() >= 2) {
        dl->AddPolyline(segment.data(), (int)segment.size(), cfg.lineColor, 0,
                        2.0f);
      }
      segment.clear();
    };

    for (int i = mVisibleLeftIndex; i <= mVisibleRightIndex; ++i) {
      float dB = mMagdB[i];
      if (dB < mDisplayMinDB || dB > mDisplayMaxDB) {
        flushSegment();
        continue;
      }
      float y = DBToY(dB, bb);
      segment.push_back({mX[i], y});
    }
    flushSegment();

    // 5) Render thumbs at band center frequencies if within range
    mThumbs.clear();
    for (auto &f : cfg.filters) {
      if (!f.filter)
        continue;
      float freq = f.filter->freq;
      float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
      float magdB = M7::math::LinearToDecibels(magLin);
      if (magdB < mDisplayMinDB || magdB > mDisplayMaxDB)
        continue;

      mThumbs.push_back({f.thumbColor, {FreqToX(freq, bb), DBToY(magdB, bb)}});
    }
    for (auto &th : mThumbs) {
        dl->AddCircleFilled(th.point, cfg.thumbRadius, ColorFromHTML(th.color, 0.8f));
        dl->AddCircle(th.point, cfg.thumbRadius + 1, ColorFromHTML("000000"), 0, 2);
        dl->AddCircle(th.point, cfg.thumbRadius + 2, ColorFromHTML(th.color), 0, 1);
    }

    ImGui::Dummy(gSize);
  }
  
  // Convert screen X coordinate back to frequency (inverse of FreqToX)
  float XToFreq(float x, ImRect &bb) {
    float t01 = M7::math::lerp_rev(bb.Min.x, bb.Max.x, x);
    t01 = M7::math::clamp01(t01);
    
    // Use the same frequency mapping as FreqToX but in reverse
    float underlyingValue = t01;
    M7::ParamAccessor p{&underlyingValue, 0};
    return p.GetFrequency(M7::gFilterFreqConfig);
  }
};
} // namespace WaveSabreVstLib
