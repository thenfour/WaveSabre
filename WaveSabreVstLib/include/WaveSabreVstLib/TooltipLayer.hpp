#pragma once

#include <functional>

#include "FrequencyMagnitudeGraph.hpp"
#include "ThumbInteractionLayer.hpp"
#include "Maj7VstUtils.hpp"

namespace WaveSabreVstLib {

// Forward declarations to avoid circular dependencies
template <int TSegmentCount, size_t TFilterCount, size_t TParamCount>
class EQResponseLayer;

//=============================================================================
// Tooltip layer - handles hover information display
//=============================================================================
template <size_t TFilterCount, size_t TParamCount>
class TooltipLayer : public IFrequencyGraphLayer {
private:
  // Pointer to the thumb layer for getting thumb information
  const ThumbInteractionLayer<TFilterCount, TParamCount>* mThumbLayer = nullptr;
  
  // Pointer to the EQ response layer for getting curve data (use void* to avoid template issues)
  const void* mEQLayer = nullptr;

  // Type-erased evaluator for querying EQ response at a given screen X
  // Returns true when the response is available and writes dB to outDB.
  std::function<bool(float, const FrequencyMagnitudeCoordinateSystem&, const ImRect&, float&)> mEvalAtX;
  
public:
  TooltipLayer() = default;
  
  void SetThumbLayer(const ThumbInteractionLayer<TFilterCount, TParamCount>* thumbLayer) {
    mThumbLayer = thumbLayer;
  }
  
  template <int TSegmentCount>
  void SetEQLayer(const EQResponseLayer<TSegmentCount, TFilterCount, TParamCount>* eqLayer) {
    mEQLayer = eqLayer;
    if (eqLayer) {
      mEvalAtX = [eqLayer](float mx, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, float& outDB) -> bool {
        return eqLayer->EvaluateAtX(mx, coords, bb, outDB);
      };
    } else {
      mEvalAtX = {};
    }
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Tooltips don't need data updates
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    
    if (!mouseInBounds) return;
    
    // Check if we have thumb layer and if mouse is over a thumb
    bool showingThumbTooltip = false;
    if (mThumbLayer && !mThumbLayer->GetInteractionState().isDragging) {
      int hoveredThumb = mThumbLayer->FindThumbUnderMouse(mousePos);
      
      if (hoveredThumb >= 0) {
        const auto& thumbs = mThumbLayer->GetThumbs();
        const auto& thumb = thumbs[hoveredThumb];
        
        float freq = coords.XToFreq(thumb.point.x, bb);
        float gain = coords.YToDB(thumb.point.y, bb);
        
        ImGui::BeginTooltip();
        if (thumb.label) {
          ImGui::Text("%s", thumb.label);
        }
        ImGui::Text("%.0f Hz", freq);
        ImGui::Text("%.1f dB", gain);
        
        if (thumb.isInteractive) {
          ImGui::TextDisabled("Click & drag to adjust freq/gain");
          ImGui::TextDisabled("Mouse wheel to adjust Q");
        }
        ImGui::EndTooltip();
        
        showingThumbTooltip = true;
      }
    }
    
    // Show frequency response tooltip if not hovering over thumb
    if (!showingThumbTooltip) {
      ImVec2 clampedMouse = { 
        M7::math::clamp(mousePos.x, bb.Min.x, bb.Max.x), 
        M7::math::clamp(mousePos.y, bb.Min.y, bb.Max.y) 
      };

      // Draw crosshair lines
      ImU32 crossColH = ColorFromHTML("ff8800", 0.25f);
      ImU32 crossColV = ColorFromHTML("00dddd", 0.25f);
      dl->AddLine({clampedMouse.x, bb.Min.y}, {clampedMouse.x, bb.Max.y}, crossColV, 1.0f);
      dl->AddLine({bb.Min.x, clampedMouse.y}, {bb.Max.x, clampedMouse.y}, crossColH, 1.0f);

      // Evaluate curve value if EQ layer is available
      float magDBLerp = 0.0f;
      bool hasCurve = false;
      if (mEvalAtX) {
        hasCurve = mEvalAtX(clampedMouse.x, coords, bb, magDBLerp);
      }

      if (hasCurve) {
        float hoverFreq = coords.XToFreq(clampedMouse.x, bb);

        // Indicator on the response curve
        ImVec2 curvePt = { clampedMouse.x, coords.DBToY(magDBLerp, bb) };
        ImU32 indicatorFill = ColorFromHTML("00dddd", 0.95f);
        ImU32 indicatorOutline = ColorFromHTML("000000", 0.9f);
        ImU32 ptCrossCol = ColorFromHTML("00dddd", 0.25f);
        
        dl->AddLine({ bb.Min.x, curvePt.y }, { bb.Max.x, curvePt.y }, ptCrossCol, 1.0f);
        dl->AddCircleFilled(curvePt, 4.0f, indicatorFill);
        dl->AddCircle(curvePt, 4.5f, indicatorOutline, 0, 1.5f);

        // Build tooltip text
        char freqText[32];
        if (hoverFreq >= 1000.0f) {
          snprintf(freqText, sizeof(freqText), "%.2fkHz", hoverFreq / 1000.0f);
        } else {
          snprintf(freqText, sizeof(freqText), "%.0fHz", hoverFreq);
        }

        ImVec2 tipOffset = { 8.0f, -8.0f };
        ImVec2 tipAnchor = { curvePt.x + tipOffset.x, curvePt.y + tipOffset.y };
        ImGui::SetNextWindowPos(tipAnchor, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
        ImGui::BeginTooltip();
        ImGui::Text("%s", freqText);
        ImGui::Text("%.2f dB", magDBLerp);
        ImGui::EndTooltip();
      }
    }
  }
};

} // namespace WaveSabreVstLib