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
		VstEditor(audioEffect, 850, 750),
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

		// Stereo imaging visualization
		ImGui::SameLine(); 
		RenderStereoImagingDisplay("stereo_imaging_inp", mpMaj7Width->mInputImagingAnalysis);
		ImGui::SameLine(); RenderStereoImagingDisplay("stereo_imaging_outp", mpMaj7Width->mOutputImagingAnalysis);

	}

private:
	static void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& analysis) {
		ImGuiGroupScope _scope(id);

		static constexpr int dim = 250;

		RenderCorrelationMeter("correlation_in", analysis.mPhaseCorrelation, { dim, 30 });

		RenderStereoBalance("stereo_balance_in", analysis, { dim, 30 });

		// Add tabs to switch between different visualization modes
		if (ImGui::BeginTabBar("StereoVizTabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Goniometer")) {
				RenderGoniometer("inp_gonio", analysis, { dim, dim });
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Polar L")) {
				RenderPolarL("inp_polar_l", analysis, { dim, dim });
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	
};

