#ifndef __WAVESABREVSTLIB_VSTEDITOR_H__
#define __WAVESABREVSTLIB_VSTEDITOR_H__

#include "Common.h"
#include <d3d9.h>
#include <string>
#include <vector>
#include <queue>
#include <functional>

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../imgui/imgui.h"
#include "../imgui/imgui-knobs.h"
#include "../imgui/imgui_internal.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include "VstPlug.h"


using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{
	// this is basically a naive envelope follower, 0 attack to rectified signal peak, holding the peak for some samples.
	// why don't we use something like the Maj7Comp Envelope Follower? because the sample rate is VERY different; this is per frame.
	// and when you scale it back that far it behaves in an ugly way.
	struct HistoryEnvFollower
	{
		static constexpr size_t gBufferSize = 6;
		std::deque<float> mSamples;
		float ProcessSample(float sample) {
			mSamples.push_back(::fabsf(sample));
			if (mSamples.size() > gBufferSize) {
				mSamples.pop_front();
			}
			float ret = 0;
			for (auto s : mSamples) {
				ret = std::max(ret, s);
			}
			return ret;
		}
	};

	template<size_t window_size>
	struct MovingRMS {
		std::array<float, window_size> samples;
		int next_sample_index;
		float sum_squared;

		MovingRMS() {
			reset();
		}

		void addSample(float sample) {
			sum_squared -= samples[next_sample_index] * samples[next_sample_index];
			samples[next_sample_index] = sample;
			sum_squared += sample * sample;
			next_sample_index = (next_sample_index + 1) % window_size;
		}

		float getRMS() const {
			return ::sqrtf(sum_squared / window_size);
		}

		void reset() {
			for (int i = 0; i < window_size; i++) {
				samples[i] = 0.0;
			}
			next_sample_index = 0;
			sum_squared = 0.0;
		}
	};


	struct HistoryViewSeriesConfig
	{
		ImColor mLineColor;// = ColorFromHTML("ff8080", 0.7f);
		float mLineThickness;
	};

	struct HistoryViewSeries
	{
		HistoryEnvFollower mFollower; // actual sample values are not helpful; we need a peak env follower to look nice.
		std::deque<float> mHistDecibels;
		
		HistoryViewSeries() {
			//mFollower.SetParams(0, 100);
		}
	};

	static constexpr float gHistoryViewMinDB = -50;

	template<size_t TSeriesCount, int Twidth, int Theight>
	struct HistoryView
	{
		static constexpr ImVec2 gHistViewSize = { Twidth, Theight };
		static constexpr float gPixelsPerSample = 4;
		static constexpr int gSamplesInHist = (int)(gHistViewSize.x / gPixelsPerSample);

		HistoryViewSeries mSeries[TSeriesCount];

		void Render(const std::array< HistoryViewSeriesConfig, TSeriesCount>& cfg, const std::array<float, TSeriesCount>& dbValues)
		{
			ImRect bb;
			bb.Min = ImGui::GetCursorScreenPos();
			bb.Max = bb.Min + gHistViewSize;

			for (size_t i = 0; i < TSeriesCount; ++i) {

				float lin = mSeries[i].mFollower.ProcessSample(M7::math::DecibelsToLinear(dbValues[i]));

				mSeries[i].mHistDecibels.push_back(M7::math::LinearToDecibels(lin));
				if (mSeries[i].mHistDecibels.size() > gSamplesInHist) {
					mSeries[i].mHistDecibels.pop_front();
				}
			}

			ImColor backgroundColor = ColorFromHTML("222222", 1.0f);

			auto DbToY = [&](float db) {
				// let's show a range of -x to 0 db.
				float x = (db - gHistoryViewMinDB) / -gHistoryViewMinDB;
				x = M7::math::clamp01(x);
				return M7::math::lerp(bb.Max.y, bb.Min.y, x);
			};
			auto SampleToX = [&](int sample) {
				float sx = (float)sample;
				sx /= gSamplesInHist; // 0,1
				return M7::math::lerp(bb.Min.x, bb.Max.x, sx);
			};

			auto* dl = ImGui::GetWindowDrawList();

			ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

			for (size_t iSeries = 0; iSeries < TSeriesCount; ++iSeries) {
				std::vector<ImVec2> points;
				for (int isample = 0; isample < ((signed)mSeries[iSeries].mHistDecibels.size()); ++isample) {
					points.push_back({
						SampleToX(isample),
						DbToY(mSeries[iSeries].mHistDecibels[isample])
						});
				}
				dl->AddPolyline(points.data(), (int)points.size(), cfg[iSeries].mLineColor, 0, cfg[iSeries].mLineThickness);
			}

			ImGui::Dummy(gHistViewSize);
		}
	};


	// stolen from ImGui::ColorEdit4
	static ImColor ColorFromHTML(const char* buf, float alpha = 1.0f)
	{
		int i[3] = { 0 };
		const char* p = buf;
		while (*p == '#' || ImCharIsBlankA(*p))
			p++;
		i[0] = i[1] = i[2] = 0;
		int r = sscanf(p, "%02X%02X%02X", (unsigned int*)&i[0], (unsigned int*)&i[1], (unsigned int*)&i[2]);
		float f[3] = { 0 };
		for (int n = 0; n < 3; n++)
			f[n] = i[n] / 255.0f;
		return ImColor{ f[0], f[1], f[2], alpha };
	}


	///////////////////////////////////////////////////////////////////////////////////////////////////
	template<size_t TFilterCount, size_t TParamCount>
	struct FrequencyResponseRendererConfig {
		const ImColor backgroundColor;
		const ImColor lineColor;
		const BiquadFilter* (&pFilters)[TFilterCount];
		float mParamCacheCopy[TParamCount];
	};
	template<int Twidth, int Theight, int TsegmentCount, size_t TFilterCount, size_t TParamCount>
	struct FrequencyResponseRenderer
	{
		// the response graph is extremely crude; todo:
		// - add the user-selected points to the points list explicitly, to give better looking peaks. then you could reduce # of points.
		// - respect 'thru' member.
		// - display info about freq range and amplitude
		// - adjust amplitude

		static constexpr ImVec2 gSize = { Twidth, Theight };
		static constexpr int gSegmentCount = TsegmentCount;
		static constexpr int gSegmentWidth = (int)(gSize.x / gSegmentCount);

		// because params change as a result of this immediate gui, we need at least 1 full frame of lag to catch param changes correctly.
		// so keep processing multiple frames until things settle. in the meantime, force recalculating.
		int mAdditionalForceCalcFrames = 0;
		ImVec2 mPoints[gSegmentCount]; // actual pixel values.
		float mParamCacheCache[(size_t)LevellerParamIndices::NumParams] = { 0 }; // the param cache that points have been calculated on. this way we can only recalc the freq response when params change.

		int renderSerial = 0;

		// https://forum.juce.com/t/draw-frequency-response-of-filter-from-transfer-function/20669
		// https://www.musicdsp.org/en/latest/Analysis/186-frequency-response-from-biquad-coefficients.html
		// https://dsp.stackexchange.com/questions/3091/plotting-the-magnitude-response-of-a-biquad-filter
		static float BiquadMagnitudeForFrequency(const WaveSabreCore::BiquadFilter& bq, double freq) {
			static constexpr double tau = 6.283185307179586476925286766559;
			auto w = tau * freq / Helpers::CurrentSampleRate;

			double ma1 = double(bq.coeffs[1]) / bq.coeffs[0];
			double ma2 = double(bq.coeffs[2]) / bq.coeffs[0];
			double mb0 = double(bq.coeffs[3]) / bq.coeffs[0];
			double mb1 = double(bq.coeffs[4]) / bq.coeffs[0];
			double mb2 = double(bq.coeffs[5]) / bq.coeffs[0];

			double numerator = mb0 * mb0 + mb1 * mb1 + mb2 * mb2 + 2 * (mb0 * mb1 + mb1 * mb2) * ::cos(w) + 2 * mb0 * mb2 * ::cos(2 * w);
			double denominator = 1 + ma1 * ma1 + ma2 * ma2 + 2 * (ma1 + ma1 * ma2) * ::cos(w) + 2 * ma2 * ::cos(2 * w);
			double magnitude = ::sqrt(numerator / denominator);
			return (float)magnitude;
		}

		void CalculatePoints(const FrequencyResponseRendererConfig<TFilterCount, TParamCount>& cfg, ImRect& bb) {
			float underlyingValue = 0;
			float ktdummy = 0;
			M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig };

			renderSerial++;

			for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
				underlyingValue = float(iseg) / gSegmentCount;
				float freq = param.GetFrequency(0, 0);
				float magdB = 0;

				for (auto& f : cfg.pFilters) {
					if (!f) continue; // nullptr values are valid and used when a filter is bypassed.
					if (f->thru) continue;
					float magLin = BiquadMagnitudeForFrequency(*f, freq);
					magdB += M7::math::LinearToDecibels(magLin);
				}

				float magLin = M7::math::DecibelsToLinear(magdB) / 4; // /4 to basically give a 12db range of display.

				mPoints[iseg] = ImVec2(
					(bb.Min.x + iseg * bb.GetWidth() / gSegmentCount),
					bb.Max.y - bb.GetHeight() * M7::math::clamp01(magLin)
				);
			}
		}

		void EnsurePointsPopulated(const FrequencyResponseRendererConfig<TFilterCount, TParamCount>& cfg, ImRect& bb) {
			bool areEqual = memcmp(cfg.mParamCacheCopy, mParamCacheCache, sizeof(cfg.mParamCacheCopy)) == 0;

			if (areEqual && (mAdditionalForceCalcFrames == 0)) return;

			mAdditionalForceCalcFrames = areEqual ? mAdditionalForceCalcFrames - 1 : 2;

			memcpy(mParamCacheCache, cfg.mParamCacheCopy, sizeof(cfg.mParamCacheCopy));

			CalculatePoints(cfg, bb);
		}

		// even though you pass in the filters here, you're not allowed to change them due to how things are cached.
		void OnRender(const FrequencyResponseRendererConfig<TFilterCount, TParamCount>& cfg)
		{
			ImRect bb;
			bb.Min = ImGui::GetCursorScreenPos();
			bb.Max = bb.Min + gSize;

			auto* dl = ImGui::GetWindowDrawList();

			ImGui::RenderFrame(bb.Min, bb.Max, cfg.backgroundColor);
			//ImGui::RenderFrame(bb.Min, bb.Max, (renderSerial & 1) ? 0 : cfg.backgroundColor);

			EnsurePointsPopulated(cfg, bb);

			dl->AddPolyline(mPoints, gSegmentCount, cfg.lineColor, 0, 2.0f);

			ImGui::Dummy(gSize);
		}

	};

	static inline std::string midiNoteToString(int midiNote) {
		static constexpr char * const noteNames[] = {
			"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"
		};
		int note = midiNote % 12;
		int octave = midiNote / 12 - 1;
		return std::string(noteNames[note]) + std::to_string(octave) + " (" + std::to_string(midiNote) + ")";
	}

	static inline std::vector<std::pair<std::string, int>> LoadGmDlsOptions()
	{
		std::vector<std::pair<std::string, int>> mOptions;
		mOptions.push_back({ "(no sample)", -1 });
		//optionNames.insert(pair<int, string>(noSampleOption.second, noSampleOption.first));

		// Read gm.dls file
		auto gmDls = GmDls::Load();

		// Seek to wave pool chunk's data
		auto ptr = gmDls + GmDls::WaveListOffset;

		// Walk wave pool entries
		for (int i = 0; i < M7::gGmDlsSampleCount; i++)
		{
			// Walk wave list
			auto waveListTag = *((unsigned int*)ptr); // Should be 'LIST'
			ptr += 4;
			auto waveListSize = *((unsigned int*)ptr);
			ptr += 4;

			// Walk wave entry
			auto wave = ptr;
			auto waveTag = *((unsigned int*)wave); // Should be 'wave'
			wave += 4;

			// Skip fmt chunk
			auto fmtChunkTag = *((unsigned int*)wave); // Should be 'fmt '
			wave += 4;
			auto fmtChunkSize = *((unsigned int*)wave);
			wave += 4;
			wave += fmtChunkSize;

			// Skip wsmp chunk
			auto wsmpChunkTag = *((unsigned int*)wave); // Should be 'wsmp'
			wave += 4;
			auto wsmpChunkSize = *((unsigned int*)wave);
			wave += 4;
			wave += wsmpChunkSize;

			// Skip data chunk
			auto dataChunkTag = *((unsigned int*)wave); // Should be 'data'
			wave += 4;
			auto dataChunkSize = *((unsigned int*)wave);
			wave += 4;
			wave += dataChunkSize;

			// Walk info list
			auto infoList = wave;
			auto infoListTag = *((unsigned int*)infoList); // Should be 'LIST'
			infoList += 4;
			auto infoListSize = *((unsigned int*)infoList);
			infoList += 4;

			// Walk info entry
			auto info = infoList;
			auto infoTag = *((unsigned int*)info); // Should be 'INFO'
			info += 4;

			// Skip copyright chunk
			auto icopChunkTag = *((unsigned int*)info); // Should be 'ICOP'
			info += 4;
			auto icopChunkSize = *((unsigned int*)info);
			info += 4;
			// This size appears to be the size minus null terminator, yet each entry has a null terminator
			//  anyways, so they all seem to end in 00 00. Not sure why.
			info += icopChunkSize + 1;

			// Read name (finally :D)
			auto nameChunkTag = *((unsigned int*)info); // Should be 'INAM'
			info += 4;
			auto nameChunkSize = *((unsigned int*)info);
			info += 4;

			// Insert name into appropriate group
			auto name = std::string((char*)info);
			//OutputDebugStringA(name.c_str());
			//auto groupKey = string(1, name[0]);

			mOptions.push_back({ name, i });

			//options[groupKey].push_back(pair<string, int>(name, i));
			//optionNames.insert(pair<int, string>(i, name));

			ptr += waveListSize;
		}

		delete[] gmDls;
		return mOptions;
	}

	// making this an enum keeps the core from getting bloated. if we for example have some kind of behavior class passed in from the core, then those details will pollute core.
	enum class ParamBehavior
	{
		Default01,
		Unisono,
		VibratoFreq,
		Frequency,
		FilterQ,
		Db,
	};

	class VstEditor : public AEffEditor
	{
	public:

		static constexpr double gNormalKnobSpeed = 0.0015f;
		static constexpr double gSlowKnobSpeed = 0.000003f;


		ImRect tabBarBB;
		ImU32 tabBarStoredSeparatorColor;

		bool BeginTabBar2(const char* str_id, ImGuiTabBarFlags flags, float width = 0)
		{
			tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;
			ImGuiID id = window->GetID(str_id);
			ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
			// Copied from imgui with changes:
			// 1. shrinking workrect max for column support
			// 2. adding X cursor pos for side-by-side positioning
			// 3. pushing empty separator color so BeginTabBarEx doesn't display a separator line (we'll do it later).
			if (width == 0) {
				tabBarBB = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->DC.CursorPos.x + (window->WorkRect.Max.x), window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
			}
			else {
				tabBarBB = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->DC.CursorPos.x + width, window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
			}
			tab_bar->ID = id;
			ImGui::PushStyleColor(ImGuiCol_TabActive, {});// : ImGuiCol_TabUnfocusedActive
			bool ret = ImGui::BeginTabBarEx(tab_bar, tabBarBB, flags | ImGuiTabBarFlags_IsFocused);
			ImGui::PopStyleColor(1);
			return ret;
		}

		bool WSBeginTabItem(const char* label, bool* p_open = 0, ImGuiTabItemFlags flags = 0)
		{
			if (ImGui::BeginTabItem(label, p_open, flags)) {
				tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
				return true;
			}
			return false;
		}

		struct ImGuiTabSelectionHelper
		{
			int* mpSelectedTabIndex = nullptr; // pointer to the setting to read and store the current tab index
			int mImGuiSelection = 0; // which ID does ImGui currently have selected
		};

		// I HATE THIS LOGIC.
		// there's a lot of plumbing in the ImGuiTabBar stuff, so i don't want to mess with that to manipulate selection
		// there's a flag, ImGuiTabItemFlags_SetSelected , which lets us manually select a tab. but there are scenarios we have to specifically account for:
		// - 1st frame (think closing & reopening the editor window), we need to make sure our ImGui state shows selection of 0.
		// - upon loading a new patch make sure we're careful about responding to the selected tab.
		bool WSBeginTabItemWithSel(const char* label, int thisTabIndex, ImGuiTabSelectionHelper& helper)
		{
			// when losing and regaining focus, ImGui will reset its selection to 0, but we may think it's something else.
			// so have to specially check for that situation.
			if (GImGui->CurrentTabBar->SelectedTabId == 0) {
				helper.mImGuiSelection = 0;
			}

			// between BeginTabBar() and EndTabBar(), need to know when BeginTabItem() returns true whether that means the user selected a new tab,
			// or ImGui is just rendering the incorrect tab because we haven't reached the selected one yet. latter case make sure we don't misinterpret the result
			// of BeginTabItem().
			bool waitingForCorrectTab = false;
			int flags = 0;
			if (helper.mImGuiSelection != *helper.mpSelectedTabIndex) {
				waitingForCorrectTab = true;
				if (thisTabIndex == *helper.mpSelectedTabIndex) {
					flags |= ImGuiTabItemFlags_SetSelected;
				}
			}

			if (ImGui::BeginTabItem(label, 0, flags)) {
				tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);

				helper.mImGuiSelection = thisTabIndex;

				if (GImGui->CurrentTabBar->SelectedTabId != 0 && (*helper.mpSelectedTabIndex != thisTabIndex)) {
					if (!waitingForCorrectTab) {
						*helper.mpSelectedTabIndex = thisTabIndex;
					}
				}

				return true;
			}
			return false;
		}

		void EndTabBarWithColoredSeparator()
		{
			ImGuiContext& g = *GImGui;
			ImGuiWindow* window = g.CurrentWindow;
			ImGuiTabBar* tab_bar = g.CurrentTabBar;
			const ImU32 col = tabBarStoredSeparatorColor;
			const float y = tab_bar->BarRect.Max.y - 1.0f;

			// account for the popup indicator arrow thingy.
			float separator_min_x = std::min(tab_bar->BarRect.Min.x, tabBarBB.Min.x) - M7::math::floor(window->WindowPadding.x * 0.5f);
			float separator_max_x = tab_bar->BarRect.Max.x + M7::math::floor(window->WindowPadding.x * 0.5f);

			ImRect body_bb;
			body_bb.Min = { separator_min_x, y };
			body_bb.Max = { separator_max_x, window->DC.CursorPos.y };

			ImColor c2 = { col };
			float h, s, v;
			ImGui::ColorConvertRGBtoHSV(c2.Value.x, c2.Value.y, c2.Value.z, h, s, v);
			auto halfAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.5f);
			auto lowAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.15f);

			//window->DrawList->AddLine(ImVec2(separator_min_x, y), ImVec2(separator_max_x, y), col, 1.0f);

			ImGui::GetBackgroundDrawList()->AddRectFilled(body_bb.Min, body_bb.Max, lowAlpha, 6, ImDrawFlags_RoundCornersAll);
			ImGui::GetBackgroundDrawList()->AddRect(body_bb.Min, body_bb.Max, halfAlpha, 6, ImDrawFlags_RoundCornersAll);

			ImGui::EndTabBar();

			// add some bottom margin.
			ImGui::Dummy({ 0, 7 });
		}


		VstEditor(AudioEffect* audioEffect, int width, int height);
		virtual ~VstEditor();



		virtual void Window_Open(HWND parent);
		virtual void Window_Close();

		virtual bool getRect(ERect** ppRect) {
			*ppRect = &mDefaultRect;
			return true;
		}

		virtual void renderImgui() = 0;
		virtual void PopulateMenuBar() {}; // can override to add menu items
		virtual std::string GetMenuBarStatusText() { return {}; };

		bool open(void* ptr) override
		{
			AEffEditor::open(ptr);
			Window_Open((HWND)ptr);
			return true;
		}

		void close() override
		{
			Window_Close();
			AEffEditor::close();
		}

		struct VibratoFreqConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", (float)::WaveSabreCore::Helpers::ParamToVibratoFreq((float)param));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::VibratoFreqToParam((float)value);
			}
		};

		struct FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", (float)::WaveSabreCore::Helpers::ParamToFrequency((float)param));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::FrequencyToParam((float)value);
			}
		};

		struct ScaledFloatConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::ScaledRealParam mParam;

			ScaledFloatConverter(float v_min, float v_max) :
				mParam(mBacking, v_min, v_max)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", mParam.GetRangedValue());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				mParam.SetRangedValue((float)value);
				return (double)mParam.GetRawParamValue();
			}
		};

		struct Maj7IntConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::IntParam mParam;

			Maj7IntConverter(const M7::IntParamConfig& cfg) :
				mParam(mBacking, cfg)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%d", mParam.GetIntValue());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				mParam.SetIntValue((int)value);
				return (double)mParam.GetRawParamValue();
			}
		};

		struct Maj7MidiNoteConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::IntParam mParam;

			Maj7MidiNoteConverter() :
				mParam(mBacking, M7::gKeyRangeCfg)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				return midiNoteToString(mParam.GetIntValue());
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		

		struct Maj7FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			float mBackingKT;
			WaveSabreCore::M7::FrequencyParam mParam;
			VstInt32 mKTParamID;

			Maj7FrequencyConverter(M7::FreqParamConfig cfg, VstInt32 ktParamID /*pass -1 if no KT*/) :
				mParam(mBacking, mBackingKT, cfg),
				mKTParamID(ktParamID)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.mValue.SetParamValue((float)param);
				VstEditor* pThis = (VstEditor*)capture;
				if (mKTParamID >= 0) {
					mParam.mKTValue.SetParamValue(pThis->GetEffectX()->getParameter(mKTParamID));
				}

				char s[100] = { 0 };
				if (mParam.mKTValue.Get01Value() < 0.00001f) {
					M7::real_t hz = mParam.GetFrequency(0, 0);
					if (hz >= 1000) {
						sprintf_s(s, "%.0fHz", hz);
					}
					else if (hz >= 100) {
						sprintf_s(s, "%.2fHz", hz);
					}
					else if (hz >= 10) {
						sprintf_s(s, "%.3fHz", hz);
					}
					else {
						sprintf_s(s, "%.4fHz", hz);
					}
				}
				else {
					// with KT applied, the frequency doesn't really matter.
					sprintf_s(s, "%0.0f%%", mParam.mValue.Get01Value() * 100);
				}

				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct EnvTimeConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::EnvTimeParam mParam;

			EnvTimeConverter() :
				mParam(mBacking, 0)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				M7::real_t ms = mParam.GetMilliseconds();
				if (ms > 2000)
				{
					sprintf_s(s, "%0.1f s", ms / 1000);
				}
				else if (ms > 1000) {
					sprintf_s(s, "%0.2f s", ms / 1000);
				}
				else {
					sprintf_s(s, "%0.2f ms", ms);
				}
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				//mParam.SetRangedValue((float)value);
				//return (double)mParam.GetRawParamValue();
				return 0;
			}
		};

		struct TimeConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParam{ &mBacking, 0 };
			const M7::TimeParamCfg& mConfig;

			TimeConverter(const M7::TimeParamCfg& cfg) : mConfig(cfg)
			{}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				char s[100] = { 0 };
				M7::real_t ms = mParam.GetTimeMilliseconds(0, mConfig, 0);
				if (ms > 2000)
				{
					sprintf_s(s, "%0.1f s", ms / 1000);
				}
				else if (ms > 1000) {
					sprintf_s(s, "%0.2f s", ms / 1000);
				}
				else {
					sprintf_s(s, "%0.2f ms", ms);
				}
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				//mParam.SetRangedValue((float)value);
				//return (double)mParam.GetRawParamValue();
				return 0;
			}
		};

		struct DivCurvedConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParam{ &mBacking, 0 };
			const M7::DivCurvedParamCfg& mConfig;

			DivCurvedConverter(const M7::DivCurvedParamCfg& cfg) : mConfig(cfg)
			{}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				char s[100] = { 0 };
				M7::real_t v = mParam.GetDivCurvedValue(0, mConfig, 0);
				sprintf_s(s, "%0.2f", v);
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct FloatN11Converter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::FloatN11Param mParam;

			FloatN11Converter() :
				mParam(mBacking)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%0.3f", mParam.GetN11Value());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct Float01Converter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::Float01Param mParam;

			Float01Converter() :
				mParam(mBacking)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%0.3f", mParam.Get01Value());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct FMValueConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::Float01Param mParam;

			FMValueConverter() :
				mParam(mBacking)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%d", int(mParam.Get01Value() * 100));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct M7VolumeConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::VolumeParam mParam;

			M7VolumeConverter(const M7::VolumeParamConfig& cfg) :
				mParam(mBacking, cfg)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				if (mParam.IsSilent()) {
					return "-inf";
				}
				float db = mParam.GetDecibels();
				sprintf_s(s, "%c%0.2fdB", db < 0 ? '-' : '+', ::fabsf(mParam.GetDecibels()));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct DbConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", ::WaveSabreCore::Helpers::ParamToDb((float)param));
				return s;
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::DbToParam((float)value);
			}
		};

		struct FilterQConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", ::WaveSabreCore::Helpers::ParamToQ((float)param));
				return s;
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::QToParam((float)value);
			}
		};

		// case ParamIndices::LoopMode: return (float)loopMode / (float)((int)LoopMode::NumLoopModes - 1);
		//template<typename T>
		float EnumToParam(int value, int valueCount, int baseVal = 0) {
			return (float)(value - baseVal) / (valueCount - 1);
		}

		// case ParamIndices::LoopMode: loopMode = (LoopMode)(int)(value * (float)((int)LoopMode::NumLoopModes - 1)); break;
		//template<typename T>
		int ParamToEnum(float value, int valueCount, int baseVal = 0) {
			if (value < 0) value = 0;
			if (value > 1) value = 1;
			return baseVal + (int)(value * (valueCount - 1));
		}

		bool WSImGuiParamKnob(VstInt32 id, const char* name, ParamBehavior behavior = ParamBehavior::Default01, const char* fmt = "%.3f") {
			float paramValue = GetEffectX()->getParameter((VstInt32)id);
			bool r = false;
			switch (behavior) {
			case ParamBehavior::Unisono:
			{
				int iparam = ::WaveSabreCore::Helpers::ParamToUnisono(paramValue);
				r = ImGuiKnobs::KnobInt(name, &iparam, 1, 16, 1, 0, .1f, .001f, NULL, ImGuiKnobVariant_WiperOnly);
				if (r) {
					paramValue = ::WaveSabreCore::Helpers::UnisonoToParam(iparam);
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::VibratoFreq:
			{
				static VibratoFreqConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::Db:
			{
				static DbConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.5f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::FilterQ:
			{
				static FilterQConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}

			case ParamBehavior::Frequency:
			{
				static FrequencyConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0.0f, 1.0f, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, "%.2fHz", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);

				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::Default01:
			default:
			{
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			}
			return r;
		}

		void WSImGuiParamKnobInt(VstInt32 id, const char* name, int v_min, int v_max, int v_default, int v_center) {
			float paramValue = GetEffectX()->getParameter((VstInt32)id);
			int v_count = v_max - v_min + 1;
			int iparam = ParamToEnum(paramValue, v_count, v_min);
			if (ImGuiKnobs::KnobInt(name, &iparam, v_min, v_max, v_default, v_center, 0.003f * v_count, 0.0001f * v_count, NULL, ImGuiKnobVariant_WiperOnly))
			{
				paramValue = EnumToParam(iparam, v_count, v_min);
				GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
			}
		}


		float Clamp01(float x)
		{
			if (x < 0) return 0;
			if (x > 1) return 1;
			return x;
		}

		std::string GetRenderedTextFromID(const char* id) {
			auto end = ImGui::FindRenderedTextEnd(id, 0);
			return std::string(id, end);
		}

		// for boolean types
		bool WSImGuiParamCheckbox(VstInt32 paramId, const char* id) {
			ImGui::BeginGroup();
			ImGui::PushID(id);
			float paramValue = GetEffectX()->getParameter((VstInt32)paramId);
			bool bval = ::WaveSabreCore::Helpers::ParamToBoolean(paramValue);
			bool r = false;
			ImGui::Text(GetRenderedTextFromID(id).c_str());
			r = ImGui::Checkbox("##cb", &bval);
			if (r) {
				GetEffectX()->setParameterAutomated(paramId, ::WaveSabreCore::Helpers::BooleanToParam(bval));
			}
			ImGui::PopID();
			ImGui::EndGroup();
			return r;
		}

		ImVec2 CalcListBoxSize(float items)
		{
			return { 0.0f, ImGui::GetTextLineHeightWithSpacing() * items + ImGui::GetStyle().FramePadding.y * 2.0f };
		}

		//template<typename Tenum>
		void WSImGuiParamEnumList(VstInt32 paramID, const char* ctrlLabel, int elementCount, const char* const* const captions) {
			//const char* captions[] = { "LP", "HP", "BP","Notch" };

			float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
			auto friendlyVal = ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			int enumIndex = (int)friendlyVal;

			const char* elem_name = "ERROR";
			//int elementCount = (int)std::size(captions);

			ImGui::PushID(ctrlLabel);
			ImGui::PushItemWidth(70);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			ImGui::Text("%s", txt.c_str());

			if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + elementCount)))
			{
				for (int n = 0; n < elementCount; n++)
				{
					const bool is_selected = (enumIndex == n);
					if (ImGui::Selectable(captions[n], is_selected)) {
						enumIndex = n;
						paramValue = EnumToParam(n, elementCount);// ::WaveSabreCore::Helpers::StateVariableFilterTypeToParam((::WaveSabreCore::StateVariableFilterType)enumIndex);
						GetEffectX()->setParameterAutomated(paramID, Clamp01(paramValue));
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			ImGui::EndGroup();
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		void WSImGuiParamVoiceMode(VstInt32 paramID, const char* ctrlLabel) {
			static constexpr char const* const captions[] = { "Poly", "Mono" };
			WSImGuiParamEnumList(paramID, ctrlLabel, 2, captions);
		}

		void WSImGuiParamFilterType(VstInt32 paramID, const char* ctrlLabel) {
			static constexpr char const* const captions[] = { "LP", "HP", "BP","Notch" };
			WSImGuiParamEnumList(paramID, ctrlLabel, 4, captions);
		}

		// For the Maj7 synth, let's add more param styles.
		template<typename Tenum, typename TparamID, typename Tcount>
		void Maj7ImGuiParamEnumList(TparamID paramID, const char* ctrlLabel, Tcount elementCount, Tenum defaultVal, const char* const* const captions) {
			M7::real_t tempVal;
			M7::EnumParam<Tenum> p(tempVal, Tenum(elementCount));
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			auto friendlyVal = p.GetEnumValue();// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			int enumIndex = (int)friendlyVal;

			const char* elem_name = "ERROR";

			ImGui::PushID(ctrlLabel);
			ImGui::PushItemWidth(70);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			ImGui::Text("%s", txt.c_str());

			if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + (int)elementCount)))
			{
				for (int n = 0; n < (int)elementCount; n++)
				{
					const bool is_selected = (enumIndex == n);
					if (ImGui::Selectable(captions[n], is_selected)) {
						p.SetEnumValue((Tenum)n);
						GetEffectX()->setParameterAutomated((VstInt32)paramID, Clamp01(tempVal));
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			ImGui::EndGroup();
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		template<typename Tenum, typename TparamID, typename Tcount>
		void Maj7ImGuiParamEnumCombo(TparamID paramID, const char* ctrlLabel, Tcount elementCount, Tenum defaultVal, const char* const* const captions, float width = 120) {
			M7::real_t tempVal;
			M7::EnumParam<Tenum> p(tempVal, Tenum(elementCount));
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			auto friendlyVal = p.GetEnumValue();// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			int enumIndex = (int)friendlyVal;

			const char* elem_name = "ERROR";

			ImGui::PushID(ctrlLabel);
			ImGui::PushItemWidth(width);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			ImGui::Text("%s", txt.c_str());

			if (ImGui::BeginCombo("##enumlist", captions[enumIndex]))
			{
				for (int n = 0; n < (int)elementCount; n++)
				{
					const bool is_selected = (enumIndex == n);
					if (ImGui::Selectable(captions[n], is_selected)) {
						p.SetEnumValue((Tenum)n);
						GetEffectX()->setParameterAutomated((VstInt32)paramID, Clamp01(tempVal));
					}
				}
				ImGui::EndCombo();
			}

			ImGui::EndGroup();
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		template<typename TEnum>
		struct EnumToggleButtonArrayItem {
			const char* caption;
			const char* selectedColor;
			const char* notSelectedColor;
			const char* selectedHoveredColor;
			const char* notSelectedHoveredColor;
			TEnum value;
			TEnum valueOnDeselect; // when the user unclicks the value, what should the underlying param get set to?
		};

		template<typename Tenum, typename TparamID, size_t Tcount>
		void Maj7ImGuiParamEnumToggleButtonArray(TparamID paramID, const char* ctrlLabel, float width, const EnumToggleButtonArrayItem<Tenum> (&itemCfg)[Tcount]) {
			M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor pa{ &tempVal, 0 };
			auto selectedVal = pa.GetEnumValue<Tenum>(0);

			ImGui::PushID(ctrlLabel);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			if (txt.length()) {
				ImGui::Text("%s", txt.c_str());
			}

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
			
			for (size_t i = 0; i < Tcount; i++)
			{
				auto& cfg = itemCfg[i];
				const bool is_selected = (selectedVal  == cfg.value);
				int colorsPushed = 0;

				if (is_selected) {
					if (cfg.selectedColor) {
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
						colorsPushed +=2;
					}
					if (cfg.selectedHoveredColor) {
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.selectedHoveredColor));
						colorsPushed++;
					}
				}
				else {
					if (cfg.notSelectedColor) {
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
						colorsPushed += 2;
					}
					if (cfg.notSelectedHoveredColor) {
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.notSelectedHoveredColor));
						colorsPushed++;
					}
				}

				if (ImGui::Button(cfg.caption, ImVec2{ width , 0 })) {
					pa.SetEnumValue(0, is_selected ? cfg.valueOnDeselect : cfg.value);
					GetEffectX()->setParameterAutomated((VstInt32)paramID, tempVal);
				}

				ImGui::PopStyleColor(colorsPushed);
			}

			ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
			 
			ImGui::EndGroup();
			ImGui::PopID();
		}



		template<typename TparamID>
		void Maj7ImGuiParamBoolToggleButton(TparamID paramID, const char* label, const char* selectedColorHTML = 0) {
			M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor pa{ &tempVal, 0 };
			auto is_selected = pa.GetBoolValue(0);

			ImGui::PushID(label);

			ImGui::BeginGroup();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

			int colorsPushed = 0;

			if (is_selected) {
				if (selectedColorHTML) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(selectedColorHTML));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(selectedColorHTML));
					colorsPushed += 2;
				}
			}

			if (ImGui::Button(label)) {
				pa.SetBoolValue(0, !is_selected);
				GetEffectX()->setParameterAutomated((VstInt32)paramID, tempVal);
			}

			ImGui::PopStyleColor(colorsPushed);

			ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

			ImGui::EndGroup();
			ImGui::PopID();
		}



		void Maj7ImGuiParamScaledFloat(VstInt32 paramID, const char* label, M7::real_t v_min, M7::real_t v_max, M7::real_t v_defaultScaled, float v_centerScaled, float sizePixels, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ScaledRealParam p{ tempVal , v_min, v_max };
			p.SetRangedValue(v_defaultScaled);
			float defaultParamVal = p.Get01Value();
			p.SetRangedValue(v_centerScaled);
			float centerParamVal = p.Get01Value();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			ScaledFloatConverter conv{ v_min, v_max };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, sizePixels, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFMKnob(VstInt32 paramID, const char* label) {
			WaveSabreCore::M7::real_t tempVal;
			M7::Float01Param p{ tempVal };
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			const float v_default = 0;
			//const float size = ImGui::GetTextLineHeight()* 2.5f;// default is 3.25f;
			FMValueConverter conv;
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, v_default, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, "%.2f", ImGuiKnobVariant_ProgressBarWithValue, 0, ImGuiKnobFlags_CustomInput | ImGuiKnobFlags_NoInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamInt(VstInt32 paramID, const char* label, const M7::IntParamConfig& cfg, int v_defaultScaled, int v_centerScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::IntParam p{ tempVal , cfg };
			p.SetIntValue(v_defaultScaled);
			float defaultParamVal = p.Get01Value();
			p.SetIntValue(v_centerScaled);
			float centerParamVal = p.Get01Value();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7IntConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFrequency(VstInt32 paramID, VstInt32 ktParamID, const char* label, M7::FreqParamConfig cfg, M7::real_t defaultParamValue, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal = 0;
			M7::real_t tempValKT = 0;
			M7::FrequencyParam p{ tempVal, tempValKT, cfg };
			p.mValue.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7FrequencyConverter conv{ cfg, ktParamID };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamValue, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFrequencyWithCenter(VstInt32 paramID, VstInt32 ktParamID, const char* label, M7::FreqParamConfig cfg, M7::real_t defaultFreqHz, float centerVal01, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal = 0;
			M7::real_t tempValKT = 0;
			M7::FrequencyParam p{ tempVal, tempValKT, cfg };
			p.SetFrequencyAssumingNoKeytracking(defaultFreqHz);
			//tempVal = defaultFrequencyHz;
			float default01 = tempVal;
			//tempVal = centerValHz;
			//float centerVal01 = p.GetFrequency(0, 0);
			p.mValue.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7FrequencyConverter conv{ cfg, ktParamID };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, default01, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamMidiNote(VstInt32 paramID, const char* label, int defaultVal, int centerVal) {
			M7::real_t tempVal = 0;
			M7::IntParam p{ tempVal, M7::gKeyRangeCfg };
			p.SetIntValue(defaultVal);
			float defaultParamVal = tempVal;
			p.SetIntValue(centerVal);
			float centerParamVal = tempVal;
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7MidiNoteConverter conv;
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_ProgressBar, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		template<typename Tparam>
		void Maj7ImGuiParamTime(Tparam paramID, const char* label, const M7::TimeParamCfg& cfg, M7::real_t defaultMS, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal;
			M7::ParamAccessor p{ &tempVal, 0 };
			p.SetTimeMilliseconds(0, cfg, defaultMS);
			float defaultParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			//float ms = p.GetTimeMilliseconds(0, cfg, 0);

			TimeConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated((VstInt32)paramID, Clamp01(tempVal));
			}
		}

		template<typename Tparam>
		void Maj7ImGuiDivCurvedParam(Tparam paramID, const char* label, const M7::DivCurvedParamCfg& cfg, M7::real_t defaultValue, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal;
			M7::ParamAccessor p{ &tempVal, 0 };
			p.SetDivCurvedValue(0, cfg, defaultValue);
			float defaultParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			//float val = p.GetDivCurvedValue(0, cfg, 0);

			DivCurvedConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated((VstInt32)paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamEnvTime(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::EnvTimeParam p{ tempVal , v_defaultScaled };
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			EnvTimeConverter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloatN11(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, float size/*=0*/, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::FloatN11Param p{ tempVal };
			p.SetN11Value(v_defaultScaled);
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			FloatN11Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0.5f, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloatN11WithCenter(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, float centerValN11, float size/*=0*/, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::FloatN11Param p{ tempVal };
			p.SetN11Value(v_defaultScaled);
			float defaultParamVal = p.GetRawParamValue();
			p.SetN11Value(centerValN11);
			float centerVal01 = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			FloatN11Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloat01(VstInt32 paramID, const char* label, M7::real_t v_default01, float centerVal01, float size = 0.0f, ImGuiKnobs::ModInfo modInfo = {}) {
			WaveSabreCore::M7::real_t tempVal = v_default01;
			M7::Float01Param p{ tempVal };
			float defaultParamVal = p.Get01Value();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Float01Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		enum class M7CurveRenderStyle
		{
			Rising,
			Falling,
		};

		void Maj7ImGuiParamCurve(VstInt32 paramID, const char* label, M7::real_t v_defaultN11, M7CurveRenderStyle style, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::CurveParam p{ tempVal };
			p.SetN11Value(v_defaultN11);
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			FloatN11Converter conv{ };
			int flags = (int)ImGuiKnobFlags_CustomInput;
			if (style == M7CurveRenderStyle::Rising) {
				flags |= ImGuiKnobFlags_InvertYCurve;
			}
			else if (style == M7CurveRenderStyle::Falling) {
				flags |= ImGuiKnobFlags_InvertYCurve | ImGuiKnobFlags_InvertXCurve;
			}
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0.5f, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_M7Curve, 0, flags, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamVolume(VstInt32 paramID, const char* label, const M7::VolumeParamConfig& cfg, M7::real_t v_defaultDB, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::VolumeParam p{ tempVal, cfg };
			p.SetDecibels(v_defaultDB);
			float defaultParamVal = p.GetRawParamValue();
			p.SetDecibels(0);
			float centerParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			M7VolumeConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		template<typename Tenum>
		void Maj7ImGuiEnvelopeGraphic(const char* label,
			Tenum delayTimeParamID)
		{
			static constexpr float gSegmentMaxWidth = 50;
			static constexpr float gMaxWidth = gSegmentMaxWidth * 6;
			static constexpr float innerHeight = 65; // excluding padding
			static constexpr float padding = 7;
			static constexpr float gLineThickness = 2.7f;
			ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_PlotLines);
			static constexpr float handleRadius = 3;
			static constexpr int circleSegments = 7;

			float delayWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DelayTime), 0, 1) * gSegmentMaxWidth;
			float attackWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackTime), 0, 1) * gSegmentMaxWidth;
			float holdWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::HoldTime), 0, 1) * gSegmentMaxWidth;
			float decayWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayTime), 0, 1) * gSegmentMaxWidth;
			float sustainWidth = gSegmentMaxWidth;
			float releaseWidth = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseTime), 0, 1) * gSegmentMaxWidth;

			float sustainLevel = M7::math::clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel), 0, 1);
			float sustainYOffset = innerHeight * (1.0f - sustainLevel);

			float attackCurveRaw;
			M7::CurveParam attackCurve{ attackCurveRaw };
			attackCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve));

			float decayCurveRaw;
			M7::CurveParam decayCurve{ decayCurveRaw };
			decayCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve));

			float releaseCurveRaw;
			M7::CurveParam releaseCurve{ releaseCurveRaw };
			releaseCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve));

			ImVec2 originalPos = ImGui::GetCursorScreenPos();
			ImVec2 p = originalPos;
			p.x += padding;
			p.y += padding;
			// D A H D S R
			ImDrawList* dl = ImGui::GetWindowDrawList();

			// background
			ImVec2 outerTL = originalPos;
			ImVec2 innerTL = { outerTL.x + padding, outerTL.y + padding };
			ImVec2 outerBottomLeft = { outerTL.x, outerTL.y + innerHeight + padding * 2 };
			ImVec2 outerBottomRight = { outerTL.x + gMaxWidth + padding * 2, outerTL.y + innerHeight + padding * 2 };
			ImVec2 innerBottomLeft = { innerTL.x, innerTL.y + innerHeight };
			ImVec2 innerBottomRight = { innerBottomLeft.x + gMaxWidth, innerBottomLeft.y };
			//dl->AddRectFilled(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg));
			//dl->AddRect(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_Frame));
			ImGui::RenderFrame(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f);
			//dl->AddRectFilled(innerTL, innerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg));

			//ImVec2 startPos = innerBottomLeft;// { innerTL.x, innerTL.y + innerHeight };
			ImVec2 delayStart = innerBottomLeft;
			ImVec2 delayEnd = { delayStart.x + delayWidth, delayStart.y };
			dl->AddLine(delayStart, delayEnd, lineColor, gLineThickness);
			//dl->PathLineTo(delayStart);
			//dl->PathLineTo(delayEnd); // delay flat line

			ImVec2 attackEnd = { delayEnd.x + attackWidth, innerTL.y };
			AddCurveToPath(dl, delayEnd, { attackEnd.x - delayEnd.x, attackEnd.y - delayEnd.y }, false, false, attackCurve, lineColor, gLineThickness);

			ImVec2 holdEnd = { attackEnd.x + holdWidth, attackEnd.y };
			//dl->PathLineTo(holdEnd);
			dl->AddLine(attackEnd, holdEnd, lineColor, gLineThickness);

			ImVec2 decayEnd = { holdEnd.x + decayWidth, innerTL.y + sustainYOffset };
			AddCurveToPath(dl, holdEnd, { decayEnd.x - holdEnd.x, decayEnd.y - holdEnd.y }, true, true, decayCurve, lineColor, gLineThickness);

			ImVec2 sustainEnd = { decayEnd.x + sustainWidth, decayEnd.y };
			//dl->PathLineTo(sustainEnd); // sustain flat line
			dl->AddLine(decayEnd, sustainEnd, lineColor, gLineThickness);

			ImVec2 releaseEnd = { sustainEnd.x + releaseWidth, innerBottomLeft.y };
			AddCurveToPath(dl, sustainEnd, { releaseEnd.x - sustainEnd.x, releaseEnd.y - sustainEnd.y }, true, true, releaseCurve, lineColor, gLineThickness);

			dl->AddCircleFilled(delayStart, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(delayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(attackEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(holdEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(decayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(sustainEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(releaseEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);

			//auto pathCopy = dl->_Path;
			//dl->PathLineTo(delayStart);
			//dl->PathFillConvex(ImGui::GetColorU32(ImGuiCol_Button));

			//dl->_Path = pathCopy;
			//dl->PathStroke(ImGui::GetColorU32(ImGuiCol_PlotHistogram), ImDrawFlags_None, gThickness);

			ImGui::Dummy({ outerBottomRight.x - originalPos.x, outerBottomRight.y - originalPos.y });
		}


		virtual bool onWheel(float distance) {
			return false;
		}		
		virtual bool setKnobMode(VstInt32 val) { 
			return false;
		}

		// these are called in response to effEditKeyDown/Up, and according to some online sources
		// they are not well/consistently supported, maybe even mac-specific, and shouldn't be used. we will instead use native processing.
		virtual bool onKeyDown(VstKeyCode& keyCode) override {
			return false; // important to return false, so the host doesn't swallow the message.
		}
		virtual bool onKeyUp(VstKeyCode& keyCode) override {
		//	if (keyCode.character > 0) {
		//		ImGui::GetIO().AddInputCharacterUTF16((unsigned short)keyCode.character);
		//		return true;
		//	}
			return false; // important to return false, so the host doesn't swallow the message.
		}

	protected:

		VstKeyCode mLastKeyDown = { 0 };
		VstKeyCode mLastKeyUp = { 0 };

		VstPlug* GetEffectX() {
			return static_cast<VstPlug*>(this->effect);
		}

		LPDIRECT3D9              g_pD3D = NULL;
		LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
		D3DPRESENT_PARAMETERS    g_d3dpp = {};

		bool CreateDeviceD3D(HWND hWnd);

		void CleanupDeviceD3D();

		void ResetDevice();

		HWND mParentWindow = nullptr;
		HWND mCurrentWindow = nullptr;

		ImGuiContext* mImGuiContext = nullptr;

		struct GuiContextRestorer
		{
			//cc::LogScope mls;
			ImGuiContext* mPrevContext = nullptr;
			bool pushed = false;
			const char* mWhy;
			GuiContextRestorer(ImGuiContext* newContext, const char* why) : mWhy(why)/*, mls(why)*/ {
				mPrevContext = ImGui::GetCurrentContext();
				//cc::log("pushing context; %p  ==>  %p", mPrevContext);
				if (mPrevContext == newContext) {
					//cc::log("  -> no change needed");
					return; // no change needed.
				}
				pushed = true;
				ImGui::SetCurrentContext(newContext);
			}
			~GuiContextRestorer()
			{
				auto current = ImGui::GetCurrentContext();
				if (!pushed) {
					//cc::log("popping context; ignoring because it was not pushed. mPrevContext=%p, currentcontext=%p", mPrevContext, current);
					return;
				}
				if (current == mPrevContext) {
					//cc::log("popping context; no need to change context because it's already correct. mPrevContext=%p, currentcontext=%p", mPrevContext, current);
					return;
				}
				//cc::log("popping context; currentcontext=%p  ==>  mPrevContext=%p", current, mPrevContext);
				ImGui::SetCurrentContext(mPrevContext);
			}
		};

		enum class VUMeterFlags
		{
			None = 0,
			InputIsLinear = 1,
			AttenuationMode = 4,
			LevelMode = 8,
			NoText = 16,
			NoForeground = 32,
			FillAttenuation = 64,
		};

		struct VUMeterColors
		{
			ImColor background;
			ImColor foreground;
			ImColor text;
			ImColor tick;
			ImColor clipTick;
			ImColor peak;
		};

		// rms level may be nullptr to not graph it.
		// peak level may be nullptr to not graph it.
		void VUMeter(ImVec2 size, float* rmsLevel, float* peakLevel, VUMeterFlags flags)
		{
			VUMeterColors colors;
			if ((int)flags & (int)VUMeterFlags::LevelMode)
			{
				colors.background = ColorFromHTML("2b321c");
				colors.foreground = ColorFromHTML("6c9b0a");
				colors.text = ColorFromHTML("ffffff");
				colors.tick = ColorFromHTML("00ffff");
				colors.clipTick = ColorFromHTML("ff0000");
				colors.peak = ColorFromHTML("ffff00");

			} else if ((int)flags & (int)VUMeterFlags::AttenuationMode)
			{
				colors.background = ColorFromHTML("462e2e");
				colors.foreground = ColorFromHTML("ed4d4d");
				colors.text = ColorFromHTML("ffffff");
				colors.tick = ColorFromHTML("00ffff");
				colors.clipTick = colors.foreground;// don't bother with clipping for attenuation ColorFromHTML("ff0000");
				colors.peak = ColorFromHTML("ffff00");
			}
			colors.text.Value.w = 0.25f;
			colors.tick.Value.w = 0.35f;
			colors.clipTick.Value.w = 0.8f;

			float rmsDB = 0;
			bool clip = false;
			if (rmsLevel) {
				if ((int)flags & (int)VUMeterFlags::InputIsLinear) {
					rmsDB = M7::math::LinearToDecibels(::fabsf(*rmsLevel));
				}
				else {
					rmsDB = (*rmsLevel);
				}
				if (rmsDB >= 0) clip = true;
			}

			float peakDb = 0;
			if (peakLevel) {
				if ((int)flags & (int)VUMeterFlags::InputIsLinear) {
					peakDb = M7::math::LinearToDecibels(::fabsf(*peakLevel));
				}
				else {
					peakDb = (*peakLevel);
				}

				if (peakDb >= 0) clip = true;
			}

			ImRect bb;
			bb.Min = ImGui::GetCursorScreenPos();
			bb.Max = bb.Min + size;
			ImGui::RenderFrame(bb.Min, bb.Max, colors.background);

			auto DbToY = [&](float db) {
				// let's show a range of -60 to 0 db.
				float x = (db + 60) / 60;
				x = Clamp01(x);
				return M7::math::lerp(bb.Max.y, bb.Min.y, x);
			};

			auto* dl = ImGui::GetWindowDrawList();
			float levelY = DbToY(rmsDB);

			ImRect forebb = bb;
			if ((int)flags & (int)VUMeterFlags::LevelMode)
			{
				forebb.Min.y = levelY;
			}
			if ((int)flags & (int)VUMeterFlags::AttenuationMode)
			{
				forebb.Max.y = levelY;
			}

			if (!((int)flags & (int)VUMeterFlags::NoForeground)) {
				if (rmsLevel) {
					dl->AddRectFilled(forebb.Min, forebb.Max, colors.foreground);
				}
			}

			// draw thumb
			ImRect threshbb = bb;
			threshbb.Min.y = threshbb.Max.y = levelY;
			float tickHeight = clip ? 40.0f : 4.0f;
			threshbb.Max.y += tickHeight;
			dl->AddRectFilled(threshbb.Min, threshbb.Max, clip ? colors.clipTick : colors.tick);

			// draw peak
			if (peakLevel != nullptr) {
				float peakY = DbToY(peakDb);
				ImRect threshbb = bb;
				threshbb.Min.y = threshbb.Max.y = peakY;
				float tickHeight = clip ? 40.0f : 4.0f;
				threshbb.Max.y += tickHeight;
				dl->AddRectFilled(threshbb.Min, threshbb.Max, colors.peak);
			}

			// draw plot lines
			auto drawTick = [&](float tickdb, const char* txt) {
				ImRect tickbb = forebb;
				tickbb.Min.y = DbToY(tickdb);
				tickbb.Max.y = tickbb.Min.y;
				dl->AddLine(tickbb.Min, tickbb.Max, colors.text);
				if (!((int)flags & (int)VUMeterFlags::NoText)) {
					dl->AddText({ tickbb.Min.x, tickbb.Min.y }, colors.text, txt);
				}
			};

			drawTick(-5, "3db");
			drawTick(-10, "6db");
			drawTick(-15, "12db");
			drawTick(-20, "18db");
			drawTick(-30, "30db");
			drawTick(-40, "40db");
			drawTick(-50, "50db");

			ImGui::Dummy(size);
		}

		void VUMeter(float* rmsLevel, float* peakLevel, VUMeterFlags flags)
		{
			VUMeter(ImVec2(44, 300), rmsLevel, peakLevel, flags);
		}



		GuiContextRestorer PushMyImGuiContext(const char *why) {
			return { mImGuiContext, why };
		}

		void ImguiPresent();

		static LRESULT WINAPI gWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		ERect mDefaultRect = { 0 };
		bool showingDemo = false;
	};


}

#endif
