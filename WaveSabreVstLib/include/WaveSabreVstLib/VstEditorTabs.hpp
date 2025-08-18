#ifndef __WAVESABREVSTLIB_VSTEDITORTABS_H__
#define __WAVESABREVSTLIB_VSTEDITORTABS_H__

#include "Common.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>

using namespace WaveSabreCore;

namespace WaveSabreVstLib {

struct ImGuiTabSelectionHelper {
  int *mpSelectedTabIndex = nullptr; // pointer to the setting to read and
                                     // store the current tab index
  int mImGuiSelection = 0; // which ID does ImGui currently have selected
};

// Tab bar helper functions that would be methods in VstEditor
inline bool BeginTabBar2(const char *str_id, ImGuiTabBarFlags flags,
                         ImRect& tabBarBB, ImU32& tabBarStoredSeparatorColor,
                         float width = 0) {
  tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;
  ImGuiID id = window->GetID(str_id);
  ImGuiTabBar *tab_bar = g.TabBars.GetOrAddByKey(id);
  
  // Copied from imgui with changes:
  // 1. shrinking workrect max for column support
  // 2. adding X cursor pos for side-by-side positioning
  // 3. pushing empty separator color so BeginTabBarEx doesn't display a
  // separator line (we'll do it later).
  if (width == 0) {
    tabBarBB = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y,
                      window->DC.CursorPos.x + (window->WorkRect.Max.x),
                      window->DC.CursorPos.y + g.FontSize +
                          g.Style.FramePadding.y * 2);
  } else {
    tabBarBB = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y,
                      window->DC.CursorPos.x + width,
                      window->DC.CursorPos.y + g.FontSize +
                          g.Style.FramePadding.y * 2);
  }
  tab_bar->ID = id;
  ImGui::PushStyleColor(ImGuiCol_TabActive, {});
  bool ret = ImGui::BeginTabBarEx(tab_bar, tabBarBB,
                                  flags | ImGuiTabBarFlags_IsFocused);
  ImGui::PopStyleColor(1);
  return ret;
}

inline bool WSBeginTabItem(const char *label, ImU32& tabBarStoredSeparatorColor,
                          bool *p_open = 0, ImGuiTabItemFlags flags = 0) {
  if (ImGui::BeginTabItem(label, p_open, flags)) {
    tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
    return true;
  }
  return false;
}

// I HATE THIS LOGIC.
// there's a lot of plumbing in the ImGuiTabBar stuff, so i don't want to mess
// with that to manipulate selection there's a flag,
// ImGuiTabItemFlags_SetSelected , which lets us manually select a tab. but
// there are scenarios we have to specifically account for:
// - 1st frame (think closing & reopening the editor window), we need to make
// sure our ImGui state shows selection of 0.
// - upon loading a new patch make sure we're careful about responding to the
// selected tab.
inline bool WSBeginTabItemWithSel(const char *label, int thisTabIndex,
                                 ImGuiTabSelectionHelper &helper,
                                 ImU32& tabBarStoredSeparatorColor) {
  // when losing and regaining focus, ImGui will reset its selection to 0, but
  // we may think it's something else. so have to specially check for that
  // situation.
  if (GImGui->CurrentTabBar->SelectedTabId == 0) {
    helper.mImGuiSelection = 0;
  }

  // between BeginTabBar() and EndTabBar(), need to know when BeginTabItem()
  // returns true whether that means the user selected a new tab, or ImGui is
  // just rendering the incorrect tab because we haven't reached the selected
  // one yet. latter case make sure we don't misinterpret the result of
  // BeginTabItem().
  bool waitingForCorrectTab = false;
  int flags = 0;
  if (helper.mImGuiSelection != *helper.mpSelectedTabIndex) {
    waitingForCorrectTab = true;
    if (thisTabIndex == *helper.mpSelectedTabIndex) {
      flags |= ImGuiTabItemFlags_SetSelected;
    }
  }

  if (ImGui::BeginTabItem(label, 0, flags)) {
    tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);

    helper.mImGuiSelection = thisTabIndex;

    if (GImGui->CurrentTabBar->SelectedTabId != 0 &&
        (*helper.mpSelectedTabIndex != thisTabIndex)) {
      if (!waitingForCorrectTab) {
        *helper.mpSelectedTabIndex = thisTabIndex;
      }
    }

    return true;
  }
  return false;
}

inline void EndTabBarWithColoredSeparator(const ImRect& tabBarBB, 
                                         ImU32 tabBarStoredSeparatorColor) {
  ImGuiContext &g = *GImGui;
  ImGuiWindow *window = g.CurrentWindow;
  ImGuiTabBar *tab_bar = g.CurrentTabBar;
  const ImU32 col = tabBarStoredSeparatorColor;
  const float y = tab_bar->BarRect.Max.y - 1.0f;

  // account for the popup indicator arrow thingy.
  float separator_min_x = std::min(tab_bar->BarRect.Min.x, tabBarBB.Min.x) -
                          M7::math::floor(window->WindowPadding.x * 0.5f);

  // this value is too big; there's some huge space at the right of all tabs.
  float separator_max_x = tab_bar->BarRect.Max.x +
                          M7::math::floor(window->WindowPadding.x * 0.5f);

  ImRect body_bb;
  body_bb.Min = {separator_min_x, y};
  body_bb.Max = {separator_max_x, window->DC.CursorPos.y};

  ImColor c2 = {col};
  float h, s, v;
  ImGui::ColorConvertRGBtoHSV(c2.Value.x, c2.Value.y, c2.Value.z, h, s, v);
  auto halfAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.45f);
  auto lowAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.15f);

  ImGui::GetBackgroundDrawList()->AddRectFilled(
      body_bb.Min, body_bb.Max, lowAlpha, 2, ImDrawFlags_RoundCornersAll);
  ImGui::GetBackgroundDrawList()->AddRect(body_bb.Min, body_bb.Max, halfAlpha,
                                          2, ImDrawFlags_RoundCornersAll);

  ImGui::EndTabBar();

  // add some bottom margin.
  ImGui::Dummy({0, 6});
}

} // namespace WaveSabreVstLib

#endif
