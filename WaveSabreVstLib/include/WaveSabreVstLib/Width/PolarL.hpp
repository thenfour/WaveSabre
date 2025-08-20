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


// "Polar L" - Ozone-style envelope goniometer with sector peak detection
inline void RenderPolarL(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size) {
	constexpr size_t kMaxSectorCount = 128; // Power of 2 for better smoothing
	static size_t kSectorCount = 96; // Power of 2 for better smoothing
	static float kSmoothnessK = 0.f; // 0.0 = original shape, 1.0 = perfect circle -- not very necessary because kernel size does a lot of smoothing already.
	static int kSmoothingKernelSize = 3; // Larger = smoother curves
	static float kVisualBoost = 1.0f; // Perceptual compensation boost


	// for debugging, controls to change sector count, smoothness K, kernel size...
	//ImGui::BeginGroup();
	//ImGui::Text("Polar L Settings");
	//ImGui::SliderInt("Sector Count", (int*)&kSectorCount, 3, kMaxSectorCount, "%d");
	//ImGui::SliderFloat("Smoothness K", &kSmoothnessK, 0.0f, 1.0f, "%.2f");
	//ImGui::SliderInt("Smoothing Kernel Size", &kSmoothingKernelSize, 1, 32, "%d");
	//ImGui::SliderFloat("Visual Boost", &kVisualBoost, 0.5f, 4.0f, "%.2f");
	//ImGui::EndGroup();


	auto* dl = ImGui::GetWindowDrawList();
	ImVec2 pos = ImGui::GetCursorScreenPos();
	ImRect bb(pos, pos + size);

	// Background
	dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
	dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

	ImVec2 center = { bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f };
	const float radius = std::min(size.x, size.y) * 0.45f;

	// Draw concentric circles for magnitude reference
	for (int i = 1; i <= 4; ++i) {
		float r = radius * i * 0.25f;
		dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
	}

	// Draw angular reference lines
	for (int angle = 0; angle < 360; angle += 30) {
		float rad = angle * 3.14159f / 180.0f;
		ImVec2 lineEnd = { center.x + cos(rad) * radius, center.y + sin(rad) * radius };
		ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
		dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
	}

	// Add reference labels for key angles
	dl->AddText({ center.x - 15, bb.Min.y + 2 }, IM_COL32(100, 255, 100, 150), "MONO");          // 0° (top)
	dl->AddText({ bb.Max.x - 15, center.y - 8 }, IM_COL32(150, 150, 255, 150), "R");             // 90° (right)  
	dl->AddText({ center.x - 20, bb.Max.y - 16 }, IM_COL32(255, 100, 100, 150), "PHASE");        // 180° (bottom)
	dl->AddText({ bb.Min.x + 2, center.y - 8 }, IM_COL32(150, 150, 255, 150), "L");              // 270° (left)

	// Static peak detection system for envelope - using unique static storage per instance
	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorPeakRadiiMap;
	static std::map<std::string, std::array<float, kMaxSectorCount>> sectorHoldTimersMap;
	static std::map<std::string, double> lastUpdateTimeMap;

	std::string instanceId = std::string(id);
	constexpr float kHoldTimeMs = 200.0f;
	constexpr float kFalloffTimeMs = 2000.0f;

	auto& sectorPeakRadii = sectorPeakRadiiMap[instanceId];
	auto& sectorHoldTimers = sectorHoldTimersMap[instanceId];
	auto& lastUpdateTime = lastUpdateTimeMap[instanceId];

	// Get current time and calculate delta
	double currentTime = ImGui::GetTime();
	float deltaTimeMs = static_cast<float>((currentTime - lastUpdateTime) * 1000.0);
	lastUpdateTime = currentTime;

	// Clamp delta time to prevent huge jumps on first frame
	deltaTimeMs = std::min(deltaTimeMs, 50.0f);

	// Update peak detection system with current audio data
	const auto* history = analysis.GetHistoryBuffer();
	size_t historySize = analysis.GetHistorySize();

	if (historySize > 0) {
		static constexpr float gMaxLevel = 1.0f;

		// Process recent samples to update sector peaks
		size_t samplesToProcess = std::min(historySize, static_cast<size_t>(16));
		for (size_t i = historySize - samplesToProcess; i < historySize; ++i) {
			const auto& sample = history[i];

			float magnitude = std::sqrt(sample.left * sample.left + sample.right * sample.right);
			if (magnitude < 0.01f) continue;

			// Convert to mid-side for proper goniometer orientation
			float mid = (sample.left + sample.right) * 0.5f;
			float side = (sample.left - sample.right) * 0.5f;

			// Calculate angle and radius
			float angle = atan2(side, mid) - 3.14159f * 0.5f; // Rotate -90° so mono appears at top

			// *** PERCEPTUAL COMPENSATION BOOST ***
			// Apply scientifically-based scaling to compensate for processing losses:
			// 1. Square root for perceptual scaling (Stevens' Power Law for loudness ≈ amplitude^0.6)
			// 2. Additional boost for visual clarity 
			// 3. Compensate for smoothing losses
			float compensatedMagnitude = std::sqrt(magnitude) * kVisualBoost;  // sqrt ≈ power law + boost
			float currentRadius = std::min(compensatedMagnitude / gMaxLevel, 1.0f) * radius;

			// Normalize angle to 0-2π range
			while (angle < 0) angle += 2 * 3.14159f;
			while (angle >= 2 * 3.14159f) angle -= 2 * 3.14159f;

			// Determine which sector this sample belongs to
			size_t sectorIndex = static_cast<size_t>((angle / (2 * 3.14159f)) * kSectorCount) % kSectorCount;

			// Update peak for this sector with slight smoothing
			if (currentRadius > sectorPeakRadii[sectorIndex]) {
				sectorPeakRadii[sectorIndex] = currentRadius;
				sectorHoldTimers[sectorIndex] = kHoldTimeMs;
			}
		}
	}

	// Update peak hold and falloff for all sectors
	for (size_t i = 0; i < kSectorCount; ++i) {
		if (sectorHoldTimers[i] > 0.0f) {
			// In hold phase
			sectorHoldTimers[i] = std::max(0.0f, sectorHoldTimers[i] - deltaTimeMs);
		}
		else {
			// In falloff phase - exponential decay
			float falloffRate = 2.0f / kFalloffTimeMs; // Decay factor per ms
			sectorPeakRadii[i] *= std::exp(-falloffRate * deltaTimeMs);

			// Prevent underflow
			if (sectorPeakRadii[i] < 0.01f) {
				sectorPeakRadii[i] = 0.0f;
			}
		}
	}

	// === SMOOTHING ALGORITHM ===
	// Apply circular convolution smoothing to eliminate sharp polygon artifacts

	// Calculate average radius for circle fallback and mono signal detection
	float averageRadius = 0.0f;
	float maxRadius = 0.0f;
	for (size_t i = 0; i < kSectorCount; ++i) {
		averageRadius += sectorPeakRadii[i];
		maxRadius = std::max(maxRadius, sectorPeakRadii[i]);
	}
	averageRadius /= static_cast<float>(kSectorCount);

	// Create smoothed radius array using circular convolution
	std::array<float, kMaxSectorCount> smoothedRadii = {};

	for (size_t i = 0; i < kSectorCount; ++i) {
		float smoothedValue = 0.0f;
		float weightSum = 0.0f;

		// Apply smoothing kernel (Gaussian-like weighting)
		for (int k = -kSmoothingKernelSize; k <= kSmoothingKernelSize; ++k) {
			// Circular array indexing
			size_t sampleIndex = (i + k + kSectorCount) % kSectorCount;

			// Gaussian-like weight (closer samples have more influence)
			float weight = std::exp(-0.5f * (k * k) / (kSmoothingKernelSize * kSmoothingKernelSize * 0.25f));

			smoothedValue += sectorPeakRadii[sampleIndex] * weight;
			weightSum += weight;
		}

		// Normalize and blend with circular fallback
		float kernelSmoothed = smoothedValue / weightSum;
		float circularFallback = averageRadius; // Perfect circle

		// Apply smoothness blending: k=0 uses original kernel smoothing, k=1 uses circle
		smoothedRadii[i] = kernelSmoothed * (1.0f - kSmoothnessK) + circularFallback * kSmoothnessK;

		// Enhanced minimum radius for mono signals (prevents disappearing)
		// Use perceptually-scaled minimum that's more generous
		float enhancedMinimumRadius = std::max(maxRadius * 0.15f, radius * 0.05f); // Either 15% of peak OR 5% of total radius
		smoothedRadii[i] = std::max(smoothedRadii[i], enhancedMinimumRadius);
	}

	// Draw faint current trace for reference  
	if (historySize > 1) {
		for (size_t i = std::max(1ULL, historySize - 8); i < historySize; ++i) {
			const auto& prev = history[(i - 1) % historySize];
			const auto& curr = history[i % historySize];

			float prevMag = std::sqrt(prev.left * prev.left + prev.right * prev.right);
			float currMag = std::sqrt(curr.left * curr.left + curr.right * curr.right);
			if (prevMag < 0.01f && currMag < 0.01f) continue;

			float prevMid = (prev.left + prev.right) * 0.5f;
			float prevSide = (prev.left - prev.right) * 0.5f;
			float currMid = (curr.left + curr.right) * 0.5f;
			float currSide = (curr.left - curr.right) * 0.5f;

			float prevAngle = atan2(prevSide, prevMid) - 3.14159f * 0.5f;
			float currAngle = atan2(currSide, currMid) - 3.14159f * 0.5f;
			float prevRadius = std::min(std::sqrt(prevMag) * kVisualBoost, 1.0f) * radius;
			float currRadius = std::min(std::sqrt(currMag) * kVisualBoost, 1.0f) * radius;

			ImVec2 prevPos = { center.x + cos(prevAngle) * prevRadius, center.y - sin(prevAngle) * prevRadius };
			ImVec2 currPos = { center.x + cos(currAngle) * currRadius, center.y - sin(currAngle) * currRadius };

			// Very faint current trace
			ImU32 color = IM_COL32(150, 150, 150, 30);
			dl->AddLine(prevPos, currPos, color, 1.0f);
		}
	}

	// Build smooth envelope polygon from smoothed sector peaks
	std::vector<ImVec2> envelopePoints;
	envelopePoints.reserve(kSectorCount);

	for (size_t i = 0; i < kSectorCount; ++i) {
		float angle = (static_cast<float>(i) / static_cast<float>(kSectorCount)) * 2 * 3.14159f;
		float envelopeRadius = smoothedRadii[i];

		if (envelopeRadius > 0.01f) { // Always add points due to minimum radius
			ImVec2 point = {
				center.x + cos(angle) * envelopeRadius,
				center.y - sin(angle) * envelopeRadius
			};
			envelopePoints.push_back(point);
		}
	}

	// Draw the smooth envelope polygon
	if (envelopePoints.size() >= 3) {
		// Color based on phase correlation
		float correlation = static_cast<float>(analysis.mPhaseCorrelation);
		ImU32 envelopeFillColor = GetCorrellationColor(correlation, 0.1f),
			envelopeLineColor = GetCorrellationColor(correlation, 0.3f),
			phaseLineColor = GetCorrellationColor(correlation, 0.7f);

		// Draw filled envelope
		dl->AddConvexPolyFilled(envelopePoints.data(), static_cast<int>(envelopePoints.size()), envelopeFillColor);

		// Draw smooth envelope outline
		for (size_t i = 0; i < envelopePoints.size(); i++) {
			size_t nextIdx = (i + 1) % envelopePoints.size();
			dl->AddLine(envelopePoints[i], envelopePoints[nextIdx], envelopeLineColor, 1.3f);
		}

		// *** ADD BI-POLAR CORRELATION LINE OVERLAY (X-SHAPED) ***
		float correlationAngle = acosf(std::max(-1.0f, std::min(1.0f, correlation * .5f + .5f))); // Map +1→0°, -1→90°
		correlationAngle -= 3.14159f * 0.5f; // Rotate -90° so +1.0 (mono) points up (0°)

		// Calculate the line extent (use max envelope radius for proper scaling)
		float lineLength = std::max(maxRadius, radius * 0.8f); // Use envelope extent or minimum 80% of display radius

		// *** ORIGINAL CORRELATION LINE ***
		ImVec2 lineStart = {
			center.x - cosf(correlationAngle) * lineLength,
			center.y + sinf(correlationAngle) * lineLength
		};
		ImVec2 lineEnd = {
			center.x + cosf(correlationAngle) * lineLength,
			center.y - sinf(correlationAngle) * lineLength
		};

		// *** REFLECTED CORRELATION LINE (mirror around Y-axis to complete the X) ***
		ImVec2 reflectedLineStart = {
			center.x + cosf(correlationAngle) * lineLength,  // Flip X coordinate
			center.y + sinf(correlationAngle) * lineLength   // Keep Y coordinate same
		};
		ImVec2 reflectedLineEnd = {
			center.x - cosf(correlationAngle) * lineLength,  // Flip X coordinate  
			center.y - sinf(correlationAngle) * lineLength   // Keep Y coordinate same
		};

		// Draw both lines to form complete X/cross
		dl->AddLine(lineStart, lineEnd, phaseLineColor, 3.0f);                    // Original line "/"
		dl->AddLine(reflectedLineStart, reflectedLineEnd, phaseLineColor, 3.0f);  // Reflected line "\"

		// Add center dot to show the correlation X origin
		//dl->AddCircleFilled(center, 3.0f, envelopeLineColor, 8);
	}


	// Labels and info
	//dl->AddText({ bb.Min.x + 2, bb.Min.y + 2 }, IM_COL32(255, 255, 255, 150), "Polar L");
	//dl->AddText({ bb.Min.x + 2, bb.Min.y + 18 }, IM_COL32(200, 200, 200, 120), "Enhanced");

	// Phase correlation warning
	//if (analysis.mPhaseCorrelation < -0.3) {
	//	dl->AddText({ bb.Min.x + 2, bb.Min.y + 34 }, IM_COL32(255, 100, 100, 200), "PHASE ISSUE!");
	//}

	//// Analysis info
	//char polarText[128];
	//sprintf_s(polarText, "Corr: %.2f Boost: %.1fx", static_cast<float>(analysis.mPhaseCorrelation), kVisualBoost);
	//ImVec2 textSize = ImGui::CalcTextSize(polarText);
	//dl->AddText({ bb.Max.x - textSize.x - 2, bb.Max.y - 14 }, IM_COL32(255, 255, 255, 150), polarText);

	ImGui::Dummy(size);
}
