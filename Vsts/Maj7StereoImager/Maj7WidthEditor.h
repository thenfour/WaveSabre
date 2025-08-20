#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7WidthVst.h"
#include <map>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include <WaveSabreVstLib/Width/CorrelationMeter.hpp>
#include <WaveSabreVstLib/Width/StereoBalance.hpp>
#include <WaveSabreVstLib/Width/Goniometer.hpp>
#include <WaveSabreVstLib/Width/PolarL.hpp>
#include <WaveSabreVstLib/HistoryVisualization.hpp>

struct Maj7WidthEditor : public VstEditor
{
	Maj7Width* mpMaj7Width;
	Maj7WidthVst* mpMaj7WidthVst;

	Maj7WidthEditor(AudioEffect* audioEffect) : //
		VstEditor(audioEffect, 950, 700),
		mpMaj7WidthVst((Maj7WidthVst*)audioEffect)
	{
		mpMaj7Width = ((Maj7WidthVst *)audioEffect)->GetMaj7Width();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Width", mpMaj7Width, mpMaj7WidthVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Width->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::BeginGroup();

		// Left/Right Source Controls with Tooltip
		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LeftSource, "Left source", -1, -1, 0, {});
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Left Channel Source");
			ImGui::Separator();
			ImGui::TextWrapped("Select where the left channel originates from:");
			ImGui::BulletText("-1.0: Pure left input signal");
			ImGui::BulletText(" 0.0: Mono mix (L+R)/2");
			ImGui::BulletText("+1.0: Pure right input signal");
			ImGui::EndTooltip();
		}
		
		ImGui::SameLine(); 
		Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RightSource, "Right source", 1, 1, 0, {});
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Right Channel Source");
			ImGui::Separator();
			ImGui::TextWrapped("Select where the right channel originates from:");
			ImGui::BulletText("-1.0: Pure left input signal");
			ImGui::BulletText(" 0.0: Mono mix (L+R)/2");
			ImGui::BulletText("+1.0: Pure right input signal");
			ImGui::EndTooltip();
		}
		ImGui::EndGroup();

		// Rotation Control with Tooltip
		ImGui::BeginGroup();
		Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle, "Rotation", -M7::math::gPI, M7::math::gPI, 0, 0, 0, {});
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Stereo Field Rotation");
			ImGui::Separator();
			ImGui::TextWrapped("Rotate the stereo image around the center point.");
			ImGui::Spacing();
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.7f, 1.0f), "Use Cases:");
			ImGui::BulletText("Correct imbalanced recordings");
			ImGui::BulletText("Fix poor microphone placement");
			ImGui::BulletText("Creative stereo field manipulation");
			ImGui::EndTooltip();
		}
		ImGui::EndGroup();

		// Side HPF Control with Tooltip
		ImGui::BeginGroup();
		Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency, -1, "Side HPF", M7::gFilterFreqConfig, 0, {});
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Side Channel High-Pass Filter");
			ImGui::Separator();
			ImGui::TextWrapped("Reduces stereo width of low frequencies using a 6dB/octave slope.");
			ImGui::EndTooltip();
		}
		ImGui::EndGroup();

		// Mid-Side Balance Control with Tooltip
		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidSideBalance, "Mid-Side balance", 0.0f, 0, {});
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Mid-Side Balance Control");
			ImGui::Separator();
			ImGui::TextWrapped("Blend between mono (mid) and stereo (side) content:");
			ImGui::EndTooltip();
		}
		ImGui::EndGroup();

		// Final Output Section with Tooltips
		ImGui::BeginGroup();
		
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});
		
		ImGui::SameLine(); 
		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain, "Output", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
		ImGui::EndGroup();

		ImGui::EndGroup();

		static const std::vector<VUMeterTick> tickSet = {
				{-3.0f, "3db"},
				{-6.0f, "6db"},
				{-12.0f, "12db"},
				{-18.0f, "18db"},
				{-24.0f, "24db"},
				{-30.0f, "30db"},
				{-40.0f, "40db"},
				//{-50.0f, "50db"},
		};

		VUMeterConfig mainCfg = {
			{24, 300},
			VUMeterLevelMode::Audio,
			VUMeterUnits::Linear,
			-50, 6,
			tickSet,
		};

		VUMeterConfig attenCfg = mainCfg;
		attenCfg.levelMode = VUMeterLevelMode::Attenuation;

		ImGui::SameLine(); VUMeter("vu_inp", mpMaj7Width->mInputAnalysis[0], mpMaj7Width->mInputAnalysis[1], mainCfg);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Input stereo L R");
			ImGui::EndTooltip();
		}
		ImGui::SameLine(); VUMeter("vu_outp", mpMaj7Width->mOutputAnalysis[0], mpMaj7Width->mOutputAnalysis[1], mainCfg);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Output stereo L R");
			ImGui::EndTooltip();
		}

		ImGui::SameLine(); VUMeterMS("ms_inp", mpMaj7Width->mInputImagingAnalysis.mMidLevelDetector, mpMaj7Width->mInputImagingAnalysis.mSideLevelDetector, mainCfg);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Input mid side");
			ImGui::EndTooltip();
		}

		ImGui::SameLine(); VUMeterMS("ms_outp", mpMaj7Width->mOutputImagingAnalysis.mMidLevelDetector, mpMaj7Width->mOutputImagingAnalysis.mSideLevelDetector, mainCfg);
		if (ImGui::IsItemHovered()) {
			ImGui::BeginTooltip();
			ImGui::Text("Output mid side");
			ImGui::EndTooltip();
		}

		ImGui::SameLine();
		// Stereo imaging visualization
		{
			ImGuiGroupScope _grp("stereo_imaging");
			RenderStereoImagingDisplay("stereo_imaging_inp", mpMaj7Width->mInputImagingAnalysis);
			ImGui::SameLine(); RenderStereoImagingDisplay("stereo_imaging_outp", mpMaj7Width->mOutputImagingAnalysis);

			// Toggle buttons for layer visibility
			{
				// Create toggle buttons with appropriate colors
				ToggleButton(&mShowGoniometer, "Gonio");
				ImGui::SameLine(); ToggleButton(&mShowPolarL, "Poly");
				ImGui::SameLine(); ToggleButton(&mShowPhaseX, "Scissor");
			}
			{
				mStereoHistory.Render(true, mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis);
			}
		}

	}

	virtual std::vector<IVstSerializableParam*> GetVstOnlyParams() override
	{
		return {
			&mShowGoniometerParam,
			& mShowPolarLParam,
			& mShowPhaseXParam,
			& mShowPhaseCorrelationParam,
			& mShowStereoWidthParam,
			& mShowStereoBalanceParam,
			& mShowMidLevelParam,
			& mShowSideLevelParam,
			& mShowInputParam,
			& mShowOutputParam,
		};
	}


