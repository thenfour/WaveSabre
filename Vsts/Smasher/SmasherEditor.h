#ifndef __SMASHEREDITOR_H__
#define __SMASHEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "SmasherVst.h"

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

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter(mpSmasherVST->mpSmasher->inputLevel, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode));
		ImGui::SameLine(); VUMeter(mpSmasherVST->mpSmasher->thresholdScalar, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText | (int)VUMeterFlags::NoForeground));
		ImGui::SameLine(); VUMeter(mpSmasherVST->mpSmasher->atten, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::AttenuationMode | (int)VUMeterFlags::NoText));
		ImGui::SameLine(); VUMeter(mpSmasherVST->mpSmasher->outputPeak, (VUMeterFlags)((int)VUMeterFlags::InputIsLinear | (int)VUMeterFlags::LevelMode | (int)VUMeterFlags::NoText));

	}

};

#endif
