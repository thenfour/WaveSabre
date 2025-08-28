
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
#include <WaveSabreVstLib/ImGuiUtils.hpp>

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
  *v = p.GetN11Value();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void KnobFilterFrequency(const char* label, float* hz, float defaultValHz, float centerValHz)
{
  const auto& cfg = M7::gFilterFreqConfig;
  M7::QuickParam param;
  param.SetFrequencyAssumingNoKeytracking(cfg, defaultValHz);
  float default01 = param.GetRawValue();
  param.SetFrequencyAssumingNoKeytracking(cfg, centerValHz);
  float center01 = param.GetRawValue();

  param.SetFrequencyAssumingNoKeytracking(cfg, *hz);
  float tempVal = param.GetRawValue();

  static WaveSabreVstLib::Maj7FrequencyConverter conv{cfg, -1};

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

}  // namespace WaveSabreVstLib

