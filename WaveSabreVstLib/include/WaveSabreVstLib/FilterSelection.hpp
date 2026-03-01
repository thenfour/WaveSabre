#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

#include <WaveSabreCore/Maj7Filter.hpp>
#include <imgui.h>
#include "ImGuiUtils.hpp"

namespace WaveSabreVstLib
{
using namespace WaveSabreCore;

struct FilterSelectionChoice
{
  M7::FilterCircuit circuit;
  M7::FilterSlope slope;
  M7::FilterResponse response;
};

inline constexpr std::array<M7::FilterCircuit, 7> gFilterCircuitOrder = {
    M7::FilterCircuit::Disabled,
    M7::FilterCircuit::OnePole,
    M7::FilterCircuit::Biquad,
    M7::FilterCircuit::Butterworth,
    M7::FilterCircuit::Moog,
    M7::FilterCircuit::K35,
    M7::FilterCircuit::Diode,
};

inline constexpr std::array<const char*, (size_t)M7::FilterCircuit::Count> gFilterCircuitLabels = {
    "Off", "1Pole", "Biquad", "Butter", "Moog", "K35", "Diode",
};

inline constexpr std::array<M7::FilterSlope, 9> gFilterSlopeOrder = {
    M7::FilterSlope::Slope6dbOct,
    M7::FilterSlope::Slope12dbOct,
    M7::FilterSlope::Slope24dbOct,
    M7::FilterSlope::Slope36dbOct,
    M7::FilterSlope::Slope48dbOct,
    M7::FilterSlope::Slope60dbOct,
    M7::FilterSlope::Slope72dbOct,
    M7::FilterSlope::Slope84dbOct,
    M7::FilterSlope::Slope96dbOct,
};

inline constexpr std::array<const char*, 9> gFilterSlopeLabels = {
    "6", "12", "24", "36", "48", "60", "72", "84", "96",
};

inline constexpr std::array<M7::FilterResponse, 8> gFilterResponseOrder = {
    M7::FilterResponse::Lowpass,
    M7::FilterResponse::LowShelf,
    M7::FilterResponse::Bandpass,
    M7::FilterResponse::Notch,
    M7::FilterResponse::Allpass,
    M7::FilterResponse::Peak,
    M7::FilterResponse::HighShelf,
    M7::FilterResponse::Highpass,
};

inline constexpr std::array<const char*, 8> gFilterResponseLabels = {
    "\\ LP", "  LS", "| BP", "- Nt", "* AP", "^ PK", "  HS", "/ HP",
};

inline const char* LabelForFilterCircuit(M7::FilterCircuit c)
{
  for (size_t i = 0; i < gFilterCircuitOrder.size(); ++i)
  {
    if (gFilterCircuitOrder[i] == c)
      return gFilterCircuitLabels[i];
  }
  return "?";
}

inline const char* LabelForFilterSlope(M7::FilterSlope s)
{
  for (size_t i = 0; i < gFilterSlopeOrder.size(); ++i)
  {
    if (gFilterSlopeOrder[i] == s)
      return gFilterSlopeLabels[i];
  }
  return "-";
}

inline const char* LabelForFilterResponse(M7::FilterResponse r)
{
  for (size_t i = 0; i < gFilterResponseOrder.size(); ++i)
  {
    if (gFilterResponseOrder[i] == r)
      return gFilterResponseLabels[i];
  }
  return "-";
}

template <typename IsSupportedFn>
inline std::vector<FilterSelectionChoice> BuildFilterSelectionChoices(IsSupportedFn isSupported)
{
  std::vector<FilterSelectionChoice> validChoices;
  constexpr size_t kMaxChoices =
      size_t(M7::FilterCircuit::Count) * size_t(M7::FilterSlope::Count) * size_t(M7::FilterResponse::Count);
  validChoices.reserve(kMaxChoices);

  if (isSupported(M7::FilterCircuit::Disabled, M7::FilterSlope::Slope6dbOct, M7::FilterResponse::Lowpass))
  {
    validChoices.push_back({M7::FilterCircuit::Disabled, M7::FilterSlope::Slope6dbOct, M7::FilterResponse::Lowpass});
  }

  for (auto c : gFilterCircuitOrder)
  {
    if (c == M7::FilterCircuit::Disabled)
      continue;

    for (auto r : gFilterResponseOrder)
    {
      for (auto s : gFilterSlopeOrder)
      {
        if (isSupported(c, s, r))
        {
          validChoices.push_back({c, s, r});
        }
      }
    }
  }

  if (validChoices.empty())
  {
    validChoices.push_back({M7::FilterCircuit::Disabled, M7::FilterSlope::Slope6dbOct, M7::FilterResponse::Lowpass});
  }

  return validChoices;
}

template <typename IsSupportedFn, typename ApplySelectionFn, typename DrawMainFn>
inline void RenderFilterSelectionWidget(const char* id,
                                        int idSeed,
                                        M7::FilterCircuit selectedCircuit,
                                        M7::FilterSlope selectedSlope,
                                        M7::FilterResponse selectedResponse,
                                        IsSupportedFn isSupported,
                                        ApplySelectionFn applySelection,
                                        DrawMainFn drawMain,
                                        ImVec2 selectorSize = ImVec2(133.0f, 60.0f))
{
  auto validChoices = BuildFilterSelectionChoices(isSupported);

  auto findChoiceIndex = [&]() -> int
  {
    for (size_t i = 0; i < validChoices.size(); ++i)
    {
      const auto& v = validChoices[i];
      if (v.circuit == selectedCircuit && v.slope == selectedSlope && v.response == selectedResponse)
        return (int)i;
    }
    return 0;
  };

  ImGui::PushID(idSeed);

  bool openPopup = false;
  const bool popupAllowed = (selectedCircuit != M7::FilterCircuit::Disabled);

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
      auto c = validChoices[(size_t)idx];
      applySelection(c.circuit, c.slope, c.response);
      selectedCircuit = c.circuit;
      selectedSlope = c.slope;
      selectedResponse = c.response;
    }
    if (ImGui::SmallButton("-"))
    {
      int idx = findChoiceIndex();
      idx = (idx - 1 + (int)validChoices.size()) % (int)validChoices.size();
      auto c = validChoices[(size_t)idx];
      applySelection(c.circuit, c.slope, c.response);
      selectedCircuit = c.circuit;
      selectedSlope = c.slope;
      selectedResponse = c.response;
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
    for (size_t i = 1; i < gFilterCircuitOrder.size(); ++i)
    {
      if (i > 1)
        ImGui::SameLine();

      ImGui::PushID((int)i);
      const bool selected = (selectedCircuit == gFilterCircuitOrder[i]);
      if (selected)
      {
        ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
      }

      if (ImGui::Button(gFilterCircuitLabels[i]))
      {
        selectedCircuit = gFilterCircuitOrder[i];
        bool matchedCurrent = isSupported(selectedCircuit, selectedSlope, selectedResponse);
        if (!matchedCurrent)
        {
          bool found = false;
          for (auto r : gFilterResponseOrder)
          {
            for (auto s : gFilterSlopeOrder)
            {
              if (isSupported(selectedCircuit, s, r))
              {
                selectedSlope = s;
                selectedResponse = r;
                found = true;
                break;
              }
            }
            if (found)
              break;
          }
        }

        applySelection(selectedCircuit, selectedSlope, selectedResponse);
      }

      if (selected)
      {
        ImGui::PopStyleColor();
      }
      ImGui::PopID();
    }

    ImGui::Separator();

    const int nCols = 1 + (int)gFilterSlopeOrder.size();
    if (ImGui::BeginTable("##filterGrid", nCols, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV))
    {
      ImGui::TableNextRow();
      ImGui::TableSetColumnIndex(0);
      ImGui::TextUnformatted("Resp\\Slope");

      for (size_t is = 0; is < gFilterSlopeOrder.size(); ++is)
      {
        ImGui::TableSetColumnIndex((int)is + 1);
        ImGui::TextUnformatted(gFilterSlopeLabels[is]);
      }

      for (size_t ir = 0; ir < gFilterResponseOrder.size(); ++ir)
      {
        ImGui::PushID((int)(ir + 100));
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::TextUnformatted(gFilterResponseLabels[ir]);

        for (size_t is = 0; is < gFilterSlopeOrder.size(); ++is)
        {
          ImGui::PushID((int)(is + 1000));
          ImGui::TableSetColumnIndex((int)is + 1);

          const auto slope = gFilterSlopeOrder[is];
          const auto response = gFilterResponseOrder[ir];
          const bool supported = isSupported(selectedCircuit, slope, response);
          const bool selected = supported && (selectedSlope == slope) && (selectedResponse == response);

          if (!supported)
          {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.35f);
            ImGui::Button("-##unsupported", ImVec2(28, 0));
            ImGui::PopStyleVar();
            ImGui::PopID();
            continue;
          }

          if (selected)
          {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyle().Colors[ImGuiCol_ButtonActive]);
          }

          if (ImGui::Button("o##choice", ImVec2(28, 0)))
          {
            selectedSlope = slope;
            selectedResponse = response;
            applySelection(selectedCircuit, selectedSlope, selectedResponse);
          }

          if (selected)
          {
            ImGui::PopStyleColor();
          }

          ImGui::PopID();
        }

        ImGui::PopID();
      }

      ImGui::EndTable();
    }

    ImGui::EndPopup();
  }

  ImGui::PopID();
}

}  // namespace WaveSabreVstLib
