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

struct Maj7ManualTestState
{
  float phaseOffset = 0;
  float freq01 = 0.372f;
  float freqkt = 1;
  bool syncEnable = false;
  float syncFreq01 = 0.40f;
  float syncFreqKT = 1;
  float volDb = 0;
  float secondsToShow = 0.01f;
  bool showLollipops = true;
  bool showStems = true;
  bool showLines = false;
  bool showExtDots = true;
  bool showEdgeEventLines = true;
  M7::OscillatorWaveform wf = M7::OscillatorWaveform::SawArtisnal;
  float waveShapeA = 0;
  float waveShapeB = 1;
};

struct Maj7SynthWrapper
{
  M7::Maj7 mSynth;
  M7::OscillatorNode& mOscNode = *mSynth.mMaj7Voice[0]->mpOscillatorNodes[0];

  Maj7SynthWrapper()
  {
    Helpers::SetSampleRate(4000);
  }
};

// read-only, multi-line, scrollable
void ImGuiLogWindow(const std::string& text, ImVec2 size)
{
  ImGui::BeginChild("logwindow", size, true, ImGuiWindowFlags_HorizontalScrollbar);
  ImGui::TextUnformatted(text.c_str());
  if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
    ImGui::SetScrollHereY(1.0f);
  ImGui::EndChild();
}

