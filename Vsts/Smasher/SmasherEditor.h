#ifndef __SMASHEREDITOR_H__
#define __SMASHEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class SmasherEditor : public VstEditor
{
public:


	SmasherEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 300)
	{
	}

	virtual void renderImgui() override
	{
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::InputGain, "INPUT GAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Threshold, "THRESHOLD");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Ratio, "RATIO");

		ImGui::SameLine(0, 80);
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Attack, "ATTACK");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Release, "RELEASE");

		ImGui::SameLine(0,80);
		WSImGuiParamCheckbox((VstInt32)Smasher::ParamIndices::Sidechain, "SIDECHAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::OutputGain, "OUTPUT GAIN");
	}

};

#endif
