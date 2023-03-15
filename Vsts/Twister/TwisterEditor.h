#ifndef __TWISTEREDITOR_H__
#define __TWISTEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class TwisterEditor : public VstEditor
{
public:

	TwisterEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 900, 300)
	{
	}

	virtual void renderImgui() override
	{
		WSImGuiParamKnobInt((VstInt32)Twister::ParamIndices::Type, "Variation", 0, 3, 0, 0);

		ImGui::SameLine(0, 60);
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::LowCutFreq, "Lowcut", ParamBehavior::Frequency);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::HighCutFreq, "Highcut", ParamBehavior::Frequency);

		ImGui::SameLine(0, 60);
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::Amount, "Depth");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::Feedback, "Feedback");

		ImGui::SameLine(0, 60);
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::VibratoFreq, "VIB FREQ", ParamBehavior::VibratoFreq);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::VibratoAmount, "VIB AMT");

		ImGui::SameLine(0, 60);
		static constexpr char const* const spreadCaptions[] = {
			"Mono",
			"Invert",
			"ModInvert",
		};
		WSImGuiParamEnumList((VstInt32)Twister::ParamIndices::Spread, "Width", 3, spreadCaptions);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Twister::ParamIndices::DryWet, "DRY/WET");
	}
};

#endif
