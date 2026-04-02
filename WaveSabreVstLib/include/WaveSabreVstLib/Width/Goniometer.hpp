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

#include "WidthBase.hpp"

	// Goniometer layer renderer (based on RenderGoniometer but without background/grid)
inline void RenderGoniometerLayer(const char* id,
		const StereoImagingAnalysisStream& analysis,
		ImVec2 size,
		ImVec2 center,
		float radius,
		bool renderAsPoints,
		const StereoImagingColorScheme& colorScheme = GetDefaultStereoImagingColorScheme()) {
	auto* dl = ImGui::GetWindowDrawList();

	constexpr bool kNormalizeToCircle = true;

	const auto* history = analysis.GetHistoryBuffer();
	size_t historySize = analysis.GetHistorySize();

	if (historySize > 1) {
		for (size_t i = 1; i < historySize; ++i) {
			const auto& prev = history[(i - 1) % historySize];
			const auto& curr = history[i % historySize];

			float prevMid = (prev.left + prev.right) * 0.5f;
			float prevSide = (prev.left - prev.right) * 0.5f;
			float currMid = (curr.left + curr.right) * 0.5f;
			float currSide = (curr.left - curr.right) * 0.5f;

			float prevAngle = atan2(prevSide, prevMid) - 3.14159f * 0.5f;
			float currAngle = atan2(currSide, currMid) - 3.14159f * 0.5f;

			float prevRadiusNorm, currRadiusNorm;
			if (kNormalizeToCircle) {
				prevRadiusNorm = std::min(std::abs(prevMid) + std::abs(prevSide), 1.0f);
				currRadiusNorm = std::min(std::abs(currMid) + std::abs(currSide), 1.0f);
			}
			else {
				float prevMag = std::sqrt(prev.left * prev.left + prev.right * prev.right);
				float currMag = std::sqrt(curr.left * curr.left + curr.right * curr.right);
				static constexpr float gMaxLevel = 1.0f;
				prevRadiusNorm = std::min(prevMag / gMaxLevel, 1.0f);
				currRadiusNorm = std::min(currMag / gMaxLevel, 1.0f);
			}

			float prevRadiusPx = prevRadiusNorm * radius;
			float currRadiusPx = currRadiusNorm * radius;

			ImVec2 prevPos = { center.x + cos(prevAngle) * prevRadiusPx, center.y - sin(prevAngle) * prevRadiusPx };
			ImVec2 currPos = { center.x + cos(currAngle) * currRadiusPx, center.y - sin(currAngle) * currRadiusPx };

			float ageFactor = static_cast<float>(i) / static_cast<float>(historySize);
			float correlation = static_cast<float>(analysis.mPhaseCorrelation);
			ImU32 baseColor = GetCorrellationColor(correlation, 1, colorScheme);
			int alpha = static_cast<int>((1.0f - ageFactor) * 180 + 50);
			ImU32 color = (baseColor & 0x00FFFFFF) | (alpha << 24);

			if (renderAsPoints) {
				//dl->AddCircleFilled(prevPos, 1.5f, color);
				dl->AddCircleFilled(currPos, 1.f, color,2);
			}
			else {
				// Draw line between previous and current position

				dl->AddLine(prevPos, currPos, color, 1.5f);
			}
		}
	}
}

