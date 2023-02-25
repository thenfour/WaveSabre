#pragma once

// adaptation from https://github.com/altschuler/imgui-knobs

// TODO: disabled state
// TODO: color schemes
// TODO: modulation indicators

#include <cstdlib>
#include <string>
#include "imgui.h"

typedef int ImGuiKnobFlags;

enum ImGuiKnobFlags_ {
    ImGuiKnobFlags_NoTitle = 1 << 0,
    ImGuiKnobFlags_NoInput = 1 << 1,
    ImGuiKnobFlags_ValueTooltip = 1 << 2,
    ImGuiKnobFlags_DragHorizontal = 1 << 3,
    ImGuiKnobFlags_CustomInput = 1 << 4,
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
};

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

    bool Knob(const char *label, float *p_value, float v_min, float v_max, float v_default, float normalSpeed, float slowSpeed, const char *format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10, IValueConverter* conv = nullptr, void* capture = nullptr);
    bool KnobInt(const char *label, int *p_value, int v_min, int v_max, int v_default, float normalSpeed, float slowSpeed, const char *format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10, IValueConverter* conv = nullptr, void* capture = nullptr);
}// namespace ImGuiKnobs