void renderManualTestsUI(Maj7SynthWrapper& synth, Maj7ManualTestState& state)
{
  auto& io = ImGui::GetIO();
  OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);

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

  KnobN11("waveshape A", &state.waveShapeA, 0, 0);
  synth.mSynth.mParams.SetN11Value((int)M7::ParamIndices::Osc1WaveshapeA, state.waveShapeA);

  ImGui::SameLine();
  KnobN11("waveshape B", &state.waveShapeB, 0, 0);
  synth.mSynth.mParams.SetN11Value((int)M7::ParamIndices::Osc1WaveshapeB, state.waveShapeB);

  ImGui::SameLine();
  KnobN11("phase offset", &state.phaseOffset, 0, 0);
  synth.mSynth.mParams.SetN11Value((int)M7::ParamIndices::Osc1PhaseOffset, state.phaseOffset);

  ImGui::SameLine();
  Knob01("freq", &state.freq01, 0.3f, 0.3f);
  synth.mSynth.mParamCache[(int)M7::ParamIndices::Osc1FrequencyParam] = state.freq01;

  ImGui::SameLine();
  Knob01("freq KT", &state.freqkt, 1, 1);
  synth.mSynth.mParamCache[(int)M7::ParamIndices::Osc1FrequencyParamKT] = state.freqkt;

  ImGui::SameLine();
  ImGui::Checkbox("sync", &state.syncEnable);
  synth.mSynth.mParams.SetBoolValue((int)M7::ParamIndices::Osc1SyncEnable, state.syncEnable);

  // sync freq & KT

  ImGui::SameLine();
  Knob01("syncFreq", &state.syncFreq01, 0.4f, 0.4f);
  synth.mSynth.mParamCache[(int)M7::ParamIndices::Osc1SyncFrequency] = state.syncFreq01;

  ImGui::SameLine();
  Knob01("syncFreqKT", &state.syncFreqKT, 1, 1);
  synth.mSynth.mParamCache[(int)M7::ParamIndices::Osc1SyncFrequencyKT] = state.syncFreqKT;


  ImGui::SameLine();
  KnobVolume("volume", &state.volDb, M7::gVolumeCfg12db);

  float otherSignals[M7::gOscillatorCount - 1] = {0};

  // let's show a full 1.5 cycles of the wave.
  ImGui::SameLine();
  KnobScaled("seconds", &state.secondsToShow, 0.001f, 0.2f);

  ImGui::SameLine();
  ImGui::SetNextItemWidth(120);
  ImGui::Combo("waveformShape", (int*)&state.wf, gWaveformCaptions, (int)M7::OscillatorWaveform::Count);

  synth.mSynth.mParams.SetEnumValue<M7::OscillatorWaveform>(M7::ParamIndices::Osc1Waveform, state.wf);


  ImGui::SameLine();
  ButtonArray<5>("waveformOptions",
                 {
                     MakeButtonSpec("Lollipops", &state.showLollipops),
                     MakeButtonSpec("Stems", &state.showStems),
                     MakeButtonSpec("Lines", &state.showLines),
                     MakeButtonSpec("ExtDots", &state.showExtDots),
                     MakeButtonSpec("EventLines", &state.showEdgeEventLines),
                     //MakeButtonSpec("Invert Bleps", &invertBleps),
                 });

  int sampleCount = (int)(Helpers::CurrentSampleRateF * state.secondsToShow);
  static std::vector<WFVSample> samples;

  if (samples.size() != sampleCount)
  {
    samples.resize(sampleCount);
  }

  {
#ifdef ENABLE_OSC_LOG
    auto logscope = M7::gOscLog.IndentBlock("ImGuiFrame render");
#endif  // ENABLE_OSC_LOG
    synth.mOscNode.NoteOn(false);

    for (int i = 0; i < sampleCount; ++i)
    {
#ifdef ENABLE_OSC_LOG
      M7::gOscLog.Clear();
#endif  // ENABLE_OSC_LOG

      auto r = synth.mOscNode.RenderSampleForAudioAndAdvancePhase(64,                    // midi note
                                                                  1.f,                   // freq detune MUL
                                                                  0.0f,                  // fm scale
                                                                  synth.mSynth.mParams,  // param accessor
                                                                  otherSignals,          // other osc signals
                                                                  0,                     // this osc index
                                                                  M7::math::DecibelsToLinear(
                                                                      state.volDb)  // amp env linear
      );

      samples[i].sample = synth.mOscNode.mLastSample;

      // collect more detailed info.
#ifdef ENABLE_OSC_LOG
      M7::gOscLog.Log(std::format("Naive: {:.3f}", synth.mOscNode.mLastSample.naive));
      M7::gOscLog.Log(std::format("Correction: {:.3f}", synth.mOscNode.mLastSample.correction));
      M7::gOscLog.Log(std::format("Amplitude: {:.3f}", synth.mOscNode.mLastSample.amplitude));
      samples[i].sample.log = M7::gOscLog.mBuffer;
#endif  // ENABLE_OSC_LOG
    }

    std::vector<float> referenceLines = {-1.0f, -0.5f, 0.0f, 0.5f, 1.0f};

    ImGui::Text("Frequency: %.3f Hz", samples.empty() ? 0.0f : samples[0].sample.phaseAdvance.ComputeFrequencyHz());

    auto hoveredSample = WaveformViewImpl("wf",
                                          {io.DisplaySize.x - 20, 600},
                                          samples,
                                          {
                                              .referenceYValues = &referenceLines,
                                              .showLollipops = state.showLollipops,
                                              .showStems = state.showStems,
                                              .showLines = state.showLines,
                                              .showExtDots = state.showExtDots,
                                              .showEdgeEventLines = state.showEdgeEventLines,
                                              .autoRangeY = false,
                                              .yMin = -2.0f,
                                              .yMax = 2.0f,
                                          });

    std::string logContents;
    if (hoveredSample.has_value())
    {
      logContents = hoveredSample->sample.log;
    }

    // when the user uses a "copy" key combo (ctrl+c, ctrl+ins), copy the log text to clipboard
    if (ImGui::IsWindowFocused() && (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl)) &&
        (ImGui::IsKeyPressed(ImGuiKey_C) || ImGui::IsKeyPressed(ImGuiKey_Insert)))
    {
      ImGui::SetClipboardText(logContents.c_str());
    }

    ImGuiLogWindow(logContents, ImVec2{io.DisplaySize.x - 20, 400});
  }

  ImGui::End();
}

}  // namespace WaveSabreVstLib
