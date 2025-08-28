#pragma once
#include <imgui.h>
#include "Common.h"
#include "Maj7VstUtils.hpp"
#include "WaveSabreVstLib.h"
#include <WaveSabreCore/DelayCore.hpp>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <vector>

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{

inline void DelayVisualization(float leftPeriodMS, float rightPeriodMS, float feedbackAmp, float crossMix10)
{
  ImRect bb;
  bb.Min = ImGui::GetCursorScreenPos();
  ImVec2 size{500, 100};
  bb.Max = bb.Min + size;
  auto dl = ImGui::GetWindowDrawList();

  static constexpr float beatsToDisplay = 6;
  static constexpr float beatsOffsetX = 0.5f;
  static constexpr float beginBeat = -beatsOffsetX;
  static constexpr float endBeat = beatsToDisplay - beatsOffsetX;
  auto beatsToX = [&](float beats)
  {
    return M7::math::map(beats, -beatsOffsetX, endBeat, bb.Min.x, bb.Max.x);
  };
  auto panToY = [&](float panN11)
  {
    return M7::math::map(panN11, -2, 2, bb.Min.y, bb.Max.y);
  };
  auto msToBeats = [&](float ms)
  {
    float msPerBeat = 60000.0f / Helpers::CurrentTempo;
    return ms / msPerBeat;
  };

  ImGui::RenderFrame(bb.Min, bb.Max, ColorFromHTML("222222"));
  ImGui::InvisibleButton("viz", size);

  // render beat grid
  static constexpr float subbeats = 8;
  for (int i = 0; i < beatsToDisplay; ++i)
  {
    if (i < endBeat)
    {
      float x = std::round(beatsToX(float(i)));
      dl->AddLine({x, bb.Min.y}, {x, bb.Max.y}, ColorFromHTML("777777"), 2);
    }
    for (int is = 0; is < subbeats - 1; ++is)
    {
      float beats = (float)i + ((is + 1) / subbeats);
      float x = std::round(beatsToX(beats));
      if (beats < endBeat)
      {
        dl->AddLine({x, bb.Min.y}, {x, bb.Min.y + 15}, ColorFromHTML("555555", 0.5f), 1);
      }
    }
  }

  static constexpr float mainRadius = 20;

  float leftPeriodBeats = msToBeats(leftPeriodMS);
  float leftBeat = 0;
  float leftRadius = mainRadius;
  float panL = -1;
  float panR = 1;
  int i = 0;

  //float feedbackAmp = mpEcho->mParams.GetLinearVolume((int)Echo::ParamIndices::FeedbackLevel, M7::gVolumeCfg6db, 0);
  float feedbackAmp01 = M7::math::sqrt01(M7::math::clamp01(feedbackAmp));

  float fbFactor = feedbackAmp01;
  float alpha = 1.0f;
  while (leftBeat < endBeat)
  {
    dl->AddLine({beatsToX(leftBeat), bb.Min.y}, {beatsToX(leftBeat), bb.Max.y}, ColorFromHTML(bandColors[0], 0.4f), 1);
    dl->AddCircleFilled({beatsToX(leftBeat), panToY(panL)}, leftRadius, ColorFromHTML(bandColors[0], alpha));

    leftBeat += leftPeriodBeats;
    leftRadius *= fbFactor;
    alpha *= fbFactor;
    i++;
    if (leftPeriodMS < 2 || i > 100)
      break;
    float xpanL = M7::math::lerp(panL, panR, crossMix10);
    panR = M7::math::lerp(panR, panL, crossMix10);
    panL = xpanL;
  }

  float rightPeriodBeats = msToBeats(rightPeriodMS);
  float rightBeat = 0;
  float rightRadius = mainRadius;
  panL = -1;
  panR = 1;
  i = 0;
  while (rightBeat < endBeat)
  {
    dl->AddLine({beatsToX(rightBeat), bb.Min.y},
                {beatsToX(rightBeat), bb.Max.y},
                ColorFromHTML(bandColors[1], 0.2f),
                1);
    dl->AddCircleFilled({beatsToX(rightBeat), panToY(panR)}, rightRadius, ColorFromHTML(bandColors[1], 0.8f));

    rightBeat += rightPeriodBeats;
    rightRadius *= fbFactor;
    alpha *= fbFactor;
    i++;
    if (rightPeriodMS < 2 || i > 100)
      break;
    float xpanL = M7::math::lerp(panL, panR, crossMix10);
    panR = M7::math::lerp(panR, panL, crossMix10);
    panL = xpanL;
  }

  char s[100];
  std::sprintf(s, "%d ms left\r\n%d ms right", (int)std::round(leftPeriodMS), (int)std::round(rightPeriodMS));

  dl->AddText(bb.Min, ColorFromHTML("999999"), s);
}
}  // namespace WaveSabreVstLib
