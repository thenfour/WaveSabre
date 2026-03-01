#ifndef __ECHOEDITOR_H__
#define __ECHOEDITOR_H__

#include <WaveSabreVstLib.h>
#include <functional>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "EchoVst.h"
#include <WaveSabreVstLib/DelayVisualization.hpp>

class EchoEditor : public VstEditor
{
  Echo* mpEcho;
  EchoVst* mpEchoVST;

public:
  EchoEditor(AudioEffect* audioEffect)
      : VstEditor(audioEffect, 570, 460)
      , mpEchoVST((EchoVst*)audioEffect)  //,
  {
    mpEcho =
        (Echo*)mpEchoVST
            ->getDevice();  // for some reason this doesn't work as initialization but has to be called in ctor body like this.
  }

  virtual void PopulateMenuBar() override
  {
    ECHO_PARAM_VST_NAMES(paramNames);
    PopulateStandardMenuBar(mCurrentWindow,
                            "Echo",
                            mpEcho,
                            mpEchoVST,
                            "gDefaults16",
                            "Echo::ParamIndices::NumParams",
                            mpEcho->mParamCache,
                            paramNames);
  }

  virtual void renderImgui() override
  {
    ImGui::BeginGroup();

    Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayCoarse, "Left 8ths", DelayCore::gDelayCoarseCfg, 8, 0);
    ImGui::SameLine();
    Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::LeftDelayFine, "(fine)##left", 0, 0, {});
    ImGui::SameLine();
    Maj7ImGuiBipolarPowCurvedParam((int)Echo::ParamIndices::LeftDelayMS, "(ms)##left", M7::gEnvTimeCfg, 0, {});

    ImGui::SameLine(0, 60);
    Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayCoarse, "Right 8ths", DelayCore::gDelayCoarseCfg, 6, 0);
    ImGui::SameLine();
    Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::RightDelayFine, "(fine)##right", 0, 0, {});
    ImGui::SameLine();
    Maj7ImGuiBipolarPowCurvedParam((int)Echo::ParamIndices::RightDelayMS, "(ms)##right", M7::gEnvTimeCfg, 0, {});

    Maj7ImGuiParamVolume(
        (VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackLevel, "Feedback", M7::gVolumeCfg6db, -15.0f, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        (VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackDriveDB, "FB Drive", M7::gVolumeCfg36db, 3, {});
    ImGui::SameLine();
    Maj7ImGuiParamFloat01((VstInt32)WaveSabreCore::Echo::ParamIndices::Cross, "crosstalk", 0.25f, 0);

    M7::QuickParam fp{M7::gFilterFreqConfig};
    fp.SetFrequencyAssumingNoKeytracking(50);

    Maj7ImGuiParamFrequency(
        (int)Echo::ParamIndices::LowCutFreq, -1, "Lowcut", M7::gFilterFreqConfig, fp.GetRawValue(), {});
    ImGui::SameLine();
    Maj7ImGuiDivCurvedParam((int)Echo::ParamIndices::LowCutQ, "Low Q", M7::gBiquadFilterQCfg, 0.75f, {});


    fp.SetFrequencyAssumingNoKeytracking(8500);
    ImGui::SameLine();
    Maj7ImGuiParamFrequency(
        (int)Echo::ParamIndices::HighCutFreq, -1, "Highcut", M7::gFilterFreqConfig, fp.GetRawValue(), {}, 1);
    ImGui::SameLine();
    Maj7ImGuiDivCurvedParam((int)Echo::ParamIndices::HighCutQ, "High Q", M7::gBiquadFilterQCfg, 0.75f, {});

    Maj7ImGuiParamVolume(
        (VstInt32)WaveSabreCore::Echo::ParamIndices::DryOutput, "Dry output", M7::gVolumeCfg12db, 0, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        (VstInt32)WaveSabreCore::Echo::ParamIndices::WetOutput, "Wet output", M7::gVolumeCfg12db, -12, {});

    ImGui::EndGroup();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    auto& iaL = mpEcho->mInputAnalysis[0];
    auto& iaR = mpEcho->mInputAnalysis[1];
    auto& oaL = mpEcho->mOutputAnalysis[0];
    auto& oaR = mpEcho->mOutputAnalysis[1];
#else
    AnalysisStream iaL{};  // mocks
    AnalysisStream iaR{};
    AnalysisStream oaL{};
    AnalysisStream oaR{};
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    ImGui::SameLine();
    VUMeter("vu_inp", iaL, iaR);
    ImGui::SameLine();
    VUMeter("vu_outp", oaL, oaR);

    using ParamIndices = WaveSabreCore::Echo::ParamIndices;
    float leftBufferLengthMs = DelayCore::CalcDelayMS(mpEcho->mParams,
                                                      (int)ParamIndices::LeftDelayCoarse);
    float rightBufferLengthMs = DelayCore::CalcDelayMS(mpEcho->mParams,
                                                       (int)ParamIndices::RightDelayCoarse);

    DelayVisualization(leftBufferLengthMs,
                       rightBufferLengthMs,
                       mpEcho->mParams.GetLinearVolume(ParamIndices::FeedbackLevel, M7::gVolumeCfg6db),
                       mpEcho->mParams.Get01Value(ParamIndices::Cross));
  }

};  // EchoEditor

#endif
