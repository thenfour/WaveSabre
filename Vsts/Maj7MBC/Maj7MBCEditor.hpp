#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7MBCVst.hpp"
#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>
#include <WaveSabreVstLib/FreqMagnitudeGraph/FFTDiffLayer.hpp>

struct Maj7MBCEditor : public VstEditor
{
	Maj7MBC* mpMaj7MBC;
	Maj7MBCVst* mpMaj7MBCVst;
	using ParamIndices = Maj7MBC::ParamIndices;

	CompressorVis<650, 220> mCompressorVisBig[Maj7MBC::gBandCount];
	CompressorVis<180, 50> mCompressorVisSmall[Maj7MBC::gBandCount];

	ColorMod mBandColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mBandDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	int mEditingBand = 0;

	FrequencyResponseRendererLayered<270, 78, 2, (size_t)Maj7MBC::ParamIndices::NumParams, false> mResponseGraphs[Maj7MBC::gBandCount];
	FrequencyResponseRendererLayered<1000, 180, 0, (size_t)Maj7MBC::ParamIndices::NumParams, false> mCrossoverGraph;
	const ImVec2 kMainVuMeterSize{ 20, 180 };

	// for small compressor vis
	bool mShowInputHistory = true;
	bool mShowDetectorHistory = false;
	bool mShowOutputHistory = false;
	bool mShowAttenuationHistory = true;
	bool mShowThresh = false;
	bool mShowLeft = true;
	bool mShowRight = false;

	bool mShowCrossoverResponse = true;
	bool mShowInputFft = true;
	bool mShowOutputFft = false;
	bool mShowFftDiff = true;
	bool mShowFftDiffFlat = false; // new toggle for FFT diff flat

	VstSerializableIntParamRef<int> mEditingBandParam{ "EditingBand", mEditingBand };
	VstSerializableBoolParamRef mShowInputHistoryParam{ "ShowInputHistory", mShowInputHistory };
	VstSerializableBoolParamRef mShowDetectorHistoryParam{ "ShowDetectorHistory", mShowDetectorHistory };
	VstSerializableBoolParamRef mShowOutputHistoryParam{ "ShowOutputHistory", mShowOutputHistory };
	VstSerializableBoolParamRef mShowAttenuationHistoryParam{ "ShowAttenuationHistory", mShowAttenuationHistory };
	VstSerializableBoolParamRef mShowThreshParam{ "ShowThresh", mShowThresh };
	VstSerializableBoolParamRef mShowLeftParam{ "ShowLeft", mShowLeft };
	VstSerializableBoolParamRef mShowRightParam{ "ShowRight", mShowRight };

	VstSerializableBoolParamRef mShowCrossoverResponseParam{ "ShowCrossoverResponse", mShowCrossoverResponse };
	VstSerializableBoolParamRef mShowInputFftParam{ "ShowInputFft", mShowInputFft };
	VstSerializableBoolParamRef mShowOutputFftParam{ "ShowOutputFft", mShowOutputFft };
	VstSerializableBoolParamRef mShowFftDiffParam{ "ShowFftDiff", mShowFftDiff }; // serialize diff toggle
	VstSerializableBoolParamRef mShowFftDiffFlatParam{ "ShowFftDiffFlat", mShowFftDiffFlat };

	Maj7MBCEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1150, 950),
		mpMaj7MBCVst((Maj7MBCVst*)audioEffect)
	{
		mpMaj7MBC = ((Maj7MBCVst*)audioEffect)->GetMaj7MBC();
	}

	virtual std::vector<IVstSerializableParam*> GetVstOnlyParams() override
	{
		auto base = std::vector<IVstSerializableParam*>{
			&mEditingBandParam,
			&mShowInputHistoryParam,
			&mShowDetectorHistoryParam,
			&mShowOutputHistoryParam,
			&mShowAttenuationHistoryParam,
			&mShowThreshParam,
			&mShowLeftParam,
			&mShowRightParam,
			&mShowCrossoverResponseParam,
			&mShowInputFftParam,
			&mShowOutputFftParam,
			&mShowFftDiffParam,
			&mShowFftDiffFlatParam
		};
		return base;
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 MBC Muli-band compressor", mpMaj7MBC, mpMaj7MBCVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7MBC->mParamCache, paramNames);
	}

	void RenderBand(size_t iBand, ParamIndices enabledParam, const char* caption, bool muteSoloEnabled, bool mbEnabledEnabled, bool mbEnabled)
	{
		ImGuiIdScope __scope{ iBand };
		auto& band = mpMaj7MBC->mBands[iBand];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];
