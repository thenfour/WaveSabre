#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7SatVst.hpp"


//#define MAJ7SAT_ENABLE_RARE_MODELS
//#define MAJ7SAT_ENABLE_ANALOG
//#define MAJ7SAT_ENABLE_MIDSIDE


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
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Sat Saturator", mpMaj7Sat, mpMaj7SatVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Sat->mParamCache, paramNames);
	}

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char *caption)
	{
		auto& band = mpMaj7Sat->mBands[iBand];
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];

		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			bool muteSoloEnabled = band.mMuteSoloEnabled;// && (band.mOutputSignal != Maj7Sat::OutputSignal::Bypass);
			bool effectEnabled = band.mEnableEffect;// && (band.mOutputSignal != Maj7Sat::OutputSignal::Bypass);
			ColorMod& cm = muteSoloEnabled ? mBandColors : mBandDisabledColors;
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

					//Maj7ImGuiParamBoolToggleButtonArray<int>("", 50, {
					//	{ param(BandParam::EnableEffect), "Enable", "339933", "294a7a", "669966", "294a44"},
					//	});

					//Maj7ImGuiParamBoolToggleButtonArray<int>("", 50, {
					//	{ param(BandParam::Mute), "Mute", "990000", "294a7a", "990044", "294a44"},
					//	{ param(BandParam::Solo), "Solo", "999900", "294a7a", "999944", "294a44"},
					//	});





					ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
					ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

					Maj7ImGuiBoolParamToggleButton(param(BandParam::EnableEffect), "Enable", { 0,0 }, { "339933", "294a7a", "999999", });

					//bool delta = bandConfig.mOutputStream == Maj7MBC::OutputStream::Delta;
					//bool sidechain = bandConfig.mOutputStream == Maj7MBC::OutputStream::Sidechain;
					//ImGui::SameLine(0, 60);
					//if (ToggleButton(&delta, "DELTA", { 0,0 }, { "22cc22", "294a7a", "999999", })) {
					//	// NB: ToggleButton() has flipped the value.
					//	bandConfig.mOutputStream = !delta ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Delta;
					//}

					//ImGui::SameLine();
					//if (ToggleButton(&sidechain, "SIDECHAIN", { 0,0 }, { "cc22cc", "294a7a", "999999", })) {
					//	// NB: ToggleButton() has flipped the value.
					//	bandConfig.mOutputStream = !sidechain ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Sidechain;
					//}

					ImGui::SameLine(0, 60);
					if (ToggleButton(&bandConfig.mMute, "MUTE", { 0,0 }, { "990000", "294a7a", "999999", })) {
						// this doesn't work; the common wisdom seems to be to 
						//GetEffectX()->setParameter(0, GetEffectX()->getParameter(0)); // tell the host that the params have changed (even though they haven't)
					}

					ImGui::SameLine();
					if (ToggleButton(&bandConfig.mSolo, "SOLO", { 0,0 }, { "999900", "294a7a", "999999", })) {
						//GetEffectX()->setParameter(0, GetEffectX()->getParameter(0)); // tell the host that the params have changed (even though they haven't)
						//mpMaj7MBCVst->updateDisplay();
					}

					ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding



				}
				ImGui::EndGroup();

				ImGui::BeginDisabled(!effectEnabled);

				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::Threshold), "Threshold", M7::gUnityVolumeCfg, -8, {});

				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume(param(BandParam::Drive), "Drive", M7::gVolumeCfg36db, 0, {});

				MAJ7SAT_MODEL_CAPTIONS(modelNames);

				ImGui::SameLine(); Maj7ImGuiParamEnumCombo(param(BandParam::Model), "Model", (int)Maj7Sat::Model::Count__, Maj7Sat::Model::TanhClip, modelNames, 100);

#ifdef MAJ7SAT_ENABLE_ANALOG
				ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::EvenHarmonics), "Analog", 0, Maj7Sat::gAnalogMaxLin, 0.12f, 0, 0, {});
