#pragma once

#include "FrequencyMagnitudeGraphUtils.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include "FFTDiffLayer.hpp"
#include "ThumbInteractionLayer.hpp"
#include "TooltipLayer.hpp"
#include "../Maj7VstUtils.hpp"
#include "CrossoverResponseRenderer.hpp"


namespace WaveSabreVstLib {
//=============================================================================
// Compatibility wrapper - maintains exact same API as original FrequencyResponseRenderer
//=============================================================================
template <int TWidth, int THeight, size_t TFilterCount, size_t TParamCount, bool TShowGridLabels>
class FrequencyResponseRendererLayered {
private:
  FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels> mGraph;
  float mSelectedYScaleHalfRangeDB = 0.0f;
  bool mHasSelectedYScale = false;
  
  // Store raw pointers to layers for configuration (graph owns the layers)
  GridLayer<TShowGridLabels>* mGridLayer = nullptr;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  FFTDiffFlatLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mFFTDiffFlatLayer = nullptr;
  FFTDiffLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mFFTDiffLayer = nullptr;
  FFTSpectrumLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mFFTLayer = nullptr;
#endif

  EQResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount, TFilterCount, TParamCount>* mEQLayer = nullptr;
  ThumbInteractionLayer<TFilterCount, TParamCount>* mThumbLayer = nullptr;
  TooltipLayer<TFilterCount, TParamCount>* mTooltipLayer = nullptr;
  
public:
    CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>* mCrossoverLayer = nullptr;
    
    FrequencyResponseRendererLayered() {
    // Create layers and add them to graph (graph takes ownership)
    auto gridLayer = std::make_unique<GridLayer<TShowGridLabels>>();
    mGridLayer = gridLayer.get();
    mGraph.AddLayer(std::move(gridLayer));

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    auto fftDiffFlatLayer = std::make_unique<FFTDiffFlatLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>>();
    mFFTDiffFlatLayer = fftDiffFlatLayer.get();
    mFFTDiffFlatLayer->SetScaling(-24.0f, +24.0f, true);
    mGraph.AddLayer(std::move(fftDiffFlatLayer));

    // Filled diff behind FFT lines
    auto fftDiffLayer = std::make_unique<FFTDiffLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>>();
    mFFTDiffLayer = fftDiffLayer.get();
    mGraph.AddLayer(std::move(fftDiffLayer));
    
    auto fftLayer = std::make_unique<FFTSpectrumLayer<FrequencyMagnitudeGraph<TWidth, THeight, TShowGridLabels>::gSegmentCount>>(std::vector<FFTAnalysisOverlay>{});
    mFFTLayer = fftLayer.get();
    mGraph.AddLayer(std::move(fftLayer));
#endif

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
    if (!cfg.yScaleHalfRangeOptionsDB.empty()) {
      auto hasPreset = [&](float v) {
        for (float candidate : cfg.yScaleHalfRangeOptionsDB) {
          if (std::abs(candidate - v) < 0.001f) return true;
        }
        return false;
      };

      if (!mHasSelectedYScale || !hasPreset(mSelectedYScaleHalfRangeDB)) {
        mSelectedYScaleHalfRangeDB = cfg.yScaleHalfRangeOptionsDB[0];
        mHasSelectedYScale = true;
      }

      ImGui::PushID(this);
      ImGui::AlignTextToFramePadding();
      ImGui::TextUnformatted("Scale");
      for (size_t i = 0; i < cfg.yScaleHalfRangeOptionsDB.size(); ++i) {
        const float half = cfg.yScaleHalfRangeOptionsDB[i];
        const bool isActive = std::abs(half - mSelectedYScaleHalfRangeDB) < 0.001f;
        char label[32] = {0};
        snprintf(label, sizeof(label), "+/-%ddB", (int)std::round(half));

        ImGui::SameLine();
        if (isActive) {
          ImGui::PushStyleColor(ImGuiCol_Button, ColorFromHTML("666666").Value);
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ColorFromHTML("777777").Value);
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, ColorFromHTML("888888").Value);
        }
        if (ImGui::SmallButton(label)) {
          mSelectedYScaleHalfRangeDB = half;
          mHasSelectedYScale = true;
        }
        if (isActive) {
          ImGui::PopStyleColor(3);
        }
      }
      ImGui::PopID();

