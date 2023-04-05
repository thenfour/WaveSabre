#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "LevellerVst.h"

class LevellerEditor : public VstEditor
{
	Leveller* mpLeveller;
	LevellerVst* mpLevellerVST;
public:
	LevellerEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 700, 500),
		mpLevellerVST((LevellerVst*)audioEffect)//,
	{
		mpLeveller = (Leveller*)mpLevellerVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
	}

	virtual void PopulateMenuBar() override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Leveller", mpLeveller, mpLevellerVST, "gLevellerDefaults16", "LevellerParamIndices::NumParams", "WSLeveller", mpLeveller->mParamCache, paramNames);
	}

	void RenderBand(int id, const char *label, LevellerParamIndices paramOffset, float defaultFreqParamHz, bool showGain)
	{
		ImGui::TableNextColumn();
		ImGui::PushID(id);
		if (BeginTabBar2("##tabs", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem(label)) {

				float f1 = 0, f2 = 0;
				M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };// :gFilterCenterFrequency, M7::gFilterFrequencyScale};
				fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);

				Maj7ImGuiParamFrequency((int)paramOffset + (int)LevellerBandParamOffsets::Freq, -1, "Freq", M7::gFilterFreqConfig, f1);
				if (showGain) {
					Maj7ImGuiParamVolume((int)paramOffset + (int)LevellerBandParamOffsets::Gain, "Gain", WaveSabreCore::gLevellerBandVolumeCfg, 0);
				}
				else {
					ImGui::Dummy({ 60,60 });
				}
				WSImGuiParamKnob((int)paramOffset + (int)LevellerBandParamOffsets::Q, "Q");

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
		ImGui::PopID();
	}

	virtual void renderImgui() override
	{

		if (BeginTabBar2("master", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Master")) {
				Maj7ImGuiParamVolume((VstInt32)LevellerParamIndices::MasterVolume, "Master volume", WaveSabreCore::gLevellerVolumeCfg, 0);
				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginTable("##fmaux", Leveller::gBandCount);
		ImGui::TableNextRow();

		RenderBand(0, "low cut", LevellerParamIndices::LowCutFreq, 0, false);
		RenderBand(1, "band 2", LevellerParamIndices::Peak1Freq, 650, true);
		RenderBand(2, "band 3", LevellerParamIndices::Peak2Freq, 2000, true);
		RenderBand(3, "band 4", LevellerParamIndices::Peak3Freq, 7000, true);
		RenderBand(4, "high cut", LevellerParamIndices::HighCutFreq, 22050, false);

		ImGui::EndTable();
	}


};

#endif
