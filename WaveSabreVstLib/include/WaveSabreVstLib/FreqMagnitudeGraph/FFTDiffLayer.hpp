#pragma once

#include "FrequencyMagnitudeGraphUtils.hpp"
#include "FrequencyMagnitudeGraph.hpp"
#include <functional>
#include <array>
#include <WaveSabreCore/../../Analysis/FFTAnalysis.hpp>

namespace WaveSabreVstLib {

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

    // Configuration for FFT difference rendering
struct FFTDiffOverlay {
  // Required analyzers (operate in dB domain)
  const WaveSabreCore::IFrequencyAnalysis* sourceA = nullptr; // e.g., Input/Source
  const WaveSabreCore::IFrequencyAnalysis* sourceB = nullptr; // e.g., Output/Processed

  // Configurable colors (tweakable by host UI)
  ImColor colorInAOnly = ColorFromHTML("cc4444", 0.50f); // A above B
  ImColor colorInBoth  = ColorFromHTML("aaaaaa", 0.0f);   // common area (min(A,B))
  ImColor colorInBOnly = ColorFromHTML("449999", 0.50f); // B above A

  bool enabled = true;
};

//=============================================================================
// FFT diff layer: renders filled regions showing A-only, overlap, B-only
//=============================================================================
template <int TSegmentCount>
class FFTDiffLayer : public IFrequencyGraphLayer {
private:
  FFTDiffOverlay mOverlay{};
  bool mHasOverlay = false;

  // Screen-space cache
  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::array<float, TSegmentCount> mA{};
  std::array<float, TSegmentCount> mB{};
  std::array<bool,  TSegmentCount> mValid{};

  // Scaling (mirror FFTSpectrumLayer behavior when configured from wrapper)
  float mDisplayMinDB = -90.0f;
  float mDisplayMaxDB = 0.0f;
  bool  mUseIndependentScale = true; // if false, uses coords scaling

  float MapToY(float value, float minV, float maxV, const ImRect& bb) const {
    const float t01 = (maxV - minV) != 0.0f ? M7::math::clamp01((value - minV) / (maxV - minV)) : 0.0f;
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
  }

  float DBToY(float dB, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
    if (mUseIndependentScale) {
      return MapToY(dB, mDisplayMinDB, mDisplayMaxDB, bb);
    }
    return coords.DBToY(dB, bb);
  }

public:
  FFTDiffLayer() {
    mFrequencies.reserve(TSegmentCount);
    mScreenX.reserve(TSegmentCount);
  }

  void SetOverlay(const FFTDiffOverlay& overlay) {
    mOverlay = overlay;
    mHasOverlay = (overlay.enabled && overlay.sourceA && overlay.sourceB);
  }

  void ClearOverlay() {
    mHasOverlay = false;
    mOverlay = FFTDiffOverlay{};
  }

  void SetScaling(float minDB, float maxDB, bool independent = true) {
    mDisplayMinDB = minDB; mDisplayMaxDB = maxDB; mUseIndependentScale = independent;
  }

  bool InfluencesAutoScale() const override { return false; }

  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Generate screen-space frequency sampling points
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);

    if (!mHasOverlay) {
      for (int i = 0; i < TSegmentCount; ++i) mValid[i] = false;
      return;
    }

    const auto* a = mOverlay.sourceA;
    const auto* b = mOverlay.sourceB;

    const float nyquistA = a ? a->GetNyquistFrequency() : 0.0f;
    const float nyquistB = b ? b->GetNyquistFrequency() : 0.0f;
    const float binResA  = a ? a->GetFrequencyResolution() : 0.0f;
    const float binResB  = b ? b->GetFrequencyResolution() : 0.0f;

