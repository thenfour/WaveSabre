#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7MBCVst.hpp"
#include <WaveSabreVstLib/FrequencyResponseRendererLayered.hpp>

struct Maj7MBCEditor : public VstEditor
{
	Maj7MBC* mpMaj7MBC;
	Maj7MBCVst* mpMaj7MBCVst;
	using ParamIndices = Maj7MBC::ParamIndices;

	//CompressorVis<330, 120> mCompressorVis[Maj7MBC::gBandCount];
	CompressorVis<650, 320> mCompressorVisBig[Maj7MBC::gBandCount];
	CompressorVis<180, 50> mCompressorVisSmall[Maj7MBC::gBandCount];

	ColorMod mBandColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mBandDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	int mEditingBand = 0;

	FrequencyResponseRendererLayered<270, 100, 2, (size_t)Maj7MBC::ParamIndices::NumParams, false> mResponseGraphs[Maj7MBC::gBandCount];
	FrequencyResponseRendererLayered<1000, 180, 0, (size_t)Maj7MBC::ParamIndices::NumParams, true> mCrossoverGraph;

	Maj7MBCEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1150, 950),
		mpMaj7MBCVst((Maj7MBCVst*)audioEffect)
	{
		mpMaj7MBC = ((Maj7MBCVst*)audioEffect)->GetMaj7MBC();
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

				ImGui::BeginGroup();

				// MUTE | SOLO
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				Maj7ImGuiBoolParamToggleButton(param(BandParam::Enable), "Enable", { 0,0 }, { "22aa22", "294a7a", "999999", });

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

				if (bandEnabled)
				{
					Maj7ImGuiParamScaledFloat(param(BandParam::Threshold), "Thresh(dB)", -60, 0, -20, 0, 0, {});
					ImGui::SameLine(); Maj7ImGuiDivCurvedParam(param(BandParam::Ratio), "Ratio", MonoCompressor::gRatioCfg, 4, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(param(BandParam::Knee), "Knee(dB)", 0, 30, 4, 0, 0, {});
					ImGui::SameLine(0, 80); Maj7ImGuiPowCurvedParam(param(BandParam::Attack), "Attack(ms)", MonoCompressor::gAttackCfg, 50, {});
					ImGui::SameLine(); Maj7ImGuiPowCurvedParam(param(BandParam::Release), "Release(ms)", MonoCompressor::gReleaseCfg, 80, {});

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

					Maj7ImGuiParamFloat01(param(BandParam::ChannelLink), "StereoLink", 0.8f, 0);
					ImGui::SameLine(0, 40); Maj7ImGuiParamVolume(param(BandParam::Drive), "Drive", M7::gVolumeCfg36db, 0, {});
					ImGui::SameLine(0, 40); Maj7ImGuiParamVolume(param(BandParam::InputGain), "Input", M7::gVolumeCfg24db, 0, {});

					ImGui::SameLine(); Maj7ImGuiParamVolume(param(BandParam::OutputGain), "Makeup", M7::gVolumeCfg24db, 0, {});
					ImGui::SameLine(0, 40); Maj7ImGuiParamFloat01(param(BandParam::DryWet), "Dry-Wet", 1.0f, 0);
				} // band enabled

				ImGui::EndGroup();

				if (bandEnabled) {
					mCompressorVisBig[iBand].Render(band.mEnable, true, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);
				}

				ImGui::EndTabItem();
			}

			EndTabBarWithColoredSeparator();
		}
	}


	void RenderBandSmall(size_t iBand, ParamIndices enabledParam, const char* caption, bool muteSoloEnabled, bool mbEnabledEnabled, bool mbEnabled)
	{
		auto& band = mpMaj7MBC->mBands[iBand];
		auto& bandConfig = band.mVSTConfig;// mpMaj7MBCVst->mBandConfig[iBand];
		bool bandEnabled = band.mEnable;

		ColorMod& cm = (muteSoloEnabled && mbEnabledEnabled) ? mBandColors : mBandDisabledColors;
		auto token = cm.Push();

		if (bandEnabled) {
			ImGui::SameLine();
			mCompressorVisSmall[iBand].Render(band.mEnable, false, band.mComp[0], band.mInputAnalysis, band.mDetectorAnalysis, band.mAttenuationAnalysis, band.mOutputAnalysis);
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

private:
	// Helper to get the button area for band overlays - REFACTORED
	ImRect GetBandButtonArea(const ImRect& bandRect) const {
		// Place buttons in the upper portion of the band rectangle
		const float buttonHeight = 20.0f;
		const float buttonWidth = 60.0f;
		const float padding = 4.0f;
		
		// Center buttons horizontally in the band, place them near the top
		float centerX = (bandRect.Min.x + bandRect.Max.x) * 0.5f;
		float totalWidth = buttonWidth * 2 + padding; // Two buttons with padding
		float startX = centerX - totalWidth * 0.5f;
		
		return ImRect(
			startX, 
			bandRect.Min.y + padding,
			startX + totalWidth,
			bandRect.Min.y + padding + buttonHeight
		);
	}

	// Handle clicks on band overlay buttons
	bool HandleBandClick(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, bool mbEnabled) {
		if (!mbEnabled || bandIndex < 0 || bandIndex >= Maj7MBC::gBandCount) {
			return false;
		}

		auto& band = mpMaj7MBC->mBands[bandIndex];
		auto& bandConfig = band.mVSTConfig;
		
		// Get button area and individual button rects
		ImRect buttonArea = GetBandButtonArea(bandRect);
		if (bandRect.GetWidth() < 80.0f) {
			return false;
		}

		const float buttonWidth = 28.0f;
		const float buttonHeight = 16.0f;
		const float padding = 2.0f;
		
		ImVec2 mutePos = { buttonArea.Min.x + padding, buttonArea.Min.y + padding };
		ImVec2 soloPos = { mutePos.x + buttonWidth + padding, mutePos.y };
		
		ImRect muteRect(mutePos, { mutePos.x + buttonWidth, mutePos.y + buttonHeight });
		ImRect soloRect(soloPos, { soloPos.x + buttonWidth, soloPos.y + buttonHeight });

		ImVec2 mousePos = ImGui::GetIO().MousePos;
		
		if (muteRect.Contains(mousePos)) {
			bandConfig.mMute = !bandConfig.mMute;
			return true;
		} else if (soloRect.Contains(mousePos)) {
			bandConfig.mSolo = !bandConfig.mSolo;
			return true;
		}
		
		return false;
	}

	// Render band overlay with state-dependent styling - REFACTORED
	bool RenderBandOverlay(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl, bool muteSoloEnabled, bool mbEnabled) {
		if (!mbEnabled || bandIndex < 0 || bandIndex >= Maj7MBC::gBandCount) {
			return false;
		}

		auto& band = mpMaj7MBC->mBands[bandIndex];
		auto& bandConfig = band.mVSTConfig;
		
		// Get button area
		ImRect buttonArea = GetBandButtonArea(bandRect);

		// Draw semi-transparent background for buttons
		//ImColor bgColor = ColorFromHTML("000000", 0.8f);
		//dl->AddRectFilled(buttonArea.Min, buttonArea.Max, bgColor, 4.0f);
		//dl->AddRect(buttonArea.Min, buttonArea.Max, ColorFromHTML("666666", 0.9f), 4.0f, 0, 1.0f);

		// Button dimensions
		const float buttonWidth = 28.0f;
		const float buttonHeight = 16.0f;
		const float padding = 2.0f;
		
		ImVec2 mutePos = { buttonArea.Min.x + padding, buttonArea.Min.y + padding };
		ImVec2 soloPos = { mutePos.x + buttonWidth + padding, mutePos.y };
		
		ImRect muteRect(mutePos, { mutePos.x + buttonWidth, mutePos.y + buttonHeight });
		ImRect soloRect(soloPos, { soloPos.x + buttonWidth, soloPos.y + buttonHeight });

		// Check for mouse hover on individual buttons
		ImVec2 mousePos = ImGui::GetIO().MousePos;
		bool muteHovered = muteRect.Contains(mousePos);
		bool soloHovered = soloRect.Contains(mousePos);
		bool anyButtonHovered = muteHovered || soloHovered;

		// Render MUTE button
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
		bool mutes[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mMute, mpMaj7MBC->mBands[1].mVSTConfig.mMute , mpMaj7MBC->mBands[2].mVSTConfig.mMute };
		bool solos[Maj7MBC::gBandCount] = { mpMaj7MBC->mBands[0].mVSTConfig.mSolo, mpMaj7MBC->mBands[1].mVSTConfig.mSolo , mpMaj7MBC->mBands[2].mVSTConfig.mSolo };
		M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);

		float backing = mpMaj7MBCVst->getParameter((int)ParamIndices::MultibandEnable);
		M7::ParamAccessor pa{ &backing, 0 };
		bool mbEnabled = pa.GetBoolValue(0);


		if (BeginTabBar2("general", ImGuiTabBarFlags_None))
		{

			if (WSBeginTabItem("IO"))
			{

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				bool mbdisabled = !mbEnabled;
				ImGui::BeginGroup();
				if (ToggleButton(&mbdisabled, "SINGLE BAND", { 90,40 })) {

					// NB: ToggleButton() has flipped the value.
					pa.SetBoolValue(0, false);
					mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
				}
				if (ToggleButton(&mbEnabled, "MULTIBAND", { 90,40 })) {
					// NB: ToggleButton() has flipped the value.
					pa.SetBoolValue(0, true);
					mpMaj7MBCVst->setParameter((int)ParamIndices::MultibandEnable, backing);
				}
				ImGui::EndGroup();

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

				// Show crossover visualization if multiband is enabled
				// Build renderer config
				FrequencyResponseRendererConfig<0, (size_t)Maj7MBC::ParamIndices::NumParams> crossoverCfg{
					ColorFromHTML("222222", 1.0f), // background
					ColorFromHTML("ff8800", 1.0f), // line (unused for crossover)
					4.0f,
					{}, // no EQ filters for crossover view
				};
				for (size_t i = 0; i < (size_t)Maj7MBC::ParamIndices::NumParams; ++i) {
					crossoverCfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
				}

				// Optional FFT overlay for input signal
				crossoverCfg.fftOverlays = {
					{
						&mpMaj7MBC->mInputSpectrum,  // Input signal (before processing)
						ColorFromHTML("888888", 0.8f),
						ColorFromHTML("444444", 0.3f),
						true,
						"Input"
					}
				};

				// Connect the crossover renderer directly to device; it will read params itself
				mCrossoverGraph.SetCrossoverFilter(mbEnabled ? mpMaj7MBC : nullptr);

				// Set the current editing band for highlighting
				mCrossoverGraph.SetCurrentEditingBand(mEditingBand);

				// Set up band renderer - REFACTORED
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
				ImGui::BeginGroup();
				mCrossoverGraph.OnRender(crossoverCfg);

				ImGui::NewLine();

				if (mbEnabled) {
					ImGui::PushID("band0small");
					RenderBandSmall(0, ParamIndices::AInputGain, "Lows", muteSoloEnabled[0], mbEnabled, mbEnabled);
					ImGui::PopID();

					ImGui::PushID("band1small");
					RenderBandSmall(1, ParamIndices::BInputGain, mbEnabled ? "Mids" : "All frequencies", muteSoloEnabled[1], true, mbEnabled);
					ImGui::PopID();

					ImGui::PushID("band2small");
					RenderBandSmall(2, ParamIndices::CInputGain, "Highs", muteSoloEnabled[2], mbEnabled, mbEnabled);
					ImGui::PopID();
				}

				ImGui::EndGroup();

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

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
