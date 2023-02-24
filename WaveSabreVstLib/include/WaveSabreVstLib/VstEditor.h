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
			virtual double ParamToDisplayValue(double param, void* capture) override {
				return ::WaveSabreCore::Helpers::ParamToVibratoFreq((float)param);
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::VibratoFreqToParam((float)value);
			}
		};

		struct FrequencyConverter : ImGuiKnobs::IValueConverter
		{
			virtual double ParamToDisplayValue(double param, void* capture) override {
				return ::WaveSabreCore::Helpers::ParamToFrequency((float)param);
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

			virtual double ParamToDisplayValue(double param, void* capture) override {
				mParam.SetParamValue((float)param);
				return (double)mParam.GetRangedValue();
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				mParam.SetRangedValue((float)value);
				return (double)mParam.GetRawParamValue();
			}
		};
		struct DbConverter : ImGuiKnobs::IValueConverter
		{
			virtual double ParamToDisplayValue(double param, void* capture) override {
				return ::WaveSabreCore::Helpers::ParamToDb((float)param);
			}
			virtual double DisplayValueToParam(double value, void* capture) {
				return ::WaveSabreCore::Helpers::DbToParam((float)value);
			}
		};

		struct FilterQConverter : ImGuiKnobs::IValueConverter
		{
			virtual double ParamToDisplayValue(double param, void* capture) override {
				return ::WaveSabreCore::Helpers::ParamToQ((float)param);
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
