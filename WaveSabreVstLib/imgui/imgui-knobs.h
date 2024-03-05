#pragma once

// adaptation from https://github.com/altschuler/imgui-knobs

// TODO: disabled state
// TODO: color schemes
// TODO: modulation indicators

#include <cstdlib>
#include <string>
#include <WaveSabreCore/Maj7.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui.h"

// correction for windows.h macros.
// i cannot use NOMINMAX, because VST and Gdiplus depend on these macros. 
#undef min
#undef max

#include <algorithm>

using std::min;
using std::max;

struct ColorMod
{
	float mHueAdd;
	float mSaturationMul;
	float mValueMul;
	float mTextValue;
	float mTextDisabledValue;

	bool mInitialized = false;
	ImVec4 mNewColors[ImGuiCol_COUNT];

	struct Token
	{
		ImVec4 mOldColors[ImGuiCol_COUNT];
		bool isSet = false;
		Token(ImVec4* newColors) {
			isSet = !!newColors;
			if (isSet) {
				memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
				memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
			}
		}
		Token(Token&& rhs)
		{
			isSet = rhs.isSet;
			rhs.isSet = false;
			memcpy(mOldColors, rhs.mOldColors, sizeof(mOldColors));
		}
		Token& operator =(Token&& rhs)
		{
			isSet = rhs.isSet;
			rhs.isSet = false;
			memcpy(mOldColors, rhs.mOldColors, sizeof(mOldColors));
			return *this;
		}
		Token() {

		}
		~Token() {
			if (isSet) {
				memcpy(ImGui::GetStyle().Colors, mOldColors, sizeof(mOldColors));
			}
		}
	};
	Token Push()
	{
		if (!this->mInitialized) {
			return { nullptr };
		}
		return { mNewColors };
	}

	static ImColor GetModifiedColor(ImVec4 orig, float mHueAdd, float mSaturationMul, float mValueMul)
	{
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(orig.x, orig.y, orig.z, h, s, v);
		auto newcolor = ImColor::HSV(WaveSabreCore::M7::math::fract(h + mHueAdd), WaveSabreCore::M7::math::clamp01(s * mSaturationMul), WaveSabreCore::M7::math::clamp01(v * mValueMul));
		return newcolor;
	}

	void EnsureInitialized() {
		if (mInitialized) return;
		ImGuiStyle& style = ImGui::GetStyle();

		// correct some things in default style.
		{
			ImVec4 color = style.Colors[ImGuiCol_TabActive];
			float h, s, v;
			ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
			auto newcolor = ImColor::HSV(h, s, 1);
			style.Colors[ImGuiCol_TabActive] = newcolor.Value;
		}

		mInitialized = true;
		for (size_t i = 0; i < ImGuiCol_COUNT; ++i) {
			ImVec4 color = style.Colors[i];
			//float h, s, v;
			//ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
			//auto newcolor = ImColor::HSV(h + mHueAdd, s * mSaturationMul, v * mValueMul);
			mNewColors[i] = GetModifiedColor(color, mHueAdd, mSaturationMul, mValueMul);// newcolor.Value;
		}
		mNewColors[ImGuiCol_Text] = ImColor::HSV(0, 0, mTextValue);
		mNewColors[ImGuiCol_TextDisabled] = ImColor::HSV(0, 0, mTextDisabledValue);
	}

	ColorMod(float hueAdd, float satMul, float valMul, float textVal, float textDisabledVal) :
		mHueAdd(hueAdd),
		mSaturationMul(satMul),
		mValueMul(valMul),
		mTextValue(textVal),
		mTextDisabledValue(textDisabledVal)
	{
	}

	// really just for the NOP color
	ColorMod() :
		mHueAdd(0),
		mSaturationMul(1),
		mValueMul(1),
		mTextValue(.8f),
		mTextDisabledValue(.4f)
	{
	}
};


typedef int ImGuiKnobFlags;

