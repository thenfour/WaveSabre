#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

//#include "Maj7WidthVst.h"
#include <map>
#include <array>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>
#include <variant>

#include "WidthBase.hpp"

// usage:
// specify initializer list from low to high values.
// colors are specified as html strings ("22bbCC") -- converted to ImU32 internally via ColorFromHTML.
// first value covers low range, then specified boundaries and colors, last color covers high range.
// examples:
// 
// VU meter with 3 regions:
// ColorRegionMap map = {
//   "008800", // everything below 0.8 gets green
//   0.8, "FFFF00", // 0.8 to 1.0 gets yellow
//   1.0, "FF0000" // 1.0 and above gets red
// };
struct ColorRegionMap {
	struct Region {
		double threshold;
		ImU32 color;
	};
	
	std::vector<Region> regions;
	ImU32 defaultColor; // For values below the first threshold

	// Constructor that accepts initializer list
	ColorRegionMap(std::initializer_list<std::variant<const char*, double>> init) {
		auto it = init.begin();
		
		// First element should be the default color (for low values)
		if (it != init.end() && std::holds_alternative<const char*>(*it)) {
			defaultColor = ColorFromHTML(std::get<const char*>(*it));
			++it;
		} else {
			defaultColor = IM_COL32(128, 128, 128, 255); // Gray fallback
		}
		
		// Process pairs of (threshold, color)
		while (it != init.end()) {
			if (std::holds_alternative<double>(*it)) {
				double threshold = std::get<double>(*it);
				++it;
				
				if (it != init.end() && std::holds_alternative<const char*>(*it)) {
					const char* colorStr = std::get<const char*>(*it);
					regions.push_back({threshold, ColorFromHTML(colorStr)});
					++it;
				}
			} else {
				++it; // Skip unexpected types
			}
		}
		
		// Sort regions by threshold to ensure proper lookup
		std::sort(regions.begin(), regions.end(), 
			[](const Region& a, const Region& b) { return a.threshold < b.threshold; });
	}

	ImU32 GetColor(double value) const {
		// Check if value is below all thresholds
		if (regions.empty() || value < regions[0].threshold) {
			return defaultColor;
		}
		
		// Find the appropriate region by looking for the highest threshold that value meets or exceeds
		for (int i = static_cast<int>(regions.size()) - 1; i >= 0; --i) {
			if (value >= regions[i].threshold) {
				return regions[i].color;
			}
		}
		
		// Fallback (should not be reached due to the first check)
		return defaultColor;
	}
};




inline void RenderCorrelationMeter(const char* id, double correlation, ImVec2 size) {
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
	dl->AddLine({ correlationToX(-1.0), bb.Min.y }, { correlationToX(-1.0), bb.Max.y }, IM_COL32(255, 100, 100, 150), 1.0f); // -1 (out of phase)
	dl->AddLine({ correlationToX(0.0), bb.Min.y }, { correlationToX(0.0), bb.Max.y }, IM_COL32(255, 255, 100, 150), 1.0f); // 0 (uncorrelated)
	dl->AddLine({ correlationToX(1.0), bb.Min.y }, { correlationToX(1.0), bb.Max.y }, IM_COL32(100, 255, 100, 150), 1.0f); // +1 (mono)

	// Draw correlation bars
	float barHeight = size.y * 0.4f;
	float inputY = bb.Min.y + size.y * 0.2f;
	float outputY = bb.Min.y + size.y * 0.6f;

	// Input correlation (top bar)
	ImU32 inputColor = correlation > 0 ? IM_COL32(100, 255, 100, 200) : IM_COL32(255, 100, 100, 200);
	dl->AddRectFilled({ correlationToX(0.0), inputY }, { correlationToX(correlation), inputY + barHeight }, inputColor);

	dl->AddText({ bb.Min.x + 2, bb.Min.y + 2 }, IM_COL32(255, 255, 255, 150), "Correlation");

	char corrText[64];
	sprintf_s(corrText, "%.2f", static_cast<float>(correlation));
	dl->AddText({ bb.Min.x + 2, bb.Max.y - 14 }, IM_COL32(255, 255, 255, 200), corrText);

	ImGui::Dummy(size);
}



