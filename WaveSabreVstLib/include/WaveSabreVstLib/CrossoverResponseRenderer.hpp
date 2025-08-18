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
#include <WaveSabreCore/Maj7MBC.hpp>
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
  // Connected device for param access
  const WaveSabreCore::Maj7MBC* mDevice = nullptr;

  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::vector<std::vector<float>> mBandResponses; // Per-band magnitude responses
  
  // Visuals
  // Default colors (lows/mids/highs). If a global theme is available, substitute here.
  std::vector<ImColor> mBandColors { ColorFromHTML(bandColors[0], 0.8f), ColorFromHTML(bandColors[1], 0.8f), ColorFromHTML(bandColors[2], 0.8f)};
  std::vector<const char*> mBandLabels { "Low", "Mid", "High" };

  // Independent Y-axis scaling (like FFTSpectrumLayer)
  float mXODisplayMinDB = -40.0f;
  float mXODisplayMaxDB = 6.0f;
  bool mUseIndependentScale = true;

  // Convert dB to Y using independent scale if enabled
  float XODBToY(float dB, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
    if (mUseIndependentScale) {
      float t01 = M7::math::lerp_rev(mXODisplayMinDB, mXODisplayMaxDB, dB);
      return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
    }
    return coords.DBToY(dB, bb);
  }

