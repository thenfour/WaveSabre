
#pragma once

#include <Windows.h>

#include <array>
#include <iomanip>
#include <iostream>

#include "WaveSabreVstLib/VstEditor.h"
#include "WaveSabreVstLib/VstPlug.h"

#include "WaveSabreVstLib/Serialization.hpp"
#include <WaveSabreVstLib/VUMeter.hpp>

namespace WaveSabreVstLib
{

template <int historyViewWidth, int historyViewHeight>
struct CompressorVis
{
  bool mShowTransferCurve = true;

  bool mShowInputHistory = true;
  bool mShowDetectorHistory = false;
  bool mShowOutputHistory = true;
  bool mShowAttenuationHistory = true;
  bool mShowThresh = true;
  bool mShowLeft = true;
  bool mShowRight = false;

  std::vector<VUMeterTick> mTickSet{
      {-3.0f, "3db"},
      {-6.0f, "6"},
      {-12.0f, "12"},
      {-20.0f, "20"},
      {-30.0f, "30"},
      {-40.0f, "40"},
  };

  //static constexpr int historyViewHeight = 110;
  static constexpr float historyViewMinDB = -60;
  HistoryView<9, historyViewWidth, historyViewHeight> mHistoryView;

  void Render(bool enabled,
              bool showToggles,
              MonoCompressor& comp,
              AnalysisStream (&inputAnalysis)[2],
              AnalysisStream (&detectorAnalysis)[2],
              AnalysisStream (&attenuationAnalysis)[2],
              AnalysisStream (&outputAnalysis)[2])
  {
    ImGuiIdScope __scope{"compressor_vis"};
    //INLINE float LookupLUT1D(const float(&mpTable)[N], float x)

    ImGui::BeginGroup();
    static constexpr float lineWidth = 2.0f;

    // ... transfer curve.
    if (mShowTransferCurve)
    {
      CompressorTransferCurve(comp, {historyViewHeight, historyViewHeight}, detectorAnalysis);
    }

    ImGui::SameLine();
    mHistoryView.Render(
        {
            // input
            HistoryViewSeriesConfig{ColorFromHTML("666666", mShowLeft && mShowInputHistory ? 0.6f : 0), lineWidth},
            HistoryViewSeriesConfig{ColorFromHTML("505050", mShowRight && mShowInputHistory ? 0.6f : 0), lineWidth},

            HistoryViewSeriesConfig{ColorFromHTML("ffcc00", mShowLeft && mShowDetectorHistory ? 0.8f : 0), lineWidth},
            HistoryViewSeriesConfig{ColorFromHTML("aa8800", mShowRight && mShowDetectorHistory ? 0.8f : 0), lineWidth},

            HistoryViewSeriesConfig{ColorFromHTML("cc3333", mShowLeft && mShowAttenuationHistory ? 0.8f : 0),
                                    lineWidth},
            HistoryViewSeriesConfig{ColorFromHTML("881111", mShowRight && mShowAttenuationHistory ? 0.8f : 0),
                                    lineWidth},

            HistoryViewSeriesConfig{ColorFromHTML("00cccc", mShowLeft && mShowOutputHistory ? 0.9f : 0), lineWidth},
            HistoryViewSeriesConfig{ColorFromHTML("009999", mShowRight && mShowOutputHistory ? 0.9f : 0), lineWidth},

            HistoryViewSeriesConfig{ColorFromHTML("ffff00", mShowThresh ? 0.6f : 0), 1.0f},
        },
        {
            float(inputAnalysis[0].mCurrentPeak),
            float(inputAnalysis[1].mCurrentPeak),
            float(detectorAnalysis[0].mCurrentPeak),
            float(detectorAnalysis[1].mCurrentPeak),
            float(attenuationAnalysis[0].mCurrentPeak),
            float(attenuationAnalysis[1].mCurrentPeak),
            float(outputAnalysis[0].mCurrentPeak),
            float(outputAnalysis[1].mCurrentPeak),
            M7::math::DecibelsToLinear(comp.mThreshold),
        });

    if (showToggles)
    {
      ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2, 0});
      ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

      ToggleButton(&mShowInputHistory, "Input");
      ImGui::SameLine();
      ToggleButton(&mShowDetectorHistory, "Detector");
      ImGui::SameLine();
      ToggleButton(&mShowAttenuationHistory, "Attenuation");
      ImGui::SameLine();
      ToggleButton(&mShowOutputHistory, "Output");

      ImGui::SameLine(0, 40);
      ToggleButton(&mShowLeft, "L");
      ImGui::SameLine();
      ToggleButton(&mShowRight, "R");

      ImGui::PopStyleVar(2);  // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
    }
    ImGui::EndGroup();

    ImGui::SameLine();

    VUMeterConfig mainCfg = {
        {13, historyViewHeight},
        VUMeterLevelMode::Audio,
        VUMeterUnits::Linear,
        -50,
        6,
        mTickSet,
    };

    VUMeterConfig attenCfg = mainCfg;
    attenCfg.levelMode = VUMeterLevelMode::Attenuation;

    VUMeterConfig disabledCfg = mainCfg;
    disabledCfg.levelMode = VUMeterLevelMode::Disabled;

    ImGui::SameLine();
    VUMeter("vu_inp", inputAnalysis[0], inputAnalysis[1], mainCfg);

    ImGui::SameLine();
    VUMeter("vu_atten", attenuationAnalysis[0], attenuationAnalysis[1], enabled ? attenCfg : disabledCfg);

    ImGui::SameLine();
    VUMeter("vu_outp", outputAnalysis[0], outputAnalysis[1], mainCfg);
  }
};

}  // namespace WaveSabreVstLib
