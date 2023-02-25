#ifndef __WAVESABREVSTLIB_VSTEDITOR_H__
#define __WAVESABREVSTLIB_VSTEDITOR_H__

#include "Common.h"
#include "ImageManager.h"
#include "NoTextCOptionMenu.h"
#include <d3d9.h>
#include "../imgui/imgui.h"
#include "../imgui/imgui-knobs.h"
#include "../imgui/imgui_internal.h"
#include "../imgui/imfilebrowser.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include "VstPlug.h"

using real_t = WaveSabreCore::M7::real_t;

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{
	// making this an enum keeps the core from getting bloated. if we for example have some kind of behavior class passed in from the core, then those details will pollute core.
	enum class ParamBehavior
	{
		Default01,
		Unisono,
		VibratoFreq,
		Frequency,
		FilterQ,
		Db,
	};

	class VstEditor : public AEffEditor
	{
	public:

		static constexpr double gNormalKnobSpeed = 0.003f;
		static constexpr double gSlowKnobSpeed = 0.00005f;

		VstEditor(AudioEffect *audioEffect, int width, int height);
		virtual ~VstEditor();

		virtual void Window_Open(HWND parent);
		virtual void Window_Close();

		virtual bool getRect(ERect** ppRect) {
			*ppRect = &mDefaultRect;
			return true;
		}

		virtual void renderImgui() = 0;

		bool open(void* ptr) override
		{
			AEffEditor::open(ptr);
			Window_Open((HWND)ptr); 
			return true;
		}

		void close() override
		{
			Window_Close();
			AEffEditor::close();
		}

		struct VibratoFreqConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", (float)::WaveSabreCore::Helpers::ParamToVibratoFreq((float)param));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::VibratoFreqToParam((float)value);
			}
		};

		struct FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", (float)::WaveSabreCore::Helpers::ParamToFrequency((float)param));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::FrequencyToParam((float)value);
			}
		};

		struct ScaledFloatConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::ScaledRealParam mParam;

			ScaledFloatConverter(float v_min, float v_max) :
				mParam(mBacking, v_min, v_max, v_min)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", mParam.GetRangedValue());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				mParam.SetRangedValue((float)value);
				return (double)mParam.GetRawParamValue();
			}
		};

		struct Maj7IntConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::IntParam mParam;

			Maj7IntConverter(int v_min, int v_max) :
				mParam(mBacking, v_min, v_max, v_min)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%d", mParam.GetIntValue());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				mParam.SetIntValue((int)value);
				return (double)mParam.GetRawParamValue();
			}
		};

		struct Maj7FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			float mBackingKT;
			VstInt32 mKTParamID;
			WaveSabreCore::M7::FrequencyParam mParam;

			Maj7FrequencyConverter(M7::real_t centerFreq, VstInt32 ktParamID) :
				mParam(mBacking, mBackingKT, centerFreq, 0.5f, 0)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.mValue.SetParamValue((float)param);
				VstEditor* pThis = (VstEditor*)capture;
				if (mKTParamID >= 0) {
					mParam.mKTValue.SetParamValue(pThis->GetEffectX()->getParameter(mKTParamID));
				}

				char s[100] = { 0 };
				if (mParam.mKTValue.Get01Value() < 0.00001f) {
					M7::real_t hz = mParam.GetFrequency(0, 0);
					if (hz >= 1000) {
						sprintf_s(s, "%0.2fHz", hz);
					}
					if (hz >= 100) {
						sprintf_s(s, "%0.1fHz", hz);
					}
					else {
						sprintf_s(s, "%dHz", (int)(hz+0.5f));
					}
				}
				else {
					// with KT applied, the frequency doesn't really matter.
					sprintf_s(s, "%0.0f%%", mParam.mValue.Get01Value() * 100);
				}

				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct EnvTimeConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::EnvTimeParam mParam;

			EnvTimeConverter() :
				mParam(mBacking, 0)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				M7::real_t ms = mParam.GetMilliseconds();
				if (ms > 2000)
				{
					sprintf_s(s, "%0.1f s", ms / 1000);
				}
				else if (ms > 1000) {
					sprintf_s(s, "%0.2f s", ms / 1000);
				}
				else {
					sprintf_s(s, "%0.2f ms", ms);
				}
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				//mParam.SetRangedValue((float)value);
				//return (double)mParam.GetRawParamValue();
				return 0;
			}
		};

		struct FloatN11Converter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::FloatN11Param mParam;

			FloatN11Converter() :
				mParam(mBacking, 0)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", mParam.GetN11Value());
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct M7VolumeConverter : ImGuiKnobs::IValueConverter
		{
			float mBacking;
			WaveSabreCore::M7::VolumeParam mParam;

			M7VolumeConverter(M7::real_t maxDb) :
				mParam(mBacking, maxDb, maxDb)
			{
			}

			virtual std::string ParamToDisplayString(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				char s[100] = { 0 };
				if (mParam.IsSilent()) {
					return "-inf";
				}
				float db = mParam.GetDecibels();
				sprintf_s(s, "%c%0.2fdB", db < 0 ? '-' : '+', ::fabsf(mParam.GetDecibels()));
				return s;
			}

			virtual double DisplayValueToParam(double value, void* capture) {
				return 0;
			}
		};

		struct DbConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", ::WaveSabreCore::Helpers::ParamToDb((float)param));
				return s;
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::DbToParam((float)value);
			}
		};

		struct FilterQConverter : ImGuiKnobs::IValueConverter
		{
			virtual std::string ParamToDisplayString(double param, void* capture) override {
				char s[100] = { 0 };
				sprintf_s(s, "%0.2f", ::WaveSabreCore::Helpers::ParamToQ((float)param));
				return s;
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::QToParam((float)value);
			}
		};

		// case ParamIndices::LoopMode: return (float)loopMode / (float)((int)LoopMode::NumLoopModes - 1);
		//template<typename T>
		float EnumToParam(int value, int valueCount, int baseVal = 0) {
			return (float)(value - baseVal) / (valueCount - 1);
		}

		// case ParamIndices::LoopMode: loopMode = (LoopMode)(int)(value * (float)((int)LoopMode::NumLoopModes - 1)); break;
		//template<typename T>
		int ParamToEnum(float value, int valueCount, int baseVal = 0) {
			if (value < 0) value = 0;
			if (value > 1) value = 1;
			return baseVal + (int)(value * (valueCount - 1));
		}

		bool WSImGuiParamKnob(VstInt32 id, const char* name, ParamBehavior behavior = ParamBehavior::Default01, const char* fmt = "%.3f") {
			float paramValue = GetEffectX()->getParameter((VstInt32)id);
			bool r = false;
			switch (behavior) {
			case ParamBehavior::Unisono:
			{
				int iparam = ::WaveSabreCore::Helpers::ParamToUnisono(paramValue);
				r = ImGuiKnobs::KnobInt(name, &iparam, 1, 16, 1, .1f, .001f, NULL, ImGuiKnobVariant_WiperOnly);
				if (r) {
					paramValue = ::WaveSabreCore::Helpers::UnisonoToParam(iparam);
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::VibratoFreq:
			{
				static VibratoFreqConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::Db:
			{
				static DbConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::FilterQ:
			{
				static FilterQConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			
			case ParamBehavior::Frequency:
			{
				static FrequencyConverter conv;
				r = ImGuiKnobs::Knob(name, &paramValue, 0.0f, 1.0f, 0.5f, 0.003f, 0.0001f, "%.2fHz", ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this);

				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			case ParamBehavior::Default01:
			default:
			{
				r = ImGuiKnobs::Knob(name, &paramValue, 0, 1, 0.5f, 0.003f, 0.0001f, fmt, ImGuiKnobVariant_WiperOnly);
				if (r) {
					GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
				}
				break;
			}
			}
			return r;
		}

		void WSImGuiParamKnobInt(VstInt32 id, const char* name, int v_min, int v_max, int v_default) {
			float paramValue = GetEffectX()->getParameter((VstInt32)id);
			int v_count = v_max - v_min + 1;
			int iparam = ParamToEnum(paramValue, v_count, v_min);
			if (ImGuiKnobs::KnobInt(name, &iparam, v_min, v_max, v_default, 0.003f * v_count, 0.0001f * v_count, NULL, ImGuiKnobVariant_WiperOnly))
			{
				paramValue = EnumToParam(iparam, v_count, v_min);
				GetEffectX()->setParameterAutomated(id, Clamp01(paramValue));
			}
		}


		float Clamp01(float x)
		{
			if (x < 0) return 0;
			if (x > 1) return 1;
			return x;
		}

		std::string GetRenderedTextFromID(const char* id) {
			auto end = ImGui::FindRenderedTextEnd(id, 0);
			return std::string(id, end);
		}

		// for boolean types
		bool WSImGuiParamCheckbox(VstInt32 paramId, const char* id) {
			ImGui::BeginGroup();
			ImGui::PushID(id);
			float paramValue = GetEffectX()->getParameter((VstInt32)paramId);
			bool bval = ::WaveSabreCore::Helpers::ParamToBoolean(paramValue);
			bool r = false;
			ImGui::Text(GetRenderedTextFromID(id).c_str());
			r = ImGui::Checkbox("##cb", &bval);
			if (r) {
				GetEffectX()->setParameterAutomated(paramId, ::WaveSabreCore::Helpers::BooleanToParam(bval));
			}
			ImGui::PopID();
			ImGui::EndGroup();
			return r;
		}

		ImVec2 CalcListBoxSize(float items)
		{
			return { 0.0f, ImGui::GetTextLineHeightWithSpacing() * items + ImGui::GetStyle().FramePadding.y * 2.0f};
		}

		//template<typename Tenum>
		void WSImGuiParamEnumList(VstInt32 paramID, const char* ctrlLabel, int elementCount, const char * const * const captions) {
			//const char* captions[] = { "LP", "HP", "BP","Notch" };

			float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
			auto friendlyVal = ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			int enumIndex = (int)friendlyVal;

			const char* elem_name = "ERROR";
			//int elementCount = (int)std::size(captions);

			ImGui::PushID(ctrlLabel);
			ImGui::PushItemWidth(70);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			ImGui::Text("%s", txt.c_str());

			if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + elementCount)))
			{
				for (int n = 0; n < elementCount; n++)
				{
					const bool is_selected = (enumIndex == n);
					if (ImGui::Selectable(captions[n], is_selected)) {
						enumIndex = n;
						paramValue = EnumToParam(n, elementCount);// ::WaveSabreCore::Helpers::StateVariableFilterTypeToParam((::WaveSabreCore::StateVariableFilterType)enumIndex);
						GetEffectX()->setParameterAutomated(paramID, Clamp01(paramValue));
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			ImGui::EndGroup();
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		void WSImGuiParamVoiceMode(VstInt32 paramID, const char* ctrlLabel) {
			static constexpr char const * const captions[] = { "Poly", "Mono" };
			WSImGuiParamEnumList(paramID, ctrlLabel, 2, captions);
		}

		void WSImGuiParamFilterType(VstInt32 paramID, const char* ctrlLabel) {
			static constexpr char const * const captions[] = { "LP", "HP", "BP","Notch" };
			WSImGuiParamEnumList(paramID, ctrlLabel, 4, captions);
		}

		// For the Maj7 synth, let's add more param styles.
		template<typename Tenum>
		void Maj7ImGuiParamEnumList(VstInt32 paramID, const char* ctrlLabel, int elementCount, Tenum defaultVal, const char* const* const captions) {
			M7::real_t tempVal;
			M7::EnumParam<Tenum> p(tempVal, Tenum(elementCount), Tenum(0) );
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
			auto friendlyVal = p.GetEnumValue();// ParamToEnum(paramValue, elementCount); //::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			int enumIndex = (int)friendlyVal;

			const char* elem_name = "ERROR";

			ImGui::PushID(ctrlLabel);
			ImGui::PushItemWidth(70);

			ImGui::BeginGroup();

			auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			auto txt = std::string(ctrlLabel, end);
			ImGui::Text("%s", txt.c_str());

			if (ImGui::BeginListBox("##enumlist", CalcListBoxSize(0.2f + elementCount)))
			{
				for (int n = 0; n < elementCount; n++)
				{
					const bool is_selected = (enumIndex == n);
					if (ImGui::Selectable(captions[n], is_selected)) {
						p.SetEnumValue((Tenum)n);
						GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
					}

					// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
					if (is_selected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndListBox();
			}

			ImGui::EndGroup();
			ImGui::PopItemWidth();
			ImGui::PopID();
		}

		void Maj7ImGuiParamScaledFloat(VstInt32 paramID, const char* label, M7::real_t v_min, M7::real_t v_max, M7::real_t v_defaultScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::ScaledRealParam p{ tempVal , v_min, v_max, v_defaultScaled };
			float defaultParamVal = p.Get01Value();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			ScaledFloatConverter conv{v_min, v_max};
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamInt(VstInt32 paramID, const char* label, int v_min, int v_max, int v_defaultScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::IntParam p{ tempVal , v_min, v_max, v_defaultScaled };
			float defaultParamVal = p.Get01Value();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7IntConverter conv{ v_min, v_max };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFrequency(VstInt32 paramID, VstInt32 ktParamID, const char* label, M7::real_t centerFreq, M7::real_t defaultParamValue) {
			M7::real_t tempVal;
			M7::real_t tempValKT;
			M7::FrequencyParam p{ tempVal, tempValKT, centerFreq, defaultParamValue, 0 };
			p.mValue.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			Maj7FrequencyConverter conv{ centerFreq, ktParamID };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamValue, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamEnvTime(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::EnvTimeParam p{ tempVal , v_defaultScaled };
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			EnvTimeConverter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamFloatN11(VstInt32 paramID, const char* label, M7::real_t v_defaultScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::FloatN11Param p{ tempVal, v_defaultScaled };
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			FloatN11Converter conv{ };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void Maj7ImGuiParamVolume(VstInt32 paramID, const char* label, M7::real_t maxDb, M7::real_t v_defaultScaled) {
			WaveSabreCore::M7::real_t tempVal;
			M7::VolumeParam p{ tempVal, maxDb, v_defaultScaled };
			float defaultParamVal = p.GetRawParamValue();
			p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));

			M7VolumeConverter conv{ maxDb };
			if (ImGuiKnobs::Knob(label, &tempVal, 0, 1, defaultParamVal, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput, 10, &conv, this))
			{
				GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
			}
		}

		void AddCurveToPath(ImDrawList* dl, ImVec2 pos, ImVec2 size, bool invertCurveParam, const M7::CurveParam& param, ImU32 color, float thickness, int segments = 16)
		{
			//if (invertY) {
			//	pos.y += size.y;
			//	size.y = -size.y;
			//}
			ImVec2 p = pos;
			segments = std::max(segments, 2);

			// begin point
			dl->PathLineTo(pos);
			//cc::log("AddCurveToPath, size=%f,%f  curveParam=%f, color=%08x, thickness=%f", size.x, size.y, param.GetRawParamValue(), color, thickness);
			//cc::log("  line to %f, %f", pos.x, pos.y);

			//segments -= 2;
			for (int i = 0; i < segments; ++i) {
				//float x1 = float(i) / segments; // the beginning of the line
				float x2 = float(i + 1) / segments; // end of the line
				//float xc = (x1 + x2) / 2.0f;
				//float y1 = param.ApplyToValue(x1);
				float y2 = param.ApplyToValue(invertCurveParam ? 1.0f - x2 : x2);
				y2 = invertCurveParam ? 1.0f - y2 : y2;
				ImVec2 e = { pos.x + size.x * x2, pos.y + y2 * size.y };
				dl->PathLineTo(e);
				//cc::log("  line to %f, %f", e.x, e.y);
				//dl->AddLine();
			}

			//dl->PathLineTo({ pos.x + size.x, pos.y + size.y }); // end point
			dl->PathStroke(color, 0, thickness);
		}

		template<typename Tenum>
		void Maj7ImGuiEnvelopeGraphic(const char* label,
			Tenum delayTimeParamID, Tenum attackTimeID, Tenum attackCurveID,
			Tenum holdTimeID, Tenum decayTimeID, Tenum decayCurveID,
			Tenum sustainLevelID, Tenum releaseTimeID, Tenum releaseCurveID)
		{
			static constexpr float gSegmentMaxWidth = 70;
			static constexpr float gMaxWidth = gSegmentMaxWidth * 6;
			static constexpr float innerHeight = 60; // excluding padding
			static constexpr float padding = 7;
			static constexpr float gLineThickness = 2.7f;
			ImU32 lineColor = ImGui::GetColorU32(ImGuiCol_PlotLines);
			static constexpr float handleRadius = 3;
			static constexpr int circleSegments = 7;

			float delayWidth = M7::Clamp(GetEffectX()->getParameter((VstInt32)delayTimeParamID), 0, 1) * gSegmentMaxWidth;
			float attackWidth = M7::Clamp(GetEffectX()->getParameter((VstInt32)attackTimeID), 0, 1) * gSegmentMaxWidth;
			float holdWidth = M7::Clamp(GetEffectX()->getParameter((VstInt32)holdTimeID), 0, 1) * gSegmentMaxWidth;
			float decayWidth = M7::Clamp(GetEffectX()->getParameter((VstInt32)decayTimeID), 0, 1) * gSegmentMaxWidth;
			float sustainWidth = gSegmentMaxWidth;
			float releaseWidth = M7::Clamp(GetEffectX()->getParameter((VstInt32)releaseTimeID), 0, 1) * gSegmentMaxWidth;

			float sustainLevel = M7::Clamp(GetEffectX()->getParameter((VstInt32)sustainLevelID), 0, 1);
			float sustainYOffset = innerHeight * (1.0f - sustainLevel);

			float attackCurveRaw;
			M7::CurveParam attackCurve{ attackCurveRaw, 0 };
			attackCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)attackCurveID));

			float decayCurveRaw;
			M7::CurveParam decayCurve{ decayCurveRaw, 0 };
			decayCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)decayCurveID));

			float releaseCurveRaw;
			M7::CurveParam releaseCurve{ releaseCurveRaw, 0 };
			releaseCurve.SetParamValue(GetEffectX()->getParameter((VstInt32)releaseCurveID));

			ImVec2 originalPos = ImGui::GetCursorScreenPos();
			ImVec2 p = originalPos;
			p.x += padding;
			p.y += padding;
			// D A H D S R
			ImDrawList* dl = ImGui::GetWindowDrawList();

			// background
			ImVec2 outerTL = originalPos;
			ImVec2 innerTL = { outerTL.x + padding, outerTL.y + padding };
			ImVec2 outerBottomLeft = { outerTL.x, outerTL.y + innerHeight + padding * 2 };
			ImVec2 outerBottomRight = { outerTL.x + gMaxWidth + padding * 2, outerTL.y + innerHeight + padding * 2 };
			ImVec2 innerBottomLeft = { innerTL.x, innerTL.y + innerHeight };
			ImVec2 innerBottomRight = { innerBottomLeft.x + gMaxWidth, innerBottomLeft.y };
			//dl->AddRectFilled(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg));
			//dl->AddRect(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_Frame));
			ImGui::RenderFrame(outerTL, outerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg));
			//dl->AddRectFilled(innerTL, innerBottomRight, ImGui::GetColorU32(ImGuiCol_FrameBg));

			//ImVec2 startPos = innerBottomLeft;// { innerTL.x, innerTL.y + innerHeight };
			ImVec2 delayStart = innerBottomLeft;
			ImVec2 delayEnd = { delayStart.x + delayWidth, delayStart.y };
			dl->AddLine(delayStart, delayEnd, lineColor, gLineThickness);
			//dl->PathLineTo(delayStart);
			//dl->PathLineTo(delayEnd); // delay flat line

			ImVec2 attackEnd = { delayEnd.x + attackWidth, innerTL.y };
			AddCurveToPath(dl, delayEnd, { attackEnd.x - delayEnd.x, attackEnd.y - delayEnd.y }, false, attackCurve, lineColor, gLineThickness);

			ImVec2 holdEnd = { attackEnd.x + holdWidth, attackEnd.y };
			//dl->PathLineTo(holdEnd);
			dl->AddLine(attackEnd, holdEnd, lineColor, gLineThickness);

			ImVec2 decayEnd = { holdEnd.x + decayWidth, innerTL.y + sustainYOffset };
			AddCurveToPath(dl, holdEnd, { decayEnd.x - holdEnd.x, decayEnd.y - holdEnd.y }, true, decayCurve, lineColor, gLineThickness);

			ImVec2 sustainEnd = { decayEnd.x + sustainWidth, decayEnd.y};
			//dl->PathLineTo(sustainEnd); // sustain flat line
			dl->AddLine(decayEnd, sustainEnd, lineColor, gLineThickness);

			ImVec2 releaseEnd = { sustainEnd.x + releaseWidth, innerBottomLeft.y };
			AddCurveToPath(dl, sustainEnd, { releaseEnd.x - sustainEnd.x, releaseEnd.y - sustainEnd.y }, true, releaseCurve, lineColor, gLineThickness);

			dl->AddCircleFilled(delayStart, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(delayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(attackEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(holdEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(decayEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(sustainEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);
			dl->AddCircleFilled(releaseEnd, handleRadius, ImGui::GetColorU32(ImGuiCol_PlotLinesHovered), circleSegments);

			//auto pathCopy = dl->_Path;
			//dl->PathLineTo(delayStart);
			//dl->PathFillConvex(ImGui::GetColorU32(ImGuiCol_Button));

			//dl->_Path = pathCopy;
			//dl->PathStroke(ImGui::GetColorU32(ImGuiCol_PlotHistogram), ImDrawFlags_None, gThickness);

			ImGui::Dummy({ outerBottomRight.x - originalPos.x, outerBottomRight.y - originalPos.y });
		}

		virtual bool onKeyDown(VstKeyCode& keyCode) override {
			mLastKeyDown = keyCode;
			return false; // return true avoids Reaper acting like you made a mistake (ding sfx)
		}
		virtual bool onKeyUp(VstKeyCode& keyCode) override {
			mLastKeyUp = keyCode;
			if (keyCode.character > 0) {
				ImGui::GetIO().AddInputCharacterUTF16((unsigned short)keyCode.character);
				return true;
			}
			return false; // return true avoids Reaper acting like you made a mistake (ding sfx)
		}

	protected:

		VstKeyCode mLastKeyDown = {0};
		VstKeyCode mLastKeyUp = {0};

		VstPlug* GetEffectX() {
			return static_cast<VstPlug*>(this->effect);
		}

		LPDIRECT3D9              g_pD3D = NULL;
		LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
		D3DPRESENT_PARAMETERS    g_d3dpp = {};

		bool CreateDeviceD3D(HWND hWnd);

		void CleanupDeviceD3D();

		void ResetDevice();

		HWND mParentWindow = nullptr;
		HWND mCurrentWindow = nullptr;

		void ImguiPresent();

		static LRESULT WINAPI gWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		ERect mDefaultRect = { 0 };
		bool showingDemo = false;
		bool mIsRendering = false;
	};


}

#endif
