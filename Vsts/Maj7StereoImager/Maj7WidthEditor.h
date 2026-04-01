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
#include <WaveSabreVstLib/Width/StereoImagingDisplay.hpp>


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
      VstEditor(audioEffect, 1000, 900)
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

      // Source and polarity stage
      //ImGui::BeginGroup();
      {
        ImGuiGroupScope _grpSources;

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

        ImGui::SameLine();
        {
          ImGuiGroupScope _grpPolarity;
          Maj7ImGuiBoolParamToggleButton((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::LInvert, "L flip", "cc6666");
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::Text("Left Polarity Flip");
            ImGui::Separator();
            ImGui::Text("Invert the polarity of the constructed left channel before width shaping.");
            ImGui::EndTooltip();
          }

          //ImGui::SameLine();
          Maj7ImGuiBoolParamToggleButton((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RInvert, "R flip", "6699cc");
          if (ImGui::IsItemHovered())
          {
            ImGui::BeginTooltip();
            ImGui::Text("Right Polarity Flip");
            ImGui::Separator();
            ImGui::Text("Invert the polarity of the constructed right channel before width shaping.");
            ImGui::EndTooltip();
          }
        }
      }  // _grpSources

      ImGui::Spacing();

      // Width-shaping stage
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
        ImGui::Text("Frequency-dependent width control for the side channel.");
        ImGui::BulletText("Reduces low-frequency width with a 6dB/octave slope.");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();

      Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::MidSideBalance, "Width", 0.0f, 0, {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Width");
        ImGui::Separator();
        ImGui::Text("Adjust the overall stereo width before the rotation stage:");
        ImGui::BulletText("-1.0: Mono (side removed)");
        ImGui::BulletText(" 0.0: Unchanged");
        ImGui::BulletText("+1.0: Side-only (mid removed)");
        ImGui::EndTooltip();
      }
      ImGui::EndGroup();

      ImGui::Spacing();

      // Rotation stage
      ImGui::BeginGroup();
      Maj7ImGuiParamScaledFloat((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::RotationAngle,
                                "L/R rotation",
                                -M7::math::gPIQuarter,
                                M7::math::gPIQuarter,
                                0,
                                0,
                                0,
                                {});
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("L/R Geometric Rotation");
        ImGui::Separator();
        ImGui::Text("Rotate the left/right sample axes directly after width shaping.");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();


      Maj7ImGuiParamFloatN11((VstInt32)WaveSabreCore::Maj7Width::ParamIndices::Pan, "Pan", 0, 0, {});

      ImGui::EndGroup();

      ImGui::Spacing();

      // Final Output Section with Tooltips
      {
        ImGuiGroupScope _grp;

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
      RenderStereoImagingDisplay("stereo_imaging_inp",
                                 mpMaj7Width->mInputImagingAnalysis,
                                 {mShowPolarL, mShowGoniometerPoints, mShowGoniometerLines, mShowPhaseX});
      ImGui::SameLine();
      RenderStereoImagingDisplay("stereo_imaging_outp",
                                 mpMaj7Width->mOutputImagingAnalysis,
                                 {mShowPolarL, mShowGoniometerPoints, mShowGoniometerLines, mShowPhaseX});
    }  // stereo imaging group

    // Toggle buttons for layer visibility
    {
      ButtonArray<4>(
          "width_scope_layers",
          {
              MakeButtonSpec("Lines", &mShowGoniometerLines, kScopeLinesColor, "Show line trails in the stereo scope."),
              MakeButtonSpec(
                  "Points", &mShowGoniometerPoints, kScopePointsColor, "Show point cloud dots in the stereo scope."),
              MakeButtonSpec("Poly", &mShowPolarL, kScopePolyColor, "Show the polygon / polar envelope layer."),
              MakeButtonSpec("Wedge", &mShowPhaseX, kScopeScissorColor, "Show the wedge-style phase view."),
          });
    }
    {
      mStereoHistory.Render(true, mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis);
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
  bool mShowPolarL = false;
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
};
