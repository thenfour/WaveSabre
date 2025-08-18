
#include "CrossoverResponseRenderer.hpp"
#include "EQResponseLayer.hpp"
#include "FFTSpectrumLayer.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include "FrequencyResponseRendererLayered.hpp"
#include "ThumbInteractionLayer.hpp"
#include "TooltipLayer.hpp"

namespace WaveSabreVstLib {

//=============================================================================
// WAVESABRE LAYERED FREQUENCY VISUALIZATION SYSTEM
//
// This system provides a clean, composable architecture for frequency domain
// visualizations. It separates concerns into focused layers that can be mixed
// and matched for different use cases.
//
// CORE COMPONENTS:
// 1. FrequencyMagnitudeGraph - Main container and coordinate system manager
// 2. IFrequencyGraphLayer - Base interface for all visualization layers
// 3. Individual layer implementations for specific features
//
// USAGE EXAMPLES:
//
// Basic EQ visualization (same as original FrequencyResponseRenderer):
//   FrequencyMagnitudeGraph<400, 200, true> graph;
//   graph.AddLayer(std::make_unique<GridLayer<true>>());
//   graph.AddLayer(std::make_unique<EQResponseLayer<...>>(filters));
//   graph.AddLayer(std::make_unique<ThumbInteractionLayer<...>>(filters));
//   graph.Render();
//
// Crossover visualization:
//   CrossoverVisualization<400, 200> crossover;
//   crossover.SetCrossoverFrequencies({300.0f, 3000.0f});
//   crossover.Render();
//
// Custom visualization with FFT + EQ:
//   FrequencyMagnitudeGraph<400, 200, true> graph;
//   graph.AddLayer(std::make_unique<GridLayer<true>>());
//   graph.AddLayer(std::make_unique<FFTSpectrumLayer<...>>(fftOverlays));
//   graph.AddLayer(std::make_unique<EQResponseLayer<...>>(filters));
//   graph.Render();
//
//=============================================================================

// Helper function to create a standard EQ visualization
template <int TWidth, int THeight, size_t TFilterCount, size_t TParamCount>
std::unique_ptr<FrequencyMagnitudeGraph<TWidth, THeight, true>>
CreateStandardEQVisualization(
    const std::array<FrequencyResponseRendererFilter, TFilterCount> &filters) {
  auto graph =
      std::make_unique<FrequencyMagnitudeGraph<TWidth, THeight, true>>();

  // Add layers in rendering order
  graph->AddLayer(std::make_unique<GridLayer<true>>());

  auto eqLayer = std::make_unique<EQResponseLayer<
      FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount,
      TFilterCount, TParamCount>>(filters);

  auto thumbLayer =
      std::make_unique<ThumbInteractionLayer<TFilterCount, TParamCount>>(
          filters);

  auto tooltipLayer =
      std::make_unique<TooltipLayer<TFilterCount, TParamCount>>();
  tooltipLayer->SetThumbLayer(thumbLayer.get());
  tooltipLayer->SetEQLayer(eqLayer.get());

  graph->AddLayer(std::move(eqLayer));
  graph->AddLayer(std::move(thumbLayer));
  graph->AddLayer(std::move(tooltipLayer));

  return graph;
}

// Helper function to create a visualization with FFT overlay
template <int TWidth, int THeight>
std::unique_ptr<FrequencyMagnitudeGraph<TWidth, THeight, true>>
CreateFFTVisualization(const std::vector<FFTAnalysisOverlay> &fftOverlays) {
  auto graph =
      std::make_unique<FrequencyMagnitudeGraph<TWidth, THeight, true>>();

  graph->AddLayer(std::make_unique<GridLayer<true>>());
  graph->AddLayer(
      std::make_unique<FFTSpectrumLayer<
          FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount>>(
          fftOverlays));

  return graph;
}

} // namespace WaveSabreVstLib
