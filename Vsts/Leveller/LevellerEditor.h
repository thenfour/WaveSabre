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
		: VstEditor(audioEffect, 700, 600),
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

	// the response graph is extremely crude; todo:
	// - add the user-selected points to the points list explicitly, to give better looking peaks. then you could reduce # of points.
	// - respect 'thru' member.
	// - display info about freq range and amplitude
	// - adjust amplitude

	static constexpr ImVec2 gHistViewSize = { 350, 125 };

	static constexpr int gSegmentCount = 150;
	static constexpr int segmentWidth = (int)(gHistViewSize.x / gSegmentCount);

	// because params change as a result of this immediate gui, we need at least 1 full frame of lag to catch param changes correctly.
	// so keep processing multiple frames until things settle. in the meantime, force recalculating.
	int additionalForceCalcFrames = 0;
	ImVec2 points[gSegmentCount];
	float mParamCacheCache[(size_t)LevellerParamIndices::NumParams] = { 0 }; // the param cache that points have been calculated on. this way we can only recalc the freq response when params change.


	// https://forum.juce.com/t/draw-frequency-response-of-filter-from-transfer-function/20669
	// https://www.musicdsp.org/en/latest/Analysis/186-frequency-response-from-biquad-coefficients.html
	// https://dsp.stackexchange.com/questions/3091/plotting-the-magnitude-response-of-a-biquad-filter
	float mag2(WaveSabreCore::BiquadFilter& bq, double freq) const {
		static constexpr double tau = 6.283185307179586476925286766559;
		auto w = tau * freq / Helpers::CurrentSampleRate;

		double ma1 = double(bq.coeffs[1]) / bq.coeffs[0];
		double ma2 = double(bq.coeffs[2]) / bq.coeffs[0];
		double mb0 = double(bq.coeffs[3]) / bq.coeffs[0];
		double mb1 = double(bq.coeffs[4]) / bq.coeffs[0];
		double mb2 = double(bq.coeffs[5]) / bq.coeffs[0];

		double numerator = mb0 * mb0 + mb1 * mb1 + mb2 * mb2 + 2 * (mb0 * mb1 + mb1 * mb2) * ::cos(w) + 2 * mb0 * mb2 * ::cos(2 * w);
		double denominator = 1 + ma1 * ma1 + ma2 * ma2 + 2 * (ma1 + ma1 * ma2) * ::cos(w) + 2 * ma2 * ::cos(2 * w);
		double magnitude = ::sqrt(numerator / denominator);
		return (float)magnitude;
	}

	void CalculatePoints(ImRect& bb) {
		float underlyingValue = 0;
		float ktdummy = 0;
		M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig };

		for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
			underlyingValue = float(iseg) / gSegmentCount;
			float freq = param.GetFrequency(0, 0);
			float magdB = 0;
			for (auto& b : mpLeveller->mBands) {
				if (b.mIsEnabled && !b.mFilters[0].thru) {
					magdB += M7::math::LinearToDecibels(mag2(b.mFilters[0], freq));
				}
			}
			float magLin = M7::math::DecibelsToLinear(magdB) / 4; // /4 to basically give a 12db range of display.

			points[iseg] = ImVec2(
				(bb.Min.x + iseg * bb.GetWidth() / gSegmentCount),
				bb.Max.y - bb.GetHeight() * M7::math::clamp01(magLin)
			);
		}
	}

	void EnsurePointsPopulated(ImRect& bb) {
		bool areEqual = true;

		float paramCacheCopy[gSegmentCount];
		for (size_t i = 0; i < (size_t)LevellerParamIndices::NumParams; ++i) {
			paramCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
		}

		for (size_t i = 0; i < (size_t)LevellerParamIndices::NumParams; ++i) {
			if (paramCacheCopy[i] != mParamCacheCache[i]) {
				areEqual = false;
				break;
			}
		}
		if (areEqual && (additionalForceCalcFrames == 0)) return;

		additionalForceCalcFrames = areEqual ? additionalForceCalcFrames - 1 : 2;

		for (size_t i = 0; i < (size_t)LevellerParamIndices::NumParams; ++i) {
			mParamCacheCache[i] = paramCacheCopy[i];
		}

		CalculatePoints(bb);
	}

	void ResponseGraph()
	{
		ImRect bb;
		bb.Min = ImGui::GetCursorPos();
		bb.Max = bb.Min + gHistViewSize;

		ImColor backgroundColor = ColorFromHTML("222222", 1.0f);
		ImColor lineColor = ColorFromHTML("ff8800", 1.0f);

		auto* dl = ImGui::GetWindowDrawList();

		ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

		EnsurePointsPopulated(bb);

		dl->AddPolyline(points, gSegmentCount, lineColor, 0, 2.0f);

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

		//static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
		//char text[1024] = "ok? text here?";
		//ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(400, ImGui::GetTextLineHeight() * 16), flags);

		ImGui::BeginTable("##fmaux", Leveller::gBandCount);
		ImGui::TableNextRow();

		RenderBand(0, "High pass###hp", "Low shelf###hp", LevellerParamIndices::LowShelfFreq, 175);
		RenderBand(1, "Peak 1", "Peak 1", LevellerParamIndices::Peak1Freq, 650);
		RenderBand(2, "Peak 2", "Peak 2", LevellerParamIndices::Peak2Freq, 2000);
		RenderBand(3, "Peak 3", "Peak 3", LevellerParamIndices::Peak3Freq, 7000);
		RenderBand(4, "Low pass###lp", "High shelf###lp", LevellerParamIndices::HighShelfFreq, 6000);

		ImGui::EndTable();

		ResponseGraph();
	}


};

#endif
