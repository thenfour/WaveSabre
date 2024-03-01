#ifndef __SMASHEREDITOR_H__
#define __SMASHEREDITOR_H__

//#include <queue>

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "SmasherVst.h"
#include "WaveSabreCore/Maj7Basic.hpp"


class SmasherEditor : public VstEditor
{
public:

	SmasherVst* mpSmasherVST;

	SmasherEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 800, 350),
		mpSmasherVST(static_cast<SmasherVst*>(audioEffect))
	{
	}

	SoftPeaks mInputPeak;
	SoftPeaks mOutputPeak;
	MovingRMS<8> mInputRMS;
	MovingRMS<8> mOutputRMS;

	static constexpr ImVec2 gHistViewSize = { 550, 150 };
	static constexpr float gPixelsPerSample = 6;
	static constexpr int gSamplesInHist = (int)(gHistViewSize.x / gPixelsPerSample);
	std::deque<float> mInputHist;
	std::deque<float> mAttenHist;
	std::deque<float> mOutputHist;

	void HistoryView()
	{
		ImRect bb;
		bb.Min = ImGui::GetCursorPos();
		bb.Max = bb.Min + gHistViewSize;

		ImColor backgroundColor = ColorFromHTML("222222", 1.0f);
		ImColor attenLineColor = ColorFromHTML("ff8080", 0.7f);
		ImColor inpLineColor = ColorFromHTML("808080", 0.5f);
		ImColor outpLineColor = ColorFromHTML("8080ff", 0.5f);

		auto DbToY = [&](float db) {
			// let's show a range of -30 to 0 db.
			float x = (db + 30) / 30;
			x = Clamp01(x);
			return M7::math::lerp(bb.Max.y, bb.Min.y, x);
		};
		auto SampleToX = [&](int sample) {
			float sx = (float)sample;
			sx /= gSamplesInHist; // 0,1
			return M7::math::lerp(bb.Min.x, bb.Max.x, sx);
		};

		auto* dl = ImGui::GetWindowDrawList();

		ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

		// assumes that all 3 plot sources have same # of elements
		for (int isample = 0; isample < ((signed)mInputHist.size()) - 1; ++isample)
		{
			dl->AddLine({ SampleToX(isample), DbToY(M7::math::LinearToDecibels(mInputHist[isample])) }, { SampleToX(isample + 1), DbToY(M7::math::LinearToDecibels(mInputHist[isample + 1])) }, inpLineColor, 1.5);
			dl->AddLine({ SampleToX(isample), DbToY(M7::math::LinearToDecibels(mOutputHist[isample])) }, { SampleToX(isample + 1), DbToY(M7::math::LinearToDecibels(mOutputHist[isample + 1])) }, outpLineColor, 1.5);
			dl->AddLine({ SampleToX(isample), DbToY(M7::math::LinearToDecibels(mAttenHist[isample])) }, { SampleToX(isample + 1), DbToY(M7::math::LinearToDecibels(mAttenHist[isample + 1])) }, attenLineColor, 2.5);
		}

		ImGui::Dummy(gHistViewSize);
	}

	virtual void renderImgui() override
	{
		mInputRMS.addSample(mpSmasherVST->mpSmasher->inputLevel);
		mOutputRMS.addSample(mpSmasherVST->mpSmasher->outputPeak);
		mInputPeak.Add(mpSmasherVST->mpSmasher->inputLevel);
		mOutputPeak.Add(mpSmasherVST->mpSmasher->outputPeak);
		float inputPeakLevel = mInputPeak.Check();
		float outputPeakLevel = mOutputPeak.Check();
		float inputRMSlevel = mInputRMS.getRMS();
		float outputRMSlevel = mOutputRMS.getRMS();

		mInputHist.push_back(inputRMSlevel);
		mAttenHist.push_back(mpSmasherVST->mpSmasher->atten);
		mOutputHist.push_back(outputRMSlevel);
		if (mInputHist.size() > gSamplesInHist) {
			mInputHist.pop_front();
		}
		if (mAttenHist.size() > gSamplesInHist) {
			mAttenHist.pop_front();
		}
		if (mOutputHist.size() > gSamplesInHist) {
			mOutputHist.pop_front();
		}

		ImGui::BeginGroup();

		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::InputGain, "INPUT GAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Threshold, "THRESHOLD");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Ratio, "RATIO");

		ImGui::SameLine(0, 60);
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Attack, "ATTACK");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::Release, "RELEASE");

		ImGui::SameLine(0,60);
		WSImGuiParamCheckbox((VstInt32)Smasher::ParamIndices::Sidechain, "SIDECHAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Smasher::ParamIndices::OutputGain, "OUTPUT GAIN");

		HistoryView();

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter(&inputRMSlevel, &inputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode));
		ImGui::SameLine(); VUMeter(&mpSmasherVST->mpSmasher->thresholdScalar, nullptr, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText | (int)VUMeterFlags::NoForeground));
		ImGui::SameLine(); VUMeter(&mpSmasherVST->mpSmasher->atten, nullptr, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::AttenuationMode | (int)VUMeterFlags::NoText));
		ImGui::SameLine(); VUMeter(&outputRMSlevel, &outputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText));

	}

};

#endif
