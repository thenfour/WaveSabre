
#include "imgui-knobs.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <Windows.h>


#include "imgui.h"
#include "imgui_internal.h"

#define IMGUIKNOBS_PI 3.14159265358979323846f

namespace ImGui {

    // WS: this is effectively copied from Imgui, but
    // 1. hard-codes the datatype as a `double`
    // 2. uses custom formatting for displaying the label (but default behavior for editing)
    // 3. some metrics are tweaked to make the display size correct

    // Note: p_data, p_min and p_max are _pointers_ to a memory address holding the data. For a Drag widget, p_min and p_max are optional.
    // Read code of e.g. DragFloat(), DragInt() etc. or examples in 'Demo->Widgets->Data Types' to understand how to use this function directly.
    static inline bool DragScalar_Custom(const char* label, double* p_data, float v_speed, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, ImGuiKnobs::IValueConverter* conv, void* capture)
    {
        static constexpr float          DRAG_MOUSE_THRESHOLD_FACTOR = 0.50f;    // Multiplier for the default value of io.MouseDragThreshold to make DragFloat/DragInt react faster to mouse drags.
        ImGuiDataType data_type = ImGuiDataType_Double;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        char value_buf[64];
        double paramValue = *p_data;
        auto strValue = conv->ParamToDisplayString(paramValue, capture);
        strcpy_s(value_buf, strValue.c_str());

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = CalcItemWidth();

        ImVec2 label_size = CalcTextSize(label, NULL, true);
        ImVec2 value_size = CalcTextSize(value_buf, NULL, true);
        value_size.x = std::max(value_size.x + 6, 40.0f);

        float frame_bb_w = value_size.x;

        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(frame_bb_w, label_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
            return false;

        // Default format string when passing NULL
        //if (format == NULL)
        //    format = DataTypeGetInfo(data_type)->PrintFmt;

        const bool hovered = ItemHoverable(frame_bb, id);
        bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
        if (!temp_input_is_active)
        {
            // Tabbing or CTRL-clicking on Drag turns it into an InputText
            const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
            const bool clicked = hovered && IsMouseClicked(0, id);
            const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
            const bool make_active = (input_requested_by_tabbing || clicked || double_clicked || g.NavActivateId == id || g.NavActivateInputId == id);
            if (make_active && (clicked || double_clicked))
                SetKeyOwner(ImGuiKey_MouseLeft, id);
            if (make_active && temp_input_allowed)
                if (input_requested_by_tabbing || (clicked && g.IO.KeyCtrl) || double_clicked || g.NavActivateInputId == id)
                    temp_input_is_active = true;

            // (Optional) simple click (without moving) turns Drag into an InputText
            if (g.IO.ConfigDragClickToInputText && temp_input_allowed && !temp_input_is_active)
                if (g.ActiveId == id && hovered && g.IO.MouseReleased[0] && !IsMouseDragPastThreshold(0, g.IO.MouseDragThreshold * DRAG_MOUSE_THRESHOLD_FACTOR))
                {
                    g.NavActivateId = g.NavActivateInputId = id;
                    g.NavActivateFlags = ImGuiActivateFlags_PreferInput;
                    temp_input_is_active = true;
                }

            if (make_active && !temp_input_is_active)
            {
                SetActiveID(id, window);
                SetFocusID(id, window);
                FocusWindow(window);
                g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            }
        }

        if (temp_input_is_active)
        {
            // Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
            const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0 && (p_min == NULL || p_max == NULL || DataTypeCompare(data_type, p_min, p_max) < 0);
            return TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : NULL, is_clamp_input ? p_max : NULL);
        }

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

        // Drag behavior
        const bool value_changed = DragBehavior(id, data_type, p_data, v_speed, p_min, p_max, format, flags);
        if (value_changed)
            MarkItemEdited(id);

        // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
        //const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);
        if (g.LogEnabled)
            LogSetNextTextDecoration("{", "}");
        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf + strlen(value_buf), NULL, ImVec2(0.5f, 0.5f));
        //RenderText(frame_bb.Min, value_buf, 0, false);

