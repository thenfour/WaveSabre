#ifndef __ECHOEDITOR_H__
#define __ECHOEDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class EchoEditor : public VstEditor
{
public:


	EchoEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 600, 400)
	{
	}

	~EchoEditor()
	{
	}


	virtual void renderImgui() override
	{
		WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::LeftDelayCoarse, "LEFT COARSE", 0, 16, 3);
		ImGui::SameLine();
		WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::LeftDelayFine, "LEFT FINE", 0, 200, 110);
		ImGui::SameLine();
		WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::RightDelayCoarse, "RIGHT COARSE", 0, 16, 4);
		ImGui::SameLine();
		WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Echo::ParamIndices::RightDelayFine, "RIGHT FINE", 0, 200, 80);

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::Feedback, "FEEDBACK");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::Cross, "CROSS");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::DryWet, "DRY/WET");

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::LowCutFreq, "LC FREQ", ParamBehavior::Frequency);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Echo::ParamIndices::HighCutFreq, "HC FREQ", ParamBehavior::Frequency);

	}



};

#endif
