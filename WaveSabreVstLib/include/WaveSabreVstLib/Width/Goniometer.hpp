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
//
//inline void RenderGoniometer(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
//    auto* dl = ImGui::GetWindowDrawList();
//    ImVec2 pos = ImGui::GetCursorScreenPos();
//    ImRect bb(pos, pos + size);
//
//    // Background
//    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
//    dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));
//
//    ImVec2 center = { bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f };
//    float radius = std::min(size.x, size.y) * 0.45f;
//
//    // Draw concentric circles for magnitude reference
//    for (int i = 1; i <= 4; ++i) {
//        float r = radius * i * 0.25f;
//        dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
//    }
//
//    // Angular reference lines and labels
//    for (int angle = 0; angle < 360; angle += 30) {
//        float rad = angle * 3.14159f / 180.0f;
//        ImVec2 lineEnd = { center.x + cos(rad) * radius, center.y + sin(rad) * radius };
//        ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
//        dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
//    }
//
//    dl->AddText({ center.x - 15, bb.Min.y + 2 }, IM_COL32(100, 255, 100, 150), "M");
//    dl->AddText({ bb.Max.x - 15, center.y - 8 }, IM_COL32(150, 150, 255, 150), "R");
//    dl->AddText({ center.x - 20, bb.Max.y - 16 }, IM_COL32(255, 100, 100, 150), "S");
//    dl->AddText({ bb.Min.x + 2, center.y - 8 }, IM_COL32(150, 150, 255, 150), "L");
//
//    // Choose normalization:
//    // - false: legacy energy-based radius (diamond boundary)
//    // - true : L1-based circular normalization (fills the circle)
//    constexpr bool kNormalizeToCircle = true;
//
//    const auto* history = analysis.GetHistoryBuffer();
//    size_t historySize = analysis.GetHistorySize();
//
//    if (historySize > 1) {
//        for (size_t i = 1; i < historySize; ++i) {
//            const auto& prev = history[(i - 1) % historySize];
//            const auto& curr = history[i % historySize];
//
//            float prevMid = (prev.left + prev.right) * 0.5f;
//            float prevSide = (prev.left - prev.right) * 0.5f;
//            float currMid = (curr.left + curr.right) * 0.5f;
//            float currSide = (curr.left - curr.right) * 0.5f;
//
//            float prevAngle = atan2(prevSide, prevMid) - 3.14159f * 0.5f;
//            float currAngle = atan2(currSide, currMid) - 3.14159f * 0.5f;
//
//            float prevRadiusNorm, currRadiusNorm;
//            if (kNormalizeToCircle) {
//                // Circular normalization: boundary is |mid| + |side| = 1
//                prevRadiusNorm = std::min(std::abs(prevMid) + std::abs(prevSide), 1.0f);
//                currRadiusNorm = std::min(std::abs(currMid) + std::abs(currSide), 1.0f);
//            } else {
//                // Legacy energy normalization (diamond boundary)
//                float prevMag = std::sqrt(prev.left * prev.left + prev.right * prev.right);
//                float currMag = std::sqrt(curr.left * curr.left + curr.right * curr.right);
//                static constexpr float gMaxLevel = 1.0f;
//                prevRadiusNorm = std::min(prevMag / gMaxLevel, 1.0f);
//                currRadiusNorm = std::min(currMag / gMaxLevel, 1.0f);
//            }
//
//            float prevRadiusPx = prevRadiusNorm * radius;
//            float currRadiusPx = currRadiusNorm * radius;
//
//            ImVec2 prevPos = { center.x + cos(prevAngle) * prevRadiusPx, center.y - sin(prevAngle) * prevRadiusPx };
//            ImVec2 currPos = { center.x + cos(currAngle) * currRadiusPx, center.y - sin(currAngle) * currRadiusPx };
//
//            float ageFactor = static_cast<float>(i) / static_cast<float>(historySize);
//            float correlation = static_cast<float>(analysis.mPhaseCorrelation);
//            ImU32 baseColor = GetCorrellationColor(correlation, 1);
//            int alpha = static_cast<int>((1.0f - ageFactor) * 180 + 50);
//            ImU32 color = (baseColor & 0x00FFFFFF) | (alpha << 24);
//
//            dl->AddLine(prevPos, currPos, color, 1.5f);
//        }
//    }
//
//    ImGui::Dummy(size);
//}
//


	// Goniometer layer renderer (based on RenderGoniometer but without background/grid)
inline void RenderGoniometerLayer(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size, ImVec2 center, float radius) {
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
			ImU32 baseColor = GetCorrellationColor(correlation, 1);
			int alpha = static_cast<int>((1.0f - ageFactor) * 180 + 50);
			ImU32 color = (baseColor & 0x00FFFFFF) | (alpha << 24);

			dl->AddLine(prevPos, currPos, color, 1.5f);
		}
	}
}