        if (label_size.x > 0.0f)
            RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | (temp_input_allowed ? ImGuiItemStatusFlags_Inputable : 0));
        return value_changed;
    }

    // derived from DragScalar_Custom to work on text labels
    static inline bool EditableTextUnformatted(std::string& text, const char* id_)
    {
        const char* label = id_;// text.c_str();
        int flags = ImGuiSliderFlags_Vertical;

        ImGuiWindow* window = GetCurrentWindow();
        if (window->SkipItems)
            return false;

        static constexpr size_t maxTextLen = 20;
        char value_buf[maxTextLen];
        strcpy_s(value_buf, text.c_str());

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const float w = CalcItemWidth();

        ImVec2 value_size = CalcTextSize(value_buf, NULL, true);
        value_size.x = std::max(value_size.x + 6, 40.0f);

        float frame_bb_w = value_size.x;

        const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(frame_bb_w, value_size.y + style.FramePadding.y * 2.0f));
        const ImRect total_bb{ frame_bb };

        const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
        ItemSize(total_bb, style.FramePadding.y);
        if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
            return false;

        const bool hovered = ItemHoverable(frame_bb, id);
        bool temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
        if (!temp_input_is_active)
        {
            // Tabbing or CTRL-clicking on Drag turns it into an InputText
            const bool input_requested_by_tabbing = temp_input_allowed && (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
            const bool double_clicked = (hovered && g.IO.MouseClickedCount[0] == 2 && TestKeyOwner(ImGuiKey_MouseLeft, id));
            const bool make_active = (input_requested_by_tabbing || double_clicked || g.NavActivateId == id || g.NavActivateInputId == id);
            if (make_active && (double_clicked))
            {
                SetKeyOwner(ImGuiKey_MouseLeft, id);
            }
            if (make_active && temp_input_allowed)
            {
                if (input_requested_by_tabbing || double_clicked || g.NavActivateInputId == id) {
                    temp_input_is_active = true;
                }
            }

            if (make_active && !temp_input_is_active)
            {
                SetActiveID(id, window);
                SetFocusID(id, window);
                FocusWindow(window);
                g.ActiveIdUsingNavDirMask = (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
            }
        }

        if (temp_input_is_active)
        {
            if (!TempInputText(frame_bb, id, label, value_buf, (int)std::size(value_buf), ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoMarkEdited))
            {
                return false;
            }
            text = value_buf;
            return true;
        }

        // Draw frame
        const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg);
        RenderNavHighlight(frame_bb, id);
        RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, style.FrameRounding);

        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf + strlen(value_buf), NULL, ImVec2(0.5f, 0.5f));

        return false;
    }

}


namespace ImGuiKnobs {
    namespace detail {





        void draw_arc1(ImVec2 center, float radius, float start_angle__, float end_angle__, float thickness, ImColor color, int num_segments) {
            float start_angle = std::min(start_angle__, end_angle__);
            float end_angle = std::max(start_angle__, end_angle__);
            ImVec2 start = {
                    center[0] + cosf(start_angle) * radius,
                    center[1] + sinf(start_angle) * radius,
            };

            ImVec2 end = {
                    center[0] + cosf(end_angle) * radius,
                    center[1] + sinf(end_angle) * radius,
            };

            // Calculate bezier arc points
            auto ax = start[0] - center[0];
            auto ay = start[1] - center[1];
            auto bx = end[0] - center[0];
            auto by = end[1] - center[1];
            auto q1 = ax * ax + ay * ay;
            auto q2 = q1 + ax * bx + ay * by;
            auto k2 = (4.0f / 3.0f) * (sqrtf((2.0f * q1 * q2)) - q2) / (ax * by - ay * bx);
            auto arc1 = ImVec2{ center[0] + ax - k2 * ay, center[1] + ay + k2 * ax };
            auto arc2 = ImVec2{ center[0] + bx + k2 * by, center[1] + by - k2 * bx };

            auto* draw_list = ImGui::GetWindowDrawList();

            draw_list->AddBezierCurve(start, arc1, arc2, end, color, thickness, num_segments);
        }

