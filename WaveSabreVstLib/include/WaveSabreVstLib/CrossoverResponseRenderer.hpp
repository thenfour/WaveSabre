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
#include "FrequencyMagnitudeGraphUtils.hpp"

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib {

//=============================================================================
// Crossover response layer - renders Linkwitz-Riley crossover filters
//=============================================================================
template <int TSegmentCount>
class CrossoverResponseLayer : public IFrequencyGraphLayer {
private:
  struct CrossoverBand {
    float frequencyHz;
    ImColor color;
    const char* label;
    std::function<void(float newFreqHz)> onFrequencyChanged;
    bool isActive = true;
  };
  
  std::vector<CrossoverBand> mBands;
  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::vector<std::vector<float>> mBandResponses; // Per-band magnitude responses
  
  // Calculate Linkwitz-Riley 4th order response
  float CalculateLinkwitzRileyResponse(float freq, float crossoverFreq, bool isHighpass) const {
    if (crossoverFreq <= 0) return isHighpass ? 1.0f : 0.0f;
    
    float ratio = freq / crossoverFreq;
    float ratio2 = ratio * ratio;
    float ratio4 = ratio2 * ratio2;
    
    if (isHighpass) {
      // Linkwitz-Riley 4th order highpass: H(s) = s^4 / (s^4 + 2*sqrt(2)*s^3*wc + 2*s^2*wc^2 + 2*sqrt(2)*s*wc^3 + wc^4)
      return ratio4 / std::sqrt((1.0f - 2.0f*ratio2 + ratio4) * (1.0f - 2.0f*ratio2 + ratio4) + 
                               (2.0f*1.414213562f*ratio - 2.0f*1.414213562f*ratio*ratio2) * 
                               (2.0f*1.414213562f*ratio - 2.0f*1.414213562f*ratio*ratio2));
    } else {
      // Linkwitz-Riley 4th order lowpass
      return 1.0f / std::sqrt((1.0f - 2.0f*ratio2 + ratio4) * (1.0f - 2.0f*ratio2 + ratio4) + 
                             (2.0f*1.414213562f*ratio - 2.0f*1.414213562f*ratio*ratio2) * 
                             (2.0f*1.414213562f*ratio - 2.0f*1.414213562f*ratio*ratio2));
    }
  }
  
public:
  CrossoverResponseLayer() {
    mBandResponses.resize(4); // Support up to 4 bands initially
  }
  
  void SetCrossoverFrequencies(const std::vector<float>& frequencies, 
                              const std::vector<ImColor>& colors,
                              const std::vector<const char*>& labels) {
    mBands.clear();
    
    for (size_t i = 0; i < frequencies.size() && i < colors.size(); ++i) {
      CrossoverBand band;
      band.frequencyHz = frequencies[i];
      band.color = colors[i];
      band.label = (i < labels.size()) ? labels[i] : nullptr;
      mBands.push_back(band);
    }
  }
  
  bool InfluencesAutoScale() const override { return true; }
  
  void GetDataBounds(float& minDB, float& maxDB) const override {
    minDB = -40.0f; // Crossovers typically don't go below -40dB
    maxDB = 6.0f;   // Allow some headroom for summing
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Generate screen-space frequency sampling
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);
    
    // Ensure we have enough band response arrays
    if (mBandResponses.size() < mBands.size() + 1) {
      mBandResponses.resize(mBands.size() + 1);
    }
    
    // Calculate crossover responses for each band
    for (size_t bandIdx = 0; bandIdx <= mBands.size(); ++bandIdx) {
      auto& bandResponse = mBandResponses[bandIdx];
      bandResponse.resize(TSegmentCount);
      
      for (int i = 0; i < TSegmentCount; ++i) {
        float freq = mFrequencies[i];
        float magnitude = 1.0f; // Start with unity gain
        
        // Apply crossover filtering for this band
        if (bandIdx == 0) {
          // First band (lowest) - apply lowpass filters from all crossover points above
          for (const auto& xover : mBands) {
            if (xover.isActive) {
              magnitude *= CalculateLinkwitzRileyResponse(freq, xover.frequencyHz, false);
            }
          }
        } else if (bandIdx <= mBands.size()) {
          // Middle/high bands
          for (size_t xoverIdx = 0; xoverIdx < mBands.size(); ++xoverIdx) {
            const auto& xover = mBands[xoverIdx];
            if (!xover.isActive) continue;
            
            if (xoverIdx < bandIdx) {
              // Apply highpass for crossovers below this band
              magnitude *= CalculateLinkwitzRileyResponse(freq, xover.frequencyHz, true);
            } else if (xoverIdx >= bandIdx) {
              // Apply lowpass for crossovers at/above this band
              magnitude *= CalculateLinkwitzRileyResponse(freq, xover.frequencyHz, false);
            }
          }
        }
        
        bandResponse[i] = M7::math::LinearToDecibels(magnitude);
      }
    }
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    // Render each crossover band response
    for (size_t bandIdx = 0; bandIdx < mBandResponses.size() && bandIdx <= mBands.size(); ++bandIdx) {
      const auto& bandResponse = mBandResponses[bandIdx];
      if (bandResponse.empty()) continue;
      
      // Determine band color
      ImColor bandColor = (bandIdx < mBands.size()) ? mBands[bandIdx].color : ColorFromHTML("888888", 0.7f);
      
      // Build polyline points
      std::vector<ImVec2> points;
      points.reserve(TSegmentCount);
      
      for (int i = 0; i < TSegmentCount; ++i) {
        float dB = bandResponse[i];
        if (dB < coords.mDisplayMinDB || dB > coords.mDisplayMaxDB) continue;
        
        float y = coords.DBToY(dB, bb);
        points.push_back({mScreenX[i], y});
      }
      
      if (points.size() >= 2) {
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), bandColor, 0, 2.0f);
      }
    }
    
    // Draw crossover frequency lines
    for (const auto& band : mBands) {
      if (!band.isActive) continue;
      
      float x = coords.FreqToX(band.frequencyHz, bb);
      if (x >= bb.Min.x && x <= bb.Max.x) {
        dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, 
                   ImColor(band.color.Value.x, band.color.Value.y, band.color.Value.z, 0.5f), 2.0f);
        
        // Add frequency label
        if (band.label) {
          ImVec2 textSize = ImGui::CalcTextSize(bband.label);
          ImVec2 textPos = {x - textSize.x * 0.5f, bb.Min.y + 5};
          dl->AddText(textPos, band.color, band.label);
        }
      }
    }
  }
};

