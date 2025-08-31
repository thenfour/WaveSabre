
#pragma once

#include <d3d9.h>
#include <tchar.h>
#include <windows.h>

#include <imgui-knobs.h>
#include <imgui.h>
#include <imgui_impl_dx9.h>
#include <imgui_impl_win32.h>

#include <WaveSabreCore.h>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreVstLib/ImGuiUtils.hpp>
#include <WaveSabreVstLib/Maj7ParamConverters.hpp>

namespace WaveSabreVstLib
{

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Knob01(const char* label, float* v, float defaultVal01 = 0, float centerVal01 = 0)
{
  static WaveSabreVstLib::Float01Converter conv{};

  ImGuiKnobs::Knob(label,
                   v,
                   0,
                   1,
                   defaultVal01,
                   centerVal01,
                   {},  // modinfo
                   WaveSabreVstLib::gNormalKnobSpeed,
                   WaveSabreVstLib::gSlowKnobSpeed,
                   nullptr,
                   ImGuiKnobVariant_WiperOnly,
                   0,
                   ImGuiKnobFlags_CustomInput,
                   10,  // steps
                   &conv);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KnobN11(const char* label, float* v, float defaultValN11 = 0, float centerValN11 = 0)
{
  static WaveSabreVstLib::FloatN11Converter conv{};

  M7::QuickParam p;
  p.SetN11Value(defaultValN11);
  float defaultVal01 = p.GetRawValue();
  p.SetN11Value(centerValN11);
  float centerVal01 = p.GetRawValue();
  p.SetN11Value(*v);
  float tempVal = p.GetRawValue();
  ImGuiKnobs::Knob(label,
                   &tempVal,
                   0,
                   1,
                   defaultVal01,
                   centerVal01,
                   {},  // modinfo
                   WaveSabreVstLib::gNormalKnobSpeed,
                   WaveSabreVstLib::gSlowKnobSpeed,
                   nullptr,
                   ImGuiKnobVariant_WiperOnly,
                   0,
                   ImGuiKnobFlags_CustomInput,
                   10,  // steps
                   &conv);
  p.SetRawValue(tempVal);
  *v = p.GetN11Value();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KnobScaled(const char* label, float* v, float vmin, float vmax, float defaultVal = 0, float centerVal = 0)
{
  WaveSabreVstLib::ScaledFloatConverter conv{vmin, vmax};

  M7::QuickParam p;
  p.SetScaledValue(vmin, vmax, defaultVal);
  float defaultVal01 = p.GetRawValue();
  p.SetScaledValue(vmin, vmax, centerVal);
  float centerVal01 = p.GetRawValue();
  p.SetScaledValue(vmin, vmax, *v);
  float tempVal = p.GetRawValue();
  ImGuiKnobs::Knob(label,
                   &tempVal,
                   0,
                   1,
                   defaultVal01,
                   centerVal01,
                   {},  // modinfo
                   WaveSabreVstLib::gNormalKnobSpeed,
                   WaveSabreVstLib::gSlowKnobSpeed,
                   nullptr,
                   ImGuiKnobVariant_WiperOnly,
                   0,
                   ImGuiKnobFlags_CustomInput,
                   10,  // steps
                   &conv);
  p.SetRawValue(tempVal);
  *v = p.GetScaledValue(vmin, vmax);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KnobFilterFrequency(const char* label, float* hz, float defaultValHz, float centerValHz, std::function<bool()>&& getKT)
{
  const auto& cfg = M7::gFilterFreqConfig;
  M7::QuickParam param;
  param.SetFrequencyAssumingNoKeytracking(cfg, defaultValHz);
  float default01 = param.GetRawValue();
  param.SetFrequencyAssumingNoKeytracking(cfg, centerValHz);
  float center01 = param.GetRawValue();

  param.SetFrequencyAssumingNoKeytracking(cfg, *hz);
  float tempVal = param.GetRawValue();

  static WaveSabreVstLib::Maj7FrequencyConverter conv{cfg, std::move(getKT)};

  ImGuiKnobs::Knob(label,
                   &tempVal,
                   0,
                   1,
                   default01,
                   center01,
                   {},  // mod info
                   gNormalKnobSpeed,
                   gSlowKnobSpeed,
                   nullptr,
                   ImGuiKnobVariant_WiperOnly,
                   0,
                   ImGuiKnobFlags_CustomInput,
                   10,
                   &conv);
  param.SetRawValue(tempVal);
  *hz = param.GetFrequency(cfg);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KnobVolume(const char* label,
                float* db,
                const M7::VolumeParamConfig& cfg,
                float defaultValDb = 0,
                float centerValDb = 0)
{
  WaveSabreCore::M7::real_t tempVal;
  M7::ParamAccessor pa{&tempVal, 0};
  pa.SetDecibels(0, cfg, defaultValDb);
  float defaultParamVal = tempVal;
  pa.SetDecibels(0, cfg, centerValDb);
  float centerParamVal = tempVal;

  pa.SetDecibels(0, cfg, *db);

  M7VolumeConverter conv{cfg};
  ImGuiKnobs::Knob(label,
                   &tempVal,
                   0,
                   1,
                   defaultParamVal,
                   centerParamVal,
                   {},  // modinfo
                   gNormalKnobSpeed,
                   gSlowKnobSpeed,
                   nullptr,
                   ImGuiKnobVariant_WiperOnly,
                   0,
                   ImGuiKnobFlags_CustomInput,
                   10,
                   &conv);
  *db = pa.GetDecibels(0, cfg);
}

template <typename TEnum>
struct SelectEnumButtonArrayItem
{
  const char* caption;
  TEnum value;
};


template <typename Tenum, size_t Tcount>
void SelectEnumButtonArray(const char* ctrlLabel,
                           Tenum& value,
                           const std::array<SelectEnumButtonArrayItem<Tenum>, Tcount>& items,
                           bool horiz = true,
                           float buttonWidth = 0,
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

  //M7::real_t tempVal = value;
  //M7::ParamAccessor pa{&tempVal, 0};
  //auto selectedVal = pa.GetEnumValue<Tenum>(0);

  ImGui::PushID(ctrlLabel);

  ImGui::BeginGroup();

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});
  ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

  int column = 0;
  for (size_t i = 0; i < Tcount; i++)
  {
    auto& cfg = items[i];
    const bool is_selected = (value == cfg.value);
    int colorsPushed = 0;

    if (is_selected)
    {
      ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(defaultSelectedColor));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(defaultSelectedColor));
      colorsPushed += 2;
    }
    else
    {
      ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(defaultNotSelectedColor));
      ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(defaultNotSelectedColor));
      ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(defaultNotSelectedHoveredColor));
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
      if (ImGui::Button(cfg.caption, ImVec2{buttonWidth, 0}))
      {
        value = cfg.value;
      }
    }
    else
    {
      float height = ImGui::GetTextLineHeightWithSpacing();
      ImGui::Dummy(ImVec2{buttonWidth, height});
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


}  // namespace WaveSabreVstLib