public:
  CrossoverResponseLayer() {
    mBandResponses.resize(3); // Low, Mid, High
  }

  void SetCrossoverDevice(const WaveSabreCore::Maj7MBC* device) {
    mDevice = device;
  }
  
  bool InfluencesAutoScale() const override { return true; }
  
  void GetDataBounds(float& minDB, float& maxDB) const override {
    minDB = -40.0f; // Crossovers typically don't go below -40dB
    maxDB = 6.0f;   // Allow some headroom for summing
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Generate screen-space frequency sampling
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);

    // Default: empty responses if no device
    if (!mDevice) {
      for (auto &br : mBandResponses) br.assign(TSegmentCount, -100.0f);
      return;
    }

    // Read crossover frequencies from device params
    float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
    float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
    M7::ParamAccessor paA{ &rawA, 0 };
    M7::ParamAccessor paB{ &rawB, 0 };
    float crossoverA = paA.GetFrequency(0, M7::gFilterFreqConfig);
    float crossoverB = paB.GetFrequency(0, M7::gFilterFreqConfig);

    // Use splitter magnitudes from the device
    auto* pSplitter = &mDevice->splitter0;

    // Calculate responses for each band
    for (int i = 0; i < TSegmentCount; ++i) {
      float f = mFrequencies[i];

      // Low/Mid/High band magnitudes from device splitter (LR topology)
      auto mags = pSplitter->GetMagnitudesAtFrequency(f, crossoverA, crossoverB);
      float lowLin = mags[0];
      float midLin = mags[1];
      float highLin = mags[2];

      if (mBandResponses.size() < 3) mBandResponses.resize(3);
      if ((int)mBandResponses[0].size() != TSegmentCount) {
        mBandResponses[0].resize(TSegmentCount);
        mBandResponses[1].resize(TSegmentCount);
        mBandResponses[2].resize(TSegmentCount);
      }

      mBandResponses[0][i] = M7::math::LinearToDecibels(lowLin);
      mBandResponses[1][i] = M7::math::LinearToDecibels(midLin);
      mBandResponses[2][i] = M7::math::LinearToDecibels(highLin);
    }
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    // Render each crossover band response
    for (size_t bandIdx = 0; bandIdx < mBandResponses.size(); ++bandIdx) {
      const auto& bandResponse = mBandResponses[bandIdx];
      if (bandResponse.empty()) continue;
      
      // Determine band color
      ImColor bandColor = (bandIdx < mBandColors.size()) ? mBandColors[bandIdx] : ColorFromHTML("888888", 0.7f);
      
      // Fill area under curve by drawing trapezoids between successive samples
      ImColor fillColor = ImColor(bandColor.Value.x, bandColor.Value.y, bandColor.Value.z, 0.20f);
      float yBottom = bb.Max.y; // bottom of plot area
      float displayMin = mUseIndependentScale ? mXODisplayMinDB : coords.mDisplayMinDB;
      float displayMax = mUseIndependentScale ? mXODisplayMaxDB : coords.mDisplayMaxDB;
      for (int i = 0; i < TSegmentCount - 1; ++i) {
        float dB0 = bandResponse[i];
        float dB1 = bandResponse[i + 1];
        dB0 = M7::math::clamp(dB0, displayMin, displayMax);
        dB1 = M7::math::clamp(dB1, displayMin, displayMax);
        float x0 = mScreenX[i];
        float x1 = mScreenX[i + 1];
        float y0 = XODBToY(dB0, coords, bb);
        float y1 = XODBToY(dB1, coords, bb);
        ImVec2 quad[4] = { {x0, y0}, {x1, y1}, {x1, yBottom}, {x0, yBottom} };
        dl->AddConvexPolyFilled(quad, 4, fillColor);
      }
      
      // Build polyline points
      std::vector<ImVec2> points;
      points.reserve(TSegmentCount);
      
      for (int i = 0; i < TSegmentCount; ++i) {
        float dB = bandResponse[i];
        if (dB < displayMin || dB > displayMax) continue;
        
        float y = XODBToY(dB, coords, bb);
        points.push_back({mScreenX[i], y});
      }
      
      if (points.size() >= 2) {
        dl->AddPolyline(points.data(), static_cast<int>(points.size()), bandColor, 0, 2.0f);
      }
    }

    // Draw crossover marker lines (if device present)
    if (mDevice) {
      float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
      float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
      M7::ParamAccessor paA{ &rawA, 0 };
      M7::ParamAccessor paB{ &rawB, 0 };
      float fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
      float fB = paB.GetFrequency(0, M7::gFilterFreqConfig);

      std::vector<float> lines;
      if (fA > 0) lines.push_back(fA);
      if (fB > 0) lines.push_back(fB);

      for (size_t i = 0; i < lines.size(); ++i) {
        float x = coords.FreqToX(lines[i], bb);
        ImColor c = (i < mBandColors.size()) ? ImColor(mBandColors[i].Value.x, mBandColors[i].Value.y, mBandColors[i].Value.z, 0.5f) : ColorFromHTML("cccccc", 0.5f);
        

		// vertical line at crossover frequency
        dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, c, 2.0f);

        const char* label = (i < mBandLabels.size()) ? mBandLabels[i] : nullptr;
        if (label) {
          ImVec2 textSize = ImGui::CalcTextSize(label);
          ImVec2 textPos = {x - textSize.x * 0.5f, bb.Min.y + 5};
          dl->AddText(textPos, ColorFromHTML("ffffff"), label);
        }
      }
    }

    // Show scale indicator when using independent scale
    if (mUseIndependentScale) {
      char scaleText[32];
      snprintf(scaleText, sizeof(scaleText), "XO: %.0f to %.0fdB", mXODisplayMinDB, mXODisplayMaxDB);
      ImVec2 textSize = ImGui::CalcTextSize(scaleText);
      ImVec2 textPos = {bb.Max.x - textSize.x - 4, bb.Min.y + 2};
      dl->AddText(textPos, ColorFromHTML("888888", 0.7f), scaleText);
    }
  }
  
  void SetDisplayRange(float minDB, float maxDB) { mXODisplayMinDB = minDB; mXODisplayMaxDB = maxDB; }
  void SetUseIndependentScale(bool use) { mUseIndependentScale = use; }
  void SetScaling(float minDB, float maxDB, bool independent = true) {
    mXODisplayMinDB = minDB; mXODisplayMaxDB = maxDB; mUseIndependentScale = independent;
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
  
  void SetCrossoverDevice(const WaveSabreCore::Maj7MBC* device) {
    if (mCrossoverLayer) mCrossoverLayer->SetCrossoverDevice(device);
  }
  
  void Render() {
    mGraph.Render();
  }
};

} // namespace WaveSabreVstLib
