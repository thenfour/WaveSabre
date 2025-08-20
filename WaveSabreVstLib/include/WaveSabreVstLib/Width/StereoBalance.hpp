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


// Stereo balance and width analysis over time
inline void RenderStereoBalance(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
	auto* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImRect bb(pos, pos + size);

	// Background
	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(30, 30, 30, 255));
	dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

	// Center line for balance reference
	ImVec2 centerLine = { bb.Min.x + size.x * 0.5f, bb.Min.y };
	ImVec2 centerLineEnd = { bb.Min.x + size.x * 0.5f, bb.Max.y };
	dl->AddLine(centerLine, centerLineEnd, IM_COL32(120, 120, 120, 150), 1.0f);

	// *** SIMPLIFIED: Use properly smoothed balance from analysis stream ***
	float smoothedBalance = static_cast<float>(analysis.mStereoBalance);
	
	// Map balance to screen position (-1 to +1 -> 0 to width)
	float balanceX = bb.Min.x + (smoothedBalance + 1.0f) * 0.5f * size.x;
	
	// Draw balance indicator with professional color coding
	ImU32 balanceColor;
	float absBalance = std::abs(smoothedBalance);
	if (absBalance < 0.1f) {
		balanceColor = IM_COL32(100, 255, 100, 200); // Green for centered
	} else if (absBalance < 0.3f) {
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
	sprintf_s(balanceText, "Balance: %.2f", smoothedBalance);
	dl->AddText({bb.Min.x + 4, bb.Min.y + 4}, IM_COL32(255, 255, 255, 150), balanceText);

	// Stereo width indicator (also from analysis stream)
	float smoothedWidth = static_cast<float>(analysis.mStereoWidth);
	
	ImU32 widthColor;
	if (smoothedWidth < 0.5f) {
		widthColor = IM_COL32(255, 255, 100, 150); // Yellow for narrow
	} else if (smoothedWidth > 2.0f) {
		widthColor = IM_COL32(255, 100, 100, 150); // Red for too wide
	} else {
		widthColor = IM_COL32(100, 255, 100, 150); // Green for good
	}
	
	char widthText[64];
	sprintf_s(widthText, "Width: %.2f", smoothedWidth);
	ImVec2 widthTextSize = ImGui::CalcTextSize(widthText);
	dl->AddText({bb.Max.x - widthTextSize.x - 4, bb.Min.y + 4}, widthColor, widthText);
	
	// Labels
	dl->AddText({bb.Min.x + 4, bb.Max.y - 18}, IM_COL32(150, 150, 150, 150), "L");
	dl->AddText({bb.Max.x - 12, bb.Max.y - 18}, IM_COL32(150, 150, 150, 150), "R");
	
	ImGui::Dummy(size);
}
