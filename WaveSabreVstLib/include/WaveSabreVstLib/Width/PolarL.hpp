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

// Polar L layer renderer (based on RenderPolarL but without background/grid)
inline void RenderPolarLLayer(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size, ImVec2 center, float radius) {
	auto* dl = ImGui::GetWindowDrawList();

	// All the same static state management as in the original PolarL implementation
	constexpr size_t kMaxSectorCount = 128;
	static size_t kSectorCount = 96;
	static float kSmoothnessK = 0.f;
	static int kSmoothingKernelSize = 3;
	static float kVisualBoost = 1.0f;

	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorPeakRadiiMap;
	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorHoldTimersMap;
	static std::map<std::string, double> lastUpdateTimeMap;
	static std::map<std::string, bool> wasVisibleLastFrameMap;

	std::string instanceId = std::string(id) + "_polar";
	constexpr float kHoldTimeMs = 200.0f;
	constexpr float kFalloffTimeMs = 2000.0f;

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
					sectorHoldTimers[sectorIndex] = kHoldTimeMs;
				}
			}
		}

		for (size_t i = 0; i < kSectorCount; ++i) {
			if (sectorHoldTimers[i] > 0.0f) {
				sectorHoldTimers[i] = std::max(0.0f, sectorHoldTimers[i] - deltaTimeMs);
			}
			else {
				float falloffRate = 2.0f / kFalloffTimeMs;
				sectorPeakRadii[i] *= std::exp(-falloffRate * deltaTimeMs);
				if (sectorPeakRadii[i] < 0.01f) {
					sectorPeakRadii[i] = 0.0f;
				}
			}
		}
	}

	// Smoothing algorithm
	float averageRadius = 0.0f;
	float maxRadius = 0.0f;
	for (size_t i = 0; i < kSectorCount; ++i) {
		averageRadius += sectorPeakRadii[i];
		maxRadius = std::max(maxRadius, sectorPeakRadii[i]);
	}
	averageRadius /= static_cast<float>(kSectorCount);

	std::array<float, kMaxSectorCount> smoothedRadii = {};
	for (size_t i = 0; i < kSectorCount; ++i) {
		float smoothedValue = 0.0f;
		float weightSum = 0.0f;

		for (int k = -kSmoothingKernelSize; k <= kSmoothingKernelSize; ++k) {
			size_t sampleIndex = (i + k + kSectorCount) % kSectorCount;
			float weight = std::exp(-0.5f * (k * k) / (kSmoothingKernelSize * kSmoothingKernelSize * 0.25f));
			smoothedValue += sectorPeakRadii[sampleIndex] * weight;
			weightSum += weight;
		}

		float kernelSmoothed = smoothedValue / weightSum;
		float circularFallback = averageRadius;
		smoothedRadii[i] = kernelSmoothed * (1.0f - kSmoothnessK) + circularFallback * kSmoothnessK;

		float enhancedMinimumRadius = std::max(maxRadius * 0.15f, radius * 0.05f);
		smoothedRadii[i] = std::max(smoothedRadii[i], enhancedMinimumRadius);
	}

	// Build and draw envelope polygon
	std::vector<ImVec2> envelopePoints;
	envelopePoints.reserve(kSectorCount);

	for (size_t i = 0; i < kSectorCount; ++i) {
		float angle = (static_cast<float>(i) / static_cast<float>(kSectorCount)) * 2 * 3.14159f;
		float envelopeRadius = smoothedRadii[i];

		if (envelopeRadius > 0.01f) {
			ImVec2 point = {
				center.x + cos(angle) * envelopeRadius,
				center.y - sin(angle) * envelopeRadius
			};
			envelopePoints.push_back(point);
		}
	}

	if (envelopePoints.size() >= 3) {
		float correlation = static_cast<float>(analysis.mPhaseCorrelation);
		ImU32 envelopeFillColor = GetCorrellationColor(correlation, 0.15f);
		ImU32 envelopeLineColor = GetCorrellationColor(correlation, 0.4f);

		std::vector<ImVec2> reversedEnvelopePoints = envelopePoints;
		std::reverse(reversedEnvelopePoints.begin(), reversedEnvelopePoints.end());
		dl->AddConcavePolyFilled(reversedEnvelopePoints.data(), static_cast<int>(reversedEnvelopePoints.size()), envelopeFillColor);

		for (size_t i = 0; i < envelopePoints.size(); i++) {
			size_t nextIdx = (i + 1) % envelopePoints.size();
			dl->AddLine(envelopePoints[i], envelopePoints[nextIdx], envelopeLineColor, 1.3f);
		}
	}
}