#endif
		bool bandEnabled = band.mEnable;

		KnobColorMod colorMod{
	bandEnabled ? "222222" : "111111",// inactiveTrackColor
	bandEnabled ? bandColors[iBand] : "444444",// activeTrackColor
	bandEnabled ? bandColors[iBand] : "666666",// thumbColor
	bandEnabled ? "ffffff" : "666666"// textColor
		};

		auto knobToken = colorMod.Push();

		//ColorMod& cm = (muteSoloEnabled && mbEnabledEnabled) ? mBandColors : mBandDisabledColors;
		//auto token = cm.Push();

		//if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{
			//if (WSBeginTabItem(caption))
			{
				using BandParam = Maj7MBC::FreqBand::BandParam;
				auto param = [&](Maj7MBC::FreqBand::BandParam bp) {
					return (VstInt32)enabledParam + (VstInt32)bp;
					};

				//ImGui::BeginGroup();

				// MUTE | SOLO
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				Maj7ImGuiBoolParamToggleButton(param(BandParam::Enable), "Enable", { 0,0 }, { bandColors[iBand], "294a7a", "999999", });

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				ImGui::SameLine(0, 40);
				if (mbEnabled)
				{
					ToggleButton(&bandConfig.mMute, "MUTE", { 0,0 }, { "990000", "294a7a", "999999", });
					ImGui::SameLine(); ToggleButton(&bandConfig.mSolo, "SOLO", { 0,0 }, { "999900", "294a7a", "999999", });
				} // mbenabled

				if (bandEnabled) {
					bool delta = bandConfig.mOutputStream == Maj7MBC::OutputStream::Delta;
					ImGui::SameLine(0, 60);
					if (ToggleButton(&delta, "DELTA", { 0,0 }, { "22cc22", "294a7a", "999999", })) {
						// NB: ToggleButton() has flipped the value.
						bandConfig.mOutputStream = !delta ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Delta;
					}
				}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

				//ImGui::BeginDisabled(!band.mEnable);

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

				if (bandEnabled)
				{
					Maj7ImGuiParamScaledFloat(param(BandParam::Threshold), "Thresh(dB)", -60, 0, -20, 0, 0, {});
					ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::Ratio), "Ratio", MonoCompressor::gRatioCfg, 4, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::Knee), "Knee(dB)", 0, 30, 4, 0, 0, {});
					ImGui::SameLine(0, 80); Maj7ImGuiPowCurvedParam(param(BandParam::Attack), "Attack(ms)", MonoCompressor::gAttackCfg, 50, {});
					ImGui::SameLine(); Maj7ImGuiPowCurvedParam(param(BandParam::Release), "Release(ms)", MonoCompressor::gReleaseCfg, 80, {});


					ImGui::SameLine(0, 120); Maj7ImGuiParamVolume(param(BandParam::InputGain), "Input", M7::gVolumeCfg24db, 0, {});
					//ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::Drive), "Drive", M7::gVolumeCfg36db, 0, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::Drive), "Drive(dB)", 0, 30, 0, 0, 0, {});
					// show tooltip for drive.
					if (ImGui::IsItemHovered()) {
						ImGui::BeginTooltip();
						ImGui::TextUnformatted("Nonlinear saturation applied post-compression stage");
						ImGui::EndTooltip();
					}

					ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Makeup", M7::gVolumeCfg24db, 0, {});
					ImGui::SameLine(); Maj7ImGuiParamFloat01(param(BandParam::DryWet), "Dry-Wet", 1.0f, 0);

					ImGui::SameLine(0,40); Maj7ImGuiParamFloatN11(param(BandParam::MidSideMix), "Width", 0, 0, {});
					ImGui::SameLine(); Maj7ImGuiParamFloatN11(param(BandParam::Pan), "Pan", 0, 0, {});

					// turns out these aren't so useful.
					//if (mbEnabled)
					//{
					//	ImGui::SameLine();
					//	ImGui::BeginGroup(); // lows mids highs group
					//	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 2 });
					//	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
					//	ImGui::PushStyleColor(ImGuiCol_Button, ColorFromHTML("222222").operator ImVec4());

					//	if (iBand != 0) {
					//		if (ImGui::Button("->Lows", { 60,0 })) {
					//			CopyBand((int)iBand, 0);
					//		}
					//	}

					//	if (iBand != 1) {
					//		if (ImGui::Button("->Mids", { 60,0 })) {
					//			CopyBand((int)iBand, 1);
					//		}
					//	}

					//	if (iBand != 2) {
					//		if (ImGui::Button("->Highs", { 60,0 })) {
					//			CopyBand((int)iBand, 2);
					//		}
					//	}
					//	ImGui::PopStyleColor();
					//	ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
					//	ImGui::EndGroup(); // lows mids highs group
					//}

					//ImGui::SameLine(); Maj7ImGuiParamFrequency(param(BandParam::HighPassFrequency), -1, "HP(Hz)", M7::gFilterFreqConfig, 0, {});
					//ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::HighPassQ), "HP Q", M7::gBiquadFilterQCfg, 1.0f, {});
					//ImGui::SameLine(); Maj7ImGuiParamFrequency(param(BandParam::LowPassFrequency), -1, "LP(Hz)", M7::gFilterFreqConfig, 22000, {});
					//ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::LowPassQ), "LP Q", M7::gBiquadFilterQCfg, 1.0f, {});

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
					bool sidechain = bandConfig.mOutputStream == Maj7MBC::OutputStream::Sidechain;
					if (ToggleButton(&sidechain, "Detector", { 0,0 }, { "cc22cc", "294a7a", "999999", })) {
						// NB: ToggleButton() has flipped the value.
						bandConfig.mOutputStream = !sidechain ? Maj7MBC::OutputStream::Normal : Maj7MBC::OutputStream::Sidechain;
					}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
					ImGui::SameLine();
					Maj7ImGuiParamFloat01(param(BandParam::ChannelLink), "StereoLink", 0.8f, 0);

					ImGui::SameLine();
					WSImGuiParamCheckbox(param(BandParam::SidechainFilterEnable), "SC Filter");

					M7::QuickParam scfEnabledParam{ GetEffectX()->getParameter(param(BandParam::SidechainFilterEnable)) };
					bool scfEnabled = scfEnabledParam.GetBoolValue();

					if (scfEnabled) {

						// Capture bandIndex in individual lambdas for each band
						auto makeBandHandler = [this](VstInt32 freqParamIndex) {
							return [this, freqParamIndex](float freqHz, float gainDb, uintptr_t userData) {
								M7::QuickParam freqParam;
								freqParam.SetFrequencyAssumingNoKeytracking(M7::gFilterFreqConfig, freqHz);
								float freqParamValue = freqParam.GetRawValue();
								GetEffectX()->setParameterAutomated(freqParamIndex, M7::math::clamp01(freqParamValue));
								};
							};

						// Create Q parameter change handlers for each band
						auto makeBandQHandler = [this](VstInt32 qParamIndex) {
							return [this, qParamIndex](float qValue, uintptr_t userData) {
								M7::QuickParam qParam;
								qParam.SetDivCurvedValue(M7::gBiquadFilterQCfg, qValue);
								float qParamValue = qParam.GetRawValue();
								GetEffectX()->setParameterAutomated(qParamIndex, M7::math::clamp01(qParamValue));
								};
							};


						const std::array<FrequencyResponseRendererFilter, 2> filters{
							FrequencyResponseRendererFilter{"cc4444", &band.mComp[0].mLowpassFilter, "LP", makeBandHandler(param(BandParam::LowPassFrequency)), makeBandQHandler(param(BandParam::LowPassQ))},
							FrequencyResponseRendererFilter{"4444cc", &band.mComp[0].mHighpassFilter, "HP", makeBandHandler(param(BandParam::HighPassFrequency)), makeBandQHandler(param(BandParam::HighPassQ))}
						};

						FrequencyResponseRendererConfig<2, (size_t)Maj7MBC::ParamIndices::NumParams> cfg{
							ColorFromHTML("222222", 1.0f), // background
								ColorFromHTML("ff8800", 1.0f), // line
								7.0f,
								filters,
						};
						for (size_t i = 0; i < (size_t)Maj7MBC::ParamIndices::NumParams; ++i) {
							cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
						}
						ImGui::SameLine();

						mResponseGraphs[iBand].OnRender(cfg);

					}

				} // band enabled

				//ImGui::EndGroup();
				if (bandEnabled) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          auto& ia = band.mInputAnalysis;
          auto& da = band.mDetectorAnalysis;
		  auto& aa = band.mAttenuationAnalysis;
          auto& oa = band.mOutputAnalysis;
