#ifndef __WAVESABREVSTLIB_MAJ7EDITORCOMPONENTS_H__
#define __WAVESABREVSTLIB_MAJ7EDITORCOMPONENTS_H__

#include "Common.h"
#include "../imgui/imgui.h"
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include "Maj7ParamConverters.hpp"
#include <functional>
#include <vector>

using namespace WaveSabreCore;

namespace WaveSabreVstLib {

struct ButtonColorSpec {
private:
  ImU32 mActive = 0;
  ImU32 mNormal = 0;
  ImU32 mHovered = 0;

public:
  ButtonColorSpec() {
    mActive = ImGui::GetColorU32(ImGuiCol_FrameBgActive);
    mNormal = ImGui::GetColorU32(ImGuiCol_FrameBg);
    mHovered = ImGui::GetColorU32(ImGuiCol_FrameBgHovered);
  }
  ButtonColorSpec(const char* selColor, const char* notSelColor,
      const char* hoverColor) {
      mActive = ColorFromHTML(selColor);
      mNormal = ColorFromHTML(notSelColor);
      mHovered = ColorFromHTML(hoverColor);
  }
  ButtonColorSpec(ImColor selColor, ImColor normal, ImColor hovered) {
      mActive = selColor;
      mNormal = normal;
      mHovered = hovered;
  }
  ImU32 ActiveU32() const { return mActive; }
  ImU32 NormalU32() const { return mNormal; }
  ImU32 HoveredU32() const { return mHovered; }
};

inline bool ToggleButton(bool *value, const char *label, ImVec2 size = {},
                         const ButtonColorSpec &cfg = {}) {
  bool ret = false;
  ImGui::PushStyleColor(ImGuiCol_ButtonHovered, cfg.HoveredU32());
  if (*value) {
    ImGui::PushStyleColor(ImGuiCol_Button, cfg.ActiveU32());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, cfg.ActiveU32());
  } else {
    ImGui::PushStyleColor(ImGuiCol_Button, cfg.NormalU32());
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, cfg.NormalU32());
  }

  if (ImGui::Button(label, size)) {
    *value = !(*value);
    ret = true;
  }

  ImGui::PopStyleColor(3);
  return ret;
}

struct ParamExplorer {
  float mPowKnobMin = 0;
  float mPowKnobMax = 100;
  float mPowKnobK = 1;
  float mPowKnobValue = 0;

  float mDivKnobMin = 0;
  float mDivKnobMax = 100;
  float mDivKnobK = 1;
  float mDivKnobValue = 0;

  float mEnvTimeValue = 0;

  float mVolumeMaxDB = 6;
  float mVolumeParamValue = 0;

  struct TransferSeries {
    std::function<float(float x)> mTransferFn;
    ImColor mForegroundColor;
    float mThickness;
  };

  void RenderTransferCurve(ImVec2 size,
                           const std::vector<TransferSeries> &serieses) {
    auto *dl = ImGui::GetWindowDrawList();
    ImRect bb;
    bb.Min = ImGui::GetCursorScreenPos();
    bb.Max = bb.Min + size;
    ImGui::RenderFrame(bb.Min, bb.Max, ColorFromHTML("222222"));

    auto LinToY = [&](float lin) {
      float t = M7::math::clamp01(lin);
      return M7::math::lerp(bb.Max.y, bb.Min.y, t);
    };

    auto LinToX = [&](float lin) {
      float t = M7::math::clamp01(lin);
      return M7::math::lerp(bb.Min.x, bb.Max.x, t);
    };

    static constexpr int segmentCount = 40;
    for (auto &series : serieses) {
      std::vector<ImVec2> points;
      for (int i = 0; i < segmentCount; ++i) {
        float tLin = float(i) / (segmentCount - 1); // touch 0 and 1
        float yLin = series.mTransferFn(tLin);
        points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
      }
      dl->AddPolyline(points.data(), (int)points.size(),
                      series.mForegroundColor, 0, series.mThickness);
    }

    ImGui::Dummy(size);
  } // void RenderTransferCurve()

