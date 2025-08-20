#pragma once

#include "FrequencyMagnitudeGraph.hpp"

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
  
  // Convert dB to Y using FFT's independent scale if enabled
  float FFTDBToY(float dB, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
    if (mUseIndependentScale) {
      float t01 = M7::math::lerp_rev(mFFTDisplayMinDB, mFFTDisplayMaxDB, dB);
      //t01 = M7::math::clamp01(t01);
      return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
    } else {
      return coords.DBToY(dB, bb);
    }
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
      
      // Build screen-space points from cache
      std::vector<ImVec2> fftPoints;
      fftPoints.reserve(TSegmentCount);
      
      for (int i = 0; i < TSegmentCount; ++i) {
        const auto& cache = fftCache[i];
        if (!cache.valid) continue;
        
        // Apply scaling (independent or shared)
        float displayMin = mUseIndependentScale ? mFFTDisplayMinDB : coords.mDisplayMinDB;
        float displayMax = mUseIndependentScale ? mFFTDisplayMaxDB : coords.mDisplayMaxDB;
        
        // Skip points outside display range
        if (cache.magnitudeDB < displayMin || cache.magnitudeDB > displayMax) continue;
        
        float y = FFTDBToY(cache.magnitudeDB, coords, bb);
        fftPoints.push_back({mScreenX[i], y});
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
            //dl->AddConcavePolyFilled(quad, 4, overlay.fftFillColor);
          }
        }
        
        // Render spectrum line
        dl->AddPolyline(fftPoints.data(), static_cast<int>(fftPoints.size()), 
                       overlay.fftColor, 0, 1.5f);
      }
    }
    
    // Draw scale indicator and legend
    if (mUseIndependentScale && !mOverlays.empty()) {
      // Draw scale indicator in top-right corner
      char scaleText[64];
      snprintf(scaleText, sizeof(scaleText), "%s: %.1f to %.1f", mScaleCaption.c_str(), mFFTDisplayMinDB, mFFTDisplayMaxDB);
      ImVec2 textSize = ImGui::CalcTextSize(scaleText);
      ImVec2 textPos = {bb.Max.x - textSize.x - 4, bb.Min.y + 2};
      dl->AddText(textPos, ColorFromHTML("888888", 0.7f), scaleText);
      
      // Draw legend for multiple overlays
      if (mOverlays.size() > 1) {
        float legendY = bb.Min.y + 20;
        for (size_t i = 0; i < mOverlays.size(); ++i) {
          const auto& overlay = mOverlays[i];
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
  }
};

} // namespace WaveSabreVstLib