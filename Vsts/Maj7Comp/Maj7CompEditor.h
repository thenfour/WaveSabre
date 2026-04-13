#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7CompVst.h"
#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>
#include <WaveSabreVstLib/CompressorVis.hpp>

struct Maj7CompEditor : public VstEditor
{
	Maj7Comp* mpMaj7Comp;
	Maj7CompVst* mpMaj7CompVst;

	using ParamIndices = WaveSabreCore::Maj7Comp::ParamIndices;
	static constexpr float kSoftClipTransferCurveWidth = 120.0f;
	static constexpr float kSoftClipTransferCurveHeight = 100.0f;

	FrequencyResponseRendererLayered<160, 75, 1, (size_t)Maj7Comp::ParamIndices::NumParams, false> mResponseGraph;

	CompressorVis<480, 180> mCompressorVis[Maj7MBC::gBandCount];
	const ImVec2 kSoftClipTransferCurveSize{ kSoftClipTransferCurveWidth, kSoftClipTransferCurveHeight };

	Maj7CompEditor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 834, 680),
		mpMaj7CompVst((Maj7CompVst*)audioEffect)
	{
		mpMaj7Comp = ((Maj7CompVst *)audioEffect)->GetMaj7Comp();
	}

	void RenderSoftClipControls()
	{
		Maj7ImGuiBoolParamToggleButton(ParamIndices::SoftClipEnable, "Softclip");
		M7::QuickParam enabledParam{ GetEffectX()->getParameter((VstInt32)ParamIndices::SoftClipEnable) };
		if (!enabledParam.GetBoolValue()) return;

		ImGui::SameLine();
		Maj7ImGuiParamVolume((VstInt32)ParamIndices::SoftClipDrive, "Drive", M7::gVolumeCfg36db, 0, {});

		M7::QuickParam driveParam{ GetEffectX()->getParameter((VstInt32)ParamIndices::SoftClipDrive) };
		float softClipDriveLin = driveParam.GetVolumeLin(M7::gVolumeCfg36db);

		ImGui::SameLine();
		RenderTransferCurve(kSoftClipTransferCurveSize,
			{
				ColorFromHTML("222222"),
				ColorFromHTML("8888cc"),
				ColorFromHTML("ffff00"),
				ColorFromHTML("444444"),
			},
			[softClipDriveLin](float x) -> float
			{
				return M7::math::SineSoftClip(x * softClipDriveLin);
			});
	}

	void RenderOutputSignalControl()
	{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		struct OutputSignalOption
		{
			Maj7Comp::OutputSignal mValue;
			const char* mLabel;
		};

		static constexpr OutputSignalOption outputSignalOptions[] = {
			{ Maj7Comp::OutputSignal::Normal, "Normal" },
			{ Maj7Comp::OutputSignal::Sidechain, "Sidechain" },
			{ Maj7Comp::OutputSignal::Diff, "Delta" },
		};

		auto currentSignal = mpMaj7Comp->mOutputSignal;
		const char* preview = outputSignalOptions[0].mLabel;
		for (const auto& option : outputSignalOptions)
		{
			if (option.mValue == currentSignal)
			{
				preview = option.mLabel;
				break;
			}
		}

		ImGui::SetNextItemWidth(140);
		if (ImGui::BeginCombo("Output signal", preview))
		{
			for (const auto& option : outputSignalOptions)
			{
				bool isSelected = currentSignal == option.mValue;
				if (ImGui::Selectable(option.mLabel, isSelected))
				{
					mpMaj7Comp->mOutputSignal = option.mValue;
					currentSignal = option.mValue;
				}
				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted("Normal: processed output signal.");
			ImGui::TextUnformatted("Sidechain: hear the detector path after filtering.");
			ImGui::TextUnformatted("Delta: hear only the material removed by compression.");
			ImGui::EndTooltip();
		}
#else
		ImGui::Text("Output signal\r\n#undef");
#endif
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


				//ImGui::SameLine(0, 80); ImGui::Text("Peak Detection\r\n(RMS removed)");
				ImGui::SameLine(0, 80); WSImGuiParamCheckbox((VstInt32)ParamIndices::EnableSidechainFilter, "Sidechain Filter");

				M7::QuickParam scfEnabledParam{ GetEffectX()->getParameter((int)ParamIndices::EnableSidechainFilter) };
				bool scfEnabled = scfEnabledParam.GetBoolValue();

				if (scfEnabled) {

					ImGui::SameLine(); Maj7ImGuiParamFrequency((int)ParamIndices::HighPassFrequency, -1, "HP(Hz)", M7::gFilterFreqConfig, 0, {});
					//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::HighPassQ, "HP Q", 0.2f, 0.2f);
					//ImGui::SameLine(); Maj7ImGuiParamFrequency((int)ParamIndices::LowPassFrequency, -1, "LP(Hz)", M7::gFilterFreqConfig, 22000, {});
					//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)ParamIndices::LowPassQ, "LP Q", 0.2f, 0.2f);



					const std::array<FrequencyResponseRendererFilter, 1> filters{
						//FrequencyResponseRendererFilter{"cc4444", &mpMaj7Comp->mComp[0].mLowpassFilter},
						FrequencyResponseRendererFilter{"4444cc", &mpMaj7Comp->mComp[0].mHighpassFilter}
					};

					FrequencyResponseRendererConfig<1, (size_t)Maj7Comp::ParamIndices::NumParams> cfg{
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

				}

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
				ImGui::SameLine(0, 80);

				Maj7ImGuiParamVolume((VstInt32)ParamIndices::InputGain, "Input gain", M7::gVolumeCfg24db, 0, {});
				ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)ParamIndices::OutputGain, "Output gain", M7::gVolumeCfg24db, 0, {});

				ImGui::Spacing();
				ImGui::SeparatorText("Soft clip");
				RenderSoftClipControls();

				ImGui::Spacing();
				ImGui::SeparatorText("Output signal");
				RenderOutputSignalControl();

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    auto& ia = mpMaj7Comp->mInputAnalysis;
    auto& da = mpMaj7Comp->mDetectorAnalysis;
    auto& aa = mpMaj7Comp->mAttenuationAnalysis;
    auto& oa = mpMaj7Comp->mOutputAnalysis;
#else
    AnalysisStream ia[2];  // mocks
    AnalysisStream da[2];
    AnalysisStream aa[2];
    AnalysisStream oa[2];
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

		mCompressorVis->Render(true, true, mpMaj7Comp->mComp[0],
			ia, da, aa, oa);
	}
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7CompEditor(audioEffect);
}
