#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7AnalyzeVst.h"

struct Maj7AnalyzeEditor : public VstEditor
{
	Maj7Analyze* mpMaj7Analyze;
	Maj7AnalyzeVst* mpMaj7AnalyzeVst;

	Maj7AnalyzeEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7AnalyzeVst((Maj7AnalyzeVst*)audioEffect)
	{
		mpMaj7Analyze = ((Maj7AnalyzeVst *)audioEffect)->GetMaj7Analyze();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Analyze", mpMaj7Analyze, mpMaj7AnalyzeVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Analyze->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7AnalyzeEditor(audioEffect);
}
