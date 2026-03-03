#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7ModulateVst.hpp"

struct Maj7ModulateEditor : public VstEditor
{
	Maj7Modulate* mpMaj7Modulate;
	Maj7ModulateVst* mpMaj7ModulateVst;

	Maj7ModulateEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7ModulateVst((Maj7ModulateVst*)audioEffect)
	{
		mpMaj7Modulate = ((Maj7ModulateVst *)audioEffect)->GetMaj7Modulate();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7MODULATE_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Modulate", mpMaj7Modulate, mpMaj7ModulateVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Modulate->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
		ImGui::Text("coming soon... a builder to make vibrato, tremolo, auto-pan, phaser, flanger, chorus, ensemble, doubler, etc effects");
	}
};
