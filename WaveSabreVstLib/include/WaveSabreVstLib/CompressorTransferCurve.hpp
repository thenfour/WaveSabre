#ifndef __WAVESABREVSTLIB_COMPRESSORTRANSFERCURVE_H__
#define __WAVESABREVSTLIB_COMPRESSORTRANSFERCURVE_H__

#include <imgui.h>
#include "Common.h"
#include "HistoryVisualization.hpp"
#include "Maj7VstUtils.hpp"
#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <functional>
#include <vector>

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{

inline void CompressorTransferCurve(const MonoCompressor& c, ImVec2 size, AnalysisStream (&detectorAnalysis)[2])
{
  ImRect bb;
  bb.Min = ImGui::GetCursorScreenPos();
  bb.Max = bb.Min + size;

  ImColor backgroundColor = ColorFromHTML("222222", 1.0f);
  ImColor curveColor = ColorFromHTML("ffff00", 1.0f);
  ImColor gridColor = ColorFromHTML("444444", 1.0f);
  ImColor detectorColorAtt = ColorFromHTML("ed4d4d", 0.9f);
  ImColor detectorColor = ColorFromHTML("eeeeee", 0.9f);

  ImGui::RenderFrame(bb.Min, bb.Max, backgroundColor);

  static constexpr int segmentCount = 33;
  std::vector<ImVec2> points;

  auto dbToY = [&](float dB)
  {
    float lin = dB / gHistoryViewMinDB;
    float t01 = M7::math::clamp01(1.0f - lin);
    return bb.Max.y - t01 * size.y;
  };

  auto dbToX = [&](float dB)
  {
    float lin = dB / gHistoryViewMinDB;
    float t01 = M7::math::clamp01(1.0f - lin);
    return bb.Min.x + t01 * size.y;
  };

  for (int iSeg = 0; iSeg < segmentCount; ++iSeg)
  {
    float inLin = float(iSeg) / (segmentCount - 1);   // touch 0 and 1
    float dbIn = (1.0f - inLin) * gHistoryViewMinDB;  // db from gHistoryViewMinDB to 0.
    float dbAttenOut = c.TransferDecibels(dbIn);
    float outDB = dbIn - dbAttenOut;
    points.push_back(ImVec2{bb.Min.x + inLin * size.x, dbToY(outDB)});
  }

  auto* dl = ImGui::GetWindowDrawList();

  // linear guideline
  dl->AddLine({bb.Min.x, bb.Max.y}, {bb.Max.x, bb.Min.y}, gridColor, 1.0f);

  // threshold guideline
  float threshY = dbToY(c.mThreshold);
  dl->AddLine({bb.Min.x, threshY}, {bb.Max.x, threshY}, gridColor, 1.0f);

  dl->AddPolyline(points.data(), (int)points.size(), curveColor, 0, 2.0f);

  // detector indicator dot
  double detectorLevel = std::max(detectorAnalysis[0].mCurrentPeak, detectorAnalysis[1].mCurrentPeak);
  float detectorDB = M7::math::LinearToDecibels(float(detectorLevel));

  float detAtten = c.TransferDecibels(detectorDB);
  float detOutDB = detectorDB - detAtten;

  if (detAtten > 0.00001)
  {
    dl->AddCircleFilled({dbToX(detectorDB), dbToY(detOutDB)}, 6.0f, detectorColorAtt);
  }
  else
  {
    dl->AddCircleFilled({dbToX(detectorDB), dbToY(detOutDB)}, 3.0f, detectorColor);
  }

  ImGui::Dummy(size);
}

struct TransferCurveColors
{
  ImColor background;
  ImColor line;
  ImColor lineClip;
  ImColor tick;
};

inline void RenderTransferCurve(ImVec2 size, const TransferCurveColors& colors, std::function<float(float)> transferFn)
{
  auto* dl = ImGui::GetWindowDrawList();
  ImRect bb;
  bb.Min = ImGui::GetCursorScreenPos();
  bb.Max = bb.Min + size;
  ImGui::RenderFrame(bb.Min, bb.Max, colors.background);

  static constexpr float gMinLin = -1.5f;
  static constexpr float gMaxLin = 1.5f;

  auto LinToY = [&](float lin)
  {
    return M7::math::map(lin, gMinLin, gMaxLin, bb.Max.y, bb.Min.y);
  };

  auto LinToX = [&](float lin)
  {
    return M7::math::map(lin, gMinLin, gMaxLin, bb.Min.x, bb.Max.x);
  };

  static constexpr int segmentCount = 16;
  std::vector<ImVec2> points;
  std::vector<ImVec2> clipPoints;

  for (int i = 0; i < segmentCount; ++i)
  {
    float t01 = float(i) / (segmentCount - 1);  // touch 0 and 1
    float tLin = M7::math::lerp(gMinLin, gMaxLin, t01);
    float yLin = transferFn(tLin);
    if (tLin >= 1)
    {
      if (clipPoints.empty())
      {
        points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
      }
      clipPoints.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
    }
    else
    {
      points.push_back(ImVec2(LinToX(tLin), LinToY(yLin)));
    }
  }

  dl->AddLine({bb.Min.x, bb.Max.y}, {bb.Max.x, bb.Min.y}, colors.tick,
              1);  // diag linear tick
  dl->AddLine({bb.Min.x, LinToY(1)}, {bb.Max.x, LinToY(1)}, colors.tick, 1);
  dl->AddLine({bb.Min.x, LinToY(0)}, {bb.Max.x, LinToY(0)}, colors.tick, 1);
  dl->AddLine({bb.Min.x, LinToY(-1)}, {bb.Max.x, LinToY(-1)}, colors.tick, 1);
  dl->AddLine({LinToX(1), bb.Min.y}, {LinToX(1), bb.Max.y}, colors.tick, 1);
  dl->AddLine({LinToX(0), bb.Min.y}, {LinToX(0), bb.Max.y}, colors.tick, 1);
  dl->AddLine({LinToX(-1), bb.Min.y}, {LinToX(-1), bb.Max.y}, colors.tick, 1);

  dl->AddPolyline(points.data(), (int)points.size(), colors.line, 0, 3);
  dl->AddPolyline(clipPoints.data(), (int)clipPoints.size(), colors.lineClip, 0, 3);

  ImGui::Dummy(size);
}

}  // namespace WaveSabreVstLib

#endif
