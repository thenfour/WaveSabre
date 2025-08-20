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
		ImGui::SameLine(); VUMeter("vu_outp", mpMaj7Width->mOutputAnalysis[0], mpMaj7Width->mOutputAnalysis[1], mainCfg);

		ImGui::SameLine(); VUMeter("ms_inp", mpMaj7Width->mInputImagingAnalysis.mMidLevelDetector, mpMaj7Width->mInputImagingAnalysis.mSideLevelDetector, mainCfg);
		ImGui::SameLine(); VUMeter("ms_outp", mpMaj7Width->mOutputImagingAnalysis.mMidLevelDetector, mpMaj7Width->mOutputImagingAnalysis.mSideLevelDetector, mainCfg);

		// Stereo imaging visualization
		ImGui::SameLine(); 
		RenderStereoImagingDisplay("stereo_imaging_inp", mpMaj7Width->mInputImagingAnalysis);
		ImGui::SameLine(); RenderStereoImagingDisplay("stereo_imaging_outp", mpMaj7Width->mOutputImagingAnalysis);

	}

private:
	bool mShowGoniometer = false;
	bool mShowPolarL = true;
	bool mShowPhaseX = true;


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

		// Toggle buttons for layer visibility
		ImGui::BeginGroup();
		{
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2);
			
			// Create toggle buttons with appropriate colors
			if (mShowGoniometer) {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 120, 60, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 140, 80, 200));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 60, 60, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 200));
			}
			if (ImGui::Button("Gonio", ImVec2(50, 20))) {
				mShowGoniometer = !mShowGoniometer;
			}
			ImGui::PopStyleColor(2);
			
			ImGui::SameLine();
			if (mShowPolarL) {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(120, 60, 120, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(140, 80, 140, 200));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 60, 60, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 200));
			}
			if (ImGui::Button("Poly", ImVec2(50, 20))) {
				mShowPolarL = !mShowPolarL;
			}
			ImGui::PopStyleColor(2);
			
			ImGui::SameLine();
			if (mShowPhaseX) {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(120, 120, 60, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(140, 140, 80, 200));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(60, 60, 60, 200));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(80, 80, 80, 200));
			}
			if (ImGui::Button("Scissor", ImVec2(60, 20))) {
				mShowPhaseX = !mShowPhaseX;
			}
			ImGui::PopStyleColor(2);
			
			ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
		}
		ImGui::EndGroup();

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
		dl->AddText({ center.x - 15, bb.Min.y + 2 }, IM_COL32(100, 255, 100, 150), "M");      // Mono
		dl->AddText({ bb.Max.x - 15, center.y - 8 }, IM_COL32(150, 150, 255, 150), "R");     // Right
		dl->AddText({ center.x - 20, bb.Max.y - 16 }, IM_COL32(255, 100, 100, 150), "S");    // Side/Phase
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

