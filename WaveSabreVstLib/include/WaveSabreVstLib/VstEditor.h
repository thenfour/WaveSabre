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
	static constexpr double gNormalKnobSpeed = 0.0015;
	static constexpr double gSlowKnobSpeed = 0.000003;

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

	struct ButtonColorSpec {
		const char* selectedColor = nullptr;
		const char* notSelectedColor = nullptr;
		const char* hoveredColor = nullptr;
	};

	inline bool ToggleButton(bool* value, const char* label, ImVec2 size = {}, const ButtonColorSpec& cfg = {}) {
		bool ret = false;
		int colorsPushed = 0;
		if (cfg.hoveredColor) {
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.hoveredColor));
			colorsPushed++;
		}
		if (*value) {
			if (cfg.selectedColor) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
				colorsPushed += 2;
			}
		}
		else {
			if (cfg.notSelectedColor) {
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
				colorsPushed += 2;
			}
		}

		if (ImGui::Button(label, size)) {
			*value = !(*value);
			ret = true;
		}

		ImGui::PopStyleColor(colorsPushed);
		return ret;
	}

	//struct EnvTimeConverter : ImGuiKnobs::IValueConverter
	//{
	//	float mBacking;
	//	WaveSabreCore::M7::EnvTimeParam mParam;

	//	EnvTimeConverter() :
	//		mParam(mBacking, 0)
	//	{
	//	}

	//	virtual std::string ParamToDisplayString(double param, void* capture) override {
	//		mParam.SetParamValue((float)param);
	//		char s[100] = { 0 };
	//		M7::real_t ms = mParam.GetMilliseconds();
	//		if (ms > 2000)
	//		{
	//			sprintf_s(s, "%0.1f s", ms / 1000);
	//		}
	//		else if (ms > 1000) {
	//			sprintf_s(s, "%0.2f s", ms / 1000);
	//		}
	//		else {
	//			sprintf_s(s, "%0.2f ms", ms);
	//		}
	//		return s;
	//	}

	//	virtual double DisplayValueToParam(double value, void* capture) {
	//		//mParam.SetRangedValue((float)value);
	//		//return (double)mParam.GetRawParamValue();
	//		return 0;
	//	}
	//};

	struct PowCurvedConverter : ImGuiKnobs::IValueConverter
	{
		float mBacking;
		M7::ParamAccessor mParam{ &mBacking, 0 };
		const M7::PowCurvedParamCfg& mConfig;

		PowCurvedConverter(const M7::PowCurvedParamCfg& cfg) : mConfig(cfg)
		{}

		virtual std::string ParamToDisplayString(double param, void* capture) override {
			mBacking = (float)param;
			char s[100] = { 0 };
			M7::real_t ms = mParam.GetPowCurvedValue(0, mConfig, 0);
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

		//virtual double DisplayValueToParam(double value, void* capture) {
		//	//mParam.SetRangedValue((float)value);
		//	//return (double)mParam.GetRawParamValue();
		//	return 0;
		//}
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

		//virtual double DisplayValueToParam(double value, void* capture) {
		//	return 0;
		//}
	};

	struct M7VolumeConverter : ImGuiKnobs::IValueConverter
	{
		float mBacking;
		M7::ParamAccessor mParam{ &mBacking, 0 };
		M7::VolumeParamConfig mCfg;
		//WaveSabreCore::M7::VolumeParam mParam;

		M7VolumeConverter(const M7::VolumeParamConfig& cfg) :
			mCfg(cfg)
		{
		}

		virtual std::string ParamToDisplayString(double param, void* capture) override {
			mBacking = (float)param;
			//mParam.SetParamValue((float)param);
			char s[100] = { 0 };
			
			if (mParam.IsSilentVolume(0, mCfg)) {
				return "-inf";
			}
			float db = mParam.GetDecibels(0, mCfg);
			sprintf_s(s, "%c%0.2fdB", db < 0 ? '-' : '+', ::fabsf(db));
			return s;
		}

	};



	struct ParamExplorer {

		float mPowKnobMin = 0;
		float mPowKnobMax = 100;
		float mPowKnobK = 1;
		float mPowKnobValue = 0;

		float mDivKnobMin = 0;
		float mDivKnobMax = 100;
		float mDivKnobK = 1;
		float mDivKnobValue = 0;

		float mEnvTimeValue = 0;

		float mVolumeMaxDB = 6;
		float mVolumeParamValue = 0;


		struct TransferSeries {
			std::function<float(float x)> mTransferFn;
			ImColor mForegroundColor;
			float mThickness;
		};

		void RenderTransferCurve(ImVec2 size, const std::vector<TransferSeries>& serieses)
		{
			auto* dl = ImGui::GetWindowDrawList();
			ImRect bb;
			bb.Min = ImGui::GetCursorScreenPos();
			bb.Max = bb.Min + size;
			ImGui::RenderFrame(bb.Min, bb.Max, ColorFromHTML("222222"));

			auto LinToY = [&](float lin) {
				float t = M7::math::clamp01(lin);
				return M7::math::lerp(bb.Max.y, bb.Min.y, t);
			};

			auto LinToX = [&](float lin) {
				float t = M7::math::clamp01(lin);
				return M7::math::lerp(bb.Min.x, bb.Max.x, t);
			};

			static constexpr int segmentCount = 40;
			for (auto& series : serieses)
			{
				std::vector<ImVec2> points;
				for (int i = 0; i < segmentCount; ++i)
				{
					float tLin = float(i) / (segmentCount - 1); // touch 0 and 1
					float yLin = series.mTransferFn(tLin);
					points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
				}
				dl->AddPolyline(points.data(), (int)points.size(), series.mForegroundColor, 0, series.mThickness);
			}

			ImGui::Dummy(size);
		} // void RenderTransferCurve()

		void Render()
		{
			ImGui::SetNextItemWidth(160);
			ImGui::InputFloat("min", &mPowKnobMin, 0.01f, 1.0f);
			ImGui::SetNextItemWidth(160);
			ImGui::SameLine(); ImGui::InputFloat("max", &mPowKnobMax, 0.01f, 1.0f, "%.3f");
			ImGui::SetNextItemWidth(160);
			ImGui::SameLine(); ImGui::SliderFloat("k", &mPowKnobK, 0.001f, 20);
			M7::PowCurvedParamCfg powCurvedCfg{ mPowKnobMin, mPowKnobMax, mPowKnobK };
			PowCurvedConverter powConv{ powCurvedCfg };
			ImGuiKnobs::Knob("Pow curved", &mPowKnobValue, 0, 1, 0, 0, {}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &powConv, this);
			ImGui::SameLine(); ImGui::BeginGroup();
			ImGui::Text("@ .25 : %f", powCurvedCfg.Param01ToValue(0.25f));
			ImGui::Text("@ .50 : %f", powCurvedCfg.Param01ToValue(0.5f));
			ImGui::Text("@ .75 : %f", powCurvedCfg.Param01ToValue(0.75f));
			ImGui::EndGroup();
			ImColor powColor = ColorFromHTML("ffff00", 0.7f);

			auto transferPow01 = [&](float x01) {
				float bigval = powCurvedCfg.Param01ToValue(x01);
				return M7::math::lerp_rev(mPowKnobMin, mPowKnobMax, bigval);
			};

			ImGui::SetNextItemWidth(160);
			ImGui::InputFloat("min##div", &mDivKnobMin, 0.01f, 1.0f);
			ImGui::SetNextItemWidth(160);
			ImGui::SameLine(); ImGui::InputFloat("max##div", &mDivKnobMax, 0.01f, 1.0f, "%.3f");
			ImGui::SetNextItemWidth(160);
			ImGui::SameLine(); ImGui::SliderFloat("k##div", &mDivKnobK, 1.001f, 2);
			M7::DivCurvedParamCfg divCurvedCfg{ mDivKnobMin, mDivKnobMax, mDivKnobK };
			DivCurvedConverter divConv{ divCurvedCfg };
			ImGuiKnobs::Knob("Div curved##div", &mDivKnobValue, 0, 1, 0, 0, {}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &divConv, this);
			ImGui::SameLine(); ImGui::BeginGroup();
			ImGui::Text("@ .25 : %f", divCurvedCfg.Param01ToValue(0.25f));
			ImGui::Text("@ .50 : %f", divCurvedCfg.Param01ToValue(0.5f));
			ImGui::Text("@ .75 : %f", divCurvedCfg.Param01ToValue(0.75f));
			ImGui::EndGroup();
			ImColor divColor = ColorFromHTML("ff8800", 0.7f);

			auto transferDiv01 = [&](float x01) {
				float bigval = divCurvedCfg.Param01ToValue(x01);
				return M7::math::lerp_rev(mDivKnobMin, mDivKnobMax, bigval);
			};

			//env time
			//EnvTimeConverter envConv{ };
			//ImGuiKnobs::Knob("Env time", &mEnvTimeValue, 0, 1, 0, 0, {}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &envConv, this);
			//ImGui::SameLine(); ImGui::BeginGroup();
			//float envT = 0.25f;
			//M7::ParamAccessor envPA{ &envT , 0 };
			//ImGui::Text("@ .25 : %f", envPA.GetEnvTimeMilliseconds(0, 0));
			//envT = 0.5f;
			//ImGui::Text("@ .50 : %f", envPA.GetEnvTimeMilliseconds(0, 0));
			//envT = 0.75f;
			//ImGui::Text("@ .75 : %f", envPA.GetEnvTimeMilliseconds(0, 0));
			//ImGui::EndGroup();
			//ImColor envTimeColor = ColorFromHTML("00ff00", 0.7f);

			//auto transferEnvTime = [&](float x01) {
			//	envT = x01;
			//	float bigval = envPA.GetEnvTimeMilliseconds(0, 0);
			//	return M7::math::lerp_rev(M7::EnvTimeParam::gMinRealVal, M7::EnvTimeParam::gMaxRealVal, bigval);
			//};

			//Maj7ImGuiParamVolume
			{
				ImGui::SetNextItemWidth(160);
				//ImGui::InputFloat("max dB##div", &mVolumeMaxDB, 0.01f, 1.0f, "%.3f");
				ImGui::SliderFloat("max dB##vol", &mVolumeMaxDB, 0, 36);

				M7::VolumeParamConfig cfg{
					M7::math::DecibelsToLinear(mVolumeMaxDB),
					mVolumeMaxDB,
				};

				M7VolumeConverter conv{ cfg };
				ImGuiKnobs::Knob("M7Volume", &mVolumeParamValue, 0, 1, 0, 0, {}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
			}
			ImColor volumeColor = ColorFromHTML("9900ff", 0.7f);

			auto transferVolume = [&](float x01) {
				M7::VolumeParamConfig cfg{
					M7::math::DecibelsToLinear(mVolumeMaxDB),
					mVolumeMaxDB,
				};
				float t = x01;
				M7::ParamAccessor p{ &t, 0 };
				float bigval = p.GetLinearVolume(0, cfg, 0);
				return M7::math::lerp_rev(M7::gMinGainLinear, mVolumeMaxDB, bigval);
			};

			RenderTransferCurve({ 300,300 }, {
				{ [&](float x01) { return x01; }, ColorFromHTML("444444", 0.5f), 1.0f},
				{ [&](float x01) { return transferPow01(x01); }, powColor, 2.0f},
				{ [&](float x01) { return transferDiv01(x01); }, divColor, 2.0f},
				//{ [&](float x01) { return transferEnvTime(x01); }, envTimeColor, 2.0f},
				{ [&](float x01) { return transferVolume(x01); }, volumeColor, 2.0f},
				});
		}

	};

	struct HistoryViewSeriesConfig
	{
		ImColor mLineColor;// = ColorFromHTML("ff8080", 0.7f);
		float mLineThickness;
	};

	struct HistoryViewSeries
	{
		//HistoryEnvFollower mFollower; // actual sample values are not helpful; we need a peak env follower to look nice.
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

		void Render(const std::array<HistoryViewSeriesConfig, TSeriesCount>& cfg, const std::array<float, TSeriesCount>& linValues)
		{
			ImRect bb;
			bb.Min = ImGui::GetCursorScreenPos();
			bb.Max = bb.Min + gHistViewSize;

			for (size_t i = 0; i < TSeriesCount; ++i) {

				//float lin = mSeries[i].mFollower.ProcessSample(M7::math::DecibelsToLinear(dbValues[i]));
				float lin = linValues[i];

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


	///////////////////////////////////////////////////////////////////////////////////////////////////
	struct FrequencyResponseRendererFilter {
		const ImColor thumbColor;
		const BiquadFilter* filter;
	};

	template<size_t TFilterCount, size_t TParamCount>
	struct FrequencyResponseRendererConfig {
		const ImColor backgroundColor;
		const ImColor lineColor;
		const float thumbRadius;
		const std::array<FrequencyResponseRendererFilter, TFilterCount> filters;
		float mParamCacheCopy[TParamCount];
	};

	template<int Twidth, int Theight, int TsegmentCount, size_t TFilterCount, size_t TParamCount>
	struct FrequencyResponseRenderer
	{
		struct ThumbRenderInfo {
			ImColor color;
			ImVec2 point;
		};
		// the response graph is extremely crude; todo:
		// - add the user-selected points to the points list explicitly, to give better looking peaks. then you could reduce # of points.
		// - respect 'thru' member.
		// - display info about freq range and amplitude
		// - adjust amplitude

		static constexpr ImVec2 gSize = { Twidth, Theight };
		static constexpr int gSegmentCount = TsegmentCount;

		const float gMinDB = -12;
		const float gMaxDB = 12;

		// because params change as a result of this immediate gui, we need at least 1 full frame of lag to catch param changes correctly.
		// so keep processing multiple frames until things settle. in the meantime, force recalculating.
		int mAdditionalForceCalcFrames = 0;
		ImVec2 mPoints[gSegmentCount]; // actual pixel values.
		std::vector< ThumbRenderInfo> mThumbs;

		// the param cache that points have been calculated on. this way we can only recalc the freq response when params change.
		float mParamCacheCache[TParamCount] = { 0 };

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

		float FreqToX(float hz, ImRect& bb) {
			float underlyingValue = 0;
			//float ktdummy = 0;
			M7::ParamAccessor p{ &underlyingValue, 0 };
			p.SetFrequencyAssumingNoKeytracking(0, M7::gFilterFreqConfig, hz);
			return M7::math::lerp(bb.Min.x, bb.Max.x, underlyingValue);
			//M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig };
			//return 0;
		}

		float DBToY(float dB, ImRect& bb) {
			float t01 = M7::math::lerp_rev(gMinDB, gMaxDB, dB);
			t01 = M7::math::clamp01(t01);
			return M7::math::lerp(bb.Max.y, bb.Min.y, t01);
			//return bb.Max.y - bb.GetHeight() * M7::math::clamp01(magLin);
		}

		void CalculatePoints(const FrequencyResponseRendererConfig<TFilterCount, TParamCount>& cfg, ImRect& bb) {
			//float underlyingValue = 0;
			float ktdummy = 0;
			//M7::FrequencyParam param{ underlyingValue, ktdummy, M7::gFilterFreqConfig };
			M7::QuickParam param{ 0, M7::gFilterFreqConfig };

			renderSerial++;

			mThumbs.clear();

			for (auto& f : cfg.filters) {
				if (!f.filter) continue; // nullptr values are valid and used when a filter is bypassed.
				//if (f.filter->thru) continue;
				float freq = f.filter->freq;
				float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
				float magdB = M7::math::LinearToDecibels(magLin);
				mThumbs.push_back({
					f.thumbColor,
					{ FreqToX(freq, bb), DBToY(magdB, bb) }
				});
			}

			for (int iseg = 0; iseg < gSegmentCount; ++iseg) {
				param.SetRawValue(float(iseg) / gSegmentCount);
				float freq = param.GetFrequency();
				float magdB = 0;

				for (auto& f : cfg.filters) {
					if (!f.filter) continue; // nullptr values are valid and used when a filter is bypassed.
					//if (f.filter->thru) continue;
					float magLin = BiquadMagnitudeForFrequency(*(f.filter), freq);
					magdB += M7::math::LinearToDecibels(magLin);
				}

				//float magLin = M7::math::DecibelsToLinear(magdB) / 4; // /4 to basically give a 12db range of display.

				mPoints[iseg] = ImVec2(
					FreqToX(freq, bb),// (bb.Min.x + iseg * bb.GetWidth() / gSegmentCount),
					DBToY(magdB, bb)
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

			float unityY = std::round(DBToY(0, bb)); // round for crisp line.
			dl->AddLine({bb.Min.x, unityY }, {bb.Max.x, unityY }, ColorFromHTML("444444"), 1.0f);

			dl->AddPolyline(mPoints, gSegmentCount, cfg.lineColor, 0, 2.0f);

			for (auto& th : mThumbs) {
				dl->AddCircleFilled(th.point, cfg.thumbRadius, th.color);
			}

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
		//VibratoFreq,
		Frequency,
		//FilterQ,
		//Db,
	};


	inline void CompressorTransferCurve(const MonoCompressor& c, ImVec2 size, AnalysisStream(&detectorAnalysis)[2]) {
		ImRect bb;
		bb.Min = ImGui::GetCursorScreenPos();
		bb.Max = bb.Min + size;

		ImColor backgroundColor = ColorFromHTML("222222", 1.0f);
		ImColor curveColor = ColorFromHTML("999999", 1.0f);
		ImColor gridColor = ColorFromHTML("444444", 1.0f);
		ImColor detectorColorAtt = ColorFromHTML("ed4d4d", 0.9f);
		ImColor detectorColor = ColorFromHTML("eeeeee", 0.9f);

		ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

		static constexpr int segmentCount = 33;
		std::vector<ImVec2> points;

		auto dbToY = [&](float dB) {
			float lin = dB / gHistoryViewMinDB;
			float t01 = M7::math::clamp01(1.0f - lin);
			return bb.Max.y - t01 * size.y;
		};

		auto dbToX = [&](float dB) {
			float lin = dB / gHistoryViewMinDB;
			float t01 = M7::math::clamp01(1.0f - lin);
			return bb.Min.x + t01 * size.y;
		};

		for (int iSeg = 0; iSeg < segmentCount; ++iSeg) {
			float inLin = float(iSeg) / (segmentCount - 1); // touch 0 and 1
			float dbIn = (1.0f - inLin) * gHistoryViewMinDB; // db from gHistoryViewMinDB to 0.
			//float dbAttenOut = mpMaj7Comp->mComp[0].TransferDecibels(dbIn);// / 5;
			float dbAttenOut = c.TransferDecibels(dbIn);// / 5;
			float outDB = dbIn - dbAttenOut;
			points.push_back(ImVec2{ bb.Min.x + inLin * size.x, dbToY(outDB) });
		}

		auto* dl = ImGui::GetWindowDrawList();

		// linear guideline
		dl->AddLine({ bb.Min.x, bb.Max.y }, { bb.Max.x, bb.Min.y }, gridColor, 1.0f);

		// threshold guideline
		float threshY = dbToY(c.mThreshold);
		dl->AddLine({ bb.Min.x, threshY }, { bb.Max.x, threshY }, gridColor, 1.0f);

		dl->AddPolyline(points.data(), (int)points.size(), curveColor, 0, 2.0f);

		// detector indicator dot
		double detectorLevel = std::max(detectorAnalysis[0].mCurrentPeak, detectorAnalysis[1].mCurrentPeak);// std::max(mpMaj7Comp->mComp[0].mPostDetector, mpMaj7Comp->mComp[1].mPostDetector);
		float detectorDB = M7::math::LinearToDecibels(float(detectorLevel));

		float detAtten = c.TransferDecibels(detectorDB);
		float detOutDB = detectorDB - detAtten;

		if (detAtten > 0.00001) {
			dl->AddCircleFilled({ dbToX(detectorDB), dbToY(detOutDB) }, 6.0f, detectorColorAtt);
		}
		else {
			dl->AddCircleFilled({ dbToX(detectorDB), dbToY(detOutDB) }, 3.0f, detectorColor);
		}

		ImGui::Dummy(size);
	}










	struct VUMeterColors
	{
		ImColor background;
		ImColor foregroundRMS;
		ImColor foregroundPeak;
		ImColor backgroundOverUnity;
		ImColor foregroundOverUnity;
		ImColor text;
		ImColor tick;
		ImColor clipTick;
		ImColor peak;
	};

	struct VUMeterTick {
		float dBvalue;
		const char* caption; // may be nullptr to not display text.
	};

	enum VUMeterLevelMode {
		Audio,
		Attenuation,
		Disabled,
	};

	enum VUMeterUnits {
		dB,
		Linear,
	};

	struct VUMeterConfig {
		ImVec2 size;
		VUMeterLevelMode levelMode;
		VUMeterUnits units;
		float minDB;
		float maxDB;
		std::vector<VUMeterTick> ticks;
	};

	// rms level may be nullptr to not graph it.
	// peak level may be nullptr to not graph it.
	// clipindicator same.
	// return true if clicked.
	inline bool VUMeter(const char* id, const double* rmsLevel, const double* peakLevel, const double* heldPeakLevel, const bool* clipIndicator, bool allowTickText, const VUMeterConfig& cfg)
	{
		VUMeterColors colors;
		switch (cfg.levelMode) {
		case VUMeterLevelMode::Audio:
		{
			colors.background = ColorFromHTML("2b321c");
			colors.foregroundRMS = ColorFromHTML("6c9b0a");
			colors.foregroundPeak = ColorFromHTML("4a6332");

			colors.backgroundOverUnity = ColorFromHTML("440000");
			colors.foregroundOverUnity = ColorFromHTML("cccc00");

			colors.text = ColorFromHTML("ffffff");
			colors.tick = ColorFromHTML("00ffff");
			colors.clipTick = ColorFromHTML("ff0000");
			colors.peak = ColorFromHTML("6c9b0a", 0.8f);
			break;

		}
		case VUMeterLevelMode::Attenuation:
		{
			colors.background = ColorFromHTML("462e2e");
			colors.foregroundRMS = ColorFromHTML("ed4d4d");
			colors.foregroundPeak = ColorFromHTML("4a6332");

			colors.backgroundOverUnity = ColorFromHTML("440000");
			colors.foregroundOverUnity = ColorFromHTML("cccc00");

			colors.text = ColorFromHTML("ffffff");
			colors.tick = ColorFromHTML("00ffff");
			colors.clipTick = colors.foregroundRMS;// don't bother with clipping for attenuation ColorFromHTML("ff0000");
			colors.peak = ColorFromHTML("6c9b0a", 0.8f);
			break;
		}
		default:
		case VUMeterLevelMode::Disabled:
		{
			colors.background = ColorFromHTML("222222");
			colors.foregroundRMS = ColorFromHTML("eeeeee", 0);
			colors.foregroundPeak = ColorFromHTML("666666", 0);

			colors.backgroundOverUnity = ColorFromHTML("666666", 0);
			colors.foregroundOverUnity = ColorFromHTML("777777", 0);

			colors.text = ColorFromHTML("555555", .5);
			colors.tick = ColorFromHTML("666666", 0);
			colors.clipTick = colors.foregroundRMS;// don't bother with clipping for attenuation ColorFromHTML("ff0000");
			colors.peak = ColorFromHTML("cccccc", 0);
			break;
		}
		}



		colors.text.Value.w = 0.33f;
		colors.tick.Value.w = 0.33f;

		float rmsDB = 0;
		if (rmsLevel) {
			if (cfg.units == VUMeterUnits::Linear) {
				rmsDB = M7::math::LinearToDecibels(::fabsf(float(* rmsLevel)));
			}
			else {
				rmsDB = float(*rmsLevel);
			}
		}

		float peakDb = 0;
		if (peakLevel) {
			if (cfg.units == VUMeterUnits::Linear) {
				peakDb = M7::math::LinearToDecibels(::fabsf(float(*peakLevel)));
			}
			else {
				peakDb = float(*peakLevel);
			}
		}

		float heldPeakDb = 0;
		if (heldPeakLevel) {
			if (cfg.units == VUMeterUnits::Linear) {
				heldPeakDb = M7::math::LinearToDecibels(::fabsf(float(*heldPeakLevel)));
			}
			else {
				heldPeakDb = float(*heldPeakLevel);
			}
		}

		ImRect bb;
		bb.Min = ImGui::GetCursorScreenPos();
		bb.Max = bb.Min + cfg.size;

		auto DbToY = [&](float db) {
			// let's show a range of -60 to 0 db.
			//float x = (db + 60) / 60;
			float x = M7::math::lerp_rev(cfg.minDB, cfg.maxDB, db);
			x = M7::math::clamp01(x);
			return M7::math::lerp(bb.Max.y, bb.Min.y, x);
		};

		auto* dl = ImGui::GetWindowDrawList();

		assert(cfg.minDB < cfg.maxDB); // simplifies logic.

		auto RenderDBRegion = [&](float dB1, float dB2, ImColor bgUnderUnity, ImColor bgOverUnity) {
			if (dB1 > dB2) std::swap(dB1, dB2); // make sure in order to simplify logic

			// find the intersection of the desired region and the renderable region.
			float cmin = std::max(dB1, cfg.minDB);
			float cmax = std::min(dB2, cfg.maxDB);
			if (cmin >= cmax) {
				return; // nothing to render.
			}

			// break render rgn into positive & negative dB regions.
			if (cmax <= 0) {// Entire range is negative; render cmin,cmax in under-unity style
				dl->AddRectFilled({ bb.Min.x, DbToY(cmin) }, { bb.Max.x, DbToY(cmax) }, bgUnderUnity);
			}
			else if (cmin >= 0) { // Entire range is non-negative, render cmin,cmax in over-unity style
				dl->AddRectFilled({ bb.Min.x, DbToY(cmin) }, { bb.Max.x, DbToY(cmax) }, bgOverUnity);
			}
			else {
				// Range spans across 0, split into two regions
				dl->AddRectFilled({ bb.Min.x, DbToY(cmin) }, { bb.Max.x, DbToY(0) }, bgUnderUnity);
				dl->AddRectFilled({ bb.Min.x, DbToY(0) }, { bb.Max.x, DbToY(cmax) }, bgOverUnity);
			}
		};

		RenderDBRegion(cfg.minDB, cfg.maxDB, colors.background, colors.backgroundOverUnity);

		float levelY = DbToY(rmsDB);

		// draw the peak bar
		if (peakLevel) {
			float fgBaseDB = cfg.minDB;
			if (cfg.levelMode == VUMeterLevelMode::Attenuation) fgBaseDB = 0;
			RenderDBRegion(peakDb, fgBaseDB, colors.foregroundPeak, colors.foregroundOverUnity);
		}

		// draw the RMS bar
		if (rmsLevel) {
			float fgBaseDB = cfg.minDB;
			if (cfg.levelMode == VUMeterLevelMode::Attenuation) fgBaseDB = 0;
			RenderDBRegion(rmsDB, fgBaseDB, colors.foregroundRMS, colors.foregroundOverUnity);
		}

		//// draw thumb (a little line separating foreground from background)
		//ImRect threshbb = bb;
		//threshbb.Min.y = threshbb.Max.y = levelY;
		//float tickHeight = 3.0f;
		//threshbb.Max.y += tickHeight;
		//dl->AddRectFilled(threshbb.Min, threshbb.Max, colors.tick);

		// draw clip
		if (clipIndicator && *clipIndicator && (cfg.levelMode != VUMeterLevelMode::Attenuation)) {
			dl->AddRectFilled(bb.Min, { bb.Max.x, bb.Min.y + 8 }, colors.clipTick);
		}

		// draw held peak
		if (heldPeakLevel != nullptr) {
			float peakY = DbToY(heldPeakDb);
			ImRect threshbb = bb;
			threshbb.Min.y = threshbb.Max.y = peakY;
			float tickHeight = 2.0f;
			threshbb.Max.y += tickHeight;
			dl->AddRectFilled(threshbb.Min, threshbb.Max, colors.peak);
		}

		// draw plot lines
		auto drawTick = [&](float tickdb, const char* txt) {
			ImRect tickbb = bb;
			tickbb.Min.y = std::round(DbToY(tickdb)); // make crisp lines plz.
			tickbb.Max.y = tickbb.Min.y;
			dl->AddLine(tickbb.Min, tickbb.Max, colors.text);
			if (txt && allowTickText) {
				dl->AddText({ tickbb.Min.x, tickbb.Min.y }, colors.text, txt);
			}
		};

		for (auto& t : cfg.ticks) {
			drawTick(t.dBvalue, t.caption);
		}

		return ImGui::InvisibleButton(id, cfg.size);
	}

	inline void VUMeter(const char* id, AnalysisStream& a0, AnalysisStream& a1, ImVec2 size = { 30,300 })
	{
		static const std::vector<VUMeterTick> standardTickSet = {
				{-3.0f, "3db"},
				{-6.0f, "6db"},
				{-12.0f, "12db"},
				{-18.0f, "18db"},
				{-24.0f, "24db"},
				{-30.0f, "30db"},
				{-40.0f, nullptr},
				//{-50.0f, "50db"},
		};

		static const std::vector<VUMeterTick> smallTickSet = {
				{-10.0f, "10db"},
				{-20.0f, nullptr},
				{-30.0f, "30db"},
				{-40.0f, nullptr},
				//{-50.0f, "50db"},
		};

		const VUMeterConfig cfg = {
			size,
			VUMeterLevelMode::Audio,
			VUMeterUnits::Linear,
			-50, 6,
			smallTickSet
		};

		ImGui::PushID(id);
		if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
		{
			a0.Reset();
			a1.Reset();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, 0 });
		ImGui::SameLine();
		if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
		{
			a0.Reset();
			a1.Reset();
		}
		ImGui::PopID();
		ImGui::PopStyleVar();
	}

	inline void VUMeter(const char* id, AnalysisStream& a0, AnalysisStream& a1, const VUMeterConfig& cfg)
	{
		ImGui::PushID(id);
		if (VUMeter("VU L", &a0.mCurrentRMSValue, &a0.mCurrentPeak, &a0.mCurrentHeldPeak, &a0.mClipIndicator, true, cfg))
		{
			a0.Reset();
			a1.Reset();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 1, 0 });
		ImGui::SameLine();
		if (VUMeter("VU R", &a1.mCurrentRMSValue, &a1.mCurrentPeak, &a1.mCurrentHeldPeak, &a1.mClipIndicator, false, cfg))
		{
			a0.Reset();
			a1.Reset();
		}
		ImGui::PopID();
		ImGui::PopStyleVar();
	}











	class VstEditor : public AEffEditor
	{
	public:
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
				tabBarBB = ImRect(
					window->DC.CursorPos.x,
					window->DC.CursorPos.y,
					window->DC.CursorPos.x + (window->WorkRect.Max.x),
					window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
			}
			else {
				tabBarBB = ImRect(
					window->DC.CursorPos.x,
					window->DC.CursorPos.y,
					window->DC.CursorPos.x + width,
					window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
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

			// this value is too big; there's some huge space at the right of all tabs.
			float separator_max_x = tab_bar->BarRect.Max.x + M7::math::floor(window->WindowPadding.x * 0.5f);

			//auto cursor = ImGui::GetCursorScreenPos();
			//float separator_max_x = cursor.x;// +M7::math::floor(window->WindowPadding.x * 0.5f);

			ImRect body_bb;
			body_bb.Min = { separator_min_x, y };
			body_bb.Max = { separator_max_x, window->DC.CursorPos.y };

			ImColor c2 = { col };
			float h, s, v;
			ImGui::ColorConvertRGBtoHSV(c2.Value.x, c2.Value.y, c2.Value.z, h, s, v);
			auto halfAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.45f);
			auto lowAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.15f);

			//window->DrawList->AddLine(ImVec2(separator_min_x, y), ImVec2(separator_max_x, y), col, 1.0f);

			ImGui::GetBackgroundDrawList()->AddRectFilled(body_bb.Min, body_bb.Max, lowAlpha, 2, ImDrawFlags_RoundCornersAll);
			ImGui::GetBackgroundDrawList()->AddRect(body_bb.Min, body_bb.Max, halfAlpha, 2, ImDrawFlags_RoundCornersAll);

			ImGui::EndTabBar();

			// add some bottom margin.
			ImGui::Dummy({ 0, 6 });
		}

		void IndicatorDot(const char* colorHTML, const char* caption = nullptr) {

			auto c = ImGui::GetCursorScreenPos();
			ImRect bb = { c, c + ImVec2{20,20} };
			auto* dl = ImGui::GetWindowDrawList();
			dl->AddCircleFilled(c, 6, ColorFromHTML(colorHTML, 0.8f));
			if (caption) {
				dl->AddText(c + ImVec2{ 8,8 }, ColorFromHTML(colorHTML, 0.8f), caption);
			}

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

		struct FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", (float)::WaveSabreCore::Helpers::ParamToFrequency((float)param));
				return s;
			}

			//virtual double DisplayValueToParam(double value, void* capture) {
			//	return ::WaveSabreCore::Helpers::FrequencyToParam((float)value);
			//}
		};

		struct ScaledFloatConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };
			float mMin;
			float mMax;
			//WaveSabreCore::M7::ScaledRealParam mParam;

			ScaledFloatConverter(float v_min, float v_max) :
				mMin(v_min), mMax(v_max)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				//mParam.SetParamValue((float)param);
				mBacking = (float)param;
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", mParams.GetScaledRealValue(0, mMin, mMax, 0));
				return s;
			}

			//virtual double DisplayValueToParam(double value, void* capture) {
			//	//mParam.SetRangedValue((float)value);
			//	mParams.SetRangedValue(0, mMin, mMax, (float)value);
			//	return (double)mBacking;
			//}
		};

		struct Maj7IntConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };
			const M7::IntParamConfig mCfg;
			//WaveSabreCore::M7::IntParam mParam;

			Maj7IntConverter(const M7::IntParamConfig& cfg) :
				mCfg(cfg)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				//mParams.SetParamValue((float)param);
				mBacking = (float)param;
				int val = mParams.GetIntValue(0, mCfg);
				char s[100] = { 0 };
				sprintf_s(s, "%d", val);
				return s;
			}

			//virtual double DisplayValueToParam(double value, void* capture) {

			//	mParam.SetIntValue((int)value);
			//	return (double)mParam.GetRawParamValue();
			//}
		};

		struct Maj7MidiNoteConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				return midiNoteToString(mParams.GetIntValue(0, M7::gKeyRangeCfg));
			}

		};

		

		struct Maj7FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking[2]{ 0,0 }; // freq, kt
			VstInt32 mKTParamID;
			M7::ParamAccessor mParams{ mBacking, 0 };
			const M7::FreqParamConfig mCfg;


			Maj7FrequencyConverter(M7::FreqParamConfig cfg, VstInt32 ktParamID /*pass -1 if no KT*/) :
				mCfg(cfg),
				mKTParamID(ktParamID)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking[0] = (float)param;
				mBacking[1] = 0;
				VstEditor* pThis = (VstEditor*)capture;
				if (mKTParamID >= 0) {
					mBacking[1] = pThis->GetEffectX()->getParameter(mKTParamID);
				}


				char s[100] = { 0 };
				if ((mKTParamID < 0) || mBacking[1] < 0.00001f) {
					float hz = mParams.GetFrequency(0, 1, mCfg, 0, 0);
					//M7::real_t hz = mParam.GetFrequency(0, 0);
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
					sprintf_s(s, "%0.0f%%", mBacking[0] * 100);
				}

				return s;
			}

		};

		struct FloatN11Converter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				float n11 = mParams.GetN11Value(0, 0);
				char s[100] = { 0 };
				sprintf_s(s, "%0.3f", n11);
				return s;
			}

		};

		struct Float01Converter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				char s[100] = { 0 };
				sprintf_s(s, "%0.3f", mParams.Get01Value(0));
				return s;
			}
		};

		struct FMValueConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			M7::ParamAccessor mParams{ &mBacking, 0 };

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mBacking = (float)param;
				char s[100] = { 0 };
				sprintf_s(s, "%d", int(mParams.Get01Value(0) * 100));
				return s;
			}
		};

		//// case ParamIndices::LoopMode: return (float)loopMode / (float)((int)LoopMode::NumLoopModes - 1);
		////template<typename T>
		//float EnumToParam(int value, int valueCount, int baseVal = 0) {
		//	return (float)(value - baseVal) / (valueCount - 1);
		//}

		//// case ParamIndices::LoopMode: loopMode = (LoopMode)(int)(value * (float)((int)LoopMode::NumLoopModes - 1)); break;
		////template<typename T>
		//int ParamToEnum(float value, int valueCount, int baseVal = 0) {
		//	if (value < 0) value = 0;
		//	if (value > 1) value = 1;
		//	return baseVal + (int)(value * (valueCount - 1));
		//}

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
					GetEffectX()->setParameterAutomated(id, M7::math::clamp01(paramValue));
				}
				break;
			}
			//case ParamBehavior::VibratoFreq:
			//{
			//	static VibratoFreqConverter conv;
			//	r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
			//	if (r) {
			//		GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
			//	}
			//	break;
			//}
			//case ParamBehavior::Db:
			//{
			//	static DbConverter conv;
			//	r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.5f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
			//	if (r) {
			//		GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
			//	}
			//	break;
			//}
			//case ParamBehavior::FilterQ:
			//{
			//	static FilterQConverter conv;
			//	r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
			//	if (r) {
			//		GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
			//	}
			//	break;
			//}

			case ParamBehavior::Frequency:
			{
				static FrequencyConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0.0f, 1.0f, 0.5f, 0.0f, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, "%.2fHz", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);

				if (r) {
					GetEffectX()->setParameterAutomated(id, M7::math::clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::Default01:
			default:
			{
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0, ImGuiKnobs::ModInfo{}, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly);
				if (r) {
					GetEffectX()->setParameterAutomated(id, M7::math::clamp01(paramValue));
				}
				break;
			}
			}
			return r;
		}

		//void WSImGuiParamKnobInt(VstInt32 id, const char* name, int v_min, int v_max, int v_default, int v_center) {
		//	float paramValue = GetEffectX()->getParameter((VstInt32)id);
		//	M7::ParamAccessor pa{ &paramValue, 0 };
		//	//int v_count = v_max - v_min + 1;
		//	//int iparam = ParamToEnum(paramValue, v_count, v_min);
		//	enum AnyEnum{};
		//	int iparam = (int)pa.GetEnumValue<AnyEnum>(0);
		//	if (ImGuiKnobs::KnobInt(name, &iparam, v_min, v_max, v_default, v_center, 0.003f * v_count, 0.0001f * v_count, NULL, ImGuiKnobVariant_WiperOnly))
		//	{
		//		paramValue = EnumToParam(iparam, v_count, v_min);
		//		GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
		//	}
		//}


		//float Clamp01(float x)
		//{
		//	if (x < 0) return 0;
		//	if (x > 1) return 1;
		//	return x;
		//}

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

		template<typename TParamID>
		void Maj7ImGuiBoolParamToggleButton(TParamID paramID, const char* label, ImVec2 size = {}, const ButtonColorSpec& cfg = {}) {
			bool ret = false;
			int colorsPushed = 0;
			float backing = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor p{ &backing, 0 };
			bool value = p.GetBoolValue(0);

			if (cfg.hoveredColor) {
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(cfg.hoveredColor));
				colorsPushed++;
			}
			if (value) {
				if (cfg.selectedColor) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
					colorsPushed += 2;
				}
			}
			else {
				if (cfg.notSelectedColor) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.notSelectedColor));
					colorsPushed += 2;
				}
			}

			if (ImGui::Button(label, size)) {
				p.SetBoolValue(0, !value);
				GetEffectX()->setParameterAutomated(paramID, backing);
			}

			ImGui::PopStyleColor(colorsPushed);
		}



		ImVec2 CalcListBoxSize(float items)
		{
			return { 0.0f, ImGui::GetTextLineHeightWithSpacing() * items + ImGui::GetStyle().FramePadding.y * 2.0f };
		}

		//template<typename Tenum>
		void WSImGuiParamEnumList(VstInt32 paramID, const char* ctrlLabel, int elementCount, const char* const* const captions) {
			//const char* captions[] = { "LP", "HP", "BP","Notch" };

			float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor pa{ &paramValue, 0 };
			enum AnyEnum {};
			auto friendlyVal = pa.GetEnumValue<AnyEnum>(0);// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
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
						pa.SetEnumIntValue(0, n);
						//paramValue = EnumToParam(n, elementCount);// ::WaveSabreCore::Helpers::StateVariableFilterTypeToParam((::WaveSabreCore::StateVariableFilterType)enumIndex);
						GetEffectX()->setParameterAutomated(paramID, (paramValue));
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
			//M7::real_t tempVal;
			//M7::EnumParam<Tenum> p(tempVal, Tenum(elementCount));
			M7::QuickParam p{ GetEffectX()->getParameter((VstInt32)paramID) };
			//p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			auto friendlyVal = p.GetEnumValue<Tenum>();// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
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
						GetEffectX()->setParameterAutomated((VstInt32)paramID, p.GetRawValue());
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
			//M7::real_t tempVal;
			//M7::EnumParam<Tenum> p(tempVal, Tenum(elementCount));
			//p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			M7::QuickParam p{ GetEffectX()->getParameter((VstInt32)paramID) };
			auto friendlyVal = p.GetEnumValue<Tenum>();// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
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
						GetEffectX()->setParameterAutomated((VstInt32)paramID, p.GetRawValue());
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

		// the point of this is that you can have a different set of buttons than enum options. the idea is that when "none" are selected, you can do something different.
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
				const bool is_selected = (selectedVal == cfg.value);
				int colorsPushed = 0;

				if (is_selected) {
					if (cfg.selectedColor) {
						ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
						colorsPushed += 2;
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


		template<typename TEnum>
		struct EnumMutexButtonArrayItem {
			const char* caption;
			const char* selectedColor;
			const char* notSelectedColor;
			const char* selectedHoveredColor;
			const char* notSelectedHoveredColor;
			TEnum value;
		};

		// the point of this is that you can have a different set of buttons than enum options. the idea is that when "none" are selected, you can do something different.
		template<typename Tenum, typename TparamID, size_t Tcount>
		void Maj7ImGuiParamEnumMutexButtonArray(TparamID paramID, const char* ctrlLabel, float width, bool horiz, const EnumMutexButtonArrayItem<Tenum>(&itemCfg)[Tcount], int spacing = 0, int columns = 0) {
			const char* defaultSelectedColor = "4400aa";
			const char* defaultNotSelectedColor = "222222";
			const char* defaultSelectedHoveredColor = "8800ff";
			const char* defaultNotSelectedHoveredColor = "222299";

			auto coalesce = [](const char* a, const char* b) {
				return !!a ? a : b;
			};

			M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor pa{ &tempVal, 0 };
			auto selectedVal = pa.GetEnumValue<Tenum>(0);

			ImGui::PushID(ctrlLabel);

			ImGui::BeginGroup();

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

			int column = 0;
			for (size_t i = 0; i < Tcount; i++)
			{
				auto& cfg = itemCfg[i];
				const bool is_selected = (selectedVal == cfg.value);
				int colorsPushed = 0;

				if (is_selected) {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(coalesce(cfg.selectedColor, defaultSelectedColor)));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(coalesce(cfg.selectedColor, defaultSelectedColor)));
					colorsPushed += 2;
				}
				else {
					ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(coalesce(cfg.notSelectedColor, defaultNotSelectedColor)));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(coalesce(cfg.notSelectedColor, defaultNotSelectedColor)));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ColorFromHTML(coalesce(cfg.notSelectedHoveredColor, defaultNotSelectedHoveredColor)));
					colorsPushed += 3;
				}

				if (horiz && column != 0) {
					ImGui::SameLine(0, (float)spacing);
				}
				else if (spacing != 0) {
					ImGui::Dummy({ 1.0f,(float)spacing });
				}

				if (cfg.caption) {
					if (ImGui::Button(cfg.caption, ImVec2{ width , 0 })) {
						pa.SetEnumValue(0, cfg.value);
						GetEffectX()->setParameterAutomated((VstInt32)paramID, tempVal);
					}
				}
				else {
					float height = ImGui::GetTextLineHeightWithSpacing();
					ImGui::Dummy(ImVec2{width, height});
				}

				column = (column + 1);
				if (columns) column %= columns;

				ImGui::PopStyleColor(colorsPushed);
			}

			ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding

			ImGui::EndGroup();
			ImGui::PopID();
		}


			template<typename TparamID>
			struct BoolToggleButtonArrayItem {
				TparamID paramID;
				const char* caption;
				const char* selectedColor;
				const char* notSelectedColor;
				const char* selectedHoveredColor;
				const char* notSelectedHoveredColor;
			};


			// the point of this is that you can have a different set of buttons than enum options. the idea is that when "none" are selected, you can do something different.
			template<typename TparamID, size_t Tcount>
			void Maj7ImGuiParamBoolToggleButtonArray(const char* ctrlLabel, float width, const BoolToggleButtonArrayItem<TparamID>(&itemCfg)[Tcount]) {
				ImGui::PushID(ctrlLabel);

				ImGui::BeginGroup();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 0, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				for (size_t i = 0; i < Tcount; i++)
				{
					auto& cfg = itemCfg[i];
					//const bool is_selected = (selectedVal == cfg.value);
					int colorsPushed = 0;

					M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)cfg.paramID);
					M7::ParamAccessor pa{ &tempVal, 0 };
					auto boolVal = pa.GetBoolValue(0);

					if (boolVal) {
						if (cfg.selectedColor) {
							ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ColorFromHTML(cfg.selectedColor));
							ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ColorFromHTML(cfg.selectedColor));
							colorsPushed += 2;
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
						pa.SetBoolValue(0, !boolVal);
						GetEffectX()->setParameterAutomated((VstInt32)cfg.paramID, tempVal);
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
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::ScaledRealParam p{ tempVal , v_min, v_max };
			pa.SetRangedValue(0, v_min, v_max, v_defaultScaled);
			//p.SetRangedValue(v_defaultScaled);
			float defaultParamVal = tempVal;
			pa.SetRangedValue(0, v_min, v_max, v_centerScaled);
			float centerParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			ScaledFloatConverter conv{ v_min, v_max };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, sizePixels, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFMKnob(VstInt32 paramID, const char* label) {
			WaveSabreCore::M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::Float01Param p{ tempVal };
			//p.SetParamValue();
			const float v_default = 0;
			//const float size = ImGui::GetTextLineHeight()* 2.5f;// default is 3.25f;
			FMValueConverter conv;
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, v_default, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, "%.2f", ImGuiKnobVariant_ProgressBarWithValue, 0, ImGuiKnobFlags_CustomInput | ImGuiKnobFlags_NoInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamInt(VstInt32 paramID, const char* label, const M7::IntParamConfig& cfg, int v_defaultScaled, int v_centerScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::IntParam p{ tempVal , cfg };
			pa.SetIntValue(0, cfg, v_defaultScaled);
			//p.SetIntValue(v_defaultScaled);
			float defaultParamVal = tempVal;
			pa.SetIntValue(0, cfg, v_centerScaled);
			float centerParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			Maj7IntConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFrequency(VstInt32 paramID, VstInt32 ktParamID, const char* label, M7::FreqParamConfig cfg, M7::real_t defaultParamValue, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			//M7::real_t tempValKT = 0;
			//M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::FrequencyParam p{ tempVal, tempValKT, cfg };

			//tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			//pa.GetFrequency

			Maj7FrequencyConverter conv{ cfg, ktParamID };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamValue, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFrequencyWithCenter(VstInt32 paramID, VstInt32 ktParamID, const char* label, M7::FreqParamConfig cfg, M7::real_t defaultFreqHz, float centerVal01, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal = 0;
			//M7::real_t tempValKT = 0;
			//M7::FrequencyParam p{ tempVal, tempValKT, cfg };
			M7::ParamAccessor pa{ &tempVal, 0 };
			pa.SetFrequencyAssumingNoKeytracking(0, cfg, defaultFreqHz);

			//tempVal = defaultFrequencyHz;
			float default01 = tempVal;
			//tempVal = centerValHz;
			//float centerVal01 = p.GetFrequency(0, 0);
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			Maj7FrequencyConverter conv{ cfg, ktParamID };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, default01, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamMidiNote(VstInt32 paramID, const char* label, int defaultVal, int centerVal) {
			M7::real_t tempVal = 0;
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::IntParam p{ tempVal, M7::gKeyRangeCfg };
			pa.SetIntValue(0, M7::gKeyRangeCfg, defaultVal);
			//p.SetIntValue(defaultVal);
			float defaultParamVal = tempVal;
			//p.SetIntValue(centerVal);
			pa.SetIntValue(0, M7::gKeyRangeCfg, centerVal);
			float centerParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			Maj7MidiNoteConverter conv;
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_ProgressBar, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		template<typename Tparam>
		void Maj7ImGuiPowCurvedParam(Tparam paramID, const char* label, const M7::PowCurvedParamCfg& cfg, M7::real_t defaultMS, ImGuiKnobs::ModInfo modInfo) {
			M7::real_t tempVal;
			M7::ParamAccessor p{ &tempVal, 0 };
			p.SetPowCurvedValue(0, cfg, defaultMS);
			float defaultParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);
			//float ms = p.GetTimeMilliseconds(0, cfg, 0);

			PowCurvedConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated((VstInt32)paramID, (tempVal));
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
				GetEffectX()->setParameterAutomated((VstInt32)paramID, (tempVal));
			}
		}

		//void Maj7ImGuiParamEnvTime(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, ImGuiKnobs::ModInfo modInfo) {
		//	WaveSabreCore::M7::real_t tempVal;
		//	M7::EnvTimeParam p{ tempVal , v_defaultScaled };
		//	float defaultParamVal = p.GetRawParamValue();
		//	p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

		//	EnvTimeConverter conv{ };
		//	if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
		//	{
		//		GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
		//	}
		//}

		void Maj7ImGuiParamFloatN11(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, float size/*=0*/, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ParamAccessor pa{ &tempVal, 0 };
			pa.SetN11Value(0, v_defaultScaled);
			float defaultParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			FloatN11Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, 0.5f, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloatN11WithCenter(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled, float centerValN11, float size/*=0*/, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ParamAccessor pa{ &tempVal, 0 };
			pa.SetN11Value(0, v_defaultScaled);
			float defaultParamVal = tempVal;
			pa.SetN11Value(0, centerValN11);
			float centerVal01 = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			FloatN11Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloat01(VstInt32 paramID, const char* label, M7::real_t v_default01, float centerVal01, float size = 0.0f, ImGuiKnobs::ModInfo modInfo = {}) {
			WaveSabreCore::M7::real_t tempVal = v_default01;
			M7::ParamAccessor pa{ &tempVal, 0 };
			float defaultParamVal = pa.Get01Value(0);
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			Float01Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerVal01, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, size, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		enum class M7CurveRenderStyle
		{
			Rising,
			Falling,
		};

		void Maj7ImGuiParamCurve(VstInt32 paramID, const char* label, M7::real_t v_defaultN11, M7CurveRenderStyle style, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::CurveParam p{ tempVal };
			pa.SetN11Value(0, v_defaultN11);
			float defaultParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

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
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamVolume(VstInt32 paramID, const char* label, const M7::VolumeParamConfig& cfg, M7::real_t v_defaultDB, ImGuiKnobs::ModInfo modInfo) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ParamAccessor pa{ &tempVal, 0 };
			//M7::VolumeParam p{ tempVal, cfg };
			pa.SetDecibels(0, cfg, v_defaultDB);
			//p.SetDecibels(v_defaultDB);
			float defaultParamVal = tempVal;// p.GetRawParamValue();
			//p.SetDecibels(0);
			pa.SetDecibels(0, cfg, 0);
			float centerParamVal = tempVal;
			tempVal = GetEffectX()->getParameter((VstInt32)paramID);

			M7VolumeConverter conv{ cfg };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, centerParamVal, modInfo, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, M7::math::clamp01(tempVal));
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

			M7::QuickParam attackCurve{ GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve) };
			//float attackCurveRaw;
			//M7::CurveParam attackCurve{ attackCurveRaw };
			//attackCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve));

			M7::QuickParam decayCurve{ GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve) };
			//float decayCurveRaw;
			//M7::CurveParam decayCurve{ decayCurveRaw };
			//decayCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve));

			M7::QuickParam releaseCurve{ GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve) };
			//float releaseCurveRaw;
			//M7::CurveParam releaseCurve{ releaseCurveRaw };
			//releaseCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve));

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
			AddCurveToPath(dl, delayEnd, { attackEnd.x - delayEnd.x, attackEnd.y - delayEnd.y }, false, false, attackCurve.GetRawValue(), lineColor, gLineThickness);

			ImVec2 holdEnd = { attackEnd.x + holdWidth, attackEnd.y };
			//dl->PathLineTo(holdEnd);
			dl->AddLine(attackEnd, holdEnd, lineColor, gLineThickness);

			ImVec2 decayEnd = { holdEnd.x + decayWidth, innerTL.y + sustainYOffset };
			AddCurveToPath(dl, holdEnd, { decayEnd.x - holdEnd.x, decayEnd.y - holdEnd.y }, true, true, decayCurve.GetRawValue(), lineColor, gLineThickness);

			ImVec2 sustainEnd = { decayEnd.x + sustainWidth, decayEnd.y };
			//dl->PathLineTo(sustainEnd); // sustain flat line
			dl->AddLine(decayEnd, sustainEnd, lineColor, gLineThickness);

			ImVec2 releaseEnd = { sustainEnd.x + releaseWidth, innerBottomLeft.y };
			AddCurveToPath(dl, sustainEnd, { releaseEnd.x - sustainEnd.x, releaseEnd.y - sustainEnd.y }, true, true, releaseCurve.GetRawValue(), lineColor, gLineThickness);

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


		GuiContextRestorer PushMyImGuiContext(const char *why) {
			return { mImGuiContext, why };
		}

		void ImguiPresent();

		static LRESULT WINAPI gWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		ERect mDefaultRect = { 0 };
		bool showingDemo = false;

		bool showingParamExplorer = false;
		ParamExplorer mParamExplorer;
	};

}

#endif
