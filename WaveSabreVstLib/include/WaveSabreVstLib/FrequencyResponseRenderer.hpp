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

//  FrequencyResponseRenderer - Interactive EQ GUI Component
//  
//  NEW FEATURE: Thumb Drag Interaction
//  ===================================
//  
//  Thumbs (the circular markers on filter center frequencies) now support click & drag interaction
//  when a HandleChangeParam callback is provided in FrequencyResponseRendererFilter.
//  
//  Usage Example:
//  --------------
//  
//  // Define a callback to handle parameter changes
//  void MyFilterParamHandler(float freqHz, float gainDb, void* userData) {
//      MyFilterData* filterData = static_cast<MyFilterData*>(userData);
//      
//      // Update your filter parameters
//      filterData->frequency = freqHz;
//      filterData->gain = gainDb;
//      
//      // Update VST parameters, DSP, etc.
//      UpdateMyFilter(filterData);
//  }
//  
//  // Create interactive filters
//  FrequencyResponseRendererFilter filters[] = {
//      {"ff0000", &myFilter1, "LP",  MyFilterParamHandler, &myFilterData1}, // Interactive
//      {"00ff00", &myFilter2, "HP",  MyFilterParamHandler, &myFilterData2}, // Interactive  
//      {"0000ff", &myFilter3, "BP",  nullptr,              nullptr},        // Visual only
//  };
//  
//  Visual Feedback:
//  ----------------
//  - Interactive thumbs show hover glow and resize slightly when hovered
//  - During drag, thumbs get bright white outline and scale up 10%
//  - Tooltips show "Click & drag to adjust" for interactive thumbs
//  - Non-interactive thumbs (HandleChangeParam == nullptr) remain visual-only
//
///////////////////////////////////////////////////////////////////////////////////////////////////
struct FrequencyResponseRendererFilter {
  const char *thumbColor;
  const BiquadFilter *filter;
  const char* label = nullptr;
  std::function<void(float freqHz, float gainDb, uintptr_t userData)> HandleChangeParam;
  uintptr_t userData = 0; // Optional user data for custom handling
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
    bool enableFftFill = true;                              // enable filled area under curve
    const char* label = nullptr;                            // optional label for legend/display
  };
  
  std::vector<FFTAnalysisOverlay> fftOverlays{};            // Multiple FFT overlays
  
  // Independent FFT Y-axis scaling (shared by all overlays)
  float fftDisplayMinDB = -60.0f;  // Noise floor (iZotope style: -60dB to 0dB)
  float fftDisplayMaxDB = 0.0f;    // Digital maximum (0dB)
  bool useIndependentFFTScale = true;  // Use separate scale for FFT vs EQ response
};

template <int Twidth, int Theight, int TsegmentCount, size_t TFilterCount,
          size_t TParamCount, bool TshowGridLabels>
struct FrequencyResponseRenderer {
  
  // Extended thumb rendering info with interaction data
  struct ThumbRenderInfo {
    const char* color;
    ImVec2 point;
    const char* label;
    int filterIndex = -1;           // Index into the filters array
    bool isInteractive = false;     // Whether this thumb supports dragging
    ImRect hitTestRect;             // Screen-space hit test area (slightly larger than visual)
  };

  // Thumb interaction state management
  struct ThumbInteractionState {
    int activeThumbIndex = -1;      // Which thumb is being dragged (-1 = none)
    ImVec2 dragStartMousePos;       // Mouse position when drag started
    ImVec2 dragStartThumbPos;       // Thumb position when drag started
    float originalFreq = 0.0f;      // Original frequency when drag started
    float originalGain = 0.0f;      // Original gain when drag started (derived from response)
    bool isDragging = false;        // Currently in drag operation
    bool wasHovered = false;        // Was hovered last frame (for hover state tracking)
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

  // NEW: Thumb interaction state
  ThumbInteractionState mThumbInteraction;

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

