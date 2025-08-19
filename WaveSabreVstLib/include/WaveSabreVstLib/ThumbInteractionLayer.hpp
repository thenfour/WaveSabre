#pragma once

#include "FrequencyMagnitudeGraph.hpp"
#include "Maj7VstUtils.hpp"
#include "FrequencyDomainMath.hpp"

namespace WaveSabreVstLib {

//=============================================================================
// Thumb/control point interaction layer
//=============================================================================
template <size_t TFilterCount, size_t TParamCount>
class ThumbInteractionLayer : public IFrequencyGraphLayer {
private:
  // Thumb rendering and interaction data
  struct ThumbRenderInfo {
    const char* color;
    ImVec2 point;
    const char* label;
    int filterIndex = -1;           // Index into the filters array
    bool isInteractive = false;     // Whether this thumb supports dragging
    ImRect primaryHitTestRect;             // Screen-space hit test area (slightly larger than visual thumb)
    ImRect secondaryHitTestRect;             // Screen-space hit test area -- if no primaries are hit, then go to secondary. allows precise selection among large hit test rects
  };

  // Thumb interaction state management
  struct ThumbInteractionState {
    int activeThumbIndex = -1;      // Which thumb is being dragged (-1 = none)
    ImVec2 dragStartMousePos;       // Mouse position when drag started
    ImVec2 dragStartThumbPos;       // Thumb position when drag started
    float originalFreq = 0.0f;      // Original frequency when drag started
    float originalGain = 0.0f;      // Original gain when drag started (derived from response)
    bool isDragging = false;        // Currently in drag operation
    bool wasHovered = false;        // Was hovered last frame (for hover state tracking)
  };
  
  std::array<FrequencyResponseRendererFilter, TFilterCount> mFilters;
  std::vector<ThumbRenderInfo> mThumbs;
  ThumbInteractionState mThumbInteraction;
  float mThumbRadius = 6.0f;
  
public:
  ThumbInteractionLayer(const std::array<FrequencyResponseRendererFilter, TFilterCount>& filters, float thumbRadius = 6.0f)
    : mFilters(filters), mThumbRadius(thumbRadius) {
  }
  
  void SetThumbRadius(float radius) { mThumbRadius = radius; }
  void SetFilters(const std::array<FrequencyResponseRendererFilter, TFilterCount>& filters) {
    mFilters = filters;
  }
  
  bool InfluencesAutoScale() const override { 
    return false; // Let EQ layer drive bounds; graph prevents shrink while interacting
  }
  
  void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    // Calculate thumb positions from filter parameters
    mThumbs.clear();
    
    for (size_t filterIdx = 0; filterIdx < mFilters.size(); ++filterIdx) {
      const auto &f = mFilters[filterIdx];
      if (!f.filter) continue;
      
      float freq = f.filter->freq;
      float magLin = f.filter->GetMagnitudeAtFrequency(freq);
      float magdB = M7::math::LinearToDecibels(magLin);

      ThumbRenderInfo thumbInfo;
      thumbInfo.color = f.thumbColor;
      thumbInfo.point = {coords.FreqToX(freq, bb), coords.DBToY(magdB, bb)};
      thumbInfo.label = f.label;
      thumbInfo.filterIndex = (int)filterIdx;
      thumbInfo.isInteractive = (f.HandleChangeParam != nullptr);
      
      // Create hit test area (slightly larger than visual thumb)
      float hitRadius = mThumbRadius + 3.0f;
      thumbInfo.primaryHitTestRect = ImRect(
        thumbInfo.point.x - hitRadius, thumbInfo.point.y - hitRadius,
        thumbInfo.point.x + hitRadius, thumbInfo.point.y + hitRadius
      );

      // a hit test rect which is the "column" containing the thumb
      ImRect thumbColumnRect = ImRect(
        thumbInfo.point.x - mThumbRadius, bb.Min.y,
        thumbInfo.point.x + mThumbRadius, bb.Max.y
	  );

      thumbInfo.secondaryHitTestRect = thumbColumnRect;
      
      mThumbs.push_back(thumbInfo);
    }
  }
  
