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
#include <WaveSabreVstLib/Maj7ParamConverters.hpp>

void Knob01(const char* label, float* v, float defaultVal01, float centerVal01)
{
  WaveSabreVstLib::Float01Converter conv{};

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


void renderManualTestsUI()
{
  ImGui::Begin("Manual Tests");

  static float v = 0;
  Knob01("knob", &v, 0.25f, 0.75f);

  ImGui::End();
}
