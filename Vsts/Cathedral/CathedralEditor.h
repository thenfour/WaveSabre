#ifndef __CATHEDRALEDITOR_H__
#define __CATHEDRALEDITOR_H__

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>
using namespace WaveSabreVstLib;

class CathedralEditor : public VstEditor
{
public:
	CathedralEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 600)
	{
	}

	virtual void renderImgui() override
	{
		WSImGuiParamCheckbox((VstInt32)WaveSabreCore::Cathedral::ParamIndices::Freeze, "Freeze");

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::PreDelay, "Pre delay");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::RoomSize, "Size");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::Damp, "Damp");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::Width, "Width");

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::LowCutFreq, "Lowcut", ParamBehavior::Frequency);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::HighCutFreq, "Highcut", ParamBehavior::Frequency);

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Cathedral::ParamIndices::DryWet, "Dry/Wet");
	}

};

#endif
