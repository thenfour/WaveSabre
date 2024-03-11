#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "LevellerVst.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class LevellerEditor : public VstEditor
{
	Leveller* mpLeveller;
	LevellerVst* mpLevellerVST;

	ColorMod mEnabledColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	FrequencyResponseRenderer<680,200,100, Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams> mResponseGraph;

public:
	LevellerEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 700, 800),
		mpLevellerVST((LevellerVst*)audioEffect)//,
	{
		mpLeveller = (Leveller*)mpLevellerVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
	}

	virtual void PopulateMenuBar() override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Leveller", mpLeveller, mpLevellerVST, "gLevellerDefaults16", "ParamIndices::NumParams", mpLeveller->mParamCache, paramNames);
	}

	void RenderBand(int id, const char* label, Leveller::ParamIndices paramOffset, float defaultFreqParamHz, ImColor color)
	{
		ImGui::PushID(id);

		M7::QuickParam enabledParam{ GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Enable) };
		ColorMod& cm = enabledParam.GetBoolValue() ? mEnabledColors : mDisabledColors;

		auto token = cm.Push();

		ImRect bb;
		ImVec2 gSize{ 20,80 };
		bb.Min = ImGui::GetCursorScreenPos();
		bb.Max = bb.Min + gSize;
		auto* dl = ImGui::GetWindowDrawList();
		dl->AddRectFilled(bb.Min, bb.Max, color);
		ImGui::Dummy(gSize);

		ImGui::SameLine(); WSImGuiParamCheckbox((int)paramOffset + (int)Leveller::BandParamOffsets::Enable, "Enable?");

		float gainParam = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Gain);

		M7::QuickParam fp{ M7::gFilterFreqConfig };
		//float f1 = 0, f2 = 0;
		//M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };// :gFilterCenterFrequency, M7::gFilterFrequencyScale};
		fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);

		ImGui::SameLine();  Maj7ImGuiParamFrequency((int)paramOffset + (int)Leveller::BandParamOffsets::Freq, -1, "Freq", M7::gFilterFreqConfig, fp.GetRawValue(), {});

		float typeB = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Type);
		M7::ParamAccessor typePA{ &typeB, 0 };
		BiquadFilterType type = typePA.GetEnumValue< BiquadFilterType>(0);
		ImGui::BeginDisabled(type == BiquadFilterType::Highpass || type == BiquadFilterType::Lowpass);
		ImGui::SameLine(); Maj7ImGuiParamVolume((int)paramOffset + (int)Leveller::BandParamOffsets::Gain, "Gain", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
		ImGui::EndDisabled();
		//WSImGuiParamKnob((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q");
		//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q", 0.2f, 0);
		ImGui::SameLine(); Maj7ImGuiDivCurvedParam((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Q", M7::gBiquadFilterQCfg, 0.2f, {});

		const char* selectedColor = "4400aa";
		const char* notSelectedColor = "222222";
		const char* selectedHoveredColor = "8800ff";
		const char* notSelectedHoveredColor = "222299";

		ImGui::SameLine(); Maj7ImGuiParamEnumMutexButtonArray<BiquadFilterType>((int)paramOffset + (int)Leveller::BandParamOffsets::Type, "type", 40, true, {
			{ "/HP", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Highpass, },
			{ "/LS", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::LowShelf, },
			{ "^Peak", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Peak, },
			{ "\\HS", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::HighShelf, },
			{ "\\LP", selectedColor, notSelectedColor, selectedHoveredColor, notSelectedHoveredColor, BiquadFilterType::Lowpass, },
			});


		//ImGui::EndTabItem();
	//}

	//EndTabBarWithColoredSeparator();
//}
		ImGui::PopID();
	}


	virtual void renderImgui() override
	{
		mEnabledColors.EnsureInitialized();
		mDisabledColors.EnsureInitialized();

		ImGui::BeginGroup();

		Maj7ImGuiParamVolume((VstInt32)Leveller::ParamIndices::OutputVolume, "Output volume", WaveSabreCore::M7::gVolumeCfg12db, 0, {});

		ImGui::SameLine();

		WSImGuiParamCheckbox((VstInt32)Leveller::ParamIndices::EnableDCFilter, "Enable DC Filter?");

		const ImColor bandColors[Leveller::gBandCount] = {
			ColorFromHTML("cc44cc", 0.8f),
			ColorFromHTML("44cc44", 0.8f),
			ColorFromHTML("cc4444", 0.8f),
			ColorFromHTML("4444ff", 0.8f),
			ColorFromHTML("ff8822", 0.8f),
		};

		RenderBand(0, "band1", Leveller::ParamIndices::Band1Type, 90, bandColors[0]);
		RenderBand(1, "band2", Leveller::ParamIndices::Band2Type, 250, bandColors[1]);
		RenderBand(2, "band3", Leveller::ParamIndices::Band3Type, 1100, bandColors[2]);
		RenderBand(3, "band4", Leveller::ParamIndices::Band4Type, 3000, bandColors[3]);
		RenderBand(4, "band5", Leveller::ParamIndices::Band5Type, 8500, bandColors[4]);

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter("vu_inp", mpLeveller->mInputAnalysis[0], mpLeveller->mInputAnalysis[1]);
		ImGui::SameLine(); VUMeter("vu_outp", mpLeveller->mOutputAnalysis[0], mpLeveller->mOutputAnalysis[1]);

		//ImGui::EndTable();
		//
		const std::array<FrequencyResponseRendererFilter, Leveller::gBandCount> filters = {
			FrequencyResponseRendererFilter{bandColors[0], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band1Enable) > 0.5f) ? &mpLeveller->mBands[0].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[1], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band2Enable) > 0.5f) ? &mpLeveller->mBands[1].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[2], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band3Enable) > 0.5f) ? &mpLeveller->mBands[2].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[3], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band4Enable) > 0.5f) ? &mpLeveller->mBands[3].mFilters[0] : nullptr},
			FrequencyResponseRendererFilter{bandColors[4], (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band5Enable) > 0.5f) ? &mpLeveller->mBands[4].mFilters[0] : nullptr},
		};

		FrequencyResponseRendererConfig<Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams> cfg{
			ColorFromHTML("222222", 1.0f), // background
				ColorFromHTML("aaaaaa", 1.0f), // line
				8.0f,
				filters,
		};
		for (size_t i = 0; i < (size_t)Leveller::ParamIndices::NumParams; ++i) {
			cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
		}

		mResponseGraph.OnRender(cfg);


	}


};

#endif
