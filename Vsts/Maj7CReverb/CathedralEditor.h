#ifndef __CATHEDRALEDITOR_H__
#define __CATHEDRALEDITOR_H__

#include <WaveSabreCore.h>
#include <WaveSabreVstLib.h>

using namespace WaveSabreVstLib;

#include "CathedralVst.h"

class CathedralEditor : public VstEditor {
  Cathedral *mpCathedral;
  CathedralVst *mpCathedralVST;

public:
  CathedralEditor(AudioEffect *audioEffect)
      : VstEditor(audioEffect, 800, 600),
        mpCathedralVST((CathedralVst *)audioEffect) //,
  {
    mpCathedral = (Cathedral *)mpCathedralVST->getDevice();
  }

  virtual void PopulateMenuBar() override {
    CATHEDRAL_PARAM_VST_NAMES(paramNames);
    PopulateStandardMenuBar(mCurrentWindow, "Maj7 - C Reverb", mpCathedral,
                            mpCathedralVST, "gParamDefaults",
                            "Cathedral::ParamIndices::NumParams",
                            mpCathedral->mParamCache,
        paramNames);
  }

  virtual void renderImgui() override {
    ImGui::BeginGroup();

    using ParamIndices = WaveSabreCore::Cathedral::ParamIndices;

    M7::QuickParam fp{M7::gFilterFreqConfig};

    fp.SetFrequencyAssumingNoKeytracking(145);
    Maj7ImGuiParamFrequency((int)ParamIndices::LowCutFreq, -1, "HP Freq(Hz)",
                            M7::gFilterFreqConfig, fp.GetRawValue(), {});
    ImGui::SameLine();

    fp.SetFrequencyAssumingNoKeytracking(5500);
    Maj7ImGuiParamFrequency((int)ParamIndices::HighCutFreq, -1, "LP Freq(Hz)",
                            M7::gFilterFreqConfig, fp.GetRawValue(), {}, 1);

    Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::PreDelay, "PreDelay(ms)", 0, 500, 1, 1, 0, {});
    ImGui::SameLine();

    Maj7ImGuiParamFloat01((VstInt32)ParamIndices::RoomSize, "RoomSize", 0.5f, 0);
    ImGui::SameLine();

    Maj7ImGuiParamFloat01((VstInt32)ParamIndices::Damp, "Damp", 0.15f, 0);
    ImGui::SameLine();

    Maj7ImGuiParamFloat01((VstInt32)ParamIndices::Width, "Width", 0.9f, 0);

    Maj7ImGuiParamVolume((VstInt32)ParamIndices::DryOut, "Dry output",
                         M7::gVolumeCfg12db, 0, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume((VstInt32)ParamIndices::WetOut, "Wet output",
                         M7::gVolumeCfg12db, -6, {});

    ImGui::EndGroup();

    ImGui::SameLine();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    auto& iaL = mpCathedral->mInputAnalysis[0];
    auto& iaR = mpCathedral->mInputAnalysis[1];
    auto& oaL = mpCathedral->mOutputAnalysis[0];
    auto& oaR = mpCathedral->mOutputAnalysis[1];
#else
    AnalysisStream iaL {}; // mocks
    AnalysisStream iaR {};
    AnalysisStream oaL {};
    AnalysisStream oaR {};
#endif
    {
      VUMeterTooltipStripScope tooltipStrip{"cathedral_vu_strip"};
      VUMeter("vu_inp", iaL, iaR, {30, 300}, "Input Left", "Input Right", nullptr, &tooltipStrip);

      ImGui::SameLine();
      VUMeter("vu_outp", oaL, oaR, {30, 300}, "Output Left", "Output Right", nullptr, &tooltipStrip);
    }
  }
};

#endif
