#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7SpaceVst.h"
#include <WaveSabreVstLib/DelayVisualization.hpp>

struct Maj7SpaceEditor : public VstEditor
{
	Maj7Space* mpMaj7Space;
	Maj7SpaceVst* mpMaj7SpaceVst;
    using ParamIndices = WaveSabreCore::Maj7Space::ParamIndices;

	Maj7SpaceEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7SpaceVst((Maj7SpaceVst*)audioEffect)
	{
		mpMaj7Space = ((Maj7SpaceVst *)audioEffect)->GetMaj7Space();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7SPACE_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Space", mpMaj7Space, mpMaj7SpaceVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Space->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
    ImGui::BeginGroup();

    Maj7ImGuiParamInt((int)ParamIndices::DelayLeftDelayCoarse, "Left 8ths", DelayCore::gDelayCoarseCfg, 8, 0);
    ImGui::SameLine();
    Maj7ImGuiParamFloatN11((int)ParamIndices::DelayLeftDelayFine, "(fine)##left", 0, 0, {});
    ImGui::SameLine();
    Maj7ImGuiBipolarPowCurvedParam((int)ParamIndices::DelayLeftDelayMS, "(ms)##left", M7::gEnvTimeCfg, 0, {});

    ImGui::SameLine(0, 60);
    Maj7ImGuiParamInt((int)ParamIndices::DelayRightDelayCoarse, "Right 8ths", DelayCore::gDelayCoarseCfg, 6, 0);
    ImGui::SameLine();
    Maj7ImGuiParamFloatN11((int)ParamIndices::DelayRightDelayFine, "(fine)##right", 0, 0, {});
    ImGui::SameLine();
    Maj7ImGuiBipolarPowCurvedParam((int)ParamIndices::DelayRightDelayMS, "(ms)##right", M7::gEnvTimeCfg, 0, {});

    Maj7ImGuiParamVolume(
        (VstInt32)ParamIndices::DelayFeedbackLevel, "Feedback", M7::gVolumeCfg6db, -15.0f, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        (VstInt32)ParamIndices::DelayFeedbackDriveDB, "FB Drive", M7::gVolumeCfg36db, 3, {});
    ImGui::SameLine();
    Maj7ImGuiParamFloat01((VstInt32)ParamIndices::DelayCross, "crosstalk", 0.25f, 0);

    M7::QuickParam fp{M7::gFilterFreqConfig};
    fp.SetFrequencyAssumingNoKeytracking(50);

    Maj7ImGuiParamFrequency(
        (int)ParamIndices::DelayLowCutFreq, -1, "Lowcut", M7::gFilterFreqConfig, fp.GetRawValue(), {});
    ImGui::SameLine();
    Maj7ImGuiDivCurvedParam((int)ParamIndices::DelayLowCutQ, "Low Q", M7::gBiquadFilterQCfg, 0.75f, {});

    fp.SetFrequencyAssumingNoKeytracking(8500);
    ImGui::SameLine();
    Maj7ImGuiParamFrequency(
        (int)ParamIndices::DelayHighCutFreq, -1, "Highcut", M7::gFilterFreqConfig, fp.GetRawValue(), {}, 1);
    ImGui::SameLine();
    Maj7ImGuiDivCurvedParam((int)ParamIndices::DelayHighCutQ, "High Q", M7::gBiquadFilterQCfg, 0.75f, {});

    ImGui::EndGroup();

    ImGui::SameLine();
    VUMeter("vu_inp", mpMaj7Space->mInputAnalysis[0], mpMaj7Space->mInputAnalysis[1]);
    ImGui::SameLine();
    VUMeter("vu_outp", mpMaj7Space->mOutputAnalysis[0], mpMaj7Space->mOutputAnalysis[1]);

    float leftBufferLengthMs = DelayCore::CalcDelayMS(mpMaj7Space->mParams,
                                                      (int)ParamIndices::DelayLeftDelayCoarse);
    float rightBufferLengthMs = DelayCore::CalcDelayMS(mpMaj7Space->mParams,
                                                       (int)ParamIndices::DelayRightDelayCoarse);

    DelayVisualization(leftBufferLengthMs,
                       rightBufferLengthMs,
                       mpMaj7Space->mParams.GetLinearVolume(ParamIndices::DelayFeedbackLevel, M7::gVolumeCfg6db),
                       mpMaj7Space->mParams.Get01Value(ParamIndices::DelayCross));


    Maj7ImGuiParamVolume(
        (VstInt32)ParamIndices::DryVolume, "Dry output", M7::gVolumeCfg12db, 0, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        (VstInt32)ParamIndices::DelayVolume, "Delay output", M7::gVolumeCfg12db, -12, {});
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        (VstInt32)ParamIndices::ReverbVolume, "Reverb output", M7::gVolumeCfg12db, -12, {});
    }
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SpaceEditor(audioEffect);
}
