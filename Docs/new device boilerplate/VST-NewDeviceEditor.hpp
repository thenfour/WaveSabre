#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7CompVst.hpp"

struct Maj7CompEditor : public VstEditor
{
	Maj7Comp* mpMaj7Comp;
	Maj7CompVst* mpMaj7CompVst;

	Maj7CompEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1000, 800),
		mpMaj7CompVst((Maj7CompVst*)audioEffect)
	{
		mpMaj7Comp = ((Maj7CompVst *)audioEffect)->GetMaj7Comp();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Comp Compressor", mpMaj7Comp, mpMaj7CompVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Comp", mpMaj7Comp->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::Text("todo.");
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
