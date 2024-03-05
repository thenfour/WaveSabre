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

	virtual void renderImgui() override
	{
		//mInputRMS.addSample(mpSmasherVST->mpSmasher->inputLevel);
		//mOutputRMS.addSample(mpSmasherVST->mpSmasher->outputPeak);
		//float inputPeakLevel = mInputPeak.ProcessSample(mpSmasherVST->mpSmasher->inputLevel);
		//float outputPeakLevel = mOutputPeak.ProcessSample(mpSmasherVST->mpSmasher->outputPeak);
		//float inputRMSlevel = mInputRMS.getRMS();
		//float outputRMSlevel = mOutputRMS.getRMS();

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

		//mHistoryView.Render({
		//	HistoryViewSeriesConfig{ColorFromHTML("ff8080", 0.7f), 2.0f},
		//	HistoryViewSeriesConfig{ColorFromHTML("808080", 0.5f), 2.0f},
		//	HistoryViewSeriesConfig{ColorFromHTML("8080ff", 0.5f), 2.0f},
		//	}, {
		//	M7::math::LinearToDecibels(inputPeakLevel),
		//	M7::math::LinearToDecibels(mpSmasherVST->mpSmasher->atten),
		//	M7::math::LinearToDecibels(outputPeakLevel),
		//	});

		ImGui::EndGroup();

		//ImGui::SameLine(); VUMeter(&inputRMSlevel, &inputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode));
		////ImGui::SameLine(); VUMeter(&mpSmasherVST->mpSmasher->thresholdScalar, nullptr, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText | (int)VUMeterFlags::NoForeground));
		//ImGui::SameLine(); VUMeter(&mpSmasherVST->mpSmasher->atten, nullptr, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::AttenuationMode | (int)VUMeterFlags::NoText));
		//ImGui::SameLine(); VUMeter(&outputRMSlevel, &outputPeakLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText));

	}

};

#endif
