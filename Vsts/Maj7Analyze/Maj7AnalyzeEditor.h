#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7AnalyzeVst.h"

#include <WaveSabreCore/StereoImageAnalysis.hpp>
#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>
#include <WaveSabreVstLib/HistoryVisualization.hpp>
#include <WaveSabreVstLib/Width/CorrelationMeter.hpp>
#include <WaveSabreVstLib/Width/Goniometer.hpp>
#include <WaveSabreVstLib/Width/PolarL.hpp>
#include <WaveSabreVstLib/Width/StereoBalance.hpp>


struct Maj7AnalyzeEditor : public VstEditor
{
  Maj7Analyze* mpMaj7Analyze;
  Maj7AnalyzeVst* mpMaj7AnalyzeVst;

  // Frequency analysis visualization (width by frequency)
  FrequencyResponseRendererLayered<800, 200, 0, (size_t)WaveSabreCore::Maj7Analyze::ParamIndices::NumParams, false>
      mWidthGraph;

  // FFT overlay toggles (single signal)
  bool mShowSignalFFT = true;
  bool mShowSignalMid = false;
  bool mShowSignalSide = true;
  bool mShowSignalWidth = false;

  // Persist across sessions (FFT toggles)
  VstSerializableBoolParamRef mShowSignalFFTParam{"ShowSignalFFT", mShowSignalFFT};
  VstSerializableBoolParamRef mShowSignalMidParam{"ShowSignalMidFFT", mShowSignalMid};
  VstSerializableBoolParamRef mShowSignalSideParam{"ShowSignalSideFFT", mShowSignalSide};
  VstSerializableBoolParamRef mShowSignalWidthParam{"ShowSignalWidthFFT", mShowSignalWidth};

  const char* kSignalMidFftColor = bandColors[0];
  const char* kSignalSideFftColor = bandColors[1];
  const char* kSignalWidthFftColor = bandColors[2];
  const char* kSignalFftColor = bandColors[3];

  // Stereo visualization layer toggles
  bool mShowGoniometerLines = false;
  bool mShowGoniometerPoints = true;
  bool mShowPolarL = true;
  bool mShowPhaseX = true;

  VstSerializableBoolParamRef mShowGoniometerLinesParam{"ShowGoniometerLines", mShowGoniometerLines};
  VstSerializableBoolParamRef mShowGoniometerPointsParam{"ShowGoniometerPoints", mShowGoniometerPoints};
  VstSerializableBoolParamRef mShowPolarLParam{"ShowPolarL", mShowPolarL};
  VstSerializableBoolParamRef mShowPhaseXParam{"ShowPhaseX", mShowPhaseX};

  Maj7AnalyzeEditor(AudioEffect* audioEffect)
      : VstEditor(audioEffect, 850, 850)
      , mpMaj7AnalyzeVst((Maj7AnalyzeVst*)audioEffect)
  {
    mpMaj7Analyze = ((Maj7AnalyzeVst*)audioEffect)->GetMaj7Analyze();
  }


  virtual void PopulateMenuBar() override
  {
    MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
    PopulateStandardMenuBar(mCurrentWindow,
                            "Maj7 Analyze",
                            mpMaj7Analyze,
                            mpMaj7AnalyzeVst,
                            "gParamDefaults",
                            "ParamIndices::NumParams",
                            mpMaj7Analyze->mParamCache,
                            paramNames);
  }


  virtual void renderImgui() override
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    ImGui::BeginGroup();

    // VU Meter Ticks
    static const std::vector<VUMeterTick> tickSet = {
        {-3.0f, "3db"},
        {-6.0f, "6db"},
        {-12.0f, "12db"},
        {-18.0f, "18db"},
        {-24.0f, "24db"},
        {-30.0f, "30db"},
        {-40.0f, "40db"},
    };

    VUMeterConfig mainCfg = {
        {24, 300},
        VUMeterLevelMode::Audio,
        VUMeterUnits::Linear,
        -50,
        6,
        tickSet,
    };

