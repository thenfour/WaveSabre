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
	using ParamIndices = Maj7Sat::ParamIndices;

	ColorMod mBandColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mBandDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	Maj7SatEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1000, 820),
		mpMaj7SatVst((Maj7SatVst*)audioEffect)
	{
		mpMaj7Sat = ((Maj7SatVst*)audioEffect)->GetMaj7Sat();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Sat Saturator", mpMaj7Sat, mpMaj7SatVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Sat", mpMaj7Sat->mParamCache, paramNames);
	}

	struct TransferCurveColors
	{
		ImColor background;
		ImColor line;
		ImColor lineClip;
		ImColor tick;
	};

	void RenderTransferCurve(ImVec2 size, const TransferCurveColors& colors, const Maj7Sat::FreqBand& band)
	{
		auto* dl = ImGui::GetWindowDrawList();
		ImRect bb;
		bb.Min = ImGui::GetCursorScreenPos();
		bb.Max = bb.Min + size;
		ImGui::RenderFrame(bb.Min, bb.Max, colors.background);

		static constexpr float gMaxLin = 1.5f;

		auto LinToY = [&](float lin) {
			float t = M7::math::lerp_rev(0, gMaxLin, lin);
			t = Clamp01(t);
			return M7::math::lerp(bb.Max.y, bb.Min.y, t);
		};

		auto LinToX = [&](float lin) {
			float t = M7::math::lerp_rev(0, gMaxLin, lin);
			t = Clamp01(t);
			return M7::math::lerp(bb.Min.x, bb.Max.x, t);
		};

		static constexpr int segmentCount = 16;
		std::vector<ImVec2> points;
		std::vector<ImVec2> clipPoints;
		float unused;

		for (int i = 0; i < segmentCount; ++i)
		{
			float t01 = float(i) / (segmentCount - 1); // touch 0 and 1
			float tLin = M7::math::lerp(0, gMaxLin, t01);
			float yLin = band.transfer(tLin, unused);
			if (tLin >= 1) {
				if (clipPoints.empty()) {
					points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
				}
				clipPoints.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
			}
			else {
				points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
			}
		}

		dl->AddLine({ bb.Min.x, bb.Max.y }, {bb.Max.x, bb.Min.y}, colors.tick, 1);
		dl->AddLine({ bb.Min.x, LinToY(1) }, { bb.Max.x, LinToY(1) }, colors.tick, 1);
		dl->AddLine({ LinToX(1), bb.Min.y }, { LinToX(1), bb.Max.y }, colors.tick, 1);

		dl->AddPolyline(points.data(), (int)points.size(), colors.line, 0, 3);
		dl->AddPolyline(clipPoints.data(), (int)clipPoints.size(), colors.lineClip, 0, 3);

		ImGui::Dummy(size);

	} // void RenderTransferCurve()

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char *caption)
	{
		auto& band = mpMaj7Sat->mBands[iBand];

		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			bool enabled = band.mMuteSoloEnabled && (band.mOutputSignal != Maj7Sat::OutputSignal::Bypass);
			ColorMod& cm = enabled ? mBandColors : mBandDisabledColors;
			auto token = cm.Push();

			if (WSBeginTabItem(caption))
			{
				using BandParam = Maj7Sat::FreqBand::BandParam;
				auto param = [&](Maj7Sat::FreqBand::BandParam bp) {
					return (VstInt32)enabledParam + (VstInt32)bp;
				};

				ImGui::BeginGroup();
				{
					ColorMod& cm = mBandColors;
					auto token = cm.Push();

					Maj7ImGuiParamBoolToggleButtonArray<int>("", 40, {
						{ param(BandParam::Mute), "Mute", "990000", "294a7a", "990000", "294a7a"},
						{ param(BandParam::Solo), "Solo", "999900", "294a7a", "999900", "294a7a"},
						});

					// selected, not selected, hover, not selected hover
					Maj7ImGuiParamEnumToggleButtonArray<Maj7Sat::OutputSignal>(param(BandParam::OutputSignal), "", 40, {
						{ "Enable", "009900", "294a7a", "009900", "294a7a", Maj7Sat::OutputSignal::Normal, Maj7Sat::OutputSignal::Bypass},
						{ "Diff", "990099", "294a7a", "990099", "294a7a", Maj7Sat::OutputSignal::Diff, Maj7Sat::OutputSignal::Normal},
						});
				}
				ImGui::EndGroup();

				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::Threshold), "Threshold", M7::gUnityVolumeCfg, -8, {});

				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume(param(BandParam::Drive), "Drive", M7::gVolumeCfg36db, 0, {});

				static constexpr char const* const modelNames[(size_t)Maj7Sat::Model::Count__] = {
					"Thru",
					"SineClip",
					"DivClipSoft",
					"TanhClip",
					"DivClipMedium",
					"DivClipHard",
					"HardClip",
					"TanhFold",
					"TanhSinFold",
					"SinFold",
					"LinearFold",
				};

				ImGui::SameLine(); Maj7ImGuiParamEnumCombo(param(BandParam::Model), "Model", (int)Maj7Sat::Model::Count__, Maj7Sat::Model::TanhClip, modelNames, 100);

				ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::EvenHarmonics), "Analog", 0, Maj7Sat::gAnalogMaxLin, 0.12f, 0, 0, {});

				static constexpr char const* const panModeNames[(size_t)Maj7Sat::PanMode::Count__] = {
					"Stereo",
					"MidSide",
				};
				ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::PanMode>(param(BandParam::PanMode), "PanMode", (int)Maj7Sat::PanMode::Count__, Maj7Sat::PanMode::Stereo, panModeNames);
				ImGui::SameLine(); Maj7ImGuiParamFloatN11(param(BandParam::Pan), "EffectPan", 0, 0, {});

				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::CompensationGain), "Makeup", M7::gVolumeCfg12db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::DryWet), "DryWet", 1, 0);
				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Output", M7::gVolumeCfg12db, 0, {});

				RenderTransferCurve({ 100, 100 }, {
					ColorFromHTML("222222"), // bg
					ColorFromHTML("8888cc"), // line
					 ColorFromHTML("ffff00"), // line clipped
					 ColorFromHTML("444444"), // tick
					}, band);

				ImGui::SameLine(); VUMeter("inputVU", band.mInputAnalysis0, band.mInputAnalysis1, {15,100 });
				ImGui::SameLine(); VUMeter("outputVU", band.mOutputAnalysis0, band.mOutputAnalysis1, { 15,100 });

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
	}

	virtual void renderImgui() override
	{
		mBandColors.EnsureInitialized();
		mBandDisabledColors.EnsureInitialized();

		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("IO"))
			{
				static constexpr char const* const slopeNames[(size_t)M7::LinkwitzRileyFilter::Slope::Count__] = {
			"6dB/oct",
			"12dB/oct",
			"24dB/oct",
			"36dB/oct",
			"48dB/oct",
				};

				Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverAFrequency, -1, "x1freq(Hz)", M7::gFilterFreqConfig, 550, 0, {});
				ImGui::SameLine(0); Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverBFrequency, -1, "x2Freq(Hz)", M7::gFilterFreqConfig, 3000, 1, {});
				ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)ParamIndices::CrossoverASlope, "xASlope", (int)M7::LinkwitzRileyFilter::Slope::Count__, M7::LinkwitzRileyFilter::Slope::Slope_12dB, slopeNames, 100);

				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::OverallDryWet, "DryWet", 1, 0);

				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); VUMeter("inputVU", mpMaj7Sat->mInputAnalysis0, mpMaj7Sat->mInputAnalysis1, { 15,100 });
				ImGui::SameLine(); VUMeter("outputVU", mpMaj7Sat->mOutputAnalysis0, mpMaj7Sat->mOutputAnalysis1, { 15,100 });

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		ImGui::PushID("band0");
		RenderBand(0, ParamIndices::AMute, "Lows");
		ImGui::PopID();
		ImGui::PushID("band1");
		RenderBand(1, ParamIndices::BMute, "Mids");
		ImGui::PopID();
		ImGui::PushID("band2");
		RenderBand(2, ParamIndices::CMute, "Highs");
		ImGui::PopID();
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SatEditor(audioEffect);
}
