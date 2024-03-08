#ifndef __ECHOEDITOR_H__
#define __ECHOEDITOR_H__

#include <functional>
#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "EchoVst.h"

class EchoEditor : public VstEditor
{
	Echo* mpEcho;
	EchoVst* mpEchoVST;
public:
	EchoEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 700, 900),
		mpEchoVST((EchoVst*)audioEffect)//,
	{
		mpEcho = (Echo*)mpEchoVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
	}

	virtual void PopulateMenuBar() override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Echo", mpEcho, mpEchoVST, "gDefaults16", "Echo::ParamIndices::NumParams", mpEcho->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayCoarse, "Left coarse", Echo::gDelayCoarseCfg, 3, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayFine, "Left fine", Echo::gDelayFineCfg, 0, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayCoarse, "Right coarse", Echo::gDelayCoarseCfg, 4, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayFine, "Right fine", Echo::gDelayFineCfg, 0, 0);

		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackLevel, "Feedback", M7::gVolumeCfg6db, -5, {});
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackDriveDB, "FB Drive", M7::gVolumeCfg12db, 0, {});
		ImGui::SameLine();
		Maj7ImGuiParamFloat01((VstInt32)WaveSabreCore::Echo::ParamIndices::Cross, "crosstalk", 0.25f, 0);

		float f1 = 0, f2 = 0;
		M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };
		fp.SetFrequencyAssumingNoKeytracking(50);

		Maj7ImGuiParamFrequency((int)Echo::ParamIndices::LowCutFreq, -1, "Lowcut", M7::gFilterFreqConfig, f1, {});
		ImGui::SameLine(); Maj7ImGuiParamFloat01((int)Echo::ParamIndices::LowCutQ, "Low Q", 0.2f, 0.2f);

		fp.SetFrequencyAssumingNoKeytracking(8500);
		ImGui::SameLine(); Maj7ImGuiParamFrequency((int)Echo::ParamIndices::HighCutFreq, -1, "Highcut", M7::gFilterFreqConfig, f1, {});
		ImGui::SameLine(); Maj7ImGuiParamFloat01((int)Echo::ParamIndices::HighCutQ, "High Q", 0.2f, 0.2f);

		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::DryOutput, "Dry output", M7::gVolumeCfg12db, 0, {});
		ImGui::SameLine();
		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::WetOutput, "Wet output", M7::gVolumeCfg12db, -6, {});

	}

}; // EchoEditor

#endif
