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

	Maj7SatEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1000, 800),
		mpMaj7SatVst((Maj7SatVst*)audioEffect)
	{
		mpMaj7Sat = ((Maj7SatVst*)audioEffect)->GetMaj7Sat();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Sat Saturator", mpMaj7Sat, mpMaj7SatVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Sat", mpMaj7Sat->mParamCache, paramNames);
	}

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char *caption)
	{
		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem(caption))
			{

				using BandParam = Maj7Sat::FreqBand::BandParam;
				auto param = [&](Maj7Sat::FreqBand::BandParam bp) {
					return (VstInt32)enabledParam + (VstInt32)bp;
				};

				//WSImGuiParamCheckbox(param(BandParam::Enable), "Enable band?");

				ImGui::BeginGroup();

				Maj7ImGuiParamBoolToggleButton(param(BandParam::Enable), "Enable", "008800");


				//static constexpr char const* const outputSignalNames[(size_t)Maj7Sat::OutputSignal::Count__] = {
				//	"Mute",
				//	"Diff",
				//};

				//const char* caption;
				//ImVec4 selectedColor;
				//ImVec4 notSelectedColor;
				//ImVec4 selectedHoveredColor;
				//ImVec4 notSelectedHoveredColor;
				//TEnum value;
				//TEnum valueOnDeselect; // when the user unclicks the value, what should the underlying param get set to?

				 Maj7ImGuiParamEnumToggleButtonArray<Maj7Sat::OutputSignal>(param(BandParam::OutputSignal), "", 40, {
					{ "Mute", "990000", nullptr, nullptr, nullptr, Maj7Sat::OutputSignal::None, Maj7Sat::OutputSignal::Wet},
					{ "Diff", "999900", nullptr, nullptr, nullptr, Maj7Sat::OutputSignal::Diff, Maj7Sat::OutputSignal::Wet},
					});

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
				//ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::Model>(param(BandParam::Model), "Model", (int)Maj7Sat::Model::Count__, Maj7Sat::Model::TanhClip, modelNames);

				ImGui::SameLine(); Maj7ImGuiParamEnumCombo(param(BandParam::Model), "Model", (int)Maj7Sat::Model::Count__, Maj7Sat::Model::TanhClip, modelNames, 100);

				ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::EvenHarmonics), "Analog", 0, Maj7Sat::gAnalogMaxLin, 0.12f, 0, 0, {});

				//static constexpr char const* const analogStyleNames[(size_t)Maj7Sat::EvenHarmonicsStyle::Count__] = {
				//	"SqrtHard",
				//	"SqrtSin",
				//	"LinHard",
				//	"LinSin",
				//};
				//ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::EvenHarmonicsStyle>(param(BandParam::EvenHarmonicsStyle), "AnalogStyle", (int)Maj7Sat::EvenHarmonicsStyle::Count__, Maj7Sat::EvenHarmonicsStyle::LinHard, analogStyleNames);


				static constexpr char const* const panModeNames[(size_t)Maj7Sat::PanMode::Count__] = {
					"Stereo",
					"MidSide",
				};
				ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::PanMode>(param(BandParam::PanMode), "PanMode", (int)Maj7Sat::PanMode::Count__, Maj7Sat::PanMode::Stereo, panModeNames);
				ImGui::SameLine(); Maj7ImGuiParamFloatN11(param(BandParam::Pan), "EffectPan", 0, 0, {});

				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::CompensationGain), "Makeup", M7::gVolumeCfg12db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::DryWet), "DryWet", 1, 0);
				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Output", M7::gVolumeCfg12db, 0, {});
				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
	}

	virtual void renderImgui() override
	{
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
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				//static constexpr char const* const oversamplingNames[(size_t)Maj7Sat::Oversampling::Count__] = {
				//	"Off",
				//	"x2",
				//	"x4",
				//	"x8",
				//};
				//ImGui::SameLine(0, 80); Maj7ImGuiParamEnumList<Maj7Sat::Oversampling>(ParamIndices::Oversampling, "Oversampling", (int)Maj7Sat::Oversampling::Count__, Maj7Sat::Oversampling::Off, oversamplingNames);

				//static constexpr char const* const outputSignalNames[(size_t)Maj7Sat::OutputSignal::Count__] = {
				//	"WetCombined",
				//	"WetBandLow",
				//	"WetBandMid",
				//	"WetBandHigh",
				//	"DryCombined",
				//	"DryBandLow",
				//	"DryBandMid",
				//	"DryBandHigh",
				//	"Diff",
				//};
				//ImGui::SameLine(0, 80); ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::OutputSignal>(ParamIndices::OutputSignal, "OutputSignal", (int)Maj7Sat::OutputSignal::Count__, Maj7Sat::OutputSignal::WetCombined, outputSignalNames);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		ImGui::PushID("band0");
		RenderBand(0, ParamIndices::AEnable, "Lows");
		ImGui::PopID();
		ImGui::PushID("band1");
		RenderBand(1, ParamIndices::BEnable, "Mids");
		ImGui::PopID();
		ImGui::PushID("band2");
		RenderBand(2, ParamIndices::CEnable, "Highs");
		ImGui::PopID();
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SatEditor(audioEffect);
}
