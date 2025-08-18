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
  struct CrossoverBandLegacy {
    float frequencyHz;
    ImColor color;
    const char* label;
    std::function<void(float newFreqHz)> onFrequencyChanged;
    bool isActive = true;
  };
  
  // Legacy param-driven bands (for backward compatibility)
  std::vector<CrossoverBandLegacy> mBands;

  // New: direct magnitude providers per band
  std::vector<std::function<float(float /*freqHz*/)>> mBandMagnitudeFns;
  std::vector<ImColor> mBandColors;
  std::vector<const char*> mBandLabels;

  // Optional: provider to retrieve crossover line frequencies (for drawing vertical markers)
  std::function<void(std::vector<float>&)> mGetCrossoverFrequencies;

  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::vector<std::vector<float>> mBandResponses; // Per-band magnitude responses
  
public:
  CrossoverResponseLayer() {
    mBandResponses.resize(4); // Support up to 4 bands initially
  }
  
  // New API: operate directly with underlying filters by supplying per-band magnitude functions.
  // - bandMagnitudeFns: one function per band returning linear magnitude for a given frequency in Hz.
  // - colors/labels: optional visual configuration per band (size should match bandMagnitudeFns or be empty).
  // - getCrossoverFrequencies: optional callback to provide current crossover frequencies for vertical lines.
  void SetBands(const std::vector<std::function<float(float)>>& bandMagnitudeFns,
                const std::vector<ImColor>& colors = {},
                const std::vector<const char*>& labels = {},
                std::function<void(std::vector<float>&)> getCrossoverFrequencies = nullptr)
  {
    mBandMagnitudeFns = bandMagnitudeFns;
    mBandColors = colors;
    mBandLabels = labels;
    mGetCrossoverFrequencies = std::move(getCrossoverFrequencies);

    // Ensure band responses storage large enough
    if (mBandResponses.size() < mBandMagnitudeFns.size()) {
      mBandResponses.resize(mBandMagnitudeFns.size());
    }
  }

  // Legacy API: frequency-param driven LR crossover
  void SetCrossoverFrequencies(const std::vector<float>& frequencies, 
                              const std::vector<ImColor>& colors,
                              const std::vector<const char*>& labels) {
    mBands.clear();
    
    for (size_t i = 0; i < frequencies.size() && i < colors.size(); ++i) {
      CrossoverBandLegacy band;
      band.frequencyHz = frequencies[i];
      band.color = colors[i];
      band.label = (i < labels.size()) ? labels[i] : nullptr;
      mBands.push_back(band);
    }

    // Reset new-API storage
    mBandMagnitudeFns.clear();
    mBandColors.clear();
    mBandLabels.clear();
    mGetCrossoverFrequencies = nullptr;
  }
  
  bool InfluencesAutoScale() const override { return true; }
  
  void GetDataBounds(float& minDB, float& maxDB) const override {
    minDB = -40.0f; // Crossovers typically don't go below -40dB
    maxDB = 6.0f;   // Allow some headroom for summing
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Generate screen-space frequency sampling
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);
    
    // Determine number of bands via new API or legacy fallback
    size_t bandCount = !mBandMagnitudeFns.empty() ? mBandMagnitudeFns.size() : (mBands.size() + 1);

    if (mBandResponses.size() < bandCount) {
      mBandResponses.resize(bandCount);
    }

    // Calculate responses per band
    for (size_t bandIdx = 0; bandIdx < bandCount; ++bandIdx) {
      auto& bandResponse = mBandResponses[bandIdx];
      bandResponse.resize(TSegmentCount);
      
      for (int i = 0; i < TSegmentCount; ++i) {
        float freq = mFrequencies[i];
        float magLin = 1.0f;

        if (!mBandMagnitudeFns.empty()) {
          // New path: call provided magnitude function
          magLin = mBandMagnitudeFns[bandIdx](freq);
        } else {
          // Legacy path: derive from LR crossovers list (bands = crossovers + 1)
          if (bandIdx == 0) {
            // Lowest band: product of LPFs at each active crossover
            for (const auto& xover : mBands) {
              if (xover.isActive) {
                magLin *= M7::LinkwitzRileyFilter::MagnitudeLPF(freq, xover.frequencyHz);
              }
            }
          } else {
            for (size_t xoverIdx = 0; xoverIdx < mBands.size(); ++xoverIdx) {
              const auto& xover = mBands[xoverIdx];
              if (!xover.isActive) continue;
              if (xoverIdx < bandIdx) {
                magLin *= M7::LinkwitzRileyFilter::MagnitudeHPF(freq, xover.frequencyHz);
              } else {
                magLin *= M7::LinkwitzRileyFilter::MagnitudeLPF(freq, xover.frequencyHz);
              }
            }
          }
        }
        bandResponse[i] = M7::math::LinearToDecibels(magLin);
      }
    }
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    const size_t bandCount = mBandResponses.size();

    // Render each crossover band response
    for (size_t bandIdx = 0; bandIdx < bandCount; ++bandIdx) {
      const auto& bandResponse = mBandResponses[bandIdx];
      if (bandResponse.empty()) continue;
      
      // Determine band color
      ImColor bandColor = ColorFromHTML("888888", 0.7f);
      if (!mBandMagnitudeFns.empty()) {
        if (bandIdx < mBandColors.size()) bandColor = mBandColors[bandIdx];
      } else {
        if (bandIdx < mBands.size()) bandColor = mBands[bandIdx].color;
      }
      
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
    std::vector<float> lines;
    if (mGetCrossoverFrequencies) {
      mGetCrossoverFrequencies(lines);
    } else if (!mBands.empty()) {
      for (const auto& band : mBands) if (band.isActive) lines.push_back(band.frequencyHz);
    }

    for (size_t i = 0; i < lines.size(); ++i) {
      float x = coords.FreqToX(lines[i], bb);
      if (x >= bb.Min.x && x <= bb.Max.x) {
        // Pick a color: try band color at same index, else default
        ImColor c = ColorFromHTML("cccccc", 0.5f);
        if (!mBandMagnitudeFns.empty()) {
          if (i < mBandColors.size()) c = ImColor(mBandColors[i].Value.x, mBandColors[i].Value.y, mBandColors[i].Value.z, 0.5f);
        } else if (i < mBands.size()) {
          c = ImColor(mBands[i].color.Value.x, mBands[i].color.Value.y, mBands[i].color.Value.z, 0.5f);
        }
        dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, c, 2.0f);

        const char* label = nullptr;
        if (!mBandMagnitudeFns.empty()) {
          if (i < mBandLabels.size()) label = mBandLabels[i];
        } else if (i < mBands.size()) {
          label = mBands[i].label;
        }
        if (label) {
          ImVec2 textSize = ImGui::CalcTextSize(label);
          ImVec2 textPos = {x - textSize.x * 0.5f, bb.Min.y + 5};
          dl->AddText(textPos, ColorFromHTML("ffffff"), label);
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
  
  // New helper: configure from magnitude functions directly
  void SetBands(const std::vector<std::function<float(float)>>& bandMagnitudeFns,
                const std::vector<ImColor>& colors = {},
                const std::vector<const char*>& labels = {},
                std::function<void(std::vector<float>&)> getCrossoverFrequencies = nullptr) {
    if (mCrossoverLayer) mCrossoverLayer->SetBands(bandMagnitudeFns, colors, labels, std::move(getCrossoverFrequencies));
  }

  // Legacy helper kept for compatibility
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
