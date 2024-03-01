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

	Maj7CompEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 600, 600),
		mpMaj7CompVst((Maj7CompVst*)audioEffect)
	{
		mpMaj7Comp = ((Maj7CompVst *)audioEffect)->GetMaj7Comp();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Comp Compressor", mpMaj7Comp, mpMaj7CompVst, "gParamDefaults", "ParamIndices::NumParams", "Maj7Comp", mpMaj7Comp->mParamCache, paramNames);
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

				static constexpr char const* const signalNames[] = { "Normal", "Diff", "Sidechain", "GainReduction", "Detector" };
				ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::Maj7Comp::OutputSignal>(ParamIndices::OutputSignal,
					"Output signal", (int)WaveSabreCore::Maj7Comp::OutputSignal::Count__, WaveSabreCore::Maj7Comp::OutputSignal::Normal, signalNames);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