    // Signal meters (single stream)
    ImGui::SameLine();
    VUMeter("vu_sig",
            mpMaj7Analyze->mLoudnessAnalysis[0],
            mpMaj7Analyze->mLoudnessAnalysis[1],
            mainCfg,
            "Input left channel",
            "Input right channel");
    ImGui::SameLine();
    VUMeterMS("ms_sig",
              mpMaj7Analyze->mInputImagingAnalysis.mMidLevelDetector,
              mpMaj7Analyze->mInputImagingAnalysis.mSideLevelDetector,
              mainCfg,
              "Mid channel",
              "Side channel");

    ImGui::SameLine();
    {
      ImGuiGroupScope _grp("stereo_imaging_sig");
      RenderStereoImagingDisplay("stereo_imaging_sig", mpMaj7Analyze->mInputImagingAnalysis);

      // Layer toggles
      ToggleButton(&mShowGoniometerLines, "Lines");
      ImGui::SameLine();
      ToggleButton(&mShowGoniometerPoints, "Points");
      ImGui::SameLine();
      ToggleButton(&mShowPolarL, "Poly");
      ImGui::SameLine();
      ToggleButton(&mShowPhaseX, "Scissor");
    }

    mStereoHistory.Render(true,
                          mpMaj7Analyze->mInputImagingAnalysis,
                          mpMaj7Analyze->mLoudnessAnalysis[0],
                          mpMaj7Analyze->mLoudnessAnalysis[1]);

    // Frequency Analysis Controls and Graph
    ImGui::BeginGroup();
    if (!mpMaj7Analyze->mInputImagingAnalysis.IsFrequencyAnalysisEnabled())
    {
      mpMaj7Analyze->mInputImagingAnalysis.SetFrequencyAnalysisEnabled(true);
    }

    ImGui::Separator();
    ImGui::Text("Freq overlays:");

    ToggleButton(&mShowSignalFFT, "Signal", {}, ButtonColorSpec{kSignalFftColor});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Main signal");
      ImGui::EndTooltip();
    }

    ImGui::SameLine();
    ToggleButton(&mShowSignalMid, "Mid", {}, ButtonColorSpec{kSignalMidFftColor});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Mid / center channel (dB)");
      ImGui::EndTooltip();
    }

    ImGui::SameLine();
    ToggleButton(&mShowSignalSide, "Side", {}, ButtonColorSpec{kSignalSideFftColor});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Side channel (dB)");
      ImGui::EndTooltip();
    }
    ImGui::SameLine();
    ToggleButton(&mShowSignalWidth, "Width", {}, ButtonColorSpec{kSignalWidthFftColor});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Width (normalized side) mapped to dB scale");
      ImGui::EndTooltip();
    }

    RenderFrequencyAnalysis();
    ImGui::EndGroup();

    ImGui::EndGroup();
#else
    ImGui::Text("Analysis features are disabled in this build.");
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  }

  virtual std::vector<IVstSerializableParam*> GetVstOnlyParams() override
  {
    return {
        // Stereo layers
        &mShowGoniometerLinesParam,
        &mShowGoniometerPointsParam,
        &mShowPolarLParam,
        &mShowPhaseXParam,

        // FFT overlays
        &mShowSignalFFTParam,
        &mShowSignalMidParam,
        &mShowSignalSideParam,
        &mShowSignalWidthParam,

        // Stereo history toggles
        &mShowLeftPeakParam,
        &mShowRightPeakParam,
        &mShowPhaseCorrelationParam,
        &mShowStereoWidthParam,
        &mShowStereoBalanceParam,
        &mShowMidLevelParam,
        &mShowSideLevelParam,
    };
  }

