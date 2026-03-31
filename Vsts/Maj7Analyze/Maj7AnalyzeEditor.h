#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7AnalyzeVst.h"

#include <WaveSabreCore/../../Analysis/StereoImageAnalysis.hpp>
#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>
#include <WaveSabreVstLib/HistoryVisualization.hpp>
#include <WaveSabreVstLib/Width/StereoImagingDisplay.hpp>


struct Maj7AnalyzeEditor : public VstEditor
{
  Maj7Analyze* mpMaj7Analyze;
  Maj7AnalyzeVst* mpMaj7AnalyzeVst;

  // Frequency analysis visualization (width by frequency)
  FrequencyResponseRendererLayered<800, 200, 0, (size_t)WaveSabreCore::Maj7Analyze::ParamIndices::NumParams, false>
      mWidthGraph;

  // FFT overlay toggles (single signal)
  bool mShowSignalLeft = false;
  bool mShowSignalRight = false;
  bool mShowSignalStereo = true;
  bool mShowSignalMid = false;
  bool mShowSignalSide = true;
  bool mShowSignalWidth = false;

  // Persist across sessions (FFT toggles)
  VstSerializableBoolParamRef mShowSignalLeftParam{"ShowSignalLeftFFT", mShowSignalLeft};
  VstSerializableBoolParamRef mShowSignalRightParam{"ShowSignalRightFFT", mShowSignalRight};
  VstSerializableBoolParamRef mShowSignalStereoParam{"ShowSignalStereoFFT", mShowSignalStereo};
  VstSerializableBoolParamRef mShowSignalMidParam{"ShowSignalMidFFT", mShowSignalMid};
  VstSerializableBoolParamRef mShowSignalSideParam{"ShowSignalSideFFT", mShowSignalSide};
  VstSerializableBoolParamRef mShowSignalWidthParam{"ShowSignalWidthFFT", mShowSignalWidth};

  const char* kSignalLeftFftColor = "4f7ddb";
  const char* kSignalRightFftColor = "cc6b7a";
  const char* kSignalMidFftColor = bandColors[0];
  const char* kSignalSideFftColor = bandColors[1];
  const char* kSignalWidthFftColor = bandColors[2];
  const char* kSignalStereoFftColor = bandColors[3];
  const char* kScopeLinesColor = "66b3ff";
  const char* kScopePointsColor = "ffcc66";
  const char* kScopePolyColor = "66dd88";
  const char* kScopeScissorColor = "cc66ff";

  // Stereo visualization layer toggles
  bool mShowGoniometerLines = false;
  bool mShowGoniometerPoints = true;
  bool mShowPolarL = false;
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
        {
          VUMeterTooltipStripScope tooltipStrip{"analyze_vu_strip"};
          VUMeter("vu_sig",
            mpMaj7Analyze->mLoudnessAnalysis[0],
            mpMaj7Analyze->mLoudnessAnalysis[1],
            mainCfg,
            "Input Left",
            "Input Right",
            &tooltipStrip);
          ImGui::SameLine();
          VUMeterMS("ms_sig",
        mpMaj7Analyze->mInputImagingAnalysis.mMidLevelDetector,
        mpMaj7Analyze->mInputImagingAnalysis.mSideLevelDetector,
        mainCfg,
        "Input Mid",
        "Input Side",
        &tooltipStrip);
        }

