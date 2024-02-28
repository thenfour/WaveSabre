#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7WidthVst.h"

struct Maj7WidthEditor : public VstEditor
{
	Maj7Width* mpMaj7Width;

	Maj7WidthEditor(AudioEffect* audioEffect) : VstEditor(audioEffect, 1000, 800)
	{
		mpMaj7Width = ((Maj7WidthVst *)audioEffect)->GetMaj7Width();
	}

	virtual void renderImgui() override
	{
		//// in order of processing,
		//LeftSource, // 0 = left, 1 = right
		//	RightSource, // 0 = left, 1 = right
		//	SideHPFrequency,
		//	MidAmt,
		//	SideAmt,
		//	Pan,
		//	OutputGain,

		ImGui::Text("Select where the left / right channels originate. This can be used to swap L/R for example.");

		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LeftSource, "Left source", -1, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RightSource, "Right source", 1, 0, {});

		ImGui::Text("Rotate is just another way to manipulate the image. It may cause phase issues, or it may help center an imbalanced image, e.g. correcting weird mic placement.");
		Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle, "Rotation", -M7::math::gPI, M7::math::gPI, 0, 0, 0, {});

		ImGui::Text("This reduces width of low frequencies. Fixed at 6db/Oct slope.");
		Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency, -1, "Side HPF", M7::gFilterFreqConfig, 0, {});

		ImGui::Text("Mid/Side mixes. Ability to remove center channel, or widen / narrow image.");
		Maj7ImGuiParamFloat01((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidAmt, "Mid mix", 1, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideAmt, "Side mix", 1, 0, {});

		ImGui::Text("Final output panning");
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});
	//} M7::gFilterCenterFrequency, M7::gFilterFrequencyScale, 0);

		ImGui::Text("Final output gain");
		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain, "Output", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
	}

};