private:
  // Render frequency analysis graph (single stream)
  void RenderFrequencyAnalysis()
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    const auto* analyzer = mpMaj7Analyze->mInputImagingAnalysis.GetFrequencyAnalyzer();
    if (!analyzer)
    {
      ImGui::Text("Frequency analysis not available");
      return;
    }

    static constexpr float scaleMinDb = -90.0f;
    const float scaleMin = scaleMinDb;
    const float scaleMax = 0.0f;

    FrequencyResponseRendererConfig<0, (size_t)WaveSabreCore::Maj7Analyze::ParamIndices::NumParams> cfg{
        ColorFromHTML("222222", 1.0f),  // background
        ColorFromHTML("aaaa00", 1.0f),  // line (unused)
        0.0f,                           // thumb radius (no thumbs)
        {},                             // filters
        {},                             // param cache copy
        {},                             // major ticks
        {},                             // minor ticks
        {},                             // fft overlays
        scaleMin,                       // min
        scaleMax,                       // max
        true                            // independent scale
    };
    for (size_t i = 0; i < (size_t)WaveSabreCore::Maj7Analyze::ParamIndices::NumParams; ++i)
      cfg.mParamCacheCopy[i] = mpMaj7Analyze->mParamCache[i];

    cfg.fftOverlays.clear();

    if (mShowSignalFFT)
    {
      cfg.fftOverlays.push_back({&mpMaj7Analyze->mFFT,
                                 ColorFromHTML(kSignalFftColor, 0.9f),
                                 ColorFromHTML(kSignalFftColor, 0.25f),
                                 true,
                                 "Signal",
                                 nullptr});
    }

    // Mid/Side (dB)
    if (mShowSignalMid && analyzer->GetMidAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzer->GetMidAnalyzer(),
                                 ColorFromHTML(kSignalMidFftColor, 0.9f),
                                 ColorFromHTML(kSignalMidFftColor, 0.25f),
                                 true,
                                 "Mid",
                                 nullptr});
    }
    if (mShowSignalSide && analyzer->GetSideAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzer->GetSideAnalyzer(),
                                 ColorFromHTML(kSignalSideFftColor, 0.9f),
                                 ColorFromHTML(kSignalSideFftColor, 0.25f),
                                 true,
                                 "Side",
                                 nullptr});
    }

    // Map Width (0..~3) into dB scale [-90..0] for consistent view
    auto widthAsDbScale = [](float v) -> float
    {
      if (v <= 0.0f)
        return v;
      const float a = 1.8f;
      float t01 = std::log1p(a * std::max(0.0f, v)) / std::log1p(a * 3.0f);
      return M7::math::lerp(scaleMinDb, 0.0f, t01);
    };
    std::function<float(float)> widthTransform = std::function<float(float)>(widthAsDbScale);

    if (mShowSignalWidth)
    {
      cfg.fftOverlays.push_back({analyzer,
                                 ColorFromHTML(kSignalWidthFftColor, 0.9f),
                                 ColorFromHTML(kSignalWidthFftColor, 0.25f),
                                 true,
                                 "Width",
                                 widthTransform});
    }

    mWidthGraph.OnRender(cfg);
#endif
  }

