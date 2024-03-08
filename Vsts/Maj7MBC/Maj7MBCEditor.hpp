#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7MBCVst.hpp"

struct Maj7MBCEditor : public VstEditor
{
	Maj7MBC* mpMaj7MBC;
	Maj7MBCVst* mpMaj7MBCVst;
	using ParamIndices = Maj7MBC::ParamIndices;

	CompressorVis<330, 120> mCompressorVis[Maj7MBC::gBandCount];

	ColorMod mBandColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mBandDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	Maj7MBCEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1150, 820),
		mpMaj7MBCVst((Maj7MBCVst*)audioEffect)
	{
		mpMaj7MBC = ((Maj7MBCVst *)audioEffect)->GetMaj7MBC();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 MBC Muli-band compressor", mpMaj7MBC, mpMaj7MBCVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7MBC->mParamCache, paramNames);
	}

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char* caption, bool muteSoloEnabled)
	{
		auto& band = mpMaj7MBC->mBands[iBand];
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];

		ColorMod& cm = muteSoloEnabled ? mBandColors : mBandDisabledColors;
		auto token = cm.Push();

		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem(caption))
			{
				using BandParam = Maj7MBC::FreqBand::BandParam;
				auto param = [&](Maj7MBC::FreqBand::BandParam bp) {
					return (VstInt32)enabledParam + (VstInt32)bp;
				};

				//bool effectEnabled = band.mEnableEffect;// && (band.mOutputSignal != Maj7Sat::OutputSignal::Bypass);
				ColorMod& cm = muteSoloEnabled ? mBandColors : mBandDisabledColors;
				auto token = cm.Push();

				ImGui::BeginGroup();

				// MUTE | SOLO
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				Maj7ImGuiBoolParamToggleButton(param(BandParam::Enable), "Enable", { 0,0 }, { "22aa22", "294a7a", "999999", });

				bool delta = bandConfig.mOutputStream == Maj7MBC::OutputStream::Delta;
				bool sidechain = bandConfig.mOutputStream == Maj7MBC::OutputStream::Sidechain;
				ImGui::SameLine(0, 60);
				if (ToggleButton(&delta, "DELTA", { 0,0 }, { "22cc22", "294a7a", "999999", })) {
					// NB: ToggleButton() has flipped the value.
					bandConfig.mOutputStream = !delta ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Delta;
				}

				ImGui::SameLine();
				if (ToggleButton(&sidechain, "SIDECHAIN", { 0,0 }, { "cc22cc", "294a7a", "999999", })) {
					// NB: ToggleButton() has flipped the value.
					bandConfig.mOutputStream = !sidechain ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Sidechain;
				}

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

				ImGui::BeginDisabled(!band.mEnable);

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

				Maj7ImGuiParamScaledFloat(param(BandParam::Threshold), "Thresh(dB)", -60, 0, -20, 0, 0, {});
				ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::Ratio), "Ratio", MonoCompressor::gRatioCfg, 4, {});
				ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::Knee), "Knee(dB)", 0, 30, 4, 0, 0, {});
				ImGui::SameLine(0,90); Maj7ImGuiPowCurvedParam(param(BandParam::Attack), "Attack(ms)", MonoCompressor::gAttackCfg, 50, {});
				ImGui::SameLine(); Maj7ImGuiPowCurvedParam(param(BandParam::Release), "Release(ms)", MonoCompressor::gReleaseCfg, 80, {});

				Maj7ImGuiParamFrequency(param(BandParam::HighPassFrequency), -1, "HP Freq(Hz)", M7::gFilterFreqConfig, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::HighPassQ), "HP Q", 0.2f, 0.2f);

				ImGui::SameLine(0, 90); Maj7ImGuiParamFloat01(param(BandParam::ChannelLink), "ChanLink", 0.8f, 0);
				ImGui::SameLine(0, 90); Maj7ImGuiParamVolume(param(BandParam::InputGain), "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::EndDisabled();

				ImGui::EndGroup();

				ImGui::SameLine();
				mCompressorVis[iBand].Render(band.mEnable, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
	}







	virtual void renderImgui() override
	{
		mBandColors.EnsureInitialized();
		mBandDisabledColors.EnsureInitialized();

		bool muteSoloEnabled[Maj7MBC::gBandCount] = { false, false, false };
		bool mutes[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mMute, mpMaj7MBC->mBands[1].mVSTConfig.mMute , mpMaj7MBC->mBands[2].mVSTConfig.mMute};
		bool solos[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mSolo, mpMaj7MBC->mBands[1].mVSTConfig.mSolo , mpMaj7MBC->mBands[2].mVSTConfig.mSolo };
		M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);

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
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); VUMeter("inputVU", mpMaj7MBC->mInputAnalysis[0], mpMaj7MBC->mInputAnalysis[1], {15,100});
				ImGui::SameLine(); VUMeter("outputVU", mpMaj7MBC->mOutputAnalysis[0], mpMaj7MBC->mOutputAnalysis[1], {15,100});


				bool showWarning = false;
				for (auto& b : mpMaj7MBC->mBands) {
					if (b.mVSTConfig.mMute || b.mVSTConfig.mSolo || (b.mVSTConfig.mOutputStream != Maj7MBC::OutputStream::Normal)) {
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
		RenderBand(0, ParamIndices::AInputGain, "Lows", muteSoloEnabled[0]);
		ImGui::PopID();
		ImGui::PushID("band1");
		RenderBand(1, ParamIndices::BInputGain, "Mids", muteSoloEnabled[1]);
		ImGui::PopID();
		ImGui::PushID("band2");
		RenderBand(2, ParamIndices::CInputGain, "Highs", muteSoloEnabled[2]);
		ImGui::PopID();

	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7MBCEditor(audioEffect);
}