#else 
					AnalysisStream ia[2]; // mock
					AnalysisStream da[2]; // mock
					AnalysisStream aa[2]; // mock
					AnalysisStream oa[2]; // mock
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
					mCompressorVisBig[iBand].Render(band.mEnable, true, band.mComp[0], ia, da, aa, oa);
				}
				//ImGui::EndTabItem();
			}

			//EndTabBarWithColoredSeparator();
		}
	}


	void RenderBandSmall(size_t iBand, ParamIndices enabledParam, const char* caption, bool muteSoloEnabled, bool mbEnabledEnabled, bool mbEnabled)
	{
		auto& band = mpMaj7MBC->mBands[iBand];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];
#endif
		bool bandEnabled = band.mEnable;

		ColorMod& cm = (muteSoloEnabled && mbEnabledEnabled) ? mBandColors : mBandDisabledColors;
		auto token = cm.Push();

		auto& vis = mCompressorVisSmall[iBand];

		vis.mShowInputHistory = mShowInputHistory;
		vis.mShowDetectorHistory = mShowDetectorHistory;
		vis.mShowOutputHistory = mShowOutputHistory;
		vis.mShowAttenuationHistory = mShowAttenuationHistory;
		vis.mShowThresh = mShowThresh;
		vis.mShowLeft = mShowLeft;
		vis.mShowRight = mShowRight;

		vis.mShowTransferCurve = false;
		vis.mTickSet = {
		};

		//if (bandEnabled) {
			//ImGui::SameLine();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          auto& ia = band.mInputAnalysis;
          auto& da = band.mDetectorAnalysis;
		  auto& aa = band.mAttenuationAnalysis;
          auto& oa = band.mOutputAnalysis;