inline void RenderGeneralMeter(double value, double minValue, double maxValue, ImVec2 size, const char *label, double centerValue, const ColorRegionMap& colorMap) {
	auto* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImRect bb(pos, pos + size);

	// Background
	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 40, 40, 255));
	dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

	// Clamp value and center to range
	double clampedValue = std::max(minValue, std::min(maxValue, value));
	double clampedCenter = std::max(minValue, std::min(maxValue, centerValue));
	double range = maxValue - minValue;
	
	// Scale from minValue to maxValue
	auto valueToX = [&](double val) -> float {
		if (range <= 0.0001) return bb.Min.x; // Avoid division by zero
		return bb.Min.x + static_cast<float>((val - minValue) / range * size.x);
	};

	// Draw scale markers - always show min, center, and max
	//ImU32 minColor = IM_COL32(255, 100, 100, 150);  // Red for minimum
	//ImU32 centerColor = IM_COL32(255, 255, 100, 150);  // Yellow for center
	//ImU32 maxColor = IM_COL32(100, 255, 100, 150);  // Green for maximum
	//
	//dl->AddLine({ valueToX(minValue), bb.Min.y }, { valueToX(minValue), bb.Max.y }, minColor, 1.0f);
	//dl->AddLine({ valueToX(clampedCenter), bb.Min.y }, { valueToX(clampedCenter), bb.Max.y }, centerColor, 1.0f);
	//dl->AddLine({ valueToX(maxValue), bb.Min.y }, { valueToX(maxValue), bb.Max.y }, maxColor, 1.0f);

	// Draw value bar
	float barHeight = size.y * 0.4f;
	float barY = bb.Min.y + size.y * 0.3f;
	
	// Determine bar color
	ImU32 barColor;
	//if (colorMap) {
		// Use the provided color map
		barColor = colorMap.GetColor(clampedValue);
	//} else if (range <= 0.0001) {
	//	barColor = IM_COL32(128, 128, 128, 200); // Gray for invalid range
	//} else {
	//	// Default color logic based on value position within range
	//	double normalizedValue = (clampedValue - minValue) / range;
	//	if (normalizedValue < 0.33) {
	//		// Blend from red to yellow
	//		float t = static_cast<float>(normalizedValue / 0.33);
	//		barColor = IM_COL32(255, static_cast<int>(100 + 155 * t), 100, 200);
	//	} else if (normalizedValue < 0.67) {
	//		// Blend from yellow to green
	//		float t = static_cast<float>((normalizedValue - 0.33) / 0.34);
	//		barColor = IM_COL32(static_cast<int>(255 - 155 * t), 255, 100, 200);
	//	} else {
	//		// Stay green
	//		barColor = IM_COL32(100, 255, 100, 200);
	//	}
	//}
	
	// Draw the bar from center to current value
	float centerX = valueToX(clampedCenter);
	float currentX = valueToX(clampedValue);
	
	// Ensure proper rectangle bounds (left to right)
	float leftX = std::min(centerX, currentX);
	float rightX = std::max(centerX, currentX);
	
	dl->AddRectFilled({ leftX, barY }, { rightX, barY + barHeight }, barColor);

	// Draw label
	dl->AddText({ bb.Min.x + 2, bb.Min.y + 2 }, IM_COL32(255, 255, 255, 150), label);

	// Draw value text
	char valueText[64];
	sprintf_s(valueText, "%.3f", static_cast<float>(clampedValue));
	dl->AddText({ bb.Min.x + 2, bb.Max.y - 14 }, IM_COL32(255, 255, 255, 200), valueText);

	ImGui::Dummy(size);
}




// Custom phase correlation "X" overlay renderer
inline void RenderPhaseCorrelationOverlay(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size, ImVec2 center, float radius) {
	auto* dl = ImGui::GetWindowDrawList();

	float correlation = static_cast<float>(analysis.mPhaseCorrelation);
	ImU32 phaseLineColor = GetCorrellationColor(correlation, 0.8f);

	// Calculate correlation angle - same logic as in PolarL
	float correlationAngle = acosf(std::max(-1.0f, std::min(1.0f, correlation * 0.5f + 0.5f)));
	correlationAngle -= 3.14159f * 0.5f;

	// Calculate line extent 
	float lineLength = radius * 0.9f;

	// First diagonal of the X
	ImVec2 lineStart1 = {
		center.x - cosf(correlationAngle) * lineLength,
		center.y + sinf(correlationAngle) * lineLength
	};
	ImVec2 lineEnd1 = {
		center.x + cosf(correlationAngle) * lineLength,
		center.y - sinf(correlationAngle) * lineLength
	};

	// Second diagonal of the X (perpendicular)
	ImVec2 lineStart2 = {
		center.x + cosf(correlationAngle) * lineLength,
		center.y + sinf(correlationAngle) * lineLength
	};
	ImVec2 lineEnd2 = {
		center.x - cosf(correlationAngle) * lineLength,
		center.y - sinf(correlationAngle) * lineLength
	};

	// Draw the X-shaped phase correlation lines
	dl->AddLine(lineStart1, lineEnd1, phaseLineColor, 2.5f);
	dl->AddLine(lineStart2, lineEnd2, phaseLineColor, 2.5f);
}