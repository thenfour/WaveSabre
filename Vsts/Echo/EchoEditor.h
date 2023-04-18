#ifndef __ECHOEDITOR_H__
#define __ECHOEDITOR_H__

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
		: VstEditor(audioEffect, 600, 400),
		mpEchoVST((EchoVst*)audioEffect)//,
	{
		mpEcho = (Echo*)mpEchoVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
	}

	virtual void PopulateMenuBar() override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Echo", mpEcho, mpEchoVST, "gDefaults16", "Echo::ParamIndices::NumParams", Echo::gJSONTagName, mpEcho->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		//WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::LeftDelayCoarse, "LEFT COARSE", 0, 16, 3, 0);
		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayCoarse, "Left coarse", Echo::gDelayCoarseCfg, 3, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayFine, "Left fine", Echo::gDelayFineCfg, 0, 0);
		//WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::LeftDelayFine, "LEFT FINE", 0, 200, 110, 0);
		ImGui::SameLine();
		//WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::RightDelayCoarse, "RIGHT COARSE", 0, 16, 4, 0);
		Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayCoarse, "Right coarse", Echo::gDelayCoarseCfg, 4, 0);
		ImGui::SameLine();
		//WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::RightDelayFine, "RIGHT FINE", 0, 200, 80, 0);
		Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayFine, "Right fine", Echo::gDelayFineCfg, 0, 0);

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::Feedback, "FEEDBACK");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::Cross, "CROSS");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::DryWet, "DRY/WET");


		Maj7ImGuiParamFrequency((int)Echo::ParamIndices::LowCutFreq, -1, "Lowcut", M7::gFilterFreqConfig, 0, {});
		//WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::LowCutFreq, "LC FREQ", ParamBehavior::Frequency);
		ImGui::SameLine();
		Maj7ImGuiParamFrequency((int)Echo::ParamIndices::HighCutFreq, -1, "Highcut", M7::gFilterFreqConfig, 1, {});
		//WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::HighCutFreq, "HC FREQ", ParamBehavior::Frequency);

	}



};

#endif
