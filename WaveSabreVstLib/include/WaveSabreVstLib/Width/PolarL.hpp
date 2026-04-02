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


template<typename T, size_t N>
inline std::vector<T> VectorFromArray(std::array<T, N> arr) {
	return std::vector<T>(arr.begin(), arr.end());
}

struct PolarLBallisticsConfig {
	float holdTimeMs = 0.0f;
	float falloffTimeMs = 200.0f;
};

inline PolarLBallisticsConfig GetPolarLBallisticsConfig(StereoImagingAnalysisStream::BalanceBallisticsSpeed speed) {
	switch (speed) {
		case StereoImagingAnalysisStream::BalanceBallisticsSpeed::Momentary:
			return {0.0f, 50.0f};
		case StereoImagingAnalysisStream::BalanceBallisticsSpeed::Fast:
			return {0.0f, 200.0f};
		case StereoImagingAnalysisStream::BalanceBallisticsSpeed::Medium:
			return {40.0f, 1000.0f};
		case StereoImagingAnalysisStream::BalanceBallisticsSpeed::Slow:
			return {120.0f, 3000.0f};
		case StereoImagingAnalysisStream::BalanceBallisticsSpeed::Section:
			return {250.0f, 10000.0f};
	}

	return {0.0f, 200.0f};
}

// Polar L layer renderer (based on RenderPolarL but without background/grid)
inline void RenderPolarLLayer(const char* id,
		const StereoImagingAnalysisStream& analysis,
		ImVec2 size,
		ImVec2 center,
		float radius,
		const StereoImagingColorScheme& colorScheme = GetDefaultStereoImagingColorScheme()) {
	auto* dl = ImGui::GetWindowDrawList();
	(void)size;

	// All the same static state management as in the original PolarL implementation
	constexpr size_t kMaxSectorCount = 128;
	static size_t kSectorCount = 96;
	static float kVisualBoost = 1.0f;

	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorPeakRadiiMap;
	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorHoldTimersMap;
	static std::map<std::string, double> lastUpdateTimeMap;
	static std::map<std::string, bool> wasVisibleLastFrameMap;

	std::string instanceId = std::string(id) + "_polar";
	const auto ballistics = GetPolarLBallisticsConfig(analysis.GetBalanceBallisticsSpeed());

	auto& sectorPeakRadii = sectorPeakRadiiMap[instanceId];
	auto& sectorHoldTimers = sectorHoldTimersMap[instanceId];
	auto& lastUpdateTime = lastUpdateTimeMap[instanceId];
	auto& wasVisibleLastFrame = wasVisibleLastFrameMap[instanceId];

	bool isCurrentlyVisible = ImGui::IsItemVisible();

	if (isCurrentlyVisible && !wasVisibleLastFrame) {
		std::fill(sectorPeakRadii.begin(), sectorPeakRadii.end(), 0.0f);
		std::fill(sectorHoldTimers.begin(), sectorHoldTimers.end(), 0.0f);
		lastUpdateTime = ImGui::GetTime();
	}
	wasVisibleLastFrame = isCurrentlyVisible;

	if (isCurrentlyVisible) {
		double currentTime = ImGui::GetTime();
		float deltaTimeMs = static_cast<float>((currentTime - lastUpdateTime) * 1000.0);
		lastUpdateTime = currentTime;
		deltaTimeMs = std::max(0.0f, std::min(deltaTimeMs, 50.0f));

		const auto* history = analysis.GetHistoryBuffer();
		size_t historySize = analysis.GetHistorySize();

		if (historySize > 0) {
			static constexpr float gMaxLevel = 1.0f;
			size_t samplesToProcess = std::min(historySize, static_cast<size_t>(16));
			for (size_t i = historySize - samplesToProcess; i < historySize; ++i) {
				const auto& sample = history[i];

				float magnitude = std::sqrt(sample.left * sample.left + sample.right * sample.right);
				if (magnitude < 0.01f) continue;

				float mid = (sample.left + sample.right) * 0.5f;
				float side = (sample.left - sample.right) * 0.5f;
				float angle = atan2(side, mid) - 3.14159f * 0.5f;

				float compensatedMagnitude = std::sqrt(magnitude) * kVisualBoost;
				float currentRadius = std::min(compensatedMagnitude / gMaxLevel, 1.0f) * radius;

				while (angle < 0) angle += 2 * 3.14159f;
				while (angle >= 2 * 3.14159f) angle -= 2 * 3.14159f;

				size_t sectorIndex = static_cast<size_t>((angle / (2 * 3.14159f)) * kSectorCount) % kSectorCount;

				if (currentRadius > sectorPeakRadii[sectorIndex]) {
					sectorPeakRadii[sectorIndex] = currentRadius;
					sectorHoldTimers[sectorIndex] = ballistics.holdTimeMs;
				}
			}
		}

		for (size_t i = 0; i < kSectorCount; ++i) {
			if (sectorHoldTimers[i] > 0.0f) {
				sectorHoldTimers[i] = std::max(0.0f, sectorHoldTimers[i] - deltaTimeMs);
			}
			else {
				float falloffRate = 2.0f / std::max(ballistics.falloffTimeMs, 1.0f);
				sectorPeakRadii[i] *= std::exp(-falloffRate * deltaTimeMs);
				if (sectorPeakRadii[i] < 0.01f) {
					sectorPeakRadii[i] = 0.0f;
				}
			}
		}
	}

	std::array<ImVec2, kMaxSectorCount> envelopePoints = {};
	for (size_t i = 0; i < kSectorCount; ++i) {
		float angle = (static_cast<float>(i) / static_cast<float>(kSectorCount)) * 2.0f * 3.14159f;
		float envelopeRadius = sectorPeakRadii[i];
		envelopePoints[i] = {
			center.x + cos(angle) * envelopeRadius,
			center.y - sin(angle) * envelopeRadius
		};
	}

	float correlation = static_cast<float>(analysis.mPhaseCorrelation);
	ImU32 envelopeFillColor = GetCorrellationColor(correlation, 0.15f, colorScheme);
	ImU32 envelopeLineColor = GetCorrellationColor(correlation, 0.45f, colorScheme);

	const ImDrawListFlags oldFlags = dl->Flags;
	dl->Flags &= ~(ImDrawListFlags_AntiAliasedLines | ImDrawListFlags_AntiAliasedLinesUseTex | ImDrawListFlags_AntiAliasedFill);

	for (size_t i = 0; i < kSectorCount; ++i) {
		size_t nextIdx = (i + 1) % kSectorCount;
		const float r0 = sectorPeakRadii[i];
		const float r1 = sectorPeakRadii[nextIdx];
		if (r0 <= 0.01f && r1 <= 0.01f) {
			continue;
		}

		const ImVec2 p0 = envelopePoints[i];
		const ImVec2 p1 = envelopePoints[nextIdx];
		const ImVec2 fillSegment[3] = {center, p0, p1};
		dl->AddConvexPolyFilled(fillSegment, 3, envelopeFillColor);
		dl->AddLine(p0, p1, envelopeLineColor, 1.0f);
	}

	dl->Flags = oldFlags;
}
