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

