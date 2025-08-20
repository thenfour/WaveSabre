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


inline void RenderGeneralMeter(double value, double minValue, double maxValue, ImVec2 size, const char *label) {
	auto* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImRect bb(pos, pos + size);

	// Background
	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(40, 40, 40, 255));
	dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

	// Clamp value to range
	double clampedValue = std::max(minValue, std::min(maxValue, value));
	double range = maxValue - minValue;
	
	// Scale from minValue to maxValue
	auto valueToX = [&](double val) -> float {
		if (range <= 0.0001) return bb.Min.x; // Avoid division by zero
		return bb.Min.x + static_cast<float>((val - minValue) / range * size.x);
	};

	// Determine scale marker positions and colors based on the range
	ImU32 minColor = IM_COL32(255, 100, 100, 150);  // Red for minimum
	ImU32 midColor = IM_COL32(255, 255, 100, 150);  // Yellow for middle
	ImU32 maxColor = IM_COL32(100, 255, 100, 150);  // Green for maximum
	
	// Draw scale markers
	dl->AddLine({ valueToX(minValue), bb.Min.y }, { valueToX(minValue), bb.Max.y }, minColor, 1.0f);
	
	// Only draw middle line if range is significant and not centered at zero
	if (range > 0.0001) {
		double midValue = minValue + range * 0.5;
		dl->AddLine({ valueToX(midValue), bb.Min.y }, { valueToX(midValue), bb.Max.y }, midColor, 1.0f);
	}
	
	dl->AddLine({ valueToX(maxValue), bb.Min.y }, { valueToX(maxValue), bb.Max.y }, maxColor, 1.0f);

	// Draw value bar
	float barHeight = size.y * 0.4f;
	float barY = bb.Min.y + size.y * 0.3f;
	
	// Determine bar color based on value position within range
	ImU32 barColor;
	if (range <= 0.0001) {
		barColor = IM_COL32(128, 128, 128, 200); // Gray for invalid range
	} else {
		double normalizedValue = (clampedValue - minValue) / range;
		if (normalizedValue < 0.33) {
			// Blend from red to yellow
			float t = static_cast<float>(normalizedValue / 0.33);
			barColor = IM_COL32(255, static_cast<int>(100 + 155 * t), 100, 200);
		} else if (normalizedValue < 0.67) {
			// Blend from yellow to green
			float t = static_cast<float>((normalizedValue - 0.33) / 0.34);
			barColor = IM_COL32(static_cast<int>(255 - 155 * t), 255, 100, 200);
		} else {
			// Stay green
			barColor = IM_COL32(100, 255, 100, 200);
		}
	}
	
	// Draw the bar from minimum to current value
	float minX = valueToX(minValue);
	float currentX = valueToX(clampedValue);
	dl->AddRectFilled({ minX, barY }, { currentX, barY + barHeight }, barColor);

	// Draw label
	dl->AddText({ bb.Min.x + 2, bb.Min.y + 2 }, IM_COL32(255, 255, 255, 150), label);

	// Draw value text
	char valueText[64];
	sprintf_s(valueText, "%.3f", static_cast<float>(clampedValue));
	dl->AddText({ bb.Min.x + 2, bb.Max.y - 14 }, IM_COL32(255, 255, 255, 200), valueText);

	ImGui::Dummy(size);
}