private:
	bool mShowGoniometer = false;
	bool mShowPolarL = true;
	bool mShowPhaseX = true;

	VstSerializableBoolParamRef mShowGoniometerParam{ "ShowGoniometer", mShowGoniometer };
	VstSerializableBoolParamRef mShowPolarLParam{ "ShowPolarL", mShowPolarL };
	VstSerializableBoolParamRef mShowPhaseXParam{ "ShowPhaseX", mShowPhaseX };
	VstSerializableBoolParamRef mShowPhaseCorrelationParam{ "ShowPhaseCorrelation", mStereoHistory.mShowPhaseCorrelation };
	VstSerializableBoolParamRef mShowStereoWidthParam{ "ShowStereoWidth", mStereoHistory.mShowStereoWidth };
	VstSerializableBoolParamRef mShowStereoBalanceParam{ "ShowStereoBalance", mStereoHistory.mShowStereoBalance };
	VstSerializableBoolParamRef mShowMidLevelParam{ "ShowMidLevel", mStereoHistory.mShowMidLevel };
	VstSerializableBoolParamRef mShowSideLevelParam{ "ShowSideLevel", mStereoHistory.mShowSideLevel };
	VstSerializableBoolParamRef mShowInputParam{ "ShowInput", mStereoHistory.mShowInput };
	VstSerializableBoolParamRef mShowOutputParam{ "ShowOutput", mStereoHistory.mShowOutput };

	// Stereo history visualization system
	template<int historyViewWidth, int historyViewHeight>
	struct StereoHistoryVis
	{
		// Parameter visibility toggles
		bool mShowPhaseCorrelation = true;
		bool mShowStereoWidth = true;
		bool mShowStereoBalance = true;
		bool mShowMidLevel = false;
		bool mShowSideLevel = false;
		bool mShowInput = true;
		bool mShowOutput = true;

		static constexpr float historyViewMinValue = -1.0f;  // For correlation (-1 to +1)
		static constexpr float historyViewMaxValue = +2.0f;  // For width (0 to 2+)
		
		// Expanded to 10 series: 5 parameters × 2 signals (input + output)
		HistoryView<13, historyViewWidth, historyViewHeight> mHistoryView;

		void Render(bool showToggles, const StereoImagingAnalysisStream& inputAnalysis, const StereoImagingAnalysisStream& outputAnalysis) 
		{
			ImGui::BeginGroup();
			static constexpr float lineWidth = 2.0f;

			mHistoryView.RenderCustom({
				// a unity line
				HistoryViewSeriesConfig{
					ColorFromHTML("ffffff", 0.2f),
					1, // line width
					-1.0f, +1.0f, true
				},
				HistoryViewSeriesConfig{
					ColorFromHTML("ffffff", 0.1f),
					1, // line width
					-1.0f, +1.0f, true
				},
				HistoryViewSeriesConfig{
					ColorFromHTML("ffffff", 0.1f),
					1, // line width
					-1.0f, +1.0f, true
				},
				// INPUT SIGNALS (brighter colors) with per-series scaling
				// Phase correlation: -1 to +1, bright cyan/blue  
				HistoryViewSeriesConfig{
					ColorFromHTML("00ccff", (mShowInput && mShowPhaseCorrelation) ? 0.9f : 0), 
					lineWidth, 
					-1.0f, +1.0f, true  // Custom scale for correlation
				},
				
				// Stereo width: 0 to 3, bright green/yellow
				HistoryViewSeriesConfig{
					ColorFromHTML("88ff44", (mShowInput && mShowStereoWidth) ? 0.8f : 0), 
					lineWidth,
					0.0f, 3.0f, true  // Custom scale for width (0 to 3 for better visibility)
				},
				
				// Stereo balance: -1 to +1, bright red/orange
				HistoryViewSeriesConfig{
					ColorFromHTML("ff8844", (mShowInput && mShowStereoBalance) ? 0.7f : 0), 
					lineWidth,
					-1.0f, +1.0f, true  // Custom scale for balance
				},
				
				// Mid level: 0 to 1, bright purple
				HistoryViewSeriesConfig{
					ColorFromHTML("bb88ff", (mShowInput && mShowMidLevel) ? 0.6f : 0), 
					lineWidth,
					0.0f, 1.0f, true  // Custom scale for RMS levels
				},
				
				// Side level: 0 to 1, bright orange
				HistoryViewSeriesConfig{
					ColorFromHTML("ffbb88", (mShowInput && mShowSideLevel) ? 0.6f : 0), 
					lineWidth,
					0.0f, 1.0f, true  // Custom scale for RMS levels
				},

				// OUTPUT SIGNALS (darker variants) with per-series scaling
				// Phase correlation: -1 to +1, darker cyan/blue
				HistoryViewSeriesConfig{
					ColorFromHTML("0099cc", (mShowOutput && mShowPhaseCorrelation) ? 0.8f : 0), 
					lineWidth,
					-1.0f, +1.0f, true  // Custom scale for correlation
				},
				
				// Stereo width: 0 to 3, darker green
				HistoryViewSeriesConfig{
					ColorFromHTML("66cc33", (mShowOutput && mShowStereoWidth) ? 0.7f : 0), 
					lineWidth,
					0.0f, 3.0f, true  // Custom scale for width
				},
				
				// Stereo balance: -1 to +1, darker red/orange
				HistoryViewSeriesConfig{
					ColorFromHTML("cc6633", (mShowOutput && mShowStereoBalance) ? 0.6f : 0), 
					lineWidth,
					-1.0f, +1.0f, true  // Custom scale for balance
				},
				
				// Mid level: 0 to 1, darker purple
				HistoryViewSeriesConfig{
					ColorFromHTML("9966cc", (mShowOutput && mShowMidLevel) ? 0.5f : 0), 
					lineWidth,
					0.0f, 1.0f, true  // Custom scale for RMS levels
				},
				
				// Side level: 0 to 1, darker orange
				HistoryViewSeriesConfig{
					ColorFromHTML("cc9966", (mShowOutput && mShowSideLevel) ? 0.5f : 0), 
					lineWidth,
					0.0f, 1.0f, true  // Custom scale for RMS levels
				},
			}, {
				0,
				-0.5f,
				+0.5f,
				// INPUT VALUES (first 5 series) - raw values, no normalization needed
				static_cast<float>(inputAnalysis.mPhaseCorrelation),                    // -1 to +1
				static_cast<float>(inputAnalysis.mStereoWidth),                         // 0 to 3+
				static_cast<float>(inputAnalysis.mStereoBalance),                       // -1 to +1  
				static_cast<float>(inputAnalysis.mMidLevelDetector.mCurrentRMSValue),   // 0 to 1
				static_cast<float>(inputAnalysis.mSideLevelDetector.mCurrentRMSValue),  // 0 to 1

				// OUTPUT VALUES (last 5 series) - raw values, no normalization needed
				static_cast<float>(outputAnalysis.mPhaseCorrelation),                   // -1 to +1
				static_cast<float>(outputAnalysis.mStereoWidth),                        // 0 to 3+
				static_cast<float>(outputAnalysis.mStereoBalance),                      // -1 to +1  
				static_cast<float>(outputAnalysis.mMidLevelDetector.mCurrentRMSValue),  // 0 to 1
				static_cast<float>(outputAnalysis.mSideLevelDetector.mCurrentRMSValue), // 0 to 1
			}, historyViewMinValue, historyViewMaxValue);

			if (showToggles) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				// Parameter toggles
				ToggleButton(&mShowPhaseCorrelation, "Phase");
				ImGui::SameLine(); ToggleButton(&mShowStereoWidth, "Width");
				ImGui::SameLine(); ToggleButton(&mShowStereoBalance, "Balance");
				
				ImGui::SameLine(0, 20); ToggleButton(&mShowMidLevel, "Mid");
				ImGui::SameLine(); ToggleButton(&mShowSideLevel, "Side");

				// Input/Output toggles (separate line, similar to CompressorVis L/R pattern)
				ImGui::SameLine(0, 40); ToggleButton(&mShowInput, "Input");
				ImGui::SameLine(); ToggleButton(&mShowOutput, "Output");

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
			}
			ImGui::EndGroup();
		}
	};

	StereoHistoryVis<508, 120> mStereoHistory;

	void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& analysis) {
		ImGuiGroupScope _scope(id);

		static constexpr int dim = 250;
		ImVec2 meterSize(dim, 30);

		RenderCorrelationMeter("correlation_in", analysis.mPhaseCorrelation, { dim, 30 });

		// double mStereoWidth = 0.0;       // 0 = mono, 1 = normal stereo, >1 = wide  
		// double mStereoBalance = 0.0;     // -1 to +1, -1 = full left, 0 = center, +1 = full right
		// double mMidLevel = 0.0;          // Mid channel level (RMS)
		// double mSideLevel = 0.0;         // Side channel level (RMS)

		RenderGeneralMeter(analysis.mStereoWidth, 0, 2, meterSize, "width", 0, {"008800", 0.5, "ffff00", 1.0, "ff0000"});
		RenderGeneralMeter(analysis.mStereoBalance, -1, 1, meterSize, "balance", 0, {
			"ff0000",
			-0.5, "ffff00",
			-0.2, "00ff00",
			+0.2, "ffff00",
			+0.5, "ff0000",
			});
		//RenderGeneralMeter(analysis.mMidLevel, 0, 1, meterSize, "mid level");
		//RenderGeneralMeter(analysis.mSideLevel, 0, 1, meterSize, "side level");

		//RenderStereoBalance("stereo_balance_in", analysis, { dim, 30 });

		// Layer visibility toggles - use static variables with unique IDs per instance

		//// Initialize defaults on first use
		//if (showGoniometerMap.find(instanceId) == showGoniometerMap.end()) {
		//	showGoniometer = true;  // Default to showing goniometer
		//	showPolarL = true;     // Default to hiding polar L
		//	showPhaseX = true;      // Default to showing phase correlation X
		//}

		// Render the layered visualization
		RenderLayeredStereoVisualization("stereovis", analysis, {dim, dim},
			mShowGoniometer, mShowPolarL, mShowPhaseX);
	}

	// Main layered visualization renderer
	static void RenderLayeredStereoVisualization(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size, bool showGoniometer, bool showPolarL, bool showPhaseX) {
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background and grid (always shown)
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		ImVec2 center = { bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f };
		const float radius = std::min(size.x, size.y) * 0.45f;
		
		// Draw concentric circles for magnitude reference
		for (int i = 1; i <= 4; ++i) {
			float r = radius * i * 0.25f;
			dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
		}
		
		// Draw angular reference lines
		for (int angle = 0; angle < 360; angle += 30) {
			float rad = angle * 3.14159f / 180.0f;
			ImVec2 lineEnd = { center.x + cos(rad) * radius, center.y + sin(rad) * radius };
			ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
			dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
		}
		
		// Add reference labels for key angles
		dl->AddText({ center.x - 4, bb.Min.y + 2 }, IM_COL32(100, 255, 100, 150), "M");      // Mono
		dl->AddText({ bb.Max.x - 15, center.y - 8 }, IM_COL32(150, 150, 255, 150), "R");     // Right
		dl->AddText({ center.x - 20, bb.Max.y - 16 }, IM_COL32(255, 100, 100, 150), "inv");    // Side/Phase
		dl->AddText({ bb.Min.x + 2, center.y - 8 }, IM_COL32(150, 150, 255, 150), "L");      // Left
		
		// Render layers in order (bottom to top)
		if (showPolarL) {
			RenderPolarLLayer(id, analysis, size, center, radius);
		}
		
		if (showGoniometer) {
			RenderGoniometerLayer(id, analysis, size, center, radius);
		}
		
		if (showPhaseX) {
			RenderPhaseCorrelationOverlay(id, analysis, size, center, radius);
		}
		
		ImGui::Dummy(size);
	}
	
};