        void draw_arc(ImVec2 center, float radius, float start_angle__, float end_angle__, float thickness, ImColor color, int num_segments, int bezier_count) {
            // Overlap and angle of ends of bezier curves needs work, only looks good when not transperant
            auto overlap = thickness * radius * 0.00001f * IMGUIKNOBS_PI;
            float start_angle = std::min(start_angle__, end_angle__);
            float end_angle = std::max(start_angle__, end_angle__);
            auto delta = end_angle - start_angle;
            auto bez_step = 1.0f / bezier_count;
            auto mid_angle = start_angle + overlap;

            for (auto i = 0; i < bezier_count - 1; i++) {
                auto mid_angle2 = delta * bez_step + mid_angle;
                draw_arc1(center, radius, mid_angle - overlap, mid_angle2 + overlap, thickness, color, num_segments);
                mid_angle = mid_angle2;
            }

            draw_arc1(center, radius, mid_angle - overlap, end_angle, thickness, color, num_segments);
        }

        template<typename DataType>
        struct knob {
            float radius = false;
            bool value_changed = false;
            ImVec2 center;
            ImRect graphic_rect;
            bool is_active = false;
            bool is_hovered = false;
            float angle_min = 0;
            float angle_max = 0;
            float angle_center = 0;
            float t = 0;
            float angle = 0;
            float angle_cos = 0;
            float angle_sin = 0;

            float angle_modMin = 0;
            float angle_modMax = 0;
            float angle_modVal = 0;

            knob(std::string& _label, const char *id, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default, DataType v_center, ModInfo modInfo, float speed, float _radius, const char* format,
                ImGuiKnobVariant variant, ImGuiKnobFlags flags)
            {
                std::string labelAndID = _label;

                if (id) {
                    labelAndID += "###";
                    labelAndID += id;
                }

                auto screen_pos = ImGui::GetCursorScreenPos();

                if (flags & ImGuiKnobFlags_NoKnob)
                {
                    center = screen_pos;
                    graphic_rect.Min = graphic_rect.Max = screen_pos;
                    return;
                }

                radius = _radius;

                t = ((float)*p_value - v_min) / (v_max - v_min);

                // Handle dragging
                ImVec2 knobSize = { radius * 2, radius * 2 };
                if (variant == ImGuiKnobVariant_ProgressBar) {
                    knobSize = { radius * 2, radius * 0.33f };
                }

                ImGui::InvisibleButton(labelAndID.c_str(), knobSize); // TODO: what is this for??
                auto gid = ImGui::GetID(labelAndID.c_str());
                ImGuiSliderFlags drag_flags = 0;
                if (!(flags & ImGuiKnobFlags_DragHorizontal)) {
                    drag_flags |= ImGuiSliderFlags_Vertical;
                }

                value_changed = ImGui::DragBehavior(gid, data_type, p_value, speed, &v_min, &v_max, format, drag_flags);

                if (ImGui::IsItemClicked() && ImGui::GetIO().KeyCtrl) {
                    *p_value = v_default;
                    value_changed = true;
                }

                angle_min = IMGUIKNOBS_PI * 0.75f;
                angle_max = IMGUIKNOBS_PI * 2.25f;

                auto valToAngle = [&](float val) {
                    val = WaveSabreCore::M7::math::clamp(val, (float)v_min, (float)v_max);
                    return WaveSabreCore::M7::math::map((float)val, (float)v_min, (float)v_max, angle_min, angle_max);
                };

                angle_center = valToAngle((float)v_center);// WaveSabreCore::M7::math::map((float)v_center, (float)v_min, (float)v_max, angle_min, angle_max);

                angle_modMin = valToAngle(*p_value - modInfo.mExtentMin);
                angle_modMax = valToAngle(*p_value + modInfo.mExtentMax);
                angle_modVal = valToAngle(*p_value + modInfo.mValue);

                graphic_rect = { screen_pos, screen_pos + knobSize };
                center = { screen_pos[0] + radius, screen_pos[1] + radius };
                is_active = ImGui::IsItemActive();
                is_hovered = ImGui::IsItemHovered();
                angle = angle_min + (angle_max - angle_min) * t;
                angle_cos = cosf(angle);
                angle_sin = sinf(angle);
            }

