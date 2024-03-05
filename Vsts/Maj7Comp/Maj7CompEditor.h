#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7CompVst.h"

struct Maj7CompEditor : public VstEditor
{
	Maj7Comp* mpMaj7Comp;
	Maj7CompVst* mpMaj7CompVst;

	using ParamIndices = WaveSabreCore::Maj7Comp::ParamIndices;

	FrequencyResponseRenderer<150, 50, 50, 2, (size_t)Maj7Comp::ParamIndices::NumParams> mResponseGraph;

	bool mShowInputHistory = true;
	bool mShowDetectorHistory = false;
	bool mShowOutputHistory = true;
	bool mShowAttenuationHistory = false;
	bool mShowThresh = true;
	bool mShowLeft = true;
	bool mShowRight = false;

	static constexpr int historyViewHeight = 150;
	static constexpr float historyViewMinDB = -60;
	HistoryView<9, 500, historyViewHeight> mHistoryView;

	Maj7CompEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7CompVst((Maj7CompVst*)audioEffect)
	{
		mpMaj7Comp = ((Maj7CompVst *)audioEffect)->GetMaj7Comp();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Comp Compressor", mpMaj7Comp, mpMaj7CompVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Comp", mpMaj7Comp->mParamCache, paramNames);
	}

	void TransferCurve() {
		ImRect bb;
		ImVec2 size { historyViewHeight , historyViewHeight };
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
			float dbAttenOut = mpMaj7Comp->mComp[0].TransferDecibels(dbIn);// / 5;
			float outDB = dbIn - dbAttenOut;
			points.push_back(ImVec2{ bb.Min.x + inLin * size.x, dbToY(outDB)});
		}

		auto* dl = ImGui::GetWindowDrawList();

		// linear guideline
		dl->AddLine({ bb.Min.x, bb.Max.y }, {bb.Max.x, bb.Min.y}, gridColor, 1.0f);

		// threshold guideline
		float threshY = dbToY(mpMaj7Comp->mComp[0].mThreshold);
		dl->AddLine({ bb.Min.x, threshY }, {bb.Max.x, threshY}, gridColor, 1.0f);

		dl->AddPolyline(points.data(), (int)points.size(), curveColor, 0, 2.0f);

		// detector indicator dot
		float detectorLevel = std::max(mpMaj7Comp->mDetectorAnalysis[0].mCurrentPeak, mpMaj7Comp->mDetectorAnalysis[1].mCurrentPeak);// std::max(mpMaj7Comp->mComp[0].mPostDetector, mpMaj7Comp->mComp[1].mPostDetector);
		float detectorDB = M7::math::LinearToDecibels(detectorLevel);

		float detAtten = mpMaj7Comp->mComp[0].TransferDecibels(detectorDB);
		float detOutDB = detectorDB - detAtten;

		if (detAtten > 0.00001) {
			dl->AddCircleFilled({ dbToX(detectorDB), dbToY(detOutDB) }, 6.0f, detectorColorAtt);
		}
		else {
			dl->AddCircleFilled({ dbToX(detectorDB), dbToY(detOutDB) }, 3.0f, detectorColor);
		}

		ImGui::Dummy(size);
	}

	virtual void renderImgui() override
	{
		if (BeginTabBar2("main", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Main controls"))
			{
				Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::Threshold, "Thresh(dB)", -60, 0, -20, 0, 0, {});

				ImGui::SameLine(); Maj7ImGuiPowCurvedParam(ParamIndices::Attack, "Attack(ms)", Maj7Comp::gAttackCfg, 50, {});
				ImGui::SameLine(); Maj7ImGuiPowCurvedParam(ParamIndices::Release, "Release(ms)", Maj7Comp::gReleaseCfg, 80, {});

				ImGui::SameLine(); Maj7ImGuiDivCurvedParam((VstInt32)ParamIndices::Ratio, "Ratio", Maj7Comp::gRatioCfg, 4, {});
				ImGui::SameLine(); Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::Knee, "Knee(dB)", 0, 30, 8, 0, 0, {});

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
		if (BeginTabBar2("detector", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Envelope detection"))
			{
				//WSImGuiParamCheckbox((VstInt32)ParamIndices::MidSideEnable, "Mid-Side?");
				//ImGui::SameLine();
				Maj7ImGuiParamFloat01((VstInt32)ParamIndices::ChannelLink, "Stereo", 0.8f, 0);

				//ImGui::Separator();

				//ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)ParamIndices::Pan, "EffectPan", 0, 0, {});

				//ImGui::SameLine(0, 60); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::PeakRMSMix, "Peak-RMS", 0, 0);


				M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)ParamIndices::RMSWindow);
				M7::ParamAccessor p{ &tempVal, 0 };
				float windowMS = p.GetPowCurvedValue(0, Maj7Comp::gRMSWindowSizeCfg, 0);
				const char* windowCaption = "RMS (ms)###rmswindow";
				if (windowMS < Maj7Comp::gMinRMSWindowMS) {
					windowCaption = "Peak###rmswindow";
				}
				ImGui::SameLine(0, 80); Maj7ImGuiPowCurvedParam(ParamIndices::RMSWindow, windowCaption, Maj7Comp::gRMSWindowSizeCfg, 30, {});

				ImGui::SameLine(0, 80); Maj7ImGuiParamFrequency((int)ParamIndices::HighPassFrequency, -1, "HP Freq(Hz)", M7::gFilterFreqConfig, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::HighPassQ, "HP Q", 0.2f, 0.2f);

				ImGui::SameLine(); Maj7ImGuiParamFrequency((int)ParamIndices::LowPassFrequency, -1, "LP Freq(Hz)", M7::gFilterFreqConfig, 22000, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::LowPassQ, "LP Q", 0.2f, 0.2f);
				ImGui::SameLine(); 

				const BiquadFilter* filters[2] = {
					&mpMaj7Comp->mComp[0].mLowpassFilter,
					& mpMaj7Comp->mComp[0].mHighpassFilter,
				};

				FrequencyResponseRendererConfig<2, (size_t)Maj7Comp::ParamIndices::NumParams> cfg{
					ColorFromHTML("222222", 1.0f), // background
						ColorFromHTML("ff8800", 1.0f), // line
						filters,
				};
				for (size_t i = 0; i < (size_t)Maj7Comp::ParamIndices::NumParams; ++i) {
					cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
				}

				mResponseGraph.OnRender(cfg);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
		if (BeginTabBar2("other", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("IO"))
			{
				Maj7ImGuiParamVolume((VstInt32)ParamIndices::CompensationGain, "Makeup", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::DryWet, "Dry-Wet", 1, 0);
				ImGui::SameLine(0, 80); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				static constexpr char const* const signalNames[] = { "Normal", "Diff", "Sidechain" };//  , "GainReduction", "Detector"};
				ImGui::SameLine(0, 80); Maj7ImGuiParamEnumList<WaveSabreCore::Maj7Comp::OutputSignal>(ParamIndices::OutputSignal,
					"Output signal", (int)WaveSabreCore::Maj7Comp::OutputSignal::Count__, WaveSabreCore::Maj7Comp::OutputSignal::Normal, signalNames);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginGroup();
		static constexpr float lineWidth = 2.0f;

		mHistoryView.Render({
			// input
			HistoryViewSeriesConfig{ColorFromHTML("666666", mShowLeft && mShowInputHistory ? 0.6f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("444444", mShowRight && mShowInputHistory ? 0.6f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("ff00ff", mShowLeft && mShowDetectorHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("880088", mShowRight && mShowDetectorHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("00ff00", mShowLeft && mShowAttenuationHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("008800", mShowRight && mShowAttenuationHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("4444ff", mShowLeft && mShowOutputHistory ? 0.9f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("0000ff", mShowRight && mShowOutputHistory ? 0.9f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("ffff00", mShowThresh ? 0.2f : 0), 1.0f},
			}, {
			mpMaj7Comp->mInputAnalysis[0].mCurrentPeak,
			mpMaj7Comp->mInputAnalysis[1].mCurrentPeak,
			mpMaj7Comp->mDetectorAnalysis[0].mCurrentPeak,
			mpMaj7Comp->mDetectorAnalysis[1].mCurrentPeak,
			mpMaj7Comp->mAttenuationAnalysis[0].mCurrentPeak,
			mpMaj7Comp->mAttenuationAnalysis[1].mCurrentPeak,
			mpMaj7Comp->mOutputAnalysis[0].mCurrentPeak,
			mpMaj7Comp->mOutputAnalysis[1].mCurrentPeak,
			M7::math::DecibelsToLinear(mpMaj7Comp->mComp[0].mThreshold),
			});

		// ... transfer curve.
		ImGui::SameLine(); TransferCurve();

		ImGui::Checkbox("Input", &mShowInputHistory);
		ImGui::SameLine(); ImGui::Checkbox("Detector", &mShowDetectorHistory);
		ImGui::SameLine(); ImGui::Checkbox("Attenuation", &mShowAttenuationHistory);
		ImGui::SameLine(); ImGui::Checkbox("Output", &mShowOutputHistory);
		//ImGui::SameLine(); ImGui::Checkbox("Threshold", &mShowThresh);

		ImGui::Checkbox("Left", &mShowLeft);
		ImGui::SameLine(); ImGui::Checkbox("Right", &mShowRight);

		ImGui::EndGroup();

		static const std::vector<VUMeterTick> tickSet = {
				{-3.0f, "3db"},
				{-6.0f, "6db"},
				{-12.0f, "12db"},
				{-18.0f, "18db"},
				{-24.0f, "24db"},
				{-30.0f, "30db"},
				{-40.0f, "40db"},
				//{-50.0f, "50db"},
		};

		VUMeterConfig mainCfg = {
			{24, 300},
			VUMeterLevelMode::Audio,
			VUMeterUnits::Linear,
			-50, 6,
			tickSet,
		};

		VUMeterConfig attenCfg = mainCfg;
		attenCfg.levelMode = VUMeterLevelMode::Attenuation;

		ImGui::SameLine(); VUMeter("vu_inp", mpMaj7Comp->mInputAnalysisSlow[0], mpMaj7Comp->mInputAnalysisSlow[1], mainCfg);

		ImGui::SameLine(); VUMeter("vu_atten", &mpMaj7Comp->mAttenuationAnalysis[0].mCurrentPeak, nullptr, nullptr, nullptr, false, attenCfg);

		ImGui::SameLine(); VUMeter("vu_outp", mpMaj7Comp->mOutputAnalysisSlow[0], mpMaj7Comp->mOutputAnalysisSlow[1], mainCfg);

	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
