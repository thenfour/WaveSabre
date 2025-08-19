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

struct Maj7WidthEditor : public VstEditor
{
	Maj7Width* mpMaj7Width;
	Maj7WidthVst* mpMaj7WidthVst;

	Maj7WidthEditor(AudioEffect* audioEffect) : //
		VstEditor(audioEffect, 800, 750),
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

		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LeftSource, "Left source", -1, -1, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RightSource, "Right source", 1, 1, 0, {});
		ImGui::EndGroup();

		//ImGui::SameLine(); ImGui::Text("Select where the left / right channels originate.\r\nThis can be used to swap L/R for example.");

		ImGui::BeginGroup();
		Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle, "Rotation", -M7::math::gPI, M7::math::gPI, 0, 0, 0, {});
		ImGui::EndGroup();

		//ImGui::SameLine(); ImGui::Text("Rotate is just another way to manipulate the image.\r\nIt may cause phase issues, or it may help center an imbalanced image,\r\ne.g. correcting weird mic placement.");

		ImGui::BeginGroup();
		Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency, -1, "Side HPF", M7::gFilterFreqConfig, 0, {});
		ImGui::EndGroup();
		//ImGui::SameLine(); ImGui::Text("This reduces width of low frequencies. Fixed at 6db/Oct slope.");

		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidSideBalance, "Mid-Side balance", 0.0f, 0, {});
		ImGui::EndGroup();
		//ImGui::SameLine(); ImGui::Text("Mix between mono -> normal -> wide");

		ImGui::BeginGroup();
		ImGui::Text("Final output panning");
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain, "Output", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
		ImGui::EndGroup();
		//ImGui::SameLine(); ImGui::Text("Final output panning & gain");

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
		RenderStereoImagingDisplay("stereo_imaging", mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis);

	}