private:
  // Stereo history visualization (single stream)
  template <int historyViewWidth, int historyViewHeight>
  struct SingleStereoHistoryVis
  {
    // Parameter visibility toggles
    bool mShowPhaseCorrelation = true;
    bool mShowStereoWidth = true;
    bool mShowStereoBalance = true;
    bool mShowMidLevel = false;
    bool mShowSideLevel = false;

    bool mShowLeftPeak = false;
    bool mShowLeftRMS = false;
    bool mShowRightPeak = false;
    bool mShowRightRMS = false;

    static constexpr float historyViewMinValue = -1.0f;  // For correlation (-1 to +1)
    static constexpr float historyViewMaxValue = +2.0f;  // For width (0 to 2+)

    HistoryView<10, historyViewWidth, historyViewHeight> mHistoryView;

    const char* kCorrelationColor = "00ccff";
    const char* kWidthColor = "88ff44";
    const char* kBalanceColor = "ff8844";
    const char* kMidLevelColor = "bb88ff";
    const char* kSideLevelColor = "ffbb88";

    const char* kLeftPeakColor = bandColors[0];
    const char* kRightPeakColor = bandColors[2];

    void Render(bool showToggles,
                const StereoImagingAnalysisStream& analysis,
                const AnalysisStream& analStreamL,
                const AnalysisStream& analStreamR)
    {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

      ImGuiGroupScope __group{"history"};
      static constexpr float lineWidth = 2.0f;

      mHistoryView.RenderCustom(
          {
              // reference lines
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.2f), 1, -1.0f, +1.0f, true},
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.1f), 1, -1.0f, +1.0f, true},
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.1f), 1, -1.0f, +1.0f, true},

              HistoryViewSeriesConfig{
                  ColorFromHTML(kLeftPeakColor, mShowLeftPeak ? 0.9f : 0), lineWidth, 0, +1.0f, true},
              HistoryViewSeriesConfig{
                  ColorFromHTML(kRightPeakColor, mShowRightPeak ? 0.9f : 0), lineWidth, 0, +1.0f, true},

              // single signal series with per-series scaling
              HistoryViewSeriesConfig{
                  ColorFromHTML(kCorrelationColor, (mShowPhaseCorrelation) ? 0.9f : 0), lineWidth, -1.0f, +1.0f, true},
              HistoryViewSeriesConfig{
                  ColorFromHTML(kWidthColor, (mShowStereoWidth) ? 0.8f : 0), lineWidth, 0.0f, 3.0f, true},
              HistoryViewSeriesConfig{
                  ColorFromHTML(kBalanceColor, (mShowStereoBalance) ? 0.7f : 0), lineWidth, -1.0f, +1.0f, true},
              HistoryViewSeriesConfig{
                  ColorFromHTML(kMidLevelColor, (mShowMidLevel) ? 0.6f : 0), lineWidth, 0.0f, 1.0f, true},
              HistoryViewSeriesConfig{
                  ColorFromHTML(kSideLevelColor, (mShowSideLevel) ? 0.6f : 0), lineWidth, 0.0f, 1.0f, true},
          },
          {
              0,
              -0.5f,
              +0.5f,
              static_cast<float>(analStreamL.mCurrentPeak),
              static_cast<float>(analStreamR.mCurrentPeak),
              static_cast<float>(analysis.mPhaseCorrelation),
              static_cast<float>(analysis.mStereoWidth),
              static_cast<float>(analysis.mStereoBalance),
              static_cast<float>(analysis.mMidLevelDetector.mCurrentRMSValue),
              static_cast<float>(analysis.mSideLevelDetector.mCurrentRMSValue),
          },
          historyViewMinValue,
          historyViewMaxValue);

      if (showToggles)
      {
        ButtonArray<7>({
            MakeButtonSpec("L Peak", &mShowLeftPeak, kLeftPeakColor),
            MakeButtonSpec("R Peak", &mShowRightPeak, kRightPeakColor),
            MakeButtonSpec("Correllation", &mShowPhaseCorrelation, kCorrelationColor),
            MakeButtonSpec("Width", &mShowStereoWidth, kWidthColor),
            MakeButtonSpec("Balance", &mShowStereoBalance, kBalanceColor),
            MakeButtonSpec("Mid", &mShowMidLevel, kMidLevelColor),
            MakeButtonSpec("Side", &mShowSideLevel, kSideLevelColor),
        });
      }
