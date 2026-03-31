#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "Maj7WidthVst.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <string>
#include <vector>


#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>
#include <WaveSabreVstLib/HistoryVisualization.hpp>
#include <WaveSabreVstLib/Width/CorrelationMeter.hpp>
#include <WaveSabreVstLib/Width/Goniometer.hpp>
#include <WaveSabreVstLib/Width/PolarL.hpp>
#include <WaveSabreVstLib/Width/StereoBalance.hpp>


struct Maj7WidthEditor : public VstEditor
{
  Maj7Width* mpMaj7Width;
  Maj7WidthVst* mpMaj7WidthVst;

  // Frequency analysis visualization (width by frequency)
  FrequencyResponseRendererLayered<800, 200, 0, (size_t)WaveSabreCore::Maj7Width::ParamIndices::NumParams, false>
      mWidthGraph;

  // FFT overlay toggles
  bool mShowInputMid = false;
  bool mShowInputSide = true;
  bool mShowInputWidth = false;
  bool mShowOutputMid = false;
  bool mShowOutputSide = true;
  bool mShowOutputWidth = false;

  // Persist across sessions
  VstSerializableBoolParamRef mShowInputMidParam{"ShowInputMidFFT", mShowInputMid};
  VstSerializableBoolParamRef mShowInputSideParam{"ShowInputSideFFT", mShowInputSide};
  VstSerializableBoolParamRef mShowInputWidthParam{"ShowInputWidthFFT", mShowInputWidth};
  VstSerializableBoolParamRef mShowOutputMidParam{"ShowOutputMidFFT", mShowOutputMid};
  VstSerializableBoolParamRef mShowOutputSideParam{"ShowOutputSideFFT", mShowOutputSide};
  VstSerializableBoolParamRef mShowOutputWidthParam{"ShowOutputWidthFFT", mShowOutputWidth};

  const char* kInputMidFftColor = "664444";
  const char* kInputSideFftColor = "446644";
  const char* kInputWidthFftColor = "444466";

  const char* kOutputMidFftColor = bandColors[0];
  const char* kOutputSideFftColor = bandColors[1];
  const char* kOutputWidthFftColor = bandColors[2];
  const char* kScopeLinesColor = "66b3ff";
  const char* kScopePointsColor = "ffcc66";
  const char* kScopePolyColor = "66dd88";
  const char* kScopeScissorColor = "cc66ff";


  Maj7WidthEditor(AudioEffect* audioEffect)
      :  //
      VstEditor(audioEffect, 950, 900)
      ,  // Increase height to accommodate frequency graph
      mpMaj7WidthVst((Maj7WidthVst*)audioEffect)
  {
    mpMaj7Width = ((Maj7WidthVst*)audioEffect)->GetMaj7Width();
  }

  virtual void PopulateMenuBar() override
  {
    MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
    PopulateStandardMenuBar(mCurrentWindow,
                            "Maj7 Width",
                            mpMaj7Width,
                            mpMaj7WidthVst,
                            "gParamDefaults",
                            "ParamIndices::NumParams",
                            mpMaj7Width->mParamCache,
                            paramNames);
  }

