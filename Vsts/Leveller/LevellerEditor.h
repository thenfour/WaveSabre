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

	ColorMod mEnabledColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

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

	void RenderBand(int id, const char *labelWhenZero, const char *labelWhenNonZero, LevellerParamIndices paramOffset, float defaultFreqParamHz)
	{
		ImGui::TableNextColumn();
		ImGui::PushID(id);

		float enabledBacking;
		M7::BoolParam bp{ enabledBacking };
		bp.SetRawParamValue(GetEffectX()->getParameter((int)paramOffset + (int)LevellerBandParamOffsets::Enable));
		ColorMod& cm = bp.GetBoolValue() ? mEnabledColors : mDisabledColors;

		auto token = cm.Push();

		if (BeginTabBar2("##tabs", ImGuiTabBarFlags_None))
		{
			//WaveSabreCore::M7::real_t tempVal;
			//M7::VolumeParam p{ tempVal, cfg };
			float gainParam = GetEffectX()->getParameter((int)paramOffset + (int)LevellerBandParamOffsets::Gain);
			auto label = gainParam <= 0 ? labelWhenZero : labelWhenNonZero;

			if (WSBeginTabItem(label)) {
				float f1 = 0, f2 = 0;
				M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };// :gFilterCenterFrequency, M7::gFilterFrequencyScale};
				fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);

				Maj7ImGuiParamFrequency((int)paramOffset + (int)LevellerBandParamOffsets::Freq, -1, "Freq", M7::gFilterFreqConfig, f1, {});
				Maj7ImGuiParamVolume((int)paramOffset + (int)LevellerBandParamOffsets::Gain, "Gain", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
				WSImGuiParamKnob((int)paramOffset + (int)LevellerBandParamOffsets::Q, "Q");

				WSImGuiParamCheckbox((int)paramOffset + (int)LevellerBandParamOffsets::Enable, "Enable?");

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
		ImGui::PopID();
	}

	static constexpr ImVec2 gHistViewSize = { 550, 150 };

	void ResponseGraph()
	{
		ImRect bb;
		bb.Min = ImGui::GetCursorPos();
		bb.Max = bb.Min + gHistViewSize;

		ImColor backgroundColor = ColorFromHTML("222222", 1.0f);
		ImColor lineColor = ColorFromHTML("ff8800", 1.0f);

		auto* dl = ImGui::GetWindowDrawList();

		ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

		float underlyingValue = 0;
		float ktdummy = 0;
		M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig };

		static constexpr int gSegmentCount = 50;
		static constexpr int segmentWidth = (int)(gHistViewSize.x / gSegmentCount);

		for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
			underlyingValue = float(iseg) / gSegmentCount;
			float freq = param.GetFrequency(0, 0);
			float magdB = 0;
			for (auto& b : mpLeveller->mBands) {
				if (b.mIsEnabled) {
					magdB += M7::math::LinearToDecibels(b.mFilters[0].calcMagnitude(freq));
				}
			}
			float magLin = M7::math::DecibelsToLinear(magdB);
			ImRect rc;
			rc.Min.x = (bb.Min.x + iseg * bb.GetWidth() / gSegmentCount);
			rc.Min.y = bb.Max.y - bb.GetHeight() * M7::math::clamp01(magLin);
			rc.Max.x = rc.Min.x + segmentWidth;
			rc.Max.y = bb.Max.y;
			dl->AddRectFilled(rc.Min, rc.Max, lineColor);
		}

		ImGui::Dummy(gHistViewSize);
	}


	virtual void renderImgui() override
	{
		mEnabledColors.EnsureInitialized();
		mDisabledColors.EnsureInitialized();

		if (BeginTabBar2("master", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Master")) {
				Maj7ImGuiParamVolume((VstInt32)LevellerParamIndices::MasterVolume, "Master volume", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}

		static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
		char text[1024] = "ok? text here?";
		ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(400, ImGui::GetTextLineHeight() * 16), flags);

		ImGui::BeginTable("##fmaux", Leveller::gBandCount);
		ImGui::TableNextRow();

		RenderBand(0, "High pass###hp", "Low shelf###hp", LevellerParamIndices::LowShelfFreq, 175);
		RenderBand(1, "Peak 1", "Peak 1", LevellerParamIndices::Peak1Freq, 650);
		RenderBand(2, "Peak 2", "Peak 2", LevellerParamIndices::Peak2Freq, 2000);
		RenderBand(3, "Peak 3", "Peak 3", LevellerParamIndices::Peak3Freq, 7000);
		RenderBand(4, "Low pass###lp", "High shelf###lp", LevellerParamIndices::HighShelfFreq, 6000);

		ImGui::EndTable();

		//ResponseGraph();
	}


};

#endif
