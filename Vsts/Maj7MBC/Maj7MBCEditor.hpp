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
	CompressorVis<650, 320> mCompressorVisBig[Maj7MBC::gBandCount];
	CompressorVis<330, 60> mCompressorVisSmall[Maj7MBC::gBandCount];

	ColorMod mBandColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mBandDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	Maj7MBCEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1150, 950),
		mpMaj7MBCVst((Maj7MBCVst*)audioEffect)
	{
		mpMaj7MBC = ((Maj7MBCVst *)audioEffect)->GetMaj7MBC();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 MBC Muli-band compressor", mpMaj7MBC, mpMaj7MBCVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7MBC->mParamCache, paramNames);
	}

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char* caption, bool muteSoloEnabled, bool mbEnabledEnabled, bool mbEnabled)
	{
		auto& band = mpMaj7MBC->mBands[iBand];
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];
		bool bandEnabled = band.mEnable;

		ColorMod& cm = (muteSoloEnabled && mbEnabledEnabled) ? mBandColors : mBandDisabledColors;
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
				ColorMod& cm = (muteSoloEnabled && mbEnabledEnabled) ? mBandColors : mBandDisabledColors;
				auto token = cm.Push();

				ImGui::BeginGroup();

				// MUTE | SOLO
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				Maj7ImGuiBoolParamToggleButton(param(BandParam::Enable), "Enable", { 0,0 }, { "22aa22", "294a7a", "999999", });

				bool isCompactDisplay = bandConfig.mDisplayStyle == Maj7MBC::DisplayStyle::Compact;
				ImGui::SameLine(0, 30);
				if (ToggleButton(&isCompactDisplay, "Small")) {
					bandConfig.mDisplayStyle = Maj7MBC::DisplayStyle::Compact;
				}
				bool isNormalDisplay = bandConfig.mDisplayStyle == Maj7MBC::DisplayStyle::Normal;
				ImGui::SameLine();
				if (ToggleButton(&isNormalDisplay, "Default")) {
					bandConfig.mDisplayStyle = Maj7MBC::DisplayStyle::Normal;
				}
				bool isBigDisplay = bandConfig.mDisplayStyle == Maj7MBC::DisplayStyle::Big;
				ImGui::SameLine();
				if (ToggleButton(&isBigDisplay, "Big")) {
					bandConfig.mDisplayStyle = Maj7MBC::DisplayStyle::Big;
				}


				ImGui::SameLine(0, 40);
				if (mbEnabled)
				{
					ToggleButton(&bandConfig.mMute, "MUTE", { 0,0 }, { "990000", "294a7a", "999999", });
					ImGui::SameLine(); ToggleButton(&bandConfig.mSolo, "SOLO", { 0,0 }, { "999900", "294a7a", "999999", });
				} // mbenabled

				if (bandEnabled) {
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
				}

				//ImGui::BeginDisabled(!band.mEnable);

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

				if (bandEnabled && !isCompactDisplay)
				{
					Maj7ImGuiParamScaledFloat(param(BandParam::Threshold), "Thresh(dB)", -60, 0, -20, 0, 0, {});
					ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::Ratio), "Ratio", MonoCompressor::gRatioCfg, 4, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::Knee), "Knee(dB)", 0, 30, 4, 0, 0, {});
					ImGui::SameLine(0, 80); Maj7ImGuiPowCurvedParam(param(BandParam::Attack), "Attack(ms)", MonoCompressor::gAttackCfg, 50, {});
					ImGui::SameLine(); Maj7ImGuiPowCurvedParam(param(BandParam::Release), "Release(ms)", MonoCompressor::gReleaseCfg, 80, {});

					if (mbEnabled)
					{
						ImGui::SameLine();
						ImGui::BeginGroup(); // lows mids highs group
						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 2 });
						ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
						ImGui::PushStyleColor(ImGuiCol_Button, ColorFromHTML("222222").operator ImVec4());

						if (iBand != 0) {
							if (ImGui::Button("->Lows", { 60,0 })) {
								CopyBand((int)iBand, 0);
							}
						}

						if (iBand != 1) {
							if (ImGui::Button("->Mids", { 60,0 })) {
								CopyBand((int)iBand, 1);
							}
						}

						if (iBand != 2) {
							if (ImGui::Button("->Highs", { 60,0 })) {
								CopyBand((int)iBand, 2);
							}
						}
						ImGui::PopStyleColor();
						ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
						ImGui::EndGroup(); // lows mids highs group
					}

					Maj7ImGuiParamFrequency(param(BandParam::HighPassFrequency), -1, "HP Freq(Hz)", M7::gFilterFreqConfig, 0, {});
					ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::HighPassQ), "HP Q", M7::gBiquadFilterQCfg, 1.0f, {});
					//ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::HighPassQ), "HP Q", 0.2f, 0.2f);

					ImGui::SameLine(0, 40); Maj7ImGuiParamFloat01(param(BandParam::ChannelLink), "ChanLink", 0.8f, 0);
					ImGui::SameLine(0, 40); Maj7ImGuiParamVolume(param(BandParam::Drive), "Drive", M7::gVolumeCfg36db, 0, {});
					ImGui::SameLine(0, 40); Maj7ImGuiParamVolume(param(BandParam::InputGain), "Input", M7::gVolumeCfg24db, 0, {});

					ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Output", M7::gVolumeCfg24db, 0, {});
				} // band enabled

				ImGui::EndGroup();

				if (bandEnabled) {
					switch (bandConfig.mDisplayStyle) {
					case Maj7MBC::DisplayStyle::Compact:
						ImGui::SameLine();
						mCompressorVisSmall[iBand].Render(band.mEnable, false, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);
						break;
					case Maj7MBC::DisplayStyle::Normal:
						ImGui::SameLine();
						mCompressorVis[iBand].Render(band.mEnable, true, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);
						break;
					case Maj7MBC::DisplayStyle::Big:
						mCompressorVisBig[iBand].Render(band.mEnable, true, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);
						break;
					}
				}

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
	}


	void CopyBand(int ifrom, int ito)
	{
		auto bandToStr = [&](int i) {
			switch (i) {
			case 0:
				return "lows";
			case 1:
				return "mids";
			case 2:
				return "highs";
			default:
				return "unknown";
			}
		};
		auto bandToParamOffset = [&](int i) {
			return mpMaj7MBC->mBands[i].mParams.mBaseParamID;
		};
		const char* fromstr = bandToStr(ifrom);
		const char* tostr = bandToStr(ito);
		char s[200];
		const int fromParamOffset = bandToParamOffset(ifrom);
		const int toParamOffset = bandToParamOffset(ito);
		std::sprintf(s, "click OK to copy settings from band %s to %s", fromstr, tostr);
		if (::MessageBox(mCurrentWindow, s, "Maj7", MB_ICONQUESTION | MB_OKCANCEL) == IDOK) {

			int bandParamCount = (int)ParamIndices::BAttack - (int)ParamIndices::AAttack;
			for (int i = 0; i < bandParamCount; ++i) {
				float val = mpMaj7MBCVst->getParameter(fromParamOffset + i);
				mpMaj7MBCVst->setParameter(toParamOffset + i, val);
			}
			::MessageBoxA(mCurrentWindow, "Done.", "Maj7", MB_ICONINFORMATION | MB_OK);
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

		float backing = mpMaj7MBCVst->getParameter((int)ParamIndices::MultibandEnable);
		M7::ParamAccessor pa{ &backing, 0 };
		bool mbEnabled = pa.GetBoolValue(0);


		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("IO"))
			{
				//LR_SLOPE_CAPTIONS(slopeNames);

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				bool mbdisabled = !mbEnabled;
				ImGui::BeginGroup();
				if (ToggleButton(&mbdisabled, "SINGLE BAND", { 100,40 }, { "5555ee", "294a7a", "999999", })) {
					// NB: ToggleButton() has flipped the value.
					pa.SetBoolValue(0, false);
					mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
				}
				if (ToggleButton(&mbEnabled, "MULTIBAND", { 100,40 }, { "ee55ee", "294a7a", "999999", })) {
					// NB: ToggleButton() has flipped the value.
					pa.SetBoolValue(0, true);
					mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
				}
				ImGui::EndGroup();

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

				if (mbEnabled) {

					ImGui::BeginDisabled(mbdisabled);

					ImGui::SameLine(); Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverAFrequency, -1, "x1freq(Hz)", M7::gFilterFreqConfig, 550, 0, {});
					ImGui::SameLine(); Maj7ImGuiParamFrequencyWithCenter((int)ParamIndices::CrossoverBFrequency, -1, "x2Freq(Hz)", M7::gFilterFreqConfig, 3000, 1, {});
					//ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)ParamIndices::CrossoverASlope, "xASlope", (int)M7::LinkwitzRileyFilter::Slope::Count__, M7::LinkwitzRileyFilter::Slope::Slope_12dB, slopeNames, 100);
					ImGui::SameLine(); ImGui::Text("slope\r\n#disabled");

					ImGui::EndDisabled();
				}

				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::SameLine(); VUMeter("inputVU", mpMaj7MBC->mInputAnalysis[0], mpMaj7MBC->mInputAnalysis[1], {15,120 });
				ImGui::SameLine(); VUMeter("outputVU", mpMaj7MBC->mOutputAnalysis[0], mpMaj7MBC->mOutputAnalysis[1], {15,120 });


				ImGui::SameLine(); Maj7ImGuiBoolParamToggleButton(ParamIndices::SoftClipEnable, "Softclip");
				M7::QuickParam qp{mpMaj7MBCVst->getParameter((VstInt32)ParamIndices::SoftClipEnable)};
				if (qp.GetBoolValue()) {
					//ImGui::BeginDisabled(!qp.GetBoolValue());
					ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::SoftClipThresh, "Thresh", M7::gUnityVolumeCfg, -6, {});
					ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::SoftClipOutput, "Output", M7::gUnityVolumeCfg, -0.3f, {});
					//ImGui::EndDisabled();

					M7::QuickParam qp{ mpMaj7MBCVst->getParameter((VstInt32)ParamIndices::SoftClipThresh) };
					float softClipThreshLin = qp.GetVolumeLin(M7::gUnityVolumeCfg);

					ImGui::SameLine(); RenderTransferCurve({ 120, 120 }, {
						ColorFromHTML("222222"), // bg
						ColorFromHTML("8888cc"), // line
						 ColorFromHTML("ffff00"), // line clipped
						 ColorFromHTML("444444"), // tick
						}, [&](float x) {
							return Maj7MBC::Softclip(x, softClipThreshLin, 1)[0];
						});


					ImGui::SameLine(); VUMeterAtten("scclip", mpMaj7MBC->mClippingAnalysis[0], mpMaj7MBC->mClippingAnalysis[1], {30,120});
				}


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

		if (mbEnabled) {
			ImGui::PushID("band0");
			RenderBand(0, ParamIndices::AInputGain, "Lows", muteSoloEnabled[0], mbEnabled, mbEnabled);
			ImGui::PopID();
		}

		ImGui::PushID("band1");
		RenderBand(1, ParamIndices::BInputGain, mbEnabled ? "Mids" : "All frequencies", muteSoloEnabled[1], true, mbEnabled);
		ImGui::PopID();

		if (mbEnabled) {
			ImGui::PushID("band2");
			RenderBand(2, ParamIndices::CInputGain, "Highs", muteSoloEnabled[2], mbEnabled, mbEnabled);
			ImGui::PopID();
		}
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7MBCEditor(audioEffect);
}
