#pragma once

#include "FrequencyMagnitudeGraph.hpp"
#include "Maj7VstUtils.hpp"
#include "FrequencyDomainMath.hpp"

namespace WaveSabreVstLib {

// Forward declare the filter structure from existing code
struct FrequencyResponseRendererFilter;

//=============================================================================
// EQ/Filter response layer - handles biquad filter chains
//=============================================================================
template <int TSegmentCount, size_t TFilterCount, size_t TParamCount>
class EQResponseLayer : public IFrequencyGraphLayer {
private:
  std::array<FrequencyResponseRendererFilter, TFilterCount> mFilters;
  float mParamCacheCopy[TParamCount];
  float mParamCacheCache[TParamCount] = {0};
  
  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::vector<float> mMagdB;
  
  ImColor mLineColor = ColorFromHTML("ff8800", 1.0f);
  int mVisibleLeftIndex = 0;
  int mVisibleRightIndex = TSegmentCount - 1;
  
  int mAdditionalForceCalcFrames = 0;
  
public:
  EQResponseLayer(const std::array<FrequencyResponseRendererFilter, TFilterCount>& filters, 
                  ImColor lineColor = ColorFromHTML("ff8800", 1.0f))
    : mFilters(filters), mLineColor(lineColor) {
    mFrequencies.reserve(TSegmentCount);
    mScreenX.reserve(TSegmentCount);
    mMagdB.reserve(TSegmentCount);
  }
  
  void SetParamCacheCopy(const float* paramCache) {
    memcpy(mParamCacheCopy, paramCache, sizeof(mParamCacheCopy));
  }
  
  void SetFilters(const std::array<FrequencyResponseRendererFilter, TFilterCount>& filters) {
    mFilters = filters;
    ForceRecalculation();
  }
  
  void SetLineColor(ImColor color) {
    mLineColor = color;
  }
  
  bool InfluencesAutoScale() const override { return true; }
  
  void GetDataBounds(float& minDB, float& maxDB) const override {
    minDB = 1e9f;
    maxDB = -1e9f;
    
    for (int i = mVisibleLeftIndex; i <= mVisibleRightIndex; ++i) {
      minDB = std::min(minDB, mMagdB[i]);
      maxDB = std::max(maxDB, mMagdB[i]);
    }
    
    if (minDB > maxDB) {
      minDB = maxDB = 0;
    }
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Check if recalculation is needed
    bool areEqual = memcmp(mParamCacheCopy, mParamCacheCache, sizeof(mParamCacheCopy)) == 0;
    
    if (areEqual && mAdditionalForceCalcFrames == 0) {
      return; // No update needed
    }
    
    mAdditionalForceCalcFrames = areEqual ? mAdditionalForceCalcFrames - 1 : 2;
    memcpy(mParamCacheCache, mParamCacheCopy, sizeof(mParamCacheCopy));
    
    // Generate screen-space frequency sampling
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);
    
    mMagdB.resize(TSegmentCount);
    
    // Calculate filter response at screen resolution
    for (int iseg = 0; iseg < TSegmentCount; ++iseg) {
      float freq = mFrequencies[iseg];
      
      // Calculate combined filter magnitude response
      float filterMagdB = 0;
      for (const auto &f : mFilters) {
        if (!f.filter) continue; // nullptr values are valid when filter is bypassed
        
        float magLin = WaveSabreVstLib::BiquadMagnitudeForFrequency(*(f.filter), freq);
        filterMagdB += M7::math::LinearToDecibels(magLin);
      }
      
      mMagdB[iseg] = filterMagdB;
    }
    
    // Determine visible window (exclude deep stopband edges)
    const float cutThreshold = -24.0f; // dB
    const int margin = std::max(1, TSegmentCount / 50); // ~2%

    mVisibleLeftIndex = 0;
    for (int i = 0; i < TSegmentCount; ++i) {
      if (mMagdB[i] > cutThreshold) {
        mVisibleLeftIndex = std::max(0, i - margin);
        break;
      }
      if (i == TSegmentCount - 1)
        mVisibleLeftIndex = 0; // none found
    }

    mVisibleRightIndex = TSegmentCount - 1;
    for (int i = TSegmentCount - 1; i >= 0; --i) {
      if (mMagdB[i] > cutThreshold) {
        mVisibleRightIndex = std::min(TSegmentCount - 1, i + margin);
        break;
      }
      if (i == 0)
        mVisibleRightIndex = TSegmentCount - 1;
    }

    if (mVisibleRightIndex < mVisibleLeftIndex) {
      mVisibleLeftIndex = 0;
      mVisibleRightIndex = TSegmentCount - 1;
    }
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    // Render response as visible segments only
    std::vector<ImVec2> segment;
    segment.reserve(TSegmentCount);

    auto flushSegment = [&]() {
      if (segment.size() >= 2) {
        dl->AddPolyline(segment.data(), (int)segment.size(), mLineColor, 0, 2.0f);
      }
      segment.clear();
    };

    for (int i = mVisibleLeftIndex; i <= mVisibleRightIndex; ++i) {
      float dB = mMagdB[i];
      if (dB < coords.mDisplayMinDB || dB > coords.mDisplayMaxDB) {
        flushSegment();
        continue;
      }
      float y = coords.DBToY(dB, bb);
      segment.push_back({mScreenX[i], y});
    }
    flushSegment();
  }
  
  void ForceRecalculation() {
    mAdditionalForceCalcFrames = 2;
  }
};

} // namespace WaveSabreVstLib