private:
	void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& inputAnalysis, const StereoImagingAnalysisStream& outputAnalysis) {
		ImGui::BeginGroup();

		static constexpr int dim = 250;

		// Phase correlation meter
		RenderCorrelationMeter("correlation_in", inputAnalysis.mPhaseCorrelation, { dim, 30 });
		RenderCorrelationMeter("correlation_out", outputAnalysis.mPhaseCorrelation, { dim, 30 });

		RenderStereoBalance("stereo_balance_in", inputAnalysis, { dim, 30 });
		RenderStereoBalance("stereo_balance_out", outputAnalysis, { dim, 30 });

		// Add tabs to switch between different visualization modes
		if (ImGui::BeginTabBar("StereoVizTabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Diamond")) {
				RenderPhaseScope("inp_phase_scope", inputAnalysis, { dim, dim });
				RenderPhaseScope("outp_phase_scope", outputAnalysis, { dim, dim });
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Goniometer")) {
				RenderGoniometer("inp_gonio", inputAnalysis, { dim, dim });
				RenderGoniometer("outp_gonio", outputAnalysis, { dim, dim });
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Polar L")) {
				RenderPolarL("inp_polar_l", inputAnalysis, { dim, dim });
				RenderPolarL("outp_polar_l", outputAnalysis, { dim, dim });
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::EndGroup();
	}
	
	void RenderCorrelationMeter(const char* id, double correlation, ImVec2 size) {
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 40, 40, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		// Scale from -1 to +1
		auto correlationToX = [&](double corr) -> float {
			return bb.Min.x + static_cast<float>((corr + 1.0) * 0.5 * size.x);
		};
		
		// Draw scale markers
		dl->AddLine({correlationToX(-1.0), bb.Min.y}, {correlationToX(-1.0), bb.Max.y}, IM_COL32(255, 100, 100, 150), 1.0f); // -1 (out of phase)
		dl->AddLine({correlationToX(0.0), bb.Min.y}, {correlationToX(0.0), bb.Max.y}, IM_COL32(255, 255, 100, 150), 1.0f); // 0 (uncorrelated)
		dl->AddLine({correlationToX(1.0), bb.Min.y}, {correlationToX(1.0), bb.Max.y}, IM_COL32(100, 255, 100, 150), 1.0f); // +1 (mono)
		
		// Draw correlation bars
		float barHeight = size.y * 0.4f;
		float inputY = bb.Min.y + size.y * 0.2f;
		float outputY = bb.Min.y + size.y * 0.6f;
		
		// Input correlation (top bar)
		ImU32 inputColor = correlation > 0 ? IM_COL32(100, 255, 100, 200) : IM_COL32(255, 100, 100, 200);
		dl->AddRectFilled({correlationToX(0.0), inputY}, {correlationToX(correlation), inputY + barHeight}, inputColor);
		//
		//// Output correlation (bottom bar)
		//ImU32 outputColor = outputCorrelation > 0 ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
		//dl->AddRectFilled({correlationToX(0.0), outputY}, {correlationToX(outputCorrelation), outputY + barHeight}, outputColor);
		//
		// Labels
		dl->AddText({bb.Min.x + 2, bb.Min.y + 2}, IM_COL32(255, 255, 255, 150), "Correlation");
		
		char corrText[64];
		sprintf_s(corrText, "%.2f", static_cast<float>(correlation));
		dl->AddText({bb.Min.x + 2, bb.Max.y - 14}, IM_COL32(255, 255, 255, 200), corrText);
		
		ImGui::Dummy(size);
	}
	
	void RenderPhaseScope(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		// Center point
		ImVec2 center = {bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f};
		
		// Draw diamond-shaped reference frame instead of square
		float diamondRadius = size.x * 0.4f;
		
		// Diamond vertices (top, right, bottom, left)
		ImVec2 diamondTop = {center.x, center.y - diamondRadius};
		ImVec2 diamondRight = {center.x + diamondRadius, center.y};
		ImVec2 diamondBottom = {center.x, center.y + diamondRadius};
		ImVec2 diamondLeft = {center.x - diamondRadius, center.y};
		
		// Draw diamond outline
		dl->AddLine(diamondTop, diamondRight, IM_COL32(80, 80, 80, 255), 1.0f);
		dl->AddLine(diamondRight, diamondBottom, IM_COL32(80, 80, 80, 255), 1.0f);
		dl->AddLine(diamondBottom, diamondLeft, IM_COL32(80, 80, 80, 255), 1.0f);
		dl->AddLine(diamondLeft, diamondTop, IM_COL32(80, 80, 80, 255), 1.0f);
		
		// Center cross (lighter)
		dl->AddLine({center.x, center.y - 4}, {center.x, center.y + 4}, IM_COL32(120, 120, 120, 255), 1.0f);
		dl->AddLine({center.x - 4, center.y}, {center.x + 4, center.y}, IM_COL32(120, 120, 120, 255), 1.0f);
		
		// Draw phase scope (Lissajous curve) with diamond transformation
		const auto* history = analysis.GetHistoryBuffer();
		size_t historySize = analysis.GetHistorySize();
		
		if (historySize > 1) {
			static constexpr float gMaxLevel = 1.0f; // Clamp display range
			static constexpr float gScale = 0.35f; // Scale factor for diamond
			
			for (size_t i = 1; i < historySize; ++i) {
				const auto& prev = history[(i - 1) % historySize];
				const auto& curr = history[i % historySize];
				
				// Skip very quiet samples to avoid noise
				float prevMagnitude = std::sqrt(prev.left * prev.left + prev.right * prev.right);
				float currMagnitude = std::sqrt(curr.left * curr.left + curr.right * curr.right);
				if (prevMagnitude < 0.01f && currMagnitude < 0.01f) {
					continue;
				}
				
				// Normalize inputs
				float prevL = prev.left / gMaxLevel;
				float prevR = prev.right / gMaxLevel;
				float currL = curr.left / gMaxLevel;
				float currR = curr.right / gMaxLevel;
				
				// Convert to Mid-Side for diamond visualization
				// Mid = (L + R) -> Y axis (vertical): top = mono, bottom = out-of-phase
				// Side = (L - R) -> X axis (horizontal): left = left channel, right = right channel
				float prevMid = (prevL + prevR) * 0.5f;  // Sum component (mono information)
				float prevSide = (prevL - prevR) * 0.5f; // Difference component (stereo width)
				float currMid = (currL + currR) * 0.5f;
				float currSide = (currL - currR) * 0.5f;
				
				// Map to screen coordinates with diamond orientation
				// Side (L-R difference) = X axis, Mid (L+R sum) = Y axis (inverted for screen)
				float prevScreenX = center.x + prevSide * gScale * size.x;
				float prevScreenY = center.y - prevMid * gScale * size.y; // Invert Y for screen coordinates
				float currScreenX = center.x + currSide * gScale * size.x;
				float currScreenY = center.y - currMid * gScale * size.y; // Invert Y for screen coordinates
				
				// Clamp to display area
				prevScreenX = std::max(bb.Min.x, std::min(bb.Max.x, prevScreenX));
				prevScreenY = std::max(bb.Min.y, std::min(bb.Max.y, prevScreenY));
				currScreenX = std::max(bb.Min.x, std::min(bb.Max.x, currScreenX));
				currScreenY = std::max(bb.Min.y, std::min(bb.Max.y, currScreenY));
				
				// Fade older samples - newest samples are brightest
				float ageFactor = static_cast<float>(i) / static_cast<float>(historySize);
				ImU32 color = IM_COL32(100, 200, 255, static_cast<int>((1.0f - ageFactor) * 180 + 50));
				
				dl->AddLine({prevScreenX, prevScreenY}, {currScreenX, currScreenY}, color, 1.5f);
			}
		}
		
		// Draw reference patterns and labels for diamond interpretation
		if (ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
			// Show reference patterns
			dl->AddLine(diamondTop, diamondBottom, IM_COL32(255, 255, 0, 150), 1.0f); // Mono reference (vertical)
			dl->AddLine(diamondLeft, diamondRight, IM_COL32(255, 100, 100, 150), 1.0f); // Wide/phase reference (horizontal)
		}
		
		// Labels for diamond axes
		dl->AddText({center.x - 15, bb.Min.y + 2}, IM_COL32(255, 255, 255, 120), "MONO");
		dl->AddText({center.x - 25, bb.Max.y - 16}, IM_COL32(255, 255, 255, 120), "PHASE");
		dl->AddText({bb.Min.x + 2, center.y - 8}, IM_COL32(255, 255, 255, 120), "L");
		dl->AddText({bb.Max.x - 12, center.y - 8}, IM_COL32(255, 255, 255, 120), "R");
		
		// Main title and analysis info
		dl->AddText({bb.Min.x + 2, bb.Min.y + 2}, IM_COL32(255, 255, 255, 150), "Phase Scope");
		dl->AddText({bb.Min.x + 2, bb.Max.y - 42}, IM_COL32(255, 255, 255, 100), "Diamond View");
		dl->AddText({bb.Min.x + 2, bb.Max.y - 28}, IM_COL32(255, 255, 255, 80), "Shift: show ref");
		
		// Stereo width and correlation indicators
		char analysisText[128];
		sprintf_s(analysisText, "W:%.2f C:%.2f", 
			static_cast<float>(analysis.mStereoWidth), 
			static_cast<float>(analysis.mPhaseCorrelation));
		ImVec2 textSize = ImGui::CalcTextSize(analysisText);
		dl->AddText({bb.Max.x - textSize.x - 2, bb.Max.y - 14}, IM_COL32(255, 255, 255, 150), analysisText);
		
		ImGui::Dummy(size);
	}
	
	// Polar/circular goniometer - better for spotting mono compatibility issues
	void RenderGoniometer(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		ImVec2 center = {bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f};
		float radius = std::min(size.x, size.y) * 0.45f;
		
		// Draw concentric circles for magnitude reference
		for (int i = 1; i <= 4; ++i) {
			float r = radius * i * 0.25f;
			dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
		}
		
		// Draw angular reference lines and labels
		// 0° = Mono (top), 90° = Right only, 180° = Out-of-phase (bottom), 270° = Left only
		for (int angle = 0; angle < 360; angle += 30) {
			float rad = angle * 3.14159f / 180.0f;
			ImVec2 lineEnd = {center.x + cos(rad) * radius, center.y + sin(rad) * radius};
			ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
			dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
		}
		
		// Add reference labels for key angles
		dl->AddText({center.x - 15, bb.Min.y + 2}, IM_COL32(100, 255, 100, 150), "MONO");          // 0° (top)
		dl->AddText({bb.Max.x - 15, center.y - 8}, IM_COL32(150, 150, 255, 150), "R");             // 90° (right)  
		dl->AddText({center.x - 20, bb.Max.y - 16}, IM_COL32(255, 100, 100, 150), "PHASE");        // 180° (bottom)
		dl->AddText({bb.Min.x + 2, center.y - 8}, IM_COL32(150, 150, 255, 150), "L");              // 270° (left)
		
		// Draw phase relationships as polar plot with corrected orientation
		const auto* history = analysis.GetHistoryBuffer();
		size_t historySize = analysis.GetHistorySize();
		
		if (historySize > 1) {
			static constexpr float gMaxLevel = 1.0f;
			
			for (size_t i = 1; i < historySize; ++i) {
				const auto& prev = history[(i - 1) % historySize];
				const auto& curr = history[i % historySize];
				
				// Skip very quiet samples
				float prevMag = std::sqrt(prev.left * prev.left + prev.right * prev.right);
				float currMag = std::sqrt(curr.left * curr.left + curr.right * curr.right);
				if (prevMag < 0.01f && currMag < 0.01f) continue;
				
				// Convert to proper goniometer coordinates
				// Use mid-side encoding for more intuitive display:
				// - Mono content (L=R) appears at top (0°)
				// - Out-of-phase (L=-R) appears at bottom (180°)  
				// - Left-only appears at left (270°)
				// - Right-only appears at right (90°)
				
				float prevMid = (prev.left + prev.right) * 0.5f;   // Sum component  
				float prevSide = (prev.left - prev.right) * 0.5f;  // Difference component
				float currMid = (curr.left + curr.right) * 0.5f;   
				float currSide = (curr.left - curr.right) * 0.5f;  
				
				// Calculate angle from mid/side components
				// atan2 gives us: 0° = +X axis, 90° = +Y axis, etc.
				// We want: 0° = mono (top), 90° = right-only, 180° = out-of-phase (bottom), 270° = left-only
				float prevAngle = atan2(prevSide, prevMid) - 3.14159f * 0.5f; // Rotate -90° so mono appears at top
				float currAngle = atan2(currSide, currMid) - 3.14159f * 0.5f; // Rotate -90° so mono appears at top
				
				// Calculate radius from total magnitude
				float prevRadius = std::min(prevMag / gMaxLevel, 1.0f) * radius;
				float currRadius = std::min(currMag / gMaxLevel, 1.0f) * radius;
				
				// Map to screen coordinates (Y axis is inverted for screen)
				ImVec2 prevPos = {center.x + cos(prevAngle) * prevRadius, center.y - sin(prevAngle) * prevRadius};
				ImVec2 currPos = {center.x + cos(currAngle) * currRadius, center.y - sin(currAngle) * currRadius};
				
				// Color based on age and phase correlation
				float ageFactor = static_cast<float>(i) / static_cast<float>(historySize);
				float correlation = static_cast<float>(analysis.mPhaseCorrelation);
				
				// Red for out-of-phase, green for in-phase, yellow for uncorrelated
				ImU32 baseColor;
				if (correlation > 0.5f) {
					baseColor = IM_COL32(100, 255, 100, 255); // Green for good correlation
				} else if (correlation < -0.5f) {
					baseColor = IM_COL32(255, 100, 100, 255); // Red for bad correlation  
				} else {
					baseColor = IM_COL32(255, 255, 100, 255); // Yellow for neutral
				}
				
				// Apply age-based alpha
				int alpha = static_cast<int>((1.0f - ageFactor) * 180 + 50);
				ImU32 color = (baseColor & 0x00FFFFFF) | (alpha << 24);
				
				dl->AddLine(prevPos, currPos, color, 1.5f);
			}
		}
		
		// Labels and problem indicators
		dl->AddText({bb.Min.x + 2, bb.Min.y + 2}, IM_COL32(255, 255, 255, 150), "Goniometer");
		
		// Phase correlation warning
		if (analysis.mPhaseCorrelation < -0.3) {
			dl->AddText({bb.Min.x + 2, bb.Min.y + 18}, IM_COL32(255, 100, 100, 200), "PHASE ISSUE!");
		}
		
		// Analysis info
		char goniText[128];
		sprintf_s(goniText, "Corr: %.2f", static_cast<float>(analysis.mPhaseCorrelation));
		ImVec2 textSize = ImGui::CalcTextSize(goniText);
		dl->AddText({bb.Max.x - textSize.x - 2, bb.Max.y - 14}, IM_COL32(255, 255, 255, 150), goniText);
		
		ImGui::Dummy(size);
	}
	
	// "Polar L" - Ozone-style envelope goniometer with sector peak detection
	void RenderPolarL(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
		constexpr size_t kMaxSectorCount = 128; // Power of 2 for better smoothing
		static size_t kSectorCount = 96; // Power of 2 for better smoothing
		static float kSmoothnessK = 0.f; // 0.0 = original shape, 1.0 = perfect circle -- not very necessary because kernel size does a lot of smoothing already.
		static int kSmoothingKernelSize = 3; // Larger = smoother curves
		static float kVisualBoost = 1.0f; // Perceptual compensation boost


		// for debugging, controls to change sector count, smoothness K, kernel size...
		//ImGui::BeginGroup();
		//ImGui::Text("Polar L Settings");
		//ImGui::SliderInt("Sector Count", (int*)&kSectorCount, 3, kMaxSectorCount, "%d");
		//ImGui::SliderFloat("Smoothness K", &kSmoothnessK, 0.0f, 1.0f, "%.2f");
		//ImGui::SliderInt("Smoothing Kernel Size", &kSmoothingKernelSize, 1, 32, "%d");
		//ImGui::SliderFloat("Visual Boost", &kVisualBoost, 0.5f, 4.0f, "%.2f");
		//ImGui::EndGroup();


		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		ImVec2 center = {bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f};
		const float radius = std::min(size.x, size.y) * 0.45f;
		
		// Draw concentric circles for magnitude reference
		for (int i = 1; i <= 4; ++i) {
			float r = radius * i * 0.25f;
			dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
		}
		
		// Draw angular reference lines
		for (int angle = 0; angle < 360; angle += 30) {
			float rad = angle * 3.14159f / 180.0f;
			ImVec2 lineEnd = {center.x + cos(rad) * radius, center.y + sin(rad) * radius};
			ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
			dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
		}
		
		// Add reference labels for key angles
		dl->AddText({center.x - 15, bb.Min.y + 2}, IM_COL32(100, 255, 100, 150), "MONO");          // 0° (top)
		dl->AddText({bb.Max.x - 15, center.y - 8}, IM_COL32(150, 150, 255, 150), "R");             // 90° (right)  
		dl->AddText({center.x - 20, bb.Max.y - 16}, IM_COL32(255, 100, 100, 150), "PHASE");        // 180° (bottom)
		dl->AddText({bb.Min.x + 2, center.y - 8}, IM_COL32(150, 150, 255, 150), "L");              // 270° (left)
		
		// Static peak detection system for envelope - using unique static storage per instance
		static std::map<std::string, std::array<float, kMaxSectorCount>> sectorPeakRadiiMap;
		static std::map<std::string, std::array<float, kMaxSectorCount>> sectorHoldTimersMap;
		static std::map<std::string, double> lastUpdateTimeMap;
		
		std::string instanceId = std::string(id);
		constexpr float kHoldTimeMs = 200.0f;
		constexpr float kFalloffTimeMs = 2000.0f;
		
		auto& sectorPeakRadii = sectorPeakRadiiMap[instanceId];
		auto& sectorHoldTimers = sectorHoldTimersMap[instanceId];
		auto& lastUpdateTime = lastUpdateTimeMap[instanceId];
		
		// Get current time and calculate delta
		double currentTime = ImGui::GetTime();
		float deltaTimeMs = static_cast<float>((currentTime - lastUpdateTime) * 1000.0);
		lastUpdateTime = currentTime;
		
		// Clamp delta time to prevent huge jumps on first frame
		deltaTimeMs = std::min(deltaTimeMs, 50.0f);
		
		// Update peak detection system with current audio data
		const auto* history = analysis.GetHistoryBuffer();
		size_t historySize = analysis.GetHistorySize();
		
		if (historySize > 0) {
			static constexpr float gMaxLevel = 1.0f;
			
			// Process recent samples to update sector peaks
			size_t samplesToProcess = std::min(historySize, static_cast<size_t>(16));
			for (size_t i = historySize - samplesToProcess; i < historySize; ++i) {
				const auto& sample = history[i];
				
				float magnitude = std::sqrt(sample.left * sample.left + sample.right * sample.right);
				if (magnitude < 0.01f) continue;
				
				// Convert to mid-side for proper goniometer orientation
				float mid = (sample.left + sample.right) * 0.5f;   
				float side = (sample.left - sample.right) * 0.5f;  
				
				// Calculate angle and radius
				float angle = atan2(side, mid) - 3.14159f * 0.5f; // Rotate -90° so mono appears at top
				
				// *** PERCEPTUAL COMPENSATION BOOST ***
				// Apply scientifically-based scaling to compensate for processing losses:
				// 1. Square root for perceptual scaling (Stevens' Power Law for loudness ≈ amplitude^0.6)
				// 2. Additional boost for visual clarity 
				// 3. Compensate for smoothing losses
				float compensatedMagnitude = std::sqrt(magnitude) * kVisualBoost;  // sqrt ≈ power law + boost
				float currentRadius = std::min(compensatedMagnitude / gMaxLevel, 1.0f) * radius;
				
				// Normalize angle to 0-2π range
				while (angle < 0) angle += 2 * 3.14159f;
				while (angle >= 2 * 3.14159f) angle -= 2 * 3.14159f;
				
				// Determine which sector this sample belongs to
				size_t sectorIndex = static_cast<size_t>((angle / (2 * 3.14159f)) * kSectorCount) % kSectorCount;
				
				// Update peak for this sector with slight smoothing
				if (currentRadius > sectorPeakRadii[sectorIndex]) {
					sectorPeakRadii[sectorIndex] = currentRadius;
					sectorHoldTimers[sectorIndex] = kHoldTimeMs;
				}
			}
		}
		
		// Update peak hold and falloff for all sectors
		for (size_t i = 0; i < kSectorCount; ++i) {
			if (sectorHoldTimers[i] > 0.0f) {
				// In hold phase
				sectorHoldTimers[i] = std::max(0.0f, sectorHoldTimers[i] - deltaTimeMs);
			} else {
				// In falloff phase - exponential decay
				float falloffRate = 2.0f / kFalloffTimeMs; // Decay factor per ms
				sectorPeakRadii[i] *= std::exp(-falloffRate * deltaTimeMs);
				
				// Prevent underflow
				if (sectorPeakRadii[i] < 0.01f) {
					sectorPeakRadii[i] = 0.0f;
				}
			}
		}
		
		// === SMOOTHING ALGORITHM ===
		// Apply circular convolution smoothing to eliminate sharp polygon artifacts
		
		// Calculate average radius for circle fallback and mono signal detection
		float averageRadius = 0.0f;
		float maxRadius = 0.0f;
		for (size_t i = 0; i < kSectorCount; ++i) {
			averageRadius += sectorPeakRadii[i];
			maxRadius = std::max(maxRadius, sectorPeakRadii[i]);
		}
		averageRadius /= static_cast<float>(kSectorCount);
		
		// Create smoothed radius array using circular convolution
		std::array<float, kMaxSectorCount> smoothedRadii = {};
		
		for (size_t i = 0; i < kSectorCount; ++i) {
			float smoothedValue = 0.0f;
			float weightSum = 0.0f;
			
			// Apply smoothing kernel (Gaussian-like weighting)
			for (int k = -kSmoothingKernelSize; k <= kSmoothingKernelSize; ++k) {
				// Circular array indexing
				size_t sampleIndex = (i + k + kSectorCount) % kSectorCount;
				
				// Gaussian-like weight (closer samples have more influence)
				float weight = std::exp(-0.5f * (k * k) / (kSmoothingKernelSize * kSmoothingKernelSize * 0.25f));
				
				smoothedValue += sectorPeakRadii[sampleIndex] * weight;
				weightSum += weight;
			}
			
			// Normalize and blend with circular fallback
			float kernelSmoothed = smoothedValue / weightSum;
			float circularFallback = averageRadius; // Perfect circle
			
			// Apply smoothness blending: k=0 uses original kernel smoothing, k=1 uses circle
			smoothedRadii[i] = kernelSmoothed * (1.0f - kSmoothnessK) + circularFallback * kSmoothnessK;
			
			// Enhanced minimum radius for mono signals (prevents disappearing)
			// Use perceptually-scaled minimum that's more generous
			float enhancedMinimumRadius = std::max(maxRadius * 0.15f, radius * 0.05f); // Either 15% of peak OR 5% of total radius
			smoothedRadii[i] = std::max(smoothedRadii[i], enhancedMinimumRadius);
		}
		
		// Draw faint current trace for reference  
		if (historySize > 1) {
			for (size_t i = std::max(1ULL, historySize - 8); i < historySize; ++i) {
				const auto& prev = history[(i - 1) % historySize];
				const auto& curr = history[i % historySize];
				
				float prevMag = std::sqrt(prev.left * prev.left + prev.right * prev.right);
				float currMag = std::sqrt(curr.left * curr.left + curr.right * curr.right);
				if (prevMag < 0.01f && currMag < 0.01f) continue;
				
				float prevMid = (prev.left + prev.right) * 0.5f;   
				float prevSide = (prev.left - prev.right) * 0.5f;  
				float currMid = (curr.left + curr.right) * 0.5f;   
				float currSide = (curr.left - curr.right) * 0.5f;  
				
				float prevAngle = atan2(prevSide, prevMid) - 3.14159f * 0.5f;
				float currAngle = atan2(currSide, currMid) - 3.14159f * 0.5f;
				float prevRadius = std::min(std::sqrt(prevMag) * kVisualBoost, 1.0f) * radius;
				float currRadius = std::min(std::sqrt(currMag) * kVisualBoost, 1.0f) * radius;
				
				ImVec2 prevPos = {center.x + cos(prevAngle) * prevRadius, center.y - sin(prevAngle) * prevRadius};
				ImVec2 currPos = {center.x + cos(currAngle) * currRadius, center.y - sin(currAngle) * currRadius};
				
				// Very faint current trace
				ImU32 color = IM_COL32(150, 150, 150, 30);
				dl->AddLine(prevPos, currPos, color, 1.0f);
			}
		}
		
		// Build smooth envelope polygon from smoothed sector peaks
		std::vector<ImVec2> envelopePoints;
		envelopePoints.reserve(kSectorCount);
		
		for (size_t i = 0; i < kSectorCount; ++i) {
			float angle = (static_cast<float>(i) / static_cast<float>(kSectorCount)) * 2 * 3.14159f;
			float envelopeRadius = smoothedRadii[i];
			
			if (envelopeRadius > 0.01f) { // Always add points due to minimum radius
				ImVec2 point = {
					center.x + cos(angle) * envelopeRadius,
					center.y - sin(angle) * envelopeRadius
				};
				envelopePoints.push_back(point);
			}
		}
		
		// Draw the smooth envelope polygon
		if (envelopePoints.size() >= 3) {
			// Color based on phase correlation
			float correlation = static_cast<float>(analysis.mPhaseCorrelation);
			ImU32 envelopeFillColor, envelopeLineColor, phaseLineColor;
			
			if (correlation > 0.5f) {
				envelopeFillColor = ColorFromHTML("66FF66", 0.1f); // Light green fill
				envelopeLineColor = ColorFromHTML("66FF66", 0.3f); // Light green outline
				phaseLineColor = ColorFromHTML("66FF66", 0.8f); // Light green phase line
			} else if (correlation < -0.5f) {
				envelopeFillColor = ColorFromHTML("FF6666", 0.1f);
				envelopeLineColor = ColorFromHTML("FF6666", 0.3f);
				phaseLineColor = ColorFromHTML("FF6666", 0.8f);
			} else {
				envelopeFillColor = ColorFromHTML("ffff55", 0.1f); 
				envelopeLineColor = ColorFromHTML("ffff55", 0.3f);
				phaseLineColor = ColorFromHTML("ffff55", 0.8f); 
			}
			
			// Draw filled envelope
			dl->AddConvexPolyFilled(envelopePoints.data(), static_cast<int>(envelopePoints.size()), envelopeFillColor);
			
			// Draw smooth envelope outline
			for (size_t i = 0; i < envelopePoints.size(); i++) {
				size_t nextIdx = (i + 1) % envelopePoints.size();
				dl->AddLine(envelopePoints[i], envelopePoints[nextIdx], envelopeLineColor, 1.3f);
			}
			
			// *** ADD BI-POLAR CORRELATION LINE OVERLAY (X-SHAPED) ***
			float correlationAngle = acosf(std::max(-1.0f, std::min(1.0f, correlation*.5f + .5f))); // Map +1→0°, -1→90°
			correlationAngle -= 3.14159f * 0.5f; // Rotate -90° so +1.0 (mono) points up (0°)
			
			// Calculate the line extent (use max envelope radius for proper scaling)
			float lineLength = std::max(maxRadius, radius * 0.8f); // Use envelope extent or minimum 80% of display radius
			
			// *** ORIGINAL CORRELATION LINE ***
			ImVec2 lineStart = {
				center.x - cosf(correlationAngle) * lineLength,
				center.y + sinf(correlationAngle) * lineLength  
			};
			ImVec2 lineEnd = {
				center.x + cosf(correlationAngle) * lineLength,
				center.y - sinf(correlationAngle) * lineLength   
			};
			
			// *** REFLECTED CORRELATION LINE (mirror around Y-axis to complete the X) ***
			ImVec2 reflectedLineStart = {
				center.x + cosf(correlationAngle) * lineLength,  // Flip X coordinate
				center.y + sinf(correlationAngle) * lineLength   // Keep Y coordinate same
			};
			ImVec2 reflectedLineEnd = {
				center.x - cosf(correlationAngle) * lineLength,  // Flip X coordinate  
				center.y - sinf(correlationAngle) * lineLength   // Keep Y coordinate same
			};
			
			// Draw both lines to form complete X/cross
			dl->AddLine(lineStart, lineEnd, phaseLineColor, 3.0f);                    // Original line "/"
			dl->AddLine(reflectedLineStart, reflectedLineEnd, phaseLineColor, 3.0f);  // Reflected line "\"
			
			// Add center dot to show the correlation X origin
			//dl->AddCircleFilled(center, 3.0f, envelopeLineColor, 8);
		}
		
		
		// Labels and info
		dl->AddText({bb.Min.x + 2, bb.Min.y + 2}, IM_COL32(255, 255, 255, 150), "Polar L");
		dl->AddText({bb.Min.x + 2, bb.Min.y + 18}, IM_COL32(200, 200, 200, 120), "Enhanced");
		
		// Phase correlation warning
		if (analysis.mPhaseCorrelation < -0.3) {
			dl->AddText({bb.Min.x + 2, bb.Min.y + 34}, IM_COL32(255, 100, 100, 200), "PHASE ISSUE!");
		}
		
		// Analysis info
		char polarText[128];
		sprintf_s(polarText, "Corr: %.2f Boost: %.1fx", static_cast<float>(analysis.mPhaseCorrelation), kVisualBoost);
		ImVec2 textSize = ImGui::CalcTextSize(polarText);
		dl->AddText({bb.Max.x - textSize.x - 2, bb.Max.y - 14}, IM_COL32(255, 255, 255, 150), polarText);
		
		ImGui::Dummy(size);
	}
	
	// Stereo balance and width analysis over time
	void RenderStereoBalance(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
		auto* dl = ImGui::GetWindowDrawList();
		ImVec2 pos = ImGui::GetCursorScreenPos();
		ImRect bb(pos, pos + size);
		
		// Background
		dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(30, 30, 30, 255));
		dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
		
		// Center line for balance reference
		ImVec2 centerLine = {bb.Min.x + size.x * 0.5f, bb.Min.y};
		ImVec2 centerLineEnd = {bb.Min.x + size.x * 0.5f, bb.Max.y};
		dl->AddLine(centerLine, centerLineEnd, IM_COL32(120, 120, 120, 150), 1.0f);
		
		// Calculate instantaneous balance from recent samples
		const auto* history = analysis.GetHistoryBuffer();
		size_t historySize = analysis.GetHistorySize();
		
		if (historySize > 10) {
			// Average balance over recent samples for stability
			float balanceSum = 0.0f;
			int validSamples = 0;
			
			size_t startIdx = (historySize >= 32) ? historySize - 32 : 0;
			for (size_t i = startIdx; i < historySize; ++i) {
				const auto& sample = history[i];
				float magnitude = std::sqrt(sample.left * sample.left + sample.right * sample.right);
				
				if (magnitude > 0.01f) { // Skip quiet samples
					// Calculate balance: -1 = full left, 0 = center, +1 = full right
					float balance = (sample.right - sample.left) / magnitude;
					balanceSum += balance;
					validSamples++;
				}
			}
			
			if (validSamples > 0) {
				float avgBalance = balanceSum / validSamples;
				
				// Map balance to screen position (-1 to +1 -> 0 to width)
				float balanceX = bb.Min.x + (avgBalance + 1.0f) * 0.5f * size.x;
				
				// Draw balance indicator
				ImU32 balanceColor;
				if (abs(avgBalance) < 0.1f) {
					balanceColor = IM_COL32(100, 255, 100, 200); // Green for centered
				} else if (abs(avgBalance) < 0.3f) {
					balanceColor = IM_COL32(255, 255, 100, 200); // Yellow for slight imbalance
				} else {
					balanceColor = IM_COL32(255, 100, 100, 200); // Red for significant imbalance
				}
				
				// Balance indicator bar
				float barHeight = size.y * 0.6f;
				float barY = bb.Min.y + size.y * 0.2f;
				dl->AddRectFilled({balanceX - 2, barY}, {balanceX + 2, barY + barHeight}, balanceColor);
				
				// Balance value text
				char balanceText[64];
				sprintf_s(balanceText, "Balance: %.2f", avgBalance);
				dl->AddText({bb.Min.x + 4, bb.Min.y + 4}, IM_COL32(255, 255, 255, 150), balanceText);
			}
		}
		
		// Stereo width indicator
		float width = static_cast<float>(analysis.mStereoWidth);
		ImU32 widthColor;
		if (width < 0.5f) {
			widthColor = IM_COL32(255, 255, 100, 150); // Yellow for narrow
		} else if (width > 2.0f) {
			widthColor = IM_COL32(255, 100, 100, 150); // Red for too wide
		} else {
			widthColor = IM_COL32(100, 255, 100, 150); // Green for good
		}
		
		char widthText[64];
		sprintf_s(widthText, "Width: %.2f", width);
		ImVec2 widthTextSize = ImGui::CalcTextSize(widthText);
		dl->AddText({bb.Max.x - widthTextSize.x - 4, bb.Min.y + 4}, widthColor, widthText);
		
		// Labels
		dl->AddText({bb.Min.x + 4, bb.Max.y - 18}, IM_COL32(150, 150, 150, 150), "L");
		dl->AddText({bb.Max.x - 12, bb.Max.y - 18}, IM_COL32(150, 150, 150, 150), "R");
		
		ImGui::Dummy(size);
	}
};