  void Render() {
    ImGui::SetNextItemWidth(160);
    ImGui::InputFloat("min", &mPowKnobMin, 0.01f, 1.0f);
    ImGui::SetNextItemWidth(160);
    ImGui::SameLine();
    ImGui::InputFloat("max", &mPowKnobMax, 0.01f, 1.0f, "%.3f");
    ImGui::SetNextItemWidth(160);
    ImGui::SameLine();
    ImGui::SliderFloat("k", &mPowKnobK, 0.001f, 20);
    M7::PowCurvedParamCfg powCurvedCfg{mPowKnobMin, mPowKnobMax, mPowKnobK};
    PowCurvedConverter powConv{powCurvedCfg};
    ImGuiKnobs::Knob("Pow curved", &mPowKnobValue, 0, 1, 0, 0, {},
                     gNormalKnobSpeed, gSlowKnobSpeed, nullptr,
                     ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput,
                     10, &powConv, this);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("@ .25 : %f", powCurvedCfg.Param01ToValue(0.25f));
    ImGui::Text("@ .50 : %f", powCurvedCfg.Param01ToValue(0.5f));
    ImGui::Text("@ .75 : %f", powCurvedCfg.Param01ToValue(0.75f));
    ImGui::EndGroup();
    ImColor powColor = ColorFromHTML("ffff00", 0.7f);

    auto transferPow01 = [&](float x01) {
      float bigval = powCurvedCfg.Param01ToValue(x01);
      return M7::math::lerp_rev(mPowKnobMin, mPowKnobMax, bigval);
    };

    ImGui::SetNextItemWidth(160);
    ImGui::InputFloat("min##div", &mDivKnobMin, 0.01f, 1.0f);
    ImGui::SetNextItemWidth(160);
    ImGui::SameLine();
    ImGui::InputFloat("max##div", &mDivKnobMax, 0.01f, 1.0f, "%.3f");
    ImGui::SetNextItemWidth(160);
    ImGui::SameLine();
    ImGui::SliderFloat("k##div", &mDivKnobK, 1.001f, 2);
    M7::DivCurvedParamCfg divCurvedCfg{mDivKnobMin, mDivKnobMax, mDivKnobK};
    DivCurvedConverter divConv{divCurvedCfg};
    ImGuiKnobs::Knob("Div curved##div", &mDivKnobValue, 0, 1, 0, 0, {},
                     gNormalKnobSpeed, gSlowKnobSpeed, nullptr,
                     ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput,
                     10, &divConv, this);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::Text("@ .25 : %f", divCurvedCfg.Param01ToValue(0.25f));
    ImGui::Text("@ .50 : %f", divCurvedCfg.Param01ToValue(0.5f));
    ImGui::Text("@ .75 : %f", divCurvedCfg.Param01ToValue(0.75f));
    ImGui::EndGroup();
    ImColor divColor = ColorFromHTML("ff8800", 0.7f);

    auto transferDiv01 = [&](float x01) {
      float bigval = divCurvedCfg.Param01ToValue(x01);
      return M7::math::lerp_rev(mDivKnobMin, mDivKnobMax, bigval);
    };

    // env time
    // EnvTimeConverter envConv{ };
    // ImGuiKnobs::Knob("Env time", &mEnvTimeValue, 0, 1, 0, 0, {}",
    // gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0,
    // ImGuiKnobFlags_CustomInput, 10, &envConv, this); ImGui::SameLine();
    // ImGui::BeginGroup(); float envT = 0.25f; M7::ParamAccessor envPA{ &envT ,
    // 0 }; ImGui::Text("@ .25 : %f", envPA.GetEnvTimeMilliseconds(0, 0)); envT
    // = 0.5f; ImGui::Text("@ .50 : %f", envPA.GetEnvTimeMilliseconds(0, 0));
    // envT = 0.75f;
    // ImGui::Text("@ .75 : %f", envPA.GetEnvTimeMilliseconds(0, 0));
    // ImGui::EndGroup();
    // ImColor envTimeColor = ColorFromHTML("00ff00", 0.7f);

    // auto transferEnvTime = [&](float x01) {
    //	envT = x01;
    //	float bigval = envPA.GetEnvTimeMilliseconds(0, 0);
    //	return M7::math::lerp_rev(M7::EnvTimeParam::gMinRealVal,
    // M7::EnvTimeParam::gMaxRealVal, bigval);
    // };

    // Maj7ImGuiParamVolume
    {
      ImGui::SetNextItemWidth(160);
      // ImGui::InputFloat("max dB##div", &mVolumeMaxDB, 0.01f, 1.0f, "%.3f");
      ImGui::SliderFloat("max dB##vol", &mVolumeMaxDB, 0, 36);

      M7::VolumeParamConfig cfg{
          M7::math::DecibelsToLinear(mVolumeMaxDB),
          mVolumeMaxDB,
      };

      M7VolumeConverter conv{cfg};
      ImGuiKnobs::Knob("M7Volume", &mVolumeParamValue, 0, 1, 0, 0, {},
                       gNormalKnobSpeed, gSlowKnobSpeed, nullptr,
                       ImGuiKnobVariant_WiperOnly, 0,
                       ImGuiKnobFlags_CustomInput, 10, &conv, this);
    }
    ImColor volumeColor = ColorFromHTML("9900ff", 0.7f);

    auto transferVolume = [&](float x01) {
      M7::VolumeParamConfig cfg{
          M7::math::DecibelsToLinear(mVolumeMaxDB),
          mVolumeMaxDB,
      };
      float t = x01;
      M7::ParamAccessor p{&t, 0};
      float bigval = p.GetLinearVolume(0, cfg, 0);
      return M7::math::lerp_rev(M7::gMinGainLinear, mVolumeMaxDB, bigval);
    };

    RenderTransferCurve(
        {300, 300},
        {
            {[&](float x01) { return x01; }, ColorFromHTML("444444", 0.5f),
             1.0f},
            {[&](float x01) { return transferPow01(x01); }, powColor, 2.0f},
            {[&](float x01) { return transferDiv01(x01); }, divColor, 2.0f},
            //{ [&](float x01) { return transferEnvTime(x01); },
            // envTimeColor, 2.0f},
            {[&](float x01) { return transferVolume(x01); }, volumeColor, 2.0f},
        });
  }
};

