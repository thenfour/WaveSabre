#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7WidthVst.h"

struct Maj7WidthEditor : public VstEditor
{
	Maj7Width* mpMaj7Width;
	Maj7WidthVst* mpMaj7WidthVst;

	Maj7WidthEditor(AudioEffect* audioEffect) : //
		VstEditor(audioEffect, 690, 460),
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
		ImGui::SameLine(); ImGui::Text("Final output panning & gain");

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
		RenderStereoImagingDisplay("stereo_imaging", mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis, {150, 300});

	}

private:
	void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& inputAnalysis, const StereoImagingAnalysisStream& outputAnalysis, ImVec2 size) {
		ImGui::BeginGroup();
		
		// Phase correlation meter
		RenderCorrelationMeter("correlation", inputAnalysis.mPhaseCorrelation, outputAnalysis.mPhaseCorrelation, {size.x, 40});
		
		// Phase scope (Lissajous display)
		RenderPhaseScope("inp_phase_scope", inputAnalysis, { 150, 150 });
		RenderPhaseScope("outp_phase_scope", outputAnalysis, { 150, 150 });

		ImGui::EndGroup();
	}
	
	void RenderCorrelationMeter(const char* id, double inputCorrelation, double outputCorrelation, ImVec2 size) {
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
		ImU32 inputColor = inputCorrelation > 0 ? IM_COL32(100, 255, 100, 200) : IM_COL32(255, 100, 100, 200);
		dl->AddRectFilled({correlationToX(0.0), inputY}, {correlationToX(inputCorrelation), inputY + barHeight}, inputColor);
		
		// Output correlation (bottom bar)
		ImU32 outputColor = outputCorrelation > 0 ? IM_COL32(100, 255, 100, 255) : IM_COL32(255, 100, 100, 255);
		dl->AddRectFilled({correlationToX(0.0), outputY}, {correlationToX(outputCorrelation), outputY + barHeight}, outputColor);
		
		// Labels
		dl->AddText({bb.Min.x + 2, bb.Min.y + 2}, IM_COL32(255, 255, 255, 150), "Correlation");
		
		char corrText[64];
		sprintf_s(corrText, "In: %.2f Out: %.2f", static_cast<float>(inputCorrelation), static_cast<float>(outputCorrelation));
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
};