  // Helper function: Convert screen Y coordinate back to dB (inverse of DBToY)
  float YToDB(float y, ImRect &bb) {
    float t01 = M7::math::lerp_rev(bb.Max.y, bb.Min.y, y); // Note: inverted Y axis
    t01 = M7::math::clamp01(t01);
    return M7::math::lerp(mDisplayMinDB, mDisplayMaxDB, t01);
  }

  // Helper function: Check if a point is inside a circle
  bool IsPointInCircle(ImVec2 point, ImVec2 center, float radius) {
    float dx = point.x - center.x;
    float dy = point.y - center.y;
    return (dx * dx + dy * dy) <= (radius * radius);
  }

  // Helper function: Find which thumb is under the mouse cursor
  int FindThumbUnderMouse(ImVec2 mousePos, float thumbRadius) {
    for (int i = 0; i < (int)mThumbs.size(); ++i) {
      const auto& thumb = mThumbs[i];
      if (thumb.isInteractive && IsPointInCircle(mousePos, thumb.point, thumbRadius + 3.0f)) { // Slightly larger hit area
        return i;
      }
    }
    return -1;
  }

  // Helper function: Handle thumb drag interaction
  void HandleThumbInteraction(const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg, ImRect &bb) {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseClicked = ImGui::IsMouseClicked(0);
    bool mouseDown = ImGui::IsMouseDown(0);
    bool mouseReleased = ImGui::IsMouseReleased(0);
    bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

    // Handle drag end
    if (mThumbInteraction.isDragging && (mouseReleased || !mouseDown)) {
      mThumbInteraction.isDragging = false;
      mThumbInteraction.activeThumbIndex = -1;
      return;
    }

    // Handle drag start
    if (mouseClicked && mouseInBounds && !mThumbInteraction.isDragging) {
      int thumbIndex = FindThumbUnderMouse(mousePos, cfg.thumbRadius);
      if (thumbIndex >= 0) {
        const auto& thumb = mThumbs[thumbIndex];
        mThumbInteraction.activeThumbIndex = thumbIndex;
        mThumbInteraction.isDragging = true;
        mThumbInteraction.dragStartMousePos = mousePos;
        mThumbInteraction.dragStartThumbPos = thumb.point;
        mThumbInteraction.originalFreq = XToFreq(thumb.point.x, bb);
        mThumbInteraction.originalGain = YToDB(thumb.point.y, bb);
      }
    }

    // Handle active drag
    if (mThumbInteraction.isDragging && mThumbInteraction.activeThumbIndex >= 0) {
      auto& activeThumb = mThumbs[mThumbInteraction.activeThumbIndex];
      const auto& filter = cfg.filters[activeThumb.filterIndex];
      
      if (filter.HandleChangeParam) {
        // Calculate new frequency and gain from mouse position
        ImVec2 clampedMouse = {
          M7::math::clamp(mousePos.x, bb.Min.x, bb.Max.x),
          M7::math::clamp(mousePos.y, bb.Min.y, bb.Max.y)
        };
        
        float newFreq = XToFreq(clampedMouse.x, bb);
        float newGain = YToDB(clampedMouse.y, bb);
        
        // Apply reasonable limits
        newFreq = M7::math::clamp(newFreq, 20.0f, 20000.0f);
        newGain = M7::math::clamp(newGain, mDisplayMinDB, mDisplayMaxDB);
        
        // Call the parameter change handler
        filter.HandleChangeParam(newFreq, newGain, filter.userData);
        
        // Force recalculation since parameters changed
        mAdditionalForceCalcFrames = 2;
      }
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
          cache.magnitudeDB = overlay.frequencyAnalysis->GetMagnitudeAtFrequency(freq);
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
    ImColor gridMinor = ColorFromHTML("333333", 0.4f);
    ImColor gridMajor = ColorFromHTML("555555", 0.7f);
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
    dl->AddLine({bb.Min.x, unityY}, {bb.Max.x, unityY}, ColorFromHTML("cccccc", 0.4f));

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
    
    // Draw FFT scale indicator and legend
    if (cfg.useIndependentFFTScale && TshowGridLabels && (!cfg.fftOverlays.empty())) {
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

    // Render thumbs at band center frequencies if within range
    mThumbs.clear();
    for (size_t filterIdx = 0; filterIdx < cfg.filters.size(); ++filterIdx) {
      const auto &f = cfg.filters[filterIdx];
      if (!f.filter)
        continue;
      float freq = f.filter->freq;
      float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
      float magdB = M7::math::LinearToDecibels(magLin);
      if (magdB < mDisplayMinDB || magdB > mDisplayMaxDB)
        continue;

      ThumbRenderInfo thumbInfo;
      thumbInfo.color = f.thumbColor;
      thumbInfo.point = {FreqToX(freq, bb), DBToY(magdB, bb)};
      thumbInfo.label = f.label;
      thumbInfo.filterIndex = (int)filterIdx;
      thumbInfo.isInteractive = (f.HandleChangeParam != nullptr);
      
      // Create hit test area (slightly larger than visual thumb)
      float hitRadius = cfg.thumbRadius + 3.0f;
      thumbInfo.hitTestRect = ImRect(
        thumbInfo.point.x - hitRadius, thumbInfo.point.y - hitRadius,
        thumbInfo.point.x + hitRadius, thumbInfo.point.y + hitRadius
      );
      
      mThumbs.push_back(thumbInfo);
    }

    // Handle thumb interaction before rendering
    HandleThumbInteraction(cfg, bb);

    // Render thumbs with interaction feedback
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    for (int i = 0; i < (int)mThumbs.size(); ++i) {
      const auto &th = mThumbs[i];
      
      // Determine thumb visual state
      bool isActive = (mThumbInteraction.isDragging && mThumbInteraction.activeThumbIndex == i);
      bool isHovered = th.isInteractive && IsPointInCircle(mousePos, th.point, cfg.thumbRadius + 3.0f) && 
                      ImGui::IsMouseHoveringRect(bb.Min, bb.Max) && !mThumbInteraction.isDragging;
      
      // Draw thumb with state-based appearance
      ImU32 thumbColor = ColorFromHTML(th.color, isActive ? 1.0f : 0.8f);
      float thumbRadius = cfg.thumbRadius * (isActive ? 1.1f : (isHovered ? 1.05f : 1.0f));
      
      dl->AddCircleFilled(th.point, thumbRadius, thumbColor);
      dl->AddCircle(th.point, thumbRadius + 1, ColorFromHTML("000000"), 0, 1.5f);
      
      // Additional visual feedback for interactive thumbs
      if (th.isInteractive) {
        if (isHovered && !isActive) {
          // Subtle glow for hover
          dl->AddCircle(th.point, thumbRadius + 2, ColorFromHTML(th.color, 0.3f), 0, 1.0f);
        } else if (isActive) {
          // Bright outline for active drag
          dl->AddCircle(th.point, thumbRadius + 2, ColorFromHTML("ffffff", 0.8f), 0, 2.0f);
        }
      }
      
      // Draw label if available
      if (th.label) {
        ImVec2 textSize = ImGui::CalcTextSize(th.label);
        ImVec2 textPos = { th.point.x - textSize.x * 0.5f, th.point.y - textSize.y * 0.5f };
        dl->AddText(textPos, ColorFromHTML("000000"), th.label);
      }
    }

    // Show hover tooltip only when not dragging and not hovering a thumb
    if (ImGui::IsMouseHoveringRect(bb.Min, bb.Max) && !mThumbInteraction.isDragging) {
      int hoveredThumb = FindThumbUnderMouse(mousePos, cfg.thumbRadius);
      
      if (hoveredThumb >= 0) {
        // Show thumb-specific tooltip
        const auto& thumb = mThumbs[hoveredThumb];
        const auto& filter = cfg.filters[thumb.filterIndex];
        
        float freq = XToFreq(thumb.point.x, bb);
        float gain = YToDB(thumb.point.y, bb);
        
        ImGui::BeginTooltip();
        if (thumb.label) {
          ImGui::Text("%s", thumb.label);
        }
        ImGui::Text("%.0f Hz", freq);
        ImGui::Text("%.1f dB", gain);
        if (thumb.isInteractive) {
          ImGui::TextDisabled("Click & drag to adjust");
        }
        ImGui::EndTooltip();
      } else {
        // Show existing frequency response tooltip
        ImVec2 clampedMouse = { M7::math::clamp(mousePos.x, bb.Min.x, bb.Max.x), M7::math::clamp(mousePos.y, bb.Min.y, bb.Max.y) };

        // Crosshair: vertical and horizontal lines
        ImU32 crossColH = ColorFromHTML("ff8800", 0.25f);
        ImU32 crossColV = ColorFromHTML("00dddd", 0.25f);
        dl->AddLine({clampedMouse.x, bb.Min.y}, {clampedMouse.x, bb.Max.y}, crossColV, 1.0f);
        dl->AddLine({bb.Min.x, clampedMouse.y}, {bb.Max.x, clampedMouse.y}, crossColH, 1.0f);

        // Find corresponding response curve point
        const float mx = clampedMouse.x;
        int i1;
        {
          auto begin = mX + mVisibleLeftIndex;
          auto end   = mX + mVisibleRightIndex + 1;
          auto it = std::lower_bound(begin, end, mx);
          i1 = (int)(it - mX);
          if (i1 <= mVisibleLeftIndex) i1 = mVisibleLeftIndex + 1;
          if (i1 >  mVisibleRightIndex) i1 = mVisibleRightIndex;
        }
        int i0 = i1 - 1;
        float x0 = mX[i0];
        float x1 = mX[i1];
        float t = (x1 > x0) ? M7::math::clamp01((mx - x0) / (x1 - x0)) : 0.0f;
        float magDBLerp = M7::math::lerp(mMagdB[i0], mMagdB[i1], t);
        float curveY = DBToY(magDBLerp, bb);
        ImVec2 curvePt = { mx, curveY };

        // Indicator on the response curve
        ImU32 indicatorFill = ColorFromHTML("00dddd", 0.95f);
        ImU32 indicatorOutline = ColorFromHTML("000000", 0.9f);
        ImU32 ptCrossCol = ColorFromHTML("00dddd", 0.25f);
        
        dl->AddLine({ bb.Min.x, curvePt.y }, { bb.Max.x, curvePt.y }, ptCrossCol, 1.0f);
        dl->AddCircleFilled(curvePt, 4.0f, indicatorFill);
        dl->AddCircle(curvePt, 4.5f, indicatorOutline, 0, 1.5f);

        float hoverFreq = XToFreq(mx, bb);

        // Build tooltip text
        char freqText[32];
        if (hoverFreq >= 1000.0f) {
          snprintf(freqText, sizeof(freqText), "%.2fkHz", hoverFreq / 1000.0f);
        } else {
          snprintf(freqText, sizeof(freqText), "%.0fHz", hoverFreq);
        }

        ImVec2 tipOffset = { 8.0f, -8.0f };
        ImVec2 tipAnchor = { curvePt.x + tipOffset.x, curvePt.y + tipOffset.y };
        ImGui::SetNextWindowPos(tipAnchor, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::BeginTooltip();
        ImGui::Text("%s", freqText);
        ImGui::Text("%.2f dB", magDBLerp);
        ImGui::EndTooltip();
      }
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
    return p.GetFrequency(0, M7::gFilterFreqConfig);
  }
};
} // namespace WaveSabreVstLib
