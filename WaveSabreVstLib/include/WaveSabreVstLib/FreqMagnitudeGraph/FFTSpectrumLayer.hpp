#pragma once

#include "FrequencyMagnitudeGraph.hpp"
#include <functional>

namespace WaveSabreVstLib {

//=============================================================================
// FFT spectrum layer with independent scaling support
//=============================================================================
template <int TSegmentCount>
class FFTSpectrumLayer : public IFrequencyGraphLayer {
private:
  // Screen-space FFT response cache
  struct FFTSegmentCache {
    float magnitudeDB;
    bool valid;
    FFTSegmentCache() : magnitudeDB(-100.0f), valid(false) {}
  };
  
  std::vector<FFTAnalysisOverlay> mOverlays;
  std::vector<std::array<FFTSegmentCache, TSegmentCount>> mFFTCache;
  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  
  // Independent FFT Y-axis scaling
  float mFFTDisplayMinDB = -90.0f;
  float mFFTDisplayMaxDB = 0.0f;
  bool mUseIndependentScale = true;
  std::string mScaleCaption = "FFT"; // customizable caption for scale indicator

  // Optional value transform (identity by default). Applied to data and to scale endpoints when mapping to Y
  std::function<float(float)> mValueTransform;
  
  // Convert sample value to Y using scale (with optional independent scaling)
  float FFTValueToY(float value, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
    float v = mValueTransform ? mValueTransform(value) : value;
    if (mUseIndependentScale) {
      float minT = mValueTransform ? mValueTransform(mFFTDisplayMinDB) : mFFTDisplayMinDB;
      float maxT = mValueTransform ? mValueTransform(mFFTDisplayMaxDB) : mFFTDisplayMaxDB;
      float t01 = (maxT - minT) != 0.0f ? M7::math::clamp01((v - minT) / (maxT - minT)) : 0.0f;
      return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
    } else {
      return coords.DBToY(v, bb);
    }
  }
  
  float MapToY(float value, float minV, float maxV, const ImRect& bb) const {
    // will NOT clamp the output/input to the range.
    //const float t01 = (maxV - minV) != 0.0f ? M7::math::clamp01((value - minV) / (maxV - minV)) : 0.0f;
    const float t01 = M7::math::lerp_rev(minV, maxV, value);
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
  }

public:
  FFTSpectrumLayer(const std::vector<FFTAnalysisOverlay>& overlays, 
                   float minDB = -90.0f, float maxDB = 0.0f, bool independentScale = true)
    : mOverlays(overlays), mFFTDisplayMinDB(minDB), mFFTDisplayMaxDB(maxDB), mUseIndependentScale(independentScale) {
    mFFTCache.resize(mOverlays.size());
    mFrequencies.reserve(TSegmentCount);
    mScreenX.reserve(TSegmentCount);
  }
  
  void AddOverlay(const FFTAnalysisOverlay& overlay) {
    mOverlays.push_back(overlay);
    mFFTCache.resize(mOverlays.size());
  }
  
  void SetOverlays(const std::vector<FFTAnalysisOverlay>& overlays) {
    mOverlays = overlays;
    mFFTCache.resize(mOverlays.size());
  }
  
  void SetDisplayRange(float minDB, float maxDB) {
    mFFTDisplayMinDB = minDB;
    mFFTDisplayMaxDB = maxDB;
  }
  
  void SetUseIndependentScale(bool useIndependent) {
    mUseIndependentScale = useIndependent;
  }
  
  void SetScaling(float minDB, float maxDB, bool independent = true) {
    mFFTDisplayMinDB = minDB;
    mFFTDisplayMaxDB = maxDB;
    mUseIndependentScale = independent;
  }

  void SetScaleCaption(const char* caption) {
    mScaleCaption = caption ? caption : "";
  }

  void SetValueTransform(std::function<float(float)> transform) {
    mValueTransform = std::move(transform);
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Generate screen-space frequency sampling points
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);
    