#else 
					AnalysisStream ia[2]; // mock
					AnalysisStream da[2]; // mock
					AnalysisStream aa[2]; // mock
					AnalysisStream oa[2]; // mock
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
		mCompressorVisSmall[iBand].Render(band.mEnable, false, band.mComp[0], ia, da, aa, oa);
		//}
	}

	//void CopyBand(int ifrom, int ito)
	//{
	//	auto bandToStr = [&](int i) {
	//		switch (i) {
	//		case 0:
	//			return "lows";
	//		case 1:
	//			return "mids";
	//		case 2:
	//			return "highs";
	//		default:
	//			return "unknown";
	//		}
	//		};
	//	auto bandToParamOffset = [&](int i) {
	//		return mpMaj7MBC->mBands[i].mParams.mBaseParamID;
	//		};
	//	const char* fromstr = bandToStr(ifrom);
	//	const char* tostr = bandToStr(ito);
	//	char s[200];
	//	const int fromParamOffset = bandToParamOffset(ifrom);
	//	const int toParamOffset = bandToParamOffset(ito);
	//	std::sprintf(s, "click OK to copy settings from band %s to %s", fromstr, tostr);
	//	if (::MessageBox(mCurrentWindow, s, "Maj7", MB_ICONQUESTION | MB_OKCANCEL) == IDOK) {

	//		int bandParamCount = (int)ParamIndices::BAttack - (int)ParamIndices::AAttack;
	//		for (int i = 0; i < bandParamCount; ++i) {
	//			float val = mpMaj7MBCVst->getParameter(fromParamOffset + i);
	//			mpMaj7MBCVst->setParameter(toParamOffset + i, val);
	//		}
	//		::MessageBoxA(mCurrentWindow, "Done.", "Maj7", MB_ICONINFORMATION | MB_OK);
	//	}
	//}

