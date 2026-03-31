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
#include <WaveSabreVstLib/AnalysisMocks.hpp>

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


struct StereoFieldOverlayStyle {
	float backgroundPieRadiusRatio = 0.90f;
	float sectorOuterRadiusRatio = 0.83f;
	float sectorInnerRadiusRatio = 0.48f;
	float backgroundPieAlpha = 0.07f;
	float sectorFillAlpha = 0.18f;
	float sectorOutlineAlpha = 0.78f;
	float spokeAlpha = 0.65f;
	float sectorOutlineThickness = 2.0f;
	float spokeThickness = 1.5f;
	float maxBalanceAngleDegrees = 60.0f;
	float minWidthSpanDegrees = 10.0f;
	float maxWidthSpanDegrees = 150.0f;
	float maxDisplayedWidth = 1.25f;
	float fadeStartDb = -72.0f;
	float fadeEndDb = -48.0f;
	int arcSegments = 48;
	bool showBackgroundPie = true;
	bool showBalanceSpoke = true;
};

inline const StereoFieldOverlayStyle& GetDefaultStereoFieldOverlayStyle() {
	static const StereoFieldOverlayStyle style{};
	return style;
}

inline float StereoFieldOverlayRemapClamped(float value, float inMin, float inMax, float outMin, float outMax) {
	if (fabsf(inMax - inMin) < 0.0001f) return outMin;
	float t = (value - inMin) / (inMax - inMin);
	t = std::clamp(t, 0.0f, 1.0f);
	return outMin + (outMax - outMin) * t;
}

inline float StereoFieldOverlayLinearToDb(float value) {
	return 20.0f * log10f(std::max(value, 1.0e-8f));
}

inline ImVec2 StereoFieldOverlayPolarPoint(ImVec2 center, float angleRadians, float radius) {
	return {
		center.x + sinf(angleRadians) * radius,
		center.y - cosf(angleRadians) * radius,
	};
}

inline std::vector<ImVec2> BuildStereoFieldSectorPoints(ImVec2 center,
										 float startAngle,
										 float endAngle,
										 float innerRadius,
										 float outerRadius,
										 int arcSegments) {
	const int segmentCount = std::max(arcSegments, 8);
	std::vector<ImVec2> points;
	points.reserve(static_cast<size_t>(segmentCount + 1) * 2);

	for (int i = 0; i <= segmentCount; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(segmentCount);
		float angle = startAngle + (endAngle - startAngle) * t;
		points.push_back(StereoFieldOverlayPolarPoint(center, angle, outerRadius));
	}

	for (int i = segmentCount; i >= 0; --i) {
		float t = static_cast<float>(i) / static_cast<float>(segmentCount);
		float angle = startAngle + (endAngle - startAngle) * t;
		points.push_back(StereoFieldOverlayPolarPoint(center, angle, innerRadius));
	}

	return points;
}

inline std::vector<ImVec2> BuildStereoFieldPiePoints(ImVec2 center,
									 float startAngle,
									 float endAngle,
									 float outerRadius,
									 int arcSegments) {
	const int segmentCount = std::max(arcSegments, 8);
	std::vector<ImVec2> points;
	points.reserve(static_cast<size_t>(segmentCount) + 3);
	points.push_back(center);

	for (int i = 0; i <= segmentCount; ++i) {
		float t = static_cast<float>(i) / static_cast<float>(segmentCount);
		float angle = startAngle + (endAngle - startAngle) * t;
		points.push_back(StereoFieldOverlayPolarPoint(center, angle, outerRadius));
	}

	return points;
}

