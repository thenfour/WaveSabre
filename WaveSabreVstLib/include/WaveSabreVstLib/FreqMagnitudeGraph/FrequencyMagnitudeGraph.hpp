#pragma once

#include "FrequencyMagnitudeGraphUtils.hpp"

namespace WaveSabreVstLib {

//=============================================================================
// Main frequency magnitude graph component with layered architecture
//=============================================================================
template <int TWidth, int THeight, bool TShowGridLabels>
class FrequencyMagnitudeGraph {
public:
  static constexpr ImVec2 gSize = {TWidth, THeight};
  static constexpr int gSegmentCount = ScreenSpaceFrequencySampler::GetSegmentCount(TWidth);
  
  FrequencyMagnitudeCoordinateSystem mCoords;
  std::vector<std::unique_ptr<IFrequencyGraphLayer>> mLayers;
  ImColor mBackgroundColor = ColorFromHTML("222222", 1.0f);
  bool mUseFixedYScale = false;
  float mFixedYHalfRangeDB = 12.0f;

  void ApplyFixedYScale_() {
    const float half = std::max(6.0f, mFixedYHalfRangeDB);
    mCoords.mCurrentHalfRangeDB = half;
    mCoords.mDisplayMinDB = -half;
    mCoords.mDisplayMaxDB = +half;
  }
  
  void AddLayer(std::unique_ptr<IFrequencyGraphLayer> layer) {
    mLayers.push_back(std::move(layer));
  }
  
  void SetBackgroundColor(ImColor color) {
    mBackgroundColor = color;
  }

  void SetFixedYScaleHalfRange(float halfRangeDB) {
    mUseFixedYScale = true;
    mFixedYHalfRangeDB = halfRangeDB;
  }

  void ClearFixedYScale() {
    mUseFixedYScale = false;
  }
  
  void Render() {
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = bb.Min + gSize;
    
    auto* dl = ImGui::GetWindowDrawList();
    
    // Draw background
    ImGui::RenderFrame(bb.Min, bb.Max, mBackgroundColor);

    if (mUseFixedYScale) {
      ApplyFixedYScale_();
    }
    
    // Update data for all layers
    for (auto& layer : mLayers) {
      layer->UpdateData(mCoords, bb);
    }
    
    // Calculate auto-scaling from layers that influence it
    float minInfluenceDB = 1e9f, maxInfluenceDB = -1e9f;
    bool hasInfluence = false;
    bool preventShrink = false;
    
    for (const auto& layer : mLayers) {
      if (layer->InfluencesAutoScale()) {
        float layerMin, layerMax;
        layer->GetDataBounds(layerMin, layerMax);
        minInfluenceDB = std::min(minInfluenceDB, layerMin);
        maxInfluenceDB = std::max(maxInfluenceDB, layerMax);
        hasInfluence = true;
      }
    }
    
    // Handle mouse interactions (first layer that handles it wins)
    for (auto& layer : mLayers) {
      if (layer->HandleMouse(mCoords, bb)) {
        preventShrink = true; // Prevent shrinking while interacting
        break;
      }
    }
    
    if (hasInfluence && !mUseFixedYScale) {
      mCoords.UpdateScaling(minInfluenceDB, maxInfluenceDB, preventShrink);
    }

    if (mUseFixedYScale) {
      ApplyFixedYScale_();
    }
    
    // Render all layers in order
    for (auto& layer : mLayers) {
      layer->Render(mCoords, bb, dl);
    }
    
    // Reserve space in layout
    ImGui::Dummy(gSize);
  }
};

//=============================================================================
// Grid layer - handles background grid and labels
//=============================================================================
template <bool TShowLabels>
class GridLayer : public IFrequencyGraphLayer {
private:
  GridConfiguration mConfig;
  
public:
  GridLayer() = default;
  GridLayer(const GridConfiguration& config) : mConfig(config) {}
  
  void SetFrequencyTicks(const std::vector<float>& majorTicks, const std::vector<float>& minorTicks) {
    if (!majorTicks.empty()) {
      mConfig.majorFreqTicks = majorTicks;
    }
    if (!minorTicks.empty()) {
      mConfig.minorFreqTicks = minorTicks;
    }
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Grid doesn't need data updates
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    const float labelPad = mConfig.labelPad;
    
    // Frequency grid (vertical lines)
    auto drawFreqTick = [&](float f, bool major) {
      float x = std::round(coords.FreqToX(f, bb));
      if (x < bb.Min.x || x > bb.Max.x) return;
      
      dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, major ? mConfig.gridMajor : mConfig.gridMinor, 1.0f);
      
      if constexpr (TShowLabels) {
        if (major) {
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
          dl->AddText(pos, mConfig.labelColor, txt);
        }
      }
    };
    
    // Draw frequency ticks
    for (float f : mConfig.minorFreqTicks) drawFreqTick(f, false);
    for (float f : mConfig.majorFreqTicks) drawFreqTick(f, true);
    
    // dB grid (horizontal lines)
    int half = (int)std::ceil(coords.mCurrentHalfRangeDB / 6.0f) * 6;
    for (int v = -half; v <= half; v += 6) {
      float dB = (float)v;
      float y = std::round(coords.DBToY(dB, bb));
      if (y < bb.Min.y || y > bb.Max.y) continue;
      
      bool major = (v == 0) || (v % 12 == 0);
      dl->AddLine({bb.Min.x, y}, {bb.Max.x, y}, major ? mConfig.gridMajor : mConfig.gridMinor, 1.0f);
      
      if constexpr (TShowLabels) {
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
        dl->AddText(pos, mConfig.labelColor, txt);
      }
    }
    
    // Unity line (0 dB) over grid for visibility
    float unityY = std::round(coords.DBToY(0, bb));
    dl->AddLine({bb.Min.x, unityY}, {bb.Max.x, unityY}, mConfig.unityLineColor);
  }
};

} // namespace WaveSabreVstLib