#else
      ImGui::Text("Analysis features are disabled in this build.");
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    }
  };

  SingleStereoHistoryVis<800, 120> mStereoHistory;

  // Persist toggles for single-stream history
  VstSerializableBoolParamRef mShowLeftPeakParam{"ShowLeftPeak", mStereoHistory.mShowLeftPeak};
  VstSerializableBoolParamRef mShowRightPeakParam{"ShowRightPeak", mStereoHistory.mShowRightPeak};
  VstSerializableBoolParamRef mShowPhaseCorrelationParam{"ShowPhaseCorrelation", mStereoHistory.mShowPhaseCorrelation};
  VstSerializableBoolParamRef mShowStereoWidthParam{"ShowStereoWidth", mStereoHistory.mShowStereoWidth};
  VstSerializableBoolParamRef mShowStereoBalanceParam{"ShowStereoBalance", mStereoHistory.mShowStereoBalance};
  VstSerializableBoolParamRef mShowMidLevelParam{"ShowMidLevel", mStereoHistory.mShowMidLevel};
  VstSerializableBoolParamRef mShowSideLevelParam{"ShowSideLevel", mStereoHistory.mShowSideLevel};


  // Main layered visualization renderer (single stream)
  void RenderLayeredStereoVisualization(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size)
  {
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImRect bb(pos, pos + size);

    // Background and grid (always shown)
    dl->AddRectFilled(bb.Min, bb.Max, IM_COL32(20, 20, 20, 255));
    dl->AddRect(bb.Min, bb.Max, IM_COL32(100, 100, 100, 255));

    ImVec2 center = {bb.Min.x + size.x * 0.5f, bb.Min.y + size.y * 0.5f};
    const float radius = std::min(size.x, size.y) * 0.45f;

    // Draw concentric circles for magnitude reference
    for (int i = 1; i <= 4; ++i)
    {
      float r = radius * i * 0.25f;
      dl->AddCircle(center, r, IM_COL32(60, 60, 60, 100), 32, 1.0f);
    }

    // Draw angular reference lines
    for (int angle = 0; angle < 360; angle += 30)
    {
      float rad = angle * 3.14159f / 180.0f;
      ImVec2 lineEnd = {center.x + cos(rad) * radius, center.y + sin(rad) * radius};
      ImU32 color = (angle % 90 == 0) ? IM_COL32(120, 120, 120, 150) : IM_COL32(80, 80, 80, 100);
      dl->AddLine(center, lineEnd, color, (angle % 90 == 0) ? 1.5f : 1.0f);
    }

    // Add reference labels for key angles
    dl->AddText({center.x - 4, bb.Min.y + 2}, IM_COL32(100, 255, 100, 150), "M");      // Mono
    dl->AddText({bb.Max.x - 15, center.y - 8}, IM_COL32(150, 150, 255, 150), "R");     // Right
    dl->AddText({center.x - 20, bb.Max.y - 16}, IM_COL32(255, 100, 100, 150), "inv");  // Side/Phase
    dl->AddText({bb.Min.x + 2, center.y - 8}, IM_COL32(150, 150, 255, 150), "L");      // Left

    // Render layers in order (bottom to top)
    if (mShowPolarL)
    {
      RenderPolarLLayer(id, analysis, size, center, radius);
    }

    if (mShowGoniometerPoints)
    {
      RenderGoniometerLayer(id, analysis, size, center, radius, true);
    }

    if (mShowGoniometerLines)
    {
      RenderGoniometerLayer(id, analysis, size, center, radius, false);
    }

    if (mShowPhaseX)
    {
      RenderPhaseCorrelationOverlay(id, analysis, size, center, radius);
    }

    ImGui::Dummy(size);
  }


  void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& analysis)
  {
    ImGuiGroupScope _scope(id);

    static constexpr int dim = 250;
    ImVec2 meterSize(dim, 30);

    RenderCorrelationMeter("correlation", analysis.mPhaseCorrelation, {dim, 30});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Phase correlation");
      ImGui::Separator();
      ImGui::Text("Indicates stereo phase correlation:");
      ImGui::BulletText("-1.0: out of phase (180\xC2\xB0)");
      ImGui::BulletText(" 0.0: No correlation (random phase)");
      ImGui::BulletText("+1.0: Perfectly in phase (0\xC2\xB0)");
      ImGui::EndTooltip();
    }

    RenderGeneralMeter(
        analysis.mStereoWidth, 0, 1, meterSize, "Stereo width", 0, {"008800", 0.5, "ffff00", 1.0, "ff0000"});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Stereo width");
      ImGui::Separator();
      ImGui::Text("Indicates how much of the total signal energy is the side channel.");
      ImGui::BulletText("0.0: Mono (no side channel)");
      ImGui::BulletText("1.0: Widest possible (no mid signal)");
      ImGui::EndTooltip();
    }

    RenderGeneralMeter(analysis.mStereoBalance,
                       -1,
                       1,
                       meterSize,
                       "Left-right balance",
                       0,
                       {
                           "ff0000",
                           -0.5,
                           "ffff00",
                           -0.2,
                           "00ff00",
                           +0.2,
                           "ffff00",
                           +0.5,
                           "ff0000",
                       });

    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Left-right balance");
      ImGui::Separator();
      ImGui::Text("Indicates the balance between left and right channels:");
      ImGui::BulletText("-1.0: Full left (right channel silent)");
      ImGui::BulletText(" 0.0: Centered (equal left/right)");
      ImGui::BulletText("+1.0: Full right (left channel silent)");
      ImGui::EndTooltip();
    }

    RenderLayeredStereoVisualization("stereovis", analysis, {dim, dim});
  }
};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
  return new Maj7AnalyzeEditor(audioEffect);
}