    for (int i = 0; i < TSegmentCount; ++i) {
      float f = mFrequencies[i];
      bool valid = (a && b && f > 0.0f);
      if (valid && nyquistA > 0.0f && binResA > 0.0f && f >= nyquistA) {
        f = std::max(0.0f, nyquistA - 0.5f * binResA);
      }
      if (valid && nyquistB > 0.0f && binResB > 0.0f && f >= nyquistB) {
        f = std::max(0.0f, nyquistB - 0.5f * binResB);
      }

      if (valid) {
        mA[i] = a->GetMagnitudeAtFrequency(f);
        mB[i] = b->GetMagnitudeAtFrequency(f);
        mValid[i] = true;
      } else {
        mA[i] = -100.0f; mB[i] = -100.0f; mValid[i] = false;
      }
    }
  }

  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    if (!mHasOverlay) return;

    // Determine display min/max
    const float dispMin = mUseIndependentScale ? mDisplayMinDB : coords.mDisplayMinDB;
    const float dispMax = mUseIndependentScale ? mDisplayMaxDB : coords.mDisplayMaxDB;

    const float baseline = bb.Max.y;

    // Pass 1: draw common area (min(A,B)) as filled quads to baseline
    for (int i = 0; i + 1 < TSegmentCount; ++i) {
      if (!mValid[i] || !mValid[i + 1]) continue;

      float a0 = M7::math::clamp(mA[i], dispMin, dispMax);
      float a1 = M7::math::clamp(mA[i + 1], dispMin, dispMax);
      float b0 = M7::math::clamp(mB[i], dispMin, dispMax);
      float b1 = M7::math::clamp(mB[i + 1], dispMin, dispMax);

      float min0 = std::min(a0, b0);
      float min1 = std::min(a1, b1);

      float x0 = mScreenX[i];
      float x1 = mScreenX[i + 1];
      float yMin0 = DBToY(min0, coords, bb);
      float yMin1 = DBToY(min1, coords, bb);

      ImVec2 quad[4] = { {x0, baseline}, {x0, yMin0}, {x1, yMin1}, {x1, baseline} };
      dl->AddConvexPolyFilled(quad, 4, mOverlay.colorInBoth);
    }

    // Pass 2: draw A-only (A above B)
    for (int i = 0; i + 1 < TSegmentCount; ++i) {
      if (!mValid[i] || !mValid[i + 1]) continue;
      bool aAbove = (mA[i] > mB[i]) || (mA[i + 1] > mB[i + 1]);
      if (!aAbove) continue;

      float a0 = M7::math::clamp(mA[i], dispMin, dispMax);
      float a1 = M7::math::clamp(mA[i + 1], dispMin, dispMax);
      float b0 = M7::math::clamp(mB[i], dispMin, dispMax);
      float b1 = M7::math::clamp(mB[i + 1], dispMin, dispMax);

      float min0 = std::min(a0, b0);
      float min1 = std::min(a1, b1);

      float x0 = mScreenX[i];
      float x1 = mScreenX[i + 1];
      float yA0 = DBToY(a0, coords, bb);
      float yA1 = DBToY(a1, coords, bb);
      float yMin0 = DBToY(min0, coords, bb);
      float yMin1 = DBToY(min1, coords, bb);

      ImVec2 quad[4] = { {x0, yA0}, {x1, yA1}, {x1, yMin1}, {x0, yMin0} };
      dl->AddConvexPolyFilled(quad, 4, mOverlay.colorInAOnly);
    }

    // Pass 3: draw B-only (B above A)
    for (int i = 0; i + 1 < TSegmentCount; ++i) {
      if (!mValid[i] || !mValid[i + 1]) continue;
      bool bAbove = (mB[i] > mA[i]) || (mB[i + 1] > mA[i + 1]);
      if (!bAbove) continue;

      float a0 = M7::math::clamp(mA[i], dispMin, dispMax);
      float a1 = M7::math::clamp(mA[i + 1], dispMin, dispMax);
      float b0 = M7::math::clamp(mB[i], dispMin, dispMax);
      float b1 = M7::math::clamp(mB[i + 1], dispMin, dispMax);

      float min0 = std::min(a0, b0);
      float min1 = std::min(a1, b1);

      float x0 = mScreenX[i];
      float x1 = mScreenX[i + 1];
      float yB0 = DBToY(b0, coords, bb);
      float yB1 = DBToY(b1, coords, bb);
      float yMin0 = DBToY(min0, coords, bb);
      float yMin1 = DBToY(min1, coords, bb);

      ImVec2 quad[4] = { {x0, yB0}, {x1, yB1}, {x1, yMin1}, {x0, yMin0} };
      dl->AddConvexPolyFilled(quad, 4, mOverlay.colorInBOnly);
    }
  }
};

