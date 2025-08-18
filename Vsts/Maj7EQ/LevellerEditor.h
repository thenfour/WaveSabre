#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "LevellerVst.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class LevellerEditor : public VstEditor
{
	using FFTSize = MonoFFTAnalysis::FFTSize;
	using WindowType = MonoFFTAnalysis::WindowType;
	Leveller* mpLeveller;
	LevellerVst* mpLevellerVST;

	ColorMod mEnabledColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	FrequencyResponseRenderer<680,200,100, Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams, true> mResponseGraph;

	// FFT Analysis controls (not VST parameters, just UI state)
	float mFFTSmoothingFactor;// = 0.0f;      // 0.0-0.9
	int mFFTOverlapFactor;// = 2;             // 2, 4, 8, or 16 (more options for testing)
	float mSpectrumPeakHoldTime;// = 300.0f;    // 0-1000 ms
	float mSpectrumFalloffTime;//= 2000.0f;   // 50-5000 ms
	FFTSize mFFTSizeSelection;// = 1;             // 0=512, 1=1024, 2=2048, 3=4096
	WindowType mFFTWindowType;// = 1;                // 0=Rectangular, 1=Hanning, 2=Hamming, 3=Blackman
	
	// Professional FFT scaling controls (separate from EQ response)
	float mFFTDisplayMinDB = -80.0f;       // Noise floor
	float mFFTDisplayMaxDB = 10.0f;         // Digital maximum  
	float mFFTCurveSmoothing = 0.3f;        // Bézier curve smoothing factor (0.0 = sharp, 0.5 = very smooth)

public:
	LevellerEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 700, 800),
		mpLevellerVST((LevellerVst*)audioEffect)//,
	{
		mpLeveller = (Leveller*)mpLevellerVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
		auto& fft = mpLeveller->mInputFFTAnalysis;
		auto& smoother = mpLeveller->mInputSpectrumSmoother;

		// Initialize FFT control values to match current Leveller settings
		mFFTSmoothingFactor = fft.GetSmoothingFactor();// 0.0f;      // Matches Leveller constructor
		mFFTOverlapFactor = fft.GetOverlapFactor();// 2;           // Matches Leveller constructor
		mSpectrumPeakHoldTime = smoother.GetPeakHoldTime();// 0.0f;    // Matches Leveller constructor  
		mSpectrumFalloffTime = smoother.GetFalloffTime();   // Matches Leveller constructor
		mFFTSizeSelection = fft.GetFFTSize();// 1;           // 1024 FFT size used in Leveller constructor
		mFFTWindowType = fft.GetWindowType();              // Hanning window used in Leveller constructor
		//mFFTDisplayMinDB = -60.0f;       // Professional noise floor
		//mFFTDisplayMaxDB = 0.0f;         // Digital maximum
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
		ImGui::Dummy(gSize);

		ImGui::SameLine(); WSImGuiParamCheckbox((int)paramOffset + (int)Leveller::BandParamOffsets::Enable, "Enable?");

		float gainParam = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Gain);

		M7::QuickParam fp{ M7::gFilterFreqConfig };
		//float f1 = 0, f2 = 0;
		//M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };// :gFilterCenterFrequency, M7::gFilterFrequencyScale};
		fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);

		ImGui::SameLine();  Maj7ImGuiParamFrequency((int)paramOffset + (int)Leveller::BandParamOffsets::Freq, -1, "Freq", M7::gFilterFreqConfig, fp.GetRawValue(), {});

		float typeB = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Type);
		M7::ParamAccessor typePA{ &typeB, 0 };
		BiquadFilterType type = typePA.GetEnumValue< BiquadFilterType>(0);
		ImGui::BeginDisabled(type == BiquadFilterType::Highpass || type == BiquadFilterType::Lowpass);
		ImGui::SameLine(); Maj7ImGuiParamVolume((int)paramOffset + (int)Leveller::BandParamOffsets::Gain, "Gain", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
		ImGui::EndDisabled();
		//WSImGuiParamKnob((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q");
		//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q", 0.2f, 0);
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


		//ImGui::EndTabItem();
	//}

	//EndTabBarWithColoredSeparator();
//}
		ImGui::PopID();
	}


	virtual void renderImgui() override
	{
		mEnabledColors.EnsureInitialized();
		mDisabledColors.EnsureInitialized();

		ImGui::BeginGroup();

		Maj7ImGuiParamVolume((VstInt32)Leveller::ParamIndices::OutputVolume, "Output volume", WaveSabreCore::M7::gVolumeCfg12db, 0, {});

		ImGui::SameLine();

		//WSImGuiParamCheckbox((VstInt32)Leveller::ParamIndices::EnableDCFilter, "Enable DC Filter?");
		Maj7ImGuiBoolParamToggleButton(Leveller::ParamIndices::EnableDCFilter, "DC Filter");

		using HtmlColorString = char[7];
		const HtmlColorString bandColors[Leveller::gBandCount] = {
			"cc44cc",
			"44cc44",
			"cc4444",
			"4444ff",
			"ff8822",
		};

		RenderBand(0, "band1", Leveller::ParamIndices::Band1Type, 90, bandColors[0]);
		RenderBand(1, "band2", Leveller::ParamIndices::Band2Type, 250, bandColors[1]);
		RenderBand(2, "band3", Leveller::ParamIndices::Band3Type, 1100, bandColors[2]);
		RenderBand(3, "band4", Leveller::ParamIndices::Band4Type, 3000, bandColors[3]);
		RenderBand(4, "band5", Leveller::ParamIndices::Band5Type, 8500, bandColors[4]);

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter("vu_inp", mpLeveller->mInputAnalysis[0], mpLeveller->mInputAnalysis[1]);
		ImGui::SameLine(); VUMeter("vu_outp", mpLeveller->mOutputAnalysis[0], mpLeveller->mOutputAnalysis[1]);

		//ImGui::EndTable();
		//
		const std::array<FrequencyResponseRendererFilter, Leveller::gBandCount> filters = {
			FrequencyResponseRendererFilter{bandColors[0], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band1Enable) > 0.5f) ? &mpLeveller->mBands[0].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[1], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band2Enable) > 0.5f) ? &mpLeveller->mBands[1].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[2], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band3Enable) > 0.5f) ? &mpLeveller->mBands[2].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[3], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band4Enable) > 0.5f) ? &mpLeveller->mBands[3].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[4], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band5Enable) > 0.5f) ? &mpLeveller->mBands[4].mFilters[0] : nullptr},
		};

		FrequencyResponseRendererConfig<Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams> cfg{
			ColorFromHTML("222222", 1.0f), // background
			ColorFromHTML("aaaaaa", 1.0f), // line
			6.0f, // thumbRadius
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
				false,                                 // Use left channel
				true,                                  // Enable fill
				"Input"                                // Label for legend
			},
			{
				&mpLeveller->mOutputSpectrumSmoother, // Output signal (after EQ)
				ColorFromHTML("007777", 0.8f),        // Green - Output signal  
				ColorFromHTML("005555", 0.3f),        // Green fill (more transparent)
				false,                                 // Use left channel
				true,                                  // Enable fill
				"Output"                               // Label for legend
			}
		};


		// FFT Analysis Controls Section
		if (ImGui::CollapsingHeader("FFT Analysis Controls (Testing)", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::BeginGroup();

			// FFT Size selection
			const char* fftSizeNames[] = { "512", "1024", "2048", "4096" };
			if (ImGui::Combo("FFT Size", (int*)&mFFTSizeSelection, fftSizeNames, 4))
			{
				// Reconstruct FFT with new size - this is expensive but for testing it's fine

				// Note: Can't easily change FFT size at runtime without reconstruction
				// For now, just display current selection
			}

			// Window Type selection
			const char* windowTypeNames[] = { "Rectangular", "Hanning", "Hamming", "Blackman" };
			if (ImGui::Combo("Window Function", (int*)&mFFTWindowType, windowTypeNames, 4))
			{
				// Window type can now be changed at runtime!
				// Apply the window type change immediately!
				mpLeveller->mInputFFTAnalysis.SetWindowType(mFFTWindowType);
				mpLeveller->mOutputFFTAnalysis.SetWindowType(mFFTWindowType);
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Window function applied to FFT data:\n"
					"Rectangular: No windowing (sharp but prone to spectral leakage)\n"
					"Hanning: Good general purpose (smooth, low leakage)\n"
					"Hamming: Similar to Hanning with slightly different characteristics\n"
					"Blackman: Excellent side-lobe suppression (smoothest)\n"
				);
			}

			// FFT Technical Smoothing
			if (ImGui::SliderFloat("FFT Smoothing", &mFFTSmoothingFactor, 0.0f, 0.9f, "%.2f"))
			{
				mpLeveller->mInputFFTAnalysis.SetSmoothingFactor(mFFTSmoothingFactor);
				mpLeveller->mOutputFFTAnalysis.SetSmoothingFactor(mFFTSmoothingFactor);
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Technical smoothing applied to raw FFT data.\n0.0 = No smoothing\n0.9 = Heavy smoothing\nApplies to both input and output analyzers.");
			}

			// Professional FFT Y-Axis Scaling (Independent from EQ response)
			ImGui::Separator();
			ImGui::Text("FFT Display Scale (Independent from EQ):");

			bool fftScaleChanged = false;

			if (ImGui::SliderFloat("FFT Min dB", &mFFTDisplayMinDB, -90.0f, -20.0f, "%.0f dB"))
			{
				fftScaleChanged = true;
			}

			if (ImGui::SliderFloat("FFT Max dB", &mFFTDisplayMaxDB, -10.0f, +10.0f, "%.0f dB"))
			{
				fftScaleChanged = true;
			}

			// FFT Overlap Factor - Extended options for testing (including 1x)
			const char* overlapNames[] = { "1x (No overlap)", "2x (50% overlap)", "4x (75% overlap)", "8x (87.5% overlap)", "16x (93.75% overlap)" };
			int overlapIndex = (mFFTOverlapFactor == 1) ? 0 : (mFFTOverlapFactor == 2) ? 1 : (mFFTOverlapFactor == 4) ? 2 : (mFFTOverlapFactor == 8) ? 3 : 4;
			if (ImGui::Combo("Overlap Factor", &overlapIndex, overlapNames, 5))
			{
				mFFTOverlapFactor = (overlapIndex == 0) ? 1 : (overlapIndex == 1) ? 2 : (overlapIndex == 2) ? 4 : (overlapIndex == 3) ? 8 : 16;
				mpLeveller->mInputFFTAnalysis.SetOverlapFactor(mFFTOverlapFactor);
				mpLeveller->mOutputFFTAnalysis.SetOverlapFactor(mFFTOverlapFactor);

				// CRITICAL: Update spectrum smoother timing to match new overlap factor
				mpLeveller->mInputSpectrumSmoother.SetFFTUpdateRate(1024, mFFTOverlapFactor); // Assuming 1024 FFT
				mpLeveller->mOutputSpectrumSmoother.SetFFTUpdateRate(1024, mFFTOverlapFactor);
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Higher overlap = smoother spectrum updates but more CPU usage.\n"
					"1x: No overlap (choppiest, lowest CPU)\n"
					"2x: Basic (most efficient for smooth updates)\n"
					"4x: Recommended for music\n"
					"8x: Very smooth for mastering\n"
					"16x: Ultra-smooth for analysis (CPU intensive)");
			}

			ImGui::Separator();
			ImGui::Text("Spectrum Display Smoothing:");

			// Peak Hold Time
			if (ImGui::SliderFloat("Peak Hold Time", &mSpectrumPeakHoldTime, 0.0f, 1000.0f, "%.0f ms"))
			{
				mpLeveller->mInputSpectrumSmoother.SetPeakHoldTime(mSpectrumPeakHoldTime, Helpers::CurrentSampleRateF);
				mpLeveller->mOutputSpectrumSmoother.SetPeakHoldTime(mSpectrumPeakHoldTime, Helpers::CurrentSampleRateF);
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("How long peaks are held before starting to fall.\n0ms = No peak hold (immediate falloff)\n300ms = Pro-Q3 style behavior\nApplies to both input and output spectrums.");
			}

			// Falloff Rate
			if (ImGui::SliderFloat("Falloff Time", &mSpectrumFalloffTime, 50.0f, 5000.0f, "%.0f ms"))
			{
				mpLeveller->mInputSpectrumSmoother.SetFalloffRate(mSpectrumFalloffTime, Helpers::CurrentSampleRateF);
				mpLeveller->mOutputSpectrumSmoother.SetFalloffRate(mSpectrumFalloffTime, Helpers::CurrentSampleRateF);
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Time for spectrum to fall -60dB after peak hold expires.\nLower = faster response, Higher = smoother display\nApplies to both input and output spectrums.");
			}

			// Curve Smoothing Control (NEW!)
			ImGui::Separator();
			ImGui::Text("Visual Smoothing:");
			if (ImGui::SliderFloat("Curve Smoothing", &mFFTCurveSmoothing, 0.0f, 0.5f, "%.2f"))
			{
				// This affects rendering only, no need to update analyzers
			}
			ImGui::SameLine(); ImGui::TextDisabled("(?)");
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("Visual curve smoothing for prettier display:\n"
					"0.0 = Sharp lines (raw FFT bins)\n"
					"0.3 = Smooth curves (recommended)\n"
					"0.5 = Very smooth (may hide detail)");
			}


			ImGui::EndGroup();
		}




		for (size_t i = 0; i < (size_t)Leveller::ParamIndices::NumParams; ++i) {
			cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
		}


		// Apply current FFT scaling settings to renderer config
		cfg.fftDisplayMinDB = mFFTDisplayMinDB;
		cfg.fftDisplayMaxDB = mFFTDisplayMaxDB;
		cfg.fftCurveSmoothing = mFFTCurveSmoothing;

		mResponseGraph.OnRender(cfg);

		// ImGui::EndChild();
	}

};

#endif
