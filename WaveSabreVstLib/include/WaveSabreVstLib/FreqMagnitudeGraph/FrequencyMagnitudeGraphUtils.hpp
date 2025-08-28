#pragma once

#include "Common.h"
#include <algorithm>  // for std::sort
#include <d3d9.h>
#include <functional>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <WaveSabreVstLib/AnalysisMocks.hpp>
#include "Maj7VstUtils.hpp"
#include "VstPlug.h"
#include <WaveSabreCore/FFTAnalysis.hpp>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>

using real_t = WaveSabreCore::M7::real_t;
using namespace WaveSabreCore;

namespace WaveSabreVstLib
{

// Forward declarations
class IFrequencyGraphLayer;
template <int TWidth, int THeight, bool TShowGridLabels>
class FrequencyMagnitudeGraph;

//=============================================================================
// Core coordinate system shared by all layers
//=============================================================================
struct FrequencyMagnitudeCoordinateSystem
{
  // Dynamic scaling state
  float mDisplayMinDB = -12.0f;
  float mDisplayMaxDB = 12.0f;
  float mCurrentHalfRangeDB = 12.0f;
  int mShrinkCountdown = 0;  // hysteresis for shrinking range

  // Core coordinate mapping functions
  float FreqToX(float hz, const ImRect& bb) const
  {
    float underlyingValue = 0;
    M7::ParamAccessor p{&underlyingValue, 0};
    p.SetFrequencyAssumingNoKeytracking(0, M7::gFilterFreqConfig, hz);
    return M7::math::lerp(bb.Min.x, bb.Max.x, underlyingValue);
  }

  float DBToY(float dB, const ImRect& bb) const
  {
    float t01 = M7::math::lerp_rev(mDisplayMinDB, mDisplayMaxDB, dB);
    t01 = M7::math::clamp01(t01);
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
  }

  // Inverse coordinate mapping functions
  // Convert screen X coordinate back to frequency (inverse of FreqToX)
  float XToFreq(float x, const ImRect& bb) const
  {
    float t01 = M7::math::lerp_rev(bb.Min.x, bb.Max.x, x);
    t01 = M7::math::clamp01(t01);

    // Use the same frequency mapping as FreqToX but in reverse
    float underlyingValue = t01;
    M7::ParamAccessor p{&underlyingValue, 0};
    return p.GetFrequency(0, M7::gFilterFreqConfig);
  }

  // Convert screen Y coordinate back to dB (inverse of DBToY)
  float YToDB(float y, const ImRect& bb) const
  {
    float t01 = M7::math::lerp_rev(bb.Max.y, bb.Min.y, y);  // Note: inverted Y axis
    t01 = M7::math::clamp01(t01);
    return M7::math::lerp(mDisplayMinDB, mDisplayMaxDB, t01);
  }

  // Dynamic scaling logic with hysteresis
  void UpdateScaling(float minDataDB, float maxDataDB, bool preventShrink = false)
  {
    auto stepUp6 = [](float v)
    {
      return 6.0f * std::ceil(v / 6.0f);
    };

    const float baseHalf = 12.0f;
    const float maxHalf = 48.0f;

    // Add margin before scaling up - start expanding at 10dB instead of 12dB for better UX
    const float scalingMargin = 2.0f;
    float adjustedPosThreshold = std::max(0.0f, maxDataDB + scalingMargin);
    float adjustedNegThreshold = std::max(0.0f, -minDataDB + scalingMargin);

    float posNeed = stepUp6(std::max(baseHalf, adjustedPosThreshold));
    float negNeed = stepUp6(std::max(baseHalf, adjustedNegThreshold));
    float suggestedHalf = std::min(maxHalf, std::max(posNeed, negNeed));

    // Enhanced hysteresis: while dragging, only allow scale increases (never shrink)
    const int shrinkFrames = 6;

    if (suggestedHalf > mCurrentHalfRangeDB)
    {
      // Always allow scale increases
      mCurrentHalfRangeDB = suggestedHalf;
      mShrinkCountdown = shrinkFrames;
    }
    else if (suggestedHalf < mCurrentHalfRangeDB && !preventShrink)
    {
      // Only allow scale decreases when not prevented
      if (mShrinkCountdown > 0)
      {
        --mShrinkCountdown;
      }
      else
      {
        mCurrentHalfRangeDB = suggestedHalf;
      }
    }
    else
    {
      // Reset shrink countdown if we're at the same scale or if shrinking prevented
      mShrinkCountdown = shrinkFrames;
    }

    mDisplayMinDB = -mCurrentHalfRangeDB;
    mDisplayMaxDB = +mCurrentHalfRangeDB;
  }
};

//=============================================================================
// Layer plugin system interface
//=============================================================================
class IFrequencyGraphLayer
{
public:
  virtual ~IFrequencyGraphLayer() = default;

