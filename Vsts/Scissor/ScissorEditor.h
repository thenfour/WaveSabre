#ifndef __SCISSOREDITOR_H__
#define __SCISSOREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;


class ScissorEditor : public VstEditor
{
public:

	ScissorEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 600, 400)
	{
	}

	virtual void renderImgui() override
	{
		static constexpr char const* const typeCaptions[] = {
	"Clipper",
	"Sine",
	"Parabola",
		};
		static constexpr char const* const oversamplingCaptions[] = {
	"No oversampling",
	"2x",
	"4x",
		};


		WSImGuiParamEnumList((VstInt32)Scissor::ParamIndices::Type, "Type", 3, typeCaptions);
		ImGui::SameLine(0, 80);
		WSImGuiParamKnob((VstInt32)Scissor::ParamIndices::Drive, "Drive");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Scissor::ParamIndices::Threshold, "Threshold");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Scissor::ParamIndices::Foldover, "Fold");

		ImGui::SameLine(0, 80);
		WSImGuiParamKnob((VstInt32)Scissor::ParamIndices::DryWet, "Dry-Wet");
		WSImGuiParamEnumList((VstInt32)Scissor::ParamIndices::Oversampling, "Oversampling",3, oversamplingCaptions);
	}
};

#endif
