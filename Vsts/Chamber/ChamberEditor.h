#ifndef __CHAMBEREDITOR_H__
#define __CHAMBEREDITOR_H__

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>

using namespace WaveSabreVstLib;

class ChamberEditor : public VstEditor
{
public:

	ChamberEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 600)
	{
	}

	~ChamberEditor()
	{
	}

	virtual void renderImgui() override
	{
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Chamber::ParamIndices::Feedback, "Feedback");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Chamber::ParamIndices::PreDelay, "Pre delay");
		ImGui::SameLine();
		WSImGuiParamKnobInt((VstInt32)WaveSabreCore::Chamber::ParamIndices::Mode, "Variation", 0, 2, 0);

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Chamber::ParamIndices::LowCutFreq, "Lowcut", ParamBehavior::Frequency);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)WaveSabreCore::Chamber::ParamIndices::HighCutFreq, "Highcut", ParamBehavior::Frequency);

		WSImGuiParamKnob((VstInt32)WaveSabreCore::Chamber::ParamIndices::DryWet, "Dry/wet");
	}
};

#endif