//// Helper function to add curved lines to ImDrawList paths
//inline void AddCurveToPath(ImDrawList* dl, const ImVec2& start, const ImVec2& delta, 
//                          bool invertX, bool invertY, float curveParam, 
//                          ImU32 color, float thickness) {
//  static constexpr int numSegments = 16;
//  std::vector<ImVec2> points;
//  points.reserve(numSegments + 1);
//  
//  // Add the starting point
//  points.push_back(start);
//  
//  for (int i = 1; i <= numSegments; ++i) {
//    float t = float(i) / numSegments;
//    
//    // Apply curve transformation using the M7 curve system
//    float curvedT = WaveSabreCore::M7::math::modCurve_xN11_kN11(t * 2 - 1, curveParam * 2 - 1) * 0.5f + 0.5f;
//    
//    // Apply inversion flags
//    float finalT = curvedT;
//    if (invertX) {
//      finalT = 1.0f - finalT;
//    }
//    
//    float x = start.x + delta.x * finalT;
//    float y = start.y + delta.y * (invertY ? (1.0f - finalT) : finalT);
//    
//    points.push_back(ImVec2(x, y));
//  }
//  
//  // Draw the polyline
//  dl->AddPolyline(points.data(), (int)points.size(), color, 0, thickness);
//}

// making this an enum keeps the core from getting bloated. if we for example
// have some kind of behavior class passed in from the core, then those details
// will pollute core.
enum class ParamBehavior {
  Default01,
  // Unisono,
  // VibratoFreq,
  // Frequency,
  // FilterQ,
  // Db,
};

template <typename TEnum> struct EnumToggleButtonArrayItem {
  const char *caption;
  const char *selectedColor;
  const char *notSelectedColor;
  const char *selectedHoveredColor;
  const char *notSelectedHoveredColor;
  TEnum value;
  TEnum valueOnDeselect; // when the user unclicks the value, what should the
                         // underlying param get set to?
};

template <typename TEnum> struct EnumMutexButtonArrayItem {
  const char *caption;
  const char *selectedColor;
  const char *notSelectedColor;
  const char *selectedHoveredColor;
  const char *notSelectedHoveredColor;
  TEnum value;
};

template <typename TparamID> struct BoolToggleButtonArrayItem {
  TparamID paramID;
  const char *caption;
  const char *selectedColor;
  const char *notSelectedColor;
  const char *selectedHoveredColor;
  const char *notSelectedHoveredColor;
};

} // namespace WaveSabreVstLib

#endif