  bool HandleMouse(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseClicked = ImGui::IsMouseClicked(0);
    bool mouseDown = ImGui::IsMouseDown(0);
    bool mouseReleased = ImGui::IsMouseReleased(0);
    bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    
    // Check for mouse wheel events
    float mouseWheel = ImGui::GetIO().MouseWheel;
    
    // Handle mouse wheel scroll for Q parameter when hovering over a thumb
    if (mouseInBounds && mouseWheel != 0.0f) {
      int thumbIndex = FindThumbUnderMouse(mousePos);
      if (thumbIndex >= 0) {
        const auto& thumb = mThumbs[thumbIndex];
        const auto& filter = mFilters[thumb.filterIndex];
        
        if (filter.HandleChangeQ) {
          // Get current Q value from the filter
          float currentQ = filter.filter ? filter.filter->q : 1.0f;
          
          // Apply wheel delta to Q with reasonable scaling and limits
          float qDelta = mouseWheel * 0.1f; // Fine control
          float newQ = currentQ * (1.0f + qDelta);
          
          // Clamp Q to reasonable limits (typical range for audio filters)
          newQ = M7::math::clamp(newQ, 0.1f, 20.0f);
          
          // Call the Q parameter change handler
          filter.HandleChangeQ(newQ, filter.userData);
          
          return true; // Consumed the event
        }
      }
    }

    // Handle drag end
    if (mThumbInteraction.isDragging && (mouseReleased || !mouseDown)) {
      mThumbInteraction.isDragging = false;
      mThumbInteraction.activeThumbIndex = -1;
      return true;
    }

    // Handle drag start
    if (mouseClicked && mouseInBounds && !mThumbInteraction.isDragging) {
      int thumbIndex = FindThumbUnderMouse(mousePos);
      if (thumbIndex >= 0) {
        const auto& thumb = mThumbs[thumbIndex];
        mThumbInteraction.activeThumbIndex = thumbIndex;
        mThumbInteraction.isDragging = true;
        mThumbInteraction.dragStartMousePos = mousePos;
        mThumbInteraction.dragStartThumbPos = thumb.point;
        mThumbInteraction.originalFreq = coords.XToFreq(thumb.point.x, bb);
        mThumbInteraction.originalGain = coords.YToDB(thumb.point.y, bb);
        return true;
      }
    }

    // Handle active drag
    if (mThumbInteraction.isDragging && mThumbInteraction.activeThumbIndex >= 0) {
      auto& activeThumb = mThumbs[mThumbInteraction.activeThumbIndex];
      const auto& filter = mFilters[activeThumb.filterIndex];
      
      if (filter.HandleChangeParam) {
        // Calculate new frequency and gain from mouse position
        ImVec2 clampedMouse = {
          M7::math::clamp(mousePos.x, bb.Min.x, bb.Max.x),
          M7::math::clamp(mousePos.y, bb.Min.y, bb.Max.y)
        };
        
        float newFreq = coords.XToFreq(clampedMouse.x, bb);
        float newGain = coords.YToDB(clampedMouse.y, bb);
        
        // Apply reasonable limits
        newFreq = M7::math::clamp(newFreq, 20.0f, 20000.0f);
        newGain = M7::math::clamp(newGain, coords.mDisplayMinDB, coords.mDisplayMaxDB);
        
        // Call the parameter change handler
        filter.HandleChangeParam(newFreq, newGain, filter.userData);
        
        return true;
      }
    }
    
    return false; // Didn't consume any mouse events
  }
  
  void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
    // Render thumbs with interaction feedback
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
    
	auto hoveredThumb = FindThumbUnderMouse(mousePos);

    for (int i = 0; i < (int)mThumbs.size(); ++i) {
      const auto &th = mThumbs[i];
      
      // Determine thumb visual state
      bool isActive = (mThumbInteraction.isDragging && mThumbInteraction.activeThumbIndex == i);
      //bool isHovered = th.isInteractive && HitTestCircle(mousePos, th.point, mThumbRadius + 3.0f) && 
      //                mouseInBounds && !mThumbInteraction.isDragging;
	  bool isHovered = (hoveredThumb == i) && th.isInteractive && mouseInBounds && !mThumbInteraction.isDragging;
      
      // Draw thumb with state-based appearance
      ImU32 thumbColor = ColorFromHTML(th.color, isActive ? 1.0f : 0.8f);
      float thumbRadius = mThumbRadius * (isActive ? 1.1f : (isHovered ? 1.05f : 1.0f));
      
      dl->AddCircleFilled(th.point, thumbRadius, thumbColor);
      dl->AddCircle(th.point, thumbRadius + 1, ColorFromHTML("000000"), 0, 1.5f);
      
      // Additional visual feedback for interactive thumbs
      if (th.isInteractive) {
        if (isHovered && !isActive) {
          // Subtle glow for hover
          dl->AddCircle(th.point, thumbRadius + 2, ColorFromHTML(th.color, 0.3f), 0, 1.0f);
        } else if (isActive) {
          // Bright outline for active drag
          dl->AddCircle(th.point, thumbRadius + 2, ColorFromHTML("ffffff", 0.8f), 0, 2.0f);
        }
      }
      
      // Draw label if available
      if (th.label) {
        ImVec2 textSize = ImGui::CalcTextSize(th.label);
        ImVec2 textPos = { th.point.x - textSize.x * 0.5f, th.point.y - textSize.y * 0.5f };
        dl->AddText(textPos, ColorFromHTML("000000"), th.label);
      }
    }
  }
  
  // For tooltip rendering - expose the thumb information
  const std::vector<ThumbRenderInfo>& GetThumbs() const { return mThumbs; }
  const ThumbInteractionState& GetInteractionState() const { return mThumbInteraction; }
  
  // Public accessor for finding thumb under mouse
  int FindThumbUnderMouse(ImVec2 mousePos) const { 
      for (int i = 0; i < (int)mThumbs.size(); ++i) {
          const auto& thumb = mThumbs[i];
          if (!thumb.isInteractive) continue;
          if (thumb.primaryHitTestRect.Contains(mousePos)) {
              return i;
          }
      }
      for (int i = 0; i < (int)mThumbs.size(); ++i) {
          const auto& thumb = mThumbs[i];
          if (!thumb.isInteractive) continue;
          if (thumb.secondaryHitTestRect.Contains(mousePos)) {
              return i;
          }
      }
      return -1;
  }
};

} // namespace WaveSabreVstLib