
#include "CrossoverResponseRenderer.hpp"
#include "EQResponseLayer.hpp"
#include "FFTSpectrumLayer.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include "FrequencyResponseRendererLayered.hpp"
#include "ThumbInteractionLayer.hpp"
#include "TooltipLayer.hpp"

namespace WaveSabreVstLib {

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