    // Always recalculate FFT data every frame (dynamic data)
    for (size_t overlayIndex = 0; overlayIndex < mOverlays.size(); ++overlayIndex) {
      const auto& overlay = mOverlays[overlayIndex];
      auto& overlayCache = mFFTCache[overlayIndex];
      
      for (int iseg = 0; iseg < TSegmentCount; ++iseg) {
        auto& cache = overlayCache[iseg];
        
        if (overlay.frequencyAnalysis) {
          float freq = mFrequencies[iseg];

          // Clamp to strictly below Nyquist to avoid pinned/unchanging last-bin artifacts
          const float nyquist = overlay.frequencyAnalysis->GetNyquistFrequency();
          const float binRes = overlay.frequencyAnalysis->GetFrequencyResolution();
          if (nyquist > 0.0f && binRes > 0.0f && freq >= nyquist) {
            freq = std::max(0.0f, nyquist - 0.5f * binRes);
          }

          cache.magnitudeDB = overlay.frequencyAnalysis->GetMagnitudeAtFrequency(freq);
          cache.valid = true;
        } else {
          cache.magnitudeDB = -100.0f; // Silence
          cache.valid = false;
        }
      }
    }
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    // Render each FFT overlay
    for (size_t overlayIndex = 0; overlayIndex < mOverlays.size(); ++overlayIndex) {
      const auto& overlay = mOverlays[overlayIndex];
      if (overlayIndex >= mFFTCache.size()) continue;
      
      const auto& fftCache = mFFTCache[overlayIndex];
      std::vector<ImVec2> pts; pts.reserve(TSegmentCount);
      std::vector<ImVec2> fillPoly;

      // Determine transform for this overlay
      auto transform = overlay.valueTransform ? overlay.valueTransform : mValueTransform;

      // Determine display min/max
      float dispMin = mUseIndependentScale ? mFFTDisplayMinDB : coords.mDisplayMinDB;
      float dispMax = mUseIndependentScale ? mFFTDisplayMaxDB : coords.mDisplayMaxDB;
      float minT = transform ? transform(dispMin) : dispMin;
      float maxT = transform ? transform(dispMax) : dispMax;
      if (maxT < minT) std::swap(minT, maxT);

      for (int i = 0; i < TSegmentCount; ++i) {
        const auto& cache = fftCache[i];
        if (!cache.valid) continue;
        float v = transform ? transform(cache.magnitudeDB) : cache.magnitudeDB;
        //if (v < minT || v > maxT) continue; // creating gaps causes visual glitches. it's not time yet to cull oob.
        float y = MapToY(v, minT, maxT, bb);
        pts.push_back({ mScreenX[i], y });
      }

      if (pts.size() >= 2) {
        dl->PushClipRect(bb.Min, bb.Max, true);

        if (overlay.enableFftFill) {
          const float baseline = bb.Max.y;

          fillPoly.reserve(pts.size() + 4);
          fillPoly.push_back({pts.front().x, baseline});

          const bool firstAboveBaseline = pts.front().y <= baseline;
          if (firstAboveBaseline) {
            fillPoly.push_back(pts.front());
          }

          for (size_t i = 0; i + 1 < pts.size(); ++i) {
            const ImVec2& p0 = pts[i];
            const ImVec2& p1 = pts[i + 1];
            const bool p0AboveBaseline = p0.y <= baseline;
            const bool p1AboveBaseline = p1.y <= baseline;

            if (p0AboveBaseline && p1AboveBaseline) {
              fillPoly.push_back(p1);
              continue;
            }

            if (p0AboveBaseline != p1AboveBaseline) {
              const float dy = p1.y - p0.y;
              const float t = (dy != 0.0f) ? ((baseline - p0.y) / dy) : 0.0f;
              const float xCross = M7::math::lerp(p0.x, p1.x, t);
              fillPoly.push_back({xCross, baseline});

              if (p1AboveBaseline) {
                fillPoly.push_back(p1);
              }
            }
          }

          fillPoly.push_back({pts.back().x, baseline});

          if (fillPoly.size() >= 3) {
            dl->AddConcavePolyFilled(fillPoly.data(), (int)fillPoly.size(), overlay.fftFillColor);
          }
        }

        dl->AddPolyline(pts.data(), (int)pts.size(), overlay.fftColor, 0, 1.5f);
        dl->PopClipRect();
      }
    }
    
    // Draw scale indicator and legend
    if (mUseIndependentScale && !mOverlays.empty()) {
      // Draw scale indicator in top-right corner
      char scaleText[64];
      snprintf(scaleText, sizeof(scaleText), "%s: %.1f to %.1f", mScaleCaption.c_str(), mFFTDisplayMinDB, mFFTDisplayMaxDB);
      ImVec2 textSize = ImGui::CalcTextSize(scaleText);
      ImVec2 textPos = { bb.Max.x - textSize.x - 4, bb.Min.y + 2 };
      dl->AddText(textPos, ColorFromHTML("888888", 0.7f), scaleText);
      
      // Draw legend for multiple overlays
      if (mOverlays.size() > 1) {
        float legendY = bb.Min.y + 20;
        for (size_t i = 0; i < mOverlays.size(); ++i) {
          const auto& ov = mOverlays[i]; if (!ov.frequencyAnalysis || !ov.label) continue;
          float legendX = bb.Max.x - 80;
          dl->AddRectFilled({ legendX, legendY }, { legendX + 12, legendY + 8 }, ov.fftColor);
          ImVec2 labelPos = { legendX + 16, legendY - 2 };
          dl->AddText(labelPos, ColorFromHTML("CCCCCC", 0.8f), ov.label);
          legendY += 14;
        }
      }
    }
  }
};

} // namespace WaveSabreVstLib