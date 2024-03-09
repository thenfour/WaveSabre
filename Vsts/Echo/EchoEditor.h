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
		: VstEditor(audioEffect, 570, 450),
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
		ImGui::BeginGroup();

		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayCoarse, "Left 8ths", Echo::gDelayCoarseCfg, 8, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::LeftDelayFine, "(fine)##left", 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)Echo::ParamIndices::LeftDelayMS, "(ms)##left", -200, 200, 0, 0, 0, {});

		ImGui::SameLine(0, 60); Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayCoarse, "Right 8ths", Echo::gDelayCoarseCfg, 6, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::RightDelayFine, "(fine)##right", 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)Echo::ParamIndices::RightDelayMS, "(ms)##right", -200, 200, 0, 0, 0, {});



		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackLevel, "Feedback", M7::gVolumeCfg6db, -15.0f, {});
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackDriveDB, "FB Drive", M7::gVolumeCfg12db, 3, {});
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
		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::WetOutput, "Wet output", M7::gVolumeCfg12db, -12, {});

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter("vu_inp", mpEcho->mInputAnalysis[0], mpEcho->mInputAnalysis[1]);
		ImGui::SameLine(); VUMeter("vu_outp", mpEcho->mOutputAnalysis[0], mpEcho->mOutputAnalysis[1]);


	}

}; // EchoEditor

#endif