      mGraph.SetFixedYScaleHalfRange(mSelectedYScaleHalfRangeDB);
    } else {
      mGraph.ClearFixedYScale();
      mHasSelectedYScale = false;
    }

    // Configure the graph background
    mGraph.SetBackgroundColor(cfg.backgroundColor);
    
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    // Update filled FFT diff layer scaling to match FFT layer request (shares scale)
    if (mFFTDiffLayer) {
      mFFTDiffLayer->SetScaling(cfg.fftDisplayMinDB, cfg.fftDisplayMaxDB, cfg.useIndependentFFTScale);
    }
    
    // Update FFT layer configuration directly
    if (mFFTLayer) {
      mFFTLayer->SetOverlays(cfg.fftOverlays);
      mFFTLayer->SetDisplayRange(cfg.fftDisplayMinDB, cfg.fftDisplayMaxDB);
      mFFTLayer->SetUseIndependentScale(cfg.useIndependentFFTScale);
    }
#endif
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

  // Connect the crossover visualization directly to the device
  void SetCrossoverFilter(const WaveSabreCore::Maj7MBC* device) {
    if (mCrossoverLayer) mCrossoverLayer->SetCrossoverDevice(device);
  }
  
  // Set up parameter change handler for crossover frequency dragging
  void SetFrequencyChangeHandler(std::function<void(float freqHz, int crossoverIndex)> handler) {
    if (mCrossoverLayer) mCrossoverLayer->SetFrequencyChangeHandler(handler);
  }
  
  // Set up band selection handler for clicking on band regions
  void SetBandChangeHandler(std::function<void(int bandIndex)> handler) {
    if (mCrossoverLayer) mCrossoverLayer->SetBandChangeHandler(handler);
  }
  
  // Set the currently selected/editing band for highlighting
  void SetCurrentEditingBand(int bandIndex) {
    if (mCrossoverLayer) mCrossoverLayer->SetCurrentEditingBand(bandIndex);
  }
  
  // Set band renderer callback
  void SetBandRenderer(std::function<bool(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl)> renderer) {
    if (mCrossoverLayer) mCrossoverLayer->SetBandRenderer(renderer);
  }

  // Get the current active band rectangles
  const std::vector<std::pair<int, ImRect>>& GetActiveBandRects() const {
    static const std::vector<std::pair<int, ImRect>> empty;
    return mCrossoverLayer ? mCrossoverLayer->GetActiveBandRects() : empty;
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

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

  // Optional: customize caption for FFT scale (e.g., "Width", "dB")
  void SetFFTScaleCaption(const char* caption) {
      if (mFFTLayer) mFFTLayer->SetScaleCaption(caption);
  }

  // Optional: set a value transform for the FFT layer (e.g., gamma/log).
  void SetFFTValueTransform(std::function<float(float)> transform) {
    if (mFFTLayer) mFFTLayer->SetValueTransform(std::move(transform));
    }

  // FFT diff helpers
  void SetFFTDiffOverlay(const FFTDiffOverlay& overlay) {
    if (mFFTDiffLayer) mFFTDiffLayer->SetOverlay(overlay);
  }
  void ClearFFTDiffOverlay() {
    if (mFFTDiffLayer) mFFTDiffLayer->ClearOverlay();
  }

  // FFT diff flat helpers
  void SetFFTDiffFlatOverlay(const FFTDiffFlatOverlay& overlay) {
    if (mFFTDiffFlatLayer) mFFTDiffFlatLayer->SetOverlay(overlay);
  }
  void ClearFFTDiffFlatOverlay() {
    if (mFFTDiffFlatLayer) mFFTDiffFlatLayer->ClearOverlay();
  }
  void SetFFTDiffFlatScaling(float minDB, float maxDB, bool independent = true) {
    if (mFFTDiffFlatLayer) mFFTDiffFlatLayer->SetScaling(minDB, maxDB, independent);
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
};

} // namespace WaveSabreVstLib