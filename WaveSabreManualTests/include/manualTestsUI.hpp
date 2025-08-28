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

#include "manualTestUtils.hpp"
//
//namespace WaveSabreCore
//{
//namespace M7
//{
//struct MockSynth
//{
//  float paramCache[(int)M7::ParamIndices::NumParams] = {0};
//  M7::ParamAccessor pa{paramCache, 0};
//
//  M7::ModMatrixNode modMatrix;
//
//  M7::EnvelopeInfo envInfo{.mModDestBase = M7::ModDestination::Osc1AmpEnvDelayTime,
//                           .mParamBase = M7::ParamIndices::Osc1AmpEnvDelayTime,
//                           .mMyModSource = M7::ModSource::Osc1AmpEnv};
//  M7::EnvelopeNode ampEnv{modMatrix, paramCache, envInfo};
//  M7::OscillatorDevice oscillatorDevice{paramCache, 0};
//
//  M7::OscillatorNode oscillatorNode{&oscillatorDevice, M7::OscillatorIntention::Audio, &modMatrix, &ampEnv};
//
//  MockSynth()
//  {
//    pa.SetBoolValue(ParamIndices::Osc1SyncEnable, false);
//    //sync freq
//    //pa.Set01Val(ParamIndices::Osc1WaveshapeA, 0.5f);        // waveshape a
//    //pa.Set01Val(ParamIndices::Osc1WaveshapeB, 0.5f);        // waveshape b
//    //pa.Set01Val(ParamIndices::Osc1FrequencyParam, 0.5f);    // freq param
//    //pa.Set01Val(ParamIndices::Osc1FrequencyParamKT, 0.0f);  // freq kt
//    //pitch semis
//    //pitch fine
//    //freq mul
//  }
//};
//}  // namespace M7
//}  // namespace WaveSabreCore

namespace WaveSabreVstLib
{
void renderManualTestsUI()
{
  ImGui::Begin("Manual Tests");

  Helpers::SetSampleRate(4410);

  static float phaseOffset = 0;
  KnobN11("phase off", &phaseOffset, 0.25f, 0.75f);

  static float freq = 12050;
  KnobFilterFrequency("filter hz", &freq, 8000, 9000);

  static float volDb = 0;
  KnobVolume("volume", &volDb, M7::gVolumeCfg12db);


  float otherSignals[M7::gOscillatorCount - 1] = {0};

  //M7::MockSynth synth;
  M7::Maj7 synth;

  // let's show a full 1.5 cycles of the wave.
  static float secondsToShow = 0.1f;
  Knob01("seconds", &secondsToShow);
  int sampleCount = (int)(Helpers::CurrentSampleRateF * secondsToShow);
  static std::unique_ptr<float[]> samples = std::make_unique<float[]>(sampleCount);

  static int calcTrigger = 0;
  if (ImGui::Button("Trigger"))
  {
    for (int i = 0; i < sampleCount; ++i)
    {
      samples[i] = synth.mMaj7Voice[0]->mpOscillatorNodes[0]->RenderSampleForAudioAndAdvancePhase(
          64,                                // midi note
          1.f,                               // freq detune MUL
          0.0f,                              // fm scale
          synth.mParams,                     // param accessor
          otherSignals,                      // other osc signals
          0,                                 // this osc index
          M7::math::DecibelsToLinear(volDb)  // amp env linear
      );
    }
  }

  {
    ImGui::Text("Oscillator output (1.5 cycles at %.1fHz)", freq);
    ImGuiGroupScope groupScope{"plot"};
    ImVec2 plotSize = {800, 600};
    ImGui::PlotLines("", samples.get(), sampleCount, 0, nullptr, -1.0f, 1.0f, plotSize);
    // draw phase offset line
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetItemRectMin();
    ImVec2 size = ImGui::GetItemRectSize();
    float x = pos.x + size.x * M7::math::fract(phaseOffset);
    dl->AddLine({x, pos.y}, {x, pos.y + size.y}, IM_COL32(255, 0, 0, 255), 1.0f);
  }

  ImGui::End();
}

}  // namespace WaveSabreVstLib