enum ImGuiKnobFlags_ {
	ImGuiKnobFlags_None = 0,
	ImGuiKnobFlags_NoTitle = 1 << 0,
	ImGuiKnobFlags_NoInput = 1 << 1,
	ImGuiKnobFlags_ValueTooltip = 1 << 2,
	ImGuiKnobFlags_DragHorizontal = 1 << 3,
	ImGuiKnobFlags_CustomInput = 1 << 4,
	ImGuiKnobFlags_InvertXCurve = 1 << 5,
	ImGuiKnobFlags_InvertYCurve = 1 << 6,
	ImGuiKnobFlags_NoKnob = 1 << 7,
	ImGuiKnobFlags_EditableTitle = 1 << 8,
};

typedef int ImGuiKnobVariant;

enum ImGuiKnobVariant_ {
    ImGuiKnobVariant_Tick = 1 << 0,
    ImGuiKnobVariant_Dot = 1 << 1,
    ImGuiKnobVariant_Wiper = 1 << 2,
    ImGuiKnobVariant_WiperOnly = 1 << 3,
    ImGuiKnobVariant_WiperDot = 1 << 4,
    ImGuiKnobVariant_Stepped = 1 << 5,
    ImGuiKnobVariant_Space = 1 << 6,
    ImGuiKnobVariant_M7Curve = 1 << 7,
    ImGuiKnobVariant_ProgressBar = 1 << 8,
	ImGuiKnobVariant_ProgressBarWithValue = 1 << 9,
};


inline static void AddCurveToPath(ImDrawList* dl, ImVec2 pos, ImVec2 size, bool invertX, bool invertY, const WaveSabreCore::M7::CurveParam& param, ImU32 color, float thickness, int segments = 16)
{
    ImVec2 p = pos;
    segments = std::max(segments, 2);

    // begin point
    if (invertX != invertY) {
        // these variations start in the bottom left.
        p.y += size.y;
    }
    dl->PathLineTo(p);

    for (int i = 0; i < segments; ++i) {
        float x2 = float(i + 1) / segments; // end of the line
        float y2 = param.ApplyToValue(invertX ? 1.0f - x2 : x2);
        y2 = invertY ? 1.0f - y2 : y2;
        ImVec2 e = { pos.x + size.x * x2, pos.y + y2 * size.y };
        dl->PathLineTo(e);
    }

    dl->PathStroke(color, 0, thickness);
}


namespace ImGuiKnobs {

    // use just to generate a function ptr type that's similar to the syntax you'd use for std::function.
    template <typename Tsignature>
    struct function
    {
        using ptr_t = Tsignature*;

    private:
        function() = delete;
    };


    struct color_set {
        ImColor base;
        ImColor hovered;
        ImColor active;

        color_set(ImColor base, ImColor hovered, ImColor active) : base(base), hovered(hovered), active(active) {}

        color_set(ImColor color) {
            base = color;
            hovered = color;
            active = color;
        }
    };

    // converts 0-1 parameter values to a display value.
    struct IValueConverter
    {
        virtual std::string ParamToDisplayString(double param, void* capture) = 0;
        virtual double DisplayValueToParam(double param, void* capture) = 0;
    };

    struct ModInfo
    {
        bool mEnabled = false;
        float mValue = 0; // relative to param value.
        float mExtentMin = 0; // relative to param value.
        float mExtentMax = 0; // relative to param value.
    };

	bool KnobWithEditableLabel(std::string& label, const char *id, float* p_value, float v_min, float v_max, float v_default, float v_center, ModInfo modInfo, double normalSpeed, double slowSpeed, const char* format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10, IValueConverter* conv = nullptr, void* capture = nullptr);
	bool Knob(const char* labelAndID, float* p_value, float v_min, float v_max, float v_default, float v_center, ModInfo modInfo, double normalSpeed, double slowSpeed, const char* format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10, IValueConverter* conv = nullptr, void* capture = nullptr);
	bool KnobInt(const char * labelAndID, int *p_value, int v_min, int v_max, int v_default, int v_center, double normalSpeed, double slowSpeed, const char *format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10, IValueConverter* conv = nullptr, void* capture = nullptr);
}// namespace ImGuiKnobs