#else
				ImGui::SameLine(); ImGui::Text("Analog\r\n#undef");
#endif // MAJ7SAT_ENABLE_ANALOG

#ifdef MAJ7SAT_ENABLE_MIDSIDE
				static constexpr char const* const panModeNames[(size_t)Maj7Sat::PanMode::Count__] = {
					"Stereo",
					"MidSide",
				};
				ImGui::SameLine(); Maj7ImGuiParamEnumList<Maj7Sat::PanMode>(param(BandParam::PanMode), "PanMode", (int)Maj7Sat::PanMode::Count__, Maj7Sat::PanMode::Stereo, panModeNames);
				ImGui::SameLine(); Maj7ImGuiParamFloatN11(param(BandParam::Pan), "EffectPan", 0, 0, {});
#else
				ImGui::SameLine(); ImGui::Text("Mid/Side\r\n#undef");
#endif // MAJ7SAT_ENABLE_MIDSIDE

				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::CompensationGain), "Makeup", M7::gVolumeCfg12db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::DryWet), "DryWet", 1, 0);
				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Output", M7::gVolumeCfg12db, 0, {});

				RenderTransferCurve({ 100, 100 }, {
					ColorFromHTML("222222"), // bg
					ColorFromHTML(effectEnabled ? "8888cc" : "777777"), // line
					 ColorFromHTML(effectEnabled ? "ffff00" : "777777"), // line clipped
					 ColorFromHTML("444444"), // tick
					}, [&](float x) {
						return band.transfer(x);
					});

				ImGui::SameLine(); VUMeter("inputVU", band.mInputAnalysis0, band.mInputAnalysis1, {15,100 });
				ImGui::SameLine(); VUMeter("outputVU", band.mOutputAnalysis0, band.mOutputAnalysis1, { 15,100 });

				ImGui::EndDisabled();

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
				//LR_SLOPE_CAPTIONS(slopeNames);

				Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverAFrequency, -1, "x1freq(Hz)", M7::gFilterFreqConfig, 550, 0, {});
				ImGui::SameLine(0); Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverBFrequency, -1, "x2Freq(Hz)", M7::gFilterFreqConfig, 3000, 1, {});
				//ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)ParamIndices::CrossoverASlope, "xASlope", (int)M7::LinkwitzRileyFilter::Slope::Count__, M7::LinkwitzRileyFilter::Slope::Slope_12dB, slopeNames, 100);
				ImGui::SameLine(); ImGui::Text("slope\r\n#disabled");

				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::OverallDryWet, "DryWet", 1, 0);

				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); VUMeter("inputVU", mpMaj7Sat->mInputAnalysis0, mpMaj7Sat->mInputAnalysis1, { 15,100 });
				ImGui::SameLine(); VUMeter("outputVU", mpMaj7Sat->mOutputAnalysis0, mpMaj7Sat->mOutputAnalysis1, { 15,100 });


				bool showWarning = false;
				for (auto& b : mpMaj7Sat->mBands) {
					if (b.mVSTConfig.mMute || b.mVSTConfig.mSolo) {
						showWarning = true;
						break;
					}
				}

				if (showWarning) {
					ImGui::SameLine();
					ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHTML("ff3333", 1).operator ImVec4());
					ImGui::Text("Mute, solo, delta are not supported by EXE player");
					ImGui::PopStyleColor();
				}

				// do this simply to avoid jittery vertical positioning when the warning is shown/hidden.
				ImGui::SameLine();
				ImGui::Text(" ");




				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		ImGui::PushID("band0");
		RenderBand(0, ParamIndices::AModel, "Lows");
		ImGui::PopID();
		ImGui::PushID("band1");
		RenderBand(1, ParamIndices::BModel, "Mids");
		ImGui::PopID();
		ImGui::PushID("band2");
		RenderBand(2, ParamIndices::CModel, "Highs");
		ImGui::PopID();
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7SatEditor(audioEffect);
}
