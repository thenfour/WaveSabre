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

	class VstEditor : public  AEffEditor //AEffGUIEditor
	{
	public:
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
					//char b[200] = { 0 };
					//std::sprintf(b, "freq param changed : %f", paramValue);
					//OutputDebugStringA(b);
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

		// for boolean types
		bool WSImGuiParamCheckbox(VstInt32 id, const char* name) {
			float paramValue = GetEffectX()->getParameter((VstInt32)id);
			bool bval = ::WaveSabreCore::Helpers::ParamToBoolean(paramValue);
			bool r = false;
			r = ImGui::Checkbox(name, &bval);
			if (r) {
				GetEffectX()->setParameterAutomated(id, ::WaveSabreCore::Helpers::BooleanToParam(bval));
			}
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

			//float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
			//auto friendlyVal = ::WaveSabreCore::Helpers::ParamToVoiceMode(paramValue);
			//int enumIndex = (int)friendlyVal;

			//const char* elem_name = "ERROR";
			//int elementCount = (int)std::size(captions);
			//if ((enumIndex >= 0) && (enumIndex < elementCount)) {
			//	elem_name = captions[enumIndex];
			//}

			//ImGui::PushID(ctrlLabel);
			//ImGui::PushItemWidth(70);

			//ImGui::BeginGroup();

			//auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			//auto txt = std::string(ctrlLabel, end);
			//ImGui::Text("%s", txt.c_str());
			//if (ImGui::SliderInt("##slider", &enumIndex, 0, elementCount - 1, elem_name, ImGuiSliderFlags_AlwaysClamp)) {
			//	paramValue = ::WaveSabreCore::Helpers::VoiceModeToParam((::WaveSabreCore::VoiceMode)enumIndex);
			//	GetEffectX()->setParameterAutomated(paramID, Clamp01(paramValue));
			//}
			//ImGui::EndGroup();
			//ImGui::PopItemWidth();
			//ImGui::PopID();
		}

		void WSImGuiParamFilterType(VstInt32 paramID, const char* ctrlLabel) {
			static constexpr char const * const captions[] = { "LP", "HP", "BP","Notch" };
			WSImGuiParamEnumList(paramID, ctrlLabel, 4, captions);

			//float paramValue = GetEffectX()->getParameter((VstInt32)paramID);
			//auto friendlyVal = ::WaveSabreCore::Helpers::ParamToStateVariableFilterType(paramValue);
			//int enumIndex = (int)friendlyVal;

			//const char* elem_name = "ERROR";
			//int elementCount = (int)std::size(captions);

			//ImGui::PushID(ctrlLabel);
			//ImGui::PushItemWidth(70);

			//ImGui::BeginGroup();

			//auto end = ImGui::FindRenderedTextEnd(ctrlLabel, 0);
			//auto txt = std::string(ctrlLabel, end);
			//ImGui::Text("%s", txt.c_str());

			//if (ImGui::BeginListBox("##filterTypeListBox", CalcListBoxSize(0.25f + elementCount)))
			//{
			//	for (int n = 0; n < elementCount; n++)
			//	{
			//		const bool is_selected = (enumIndex == n);
			//		if (ImGui::Selectable(captions[n], is_selected)) {
			//			enumIndex = n;
			//			paramValue = ::WaveSabreCore::Helpers::StateVariableFilterTypeToParam((::WaveSabreCore::StateVariableFilterType)enumIndex);
			//			GetEffectX()->setParameterAutomated(paramID, Clamp01(paramValue));
			//		}

			//		// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			//		if (is_selected)
			//			ImGui::SetItemDefaultFocus();
			//	}
			//	ImGui::EndListBox();
			//}

			//ImGui::EndGroup();
			//ImGui::PopItemWidth();
			//ImGui::PopID();
		}


		//static void ImGui_ImplWin32_AddKeyEvent(ImGuiKey key, bool down, int native_keycode, int native_scancode = -1)
		//{
		//	ImGuiIO& io = ImGui::GetIO();
		//	io.AddKeyEvent(key, down);
		//	io.SetKeyEventNativeData(key, native_keycode, native_scancode); // To support legacy indexing (<1.87 user code)
		//	IM_UNUSED(native_scancode);
		//}

		////static bool IsModDown(int mod, VstKeyCode& keyCode)
		////{
		////	return !!(keyCode.modifier & mod);
		////}

		////static void ImGui_ImplWin32_UpdateKeyModifiers(bool is_down, VstKeyCode& keyCode)
		////{
		////	ImGuiIO& io = ImGui::GetIO();
		////	io.AddKeyEvent(ImGuiMod_Ctrl, IsModDown(MODIFIER_CONTROL, keyCode));
		////	io.AddKeyEvent(ImGuiMod_Shift, IsModDown(MODIFIER_SHIFT, keyCode));
		////	io.AddKeyEvent(ImGuiMod_Alt, IsModDown(MODIFIER_ALTERNATE, keyCode));
		////	io.AddKeyEvent(ImGuiMod_Super, IsModDown(MODIFIER_COMMAND, keyCode));
		////}

		//// Map VK_xxx to ImGuiKey_xxx.
		//static ImGuiKey ImGui_VSTKeyToImGuiKey(VstInt32 vkey)
		//{
		//	// todo: map VST virtual keys to ImGui keys.
		//	//return ImGuiKey_None;

		//	//	VKEY_CLEAR,
		//	//	VKEY_PAUSE,
		//	//	VKEY_NEXT,
		//	//	VKEY_SELECT,
		//	//	VKEY_PRINT,
		//	//	VKEY_SNAPSHOT,
		//	//	VKEY_HELP,
		//	//	VKEY_SEPARATOR,
		//	//	VKEY_NUMLOCK,
		//	//	VKEY_SCROLL,
		//	//	VKEY_EQUALS

		//	switch (vkey)
		//	{
		//	case VKEY_TAB: return ImGuiKey_Tab;
		//	case VKEY_LEFT: return ImGuiKey_LeftArrow;
		//	case VKEY_RIGHT: return ImGuiKey_RightArrow;
		//	case VKEY_UP: return ImGuiKey_UpArrow;
		//	case VKEY_DOWN: return ImGuiKey_DownArrow;
		//	case VKEY_PAGEUP: return ImGuiKey_PageUp;
		//	case VKEY_PAGEDOWN: return ImGuiKey_PageDown;
		//	case VKEY_HOME: return ImGuiKey_Home;
		//	case VKEY_END: return ImGuiKey_End;
		//	case VKEY_INSERT: return ImGuiKey_Insert;
		//	case VKEY_DELETE: return ImGuiKey_Delete;
		//	case VKEY_BACK: return ImGuiKey_Backspace;
		//	case VKEY_SPACE: return ImGuiKey_Space;
		//	case VKEY_ENTER: return ImGuiKey_Enter; // or VKEY_RETURN ?
		//	case VKEY_ESCAPE: return ImGuiKey_Escape;
		//	case VKEY_ADD: return ImGuiKey_KeypadAdd;
		//	case VKEY_SUBTRACT: return ImGuiKey_Minus; // ImGuiKey_KeypadSubtract ?
		//	case VKEY_F1: return ImGuiKey_F1;
		//	case VKEY_F2: return ImGuiKey_F2;
		//	case VKEY_F3: return ImGuiKey_F3;
		//	case VKEY_F4: return ImGuiKey_F4;
		//	case VKEY_F5: return ImGuiKey_F5;
		//	case VKEY_F6: return ImGuiKey_F6;
		//	case VKEY_F7: return ImGuiKey_F7;
		//	case VKEY_F8: return ImGuiKey_F8;
		//	case VKEY_F9: return ImGuiKey_F9;
		//	case VKEY_F10: return ImGuiKey_F10;
		//	case VKEY_F11: return ImGuiKey_F11;
		//	case VKEY_F12: return ImGuiKey_F12;
		//	case VKEY_NUMPAD0: return ImGuiKey_Keypad0;
		//	case VKEY_NUMPAD1: return ImGuiKey_Keypad1;
		//	case VKEY_NUMPAD2: return ImGuiKey_Keypad2;
		//	case VKEY_NUMPAD3: return ImGuiKey_Keypad3;
		//	case VKEY_NUMPAD4: return ImGuiKey_Keypad4;
		//	case VKEY_NUMPAD5: return ImGuiKey_Keypad5;
		//	case VKEY_NUMPAD6: return ImGuiKey_Keypad6;
		//	case VKEY_NUMPAD7: return ImGuiKey_Keypad7;
		//	case VKEY_NUMPAD8: return ImGuiKey_Keypad8;
		//	case VKEY_NUMPAD9: return ImGuiKey_Keypad9;
		//	case VKEY_DECIMAL: return ImGuiKey_Period; // ImGuiKey_KeypadDecimal?
		//	case VKEY_DIVIDE: return ImGuiKey_KeypadDivide;  // ImGuiKey_Slash ?
		//	case VKEY_MULTIPLY: return ImGuiKey_KeypadMultiply; // asterisk?
		//	case VKEY_SHIFT: return ImGuiKey_LeftShift;
		//	case VKEY_CONTROL: return ImGuiKey_LeftCtrl;
		//	case VKEY_ALT: return ImGuiKey_LeftAlt;

		//		//case VK_OEM_7: return ImGuiKey_Apostrophe;
		//		//case VK_OEM_COMMA: return ImGuiKey_Comma;
		//		//case VK_OEM_2: return ImGuiKey_Slash;
		//		//case VK_OEM_1: return ImGuiKey_Semicolon;
		//		//case VK_OEM_PLUS: return ImGuiKey_Equal;
		//		//case VK_OEM_4: return ImGuiKey_LeftBracket;
		//		//case VK_OEM_5: return ImGuiKey_Backslash;
		//		//case VK_OEM_6: return ImGuiKey_RightBracket;
		//		//case VK_OEM_3: return ImGuiKey_GraveAccent;
		//		//case VK_CAPITAL: return ImGuiKey_CapsLock;
		//		//case VK_SCROLL: return ImGuiKey_ScrollLock;
		//		//case VK_NUMLOCK: return ImGuiKey_NumLock;
		//		//case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
		//		//case VK_PAUSE: return ImGuiKey_Pause;
		//		//case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
		//		//case VK_LWIN: return ImGuiKey_LeftSuper;
		//		//case VK_RSHIFT: return ImGuiKey_RightShift;
		//		//case VK_RCONTROL: return ImGuiKey_RightCtrl;
		//		//case VK_RMENU: return ImGuiKey_RightAlt;
		//		//case VK_RWIN: return ImGuiKey_RightSuper;
		//		//case VK_APPS: return ImGuiKey_Menu;
		//		//case '0': return ImGuiKey_0;
		//		//case '1': return ImGuiKey_1;
		//		//case '2': return ImGuiKey_2;
		//		//case '3': return ImGuiKey_3;
		//		//case '4': return ImGuiKey_4;
		//		//case '5': return ImGuiKey_5;
		//		//case '6': return ImGuiKey_6;
		//		//case '7': return ImGuiKey_7;
		//		//case '8': return ImGuiKey_8;
		//		//case '9': return ImGuiKey_9;
		//		//case 'A': return ImGuiKey_A;
		//		//case 'B': return ImGuiKey_B;
		//		//case 'C': return ImGuiKey_C;
		//		//case 'D': return ImGuiKey_D;
		//		//case 'E': return ImGuiKey_E;
		//		//case 'F': return ImGuiKey_F;
		//		//case 'G': return ImGuiKey_G;
		//		//case 'H': return ImGuiKey_H;
		//		//case 'I': return ImGuiKey_I;
		//		//case 'J': return ImGuiKey_J;
		//		//case 'K': return ImGuiKey_K;
		//		//case 'L': return ImGuiKey_L;
		//		//case 'M': return ImGuiKey_M;
		//		//case 'N': return ImGuiKey_N;
		//		//case 'O': return ImGuiKey_O;
		//		//case 'P': return ImGuiKey_P;
		//		//case 'Q': return ImGuiKey_Q;
		//		//case 'R': return ImGuiKey_R;
		//		//case 'S': return ImGuiKey_S;
		//		//case 'T': return ImGuiKey_T;
		//		//case 'U': return ImGuiKey_U;
		//		//case 'V': return ImGuiKey_V;
		//		//case 'W': return ImGuiKey_W;
		//		//case 'X': return ImGuiKey_X;
		//		//case 'Y': return ImGuiKey_Y;
		//		//case 'Z': return ImGuiKey_Z;
		//	default: return ImGuiKey_None;
		//	}
		//}


		//void onKey(bool is_key_down, VstKeyCode& keyCode) {
		//	// Submit modifiers just in the weird case that they have changed without us getting the event below.
		//	//ImGui_ImplWin32_UpdateKeyModifiers(is_key_down, keyCode);

		//	// Obtain virtual key code
		//	// (keypad enter doesn't have its own... VK_RETURN with KF_EXTENDED flag means keypad enter, see IM_VK_KEYPAD_ENTER definition for details, it is mapped to ImGuiKey_KeyPadEnter.)
		//	//int vk = (int)wParam;
		//	//if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
		//	//	vk = IM_VK_KEYPAD_ENTER;

		//	// Submit key event
		//	const ImGuiKey key = ImGui_VSTKeyToImGuiKey(keyCode.virt);
		//	//const int scancode = (int)LOBYTE(HIWORD(lParam));
		//	char b[200] = { 0 };
		//	if (key != ImGuiKey_None) {
		//		std::sprintf(b, "sending ImGuiKey: %d (%s)", key, is_key_down ? "DOWN" : "UP");
		//		OutputDebugStringA(b);

		//		ImGui_ImplWin32_AddKeyEvent(key, is_key_down, keyCode.virt);
		//	}

		//	//ImGui::GetIO().AddKeyEvent(ImGuiMod_Super, is_key_down);

		//	// NOTE: VST does not send keydowns during a setcapture drag event

		//	// Submit individual left/right modifier events
		//	if (keyCode.virt == VKEY_SHIFT)
		//	{
		//		// Important: Shift keys tend to get stuck when pressed together, missing key-up events are corrected in ImGui_ImplWin32_ProcessKeyEventsWorkarounds()
		//		std::sprintf(b, " -> mod VKEY_SHIFT (%s)", is_key_down ? "DOWN" : "UP");
		//		OutputDebugStringA(b);

		//		ImGui::GetIO().AddKeyEvent(ImGuiMod_Shift, is_key_down);

		//		//ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftShift, is_key_down, keyCode.virt);
		//		//if (IsVkDown(VK_LSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftShift, is_key_down, VK_LSHIFT, scancode); }
		//		//if (IsVkDown(VK_RSHIFT) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightShift, is_key_down, VK_RSHIFT, scancode); }
		//	}
		//	else if (keyCode.virt == VKEY_CONTROL)
		//	{
		//		std::sprintf(b, " -> mod VKEY_CONTROL (%s)", is_key_down ? "DOWN" : "UP");
		//		OutputDebugStringA(b);

		//		ImGui::GetIO().AddKeyEvent(ImGuiMod_Ctrl, is_key_down);
		//		//ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, keyCode.virt);
		//		//if (IsVkDown(VK_LCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftCtrl, is_key_down, VK_LCONTROL, scancode); }
		//		//if (IsVkDown(VK_RCONTROL) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightCtrl, is_key_down, VK_RCONTROL, scancode); }
		//	}
		//	else if (keyCode.virt == VKEY_ALT)
		//	{
		//		std::sprintf(b, " -> mod VKEY_ALT (%s)", is_key_down ? "DOWN" : "UP");
		//		OutputDebugStringA(b);

		//		ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, is_key_down);
		//		//ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftAlt, is_key_down, keyCode.virt);
		//		//if (IsVkDown(VK_LMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_LeftAlt, is_key_down, VK_LMENU, scancode); }
		//		//if (IsVkDown(VK_RMENU) == is_key_down) { ImGui_ImplWin32_AddKeyEvent(ImGuiKey_RightAlt, is_key_down, VK_RMENU, scancode); }
		//	}
		//}

		virtual bool onKeyDown(VstKeyCode& keyCode) override {
			mLastKeyDown = keyCode;

			//char b[200] = { 0 };
			//std::sprintf(b, "onVSTKey Down : %d %C", keyCode.virt, (wchar_t)keyCode.character);
			//OutputDebugStringA(b);
			//onKey(true, keyCode);
			return false; // return true avoids Reaper acting like you made a mistake (ding sfx)
		}
		virtual bool onKeyUp(VstKeyCode& keyCode) override {
			mLastKeyUp = keyCode;

			//char b[200] = { 0 };
			//std::sprintf(b, "onVSTKey UP: %d %C", keyCode.virt, (wchar_t)keyCode.character);
			//OutputDebugStringA(b);

			//onKey(false, keyCode);

			if (keyCode.character > 0) {
				ImGui::GetIO().AddInputCharacterUTF16((unsigned short)keyCode.character);
				return true;
			}
			return false; // return true avoids Reaper acting like you made a mistake (ding sfx)
		}

	protected:

		VstKeyCode mLastKeyDown = {0};
		VstKeyCode mLastKeyUp = {0};

		AudioEffectX* GetEffectX() {
			return static_cast<AudioEffectX*>(this->effect);
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