//=============================================================================
// Flat diff layer: render A-B around a zero line (unity line), fill above/below
//=============================================================================
struct FFTDiffFlatOverlay {
  const WaveSabreCore::IFrequencyAnalysis* sourceA = nullptr;
  const WaveSabreCore::IFrequencyAnalysis* sourceB = nullptr;

  // Colors reused for sign semantics (positive=A-only, negative=B-only)
  ImColor colorPositive = ColorFromHTML("449999", 0.50f); // A > B
  ImColor colorNegative = ColorFromHTML("cc4444", 0.50f); // B > A
  ImColor unityLineColor = ColorFromHTML("cccccc", 0.7f);
  float   unityLineThickness = 1.5f;

  bool drawUnityLine = true;
  bool enabled = true;
};

template <int TSegmentCount>
class FFTDiffFlatLayer : public IFrequencyGraphLayer {
private:
  FFTDiffFlatOverlay mOverlay{};
  bool mHasOverlay = false;

  std::vector<float> mFrequencies;
  std::vector<float> mScreenX;
  std::array<float, TSegmentCount> mDelta{}; // A - B in dB
  std::array<bool,  TSegmentCount> mValid{};

  // Symmetric default for diff visualization; configurable
  float mDisplayMinDB = -24.0f;
  float mDisplayMaxDB = +24.0f;
  bool  mUseIndependentScale = true;

  float MapToY(float value, float minV, float maxV, const ImRect& bb) const {
    const float t01 = (maxV - minV) != 0.0f ? M7::math::clamp01((value - minV) / (maxV - minV)) : 0.0f;
    return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
  }

  float ValToY(float dB, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
    if (mUseIndependentScale) return MapToY(dB, mDisplayMinDB, mDisplayMaxDB, bb);
    return coords.DBToY(dB, bb);
  }

public:
  FFTDiffFlatLayer() {
    mFrequencies.reserve(TSegmentCount);
    mScreenX.reserve(TSegmentCount);
  }

  void SetOverlay(const FFTDiffFlatOverlay& overlay) {
    mOverlay = overlay;
    mHasOverlay = (overlay.enabled && overlay.sourceA && overlay.sourceB);
  }
  void ClearOverlay() { mHasOverlay = false; mOverlay = FFTDiffFlatOverlay{}; }

  void SetScaling(float minDB, float maxDB, bool independent = true) {
    mDisplayMinDB = minDB; mDisplayMaxDB = maxDB; mUseIndependentScale = independent;
  }

