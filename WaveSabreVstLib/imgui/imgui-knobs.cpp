
#include "imgui-knobs.h"

#include <cmath>
#include <cstdlib>
#include <string>
#include <Windows.h>


#include "imgui.h"
#include "imgui_internal.h"

#define IMGUIKNOBS_PI 3.14159265358979323846f


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

        void draw_arc(ImVec2 center, float radius, float start_angle, float end_angle, float thickness, ImColor color, int num_segments, int bezier_count) {
            // Overlap and angle of ends of bezier curves needs work, only looks good when not transperant
            auto overlap = thickness * radius * 0.00001f * IMGUIKNOBS_PI;
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

            knob(const char* _label, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default, DataType v_center, float speed, float _radius, const char* format, ImGuiKnobFlags flags)
            {
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
                ImGui::InvisibleButton(_label, { radius * 2.0f, radius * 2.0f });
                auto gid = ImGui::GetID(_label);
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
                angle_center = WaveSabreCore::M7::math::map((float)v_center, (float)v_min, (float)v_max, angle_min, angle_max);
                graphic_rect = { screen_pos, ImVec2{screen_pos.x + radius*2, screen_pos.y + radius*2} };
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

            void draw_m7_curve(color_set bgcolor, color_set linecolor, bool invertX, bool invertY) {
                ImGui::RenderFrame(graphic_rect.Min, graphic_rect.Max, bgcolor.base, false, 3);
                float val;
                WaveSabreCore::M7::CurveParam param {val };
                param.SetParamValue(this->t);
                AddCurveToPath(ImGui::GetWindowDrawList(), graphic_rect.Min, graphic_rect.GetSize(), invertX, invertY, param, linecolor.base, 2.0f);
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

        template<typename DataType>
        knob<DataType> knob_with_drag(const char* label, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default, DataType v_center, float _speed, const char* format, float size, ImGuiKnobFlags flags, IValueConverter* conv, void* capture) {
            auto speed = _speed == 0 ? (v_max - v_min) / 250.f : _speed;
            ImGui::PushID(label);
            auto width = size == 0 ? ImGui::GetTextLineHeight() * 3.25f : size * ImGui::GetIO().FontGlobalScale;
            ImGui::PushItemWidth(width);

            ImGui::BeginGroup();

            // There's an issue with `SameLine` and Groups, see https://github.com/ocornut/imgui/issues/4190.
            // This is probably not the best solution, but seems to work for now
            ImGui::GetCurrentWindow()->DC.CurrLineTextBaseOffset = 0;

            // Draw title
            if (!(flags & ImGuiKnobFlags_NoTitle)) {
                auto title_size = ImGui::CalcTextSize(label, NULL, false, width);

                // Center title
                ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (width - title_size[0]) * 0.5f);

                auto end = ImGui::FindRenderedTextEnd(label, 0);
                auto txt = std::string(label, end);

                ImGui::Text("%s", txt.c_str());
            }

            // Draw knob
            //if (!(flags & ImGuiKnobFlags_NoKnob)) {
                knob<DataType> k(label, data_type, p_value, v_min, v_max, v_default, v_center, speed, width * 0.5f, format, flags);
            //}

            // Draw tooltip
            if (flags & ImGuiKnobFlags_ValueTooltip && (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) || ImGui::IsItemActive())) {
                ImGui::BeginTooltip();
                ImGui::Text(format, *p_value);
                ImGui::EndTooltip();
            }

            // Draw input
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0,0});
            if (flags & ImGuiKnobFlags_CustomInput) {

                // TODO: a way of inputting text
                char textBuffer[100];
                std::string displayVal = conv->ParamToDisplayString((double)*p_value, capture);
                strcpy_s(textBuffer, displayVal.c_str());
                ImGui::Text("%s", displayVal.c_str());
                //if (ImGui::InputText("###knob_custom_input", textBuffer, std::size(textBuffer))) {
                //    // transform value back.
                //    *p_value = (DataType)std::strtod(textBuffer, nullptr);
                //    k.value_changed = true;
                //}

                //auto changed = LabelEditOnClick("###knob_label", p_value, conv, capture);
                //if (changed) {
                //    k.value_changed = true;
                //}
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
    }// namespace detail


    template<typename DataType>
    bool BaseKnob(const char* label, ImGuiDataType data_type, DataType* p_value, DataType v_min, DataType v_max, DataType v_default, DataType v_center, float speed, const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture) {
        auto knob = detail::knob_with_drag(label, data_type, p_value, v_min, v_max, v_default, v_center, speed, format, size, flags, conv, capture);

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

            //if (knob.t > 0.01f) {
                knob.draw_arc(0.8f, 0.43f, knob.angle_min, knob.angle, detail::GetPrimaryColorSet(), 16, 2);
            //}
            break;
        }
        case ImGuiKnobVariant_WiperOnly: {
            knob.draw_arc(0.8f, 1.00f, knob.angle_min, knob.angle_max, detail::GetTrackColorSet(), 32, 2);

            //if (knob.t > 0.01) {
                knob.draw_arc(0.8f, 0.9f, knob.angle_center, knob.angle, detail::GetPrimaryColorSet(), 16, 2);
            //}

            knob.draw_dot(0.25f, 0.85f, knob.angle, detail::GetDotColorSet(), true, 12);

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
            knob.draw_m7_curve(detail::GetTrackColorSet(), detail::GetDotColorSet(), !!(flags & ImGuiKnobFlags_InvertXCurve), !!(flags & ImGuiKnobFlags_InvertYCurve));
            break;
        }
        }

        return knob.value_changed;
    }

    bool Knob(const char* label, float* p_value, float v_min, float v_max, float v_default, float v_center, float normalSpeed, float slowSpeed,
        const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture)
    {
        const char* _format = format == NULL ? "%.3f" : format;
        return BaseKnob(label, ImGuiDataType_Float, p_value, v_min, v_max, v_default, v_center, ImGui::GetIO().KeyShift ? slowSpeed : normalSpeed, _format, variant, size, flags, steps, conv, capture);
    }

    bool KnobInt(const char* label, int* p_value, int v_min, int v_max, int v_default, int v_center, float normalSpeed, float slowSpeed,
        const char* format, ImGuiKnobVariant variant, float size, ImGuiKnobFlags flags, int steps, IValueConverter* conv, void* capture)
    {
        const char* _format = format == NULL ? "%i" : format;
        return BaseKnob(label, ImGuiDataType_S32, p_value, v_min, v_max, v_default, v_center, ImGui::GetIO().KeyShift ? slowSpeed : normalSpeed, _format, variant, size, flags, steps, conv, capture);
    }

}// namespace ImGuiKnobs