            void draw_dot(float size, float radius, float angle, color_set color, bool filled, int segments) {
                auto dot_size = size * this->radius;
                auto dot_radius = radius * this->radius;

                ImGui::GetWindowDrawList()->AddCircleFilled(
                    { center[0] + cosf(angle) * dot_radius, center[1] + sinf(angle) * dot_radius },
                    dot_size,
                    is_active ? color.active : (is_hovered ? color.hovered : color.base),
                    segments);
            }

            void draw_tick(float start, float end, float width, float angle, color_set color) {
                auto tick_start = start * radius;
                auto tick_end = end * radius;
                auto angle_cos = cosf(angle);
                auto angle_sin = sinf(angle);

                ImGui::GetWindowDrawList()->AddLine(
                    { center[0] + angle_cos * tick_end, center[1] + angle_sin * tick_end },
                    { center[0] + angle_cos * tick_start, center[1] + angle_sin * tick_start },
                    is_active ? color.active : (is_hovered ? color.hovered : color.base),
                    width * radius);
            }

            void draw_circle(float size, color_set color, bool filled, int segments) {
                auto circle_radius = size * radius;

                ImGui::GetWindowDrawList()->AddCircleFilled(
                    center,
                    circle_radius,
                    is_active ? color.active : (is_hovered ? color.hovered : color.base));
            }

            void draw_m7_curve(color_set linecolor, float val, bool invertX, bool invertY) {
                //ImGui::RenderFrame(graphic_rect.Min, graphic_rect.Max, bgcolor.base, false, 3);
                float backing;
                WaveSabreCore::M7::CurveParam param { backing };
                param.SetParamValue(val);
                AddCurveToPath(ImGui::GetWindowDrawList(), graphic_rect.Min, graphic_rect.GetSize(), invertX, invertY, param, linecolor.base, 2.0f);
            }

            void draw_progress_bar(color_set bgcolor, color_set linecolor, color_set tickColor, float center01) {
                //ImGui::RenderFrame(graphic_rect.Min, graphic_rect.Max, bgcolor.base, false, 3);
                auto* dl = ImGui::GetWindowDrawList();
                dl->AddRectFilled(graphic_rect.Min, graphic_rect.Max, bgcolor.base);

                // the track is from center to value.
                float centerX = ImLerp(graphic_rect.Min.x, graphic_rect.Max.x, center01);
                float valueX = ImLerp(graphic_rect.Min.x, graphic_rect.Max.x, this->t);
                dl->AddRectFilled({ std::min(centerX, valueX), graphic_rect.Min.y }, { std::max(centerX, valueX), graphic_rect.Max.y }, linecolor.base);

                dl->AddCircleFilled({ valueX, (graphic_rect.Min.y + graphic_rect.Max.y) * 0.5f }, graphic_rect.GetHeight() * 0.6f, tickColor.base);
            }

            void draw_arc(float radius, float size, float start_angle, float end_angle, color_set color, int segments, int bezier_count) {
                if (fabsf(start_angle - end_angle) < 0.001) return;
                auto track_radius = radius * this->radius;
                auto track_size = size * this->radius * 0.5f + 0.0001f;

                detail::draw_arc(
                    center,
                    track_radius,
                    start_angle,
                    end_angle,
                    track_size,
                    is_active ? color.active : (is_hovered ? color.hovered : color.base),
                    segments,
                    bezier_count);
            }
        };

        // label & id need to be separated if you want to allow editing label.
        template<typename DataType>
        knob<DataType> knob_with_drag(std::string& label, const char *id, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default,
            DataType v_center, ModInfo modInfo, float _speed, const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, IValueConverter* conv, void* capture)
        {
            auto speed = _speed == 0 ? (v_max - v_min) / 250.f : _speed;

            ImGui::PushID(id == nullptr ? label.c_str() : id);
            auto width = size == 0 ? ImGui::GetTextLineHeight() * 3.25f : size * ImGui::GetIO().FontGlobalScale;
            ImGui::PushItemWidth(width);

            ImGui::BeginGroup();

            // There's an issue with `SameLine` and Groups, see https://github.com/ocornut/imgui/issues/4190.
            // This is probably not the best solution, but seems to work for now
            ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;

            // Draw title
            if (!(flags & ImGuiKnobFlags_NoTitle)) {
                auto title_size = ImGui::CalcTextSize(label.c_str(), NULL, false, width);

                // Center title
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - title_size[0]) * 0.5f);