//=============================================================================
// Complete crossover visualization using the layered architecture
//=============================================================================
template <int TWidth = 400, int THeight = 200>
class CrossoverVisualization {
private:
  FrequencyMagnitudeGraph<TWidth, THeight, true> mGraph;
  std::unique_ptr<CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount>> mCrossoverLayer;
  
public:
  CrossoverVisualization() {
    // Create layers
    auto gridLayer = std::make_unique<GridLayer<true>>();
    mCrossoverLayer = std::make_unique<CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount>>();
    
    // Add layers to graph
    mGraph.AddLayer(std::move(gridLayer));
    mGraph.AddLayer(std::unique_ptr<IFrequencyGraphLayer>(mCrossoverLayer.release()));
  }
  
  void SetCrossoverFrequencies(const std::vector<float>& frequencies) {
    if (mCrossoverLayer) {
      std::vector<ImColor> colors = {
        ColorFromHTML("ff4444", 0.8f), // Red for low band
        ColorFromHTML("44ff44", 0.8f), // Green for mid band  
        ColorFromHTML("4444ff", 0.8f), // Blue for high band
        ColorFromHTML("ffff44", 0.8f)  // Yellow for ultra-high band
      };
      
      std::vector<const char*> labels = {"Low", "Mid", "High", "Ultra"};
      
      mCrossoverLayer->SetCrossoverFrequencies(frequencies, colors, labels);
    }
  }
  
  void Render() {
    mGraph.Render();
  }
};

} // namespace WaveSabreVstLib
