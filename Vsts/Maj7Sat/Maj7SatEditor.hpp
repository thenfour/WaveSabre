#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7SatVst.hpp"

struct Maj7SatEditor : public VstEditor
{
	Maj7Sat* mpMaj7Sat;
	Maj7SatVst* mpMaj7SatVst;

	Maj7SatEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1000, 800),
		mpMaj7SatVst((Maj7SatVst*)audioEffect)
	{
		mpMaj7Sat = ((Maj7SatVst *)audioEffect)->GetMaj7Sat();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Sat Saturator", mpMaj7Sat, mpMaj7SatVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Sat", mpMaj7Sat->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::Text("todo.");
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SatEditor(audioEffect);
}