private:
	// Helper to get the button area for band overlays
	ImRect GetBandButtonArea(const ImRect& bandRect) const {
		// Place buttons in the upper portion of the band rectangle
		const float buttonHeight = 20.0f;
		const float paddingY = 12.0f;
		const float padding = 4.0f;

		// Calculate width for 4 buttons: Mute, Solo, Enable, Delta
		const float buttonWidth = 22.0f;
		const float buttonPadding = 1.5f;
		const float totalWidth = buttonWidth * 4 + buttonPadding * 3 + padding * 2;

		// Center buttons horizontally in the band, place them near the top
		float centerX = (bandRect.Min.x + bandRect.Max.x) * 0.5f;
		float startX = centerX - totalWidth * 0.5f;

		return ImRect(
			startX,
			bandRect.Min.y + paddingY,
			startX + totalWidth,
			bandRect.Min.y + paddingY + buttonHeight
		);
	}

	// Handle clicks on band overlay buttons
	bool HandleBandClick(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, bool mbEnabled) {
		if (!mbEnabled || bandIndex < 0 || bandIndex >= Maj7MBC::gBandCount) {
			return false;
		}

		auto& band = mpMaj7MBC->mBands[bandIndex];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		auto& bandConfig = band.mVSTConfig;
#endif

		// Get button area and individual button rects
		ImRect buttonArea = GetBandButtonArea(bandRect);
		if (bandRect.GetWidth() < 120.0f) { // Increased width requirement
			return false;
		}

		const float buttonWidth = 22.0f;
		const float buttonHeight = 16.0f;
		const float padding = 1.5f;

		ImVec2 mutePos = { buttonArea.Min.x + 4.0f, buttonArea.Min.y + 4.0f };
		ImVec2 soloPos = { mutePos.x + buttonWidth + padding, mutePos.y };
		ImVec2 enablePos = { soloPos.x + buttonWidth + padding, soloPos.y };
		ImVec2 deltaPos = { enablePos.x + buttonWidth + padding, enablePos.y };

		ImRect muteRect(mutePos, { mutePos.x + buttonWidth, mutePos.y + buttonHeight });
		ImRect soloRect(soloPos, { soloPos.x + buttonWidth, soloPos.y + buttonHeight });
		ImRect enableRect(enablePos, { enablePos.x + buttonWidth, enablePos.y + buttonHeight });
		ImRect deltaRect(deltaPos, { deltaPos.x + buttonWidth, deltaPos.y + buttonHeight });

		ImVec2 mousePos = ImGui::GetIO().MousePos;

		if (enableRect.Contains(mousePos)) {
			// Toggle band enable via parameter system
			using BandParam = Maj7MBC::FreqBand::BandParam;
			VstInt32 paramIndex = band.mParams.mBaseParamID + (VstInt32)BandParam::Enable;
			float currentValue = GetEffectX()->getParameter(paramIndex);
			GetEffectX()->setParameterAutomated(paramIndex, currentValue > 0.5f ? 0.0f : 1.0f);
			return true;
		}
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		else if (muteRect.Contains(mousePos)) {
			bandConfig.mMute = !bandConfig.mMute;
			return true;
		}
		else if (soloRect.Contains(mousePos)) {
			bandConfig.mSolo = !bandConfig.mSolo;
			return true;
		}
		else if (deltaRect.Contains(mousePos)) {
			// Toggle delta output mode
			if (bandConfig.mOutputStream == Maj7MBC::OutputStream::Delta) {
				bandConfig.mOutputStream = Maj7MBC::OutputStream::Normal;
			}
			else {
				bandConfig.mOutputStream = Maj7MBC::OutputStream::Delta;
			}
			return true;
		}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

		return false;
	}

	// Render band overlay with state-dependent styling - REFACTORED
	bool RenderBandOverlay(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl, bool muteSoloEnabled, bool mbEnabled) {
		if (!mbEnabled || bandIndex < 0 || bandIndex >= Maj7MBC::gBandCount) {
			return false;
		}

		auto& band = mpMaj7MBC->mBands[bandIndex];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		auto& bandConfig = band.mVSTConfig;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

		// Get button area for 4 buttons (Mute, Solo, Enable, Delta)
		ImRect buttonArea = GetBandButtonArea(bandRect);

		if (isSelected) {
			dl->AddRect(bandRect.Min, bandRect.Max, ColorFromHTML(bandColors[bandIndex], 0.8f));
			// draw a "selected" rectangle indicator towards the bottom of the band rect
			//ImVec2 bottomLeft = { bandRect.Min.x, bandRect.Min.y };
			//ImVec2 bottomRight = { bandRect.Max.x, bandRect.Min.y };
			dl->AddRectFilled(
				{ bandRect.Min },// min
				{ bandRect.Max.x, bandRect.Min.y + 4 }, // max
				ColorFromHTML(bandColors[bandIndex], 0.8f), 2.0f);
		}

		// Button dimensions for 4 buttons
		const float buttonWidth = 22.0f;  // Smaller buttons to fit 4
		const float buttonHeight = 16.0f;
		const float padding = 1.5f;       // Smaller padding

		ImVec2 mutePos = { buttonArea.Min.x + padding, buttonArea.Min.y + padding };
		ImVec2 soloPos = { mutePos.x + buttonWidth + padding, mutePos.y };
		ImVec2 enablePos = { soloPos.x + buttonWidth + padding, soloPos.y };
		ImVec2 deltaPos = { enablePos.x + buttonWidth + padding, enablePos.y };

		ImRect muteRect(mutePos, { mutePos.x + buttonWidth, mutePos.y + buttonHeight });
		ImRect soloRect(soloPos, { soloPos.x + buttonWidth, soloPos.y + buttonHeight });
		ImRect enableRect(enablePos, { enablePos.x + buttonWidth, enablePos.y + buttonHeight });
		ImRect deltaRect(deltaPos, { deltaPos.x + buttonWidth, deltaPos.y + buttonHeight });

		// Check for mouse hover on individual buttons
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		bool muteHovered = muteRect.Contains(mousePos);
		bool soloHovered = soloRect.Contains(mousePos);
		bool enableHovered = enableRect.Contains(mousePos);
		bool deltaHovered = deltaRect.Contains(mousePos);
		bool anyButtonHovered = muteHovered || soloHovered || enableHovered || deltaHovered;

		// Render MUTE button
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		ImColor muteColor = bandConfig.mMute ?
			ColorFromHTML("cc4444", 0.9f) :
			ColorFromHTML("444444", muteHovered ? 0.8f : 0.6f);
		ImColor muteTextColor = bandConfig.mMute ?
			ColorFromHTML("ffffff") :
			ColorFromHTML(muteHovered ? "ffffff" : "cccccc");

		dl->AddRectFilled(muteRect.Min, muteRect.Max, muteColor, 2.0f);
		dl->AddRect(muteRect.Min, muteRect.Max, ColorFromHTML("888888", 0.8f), 2.0f, 0, 1.0f);

		// Center text in button
		ImVec2 muteTextSize = ImGui::CalcTextSize("M");
		ImVec2 muteTextPos = {
			muteRect.Min.x + (muteRect.GetWidth() - muteTextSize.x) * 0.5f,
			muteRect.Min.y + (muteRect.GetHeight() - muteTextSize.y) * 0.5f
		};
		dl->AddText(muteTextPos, muteTextColor, "M");

		// Render SOLO button  
		ImColor soloColor = bandConfig.mSolo ?
			ColorFromHTML("cccc44", 0.9f) :
			ColorFromHTML("444444", soloHovered ? 0.8f : 0.6f);
		ImColor soloTextColor = bandConfig.mSolo ?
			ColorFromHTML("000000") :
			ColorFromHTML(soloHovered ? "ffffff" : "cccccc");

		dl->AddRectFilled(soloRect.Min, soloRect.Max, soloColor, 2.0f);
		dl->AddRect(soloRect.Min, soloRect.Max, ColorFromHTML("888888", 0.8f), 2.0f, 0, 1.0f);

		// Center text in button
		ImVec2 soloTextSize = ImGui::CalcTextSize("S");
		ImVec2 soloTextPos = {
			soloRect.Min.x + (soloRect.GetWidth() - soloTextSize.x) * 0.5f,
			soloRect.Min.y + (soloRect.GetHeight() - soloTextSize.y) * 0.5f
		};
		dl->AddText(soloTextPos, soloTextColor, "S");
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
		// Render ENABLE button
		ImColor enableColor = band.mEnable ?
			ColorFromHTML("44cc44", 0.9f) :
			ColorFromHTML("444444", enableHovered ? 0.8f : 0.6f);
		ImColor enableTextColor = band.mEnable ?
			ColorFromHTML("ffffff") :
			ColorFromHTML(enableHovered ? "ffffff" : "cccccc");

		dl->AddRectFilled(enableRect.Min, enableRect.Max, enableColor, 2.0f);
		dl->AddRect(enableRect.Min, enableRect.Max, ColorFromHTML("888888", 0.8f), 2.0f, 0, 1.0f);

		// Center text in button
		ImVec2 enableTextSize = ImGui::CalcTextSize("E");
		ImVec2 enableTextPos = {
			enableRect.Min.x + (enableRect.GetWidth() - enableTextSize.x) * 0.5f,
			enableRect.Min.y + (enableRect.GetHeight() - enableTextSize.y) * 0.5f
		};
		dl->AddText(enableTextPos, enableTextColor, "E");

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		// Render DELTA button
		bool isDelta = (bandConfig.mOutputStream == Maj7MBC::OutputStream::Delta);
		ImColor deltaColor = isDelta ?
			ColorFromHTML("cc44cc", 0.9f) :
			ColorFromHTML("444444", deltaHovered ? 0.8f : 0.6f);
		ImColor deltaTextColor = isDelta ?
			ColorFromHTML("ffffff") :
			ColorFromHTML(deltaHovered ? "ffffff" : "cccccc");

		dl->AddRectFilled(deltaRect.Min, deltaRect.Max, deltaColor, 2.0f);
		dl->AddRect(deltaRect.Min, deltaRect.Max, ColorFromHTML("888888", 0.8f), 2.0f, 0, 1.0f);

		// Center text in button
		ImVec2 deltaTextSize = ImGui::CalcTextSize("D");
		ImVec2 deltaTextPos = {
			deltaRect.Min.x + (deltaRect.GetWidth() - deltaTextSize.x) * 0.5f,
			deltaRect.Min.y + (deltaRect.GetHeight() - deltaTextSize.y) * 0.5f
		};
		dl->AddText(deltaTextPos, deltaTextColor, "D");
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

		// Set cursor for hoverable buttons
		if (anyButtonHovered) {
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
		}

		return anyButtonHovered; // Return true if we're showing interactive elements
	}

