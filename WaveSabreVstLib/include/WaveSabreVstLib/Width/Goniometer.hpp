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

#include "WidthBase.hpp"

// Polar/circular goniometer - better for spotting mono compatibility issues
inline void RenderGoniometer(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
	auto* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImRect bb(pos, pos + size);

	// Background
	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
	dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

	ImVec2 center = { bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f };
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
		ImVec2 lineEnd = { center.x + cos(rad) * radius, center.y + sin(rad) * radius };
		ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
		dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
	}

	// Add reference labels for key angles
	dl->AddText({ center.x - 15, bb.Min.y + 2 }, IM_COL32(100, 255, 100, 150), "M");          // 0° (top)
	dl->AddText({ bb.Max.x - 15, center.y - 8 }, IM_COL32(150, 150, 255, 150), "R");             // 90° (right)  
	dl->AddText({ center.x - 20, bb.Max.y - 16 }, IM_COL32(255, 100, 100, 150), "S");        // 180° (bottom)
	dl->AddText({ bb.Min.x + 2, center.y - 8 }, IM_COL32(150, 150, 255, 150), "L");              // 270° (left)

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
			ImVec2 prevPos = { center.x + cos(prevAngle) * prevRadius, center.y - sin(prevAngle) * prevRadius };
			ImVec2 currPos = { center.x + cos(currAngle) * currRadius, center.y - sin(currAngle) * currRadius };

			// Color based on age and phase correlation
			float ageFactor = static_cast<float>(i) / static_cast<float>(historySize);
			float correlation = static_cast<float>(analysis.mPhaseCorrelation);

			// Red for out-of-phase, green for in-phase, yellow for uncorrelated
			ImU32 baseColor = GetCorrellationColor(correlation, 1);

			// Apply age-based alpha
			int alpha = static_cast<int>((1.0f - ageFactor) * 180 + 50);
			ImU32 color = (baseColor & 0x00FFFFFF) | (alpha << 24);

			dl->AddLine(prevPos, currPos, color, 1.5f);
		}
	}

	// Labels and problem indicators
	//dl->AddText({ bb.Min.x + 2, bb.Min.y + 2 }, IM_COL32(255, 255, 255, 150), "Goniometer");

	// Phase correlation warning
	//if (analysis.mPhaseCorrelation < -0.3) {
	//	dl->AddText({ bb.Min.x + 2, bb.Min.y + 18 }, IM_COL32(255, 100, 100, 200), "Decorrellation");
	//}

	// Analysis info
	//char goniText[128];
	//sprintf_s(goniText, "Corr: %.2f", static_cast<float>(analysis.mPhaseCorrelation));
	//ImVec2 textSize = ImGui::CalcTextSize(goniText);
	//dl->AddText({ bb.Max.x - textSize.x - 2, bb.Max.y - 14 }, IM_COL32(255, 255, 255, 150), goniText);

	ImGui::Dummy(size);
}

