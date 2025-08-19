#pragma once

#include "Common.h"
#include <d3d9.h>
#include <functional>
#include <queue>
#include <string>
#include <vector>
#include <algorithm>  // for std::sort

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../imgui/imgui-knobs.h"
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"
#include "VstPlug.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>
#include <WaveSabreCore/Maj7MBC.hpp>
#include "FrequencyMagnitudeGraphUtils.hpp"

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib {

	//=============================================================================
	// Crossover response layer - renders Linkwitz-Riley crossover filters
	//=============================================================================
	template <int TSegmentCount>
	class CrossoverResponseLayer : public IFrequencyGraphLayer {
	private:
		// Connected device for param access
		const WaveSabreCore::Maj7MBC* mDevice = nullptr;

		std::vector<float> mFrequencies;
		std::vector<float> mScreenX;
		std::vector<std::vector<float>> mBandResponses; // Per-band magnitude responses

		// Visuals
		// Default colors (lows/mids/highs). If a global theme is available, substitute here.
		std::vector<ImColor> mBandColors{ ColorFromHTML("cc4444", 0.8f), ColorFromHTML("44cc44", 0.8f), ColorFromHTML("4444cc", 0.8f) };
		std::vector<const char*> mBandLabels{ "Low", "Mid", "High" };

		// Independent Y-axis scaling (like FFTSpectrumLayer)
		float mXODisplayMinDB = -40.0f;
		float mXODisplayMaxDB = 6.0f;
		bool mUseIndependentScale = true;

		// Parameter setter functions (similar to ThumbInteractionLayer pattern)
		std::function<void(float freqHz, int crossoverIndex)> mFrequencyChangeHandler;
		
		// Band selection handler
		std::function<void(int bandIndex)> mBandChangeHandler;

		// Currently selected band index (for highlighting)
		int mCurrentEditingBand = 0;

		// Band renderer callback with state information - REFACTORED
		// Callback signature: (bandIndex, bandRect, isHovered, isSelected, drawList) -> bool (true if handled mouse events)
		std::function<bool(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl)> mBandRenderer;

		// Current hovered band for state tracking
		int mCurrentHoveredBand = -1;

		// Interaction state for crossover frequency dragging
		struct CrossoverDragState {
			int activeCrossoverIndex = -1;     // Which crossover is being dragged (-1 = none)
			bool isDragging = false;           // Currently in drag operation
			ImVec2 dragStartMousePos;          // Mouse position when drag started
			float originalFreq = 0.0f;         // Original frequency when drag started
		} mDragState;

		// Helper to check if mouse is near a frequency label
		struct FrequencyLabelHitTest {
			int crossoverIndex = -1;           // -1 = no hit, 0 = crossover A, 1 = crossover B
			ImRect labelRect;                  // Bounding box for hit testing
			float frequency = 0.0f;            // The frequency value
		};

		// Convert dB to Y using independent scale if enabled
		float XODBToY(float dB, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
			if (mUseIndependentScale) {
				float t01 = M7::math::lerp_rev(mXODisplayMinDB, mXODisplayMaxDB, dB);
				return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
			}
			return coords.DBToY(dB, bb);
		}

		// Hit test against frequency labels and vertical crossover lines
		// This allows dragging to work by clicking either on the frequency labels or near the vertical lines
		FrequencyLabelHitTest HitTestFrequencyLabelsAndLines(ImVec2 mousePos, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) {
			FrequencyLabelHitTest result;

			if (!mDevice) return result;

			float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
			float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
			M7::ParamAccessor paA{ &rawA, 0 };
			M7::ParamAccessor paB{ &rawB, 0 };
			float fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
			float fB = paB.GetFrequency(0, M7::gFilterFreqConfig);

			std::vector<std::pair<float, int>> crossovers;
			if (fA > 0) crossovers.push_back({ fA, 0 });
			if (fB > 0) crossovers.push_back({ fB, 1 });

			for (const auto& crossover : crossovers) {
				float x = coords.FreqToX(crossover.first, bb);
				
				// First check the frequency label area
				char frequencyLabel[30];
				snprintf(frequencyLabel, sizeof(frequencyLabel), "%.0f Hz", crossover.first);
				ImVec2 textSize = ImGui::CalcTextSize(frequencyLabel);
				ImVec2 textPos = { x - textSize.x * 0.5f, bb.Min.y + 5 };

				// Create hit test area slightly larger than the label
				ImRect labelRect = {
					{ textPos.x - 4, textPos.y - 4 },
					{ textPos.x + textSize.x + 4, textPos.y + textSize.y + 4 }
				};

				if (labelRect.Contains(mousePos)) {
					result.crossoverIndex = crossover.second;
					result.labelRect = labelRect;
					result.frequency = crossover.first;
					return result; // Return early if we hit the label
				}

				// If not in label area, check if we're near the vertical line
				const float lineHitDistance = 8.0f; // Pixels on either side of the line
				if (mousePos.y >= bb.Min.y && mousePos.y <= bb.Max.y && // Within vertical bounds
					std::abs(mousePos.x - x) <= lineHitDistance) { // Near the vertical line
					
					result.crossoverIndex = crossover.second;
					// Create a hit rect around the line for consistency
					result.labelRect = ImRect(
						{ x - lineHitDistance, bb.Min.y },
						{ x + lineHitDistance, bb.Max.y }
					);
					result.frequency = crossover.first;
					return result;
				}
			}

			return result;
		}

		// Helper to determine which band the mouse is in based on crossover frequencies
		int GetBandIndexAtMousePosition(ImVec2 mousePos, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) {
			if (!mDevice || !ImGui::IsMouseHoveringRect(bb.Min, bb.Max)) {
				return -1;
			}

			float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
			float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
			M7::ParamAccessor paA{ &rawA, 0 };
			M7::ParamAccessor paB{ &rawB, 0 };
			float fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
			float fB = paB.GetFrequency(0, M7::gFilterFreqConfig);

			// Ensure crossovers are in correct order (A < B)
			if (fA > 0.0f && fB > 0.0f && fA > fB) std::swap(fA, fB);

			float mouseFreq = coords.XToFreq(mousePos.x, bb);

			// Determine which band based on crossover frequencies
			if (fA > 0.0f && fB > 0.0f) {
				// Both crossovers active: 3 bands
				return (mouseFreq < fA) ? 0 : ((mouseFreq < fB) ? 1 : 2);
			} else if (fA > 0.0f || fB > 0.0f) {
				// Only one crossover active: 2 bands
				float crossoverFreq = (fA > 0.0f) ? fA : fB;
				return (mouseFreq < crossoverFreq) ? 0 : 1;
			} else {
				// No crossovers: 1 band (or default to band 1 for single-band operation)
				return 1;
			}
		}

	public:
		CrossoverResponseLayer() {
			mBandResponses.resize(3); // Low, Mid, High
		}

		void SetCrossoverDevice(const WaveSabreCore::Maj7MBC* device) {
			mDevice = device;
		}

		// Set parameter change handler for frequency dragging
		void SetFrequencyChangeHandler(std::function<void(float freqHz, int crossoverIndex)> handler) {
			mFrequencyChangeHandler = handler;
		}

		// Set band selection handler for clicking on band regions - NEW
		void SetBandChangeHandler(std::function<void(int bandIndex)> handler) {
			mBandChangeHandler = handler;
		}

		// Set the currently selected/editing band for highlighting
		void SetCurrentEditingBand(int bandIndex) {
			mCurrentEditingBand = bandIndex;
		}

		// Set band renderer callback - REFACTORED
		void SetBandRenderer(std::function<bool(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl)> renderer) {
			mBandRenderer = renderer;
		}

	private:
		// Helper to calculate the bounding rectangle for a specific band region - NEW
		ImRect GetBandRect(int bandIndex, const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) const {
			if (!mDevice || bandIndex < 0) {
				return ImRect();
			}

			// Get crossover frequencies
			float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
			float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
			M7::ParamAccessor paA{ &rawA, 0 };
			M7::ParamAccessor paB{ &rawB, 0 };
			float fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
			float fB = paB.GetFrequency(0, M7::gFilterFreqConfig);

			// Ensure crossovers are in correct order (A < B)
			if (fA > 0.0f && fB > 0.0f && fA > fB) std::swap(fA, fB);

			float leftX = bb.Min.x;
			float rightX = bb.Max.x;

			// Determine band boundaries based on which crossovers are active
			if (fA > 0.0f && fB > 0.0f) {
				// Both crossovers active: 3 bands
				float xA = coords.FreqToX(fA, bb);
				float xB = coords.FreqToX(fB, bb);
				
				switch (bandIndex) {
				case 0: // Low band
					rightX = xA;
					break;
				case 1: // Mid band
					leftX = xA;
					rightX = xB;
					break;
				case 2: // High band
					leftX = xB;
					break;
				default:
					return ImRect(); // Invalid band index
				}
			} else if (fA > 0.0f || fB > 0.0f) {
				// Only one crossover active: 2 bands
				float crossoverX = coords.FreqToX((fA > 0.0f) ? fA : fB, bb);
				
				switch (bandIndex) {
				case 0: // Low band
					rightX = crossoverX;
					break;
				case 1: // High band  
					leftX = crossoverX;
					break;
				default:
					return ImRect(); // Invalid band index for 2-band mode
				}
			} else {
				// No crossovers: single band
				if (bandIndex != 1) {
					return ImRect(); // Only band 1 is valid in single-band mode
				}
				// Use full width for single band
			}

			return ImRect(leftX, bb.Min.y, rightX, bb.Max.y);
		}

	public:
		bool InfluencesAutoScale() const override { return true; }

		void GetDataBounds(float& minDB, float& maxDB) const override {
			minDB = -40.0f; // Crossovers typically don't go below -40dB
			maxDB = 6.0f;   // Allow some headroom for summing
		}

		bool HandleMouse(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
			ImVec2 mousePos = ImGui::GetIO().MousePos;
			bool mouseClicked = ImGui::IsMouseClicked(0);
			bool mouseDown = ImGui::IsMouseDown(0);
			bool mouseReleased = ImGui::IsMouseReleased(0);
			bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

			// Update current hovered band - NEW
			mCurrentHoveredBand = -1;
			if (mouseInBounds) {
				mCurrentHoveredBand = GetBandIndexAtMousePosition(mousePos, coords, bb);
			}

			// Handle drag end
			if (mDragState.isDragging && (mouseReleased || !mouseDown)) {
				mDragState.isDragging = false;
				mDragState.activeCrossoverIndex = -1;
				return true;
			}

			// Handle drag start
			if (mouseClicked && mouseInBounds && !mDragState.isDragging) {
				auto hitTest = HitTestFrequencyLabelsAndLines(mousePos, coords, bb);
				if (hitTest.crossoverIndex >= 0) {
					// Only start drag if we have a frequency change handler
					if (mFrequencyChangeHandler) {
						mDragState.activeCrossoverIndex = hitTest.crossoverIndex;
						mDragState.isDragging = true;
						mDragState.dragStartMousePos = mousePos;
						mDragState.originalFreq = hitTest.frequency;
						return true;
					}
				} else {
					// Check if band renderer handled the mouse event first - REFACTORED
					if (mBandRenderer && mCurrentHoveredBand >= 0) {
						ImRect bandRect = GetBandRect(mCurrentHoveredBand, coords, bb);
						if (bandRect.Contains(mousePos)) {
							bool isHovered = true;
							bool isSelected = (mCurrentHoveredBand == mCurrentEditingBand);
							
							// Call with nullptr to check if this is a clickable area
							bool isClickableArea = mBandRenderer(mCurrentHoveredBand, bandRect, isHovered, isSelected, nullptr);
							if (isClickableArea) {
								// Call again with special marker to actually handle the click
								bool handled = mBandRenderer(mCurrentHoveredBand, bandRect, isHovered, isSelected, reinterpret_cast<ImDrawList*>(1));
								if (handled) {
									return true;
								}
							}
						}
					}

					// No crossover hit and no renderer handled it - check for band selection
					if (mBandChangeHandler) {
						int bandIndex = GetBandIndexAtMousePosition(mousePos, coords, bb);
						if (bandIndex >= 0) {
							mBandChangeHandler(bandIndex);
							return true;
						}
					}
				}
			}

			// Handle active drag
			if (mDragState.isDragging && mDragState.activeCrossoverIndex >= 0 && mFrequencyChangeHandler) {
				// Calculate new frequency from mouse X position
				float clampedMouseX = M7::math::clamp(mousePos.x, bb.Min.x, bb.Max.x);
				float newFreq = coords.XToFreq(clampedMouseX, bb);

				// Apply reasonable limits for crossover frequencies
				newFreq = M7::math::clamp(newFreq, 30.0f, 18000.0f);

				// Call the parameter change handler
				mFrequencyChangeHandler(newFreq, mDragState.activeCrossoverIndex);

				return true;
			}

			return false; // Didn't consume any mouse events
		}

		void UpdateData(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb) override {
			// Generate screen-space frequency sampling
			ScreenSpaceFrequencySampler::GenerateFrequencyPoints(TSegmentCount, mFrequencies, mScreenX, coords, bb);

			// Default: empty responses if no device
			if (!mDevice) {
				for (auto& br : mBandResponses) br.assign(TSegmentCount, -100.0f);
				return;
			}

			// Read crossover frequencies from device params
			float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
			float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
			M7::ParamAccessor paA{ &rawA, 0 };
			M7::ParamAccessor paB{ &rawB, 0 };
			float crossoverA = paA.GetFrequency(0, M7::gFilterFreqConfig);
			float crossoverB = paB.GetFrequency(0, M7::gFilterFreqConfig);

			// Use splitter magnitudes from the device
			auto* pSplitter = &mDevice->splitter0;

			// Calculate responses for each band
			for (int i = 0; i < TSegmentCount; ++i) {
				float f = mFrequencies[i];

				// Low/Mid/High band magnitudes from device splitter (LR topology)
				auto mags = pSplitter->GetMagnitudesAtFrequency(f, crossoverA, crossoverB);
				float lowLin = mags[0];
				float midLin = mags[1];
				float highLin = mags[2];

				if (mBandResponses.size() < 3) mBandResponses.resize(3);
				if ((int)mBandResponses[0].size() != TSegmentCount) {
					mBandResponses[0].resize(TSegmentCount);
					mBandResponses[1].resize(TSegmentCount);
					mBandResponses[2].resize(TSegmentCount);
				}

				mBandResponses[0][i] = M7::math::LinearToDecibels(lowLin);
				mBandResponses[1][i] = M7::math::LinearToDecibels(midLin);
				mBandResponses[2][i] = M7::math::LinearToDecibels(highLin);
			}
		}

		void Render(const FrequencyMagnitudeCoordinateSystem& coords, const ImRect& bb, ImDrawList* dl) override {
			// Determine hovered band region using mouse position and crossover frequencies
			int hoveredBand = -1;
			float fA = 0.0f, fB = 0.0f;
			if (mDevice) {
				float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
				float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
				M7::ParamAccessor paA{ &rawA, 0 };
				M7::ParamAccessor paB{ &rawB, 0 };
				fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
				fB = paB.GetFrequency(0, M7::gFilterFreqConfig);
				if (fA > 0.0f && fB > 0.0f && fA > fB) std::swap(fA, fB);
			}
			bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);
			if (mouseInBounds && (fA > 0.0f || fB > 0.0f)) {
				float mouseX = ImGui::GetIO().MousePos.x;
				float freq = coords.XToFreq(mouseX, bb);
				if (fA > 0.0f && fB > 0.0f) {
					hoveredBand = (freq < fA) ? 0 : ((freq < fB) ? 1 : 2);
				} else if (fA > 0.0f) {
					hoveredBand = (freq < fA) ? 0 : 1;
				} else if (fB > 0.0f) {
					hoveredBand = (freq < fB) ? 0 : 1;
				}
			}

			// Render each crossover band response
			for (size_t bandIdx = 0; bandIdx < mBandResponses.size(); ++bandIdx) {
				const auto& bandResponse = mBandResponses[bandIdx];
				if (bandResponse.empty()) continue;

				// Determine band color
				ImColor bandColor = (bandIdx < mBandColors.size()) ? mBandColors[bandIdx] : ColorFromHTML("888888", 0.7f);

				// Fill area under curve by drawing trapezoids between successive samples
				const float faintAlpha = 0.05f;        // very faint by default
				const float hoverAlpha = 0.20f;        // hover opacity
				const float selectedAlpha = 0.30f;     // selected band opacity
				const float selectedHoverAlpha = 0.35f; // selected + hover opacity
				
				// Determine highlight state
				bool isHovered = (int(bandIdx) == hoveredBand);
				bool isSelected = (int(bandIdx) == mCurrentEditingBand);
				
				float chosenAlpha = faintAlpha;
				if (isSelected && isHovered) {
					chosenAlpha = selectedHoverAlpha;
				} else if (isSelected) {
					chosenAlpha = selectedAlpha;
				} else if (isHovered) {
					chosenAlpha = hoverAlpha;
				}
				
				ImColor fillColor = ImColor(bandColor.Value.x, bandColor.Value.y, bandColor.Value.z, chosenAlpha);
				float yBottom = bb.Max.y; // bottom of plot area
				float displayMin = mUseIndependentScale ? mXODisplayMinDB : coords.mDisplayMinDB;
				float displayMax = mUseIndependentScale ? mXODisplayMaxDB : coords.mDisplayMaxDB;
				for (int i = 0; i < TSegmentCount - 1; ++i) {
					float dB0 = bandResponse[i];
					float dB1 = bandResponse[i + 1];
					dB0 = M7::math::clamp(dB0, displayMin, displayMax);
					dB1 = M7::math::clamp(dB1, displayMin, displayMax);
					float x0 = mScreenX[i];
					float x1 = mScreenX[i + 1];
					float y0 = XODBToY(dB0, coords, bb);
					float y1 = XODBToY(dB1, coords, bb);
					ImVec2 quad[4] = { {x0, y0}, {x1, y1}, {x1, yBottom}, {x0, yBottom} };
					dl->AddConvexPolyFilled(quad, 4, fillColor);
				}

				// Build polyline points - enhance line visibility for selected band
				std::vector<ImVec2> points;
				points.reserve(TSegmentCount);

				for (int i = 0; i < TSegmentCount; ++i) {
					float dB = bandResponse[i];
					if (dB < displayMin || dB > displayMax) continue;

					float y = XODBToY(dB, coords, bb);
					points.push_back({ mScreenX[i], y });
				}

				if (points.size() >= 2) {
					// Enhanced line thickness and opacity for selected band
					float lineThickness = isSelected ? 3.0f : 2.0f;
					float lineAlpha = isSelected ? 0.9f : 0.8f;
					ImColor lineColor = ImColor(bandColor.Value.x, bandColor.Value.y, bandColor.Value.z, lineAlpha);
					dl->AddPolyline(points.data(), static_cast<int>(points.size()), lineColor, 0, lineThickness);
				}
			}

			// Draw crossover marker lines (if device present)
			if (mDevice) {
				float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
				float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
				M7::ParamAccessor paA{ &rawA, 0 };
				M7::ParamAccessor paB{ &rawB, 0 };
				float fA2 = paA.GetFrequency(0, M7::gFilterFreqConfig);
				float fB2 = paB.GetFrequency(0, M7::gFilterFreqConfig);

				std::vector<std::pair<float, int>> lines;
				if (fA2 > 0) lines.push_back({ fA2, 0 });
				if (fB2 > 0) lines.push_back({ fB2, 1 });

				ImVec2 mousePos = ImGui::GetIO().MousePos;
				bool mouseInBounds = ImGui::IsMouseHoveringRect(bb.Min, bb.Max);

				for (size_t i = 0; i < lines.size(); ++i) {
					float x = coords.FreqToX(lines[i].first, bb);
					ImColor c = (i < mBandColors.size()) ? ImColor(mBandColors[i].Value.x, mBandColors[i].Value.y, mBandColors[i].Value.z, 0.5f) : ColorFromHTML("cccccc", 0.5f);

					// Render frequency label with drag interaction feedback
					char frequencyLabel[30];
					snprintf(frequencyLabel, sizeof(frequencyLabel), "%.0f Hz", lines[i].first);
					ImVec2 textSize = ImGui::CalcTextSize(frequencyLabel);
					ImVec2 textPos = { x - textSize.x * 0.5f, bb.Min.y + 5 };

					// Determine interaction state for visual feedback
					bool isActive = (mDragState.isDragging && mDragState.activeCrossoverIndex == lines[i].second);
					bool isHovered = false;
					if (!mDragState.isDragging && mouseInBounds && mFrequencyChangeHandler) {
						auto hitTest = HitTestFrequencyLabelsAndLines(mousePos, coords, bb);
						isHovered = (hitTest.crossoverIndex == lines[i].second);
					}

					// Choose colors and styling based on interaction state
					ImColor bgColor = ColorFromHTML("000000", isActive ? 0.9f : (isHovered ? 0.8f : 0.7f));
					ImColor borderColor = ColorFromHTML(isActive ? "ffffff" : (isHovered ? "aaaaaa" : "666666"), isActive ? 1.0f : 0.9f);
					ImColor textColor = ColorFromHTML(isActive ? "ffffff" : (isHovered ? "ffffff" : "cccccc"));
					ImColor lineColor = c;

					// Enhanced visual feedback for the vertical line based on interaction state
					if (isActive) {
						lineColor = ImColor(mBandColors[i].Value.x, mBandColors[i].Value.y, mBandColors[i].Value.z, 0.9f);
					} else if (isHovered) {
						lineColor = ImColor(mBandColors[i].Value.x, mBandColors[i].Value.y, mBandColors[i].Value.z, 0.7f);
					}

					// Draw vertical line with interaction feedback
					float lineThickness = isActive ? 3.0f : (isHovered ? 2.5f : 2.0f);
					dl->AddLine({ x, bb.Min.y }, { x, bb.Max.y }, lineColor, lineThickness);

					// backdrop and outline with interaction feedback
					ImRect labelRect = { {textPos.x - 2, textPos.y - 2}, {textPos.x + textSize.x + 2, textPos.y + textSize.y + 2} };
					dl->AddRectFilled(labelRect.Min, labelRect.Max, bgColor);
					dl->AddRect(labelRect.Min, labelRect.Max, borderColor, 0, ImDrawFlags_RoundCornersAll, isActive ? 2.0f : 1.0f);
					
					// Add cursor indication for draggable elements (labels and lines)
					if (isHovered && mFrequencyChangeHandler) {
						ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
					}

					dl->AddText(textPos, textColor, frequencyLabel);
				}
			}

			// Show scale indicator when using independent scale
			if (mUseIndependentScale) {
				char scaleText[32];
				snprintf(scaleText, sizeof(scaleText), "XO: %.0f to %.0fdB", mXODisplayMinDB, mXODisplayMaxDB);
				ImVec2 textSize = ImGui::CalcTextSize(scaleText);
				ImVec2 textPos = { bb.Max.x - textSize.x - 4, bb.Min.y + 2 };
				dl->AddText(textPos, ColorFromHTML("888888", 0.7f), scaleText);
			}

			// Render band overlays for all bands - REFACTORED
			if (mBandRenderer) {
				// Determine which bands exist based on crossover configuration
				std::vector<int> activeBands;
				if (mDevice) {
					float rawA = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverAFrequency];
					float rawB = mDevice->mParamCache[(int)WaveSabreCore::Maj7MBC::ParamIndices::CrossoverBFrequency];
					M7::ParamAccessor paA{ &rawA, 0 };
					M7::ParamAccessor paB{ &rawB, 0 };
					float fA = paA.GetFrequency(0, M7::gFilterFreqConfig);
					float fB = paB.GetFrequency(0, M7::gFilterFreqConfig);

					if (fA > 0.0f && fB > 0.0f) {
						// Both crossovers active: 3 bands
						activeBands = {0, 1, 2};
					} else if (fA > 0.0f || fB > 0.0f) {
						// Only one crossover active: 2 bands  
						activeBands = {0, 1};
					} else {
						// No crossovers: single band
						activeBands = {1};
					}

					// Render overlays for all active bands
					for (int bandIndex : activeBands) {
						ImRect bandRect = GetBandRect(bandIndex, coords, bb);
						if (bandRect.GetWidth() > 0 && bandRect.GetHeight() > 0) {
							bool isHovered = (bandIndex == mCurrentHoveredBand);
							bool isSelected = (bandIndex == mCurrentEditingBand);
							mBandRenderer(bandIndex, bandRect, isHovered, isSelected, dl);
						}
					}
				}
			}
		}

		void SetDisplayRange(float minDB, float maxDB) { mXODisplayMinDB = minDB; mXODisplayMaxDB = maxDB; }
		void SetUseIndependentScale(bool use) { mUseIndependentScale = use; }
		void SetScaling(float minDB, float maxDB, bool independent = true) {
			mXODisplayMinDB = minDB; mXODisplayMaxDB = maxDB; mUseIndependentScale = independent;
		}
	};

	//=============================================================================
	// Complete crossover visualization using the layered architecture
	//=============================================================================
	template <int TWidth = 400, int THeight = 200>
	class CrossoverVisualization {
	private:
		FrequencyMagnitudeGraph<TWidth, THeight, true> mGraph;
		CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount>* mCrossoverLayer;

	public:
		CrossoverVisualization() {
			// Create layers
			auto gridLayer = std::make_unique<GridLayer<true>>();
			auto crossoverLayer = std::make_unique<CrossoverResponseLayer<FrequencyMagnitudeGraph<TWidth, THeight, true>::gSegmentCount>>();
			mCrossoverLayer = crossoverLayer.get(); // Keep raw pointer for access

			// Add layers to graph
			mGraph.AddLayer(std::move(gridLayer));
			mGraph.AddLayer(std::move(crossoverLayer));
		}

		void SetCrossoverDevice(const WaveSabreCore::Maj7MBC* device) {
			if (mCrossoverLayer) mCrossoverLayer->SetCrossoverDevice(device);
		}

		void SetFrequencyChangeHandler(std::function<void(float freqHz, int crossoverIndex)> handler) {
			if (mCrossoverLayer) mCrossoverLayer->SetFrequencyChangeHandler(handler);
		}

		void SetBandChangeHandler(std::function<void(int bandIndex)> handler) {
			if (mCrossoverLayer) mCrossoverLayer->SetBandChangeHandler(handler);
		}

		void SetCurrentEditingBand(int bandIndex) {
			if (mCrossoverLayer) mCrossoverLayer->SetCurrentEditingBand(bandIndex);
		}

		void SetBandRenderer(std::function<bool(int bandIndex, const ImRect& bandRect, bool isHovered, bool isSelected, ImDrawList* dl)> renderer) {
			if (mCrossoverLayer) mCrossoverLayer->SetBandRenderer(renderer);
		}
		
		void Render() {
			mGraph.Render();
		}
	};

} // namespace WaveSabreVstLib
