#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "LevellerVst.h"
#include <WaveSabreVstLib/FrequencyResponseRendererLayered.hpp>


///////////////////////////////////////////////////////////////////////////////////////////////////
class LevellerEditor : public VstEditor
{
	using FFTSize = MonoFFTAnalysis::FFTSize;
	using WindowType = MonoFFTAnalysis::WindowType;
	Leveller* mpLeveller;
	LevellerVst* mpLevellerVST;

	ColorMod mEnabledColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	FrequencyResponseRendererLayered<780, 400, Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams, true> mResponseGraph;

	// FFT Analysis controls (not VST parameters, just UI state)
	float mFFTSmoothingFactor;// = 0.0f;      // 0.0-0.9
	int mFFTOverlapFactor;// = 2;             // 2, 4, 8, or 16 (more options for testing)
	float mSpectrumPeakHoldTime;// = 300.0f;    // 0-1000 ms
	float mSpectrumFalloffTime;//= 2000.0f;   // 50-5000 ms
	//FFTSize mFFTSizeSelection;// = 1;             // 0=512, 1=1024, 2=2048, 3=4096
	//WindowType mFFTWindowType;// = 1;                // 0=Rectangular, 1=Hanning, 2=Hamming, 3=Blackman
	float mFFTDisplayMinDB = -80.0f;       // Noise floor
	float mFFTDisplayMaxDB = 0.0f;         // Digital maximum  

public:
	LevellerEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 900),
		mpLevellerVST((LevellerVst*)audioEffect)//,
	{
		mpLeveller = (Leveller*)mpLevellerVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.

		// Initialize FFT control values to match current Leveller settings
		mFFTSmoothingFactor = mpLeveller->mInputSpectrumSmoother.GetFFTSmoothing();
		mFFTOverlapFactor = mpLeveller->mInputSpectrumSmoother.GetOverlapFactor();
		mSpectrumPeakHoldTime = mpLeveller->mInputSpectrumSmoother.GetPeakHoldTime();
		mSpectrumFalloffTime = mpLeveller->mInputSpectrumSmoother.GetFalloffTime();
		//mFFTSizeSelection = mpLeveller->mInputSpectrumSmoother.GetFFTSize();
		//mFFTWindowType = mpLeveller->mInputSpectrumSmoother.GetWindowType();
	}

	virtual void PopulateMenuBar() override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Leveller", mpLeveller, mpLevellerVST, "gLevellerDefaults16", "ParamIndices::NumParams", mpLeveller->mParamCache, paramNames);
	}

	void RenderBand(int id, const char* label, Leveller::ParamIndices paramOffset, float defaultFreqParamHz, const char* colorRaw)
	{
		ImGui::PushID(id);

		ImColor color = ColorFromHTML(colorRaw, 0.8f);

		M7::QuickParam enabledParam{ GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Enable) };
		ColorMod& cm = enabledParam.GetBoolValue() ? mEnabledColors : mDisabledColors;

		auto token = cm.Push();

		ImRect bb;
		ImVec2 gSize{ 20,80 };
		bb.Min = ImGui::GetCursorScreenPos();
		bb.Max = bb.Min + gSize;
		auto* dl = ImGui::GetWindowDrawList();

		dl->AddRectFilled(bb.Min, bb.Max, color);
		// black label centered in bb
		ImVec2 textSize = ImGui::CalcTextSize(label);
		ImVec2 bbCenter = { bb.Min.x + (bb.Max.x - bb.Min.x) * 0.5f, bb.Min.y + (bb.Max.y - bb.Min.y) * 0.5f };
		ImVec2 textPos = { bbCenter.x - textSize.x * 0.5f, bbCenter.y - textSize.y * 0.5f };
		dl->AddText(textPos, ColorFromHTML("000000"), label);

		ImGui::Dummy(gSize);

		ImGui::SameLine(); WSImGuiParamCheckbox((int)paramOffset + (int)Leveller::BandParamOffsets::Enable, "Enable?");

		M7::QuickParam fp{ M7::gFilterFreqConfig };
		fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);

		ImGui::SameLine();  Maj7ImGuiParamFrequency((int)paramOffset + (int)Leveller::BandParamOffsets::Freq, -1, "Freq", M7::gFilterFreqConfig, fp.GetRawValue(), {});

		float typeB = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Type);
		M7::ParamAccessor typePA{ &typeB, 0 };
		BiquadFilterType type = typePA.GetEnumValue< BiquadFilterType>(0);
		ImGui::BeginDisabled(type == BiquadFilterType::Highpass || type == BiquadFilterType::Lowpass);
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)paramOffset + (int)Leveller::BandParamOffsets::Gain, "Gain(db)", -30.0f, 30.0f, 0, 0, 0, {});
		ImGui::EndDisabled();
		ImGui::SameLine(); Maj7ImGuiDivCurvedParam((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q", M7::gBiquadFilterQCfg, 1.00f, {});

		const char* selectedColor = "4400aa";
		const char* notSelectedColor = "222222";
		const char* selectedHoveredColor = "8800ff";
		const char* notSelectedHoveredColor = "222299";

		ImGui::SameLine(); Maj7ImGuiParamEnumMutexButtonArray<BiquadFilterType>((int)paramOffset + (int)Leveller::BandParamOffsets::Type, "type", 40, true, {
			{ "/HP", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Highpass, },
			{ "/LS", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::LowShelf, },
			{ "^Peak", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Peak, },
			{ "\\HS", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::HighShelf, },
			{ "\\LP", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Lowpass, },
			});

		ImGui::PopID();
	}


	virtual void renderImgui() override
	{
		mEnabledColors.EnsureInitialized();
		mDisabledColors.EnsureInitialized();

		ImGui::BeginGroup();

		RenderBand(0, "A", Leveller::ParamIndices::Band1Type, 90, bandColors[0]);
		RenderBand(1, "B", Leveller::ParamIndices::Band2Type, 250, bandColors[1]);
		RenderBand(2, "C", Leveller::ParamIndices::Band3Type, 1100, bandColors[2]);
		RenderBand(3, "D", Leveller::ParamIndices::Band4Type, 3000, bandColors[3]);
		RenderBand(4, "E", Leveller::ParamIndices::Band5Type, 8500, bandColors[4]);

		ImGui::EndGroup();

		ImGui::SameLine();

		ImGui::BeginGroup();

		VUMeter("vu_inp", mpLeveller->mInputAnalysis[0], mpLeveller->mInputAnalysis[1]);
		ImGui::SameLine(); VUMeter("vu_outp", mpLeveller->mOutputAnalysis[0], mpLeveller->mOutputAnalysis[1]);

		Maj7ImGuiParamVolume((VstInt32)Leveller::ParamIndices::OutputVolume, "Output", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
		ImGui::SameLine();
		Maj7ImGuiBoolParamToggleButton(Leveller::ParamIndices::EnableDCFilter, "DC Filt");

		ImGui::EndGroup();

		// Capture bandIndex in individual lambdas for each band
		auto makeBandHandler = [this](uintptr_t bandIndex) {
			return [this, bandIndex](float freqHz, float gainDb, uintptr_t userData) {
				// Convert freqHz to the param value
				M7::QuickParam freqParam;
				freqParam.SetFrequencyAssumingNoKeytracking(M7::gFilterFreqConfig, freqHz);
				float freqParamValue = freqParam.GetRawValue();
				
				// Convert gainDb to the param value using scaled real value (-30 to +30 dB)
				M7::QuickParam gainParam;
				gainParam.SetScaledValue(M7::gEqBandGainMin, M7::gEqBandGainMax, gainDb);
				float gainParamValue = gainParam.GetRawValue();
				
				// Calculate the parameter indices based on band index
				// Each band has 5 parameters: Type, Freq, Gain, Q, Enable
				VstInt32 freqParamIndex = (VstInt32)Leveller::ParamIndices::Band1Freq + (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
				VstInt32 gainParamIndex = (VstInt32)Leveller::ParamIndices::Band1Gain + (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
				
				// Set the parameters using VST automation
				GetEffectX()->setParameterAutomated(freqParamIndex, M7::math::clamp01(freqParamValue));
				GetEffectX()->setParameterAutomated(gainParamIndex, M7::math::clamp01(gainParamValue));
			};
		};

		// Create Q parameter change handlers for each band
		auto makeBandQHandler = [this](uintptr_t bandIndex) {
			return [this, bandIndex](float qValue, uintptr_t userData) {
				// Convert Q value to parameter value using the div curved parameter configuration
				M7::QuickParam qParam;
				qParam.SetDivCurvedValue(M7::gBiquadFilterQCfg, qValue);
				float qParamValue = qParam.GetRawValue();
				
				// Calculate the Q parameter index for this band
				VstInt32 qParamIndex = (VstInt32)Leveller::ParamIndices::Band1Q + (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
				
				// Set the Q parameter using VST automation
				GetEffectX()->setParameterAutomated(qParamIndex, M7::math::clamp01(qParamValue));
			};
		};

		const std::array<FrequencyResponseRendererFilter, Leveller::gBandCount> filters = {
			FrequencyResponseRendererFilter{
				bandColors[0],
				(GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band1Enable) > 0.5f) ? &mpLeveller->mBands[0].mFilters[0] : nullptr,
				"A", makeBandHandler(0), makeBandQHandler(0), 0
		},
			FrequencyResponseRendererFilter{
				bandColors[1], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band2Enable) > 0.5f) ? &mpLeveller->mBands[1].mFilters[0] : nullptr, "B", makeBandHandler(1), makeBandQHandler(1), 1},
			FrequencyResponseRendererFilter{bandColors[2], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band3Enable) > 0.5f) ? &mpLeveller->mBands[2].mFilters[0] : nullptr, "C", makeBandHandler(2), makeBandQHandler(2), 2},
			FrequencyResponseRendererFilter{bandColors[3], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band4Enable) > 0.5f) ? &mpLeveller->mBands[3].mFilters[0] : nullptr, "D", makeBandHandler(3), makeBandQHandler(3), 3},
			FrequencyResponseRendererFilter{bandColors[4], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band5Enable) > 0.5f) ? &mpLeveller->mBands[4].mFilters[0] : nullptr, "E", makeBandHandler(4), makeBandQHandler(4), 4},
		};

		FrequencyResponseRendererConfig<Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams> cfg{
			ColorFromHTML("222222", 1.0f), // background
			ColorFromHTML("aaaa00", 1.0f), // line
			9.0f, // thumbRadius
			filters, // filters array
			{}, // mParamCacheCopy (will be filled below)
			{}, // majorFreqTicks (empty = defaults)
			{}, // minorFreqTicks (empty = defaults)
			{}, // fftOverlays - will be populated below
			-60.0f, // fftDisplayMinDB - Professional noise floor
			0.0f,   // fftDisplayMaxDB - Digital maximum  
			true    // useIndependentFFTScale - Separate from EQ response scale
		};

		// Configure FFT overlays for input/output comparison
		cfg.fftOverlays = {
			{
				&mpLeveller->mInputSpectrumSmoother,  // Input signal (before EQ)
				ColorFromHTML("888888", 0.8f),        // Orange - Input signal
				ColorFromHTML("444444", 0.3f),        // Orange fill (more transparent)
				true,                                  // Enable fill
				"Input"                                // Label for legend
			},
			{
				&mpLeveller->mOutputSpectrumSmoother, // Output signal (after EQ)
				ColorFromHTML("007777", 0.8f),        // Green - Output signal  
				ColorFromHTML("005555", 0.3f),        // Green fill (more transparent)
				true,                                  // Enable fill
				"Output"                               // Label for legend
			}
		};


		//// FFT Analysis Controls Section
		//if (ImGui::CollapsingHeader("FFT Analysis Controls (Testing)", ImGuiTreeNodeFlags_DefaultOpen))
		//{
		//	ImGui::BeginGroup();

		//	// FFT Size selection
		//	//const char* fftSizeNames[] = { "512", "1024", "2048", "4096" };
		//	//if (ImGui::Combo("FFT Size", (int*)&mFFTSizeSelection, fftSizeNames, 4))
		//	//{
		//	//	// Reconstruct FFT with new size - not supported yet
		//	//}

		//	// Window Type selection
		//	//const char* windowTypeNames[] = { "Rectangular", "Hanning", "Hamming", "Blackman" };
		//	//if (ImGui::Combo("Window Function", (int*)&mFFTWindowType, windowTypeNames, 4))
		//	//{
		//	//	mpLeveller->mInputSpectrumSmoother.SetWindowType(mFFTWindowType);
		//	//	mpLeveller->mOutputSpectrumSmoother.SetWindowType(mFFTWindowType);
		//	//}
		//	//ImGui::SameLine(); ImGui::TextDisabled("(?)");
		//	//if (ImGui::IsItemHovered())
		//	//{
		//	//	ImGui::SetTooltip("Window function applied to FFT data:\n"
		//	//		"Rectangular: No windowing (sharp but prone to spectral leakage)\n"
		//	//		"Hanning: Good general purpose (smooth, low leakage)\n"
		//	//		"Hamming: Similar to Hanning with slightly different characteristics\n"
		//	//		"Blackman: Excellent side-lobe suppression (smoothest)\n"
		//	//	);
		//	//}

		//	// FFT Technical Smoothing
		//	if (ImGui::SliderFloat("FFT Smoothing", &mFFTSmoothingFactor, 0.0f, 0.9f, "%.2f"))
		//	{
		//		mpLeveller->mInputSpectrumSmoother.SetFFTSmoothing(mFFTSmoothingFactor);
		//		mpLeveller->mOutputSpectrumSmoother.SetFFTSmoothing(mFFTSmoothingFactor);
		//	}
		//	ImGui::SameLine(); ImGui::TextDisabled("(?)");
		//	if (ImGui::IsItemHovered())
		//	{
		//		ImGui::SetTooltip("Technical smoothing applied to raw FFT data.\n0.0 = No smoothing\n0.9 = Heavy smoothing\nApplies to both input and output analyzers.");
		//	}

		//	// Professional FFT Y-Axis Scaling (Independent from EQ response)
		//	ImGui::Text("FFT Display Scale (Independent from EQ):");

		//	bool fftScaleChanged = false;

		//	if (ImGui::SliderFloat("FFT Min dB", &mFFTDisplayMinDB, -180.0f, -20.0f, "%.0f dB"))
		//	{
		//		fftScaleChanged = true;
		//	}

		//	if (ImGui::SliderFloat("FFT Max dB", &mFFTDisplayMaxDB, -10.0f, +10.0f, "%.0f dB"))
		//	{
		//		fftScaleChanged = true;
		//	}

		//	// FFT Overlap Factor - Extended options for testing (including 1x)
		//	const char* overlapNames[] = { "1x (No overlap)", "2x (50% overlap)", "4x (75% overlap)", "8x (87.5% overlap)", "16x (93.75% overlap)" };
		//	int overlapIndex = (mFFTOverlapFactor == 1) ? 0 : (mFFTOverlapFactor == 2) ? 1 : (mFFTOverlapFactor == 4) ? 2 : (mFFTOverlapFactor == 8) ? 3 : 4;
		//	if (ImGui::Combo("Overlap Factor", &overlapIndex, overlapNames, 5))
		//	{
		//		mFFTOverlapFactor = (overlapIndex == 0) ? 1 : (overlapIndex == 1) ? 2 : (overlapIndex == 2) ? 4 : (overlapIndex == 3) ? 8 : 16;
		//		mpLeveller->mInputSpectrumSmoother.SetOverlapFactor(mFFTOverlapFactor);
		//		mpLeveller->mOutputSpectrumSmoother.SetOverlapFactor(mFFTOverlapFactor);
		//		mpLeveller->mInputSpectrumSmoother.SetFFTUpdateRate(mpLeveller->mInputSpectrumSmoother.GetFFTSizeInt(), mFFTOverlapFactor);
		//		mpLeveller->mOutputSpectrumSmoother.SetFFTUpdateRate(mpLeveller->mOutputSpectrumSmoother.GetFFTSizeInt(), mFFTOverlapFactor);
		//	}
		//	ImGui::SameLine(); ImGui::TextDisabled("(?)");
		//	if (ImGui::IsItemHovered())
		//	{
		//		ImGui::SetTooltip("Higher overlap = smoother spectrum updates but more CPU usage.\n"
		//			"1x: No overlap (choppiest, lowest CPU)\n"
		//			"2x: Basic (most efficient for smooth updates)\n"
		//			"4x: Recommended for music\n"
		//			"8x: Very smooth for mastering\n"
		//			"16x: Ultra-smooth for analysis (CPU intensive)");
		//	}

		//	ImGui::Separator();
		//	ImGui::Text("Spectrum Display Smoothing:");

		//	// Peak Hold Time
		//	if (ImGui::SliderFloat("Peak Hold Time", &mSpectrumPeakHoldTime, 0.0f, 1000.0f, "%.0f ms"))
		//	{
		//		mpLeveller->mInputSpectrumSmoother.SetPeakHoldTime(mSpectrumPeakHoldTime);
		//		mpLeveller->mOutputSpectrumSmoother.SetPeakHoldTime(mSpectrumPeakHoldTime);
		//	}
		//	ImGui::SameLine(); ImGui::TextDisabled("(?)");
		//	if (ImGui::IsItemHovered())
		//	{
		//		ImGui::SetTooltip("How long peaks are held before starting to fall.\n0ms = No peak hold (immediate falloff)\n300ms = Pro-Q3 style behavior\nApplies to both input and output spectrums.");
		//	}

		//	// Falloff Rate
		//	if (ImGui::SliderFloat("Falloff Time", &mSpectrumFalloffTime, 50.0f, 5000.0f, "%.0f ms"))
		//	{
		//		mpLeveller->mInputSpectrumSmoother.SetFalloffRate(mSpectrumFalloffTime);
		//		mpLeveller->mOutputSpectrumSmoother.SetFalloffRate(mSpectrumFalloffTime);
		//	}
		//	ImGui::SameLine(); ImGui::TextDisabled("(?)");
		//	if (ImGui::IsItemHovered())
		//	{
		//		ImGui::SetTooltip("Time for spectrum to fall -60dB after peak hold expires.\nLower = faster response, Higher = smoother display\nApplies to both input and output spectrums.");
		//	}

		//	ImGui::EndGroup();
		//}

		for (size_t i = 0; i < (size_t)Leveller::ParamIndices::NumParams; ++i) {
			cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
		}

		// Apply current FFT scaling settings to renderer config
		cfg.fftDisplayMinDB = mFFTDisplayMinDB;
		cfg.fftDisplayMaxDB = mFFTDisplayMaxDB;

		mResponseGraph.OnRender(cfg);
	}

};

#endif
