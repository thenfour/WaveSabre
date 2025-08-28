#ifndef __WAVESABREVSTLIB_VSTEDITOR_H__
#define __WAVESABREVSTLIB_VSTEDITOR_H__

#include "Common.h"
#include <d3d9.h>
#include <functional>
#include <queue>
#include <string>
#include <vector>

#include "VstPlug.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "./FreqMagnitudeGraph/FrequencyResponseRenderer.hpp"

// Include the split header files
#include "CompressorTransferCurve.hpp"
#include "HistoryVisualization.hpp"
#include "Maj7EditorComponents.hpp"
#include "Maj7ParamConverters.hpp"
#include "Maj7VstUtils.hpp"
#include "VUMeter.hpp"
#include "VstEditorTabs.hpp"


using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{


struct IVstSerializableParam
{
  virtual void Serialize(clarinoid::JsonVariantWriter& out) = 0;
  virtual bool TryDeserialize(clarinoid::JsonVariantReader& inp) = 0;
};

struct VstSerializableBoolParamRef : public IVstSerializableParam
{
  const char* mKey;
  bool& mValue;
  VstSerializableBoolParamRef(const char* key, bool& value)
      : mKey(key)
      , mValue(value)
  {
  }
  void Serialize(clarinoid::JsonVariantWriter& elem) override
  {
    elem.Object_MakeKey(mKey).WriteBoolean(mValue);
  }
  bool TryDeserialize(clarinoid::JsonVariantReader& elem) override
  {
    if (elem.mKeyName == mKey)
    {
      mValue = elem.mBooleanValue;
      return true;
    }
    return false;
  }
};

template <typename T>
struct VstSerializableIntParamRef : public IVstSerializableParam
{
  const char* mKey;
  T& mValue;
  VstSerializableIntParamRef(const char* key, T& value)
      : mKey(key)
      , mValue(value)
  {
  }
  void Serialize(clarinoid::JsonVariantWriter& elem) override
  {
    elem.Object_MakeKey(mKey).WriteNumberValue(mValue);
  }
  bool TryDeserialize(clarinoid::JsonVariantReader& elem) override
  {
    if (elem.mKeyName == mKey)
    {
      mValue = elem.mNumericValue.Get<T>();
      return true;
    }
    return false;
  }
};


class VstEditor : public AEffEditor
{
public:
  char mFilterInputText[200] = {0};
  ImRect tabBarBB;
  ImU32 tabBarStoredSeparatorColor;

  bool BeginTabBar2(const char* str_id, ImGuiTabBarFlags flags, float width = 0)
  {
    return WaveSabreVstLib::BeginTabBar2(str_id, flags, tabBarBB, tabBarStoredSeparatorColor, width);
  }

  bool WSBeginTabItem(const char* label, bool* p_open = 0, ImGuiTabItemFlags flags = 0)
  {
    return WaveSabreVstLib::WSBeginTabItem(label, tabBarStoredSeparatorColor, p_open, flags);
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
  bool WSBeginTabItemWithSel(const char* label, int thisTabIndex, ImGuiTabSelectionHelper& helper)
  {
    return WaveSabreVstLib::WSBeginTabItemWithSel(label, thisTabIndex, helper, tabBarStoredSeparatorColor);
  }

  void EndTabBarWithColoredSeparator()
  {
    WaveSabreVstLib::EndTabBarWithColoredSeparator(tabBarBB, tabBarStoredSeparatorColor);
  }

  void IndicatorDot(const char* colorHTML, const char* caption = nullptr)
  {
    auto c = ImGui::GetCursorScreenPos();
    ImRect bb = {c, c + ImVec2{20, 20}};
    auto* dl = ImGui::GetWindowDrawList();
    dl->AddCircleFilled(c, 6, ColorFromHTML(colorHTML, 0.8f));
    if (caption)
    {
      dl->AddText(c + ImVec2{8, 8}, ColorFromHTML(colorHTML, 0.8f), caption);
    }
  }

  VstEditor(AudioEffect* audioEffect, int width, int height);
  virtual ~VstEditor();

  // parameters to be serialized by the VST, but not by the device (display settings etc)
  virtual std::vector<IVstSerializableParam*> GetVstOnlyParams()
  {
    return {};
  }

  virtual void Window_Open(HWND parent);
  virtual void Window_Close();

  virtual bool getRect(ERect** ppRect)
  {
    *ppRect = &mDefaultRect;
    return true;
  }

  virtual void renderImgui() = 0;
  virtual void PopulateMenuBar() {};  // can override to add menu items
  virtual std::string GetMenuBarStatusText()
  {
    return {};
  };

  bool open(void* ptr) override
  {
    AEffEditor::open(ptr);
    Window_Open((HWND)ptr);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    GetEffectX()->getDevice()->mGuiVisible.store(true, std::memory_order_relaxed);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    return true;
  }

  void close() override
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    GetEffectX()->getDevice()->mGuiVisible.store(false, std::memory_order_relaxed);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    Window_Close();
    AEffEditor::close();
  }

  bool WSImGuiParamKnob(VstInt32 id, const char* name, const char* fmt = "%.3f")
  {
    float paramValue = GetEffectX()->getParameter((VstInt32)id);
    bool r = false;
    r = ImGuiKnobs::Knob(
        name, &paramValue, 0, 1, 0.5f, 0, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly);
    if (r)
    {
      GetEffectX()->setParameterAutomated(id, M7::math::clamp01(paramValue));
    }
    return r;
  }

  std::string GetRenderedTextFromID(const char* id)
  {
    auto end = ImGui::FindRenderedTextEnd(id, 0);
    return std::string(id, end);
  }

  // for boolean types
  bool WSImGuiParamCheckbox(VstInt32 paramId, const char* id)
  {
    ImGui::BeginGroup();
    ImGui::PushID(id);
    float paramValue = GetEffectX()->getParameter((VstInt32)paramId);
    M7::QuickParam p{paramValue};

    bool bval = p.GetBoolValue();

    bool r = false;
    ImGui::Text(GetRenderedTextFromID(id).c_str());
    r = ImGui::Checkbox("##cb", &bval);
    if (r)
    {
      p.SetBoolValue(bval);
      GetEffectX()->setParameterAutomated(paramId, p.GetRawValue());
    }
    ImGui::PopID();
    ImGui::EndGroup();
    return r;
  }

  template <typename TParamID>
  void Maj7ImGuiBoolParamToggleButton(TParamID paramID,
                                      const char* label,
                                      ImVec2 size = {},
                                      const ButtonColorSpec& cfg = {})
  {
    bool ret = false;
    float backing = GetEffectX()->getParameter((VstInt32)paramID);
    M7::ParamAccessor p{&backing, 0};
    bool value = p.GetBoolValue(0);

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

    if (ToggleButton(&value, label, size, cfg))
    {
      p.SetBoolValue(0, value);
      GetEffectX()->setParameterAutomated((VstInt32)paramID, backing);
    }

    ImGui::PopStyleVar(2);
  }

  template <typename TparamID>
  void Maj7ImGuiBoolParamToggleButton(TparamID paramID, const char* label, const char* selectedColorHTML)
  {
    Maj7ImGuiBoolParamToggleButton(paramID, label, {}, ButtonColorSpec{selectedColorHTML});
  }

  void WSImGuiParamEnumList(VstInt32 paramID,
                            const char* ctrlLabel,
                            int elementCount,
                            const char* const* const captions)
  {
    float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
    M7::ParamAccessor pa{&paramValue, 0};
    enum AnyEnum
    {
    };
    auto friendlyVal = pa.GetEnumValue<AnyEnum>(0);  // ParamToEnum(paramValue, elementCount);
        // //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
    int enumIndex = (int)friendlyVal;

    ImGui::PushID(ctrlLabel);
    ImGui::PushItemWidth(70);

    ImGui::BeginGroup();

    auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
    auto txt = std::string(ctrlLabel, end);
    ImGui::Text("%s", txt.c_str());

    if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + elementCount)))
    {
      for (int n = 0; n < elementCount; n++)
      {
        const bool is_selected = (enumIndex == n);
        if (ImGui::Selectable(captions[n], is_selected))
        {
          enumIndex = n;
          pa.SetEnumIntValue(0, n);
          GetEffectX()->setParameterAutomated(paramID, (paramValue));
        }

        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndListBox();
    }

    ImGui::EndGroup();
    ImGui::PopItemWidth();
    ImGui::PopID();
  }

  void WSImGuiParamVoiceMode(VstInt32 paramID, const char* ctrlLabel)
  {
    static constexpr char const* const captions[] = {"Poly", "Mono"};
    WSImGuiParamEnumList(paramID, ctrlLabel, 2, captions);
  }

  void WSImGuiParamFilterType(VstInt32 paramID, const char* ctrlLabel)
  {
    static constexpr char const* const captions[] = {"LP", "HP", "BP", "Notch"};
    WSImGuiParamEnumList(paramID, ctrlLabel, 4, captions);
  }

  // For the Maj7 synth, let's add more param styles.
  template <typename Tenum, typename TparamID, typename Tcount>
  void Maj7ImGuiParamEnumList(TparamID paramID,
                              const char* ctrlLabel,
                              Tcount elementCount,
                              Tenum defaultVal,
                              const char* const* const captions)
  {
    M7::QuickParam p{GetEffectX()->getParameter((VstInt32)paramID)};
    auto friendlyVal = p.GetEnumValue<Tenum>();  // ParamToEnum(paramValue, elementCount);
        // //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
    int enumIndex = (int)friendlyVal;

    ImGui::PushID(ctrlLabel);
    ImGui::PushItemWidth(70);

    ImGui::BeginGroup();

    auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
    auto txt = std::string(ctrlLabel, end);
    ImGui::Text("%s", txt.c_str());

    if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + (int)elementCount)))
    {
      for (int n = 0; n < (int)elementCount; n++)
      {
        const bool is_selected = (enumIndex == n);
        if (ImGui::Selectable(captions[n], is_selected))
        {
          p.SetEnumValue((Tenum)n);
          GetEffectX()->setParameterAutomated((VstInt32)paramID, p.GetRawValue());
        }

        if (is_selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndListBox();
    }

    ImGui::EndGroup();
    ImGui::PopItemWidth();
    ImGui::PopID();
  }

  // copied from BeginComboBox
  bool ComboLookingButton(const char* label, const char* preview_value)
  {
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = ImGui::GetCurrentWindow();

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    const float arrow_size = ImGui::GetFrameHeight();
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const float w = ImGui::CalcItemWidth();
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
    const ImRect total_bb(bb.Min,
                          bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));
    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id, &bb))
      return false;

    // Open on click
    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);
    const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);
    bool popup_open = ImGui::IsPopupOpen(popup_id, ImGuiPopupFlags_None);
    if (pressed && !popup_open)
    {
      // ImGui::OpenPopupEx(popup_id, ImGuiPopupFlags_None);
      popup_open = true;
    }

    // Render shape
    const ImU32 frame_col = ImGui::GetColorU32(hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
    const float value_x2 = ImMax(bb.Min.x, bb.Max.x - arrow_size);
    ImGui::RenderNavHighlight(bb, id);
    window->DrawList->AddRectFilled(
        bb.Min, ImVec2(value_x2, bb.Max.y), frame_col, style.FrameRounding, ImDrawFlags_RoundCornersLeft);

    ImU32 bg_col = ImGui::GetColorU32((popup_open || hovered) ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    ImU32 text_col = ImGui::GetColorU32(ImGuiCol_Text);
    window->DrawList->AddRectFilled(ImVec2(value_x2, bb.Min.y),
                                    bb.Max,
                                    bg_col,
                                    style.FrameRounding,
                                    (w <= arrow_size) ? ImDrawFlags_RoundCornersAll : ImDrawFlags_RoundCornersRight);
    if (value_x2 + arrow_size - style.FramePadding.x <= bb.Max.x)
      ImGui::RenderArrow(window->DrawList,
                         ImVec2(value_x2 + style.FramePadding.y, bb.Min.y + style.FramePadding.y),
                         text_col,
                         ImGuiDir_Down,
                         1.0f);
    ImGui::RenderFrameBorder(bb.Min, bb.Max, style.FrameRounding);

    // Render preview and label
    if (preview_value != NULL)
    {
      if (g.LogEnabled)
        ImGui::LogSetNextTextDecoration("{", "}");
      ImGui::RenderTextClipped(bb.Min + style.FramePadding, ImVec2(value_x2, bb.Max.y), preview_value, NULL, NULL);
    }
    if (label_size.x > 0)
      ImGui::RenderText(ImVec2(bb.Max.x + style.ItemInnerSpacing.x, bb.Min.y + style.FramePadding.y), label);

    if (!popup_open)
      return false;

    return pressed;
  }

  template <typename Tenum, typename TparamID, typename Tcount>
  void Maj7ImGuiParamEnumCombo(TparamID paramID,
                               const char* ctrlLabel,
                               Tcount elementCount,
                               Tenum defaultVal,
                               const char* const* const captions,
                               float width = 120)
  {
    M7::QuickParam p{GetEffectX()->getParameter((VstInt32)paramID)};
    auto friendlyVal = p.GetEnumValue<Tenum>();  // ParamToEnum(paramValue, elementCount);
        // //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
    int enumIndex = (int)friendlyVal;

    CCASSERT(enumIndex >= 0);
    CCASSERT(enumIndex < elementCount);

    const char* elem_name = "ERROR";

    ImGui::PushID(ctrlLabel);
    ImGui::PushItemWidth(width);

    ImGui::BeginGroup();

    auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
    auto txt = std::string(ctrlLabel, end);

    ImGui::SetNextItemWidth(150);
    if (ComboLookingButton(txt.c_str(), captions[enumIndex]))
    {
      ImGui::OpenPopup("##enumlist");
      mFilterInputText[0] = 0;
    }

    if (ImGui::BeginPopup("##enumlist"))
    {
      // set initial focus
      // https://github.com/ocornut/imgui/issues/455
      if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive() &&
          !ImGui::IsMouseClicked(0))
        ImGui::SetKeyboardFocusHere(0);

      ImGui::InputText("##filterText", mFilterInputText, std::size(mFilterInputText));

      ImGui::SameLine();
      if (ImGui::SmallButton("close"))
      {
        ImGui::CloseCurrentPopup();
      }

      std::vector<std::pair<std::string, int>> options;
      for (int n = 0; n < (int)elementCount; n++)
      {
        options.push_back({captions[n], n});
      }

      auto matches = (std::strlen(mFilterInputText) > 0) ? autocomplete(mFilterInputText, options) : options;

      for (auto& option : matches)
      {
        ImGui::PushID(option.second);
        int n = option.second;
        std::string caption = option.first;  // captions[n];

        const bool is_selected = (enumIndex == n);
        if (ImGui::Selectable(caption.c_str(), is_selected))
        {
          p.SetEnumValue((Tenum)n);
          GetEffectX()->setParameterAutomated((VstInt32)paramID, p.GetRawValue());
          ImGui::CloseCurrentPopup();
        }

        ImGui::PopID();
      }
      ImGui::EndPopup();
    }

    ImGui::EndGroup();
    ImGui::PopItemWidth();
    ImGui::PopID();
  }

  template <typename TEnum>
  struct EnumToggleButtonArrayItem
  {
    const char* mCaption;
    ImColor mSelectedColor;
    ImColor mNotSelectedColor;
    ImColor mSelectedHoveredColor;
    ImColor mNotSelectedHoveredColor;
    TEnum mValue;
    TEnum mValueOnDeselect;  // when the user unclicks the value, what should the
                             // underlying param get set to?

    EnumToggleButtonArrayItem() = default;

    EnumToggleButtonArrayItem(const char* caption, TEnum value, const char* primaryColorHtml)
        : mCaption(caption)
        , mSelectedColor(ColorFromHTML(primaryColorHtml))
        , mNotSelectedColor(ColorFromHTML("333333"))
        , mSelectedHoveredColor(ColorFromHTML(primaryColorHtml, 0.8f))
        , mNotSelectedHoveredColor(ColorFromHTML(primaryColorHtml, 0.8f))
        , mValue(value)
        , mValueOnDeselect(value)  // default to no change
    {
    }
  };

  // the point of this is that you can have a different set of buttons than enum
  // options. the idea is that when "none" are selected, you can do something
  // different.
  template <typename Tenum, typename TparamID, size_t Tcount>
  void Maj7ImGuiParamEnumToggleButtonArray(TparamID paramID,
                                           const char* ctrlLabel,
                                           const EnumToggleButtonArrayItem<Tenum> (&itemCfg)[Tcount])
  {
    M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    M7::ParamAccessor pa{&tempVal, 0};
    auto selectedVal = pa.GetEnumValue<Tenum>(0);

    ImGuiGroupScope group{};

    ImGuiScope scope;
    if (ctrlLabel)
    {
      scope.PushID(ctrlLabel);

      auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
      auto txt = std::string(ctrlLabel, end);
      if (txt.length())
      {
        ImGui::Text("%s", txt.c_str());
      }
    }

    scope.PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    scope.PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

    for (size_t i = 0; i < Tcount; i++)
    {
      auto& cfg = itemCfg[i];
      const bool is_selected = (selectedVal == cfg.mValue);

      if (is_selected)
      {
        scope.PushStyleColor(ImGuiCol_Button, (ImVec4)cfg.mSelectedColor);
        scope.PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)cfg.mSelectedColor);
        scope.PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)cfg.mSelectedHoveredColor);
      }
      else
      {
        scope.PushStyleColor(ImGuiCol_Button, (ImVec4)cfg.mNotSelectedColor);
        scope.PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)cfg.mNotSelectedColor);
        scope.PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)cfg.mNotSelectedHoveredColor);
      }

      if (i > 0)
        ImGui::SameLine();
      if (ImGui::Button(cfg.mCaption))
      {
        pa.SetEnumValue(0, is_selected ? cfg.mValueOnDeselect : cfg.mValue);
        GetEffectX()->setParameterAutomated((VstInt32)paramID, tempVal);
      }
    }
  }

  template <typename TEnum>
  struct EnumMutexButtonArrayItem
  {
    const char* caption;
    const char* selectedColor;
    const char* notSelectedColor;
    const char* selectedHoveredColor;
    const char* notSelectedHoveredColor;
    TEnum value;
  };

  // the point of this is that you can have a different set of buttons than enum
  // options. the idea is that you can do something
  // different.
  template <typename Tenum, typename TparamID, size_t Tcount>
  void Maj7ImGuiParamEnumMutexButtonArray(TparamID paramID,
                                          const char* ctrlLabel,
                                          float width,
                                          bool horiz,
                                          const EnumMutexButtonArrayItem<Tenum> (&itemCfg)[Tcount],
                                          int spacing = 0,
                                          int columns = 0)
  {
    const char* defaultSelectedColor = "4400aa";
    const char* defaultNotSelectedColor = "222222";
    const char* defaultSelectedHoveredColor = "8800ff";
    const char* defaultNotSelectedHoveredColor = "222299";

    auto coalesce = [](const char* a, const char* b)
    {
      return !!a ? a : b;
    };

    M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    M7::ParamAccessor pa{&tempVal, 0};
    auto selectedVal = pa.GetEnumValue<Tenum>(0);

    ImGui::PushID(ctrlLabel);

    ImGui::BeginGroup();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

    int column = 0;
    for (size_t i = 0; i < Tcount; i++)
    {
      auto& cfg = itemCfg[i];
      const bool is_selected = (selectedVal == cfg.value);
      int colorsPushed = 0;

      if (is_selected)
      {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              (ImVec4)ColorFromHTML(coalesce(cfg.selectedColor, defaultSelectedColor)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              (ImVec4)ColorFromHTML(coalesce(cfg.selectedColor, defaultSelectedColor)));
        colorsPushed += 2;
      }
      else
      {
        ImGui::PushStyleColor(ImGuiCol_Button,
                              (ImVec4)ColorFromHTML(coalesce(cfg.notSelectedColor, defaultNotSelectedColor)));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive,
                              (ImVec4)ColorFromHTML(coalesce(cfg.notSelectedColor, defaultNotSelectedColor)));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered,
                              (ImVec4)ColorFromHTML(
                                  coalesce(cfg.notSelectedHoveredColor, defaultNotSelectedHoveredColor)));
        colorsPushed += 3;
      }

      if (horiz && column != 0)
      {
        ImGui::SameLine(0, (float)spacing);
      }
      else if (spacing != 0)
      {
        ImGui::Dummy({1.0f, (float)spacing});
      }

      if (cfg.caption)
      {
        if (ImGui::Button(cfg.caption, ImVec2{width, 0}))
        {
          pa.SetEnumValue(0, cfg.value);
          GetEffectX()->setParameterAutomated((VstInt32)paramID, tempVal);
        }
      }
      else
      {
        float height = ImGui::GetTextLineHeightWithSpacing();
        ImGui::Dummy(ImVec2{width, height});
      }

      column = (column + 1);
      if (columns)
        column %= columns;

      ImGui::PopStyleColor(colorsPushed);
    }

    ImGui::PopStyleVar(2);  // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

    ImGui::EndGroup();
    ImGui::PopID();
  }

  template <typename TparamID>
  struct BoolToggleButtonArrayItem
  {
    TparamID paramID;
    const char* caption;
    const char* selectedColor;
    const char* notSelectedColor;
    const char* selectedHoveredColor;
    const char* notSelectedHoveredColor;
  };

  // the point of this is that you can have a different set of buttons than enum
  // options. the idea is that when "none" are selected, you can do something
  // different.
  template <typename TparamID, size_t Tcount>
  void Maj7ImGuiParamBoolToggleButtonArray(const char* ctrlLabel,
                                           float width,
                                           const BoolToggleButtonArrayItem<TparamID> (&itemCfg)[Tcount])
  {
    ImGui::PushID(ctrlLabel);

    ImGui::BeginGroup();

    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

    for (size_t i = 0; i < Tcount; i++)
    {
      auto& cfg = itemCfg[i];
      // const bool is_selected = (selectedVal == cfg.value);
      int colorsPushed = 0;

      M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)cfg.paramID);
      M7::ParamAccessor pa{&tempVal, 0};
      auto boolVal = pa.GetBoolValue(0);

      if (boolVal)
      {
        if (cfg.selectedColor)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
          colorsPushed += 2;
        }
        if (cfg.selectedHoveredColor)
        {
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.selectedHoveredColor));
          colorsPushed++;
        }
      }
      else
      {
        if (cfg.notSelectedColor)
        {
          ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
          ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
          colorsPushed += 2;
        }
        if (cfg.notSelectedHoveredColor)
        {
          ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.notSelectedHoveredColor));
          colorsPushed++;
        }
      }

      if (ImGui::Button(cfg.caption, ImVec2{width, 0}))
      {
        pa.SetBoolValue(0, !boolVal);
        GetEffectX()->setParameterAutomated((VstInt32)cfg.paramID, tempVal);
      }

      ImGui::PopStyleColor(colorsPushed);
    }

    ImGui::PopStyleVar(2);  // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

    ImGui::EndGroup();
    ImGui::PopID();
  }

  void Maj7ImGuiParamScaledFloat(VstInt32 paramID,
                                 const char* label,
                                 M7::real_t v_min,
                                 M7::real_t v_max,
                                 M7::real_t v_defaultScaled,
                                 float v_centerScaled,
                                 float sizePixels,
                                 ImGuiKnobs::ModInfo modInfo)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetRangedValue(0, v_min, v_max, v_defaultScaled);
    float defaultParamVal = tempVal;
    pa.SetRangedValue(0, v_min, v_max, v_centerScaled);
    float centerParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    ScaledFloatConverter conv{v_min, v_max};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerParamVal,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         sizePixels,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamFMKnob(VstInt32 paramID, const char* label)
  {
    WaveSabreCore::M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    M7::ParamAccessor pa{&tempVal, 0};
    const float v_default = 0;
    FMValueConverter conv;
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         v_default,
                         0,
                         ImGuiKnobs::ModInfo{},
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         "%.2f",
                         ImGuiKnobVariant_ProgressBarWithValue,
                         0,
                         ImGuiKnobFlags_CustomInput | ImGuiKnobFlags_NoInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamInt(VstInt32 paramID,
                         const char* label,
                         const M7::IntParamConfig& cfg,
                         int v_defaultScaled,
                         int v_centerScaled)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    // M7::IntParam p{ tempVal , cfg };
    pa.SetIntValue(0, cfg, v_defaultScaled);
    // p.SetIntValue(v_defaultScaled);
    float defaultParamVal = tempVal;
    pa.SetIntValue(0, cfg, v_centerScaled);
    float centerParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    Maj7IntConverter conv{cfg};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerParamVal,
                         ImGuiKnobs::ModInfo{},
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamFrequency(VstInt32 paramID,
                               VstInt32 ktParamID,
                               const char* label,
                               M7::FreqParamConfig cfg,
                               M7::real_t defaultParamValue,
                               ImGuiKnobs::ModInfo modInfo,
                               float centerValueParam01 = 0)
  {
    M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    Maj7FrequencyConverter conv{cfg, ktParamID};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamValue,
                         centerValueParam01,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamFrequencyWithCenter(VstInt32 paramID,
                                         VstInt32 ktParamID,
                                         const char* label,
                                         M7::FreqParamConfig cfg,
                                         M7::real_t defaultFreqHz,
                                         float centerVal01,
                                         ImGuiKnobs::ModInfo modInfo)
  {
    M7::real_t tempVal = 0;
    // M7::real_t tempValKT = 0;
    // M7::FrequencyParam p{ tempVal, tempValKT, cfg };
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetFrequencyAssumingNoKeytracking(0, cfg, defaultFreqHz);

    // tempVal = defaultFrequencyHz;
    float default01 = tempVal;
    // tempVal = centerValHz;
    // float centerVal01 = p.GetFrequency(0, 0);
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    Maj7FrequencyConverter conv{cfg, ktParamID};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         default01,
                         centerVal01,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamMidiNote(VstInt32 paramID, const char* label, int defaultVal, int centerVal)
  {
    M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetIntValue(0, M7::gKeyRangeCfg, defaultVal);
    float defaultParamVal = tempVal;
    pa.SetIntValue(0, M7::gKeyRangeCfg, centerVal);
    float centerParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    Maj7MidiNoteConverter conv;
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerParamVal,
                         ImGuiKnobs::ModInfo{},
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_ProgressBar,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  template <typename Tparam>
  void Maj7ImGuiDivCurvedParam(Tparam paramID,
                               const char* label,
                               const M7::DivCurvedParamCfg& cfg,
                               M7::real_t defaultValue,
                               ImGuiKnobs::ModInfo modInfo)
  {
    M7::real_t tempVal;
    M7::ParamAccessor p{&tempVal, 0};
    p.SetDivCurvedValue(0, cfg, defaultValue);
    float defaultParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    // float val = p.GetDivCurvedValue(0, cfg, 0);

    DivCurvedConverter conv{cfg};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         0,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated((VstInt32)paramID, (tempVal));
    }
  }

  void Maj7ImGuiParamFloat01(VstInt32 paramID,
                             const char* label,
                             M7::real_t v_default01,
                             float centerVal01,
                             float size = 0.0f,
                             ImGuiKnobs::ModInfo modInfo = {})
  {
    WaveSabreCore::M7::real_t tempVal = v_default01;
    M7::ParamAccessor pa{&tempVal, 0};
    float defaultParamVal = pa.Get01Value(0);
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    Float01Converter conv{};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerVal01,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         size,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  template <typename Tparam>
  void Maj7ImGuiPowCurvedParam(Tparam paramID,
                               const char* label,
                               const M7::PowCurvedParamCfg& cfg,
                               M7::real_t defaultMS,
                               ImGuiKnobs::ModInfo modInfo)
  {
    M7::real_t tempVal;
    M7::ParamAccessor p{&tempVal, 0};
    p.SetPowCurvedValue(0, cfg, defaultMS);
    float defaultParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    // float ms = p.GetTimeMilliseconds(0, cfg, 0);

    PowCurvedConverter conv{cfg};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         0,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated((VstInt32)paramID, (tempVal));
    }
  }
  void Maj7ImGuiParamFloatN11(VstInt32 paramID,
                              const char* label,
                              M7::real_t v_defaultScaled,
                              float size /*=0*/,
                              ImGuiKnobs::ModInfo modInfo)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetN11Value(0, v_defaultScaled);
    float defaultParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    FloatN11Converter conv{};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         0.5f,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         size,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamFloatN11WithCenter(VstInt32 paramID,
                                        const char* label,
                                        M7::real_t v_defaultScaled,
                                        float centerValN11,
                                        float size /*=0*/,
                                        ImGuiKnobs::ModInfo modInfo)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetN11Value(0, v_defaultScaled);
    float defaultParamVal = tempVal;
    pa.SetN11Value(0, centerValN11);
    float centerParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    FloatN11Converter conv{};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerParamVal,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         size,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  // reflects the pow curve to negative range
  template <typename Tparam>
  void Maj7ImGuiBipolarPowCurvedParam(Tparam paramID,
                                      const char* label,
                                      const M7::PowCurvedParamCfg& cfg,
                                      M7::real_t defaultMS,
                                      ImGuiKnobs::ModInfo modInfo)
  {
    M7::real_t tempVal;
    M7::ParamAccessor p{&tempVal, 0};
    p.SetBipolarPowCurvedValue(0, cfg, defaultMS);
    float defaultParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);
    // float ms = p.GetTimeMilliseconds(0, cfg, 0);

    BipolarPowCurvedConverter conv{cfg};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         0.5f,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated((VstInt32)paramID, (tempVal));
    }
  }


  enum class M7CurveRenderStyle
  {
    Rising,
    Falling,
  };

  void Maj7ImGuiParamCurve(VstInt32 paramID,
                           const char* label,
                           M7::real_t v_defaultN11,
                           M7CurveRenderStyle style,
                           ImGuiKnobs::ModInfo modInfo)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetN11Value(0, v_defaultN11);
    float defaultParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    FloatN11Converter conv{};
    int flags = (int)ImGuiKnobFlags_CustomInput;
    if (style == M7CurveRenderStyle::Rising)
    {
      flags |= ImGuiKnobFlags_InvertYCurve;
    }
    else if (style == M7CurveRenderStyle::Falling)
    {
      flags |= ImGuiKnobFlags_InvertYCurve | ImGuiKnobFlags_InvertXCurve;
    }
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         0.5f,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_M7Curve,
                         0,
                         flags,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  void Maj7ImGuiParamVolume(VstInt32 paramID,
                            const char* label,
                            const M7::VolumeParamConfig& cfg,
                            M7::real_t v_defaultDB,
                            ImGuiKnobs::ModInfo modInfo)
  {
    WaveSabreCore::M7::real_t tempVal;
    M7::ParamAccessor pa{&tempVal, 0};
    pa.SetDecibels(0, cfg, v_defaultDB);
    float defaultParamVal = tempVal;
    pa.SetDecibels(0, cfg, 0);
    float centerParamVal = tempVal;
    tempVal = GetEffectX()->getParameter((VstInt32)paramID);

    M7VolumeConverter conv{cfg};
    if (ImGuiKnobs::Knob(label,
                         &tempVal,
                         0,
                         1,
                         defaultParamVal,
                         centerParamVal,
                         modInfo,
                         gNormalKnobSpeed,
                         gSlowKnobSpeed,
                         nullptr,
                         ImGuiKnobVariant_WiperOnly,
                         0,
                         ImGuiKnobFlags_CustomInput,
                         10,
                         &conv,
                         this))
    {
      GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
    }
  }

  template <typename Tenum>
  void Maj7ImGuiEnvelopeGraphic(const char* label, Tenum delayTimeParamID)
  {
    static constexpr float gSegmentMaxWidth = 50;
    static constexpr float gMaxWidth = gSegmentMaxWidth * 6;
    static constexpr float innerHeight = 65;  // excluding padding
    static constexpr float padding = 7;
    static constexpr float gLineThickness = 2.7f;
    ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_PlotLines);
    static constexpr float handleRadius = 3;
    static constexpr int circleSegments = 7;

    // Get envelope parameters
    float delayWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID +
                                                                  (int)M7::EnvParamIndexOffsets::DelayTime),
                                       0,
                                       1) *
                       gSegmentMaxWidth;
    float attackWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID +
                                                                   (int)M7::EnvParamIndexOffsets::AttackTime),
                                        0,
                                        1) *
                        gSegmentMaxWidth;
    float holdWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID +
                                                                 (int)M7::EnvParamIndexOffsets::HoldTime),
                                      0,
                                      1) *
                      gSegmentMaxWidth;
    float decayWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID +
                                                                  (int)M7::EnvParamIndexOffsets::DecayTime),
                                       0,
                                       1) *
                       gSegmentMaxWidth;
    float sustainWidth = gSegmentMaxWidth;
    float releaseWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID +
                                                                    (int)M7::EnvParamIndexOffsets::ReleaseTime),
                                         0,
                                         1) *
                         gSegmentMaxWidth;

    float sustainLevel = M7::math::clamp(
        GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel), 0, 1);
    float sustainYOffset = innerHeight * (1.0f - sustainLevel);

    M7::QuickParam attackCurve{
        GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve)};
    M7::QuickParam decayCurve{
        GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve)};
    M7::QuickParam releaseCurve{
        GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve)};

    ImVec2 originalPos = ImGui::GetCursorScreenPos();
    ImVec2 p = originalPos;
    p.x += padding;
    p.y += padding;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    // background
    ImVec2 outerTL = originalPos;
    ImVec2 innerTL = {outerTL.x + padding, outerTL.y + padding};
    ImVec2 outerBottomLeft = {outerTL.x, outerTL.y + innerHeight + padding * 2};
    ImVec2 outerBottomRight = {outerTL.x + gMaxWidth + padding * 2, outerTL.y + innerHeight + padding * 2};
    ImVec2 innerBottomLeft = {innerTL.x, innerTL.y + innerHeight};
    ImVec2 innerBottomRight = {innerBottomLeft.x + gMaxWidth, innerBottomLeft.y};

    ImGui::RenderFrame(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f);

    // Draw envelope segments
    ImVec2 delayStart = innerBottomLeft;
    ImVec2 delayEnd = {delayStart.x + delayWidth, delayStart.y};
    dl->AddLine(delayStart, delayEnd, lineColor, gLineThickness);

    ImVec2 attackEnd = {delayEnd.x + attackWidth, innerTL.y};
    AddCurveToPath(dl,
                   delayEnd,
                   {attackEnd.x - delayEnd.x, attackEnd.y - delayEnd.y},
                   false,
                   false,
                   attackCurve.GetRawValue(),
                   lineColor,
                   gLineThickness);

    ImVec2 holdEnd = {attackEnd.x + holdWidth, attackEnd.y};
    dl->AddLine(attackEnd, holdEnd, lineColor, gLineThickness);

    ImVec2 decayEnd = {holdEnd.x + decayWidth, innerTL.y + sustainYOffset};
    AddCurveToPath(dl,
                   holdEnd,
                   {decayEnd.x - holdEnd.x, decayEnd.y - holdEnd.y},
                   true,
                   true,
                   decayCurve.GetRawValue(),
                   lineColor,
                   gLineThickness);

    ImVec2 sustainEnd = {decayEnd.x + sustainWidth, decayEnd.y};
    dl->AddLine(decayEnd, sustainEnd, lineColor, gLineThickness);

    ImVec2 releaseEnd = {sustainEnd.x + releaseWidth, innerBottomLeft.y};
    AddCurveToPath(dl,
                   sustainEnd,
                   {releaseEnd.x - sustainEnd.x, releaseEnd.y - sustainEnd.y},
                   true,
                   true,
                   releaseCurve.GetRawValue(),
                   lineColor,
                   gLineThickness);

    // Draw control points
    dl->AddCircleFilled(delayStart, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(delayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(attackEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(holdEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(decayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(sustainEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
    dl->AddCircleFilled(releaseEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);

    ImGui::Dummy({outerBottomRight.x - originalPos.x, outerBottomRight.y - originalPos.y});
  }

  virtual bool onWheel(float distance)
  {
    return false;
  }
  virtual bool setKnobMode(VstInt32 val)
  {
    return false;
  }

  // Inject ImGui inputs from VST effEditKeyDown path (for hosts that don't send WM_*)
  virtual bool onKeyDown(VstKeyCode& keyCode) override
  {
    auto ctx = PushMyImGuiContext("onKeyDown");
    ImGuiIO& io = ImGui::GetIO();

    io.AddKeyEvent(ImGuiMod_Shift, (keyCode.modifier & MODIFIER_SHIFT) != 0);
    io.AddKeyEvent(ImGuiMod_Alt, (keyCode.modifier & MODIFIER_ALTERNATE) != 0);
#if defined(_WIN32)
    io.AddKeyEvent(ImGuiMod_Ctrl, (keyCode.modifier & MODIFIER_CONTROL) != 0);
#else
    io.AddKeyEvent(ImGuiMod_Super, (keyCode.modifier & MODIFIER_COMMAND) != 0);
#endif

    ImGuiKey ik = mapVk(keyCode.virt);
    if (ik != ImGuiKey_None)
      io.AddKeyEvent(ik, true);

    if (keyCode.character > 0 && keyCode.character < 0x10000)
    {
      io.AddInputCharacterUTF16((unsigned short)keyCode.character);
    }

    mLastKeyDown = keyCode;
    return true;
  }
  virtual bool onKeyUp(VstKeyCode& keyCode) override
  {
    auto ctx = PushMyImGuiContext("onKeyUp");
    ImGuiIO& io = ImGui::GetIO();

    ImGuiKey ik = mapVk(keyCode.virt);
    if (ik != ImGuiKey_None)
      io.AddKeyEvent(ik, false);

    mLastKeyUp = keyCode;
    return true;
  }


  static ImGuiKey mapVk(unsigned char vk)
  {
    switch (vk)
    {
      case VKEY_TAB:
        return ImGuiKey_Tab;
      case VKEY_LEFT:
        return ImGuiKey_LeftArrow;
      case VKEY_RIGHT:
        return ImGuiKey_RightArrow;
      case VKEY_UP:
        return ImGuiKey_UpArrow;
      case VKEY_DOWN:
        return ImGuiKey_DownArrow;
      case VKEY_PAGEUP:
        return ImGuiKey_PageUp;
      case VKEY_PAGEDOWN:
        return ImGuiKey_PageDown;
      case VKEY_HOME:
        return ImGuiKey_Home;
      case VKEY_END:
        return ImGuiKey_End;
      case VKEY_INSERT:
        return ImGuiKey_Insert;
      case VKEY_DELETE:
        return ImGuiKey_Delete;
      case VKEY_BACK:
        return ImGuiKey_Backspace;
      case VKEY_SPACE:
        return ImGuiKey_Space;
      case VKEY_RETURN:
        return ImGuiKey_Enter;
      case VKEY_ESCAPE:
        return ImGuiKey_Escape;
      case VKEY_NUMPAD0:
        return ImGuiKey_Keypad0;
      case VKEY_NUMPAD1:
        return ImGuiKey_Keypad1;
      case VKEY_NUMPAD2:
        return ImGuiKey_Keypad2;
      case VKEY_NUMPAD3:
        return ImGuiKey_Keypad3;
      case VKEY_NUMPAD4:
        return ImGuiKey_Keypad4;
      case VKEY_NUMPAD5:
        return ImGuiKey_Keypad5;
      case VKEY_NUMPAD6:
        return ImGuiKey_Keypad6;
      case VKEY_NUMPAD7:
        return ImGuiKey_Keypad7;
      case VKEY_NUMPAD8:
        return ImGuiKey_Keypad8;
      case VKEY_NUMPAD9:
        return ImGuiKey_Keypad9;
      case VKEY_DECIMAL:
        return ImGuiKey_KeypadDecimal;
      case VKEY_DIVIDE:
        return ImGuiKey_KeypadDivide;
      case VKEY_MULTIPLY:
        return ImGuiKey_KeypadMultiply;
      case VKEY_SUBTRACT:
        return ImGuiKey_KeypadSubtract;
      case VKEY_ADD:
        return ImGuiKey_KeypadAdd;
      case VKEY_F1:
        return ImGuiKey_F1;
      case VKEY_F2:
        return ImGuiKey_F2;
      case VKEY_F3:
        return ImGuiKey_F3;
      case VKEY_F4:
        return ImGuiKey_F4;
      case VKEY_F5:
        return ImGuiKey_F5;
      case VKEY_F6:
        return ImGuiKey_F6;
      case VKEY_F7:
        return ImGuiKey_F7;
      case VKEY_F8:
        return ImGuiKey_F8;
      case VKEY_F9:
        return ImGuiKey_F9;
      case VKEY_F10:
        return ImGuiKey_F10;
      case VKEY_F11:
        return ImGuiKey_F11;
      case VKEY_F12:
        return ImGuiKey_F12;
      default:
        return ImGuiKey_None;
    }
  };

protected:
  VstKeyCode mLastKeyDown = {0};
  VstKeyCode mLastKeyUp = {0};

  VstPlug* GetEffectX()
  {
    return static_cast<VstPlug*>(this->effect);
  }

  LPDIRECT3D9 g_pD3D = NULL;
  LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
  D3DPRESENT_PARAMETERS g_d3dpp = {};

  bool CreateDeviceD3D(HWND hWnd);
  void CleanupDeviceD3D();
  void ResetDevice();

  HWND mParentWindow = nullptr;
  HWND mCurrentWindow = nullptr;

  ImGuiContext* mImGuiContext = nullptr;

  struct GuiContextRestorer
  {
    ImGuiContext* mPrevContext = nullptr;
    bool pushed = false;
    const char* mWhy;
    GuiContextRestorer(ImGuiContext* newContext, const char* why)
        : mWhy(why)
    {
      mPrevContext = ImGui::GetCurrentContext();
      if (mPrevContext == newContext)
      {
        return;  // no change needed.
      }
      pushed = true;
      ImGui::SetCurrentContext(newContext);
    }
    ~GuiContextRestorer()
    {
      auto current = ImGui::GetCurrentContext();
      if (!pushed)
      {
        return;
      }
      if (current == mPrevContext)
      {
        return;
      }
      ImGui::SetCurrentContext(mPrevContext);
    }
  };

  GuiContextRestorer PushMyImGuiContext(const char* why)
  {
    return {mImGuiContext, why};
  }

  void ImguiPresent();

  static LRESULT WINAPI gWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
  LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

  ERect mDefaultRect = {0};
  bool showingDemo = false;

  bool showingParamExplorer = false;
  ParamExplorer mParamExplorer;
};

}  // namespace WaveSabreVstLib

#endif
