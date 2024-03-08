#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7CompVst.h"


// use to enable/disable
// - rms detection
// - parallel processing (dry-wet)
// - lowpass filter (it's rarely needed)
// - compensation gain (just use output gain if no parallel processing there's no point)
//#define MAJ7COMP_FULL


struct Maj7CompEditor : public VstEditor
{
	Maj7Comp* mpMaj7Comp;
	Maj7CompVst* mpMaj7CompVst;

	using ParamIndices = WaveSabreCore::Maj7Comp::ParamIndices;

	FrequencyResponseRenderer<160, 75, 50, 2, (size_t)Maj7Comp::ParamIndices::NumParams> mResponseGraph;

	CompressorVis<480, 180> mCompressorVis[Maj7MBC::gBandCount];

	Maj7CompEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7CompVst((Maj7CompVst*)audioEffect)
	{
		mpMaj7Comp = ((Maj7CompVst *)audioEffect)->GetMaj7Comp();
	}


	virtual void PopulateMenuBar() override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Maj7 Comp Compressor", mpMaj7Comp, mpMaj7CompVst, "gParamDefaults", "ParamIndices::NumParams", mpMaj7Comp->mParamCache, paramNames);
	}


	virtual void renderImgui() override
	{
		if (BeginTabBar2("main", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Main controls"))
			{
				Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::Threshold, "Thresh(dB)", -60, 0, -20, 0, 0, {});

				ImGui::SameLine(); Maj7ImGuiPowCurvedParam(ParamIndices::Attack, "Attack(ms)", MonoCompressor::gAttackCfg, 50, {});
				ImGui::SameLine(); Maj7ImGuiPowCurvedParam(ParamIndices::Release, "Release(ms)", MonoCompressor::gReleaseCfg, 80, {});

				ImGui::SameLine(); Maj7ImGuiDivCurvedParam((VstInt32)ParamIndices::Ratio, "Ratio", MonoCompressor::gRatioCfg, 4, {});
				ImGui::SameLine(); Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::Knee, "Knee(dB)", 0, 30, 4, 0, 0, {});

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

#ifdef MAJ7COMP_FULL
				M7::real_t tempVal = GetEffectX()->getParameter((VstInt32)ParamIndices::RMSWindow);
				M7::ParamAccessor p{ &tempVal, 0 };
				float windowMS = p.GetPowCurvedValue(0, Maj7Comp::gRMSWindowSizeCfg, 0);
				const char* windowCaption = "RMS (ms)###rmswindow";
				if (windowMS < Maj7Comp::gMinRMSWindowMS) {
					windowCaption = "Peak###rmswindow";
				}
				ImGui::SameLine(0, 80); Maj7ImGuiPowCurvedParam(ParamIndices::RMSWindow, windowCaption, Maj7Comp::gRMSWindowSizeCfg, 0, {});
#else
				ImGui::SameLine(0, 80); ImGui::Text("Peak-RMS\r\nDisabled");
#endif // MAJ7COMP_FULL

				ImGui::SameLine(0, 80); Maj7ImGuiParamFrequency((int)ParamIndices::HighPassFrequency, -1, "HP Freq(Hz)", M7::gFilterFreqConfig, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::HighPassQ, "HP Q", 0.2f, 0.2f);
#ifdef MAJ7COMP_FULL
				ImGui::SameLine(); Maj7ImGuiParamFrequency((int)ParamIndices::LowPassFrequency, -1, "LP Freq(Hz)", M7::gFilterFreqConfig, 22000, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::LowPassQ, "LP Q", 0.2f, 0.2f);
#else
				ImGui::SameLine(); ImGui::Text("Lowpass\r\nDisabled");
#endif // MAJ7COMP_FULL


				//const BiquadFilter* filters[2] = {
				//	&mpMaj7Comp->mComp[0].mLowpassFilter,
				//	& mpMaj7Comp->mComp[0].mHighpassFilter,
				//};
				const std::array<FrequencyResponseRendererFilter, 2> filters{
#ifdef MAJ7COMP_FULL
					FrequencyResponseRendererFilter{ColorFromHTML("cc4444", 0.8f), &mpMaj7Comp->mComp[0].mLowpassFilter},
#endif // MAJ7COMP_FULL
					FrequencyResponseRendererFilter{ColorFromHTML("4444cc", 0.8f), &mpMaj7Comp->mComp[0].mHighpassFilter}
				};

				FrequencyResponseRendererConfig<2, (size_t)Maj7Comp::ParamIndices::NumParams> cfg{
					ColorFromHTML("222222", 1.0f), // background
						ColorFromHTML("ff8800", 1.0f), // line
						4.0f,
						filters,
				};
				for (size_t i = 0; i < (size_t)Maj7Comp::ParamIndices::NumParams; ++i) {
					cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
				}

				ImGui::SameLine();
				mResponseGraph.OnRender(cfg);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
		if (BeginTabBar2("other", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("IO"))
			{
#ifdef MAJ7COMP_FULL
				Maj7ImGuiParamVolume((VstInt32)ParamIndices::CompensationGain, "Makeup", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::DryWet, "Dry-Wet", 1, 0);
				ImGui::SameLine(0, 80);
#else
				ImGui::Text("Makeup\r\nDisabled");
				ImGui::SameLine(); ImGui::Text("Dry-wet\r\nDisabled");
				ImGui::SameLine(0, 80);
#endif

				Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				static constexpr char const* const signalNames[] = { "Normal", "Diff", "Sidechain" };//  , "GainReduction", "Detector"};
				ImGui::SameLine(0, 80); Maj7ImGuiParamEnumList<WaveSabreCore::Maj7Comp::OutputSignal>(ParamIndices::OutputSignal,
					"Output signal", (int)WaveSabreCore::Maj7Comp::OutputSignal::Count__, WaveSabreCore::Maj7Comp::OutputSignal::Normal, signalNames);

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		mCompressorVis->Render(true, mpMaj7Comp->mComp[0], mpMaj7Comp->mInputAnalysis, mpMaj7Comp->mDetectorAnalysis, mpMaj7Comp->mAttenuationAnalysis, mpMaj7Comp->mOutputAnalysis);
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