  // Data processing phase (can be cached/dirty-checked independently)
  virtual void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) = 0;

  // Rendering phase
  virtual void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) = 0;

  // Scaling influence (for auto-scale calculation)
  virtual bool InfluencesAutoScale() const
  {
    return false;
  }
  virtual void GetDataBounds(float& minDB, float& maxDB) const
  {
    minDB = maxDB = 0;
  }

  // Interaction support
  virtual bool HandleMouse(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb)
  {
    return false;
  }
};

//=============================================================================
// Screen-space frequency sampling utilities
//=============================================================================
struct ScreenSpaceFrequencySampler
{
  static constexpr int gPixelsPerSegment = 2;

  static constexpr int GetSegmentCount(int width)
  {
    return std::max(5, width / gPixelsPerSegment);
  }

  static void GenerateFrequencyPoints(int segmentCount,
                                      std::vector<float>& frequencies,
                                      std::vector<float>& screenX,
                                      const FrequencyMagnitudeCoordinateSystem& coords,
                                      const ImRect& bb)
  {
    frequencies.resize(segmentCount);
    screenX.resize(segmentCount);

    M7::QuickParam param{0, M7::gFilterFreqConfig};
    for (int i = 0; i < segmentCount; ++i)
    {
      param.SetRawValue(float(i) / segmentCount);
      frequencies[i] = param.GetFrequency();
      screenX[i] = coords.FreqToX(frequencies[i], bb);
    }
  }
};

//=============================================================================
// Grid configuration utilities
//=============================================================================
struct GridConfiguration
{
  std::vector<float> majorFreqTicks{100.0f, 1000.0f, 10000.0f, 20000.0f};
  std::vector<float> minorFreqTicks{50.0f,    60,       70,       80,       90,       200.0f,   300,      400,
                                    500.0f,   600,      700,      800,      900,      2000.0f,  3000.0f,  4000.0f,
                                    5000.0f,  6000.0f,  7000.0f,  8000.0f,  9000.0f,  11000.0f, 12000.0f, 13000.0f,
                                    14000.0f, 15000.0f, 16000.0f, 17000.0f, 18000.0f, 19000.0f};

  // Colors for grid rendering
  ImColor gridMinor = ColorFromHTML("333333", 0.4f);
  ImColor gridMajor = ColorFromHTML("555555", 0.7f);
  ImColor labelColor = ColorFromHTML("AAAAAA", 0.65f);
  ImColor unityLineColor = ColorFromHTML("cccccc", 0.4f);

  float labelPad = 2.0f;
};

//=============================================================================
// FFT overlay configuration
//=============================================================================
struct FFTAnalysisOverlay
{
  const IFrequencyAnalysis* frequencyAnalysis = nullptr;  // FFT data source
  ImColor fftColor = ColorFromHTML("4488FF", 0.6f);       // spectrum line color
  ImColor fftFillColor = ColorFromHTML("4488FF", 0.2f);   // spectrum fill color
  bool enableFftFill = true;                              // enable filled area under curve
  const char* label = nullptr;                            // optional label for legend/display
  // Optional value transform per overlay (e.g., convert Width->dB or gamma compress Width)
  std::function<float(float)> valueTransform = nullptr;
};


}  // namespace WaveSabreVstLib
