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
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Width, "Width", 1);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0);
		ImGui::SameLine(); Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency, -1, "Side HPF", M7::gFilterCenterFrequency, M7::gFilterFrequencyScale, 0);
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain, "Output", WaveSabreCore::Maj7Width::gMaxOutputVolumeDb, 0);
	}

};