    ImGui::SameLine();
    {
      ImGuiGroupScope _grp("stereo_imaging_sig");
      RenderStereoImagingDisplay("stereo_imaging_sig",
                 mpMaj7Analyze->mInputImagingAnalysis,
                 {mShowPolarL, mShowGoniometerPoints, mShowGoniometerLines, mShowPhaseX});

      // Layer toggles
      ButtonArray<4>("analyze_scope_layers", {
          MakeButtonSpec("Lines", &mShowGoniometerLines, kScopeLinesColor, "Show line trails in the stereo scope."),
          MakeButtonSpec("Points", &mShowGoniometerPoints, kScopePointsColor, "Show point cloud dots in the stereo scope."),
          MakeButtonSpec("Poly", &mShowPolarL, kScopePolyColor, "Show the polygon / polar envelope layer."),
          MakeButtonSpec("Wedge", &mShowPhaseX, kScopeScissorColor, "Show the wedge-style phase view."),
      });
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

    ButtonArray<6>("analyze_fft_overlays", {
      MakeButtonSpec("L", &mShowSignalLeft, kSignalLeftFftColor, "Show the left-channel FFT overlay."),
      MakeButtonSpec("R", &mShowSignalRight, kSignalRightFftColor, "Show the right-channel FFT overlay."),
      MakeButtonSpec("Stereo", &mShowSignalStereo, kSignalStereoFftColor, "Show the combined stereo FFT overlay."),
        MakeButtonSpec("Mid", &mShowSignalMid, kSignalMidFftColor, "Show the mid / center FFT overlay."),
        MakeButtonSpec("Side", &mShowSignalSide, kSignalSideFftColor, "Show the side FFT overlay."),
        MakeButtonSpec("Width", &mShowSignalWidth, kSignalWidthFftColor, "Show width as a normalized side FFT overlay."),
    });

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
        &mShowSignalLeftParam,
        &mShowSignalRightParam,
        &mShowSignalStereoParam,
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

    if (mShowSignalLeft)
    {
      cfg.fftOverlays.push_back({&mpMaj7Analyze->mFFT.GetLeftView(),
                                 ColorFromHTML(kSignalLeftFftColor, 0.9f),
                                 ColorFromHTML(kSignalLeftFftColor, 0.25f),
                                 true,
                                 "Left",
                                 nullptr});
    }

    if (mShowSignalRight)
    {
      cfg.fftOverlays.push_back({&mpMaj7Analyze->mFFT.GetRightView(),
                                 ColorFromHTML(kSignalRightFftColor, 0.9f),
                                 ColorFromHTML(kSignalRightFftColor, 0.25f),
                                 true,
                                 "Right",
                                 nullptr});
    }

    if (mShowSignalStereo)
    {
      cfg.fftOverlays.push_back({&mpMaj7Analyze->mFFT,
                                 ColorFromHTML(kSignalStereoFftColor, 0.9f),
                                 ColorFromHTML(kSignalStereoFftColor, 0.25f),
                                 true,
                                 "Stereo",
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

          std::array<HistoryTooltipSeriesConfig, 10> historyTooltipCfg{};
          historyTooltipCfg[3] = HistoryTooltipSeriesConfig{"Left Peak", ColorFromHTML(kLeftPeakColor, 0.9f), true, HistoryTooltipValueFormat::LinearToDecibels};
          historyTooltipCfg[4] = HistoryTooltipSeriesConfig{"Right Peak", ColorFromHTML(kRightPeakColor, 0.9f), true, HistoryTooltipValueFormat::LinearToDecibels};
          historyTooltipCfg[5] = HistoryTooltipSeriesConfig{"Correlation", ColorFromHTML(kCorrelationColor, 0.9f), true, HistoryTooltipValueFormat::SignedFloat, 2};
          historyTooltipCfg[6] = HistoryTooltipSeriesConfig{"Width", ColorFromHTML(kWidthColor, 0.9f), true, HistoryTooltipValueFormat::UnsignedFloat, 2, "x"};
          historyTooltipCfg[7] = HistoryTooltipSeriesConfig{"Balance", ColorFromHTML(kBalanceColor, 0.9f), true, HistoryTooltipValueFormat::SignedFloat, 2};
          historyTooltipCfg[8] = HistoryTooltipSeriesConfig{"Mid", ColorFromHTML(kMidLevelColor, 0.9f), true, HistoryTooltipValueFormat::LinearToDecibels};
          historyTooltipCfg[9] = HistoryTooltipSeriesConfig{"Side", ColorFromHTML(kSideLevelColor, 0.9f), true, HistoryTooltipValueFormat::LinearToDecibels};

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
              historyViewMaxValue,
              &historyTooltipCfg);

      if (showToggles)
      {
        ButtonArray<7>("displayOptions1" , {
            MakeButtonSpec("L Peak", &mShowLeftPeak, kLeftPeakColor, "Show the left-channel peak history."),
            MakeButtonSpec("R Peak", &mShowRightPeak, kRightPeakColor, "Show the right-channel peak history."),
            MakeButtonSpec("Correllation", &mShowPhaseCorrelation, kCorrelationColor, "Show stereo phase correlation history."),
            MakeButtonSpec("Width", &mShowStereoWidth, kWidthColor, "Show stereo width history."),
            MakeButtonSpec("Balance", &mShowStereoBalance, kBalanceColor, "Show stereo balance history."),
            MakeButtonSpec("Mid", &mShowMidLevel, kMidLevelColor, "Show mid-channel level history."),
            MakeButtonSpec("Side", &mShowSideLevel, kSideLevelColor, "Show side-channel level history."),
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


};

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
  return new Maj7AnalyzeEditor(audioEffect);
}