public:
	virtual void renderImgui() override
	{
		mBandColors.EnsureInitialized();
		mBandDisabledColors.EnsureInitialized();

		bool muteSoloEnabled[Maj7MBC::gBandCount] = { false, false, false };
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		bool mutes[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mMute, mpMaj7MBC->mBands[1].mVSTConfig.mMute , mpMaj7MBC->mBands[2].mVSTConfig.mMute };
		bool solos[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mSolo, mpMaj7MBC->mBands[1].mVSTConfig.mSolo , mpMaj7MBC->mBands[2].mVSTConfig.mSolo };
		M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);
#endif
		float backing = mpMaj7MBCVst->getParameter((int)ParamIndices::MultibandEnable);
		M7::ParamAccessor pa{ &backing, 0 };
		bool mbEnabled = pa.GetBoolValue(0);

		if (!mbEnabled) {
			// "single" band = mids
			mEditingBand = 1;
		}

		{
			bool mbdisabled = !mbEnabled;
			if (ToggleButton(&mbdisabled, "Single-band", { 90,20 })) {
				pa.SetBoolValue(0, false);
				mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
			}
			ImGui::SameLine();
			if (ToggleButton(&mbEnabled, "Multi-band", { 90,20 })) {
				pa.SetBoolValue(0, true);
				mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
			}

			ImGui::SameLine(0, 200);

			ToggleButton(&mShowInputFft, "Input FFT");
			ImGui::SameLine();
			ToggleButton(&mShowOutputFft, "Output FFT");
			ImGui::SameLine();
			ToggleButton(&mShowFftDiff, "FFT Diff");
			ImGui::SameLine();
			ToggleButton(&mShowFftDiffFlat, "FFT Diff (flat)");

			if (mbEnabled) {
				ImGui::SameLine();
				ToggleButton(&mShowCrossoverResponse, "Crossover response");
			}

			ImGui::Spacing();

			FrequencyResponseRendererConfig<0, (size_t)Maj7MBC::ParamIndices::NumParams> crossoverCfg{
				ColorFromHTML("222222"),
				ColorFromHTML("ff00ff"),
				4.0f,
				{},
			};
			for (size_t i = 0; i < (size_t)Maj7MBC::ParamIndices::NumParams; ++i) {
				crossoverCfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
			}

			// FFT overlays
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			crossoverCfg.fftOverlays.clear();
			if (mShowInputFft) {
				crossoverCfg.fftOverlays.push_back({
					&mpMaj7MBC->mInputSpectrum,
					ColorFromHTML("888888", 0.8f),
					ColorFromHTML("444444", 0.3f),
					true,
					"Input"
					});
			}

			if (mShowOutputFft) {
				crossoverCfg.fftOverlays.push_back({
					&mpMaj7MBC->mOutputSpectrum,
					ColorFromHTML(bandColors[1], 0.5f),
					ColorFromHTML(bandColors[1], 0.2f),
					true,
					"Output"
					});
			}

			// Configure FFT diff overlay
			if (mShowFftDiff) {
				FFTDiffOverlay diff{};
				diff.sourceA = &mpMaj7MBC->mInputSpectrum;
				diff.sourceB = &mpMaj7MBC->mOutputSpectrum;
				mCrossoverGraph.SetFFTDiffOverlay(diff);
			} else {
				mCrossoverGraph.ClearFFTDiffOverlay();
			}

			if (mShowFftDiffFlat) {
				FFTDiffFlatOverlay diffFlat{};
				diffFlat.sourceB = &mpMaj7MBC->mInputSpectrum;
				diffFlat.sourceA = &mpMaj7MBC->mOutputSpectrum;
				// Default symmetric scale already set in layer; can override if desired:
				mCrossoverGraph.SetFFTDiffFlatOverlay(diffFlat);
				// Ensure default scale [-24..24]
				mCrossoverGraph.SetFFTDiffFlatScaling(-24.0f, +24.0f, true);
			} else {
				mCrossoverGraph.ClearFFTDiffFlatOverlay();
			}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
			mCrossoverGraph.mCrossoverLayer->mShowResponses = mShowCrossoverResponse;

			mCrossoverGraph.mCrossoverLayer->mGetBandColor = [this](size_t bandIndex, bool hovered) -> ImColor {
				auto disabledColor = ColorFromHTML("444444", hovered ? 0.8f : 0.6f);
				// determine if this band is enabled
				switch (bandIndex) {
				case 0:
				{
					M7::QuickParam aEnableParam{ GetEffectX()->getParameter((VstInt32)ParamIndices::AEnable) };
					if (!aEnableParam.GetBoolValue()) {
						return disabledColor;
					}
					break;
				}
				case 1:
				{
					M7::QuickParam aEnableParam{ GetEffectX()->getParameter((VstInt32)ParamIndices::BEnable) };
					if (!aEnableParam.GetBoolValue()) {
						return disabledColor;
					}
					break;
				}
				case 2:
				{
					M7::QuickParam aEnableParam{ GetEffectX()->getParameter((VstInt32)ParamIndices::CEnable) };
					if (!aEnableParam.GetBoolValue()) {
						return disabledColor;
					}
					break;
				}
				default:
					return ColorFromHTML("ff00ff");
				}

				return ColorFromHTML(bandColors[bandIndex]);
				};
			mCrossoverGraph.SetCrossoverFilter(mbEnabled ? mpMaj7MBC : nullptr);
			mCrossoverGraph.SetCurrentEditingBand(mEditingBand);

			auto bandRenderer = [this, mbEnabled, &muteSoloEnabled](int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl) -> bool {
				// Only process valid bands in multiband mode
				if (!mbEnabled || bandIndex < 0 || bandIndex >= Maj7MBC::gBandCount) {
					return false;
				}

				// If dl is nullptr, this is a mouse click area test
				if (dl == nullptr) {
					ImVec2 mousePos = ImGui::GetIO().MousePos;
					ImRect buttonArea = GetBandButtonArea(bandRect);
					return buttonArea.Contains(mousePos);
				}

				// If dl is special marker (1), this is click handling
				if (dl == reinterpret_cast<ImDrawList*>(1)) {
					return HandleBandClick(bandIndex, bandRect, isHovered, isSelected, mbEnabled);
				}

				// Otherwise, this is normal rendering
				return RenderBandOverlay(bandIndex, bandRect, isHovered, isSelected, dl, muteSoloEnabled[bandIndex], mbEnabled);
				};

			mCrossoverGraph.SetBandRenderer(bandRenderer);

			// Set up parameter change handler for crossover frequency dragging
			if (mbEnabled) {
				auto crossoverFreqHandler = [this](float freqHz, int crossoverIndex) {
					// Convert freqHz to the param value
					M7::QuickParam freqParam;
					freqParam.SetFrequencyAssumingNoKeytracking(M7::gFilterFreqConfig, freqHz);
					float freqParamValue = freqParam.GetRawValue();

					// Determine which parameter to set based on crossover index
					VstInt32 paramIndex;
					if (crossoverIndex == 0) {
						paramIndex = (VstInt32)ParamIndices::CrossoverAFrequency;
					}
					else if (crossoverIndex == 1) {
						paramIndex = (VstInt32)ParamIndices::CrossoverBFrequency;
					}
					else {
						return; // Invalid crossover index
					}

					// Set the parameter using VST automation
					GetEffectX()->setParameterAutomated(paramIndex, M7::math::clamp01(freqParamValue));
					};

				mCrossoverGraph.SetFrequencyChangeHandler(crossoverFreqHandler);

				// Set up band selection handler for clicking on band regions
				auto bandChangeHandler = [this](int bandIndex) {
					// Validate band index
					if (bandIndex >= 0 && bandIndex < Maj7MBC::gBandCount) {
						mEditingBand = bandIndex;
					}
					};

				mCrossoverGraph.SetBandChangeHandler(bandChangeHandler);
			}
			else {
				mCrossoverGraph.SetFrequencyChangeHandler(nullptr);
				mCrossoverGraph.SetBandChangeHandler(nullptr);
			}

			ImGui::SameLine();

			mCrossoverGraph.OnRender(crossoverCfg);
		}

		ImGui::Spacing();

		switch (mEditingBand)
		{
		default:
		case 0:
		{
			ImGui::PushID("band0");
			RenderBand(0, ParamIndices::AInputGain, "Lows", muteSoloEnabled[0], mbEnabled, mbEnabled);
			ImGui::PopID();
			break;
		}
		case 1:
		{
			ImGui::PushID("band1");
			RenderBand(1, ParamIndices::BInputGain, mbEnabled ? "Mids" : "All frequencies", muteSoloEnabled[1], true, mbEnabled);
			ImGui::PopID();
			break;
		}
		case 2:
		{
			ImGui::PushID("band2");
			RenderBand(2, ParamIndices::CInputGain, "Highs", muteSoloEnabled[2], mbEnabled, mbEnabled);
			ImGui::PopID();
			break;
		}
		}

		{
			// global controls.
			//OutputGain,
			//	SoftClipEnable,
			//	SoftClipThresh,
			//	SoftClipOutput,
			
			ImGui::Spacing();
			ImGui::SeparatorText("Global controls");
			ImGui::Spacing();


			ImGui::SameLine(0, 80); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
			ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});


			ImGui::SameLine(); Maj7ImGuiBoolParamToggleButton(ParamIndices::SoftClipEnable, "Softclip");
			M7::QuickParam qp{ mpMaj7MBCVst->getParameter((VstInt32)ParamIndices::SoftClipEnable) };
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
					}, [&](float x) -> float {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
					return Maj7MBC::Softclip(x, softClipThreshLin, 1)[0];
#else
					return 0;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
					});

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        auto& ca = mpMaj7MBC->mClippingAnalysis;
#else
        AnalysisStream ca[2];  // mock
#endif

				ImGui::SameLine(); VUMeterAtten("scclip", ca[0], ca[1], { 30,120 });
			}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      auto& ia = mpMaj7MBC->mInputAnalysis;
      auto& oa = mpMaj7MBC->mOutputAnalysis;
#else
        AnalysisStream ia[2];  // mock
        AnalysisStream oa[2];  // mock
#endif

			ImGui::SameLine(); VUMeter("main_vu_inp", ia[0], ia[0], kMainVuMeterSize, "Input Left", "Input Right");
			ImGui::SameLine(); VUMeter("main_vu_outp", oa[0], oa[0], kMainVuMeterSize, "Output Left", "Output Right");

		}
		//if (mbEnabled) {
		//}

		//ImGui::PushID("band1");
		//RenderBand(1, ParamIndices::BInputGain, mbEnabled ? "Mids" : "All frequencies", muteSoloEnabled[1], true, mbEnabled);
		//ImGui::PopID();

		//if (mbEnabled) {
		//	ImGui::PushID("band2");
		//	RenderBand(2, ParamIndices::CInputGain, "Highs", muteSoloEnabled[2], mbEnabled, mbEnabled);
		//	ImGui::PopID();
		//}
	}

};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7MBCEditor(audioEffect);
}