inline void RenderStereoFieldOverlay(const char* id,
								 const StereoImagingAnalysisStream& analysis,
								 ImVec2 size,
								 ImVec2 center,
								 float radius,
								 const StereoFieldOverlayStyle& style = GetDefaultStereoFieldOverlayStyle()) {
	(void)id;
	(void)size;
	auto* dl = ImGui::GetWindowDrawList();

	const float signalLevel = std::max(static_cast<float>(analysis.mLeftLevel), static_cast<float>(analysis.mRightLevel));
	const float signalDb = StereoFieldOverlayLinearToDb(signalLevel);
	const float visibility = StereoFieldOverlayRemapClamped(signalDb, style.fadeStartDb, style.fadeEndDb, 0.0f, 1.0f);
	if (visibility <= 0.001f) return;

	const float correlation = std::clamp(static_cast<float>(analysis.mPhaseCorrelation), -1.0f, 1.0f);
	const float balance = std::clamp(static_cast<float>(analysis.mStereoBalance), -1.0f, 1.0f);
	const float width = std::max(0.0f, static_cast<float>(analysis.mStereoWidth));

	const float radiansPerDegree = 3.14159265f / 180.0f;
	const float maxBalanceAngle = style.maxBalanceAngleDegrees * radiansPerDegree;
	const float centerAngle = StereoFieldOverlayRemapClamped(balance, -1.0f, 1.0f, -maxBalanceAngle, maxBalanceAngle);
	const float halfSpan = StereoFieldOverlayRemapClamped(width,
		0.0f,
		style.maxDisplayedWidth,
		style.minWidthSpanDegrees * radiansPerDegree * 0.5f,
		style.maxWidthSpanDegrees * radiansPerDegree * 0.5f);
	const float startAngle = centerAngle - halfSpan;
	const float endAngle = centerAngle + halfSpan;

	const float backgroundOuterRadius = radius * style.backgroundPieRadiusRatio;
	const float sectorOuterRadius = radius * style.sectorOuterRadiusRatio;
	const float sectorInnerRadius = radius * style.sectorInnerRadiusRatio;
	const float emphasis = 0.45f + 0.55f * fabsf(correlation);

	const ImU32 backgroundColor = GetCorrellationColor(correlation, style.backgroundPieAlpha * visibility);
	const ImU32 fillColor = GetCorrellationColor(correlation, style.sectorFillAlpha * emphasis * visibility);
	const ImU32 outlineColor = GetCorrellationColor(correlation, style.sectorOutlineAlpha * emphasis * visibility);
	const ImU32 spokeColor = GetCorrellationColor(correlation, style.spokeAlpha * visibility);

	if (style.showBackgroundPie) {
		auto piePoints = BuildStereoFieldPiePoints(center, startAngle, endAngle, backgroundOuterRadius, style.arcSegments);
		if (piePoints.size() >= 3) {
			dl->AddConvexPolyFilled(piePoints.data(), static_cast<int>(piePoints.size()), backgroundColor);
		}
	}

	auto sectorPoints = BuildStereoFieldSectorPoints(center, startAngle, endAngle, sectorInnerRadius, sectorOuterRadius, style.arcSegments);
	if (sectorPoints.size() >= 3) {
		dl->AddConvexPolyFilled(sectorPoints.data(), static_cast<int>(sectorPoints.size()), fillColor);
		dl->AddPolyline(sectorPoints.data(), static_cast<int>(sectorPoints.size()), outlineColor, ImDrawFlags_Closed, style.sectorOutlineThickness);
	}

	if (style.showBalanceSpoke) {
		const ImVec2 spokeEnd = StereoFieldOverlayPolarPoint(center, centerAngle, backgroundOuterRadius);
		dl->AddLine(center, spokeEnd, spokeColor, style.spokeThickness);
	}
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




// Compatibility wrapper while call sites still use the old name.
inline void RenderPhaseCorrelationOverlay(const char* id,
									   const StereoImagingAnalysisStream& analysis,
									   ImVec2 size,
									   ImVec2 center,
									   float radius,
									   const StereoFieldOverlayStyle& style = GetDefaultStereoFieldOverlayStyle()) {
	RenderStereoFieldOverlay(id, analysis, size, center, radius, style);
}