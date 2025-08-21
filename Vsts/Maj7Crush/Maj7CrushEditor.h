#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7CrushVst.h"

struct Maj7CrushEditor : public VstEditor
{
	Maj7Crush* mpMaj7Crush;
	Maj7CrushVst* mpMaj7CrushVst;

	Maj7CrushEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7CrushVst((Maj7CrushVst*)audioEffect)
	{
		mpMaj7Crush = ((Maj7CrushVst *)audioEffect)->GetMaj7Crush();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7CRUSH_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Crush", mpMaj7Crush, mpMaj7CrushVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Crush->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CrushEditor(audioEffect);
}
