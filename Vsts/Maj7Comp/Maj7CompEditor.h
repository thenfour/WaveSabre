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

	HistoryEnvFollower mInputPeak;
	HistoryEnvFollower mOutputPeak;
	MovingRMS<10> mInputRMS;
	MovingRMS<10> mOutputRMS;

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
		VstEditor(audioEffect, 850, 870),
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
		float detectorLevel = std::max(mpMaj7Comp->mComp[0].mPostDetector, mpMaj7Comp->mComp[1].mPostDetector);
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
				Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::Threshold, "Threshold(dB)", -60, 0, -20, 0, 0, {});

				ImGui::SameLine(); Maj7ImGuiParamTime(ParamIndices::Attack, "Attack(ms)", Maj7Comp::gAttackCfg, 50, {});
				ImGui::SameLine(); Maj7ImGuiParamTime(ParamIndices::Release, "Release(ms)", Maj7Comp::gReleaseCfg, 80, {});

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
				WSImGuiParamCheckbox((VstInt32)ParamIndices::MidSideEnable, "MidSide processing?");
				ImGui::SameLine();
				Maj7ImGuiParamFloat01((VstInt32)ParamIndices::ChannelLink, "Channel link", 0.8f, 0);

				Maj7ImGuiParamFloat01((VstInt32)ParamIndices::PeakRMSMix, "Peak-RMS mix", 0, 0);

				ImGui::SameLine(); Maj7ImGuiParamTime(ParamIndices::RMSWindow, "RMS Window(ms)", Maj7Comp::gRMSWindowSizeCfg, 30, {});

				Maj7ImGuiParamFrequency((int)ParamIndices::HighPassFrequency, -1, "HP Freq(Hz)", M7::gFilterFreqConfig, 0, {});
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
				Maj7ImGuiParamVolume((VstInt32)ParamIndices::CompensationGain, "Compensation gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::DryWet, "Dry-Wet mix", 1, 0);
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				static constexpr char const* const signalNames[] = { "Normal", "Diff", "Sidechain" };//  , "GainReduction", "Detector"};
				ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::Maj7Comp::OutputSignal>(ParamIndices::OutputSignal,
					"Output signal", (int)WaveSabreCore::Maj7Comp::OutputSignal::Count__, WaveSabreCore::Maj7Comp::OutputSignal::Normal, signalNames);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginGroup();
		static constexpr float lineWidth = 2.0f;

		mHistoryView.Render({
			// input
			HistoryViewSeriesConfig{ColorFromHTML("999999", mShowLeft && mShowInputHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("666666", mShowRight && mShowInputHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("ff00ff", mShowLeft && mShowDetectorHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("880088", mShowRight && mShowDetectorHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("00ff00", mShowLeft && mShowAttenuationHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("008800", mShowRight && mShowAttenuationHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("4444ff", mShowLeft && mShowOutputHistory ? 0.8f : 0), lineWidth},
			HistoryViewSeriesConfig{ColorFromHTML("0000ff", mShowRight && mShowOutputHistory ? 0.8f : 0), lineWidth},

			HistoryViewSeriesConfig{ColorFromHTML("ffff00", mShowThresh ? 0.2f : 0), 1.0f},
			}, {
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[0].mInput),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[1].mInput),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[0].mPostDetector),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[1].mPostDetector),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[0].mGainReduction),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[1].mGainReduction),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[0].mOutput),
			M7::math::LinearToDecibels(mpMaj7Comp->mComp[1].mOutput),
			mpMaj7Comp->mComp[0].mThreshold,
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

		mInputRMS.addSample(mpMaj7Comp->mComp[0].mInput);
		mOutputRMS.addSample(mpMaj7Comp->mComp[0].mOutput);
		float inputPeakLevel = mInputPeak.ProcessSample(::fabsf(mpMaj7Comp->mComp[0].mInput));
		float outputPeakLevel = mInputPeak.ProcessSample(::fabsf(mpMaj7Comp->mComp[0].mOutput));
		float inputRMSlevel = mInputRMS.getRMS();
		float outputRMSlevel = mOutputRMS.getRMS();

		ImGui::SameLine(); VUMeter(ImVec2{ 40, 300 }, &inputRMSlevel, &inputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode));
		ImGui::SameLine(); VUMeter(ImVec2{20, 300}, & mpMaj7Comp->mComp[0].mGainReduction, & mpMaj7Comp->mComp[0].mGainReduction, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::AttenuationMode | (int)VUMeterFlags::NoText));
		ImGui::SameLine(); VUMeter(ImVec2{ 20, 300 }, nullptr, &mpMaj7Comp->mComp[0].mThreshold, (VUMeterFlags)((int)VUMeterFlags::AttenuationMode | (int)VUMeterFlags::NoText));
		ImGui::SameLine(); VUMeter(ImVec2{ 40, 300 }, &outputRMSlevel, &outputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode));
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
