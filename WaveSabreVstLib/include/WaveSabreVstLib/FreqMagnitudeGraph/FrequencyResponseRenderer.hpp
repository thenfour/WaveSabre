#pragma once

#include "Common.h"
#include <d3d9.h>
#include <functional>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>  // for std::sort

#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_internal.h>
#include "VstPlug.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>
#include "FrequencyMagnitudeGraphUtils.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include "FFTSpectrumLayer.hpp"
#include "EQResponseLayer.hpp"

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib {

struct FrequencyResponseRendererFilter {
  const char *thumbColor;
  const M7::BiquadFilter *filter;
  const char* label = nullptr;
  std::function<void(float freqHz, float gainDb, uintptr_t userData)> HandleChangeParam;
  std::function<void(float qValue, uintptr_t userData)> HandleChangeQ;
  uintptr_t userData = 0; // Optional user data for custom handling
};

template <size_t TFilterCount, size_t TParamCount>
struct FrequencyResponseRendererConfig {
  const ImColor backgroundColor;
  const ImColor lineColor;
  const float thumbRadius;
  const std::array<FrequencyResponseRendererFilter, TFilterCount> filters;
  float mParamCacheCopy[TParamCount];
  // Optional: explicit frequency grid configuration (in Hz). If empty, built-in defaults are used.
  std::vector<float> majorFreqTicks{}; // labeled and drawn with major style
  std::vector<float> minorFreqTicks{}; // unlabeled and drawn with minor style
  
  // Multiple FFT spectrum overlays with independent scaling
  using FFTAnalysisOverlay = WaveSabreVstLib::FFTAnalysisOverlay; // alias shared type
  
  std::vector<FFTAnalysisOverlay> fftOverlays{};            // Multiple FFT overlays
  
  // Independent FFT Y-axis scaling (shared by all overlays)
  float fftDisplayMinDB = -90.0f;
  float fftDisplayMaxDB = 0.0f;    // Digital maximum (0dB)
  bool useIndependentFFTScale = true;  // Use separate scale for FFT vs EQ response
};

// ... legacy renderer implementation commented out below ...
} // namespace WaveSabreVstLib