  virtual void renderImgui() override
  {
    {
      ImGuiGroupScope _grp;
      // Left/Right Source Controls with Tooltip
      ImGui::BeginGroup();
      Maj7ImGuiParamFloatN11WithCenter(
          (VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LeftSource, "Left source", -1, -1, 0, {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Left Channel Source");
        ImGui::Separator();
        ImGui::Text("Select where the left channel originates from:");
        ImGui::BulletText("-1.0: Pure left input signal");
        ImGui::BulletText(" 0.0: Mono mix (L+R)/2");
        ImGui::BulletText("+1.0: Pure right input signal");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
      Maj7ImGuiParamFloatN11WithCenter(
          (VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RightSource, "Right source", 1, 1, 0, {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Right Channel Source");
        ImGui::Separator();
        ImGui::Text("Select where the right channel originates from:");
        ImGui::BulletText("-1.0: Pure left input signal");
        ImGui::BulletText(" 0.0: Mono mix (L+R)/2");
        ImGui::BulletText("+1.0: Pure right input signal");
        ImGui::EndTooltip();
      }
      ImGui::EndGroup();

      // Rotation Control with Tooltip
      ImGui::BeginGroup();
      Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle,
                                "Rotation",
                                -M7::math::gPI,
                                M7::math::gPI,
                                0,
                                0,
                                0,
                                {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Stereo Field Rotation");
        ImGui::Separator();
        ImGui::Text("Rotate the stereo image around the center point.");
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.7f, 1.0f), "Use Cases:");
        ImGui::BulletText("Correct imbalanced recordings");
        ImGui::BulletText("Fix poor microphone placement");
        ImGui::BulletText("Creative stereo field manipulation");
        ImGui::EndTooltip();
      }
      ImGui::EndGroup();

      // Side HPF Control with Tooltip
      ImGui::BeginGroup();
      Maj7ImGuiParamFrequency((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::SideHPFrequency,
                              -1,
                              "Side HPF",
                              M7::gFilterFreqConfig,
                              0,
                              {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Side Channel High-Pass Filter");
        ImGui::Separator();
        ImGui::Text("Reduces stereo width of low frequencies using a 6dB/octave slope.");
        ImGui::EndTooltip();
      }
      ImGui::EndGroup();

      // Mid-Side Balance Control with Tooltip
      ImGui::BeginGroup();
      Maj7ImGuiParamFloatN11(
          (VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidSideBalance, "Mid-Side balance", 0.0f, 0, {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Mid-Side Balance Control");
        ImGui::Separator();
        ImGui::Text("Blend between mono (mid) and stereo (side) content:");
        ImGui::EndTooltip();
      }
      ImGui::EndGroup();

      // Final Output Section with Tooltips
      {
        ImGuiGroupScope _grp;

        Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});

        ImGui::SameLine();
        Maj7ImGuiParamVolume((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::OutputGain,
                             "Output",
                             WaveSabreCore::Maj7Width::gVolumeCfg,
                             0,
                             {});
      }

    }  // group scope

    static const std::vector<VUMeterTick> tickSet = {
        {-3.0f, "3db"},
        {-6.0f, "6db"},
        {-12.0f, "12db"},
        {-18.0f, "18db"},
        {-24.0f, "24db"},
        {-30.0f, "30db"},
        {-40.0f, "40db"},
        //{-50.0f, "50db"},
    };

    VUMeterConfig mainCfg = {
        {24, 300},
        VUMeterLevelMode::Audio,
        VUMeterUnits::Linear,
        -50,
        6,
        tickSet,
    };

    VUMeterConfig attenCfg = mainCfg;
    attenCfg.levelMode = VUMeterLevelMode::Attenuation;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

    ImGui::SameLine();
    {
      VUMeterTooltipStripScope tooltipStrip{"width_vu_strip"};
      VUMeter("vu_inp",
              mpMaj7Width->mInputAnalysis[0],
              mpMaj7Width->mInputAnalysis[1],
              mainCfg,
              "Input Left",
              "Input Right",
              &tooltipStrip);
      ImGui::SameLine();
      VUMeter("vu_outp",
              mpMaj7Width->mOutputAnalysis[0],
              mpMaj7Width->mOutputAnalysis[1],
              mainCfg,
              "Output Left",
              "Output Right",
              &tooltipStrip);
      ImGui::SameLine();
      VUMeterMS("ms_inp",
                mpMaj7Width->mInputImagingAnalysis.mMidLevelDetector,
                mpMaj7Width->mInputImagingAnalysis.mSideLevelDetector,
                mainCfg,
                "Input Mid",
                "Input Side",
                &tooltipStrip);
      ImGui::SameLine();
      VUMeterMS("ms_outp",
                mpMaj7Width->mOutputImagingAnalysis.mMidLevelDetector,
                mpMaj7Width->mOutputImagingAnalysis.mSideLevelDetector,
                mainCfg,
                "Output Mid",
                "Output Side",
                &tooltipStrip);
    }

    ImGui::SameLine();
    // Stereo imaging visualization
    {
      ImGuiGroupScope _grp("stereo_imaging");
      RenderStereoImagingDisplay("stereo_imaging_inp", mpMaj7Width->mInputImagingAnalysis);
      ImGui::SameLine();
      RenderStereoImagingDisplay("stereo_imaging_outp", mpMaj7Width->mOutputImagingAnalysis);

      // Toggle buttons for layer visibility
      {
        ButtonArray<4>(
            "width_scope_layers",
            {
                MakeButtonSpec(
                    "Lines", &mShowGoniometerLines, kScopeLinesColor, "Show line trails in the stereo scope."),
                MakeButtonSpec(
                    "Points", &mShowGoniometerPoints, kScopePointsColor, "Show point cloud dots in the stereo scope."),
                MakeButtonSpec("Poly", &mShowPolarL, kScopePolyColor, "Show the polygon / polar envelope layer."),
                MakeButtonSpec("Scissor", &mShowPhaseX, kScopeScissorColor, "Show the scissor-style phase view."),
            });
      }
      {
        mStereoHistory.Render(true, mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis);
      }
    }

    // Frequency analysis visualization (when enabled)

    // Frequency Analysis Controls
    {
      ImGuiGroupScope _grp;
      bool frequencyAnalysisEnabled = mpMaj7Width->mInputImagingAnalysis.IsFrequencyAnalysisEnabled();
      if (!frequencyAnalysisEnabled)
      {
        mpMaj7Width->mInputImagingAnalysis.SetFrequencyAnalysisEnabled(true);
        mpMaj7Width->mOutputImagingAnalysis.SetFrequencyAnalysisEnabled(true);
      }

      // FFT overlay toggles and scale selection
      {
        ImGui::Separator();
        ImGui::Text("Freq overlays:");
        ButtonArray<3>(
            "width_fft_input",
            {
                MakeButtonSpec("In M", &mShowInputMid, kInputMidFftColor, "Show the input mid / center FFT overlay."),
                MakeButtonSpec("In S", &mShowInputSide, kInputSideFftColor, "Show the input side FFT overlay."),
                MakeButtonSpec("In W", &mShowInputWidth, kInputWidthFftColor, "Show the input width FFT overlay."),
            });
        ImGui::SameLine(0, 20);
        ButtonArray<3>(
            "width_fft_output",
            {
                MakeButtonSpec(
                    "Out M", &mShowOutputMid, kOutputMidFftColor, "Show the output mid / center FFT overlay."),
                MakeButtonSpec("Out S", &mShowOutputSide, kOutputSideFftColor, "Show the output side FFT overlay."),
                MakeButtonSpec("Out W", &mShowOutputWidth, kOutputWidthFftColor, "Show the output width FFT overlay."),
            });
      }

      RenderFrequencyAnalysis();
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  }

  virtual std::vector<IVstSerializableParam*> GetVstOnlyParams() override
  {
    return {
        &mShowGoniometerLinesParam,
        &mShowGoniometerPointsParam,
        &mShowPolarLParam,
        &mShowPhaseXParam,
        &mShowPhaseCorrelationParam,
        &mShowStereoWidthParam,
        &mShowStereoBalanceParam,
        &mShowMidLevelParam,
        &mShowSideLevelParam,
        &mShowInputParam,
        &mShowOutputParam,
        &mShowInputMidParam,
        &mShowInputSideParam,
        &mShowInputWidthParam,
        &mShowOutputMidParam,
        &mShowOutputSideParam,
        &mShowOutputWidthParam,
    };
  }


private:
  bool mShowGoniometerLines = false;
  bool mShowGoniometerPoints = true;
  bool mShowPolarL = true;
  bool mShowPhaseX = true;

  VstSerializableBoolParamRef mShowGoniometerLinesParam{"ShowGoniometerLines", mShowGoniometerLines};
  VstSerializableBoolParamRef mShowGoniometerPointsParam{"ShowGoniometerPoints", mShowGoniometerPoints};
  VstSerializableBoolParamRef mShowPolarLParam{"ShowPolarL", mShowPolarL};
  VstSerializableBoolParamRef mShowPhaseXParam{"ShowPhaseX", mShowPhaseX};
  VstSerializableBoolParamRef mShowPhaseCorrelationParam{"ShowPhaseCorrelation", mStereoHistory.mShowPhaseCorrelation};
  VstSerializableBoolParamRef mShowStereoWidthParam{"ShowStereoWidth", mStereoHistory.mShowStereoWidth};
  VstSerializableBoolParamRef mShowStereoBalanceParam{"ShowStereoBalance", mStereoHistory.mShowStereoBalance};
  VstSerializableBoolParamRef mShowMidLevelParam{"ShowMidLevel", mStereoHistory.mShowMidLevel};
  VstSerializableBoolParamRef mShowSideLevelParam{"ShowSideLevel", mStereoHistory.mShowSideLevel};
  VstSerializableBoolParamRef mShowInputParam{"ShowInput", mStereoHistory.mShowInput};
  VstSerializableBoolParamRef mShowOutputParam{"ShowOutput", mStereoHistory.mShowOutput};

  // Render frequency analysis graph
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  void RenderFrequencyAnalysis()
  {
    const auto* analyzerIn = mpMaj7Width->mInputImagingAnalysis.GetFrequencyAnalyzer();
    const auto* analyzerOut = mpMaj7Width->mOutputImagingAnalysis.GetFrequencyAnalyzer();

    if (!analyzerIn || !analyzerOut)
    {
      ImGui::Text("Frequency analysis not available");
      return;
    }

    // Decide scale based on which overlays are active.
    const bool anyMidSide = (mShowInputMid || mShowInputSide || mShowOutputMid || mShowOutputSide);
    const bool anyWidth = (mShowInputWidth || mShowOutputWidth);

    // If user enabled any M/S overlays, use a dB scale (-60..0) suited for them.
    // Otherwise, use width scale (0..3.2).
    static constexpr float scaleMinDb = -90.0f;
    const float scaleMin = scaleMinDb;  // anyMidSide ? -60.0f : 0.0f;
    const float scaleMax = 0;           // anyMidSide ? 0.0f : 3.2f;

    FrequencyResponseRendererConfig<0, (size_t)WaveSabreCore::Maj7Width::ParamIndices::NumParams> cfg{
        ColorFromHTML("222222", 1.0f),  // background
        ColorFromHTML("aaaa00", 1.0f),  // line (unused)
        0.0f,                           // thumb radius (no thumbs)
        {},                             // filters
        {},                             // param cache copy (unused)
        {},                             // major ticks (defaults)
        {},                             // minor ticks (defaults)
        {},                             // fft overlays (fill below)
        scaleMin,                       // min
        scaleMax,                       // max
        true                            // independent scale
    };
    for (size_t i = 0; i < (size_t)WaveSabreCore::Maj7Width::ParamIndices::NumParams; ++i)
      cfg.mParamCacheCopy[i] = mpMaj7Width->mParamCache[i];

    cfg.fftOverlays.clear();

    // Mid/Side in dB domain
    if (mShowInputMid && analyzerIn->GetMidAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzerIn->GetMidAnalyzer(),
                                 ColorFromHTML(kInputMidFftColor, 0.9f),
                                 ColorFromHTML(kInputMidFftColor, 0.25f),
                                 true,
                                 "Input Mid",
                                 nullptr});
    }
    if (mShowInputSide && analyzerIn->GetSideAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzerIn->GetSideAnalyzer(),
                                 ColorFromHTML(kInputSideFftColor, 0.9f),
                                 ColorFromHTML(kInputSideFftColor, 0.25f),
                                 true,
                                 "Input Side",
                                 nullptr});
    }
    if (mShowOutputMid && analyzerOut->GetMidAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzerOut->GetMidAnalyzer(),
                                 ColorFromHTML(kOutputMidFftColor, 0.9f),
                                 ColorFromHTML(kOutputMidFftColor, 0.25f),
                                 true,
                                 "Output Mid",
                                 nullptr});
    }
    if (mShowOutputSide && analyzerOut->GetSideAnalyzer())
    {
      cfg.fftOverlays.push_back({analyzerOut->GetSideAnalyzer(),
                                 ColorFromHTML(kOutputSideFftColor, 0.9f),
                                 ColorFromHTML(kOutputSideFftColor, 0.25f),
                                 true,
                                 "Output Side",
                                 nullptr});
    }

    // Width overlays: choose transform based on chosen scale
    //auto widthLog = [](float w)->float { const float a = 1.8f; return std::log1p(a * std::max(0.0f, w)) / std::log1p(a * 3.0f); };
    auto widthAsDbScale = [](float v) -> float
    {
      // If v is a dB endpoint (negative range), pass-through so min/max remain [-60..0]
      if (v <= 0.0f)
        return v;
      // Otherwise v is width in [0..3], map to [-60..0] using the same perceptual curve as widthLog
      const float a = 1.8f;
      float t01 = std::log1p(a * std::max(0.0f, v)) / std::log1p(a * 3.0f);  // 0..1
      return M7::math::lerp(scaleMinDb, 0.0f, t01);                          // map to [-60..0]
    };

    std::function<float(float)> widthTransform = std::function<float(float)>(widthAsDbScale);
    if (mShowInputWidth)
      cfg.fftOverlays.push_back({analyzerIn,
                                 ColorFromHTML(kInputWidthFftColor, 0.9f),
                                 ColorFromHTML(kInputWidthFftColor, 0.25f),
                                 true,
                                 "Input Width",
                                 widthTransform});
    if (mShowOutputWidth)
      cfg.fftOverlays.push_back({analyzerOut,
                                 ColorFromHTML(kOutputWidthFftColor, 0.9f),
                                 ColorFromHTML(kOutputWidthFftColor, 0.25f),
                                 true,
                                 "Output Width",
                                 widthTransform});

    mWidthGraph.OnRender(cfg);
  }
#endif

  // Stereo history visualization system
  template <int historyViewWidth, int historyViewHeight>
  struct StereoHistoryVis
  {
    // Parameter visibility toggles
    bool mShowPhaseCorrelation = true;
    bool mShowStereoWidth = true;
    bool mShowStereoBalance = true;
    bool mShowMidLevel = false;
    bool mShowSideLevel = false;
    bool mShowInput = true;
    bool mShowOutput = true;

    static constexpr float historyViewMinValue = -1.0f;  // For correlation (-1 to +1)
    static constexpr float historyViewMaxValue = +2.0f;  // For width (0 to 2+)

    // Expanded to 10 series: 5 parameters × 2 signals (input + output)
    HistoryView<13, historyViewWidth, historyViewHeight> mHistoryView;

    const char* kInputCorrellationColor = "00ccff";
    const char* kOutputCorrellationColor = "0099cc";
    const char* kInputWidthColor = "88ff44";
    const char* kOutputWidthColor = "66cc33";
    const char* kInputBalanceColor = "ff8844";
    const char* kOutputBalanceColor = "cc6633";
    const char* kInputMidLevelColor = "bb88ff";
    const char* kOutputMidLevelColor = "9966cc";
    const char* kInputSideLevelColor = "ffbb88";
    const char* kOutputSideLevelColor = "cc9966";

    void Render(bool showToggles,
                const StereoImagingAnalysisStream& inputAnalysis,
                const StereoImagingAnalysisStream& outputAnalysis)
    {
      ImGui::BeginGroup();
      static constexpr float lineWidth = 2.0f;

      const std::array<HistoryTooltipSeriesConfig, 13> historyTooltipCfg = {
          HistoryTooltipSeriesConfig{nullptr, ImColor(), false},
          HistoryTooltipSeriesConfig{nullptr, ImColor(), false},
          HistoryTooltipSeriesConfig{nullptr, ImColor(), false},
          HistoryTooltipSeriesConfig{"Input Correlation",
                                     ColorFromHTML(kInputCorrellationColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::SignedFloat,
                                     2},
          HistoryTooltipSeriesConfig{"Input Width",
                                     ColorFromHTML(kInputWidthColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::UnsignedFloat,
                                     2,
                                     "x"},
          HistoryTooltipSeriesConfig{"Input Balance",
                                     ColorFromHTML(kInputBalanceColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::SignedFloat,
                                     2},
          HistoryTooltipSeriesConfig{
              "Input Mid", ColorFromHTML(kInputMidLevelColor, 0.9f), true, HistoryTooltipValueFormat::LinearToDecibels},
          HistoryTooltipSeriesConfig{"Input Side",
                                     ColorFromHTML(kInputSideLevelColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::LinearToDecibels},
          HistoryTooltipSeriesConfig{"Output Correlation",
                                     ColorFromHTML(kOutputCorrellationColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::SignedFloat,
                                     2},
          HistoryTooltipSeriesConfig{"Output Width",
                                     ColorFromHTML(kOutputWidthColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::UnsignedFloat,
                                     2,
                                     "x"},
          HistoryTooltipSeriesConfig{"Output Balance",
                                     ColorFromHTML(kOutputBalanceColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::SignedFloat,
                                     2},
          HistoryTooltipSeriesConfig{"Output Mid",
                                     ColorFromHTML(kOutputMidLevelColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::LinearToDecibels},
          HistoryTooltipSeriesConfig{"Output Side",
                                     ColorFromHTML(kOutputSideLevelColor, 0.9f),
                                     true,
                                     HistoryTooltipValueFormat::LinearToDecibels},
      };

      mHistoryView.RenderCustom(
          {
              // a unity line
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.2f),
                                      1,  // line width
                                      -1.0f,
                                      +1.0f,
                                      true},
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.1f),
                                      1,  // line width
                                      -1.0f,
                                      +1.0f,
                                      true},
              HistoryViewSeriesConfig{ColorFromHTML("ffffff", 0.1f),
                                      1,  // line width
                                      -1.0f,
                                      +1.0f,
                                      true},
              // INPUT SIGNALS (brighter colors) with per-series scaling
              // Phase correlation: -1 to +1, bright cyan/blue
              HistoryViewSeriesConfig{
                  ColorFromHTML(kInputCorrellationColor, (mShowInput && mShowPhaseCorrelation) ? 0.9f : 0),
                  lineWidth,
                  -1.0f,
                  +1.0f,
                  true  // Custom scale for correlation
              },

              // Stereo width: 0 to 3, bright green/yellow
              HistoryViewSeriesConfig{
                  ColorFromHTML(kInputWidthColor, (mShowInput && mShowStereoWidth) ? 0.8f : 0),
                  lineWidth,
                  0.0f,
                  3.0f,
                  true  // Custom scale for width (0 to 3 for better visibility)
              },

              // Stereo balance: -1 to +1, bright red/orange
              HistoryViewSeriesConfig{
                  ColorFromHTML(kInputBalanceColor, (mShowInput && mShowStereoBalance) ? 0.7f : 0),
                  lineWidth,
                  -1.0f,
                  +1.0f,
                  true  // Custom scale for balance
              },

              // Mid level: 0 to 1, bright purple
              HistoryViewSeriesConfig{
                  ColorFromHTML(kInputMidLevelColor, (mShowInput && mShowMidLevel) ? 0.6f : 0),
                  lineWidth,
                  0.0f,
                  1.0f,
                  true  // Custom scale for RMS levels
              },

              // Side level: 0 to 1, bright orange
              HistoryViewSeriesConfig{
                  ColorFromHTML(kInputSideLevelColor, (mShowInput && mShowSideLevel) ? 0.6f : 0),
                  lineWidth,
                  0.0f,
                  1.0f,
                  true  // Custom scale for RMS levels
              },

              // OUTPUT SIGNALS (darker variants) with per-series scaling
              // Phase correlation: -1 to +1, darker cyan/blue
              HistoryViewSeriesConfig{
                  ColorFromHTML(kOutputCorrellationColor, (mShowOutput && mShowPhaseCorrelation) ? 0.8f : 0),
                  lineWidth,
                  -1.0f,
                  +1.0f,
                  true  // Custom scale for correlation
              },

              // Stereo width: 0 to 3, darker green
              HistoryViewSeriesConfig{
                  ColorFromHTML(kOutputWidthColor, (mShowOutput && mShowStereoWidth) ? 0.7f : 0),
                  lineWidth,
                  0.0f,
                  3.0f,
                  true  // Custom scale for width
              },

              // Stereo balance: -1 to +1, darker red/orange
              HistoryViewSeriesConfig{
                  ColorFromHTML(kOutputBalanceColor, (mShowOutput && mShowStereoBalance) ? 0.6f : 0),
                  lineWidth,
                  -1.0f,
                  +1.0f,
                  true  // Custom scale for balance
              },

              // Mid level: 0 to 1, darker purple
              HistoryViewSeriesConfig{
                  ColorFromHTML(kOutputMidLevelColor, (mShowOutput && mShowMidLevel) ? 0.5f : 0),
                  lineWidth,
                  0.0f,
                  1.0f,
                  true  // Custom scale for RMS levels
              },

              // Side level: 0 to 1, darker orange
              HistoryViewSeriesConfig{
                  ColorFromHTML(kOutputSideLevelColor, (mShowOutput && mShowSideLevel) ? 0.5f : 0),
                  lineWidth,
                  0.0f,
                  1.0f,
                  true  // Custom scale for RMS levels
              },
          },
          {
              0,
              -0.5f,
              +0.5f,
              // INPUT VALUES (first 5 series) - raw values, no normalization needed
              static_cast<float>(inputAnalysis.mPhaseCorrelation),                    // -1 to +1
              static_cast<float>(inputAnalysis.mStereoWidth),                         // 0 to 3+
              static_cast<float>(inputAnalysis.mStereoBalance),                       // -1 to +1
              static_cast<float>(inputAnalysis.mMidLevelDetector.mCurrentRMSValue),   // 0 to 1
              static_cast<float>(inputAnalysis.mSideLevelDetector.mCurrentRMSValue),  // 0 to 1

              // OUTPUT VALUES (last 5 series) - raw values, no normalization needed
              static_cast<float>(outputAnalysis.mPhaseCorrelation),                    // -1 to +1
              static_cast<float>(outputAnalysis.mStereoWidth),                         // 0 to 3+
              static_cast<float>(outputAnalysis.mStereoBalance),                       // -1 to +1
              static_cast<float>(outputAnalysis.mMidLevelDetector.mCurrentRMSValue),   // 0 to 1
              static_cast<float>(outputAnalysis.mSideLevelDetector.mCurrentRMSValue),  // 0 to 1
          },
          historyViewMinValue,
          historyViewMaxValue,
          &historyTooltipCfg);

      if (showToggles)
      {
        ButtonArray<5>(
            "width_history_metrics",
            {
                MakeButtonSpec("Correllation",
                               &mShowPhaseCorrelation,
                               kInputCorrellationColor,
                               "Show stereo correlation history."),
                MakeButtonSpec("Width", &mShowStereoWidth, kInputWidthColor, "Show stereo width history."),
                MakeButtonSpec("Balance", &mShowStereoBalance, kInputBalanceColor, "Show stereo balance history."),
                MakeButtonSpec("Mid", &mShowMidLevel, kInputMidLevelColor, "Show mid-channel level history.", 20.0f),
                MakeButtonSpec("Side", &mShowSideLevel, kInputSideLevelColor, "Show side-channel level history."),
            });

        ImGui::SameLine(0, 40);
        ButtonArray<2>(
            "width_history_groups",
            {
                MakeButtonSpec("Input", &mShowInput, kInputCorrellationColor, "Show input history series."),
                MakeButtonSpec("Output", &mShowOutput, kOutputCorrellationColor, "Show output history series."),
            });
      }
      ImGui::EndGroup();
    }
  };

  StereoHistoryVis<508, 120> mStereoHistory;

  void RenderStereoImagingDisplay(const char* id, const StereoImagingAnalysisStream& analysis)
  {
    ImGuiGroupScope _scope(id);

    static constexpr int dim = 250;
    ImVec2 meterSize(dim, 30);

    RenderCorrelationMeter("correlation_in", analysis.mPhaseCorrelation, {dim, 30});
    // add tooltip to phase correllation meter
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Phase correlation");
      ImGui::Separator();
      ImGui::Text("Indicates stereo phase correlation:");
      ImGui::BulletText("-1.0: out of phase (180°)");
      ImGui::BulletText(" 0.0: No correlation (random phase)");
      ImGui::BulletText("+1.0: Perfectly in phase (0°)");
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

    // Render the layered visualization
    RenderLayeredStereoVisualization("stereovis", analysis, {dim, dim});
  }

  // Main layered visualization renderer
  void RenderLayeredStereoVisualization(const char* id, const StereoImagingAnalysisStream& analysis, ImVec2 size)
  {
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImRect bb(pos, {pos.x + size.x, pos.y + size.y});

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
};