  bool InfluencesAutoScale() const override { return false; }

  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);

    if (!mHasOverlay) { for (int i = 0; i < TSegmentCount; ++i) mValid[i] = false; return; }

    const auto* a = mOverlay.sourceA;
    const auto* b = mOverlay.sourceB;

    const float nyquistA = a ? a->GetNyquistFrequency() : 0.0f;
    const float nyquistB = b ? b->GetNyquistFrequency() : 0.0f;
    const float binResA  = a ? a->GetFrequencyResolution() : 0.0f;
    const float binResB  = b ? b->GetFrequencyResolution() : 0.0f;

    for (int i = 0; i < TSegmentCount; ++i) {
      float f = mFrequencies[i];
      bool valid = (a && b && f > 0.0f);
      if (valid && nyquistA > 0.0f && binResA > 0.0f && f >= nyquistA) f = std::max(0.0f, nyquistA - 0.5f * binResA);
      if (valid && nyquistB > 0.0f && binResB > 0.0f && f >= nyquistB) f = std::max(0.0f, nyquistB - 0.5f * binResB);

      if (valid) {
        float aDB = a->GetMagnitudeAtFrequency(f);
        float bDB = b->GetMagnitudeAtFrequency(f);
        mDelta[i] = aDB - bDB;
        mValid[i] = true;
      } else {
        mDelta[i] = 0.0f;
        mValid[i] = false;
      }
    }
  }

  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    if (!mHasOverlay) return;

    const float zeroY = ValToY(0.0f, coords, bb);

    // Draw unity/zero line across graph
    if (mOverlay.drawUnityLine) {
      dl->AddLine({bb.Min.x, zeroY}, {bb.Max.x, zeroY}, mOverlay.unityLineColor, mOverlay.unityLineThickness);
    }

    const float dispMin = mUseIndependentScale ? mDisplayMinDB : coords.mDisplayMinDB;
    const float dispMax = mUseIndependentScale ? mDisplayMaxDB : coords.mDisplayMaxDB;

    // Fill positive and negative regions relative to zero, splitting at zero crossing
    for (int i = 0; i + 1 < TSegmentCount; ++i) {
      if (!mValid[i] || !mValid[i + 1]) continue;

      float d0 = M7::math::clamp(mDelta[i],     dispMin, dispMax);
      float d1 = M7::math::clamp(mDelta[i + 1], dispMin, dispMax);

      float x0 = mScreenX[i];
      float x1 = mScreenX[i + 1];
      float y0 = ValToY(d0, coords, bb);
      float y1 = ValToY(d1, coords, bb);

      const bool s0p = d0 > 0.0f;
      const bool s1p = d1 > 0.0f;
      const bool s0n = d0 < 0.0f;
      const bool s1n = d1 < 0.0f;

      // No crossing: draw a single convex quad
      if ((s0p && s1p)) {
        // Positive quad: top-left, top-right, bottom-right, bottom-left (clockwise)
        dl->AddQuadFilled({x0, y0}, {x1, y1}, {x1, zeroY}, {x0, zeroY}, mOverlay.colorPositive);
      } else if ((s0n && s1n)) {
        // Negative quad: top-left, top-right, bottom-right, bottom-left (clockwise)
        dl->AddQuadFilled({x0, zeroY}, {x1, zeroY}, {x1, y1}, {x0, y0}, mOverlay.colorNegative);
      } else if ((s0p && s1n) || (s0n && s1p)) {
        // Crossing: split into two triangles at zero
        const float denom = (d0 - d1);
        float t = denom != 0.0f ? (d0 / denom) : 0.5f; // fraction from x0->x1 where delta crosses zero
        t = M7::math::clamp01(t);
        const float xC = M7::math::lerp(x0, x1, t);
        const float yC = zeroY;

        if (s0p && s1n) {
          // Left positive triangle (clockwise: top-left -> top-right -> bottom-left)
          dl->AddTriangleFilled({x0, y0}, {xC, yC}, {x0, zeroY}, mOverlay.colorPositive);
          // Right negative triangle (clockwise: top-left -> top-right -> bottom-right)
          dl->AddTriangleFilled({xC, yC}, {x1, zeroY}, {x1, y1}, mOverlay.colorNegative);
        } else { // s0n && s1p
          // Left negative triangle (clockwise: top-left -> top-right -> bottom-left)
          dl->AddTriangleFilled({x0, zeroY}, {xC, yC}, {x0, y0}, mOverlay.colorNegative);
          // Right positive triangle (clockwise: top-left -> top-right -> bottom-right)
          dl->AddTriangleFilled({xC, yC}, {x1, y1}, {x1, zeroY}, mOverlay.colorPositive);
        }
      } else {
        // One endpoint is exactly zero, draw a thin triangle/line on the non-zero side only
        if (s0p || s1p) {
          // Positive side
          if (s0p) dl->AddTriangleFilled({x0, y0}, {x1, zeroY}, {x0, zeroY}, mOverlay.colorPositive);
          else     dl->AddTriangleFilled({x0, zeroY}, {x1, y1}, {x1, zeroY}, mOverlay.colorPositive);
        } else if (s0n || s1n) {
          // Negative side
          if (s0n) dl->AddTriangleFilled({x0, zeroY}, {x1, zeroY}, {x0, y0}, mOverlay.colorNegative);
          else     dl->AddTriangleFilled({x0, zeroY}, {x1, zeroY}, {x1, y1}, mOverlay.colorNegative);
        }
      }
    }
  }
};

#endif

} // namespace WaveSabreVstLib