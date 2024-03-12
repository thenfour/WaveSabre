#ifndef __ECHOEDITOR_H__
#define __ECHOEDITOR_H__

#include <functional>
#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "EchoVst.h"

class EchoEditor : public VstEditor
{
	Echo* mpEcho;
	EchoVst* mpEchoVST;
public:
	EchoEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 570, 460),
		mpEchoVST((EchoVst*)audioEffect)//,
	{
		mpEcho = (Echo*)mpEchoVST->getDevice(); // for some reason this doesn't work as initialization but has to be called in ctor body like this.
	}

	virtual void PopulateMenuBar() override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Echo", mpEcho, mpEchoVST, "gDefaults16", "Echo::ParamIndices::NumParams", mpEcho->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::BeginGroup();

		Maj7ImGuiParamInt((int)Echo::ParamIndices::LeftDelayCoarse, "Left 8ths", Echo::gDelayCoarseCfg, 8, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::LeftDelayFine, "(fine)##left", 0, 0, {});
		//ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)Echo::ParamIndices::LeftDelayMS, "(ms)##left", -200, 200, 0, 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiBipolarPowCurvedParam((int)Echo::ParamIndices::LeftDelayMS, "(ms)##left", M7::gEnvTimeCfg, 0, {});

		ImGui::SameLine(0, 60); Maj7ImGuiParamInt((int)Echo::ParamIndices::RightDelayCoarse, "Right 8ths", Echo::gDelayCoarseCfg, 6, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)Echo::ParamIndices::RightDelayFine, "(fine)##right", 0, 0, {});
		//ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)Echo::ParamIndices::RightDelayMS, "(ms)##right", -200, 200, 0, 0, 0, {});
		ImGui::SameLine(); Maj7ImGuiBipolarPowCurvedParam((int)Echo::ParamIndices::RightDelayMS, "(ms)##right", M7::gEnvTimeCfg, 0, {});

		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackLevel, "Feedback", M7::gVolumeCfg6db, -15.0f, {});
		ImGui::SameLine(); Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::FeedbackDriveDB, "FB Drive", M7::gVolumeCfg12db, 3, {});
		ImGui::SameLine();
		Maj7ImGuiParamFloat01((VstInt32)WaveSabreCore::Echo::ParamIndices::Cross, "crosstalk", 0.25f, 0);

		//float f1 = 0, f2 = 0;
		//M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };
		//fp.SetFrequencyAssumingNoKeytracking(50);
		M7::QuickParam fp{ M7::gFilterFreqConfig };
		fp.SetFrequencyAssumingNoKeytracking(50);

		Maj7ImGuiParamFrequency((int)Echo::ParamIndices::LowCutFreq, -1, "Lowcut", M7::gFilterFreqConfig, fp.GetRawValue(), {});
		//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)Echo::ParamIndices::LowCutQ, "Low Q", 0.2f, 0.2f);
		ImGui::SameLine(); Maj7ImGuiDivCurvedParam((int)Echo::ParamIndices::LowCutQ, "Low Q", M7::gBiquadFilterQCfg, 0.75f, {});

		fp.SetFrequencyAssumingNoKeytracking(8500);
		ImGui::SameLine(); Maj7ImGuiParamFrequency((int)Echo::ParamIndices::HighCutFreq, -1, "Highcut", M7::gFilterFreqConfig, fp.GetRawValue(), {});
		//ImGui::SameLine(); Maj7ImGuiParamFloat01((int)Echo::ParamIndices::HighCutQ, "High Q", 0.2f, 0.2f);
		ImGui::SameLine(); Maj7ImGuiDivCurvedParam((int)Echo::ParamIndices::HighCutQ, "High Q", M7::gBiquadFilterQCfg, 0.75f, {});

		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::DryOutput, "Dry output", M7::gVolumeCfg12db, 0, {});
		ImGui::SameLine();
		Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Echo::ParamIndices::WetOutput, "Wet output", M7::gVolumeCfg12db, -12, {});

		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter("vu_inp", mpEcho->mInputAnalysis[0], mpEcho->mInputAnalysis[1]);
		ImGui::SameLine(); VUMeter("vu_outp", mpEcho->mOutputAnalysis[0], mpEcho->mOutputAnalysis[1]);

		RenderViz();
	}

	void RenderViz()
	{
		ImRect bb;
		bb.Min = ImGui::GetCursorScreenPos();
		ImVec2 size{ 500, 100 };
		bb.Max = bb.Min + size;
		auto dl = ImGui::GetWindowDrawList();

		static constexpr float beatsToDisplay = 6;
		static constexpr float beatsOffsetX = 0.5f;
		static constexpr float beginBeat = -beatsOffsetX;
		static constexpr float endBeat = beatsToDisplay - beatsOffsetX;
		auto beatsToX = [&](float beats) {
			return M7::math::map(beats, -beatsOffsetX, endBeat, bb.Min.x, bb.Max.x);
		};
		auto panToY = [&](float panN11) {
			return M7::math::map(panN11, -2, 2, bb.Min.y, bb.Max.y);
		};
		auto msToBeats = [&](float ms) {
			float msPerBeat = 60000.0f / Helpers::CurrentTempo;
			return ms / msPerBeat;
		};

		ImGui::RenderFrame(bb.Min, bb.Max, ColorFromHTML("222222"));
		ImGui::InvisibleButton("viz", size);

		// render beat grid
		static constexpr float subbeats = 8;
		for (int i = 0; i < beatsToDisplay; ++i) {
			if (i < endBeat) {
				float x = std::round(beatsToX(float(i)));
				dl->AddLine({ x, bb.Min.y }, { x, bb.Max.y }, ColorFromHTML("777777"), 2);
			}
			for (int is = 0; is < subbeats - 1; ++is) {
				float beats = (float)i + ((is + 1) / subbeats);
				float x = std::round(beatsToX(beats));
				if (beats < endBeat) {
					dl->AddLine({ x, bb.Min.y }, { x, bb.Min.y + 15 }, ColorFromHTML("555555", 0.5f), 1);
				}
			}
		}

		// render primary
		static constexpr float mainRadius = 20;

		// render further.
		float backing[]{
			mpEchoVST->getParameter((int)Echo::ParamIndices::LeftDelayCoarse),
			mpEchoVST->getParameter((int)Echo::ParamIndices::LeftDelayFine),
			mpEchoVST->getParameter((int)Echo::ParamIndices::LeftDelayMS),
			mpEchoVST->getParameter((int)Echo::ParamIndices::RightDelayCoarse),
			mpEchoVST->getParameter((int)Echo::ParamIndices::RightDelayFine),
			mpEchoVST->getParameter((int)Echo::ParamIndices::RightDelayMS),
			mpEchoVST->getParameter((int)Echo::ParamIndices::Cross),
		};
		M7::ParamAccessor pa{ backing, 0 };

		float leftPeriodMS = mpEcho->CalcDelayMS(
			pa.GetIntValue(0, Echo::gDelayCoarseCfg),
			pa.GetN11Value(1, 0),
			pa.GetBipolarPowCurvedValue(2, M7::gEnvTimeCfg, 0)
			//pa.GetScaledRealValue(2, -200, 200, 0)
		);
		float rightPeriodMS = mpEcho->CalcDelayMS(
			pa.GetIntValue(3, Echo::gDelayCoarseCfg),
			pa.GetN11Value(4, 0),
			pa.GetBipolarPowCurvedValue(5, M7::gEnvTimeCfg, 0)
			//pa.GetScaledRealValue(5, -200, 200, 0)
		);
		float crossMix10 = backing[6];

		float leftPeriodBeats = msToBeats(leftPeriodMS);
		float leftBeat = 0;
		float leftRadius = mainRadius;
		float panL = -1;
		float panR = 1;
		int i = 0;
		while (leftBeat < endBeat) {
			dl->AddLine({ beatsToX(leftBeat), bb.Min.y }, { beatsToX(leftBeat), bb.Max.y }, ColorFromHTML("5555ff", 0.4f), 1);
			dl->AddCircleFilled({ beatsToX(leftBeat), panToY(panL) }, leftRadius, ColorFromHTML("5555ff", 0.8f));

			leftBeat += leftPeriodBeats;
			leftRadius *= 0.8f;
			i++;
			if (leftPeriodMS < 2 || i > 100) break;
			float xpanL = M7::math::lerp(panL, panR, crossMix10);
			panR = M7::math::lerp(panR, panL, crossMix10);
			panL = xpanL;
		}

		float rightPeriodBeats = msToBeats(rightPeriodMS);
		float rightBeat = 0;
		float rightRadius = mainRadius;
		panL = -1;
		panR = 1;
		i = 0;
		while (rightBeat < endBeat) {
			dl->AddLine({ beatsToX(rightBeat), bb.Min.y }, { beatsToX(rightBeat), bb.Max.y }, ColorFromHTML("ff3333", 0.2f), 1);
			dl->AddCircleFilled({ beatsToX(rightBeat), panToY(panR) }, rightRadius, ColorFromHTML("ff3333", 0.8f));

			rightBeat += rightPeriodBeats;
			rightRadius *= 0.8f;
			i++;
			if (rightPeriodMS < 2 || i > 100) break;
			float xpanL = M7::math::lerp(panL, panR, crossMix10);
			panR = M7::math::lerp(panR, panL, crossMix10);
			panL = xpanL;
		}

		char s[100];
		std::sprintf(s, "%d ms left\r\n%d ms right", (int)std::round(leftPeriodMS), (int)std::round(rightPeriodMS));

		dl->AddText(bb.Min, ColorFromHTML("999999"), s);

	}

}; // EchoEditor

#endif
