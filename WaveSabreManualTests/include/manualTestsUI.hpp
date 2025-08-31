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
#include <WaveSabreVstLib/Maj7EditorComponents.hpp>
#include <WaveSabreVstLib/Maj7ParamConverters.hpp>

#include "manualTestUtils.hpp"
#include "waveformView.hpp"

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
  auto& io = ImGui::GetIO();

  ImGui::SetNextWindowPos(ImVec2{0, 0});
  ImGui::SetNextWindowSize(io.DisplaySize);

  ImGui::Begin("##main",
               0,
               ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);


  if (ImGui::BeginMenuBar())
  {
    ImGui::EndMenuBar();
  }

  Helpers::SetSampleRate(4410);

  M7::Maj7 synth;  // todo ...
  auto& oscNode = *synth.mMaj7Voice[0]->mpOscillatorNodes[0];

  static float waveShapeA = 0.5f;
  Knob01("waveshape A", &waveShapeA, 0.5f, 0.5f);
  synth.mParams.SetN11Value((int)M7::ParamIndices::Osc1WaveshapeA, waveShapeA);

  static float waveShapeB = 0.5f;
  ImGui::SameLine();
  Knob01("waveshape B", &waveShapeB, 0.5f, 0.5f);
  synth.mParams.SetN11Value((int)M7::ParamIndices::Osc1WaveshapeB, waveShapeB);

  static float phaseOffset = -0.64f;
  ImGui::SameLine();
  KnobN11("phase offset", &phaseOffset, 0, 0);
  synth.mParams.SetN11Value((int)M7::ParamIndices::Osc1PhaseOffset, phaseOffset);

  //static float freq = 12050;
  ImGui::SameLine();
  static float freq01 = 0.4f;
  Knob01("freq", &freq01, 0.4f, 0.4f);
  synth.mParamCache[(int)M7::ParamIndices::Osc1FrequencyParam] = freq01;

  static float volDb = 0;
  ImGui::SameLine();
  KnobVolume("volume", &volDb, M7::gVolumeCfg12db);

  float otherSignals[M7::gOscillatorCount - 1] = {0};

  // let's show a full 1.5 cycles of the wave.
  static float secondsToShow = 0.01f;
  ImGui::SameLine();
  KnobScaled("seconds", &secondsToShow, 0.001f, 0.2f);

  static float blepScale = 1;
  KnobN11("blepscale", &blepScale, 0, 0);
  oscNode.mInv = blepScale;

  static M7::OscillatorWaveform wf = M7::OscillatorWaveform::SawClip;// = synth.mParams.GetEnumValue<M7::OscillatorWaveform>(M7::ParamIndices::Osc1Waveform);
  SelectEnumButtonArray(
      "waveform",
      wf,
      std::array<SelectEnumButtonArrayItem<M7::OscillatorWaveform>, 4>
      {
          SelectEnumButtonArrayItem<M7::OscillatorWaveform> { .caption = "Sine", .value = M7::OscillatorWaveform::SineClip },
          SelectEnumButtonArrayItem<M7::OscillatorWaveform> { .caption = "Saw", .value = M7::OscillatorWaveform::SawClip },
          SelectEnumButtonArrayItem<M7::OscillatorWaveform> { .caption = "Pulse", .value = M7::OscillatorWaveform::Pulse },
          SelectEnumButtonArrayItem<M7::OscillatorWaveform> { .caption = "Tri", .value = M7::OscillatorWaveform::TriTrunc },
      });
  synth.mParams.SetEnumValue<M7::OscillatorWaveform>(M7::ParamIndices::Osc1Waveform, wf);

  static bool showLollipops = true;
  static bool showStems = true;
  static bool showLines = true;

  ButtonArray<3>({
      MakeButtonSpec("Lollipops", &showLollipops),
      MakeButtonSpec("Stems", &showStems),
      MakeButtonSpec("Lines", &showLines),
      //MakeButtonSpec("Invert Bleps", &invertBleps),
  });

  int sampleCount = (int)(Helpers::CurrentSampleRateF * secondsToShow);
  static std::vector<WFVSample> samples;

  if (samples.size() != sampleCount)
  {
    samples.resize(sampleCount);
  }

  oscNode.NoteOn(false);

  for (int i = 0; i < sampleCount; ++i)
  {
    auto r = oscNode.RenderSampleForAudioAndAdvancePhase(
        64,                                // midi note
        1.f,                               // freq detune MUL
        0.0f,                              // fm scale
        synth.mParams,                     // param accessor
        otherSignals,                      // other osc signals
        0,                                 // this osc index
        M7::math::DecibelsToLinear(volDb)  // amp env linear
    );
    // collect more detailed info.
    samples[i].sample = oscNode.mLastSample;
  }

  std::vector<float> referenceLines = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};

  ImGui::Text("Frequency: %.3f Hz", samples.empty() ? 0.0f : samples[0].sample.phaseAdvance.ComputeFrequencyHz());

  WaveformViewImpl("wf",
                   {io.DisplaySize.x - 20, 600},
                   samples,
                   {
                       .referenceYValues = &referenceLines,
                       .showLollipops = showLollipops,
                       .showStems = showStems,
                       .showLines = showLines,
                       .autoRangeY = false,
                       .yMin = -2.0f,
                       .yMax = 2.0f,
                   });

  ImGui::End();
}

}  // namespace WaveSabreVstLib
