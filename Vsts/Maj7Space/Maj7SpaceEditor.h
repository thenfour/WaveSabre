#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7SpaceVst.h"

struct Maj7SpaceEditor : public VstEditor
{
	Maj7Space* mpMaj7Space;
	Maj7SpaceVst* mpMaj7SpaceVst;

	Maj7SpaceEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7SpaceVst((Maj7SpaceVst*)audioEffect)
	{
		mpMaj7Space = ((Maj7SpaceVst *)audioEffect)->GetMaj7Space();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7SPACE_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Space", mpMaj7Space, mpMaj7SpaceVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Space->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SpaceEditor(audioEffect);
}
