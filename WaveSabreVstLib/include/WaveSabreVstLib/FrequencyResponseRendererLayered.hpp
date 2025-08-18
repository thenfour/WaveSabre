#pragma once

#include "FrequencyMagnitudeGraphUtils.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include "ThumbInteractionLayer.hpp"
#include "TooltipLayer.hpp"
#include "Maj7VstUtils.hpp"
#include "CrossoverResponseRenderer.hpp"


namespace WaveSabreVstLib {
//=============================================================================
// Compatibility wrapper - maintains exact same API as original FrequencyResponseRenderer
//=============================================================================
template <int TWidth, int THeight, size_t TFilterCount, size_t TParamCount, bool TShowGridLabels>
class FrequencyResponseRendererLayered {
private:
  FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels> mGraph;
  
  // Store raw pointers to layers for configuration (graph owns the layers)
  GridLayer<TShowGridLabels>* mGridLayer = nullptr;
  FFTSpectrumLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mFFTLayer = nullptr;
  CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mCrossoverLayer = nullptr;
  EQResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount, TFilterCount, TParamCount>* mEQLayer = nullptr;
  ThumbInteractionLayer<TFilterCount, TParamCount>* mThumbLayer = nullptr;
  TooltipLayer<TFilterCount, TParamCount>* mTooltipLayer = nullptr;
  
public:
  FrequencyResponseRendererLayered() {
    // Create layers and add them to graph (graph takes ownership)
    auto gridLayer = std::make_unique<GridLayer<TShowGridLabels>>();
    mGridLayer = gridLayer.get();
    mGraph.AddLayer(std::move(gridLayer));
    
    auto fftLayer = std::make_unique<FFTSpectrumLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>>(std::vector<FFTAnalysisOverlay>{});
    mFFTLayer = fftLayer.get();
    mGraph.AddLayer(std::move(fftLayer));

    auto crossoverLayer = std::make_unique<CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>>();
    mCrossoverLayer = crossoverLayer.get();
    mGraph.AddLayer(std::move(crossoverLayer));
    
    auto eqLayer = std::make_unique<EQResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount, TFilterCount, TParamCount>>(std::array<FrequencyResponseRendererFilter, TFilterCount>{});
    mEQLayer = eqLayer.get();
    mGraph.AddLayer(std::move(eqLayer));
    
    auto thumbLayer = std::make_unique<ThumbInteractionLayer<TFilterCount, TParamCount>>(std::array<FrequencyResponseRendererFilter, TFilterCount>{});
    mThumbLayer = thumbLayer.get();
    mGraph.AddLayer(std::move(thumbLayer));
    
    auto tooltipLayer = std::make_unique<TooltipLayer<TFilterCount, TParamCount>>();
    mTooltipLayer = tooltipLayer.get();
    
    // Set up layer connections
    mTooltipLayer->SetThumbLayer(mThumbLayer);
    mTooltipLayer->SetEQLayer(mEQLayer);
    
    mGraph.AddLayer(std::move(tooltipLayer));
  }
  
  // Maintain exact same API as original FrequencyResponseRenderer
  void OnRender(const FrequencyResponseRendererConfig<TFilterCount, TParamCount> &cfg) {
    // Configure the graph background
    mGraph.SetBackgroundColor(cfg.backgroundColor);
    
    // Update FFT layer configuration directly
    if (mFFTLayer) {
      mFFTLayer->SetOverlays(cfg.fftOverlays);
      mFFTLayer->SetDisplayRange(cfg.fftDisplayMinDB, cfg.fftDisplayMaxDB);
      mFFTLayer->SetUseIndependentScale(cfg.useIndependentFFTScale);
    }
    
    // Configure EQ layer
    if (mEQLayer) {
      mEQLayer->SetParamCacheCopy(cfg.mParamCacheCopy);
      mEQLayer->SetFilters(cfg.filters);
      mEQLayer->SetLineColor(cfg.lineColor);
    }
    
    // Configure thumb layer
    if (mThumbLayer) {
      mThumbLayer->SetFilters(cfg.filters);
      mThumbLayer->SetThumbRadius(cfg.thumbRadius);
    }
    
    // Configure grid layer
    if (mGridLayer) {
      mGridLayer->SetFrequencyTicks(cfg.majorFreqTicks, cfg.minorFreqTicks);
    }
    
    // Render the graph
    mGraph.Render();
  }

  // Configure crossover visualization via direct magnitude providers (preferred)
  void SetCrossoverBands(const std::vector<std::function<float(float)>>& bandMagnitudeFns,
                         const std::vector<ImColor>& colors = {},
                         const std::vector<const char*>& labels = {},
                         std::function<void(std::vector<float>&)> getCrossoverFrequencies = nullptr)
  {
    if (!mCrossoverLayer) return;
    mCrossoverLayer->SetBands(bandMagnitudeFns, colors, labels, std::move(getCrossoverFrequencies));
  }

  // Configure crossover visualization frequencies (legacy)
  void SetCrossoverFrequencies(const std::vector<float>& frequencies,
                               const std::vector<ImColor>& colors = {},
                               const std::vector<const char*>& labels = {})
  {
    if (!mCrossoverLayer) return;

    // If no colors provided, choose defaults up to 4 bands
    std::vector<ImColor> useColors = colors;
    if (useColors.empty()) {
      useColors = {
        ColorFromHTML("ff4444", 0.8f),
        ColorFromHTML("44ff44", 0.8f),
        ColorFromHTML("4444ff", 0.8f),
        ColorFromHTML("ffff44", 0.8f)
      };
    }

    std::vector<const char*> useLabels = labels;
    if (useLabels.empty()) {
      useLabels = { "Low", "Mid", "High", "Ultra" };
    }

    mCrossoverLayer->SetCrossoverFrequencies(frequencies, useColors, useLabels);
  }
  
  // Expose coordinate conversion functions for compatibility
  float FreqToX(float hz, ImRect &bb) {
    return mGraph.mCoords.FreqToX(hz, bb);
  }
  
  float DBToY(float dB, ImRect &bb) {
    return mGraph.mCoords.DBToY(dB, bb);
  }
  
  float XToFreq(float x, ImRect &bb) {
    return mGraph.mCoords.XToFreq(x, bb);
  }
  
  float YToDB(float y, ImRect &bb) {
    return mGraph.mCoords.YToDB(y, bb);
  }
};

} // namespace WaveSabreVstLib