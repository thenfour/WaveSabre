#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include <WaveSabreCore/../../Filters/Maj7Filter.hpp>
#include <imgui.h>
#include "ImGuiUtils.hpp"

namespace WaveSabreVstLib
{
using namespace WaveSabreCore;

inline constexpr std::array<M7::FilterResponse, 8> gFilterResponseOrder = {
  M7::FilterResponse::Lowpass,
  M7::FilterResponse::Highpass,
  M7::FilterResponse::Bandpass,
  M7::FilterResponse::Notch,
  M7::FilterResponse::Allpass,
  M7::FilterResponse::Peak,
  M7::FilterResponse::LowShelf,
  M7::FilterResponse::HighShelf,
};

inline constexpr std::array<const char*, 8> gFilterResponseLabels = {
  "\\ LP", "/ HP", "| BP", "O NT", "~ AP", "^ PK", "_ LS", "- HS",
};

inline const char* LabelForFilterResponse(M7::FilterResponse r)
{
  for (size_t i = 0; i < gFilterResponseOrder.size(); ++i)
  {
    if (gFilterResponseOrder[i] == r)
      return gFilterResponseLabels[i];
  }
  return "-";
}

inline const char* LabelForFilterResponseShort(M7::FilterResponse r)
{
  switch (r)
  {
    case M7::FilterResponse::Lowpass:
      return "LP";
    case M7::FilterResponse::LowShelf:
      return "LS";
    case M7::FilterResponse::Bandpass:
      return "BP";
    case M7::FilterResponse::Notch:
      return "NT";
    case M7::FilterResponse::Allpass:
      return "AP";
    case M7::FilterResponse::Peak:
      return "PK";
    case M7::FilterResponse::HighShelf:
      return "HS";
    case M7::FilterResponse::Highpass:
      return "HP";
    default:
      return "?";
  }
}

inline std::string BuildFilterSelectionButtonLabel(
                                                   M7::FilterResponse response)
{
  std::string label = LabelForFilterResponseShort(response);
  return label;
}

template <typename IsSupportedFn, typename IsResponseAllowedFn>
inline std::vector<M7::FilterResponse> BuildFilterSelectionChoices(IsSupportedFn isSupported,
                                                                   IsResponseAllowedFn isResponseAllowed)
{
  std::vector<M7::FilterResponse> validChoices;
  constexpr size_t kMaxChoices = size_t(M7::FilterResponse::Count);
  validChoices.reserve(kMaxChoices);

  for (auto r : gFilterResponseOrder)
  {
    if (!isResponseAllowed(r))
      continue;

    if (isSupported(r))
    {
      validChoices.push_back(r);
    }
  }

  if (validChoices.empty())
  {
    validChoices.push_back(M7::FilterResponse::Lowpass);
  }

  return validChoices;
}

template <typename IsSupportedFn>
inline std::vector<M7::FilterResponse> BuildFilterSelectionChoices(IsSupportedFn isSupported)
{
  return BuildFilterSelectionChoices(isSupported, [](M7::FilterResponse)
                                     { return true; });
}

template <typename IsSupportedFn, typename IsResponseAllowedFn, typename ApplySelectionFn, typename DrawMainFn>
inline void RenderFilterSelectionWidget(const char* id,
                                        int idSeed,
                                        M7::FilterResponse selectedResponse,
                                        IsSupportedFn isSupported,
                                        IsResponseAllowedFn isResponseAllowed,
                                        ApplySelectionFn applySelection,
                                        DrawMainFn drawMain,
                                        ImVec2 selectorSize = ImVec2(133.0f, 60.0f))
{
  auto validChoices = BuildFilterSelectionChoices(isSupported, isResponseAllowed);

  auto findChoiceIndex = [&]() -> int
  {
    for (size_t i = 0; i < validChoices.size(); ++i)
    {
      if (validChoices[i] == selectedResponse)
        return (int)i;
    }
    return 0;
  };

  ImGui::PushID(idSeed);

  bool openPopup = false;
  const bool popupAllowed = validChoices.size() > 1;

  ImGui::PushID("FilterSelectorMain");
  ImGui::Button("##filtermain", selectorSize);
  if (popupAllowed && ImGui::IsItemClicked())
  {
    openPopup = true;
  }

  ImRect bb(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
  drawMain(bb);
  ImGui::PopID();

  ImGui::SameLine();
  ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(3, 0));
  {
      ImGuiGroupScope _grp;
    if (ImGui::SmallButton("+"))
    {
      int idx = findChoiceIndex();
      idx = (idx + 1) % (int)validChoices.size();
      selectedResponse = validChoices[(size_t)idx];
      applySelection(selectedResponse);
    }
    if (ImGui::SmallButton("-"))
    {
      int idx = findChoiceIndex();
      idx = (idx - 1 + (int)validChoices.size()) % (int)validChoices.size();
      selectedResponse = validChoices[(size_t)idx];
      applySelection(selectedResponse);
    }
  }
  ImGui::PopStyleVar();

  if (openPopup)
  {
    std::string popupName = std::string("selectFilterPopup##") + id;
    ImGui::OpenPopup(popupName.c_str());
  }

  std::string popupName = std::string("selectFilterPopup##") + id;
  if (ImGui::BeginPopup(popupName.c_str()))
  {
    static constexpr int kColumns = 4;
    for (size_t i = 0; i < validChoices.size(); ++i)
    {
      if (i > 0 && (i % kColumns) != 0)
        ImGui::SameLine();

      ImGui::PushID((int)i);
      const auto response = validChoices[i];
      const bool selected = selectedResponse == response;
      if (selected)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
      }

      if (ImGui::Button(LabelForFilterResponseShort(response), ImVec2(42, 0)))
      {
        selectedResponse = response;
        applySelection(selectedResponse);
        ImGui::CloseCurrentPopup();
      }

      if (selected)
      {
        ImGui::PopStyleColor();
      }
      ImGui::PopID();
    }

    ImGui::EndPopup();
  }

  ImGui::PopID();
}

template <typename IsSupportedFn, typename ApplySelectionFn, typename DrawMainFn>
inline void RenderFilterSelectionWidget(const char* id,
                                        int idSeed,
                                        M7::FilterResponse selectedResponse,
                                        IsSupportedFn isSupported,
                                        ApplySelectionFn applySelection,
                                        DrawMainFn drawMain,
                                        ImVec2 selectorSize = ImVec2(133.0f, 60.0f))
{
  RenderFilterSelectionWidget(id,
                              idSeed,
                              selectedResponse,
                              isSupported,
                              [](M7::FilterResponse)
                              { return true; },
                              applySelection,
                              drawMain,
                              selectorSize);
}

}  // namespace WaveSabreVstLib