                if (flags & ImGuiKnobFlags_EditableTitle) {
                    ImGui::EditableTextUnformatted(label, id);
                }
                else {
                    ImGui::TextUnformatted(label.c_str());
                }
            }

            // Draw knob
            knob<DataType> k(label, id, data_type, p_value, v_min, v_max, v_default, v_center, modInfo, speed, width * 0.5f, format, variant, flags);

            // Draw tooltip
            if (flags & ImGuiKnobFlags_ValueTooltip && (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemActive())) {
                ImGui::BeginTooltip();
                ImGui::Text(format, *p_value);
                ImGui::EndTooltip();
            }

            // Draw input
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0,0});
            if (flags & ImGuiKnobFlags_CustomInput) {
                ImGuiSliderFlags drag_flags = 0;
                if (!(flags & ImGuiKnobFlags_DragHorizontal)) {
                    drag_flags |= ImGuiSliderFlags_Vertical;
                }
                double tempVal = (double)*p_value;
                auto changed = ImGui::DragScalar_Custom("###knob_drag", &tempVal, speed, &v_min, &v_max, format, drag_flags, conv, capture);
                if (changed) {
                    k.value_changed = true;
                    *p_value = (DataType)tempVal;
                }

            }
            else if (!(flags & ImGuiKnobFlags_NoInput)) {
                ImGuiSliderFlags drag_flags = 0;
                if (!(flags & ImGuiKnobFlags_DragHorizontal)) {
                    drag_flags |= ImGuiSliderFlags_Vertical;
                }
                auto changed = ImGui::DragScalar("###knob_drag", data_type, p_value, speed, &v_min, &v_max, format, drag_flags);
                if (changed) {
                    k.value_changed = true;
                }
            }
            ImGui::PopStyleVar(1);

            ImGui::EndGroup();
            ImGui::PopItemWidth();
            ImGui::PopID();

            return k;
        }

        color_set GetDotColorSet() {
            auto* colors = ImGui::GetStyle().Colors;

            return { colors[ImGuiCol_PlotHistogram], colors[ImGuiCol_PlotHistogramHovered], colors[ImGuiCol_PlotHistogramHovered] };
        }

        color_set GetPrimaryColorSet() {
            auto* colors = ImGui::GetStyle().Colors;

            return { colors[ImGuiCol_ButtonActive], colors[ImGuiCol_ButtonHovered], colors[ImGuiCol_ButtonHovered] };
        }

        color_set GetSecondaryColorSet() {
            auto* colors = ImGui::GetStyle().Colors;
            auto active = ImVec4(
                colors[ImGuiCol_ButtonActive].x * 0.5f,
                colors[ImGuiCol_ButtonActive].y * 0.5f,
                colors[ImGuiCol_ButtonActive].z * 0.5f,
                colors[ImGuiCol_ButtonActive].w);

            auto hovered = ImVec4(
                colors[ImGuiCol_ButtonHovered].x * 0.5f,
                colors[ImGuiCol_ButtonHovered].y * 0.5f,
                colors[ImGuiCol_ButtonHovered].z * 0.5f,
                colors[ImGuiCol_ButtonHovered].w);

            return { active, hovered, hovered };
        }

        color_set GetTrackColorSet() {
            auto* colors = ImGui::GetStyle().Colors;

            return { colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBg], colors[ImGuiCol_FrameBg] };
        }

        color_set GetModulationBackgroundColorSet() {
            //auto c = ImColor{.8f, .2f, 0.2f, 1.0f};
            //auto c = ImColor{ .0f, .8f, 0.8f, 1.0f };
            auto* colors = ImGui::GetStyle().Colors;
            auto c = ColorMod::GetModifiedColor(colors[ImGuiCol_ButtonHovered], 0.69f, 1.0f, 0.5f);
            return { c, c, c };
        }

        color_set GetModulationForegroundColorSet() {
            //auto c = ImColor{.8f, .2f, 0.2f, 1.0f};
            //auto c = ImColor{ .0f, .8f, 0.8f, 1.0f };
            auto* colors = ImGui::GetStyle().Colors;
            auto c = ColorMod::GetModifiedColor(colors[ImGuiCol_ButtonHovered], 0.69f, 1.0f, 1.0f);
            return { c, c, c };
        }

        color_set GetModulationDotColorSet() {
            auto c = ImColor{.8f, .2f, 0.2f, 1.0f};
            return { c, c, c };
        } 

    }// namespace detail


    template<typename DataType>
    bool BaseKnob(std::string& label, const char *id, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default, DataType v_center,
        ModInfo modInfo,
        float speed, const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture) {
        auto knob = detail::knob_with_drag(label, id, data_type, p_value, v_min, v_max, v_default, v_center, modInfo, speed, format, variant, size, flags, conv, capture);

        switch (variant) {
        case ImGuiKnobVariant_Tick: {
            knob.draw_circle(0.85f, detail::GetSecondaryColorSet(), true, 32);
            knob.draw_tick(0.3f, 0.85f, 0.13f, knob.angle, detail::GetPrimaryColorSet());
            //knob.draw_arc(0.85f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 16, 2);
            break;
        }
        case ImGuiKnobVariant_Dot: {
            knob.draw_circle(0.85f, detail::GetSecondaryColorSet(), true, 32);
            knob.draw_dot(0.12f, 0.6f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
            break;
        }

        case ImGuiKnobVariant_Wiper: {
            knob.draw_circle(0.7f, detail::GetSecondaryColorSet(), true, 32);
            knob.draw_arc(0.8f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 16, 2);
            knob.draw_arc(0.8f, 0.43f, knob.angle_min, knob.angle, detail::GetPrimaryColorSet(), 16, 2);
            break;
        }
        case ImGuiKnobVariant_WiperOnly: {
            //knob.draw_arc(0.8f, 1.00f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 32, 2); // background from min to max angle
            knob.draw_arc(0.8f, 0.86f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 32, 2); // background from min to max angle
            //knob.draw_arc(0.8f, 0.9f, knob.angle_center, knob.angle, detail::GetPrimaryColorSet(), 16, 2); // swept track area from center to value
            knob.draw_arc(0.8f, 0.86f, knob.angle_center, knob.angle, detail::GetPrimaryColorSet(), 16, 2); // swept track area from center to value

            auto modBackgroundColorSet = detail::GetModulationBackgroundColorSet();
            auto modForegroundColorSet = detail::GetModulationForegroundColorSet();
            if (modInfo.mEnabled)
            {
                // draw modulation indicator in the center.
                ImGui::GetWindowDrawList()->AddCircleFilled(
                    knob.center,
                    0.2f * knob.radius,
                    modBackgroundColorSet.active,
                    9);

                // draw modulation ring on the outer rim of the track
                // void draw_arc(float radius, float size, float start_angle, float end_angle, color_set color, int segments, int bezier_count) {
                knob.draw_arc(0.98f, 0.3f, knob.angle_modMin, knob.angle_modMax, modBackgroundColorSet, 9, 2); // swept track area from center to value

                knob.draw_arc(0.98f, 0.3f, knob.angle_modVal, knob.angle, modForegroundColorSet, 9, 2); // swept track area from center to value
            }

            knob.draw_dot(0.25f, 0.85f, knob.angle, detail::GetDotColorSet(), true, 9); // yellow handle

            break;
        }
        case ImGuiKnobVariant_WiperDot: {
            knob.draw_circle(0.6f, detail::GetSecondaryColorSet(), true, 32);
            knob.draw_arc(0.85f, 0.41f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 16, 2);
            knob.draw_dot(0.1f, 0.85f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
            break;
        }
        case ImGuiKnobVariant_Stepped: {
            for (auto n = 0.f; n < steps; n++) {
                auto a = n / (steps - 1);
                auto angle = knob.angle_min + (knob.angle_max - knob.angle_min) * a;
                knob.draw_tick(0.7f, 0.9f, 0.04f, angle, detail::GetPrimaryColorSet());
            }

            knob.draw_circle(0.6f, detail::GetSecondaryColorSet(), true, 32);
            knob.draw_dot(0.12f, 0.4f, knob.angle, detail::GetPrimaryColorSet(), true, 12);
            break;
        }
        case ImGuiKnobVariant_Space: {
            knob.draw_circle(0.3f - knob.t * 0.1f, detail::GetSecondaryColorSet(), true, 16);

            //if (knob.t > 0.01f) {
                knob.draw_arc(0.4f, 0.15f, knob.angle_min - 1.0f, knob.angle - 1.0f, detail::GetPrimaryColorSet(), 16, 2);
                knob.draw_arc(0.6f, 0.15f, knob.angle_min + 1.0f, knob.angle + 1.0f, detail::GetPrimaryColorSet(), 16, 2);
                knob.draw_arc(0.8f, 0.15f, knob.angle_min + 3.0f, knob.angle + 3.0f, detail::GetPrimaryColorSet(), 16, 2);
            //}
            break;
        }
        case ImGuiKnobVariant_M7Curve: {
            // assumes 0-1 range
            ImGui::RenderFrame(knob.graphic_rect.Min, knob.graphic_rect.Max, detail::GetTrackColorSet().base, false, 3);
            if (modInfo.mEnabled)
            {
                auto modBackgroundColorSet = detail::GetModulationBackgroundColorSet();
                auto modForegroundColorSet = detail::GetModulationForegroundColorSet();
                knob.draw_m7_curve(modBackgroundColorSet, knob.t - modInfo.mExtentMin, !!(flags & ImGuiKnobFlags_InvertXCurve), !!(flags & ImGuiKnobFlags_InvertYCurve));
                knob.draw_m7_curve(modBackgroundColorSet, knob.t + modInfo.mExtentMax, !!(flags & ImGuiKnobFlags_InvertXCurve), !!(flags & ImGuiKnobFlags_InvertYCurve));
                knob.draw_m7_curve(modForegroundColorSet, knob.t + modInfo.mValue, !!(flags & ImGuiKnobFlags_InvertXCurve), !!(flags & ImGuiKnobFlags_InvertYCurve));
            }
            knob.draw_m7_curve(detail::GetDotColorSet(), knob.t, !!(flags & ImGuiKnobFlags_InvertXCurve), !!(flags & ImGuiKnobFlags_InvertYCurve));

            break;
        }
        case ImGuiKnobVariant_ProgressBar: {
            // assumes 0-1 range

            knob.draw_progress_bar(detail::GetTrackColorSet(), detail::GetPrimaryColorSet(), detail::GetDotColorSet(), (float) v_center);
            break;
        }
        }

        return knob.value_changed;
    }

    bool Knob(const char* labelAndID, float* p_value, float v_min, float v_max, float v_default, float v_center, ModInfo modInfo,
        float normalSpeed, float slowSpeed,
        const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture)
    {
        const char* _format = format == NULL ? "%.3f" : format;
        std::string label2{ labelAndID };
        return BaseKnob(label2, nullptr, ImGuiDataType_Float, p_value, v_min, v_max, v_default, v_center,
            modInfo,
            ImGui::GetIO().KeyShift ? slowSpeed : normalSpeed, _format, variant, size, flags, steps, conv, capture);
    }

    bool KnobWithEditableLabel(std::string& label, const char *id, float* p_value, float v_min, float v_max, float v_default, float v_center,
        ModInfo modInfo, float normalSpeed, float slowSpeed, const char* format, ImGuiKnobVariant variant,
        float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture)
    {
        const char* _format = format == NULL ? "%.3f" : format;
        return BaseKnob(label, id, ImGuiDataType_Float, p_value, v_min, v_max, v_default, v_center,
            modInfo,
            ImGui::GetIO().KeyShift ? slowSpeed : normalSpeed, _format, variant, size, flags, steps, conv, capture);
    }


    bool KnobInt(const char* labelAndID, int* p_value, int v_min, int v_max, int v_default, int v_center, float normalSpeed, float slowSpeed,
        const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture)
    {
        const char* _format = format == NULL ? "%i" : format;
        std::string label2{ labelAndID };
        return BaseKnob(label2, nullptr, ImGuiDataType_S32, p_value, v_min, v_max, v_default, v_center,
            ModInfo{},
            ImGui::GetIO().KeyShift ? slowSpeed : normalSpeed, _format, variant, size, flags, steps, conv, capture);
    }

}// namespace ImGuiKnobs
