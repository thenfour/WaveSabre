#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7WidthVst.h"

struct Maj7WidthEditor : public VstEditor
{
	Maj7Width* mpMaj7Width;
	Maj7WidthVst* mpMaj7WidthVst;

	Maj7WidthEditor(AudioEffect* audioEffect) : //
		VstEditor(audioEffect, 690, 460),
		mpMaj7WidthVst((Maj7WidthVst*)audioEffect)
	{
		mpMaj7Width = ((Maj7WidthVst *)audioEffect)->GetMaj7Width();
	}

	virtual void PopulateMenuBar() override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Width", mpMaj7Width, mpMaj7WidthVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Width->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::BeginGroup();

		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LeftSource, "Left source", -1, -1, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamFloatN11WithCenter((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RightSource, "Right source", 1, 1, 0, {});
		ImGui::EndGroup();

		ImGui::SameLine(); ImGui::Text("Select where the left / right channels originate.\r\nThis can be used to swap L/R for example.");

		ImGui::BeginGroup();
		Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle, "Rotation", -M7::math::gPI, M7::math::gPI, 0, 0, 0, {});
		ImGui::EndGroup();

		ImGui::SameLine(); ImGui::Text("Rotate is just another way to manipulate the image.\r\nIt may cause phase issues, or it may help center an imbalanced image,\r\ne.g. correcting weird mic placement.");

		ImGui::BeginGroup();
		Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency, -1, "Side HPF", M7::gFilterFreqConfig, 0, {});
		ImGui::EndGroup();
		ImGui::SameLine(); ImGui::Text("This reduces width of low frequencies. Fixed at 6db/Oct slope.");

		ImGui::BeginGroup();
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidSideBalance, "Mid-Side balance", 0.0f, 0, {});
		ImGui::EndGroup();
		ImGui::SameLine(); ImGui::Text("Mix between mono -> normal -> wide");

		ImGui::BeginGroup();
		ImGui::Text("Final output panning");
		Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain, "Output", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
		ImGui::EndGroup();
		ImGui::SameLine(); ImGui::Text("Final output panning & gain");

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

		ImGui::SameLine(); VUMeter("vu_inp", mpMaj7Width->mInputAnalysis[0], mpMaj7Width->mInputAnalysis[1], mainCfg);
		ImGui::SameLine(); VUMeter("vu_outp", mpMaj7Width->mOutputAnalysis[0], mpMaj7Width->mOutputAnalysis[1], mainCfg);

	}

};

