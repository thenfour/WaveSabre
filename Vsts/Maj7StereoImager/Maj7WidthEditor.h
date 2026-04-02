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

  int mEditingBand = 1;

  bool mShowFftLeft = false;
  bool mShowFftRight = false;
  bool mShowFftStereo = false;
  bool mShowFftMid = false;
  bool mShowFftSide = true;
  bool mShowFftWidth = false;
  bool mShowFftInput = true;
  bool mShowFftOutput = true;

  VstSerializableIntParamRef<int> mEditingBandParam{"EditingBand", mEditingBand};
  VstSerializableBoolParamRef mShowFftLeftParam{"ShowFftLeft", mShowFftLeft};
  VstSerializableBoolParamRef mShowFftRightParam{"ShowFftRight", mShowFftRight};
  VstSerializableBoolParamRef mShowFftStereoParam{"ShowFftStereo", mShowFftStereo};
  VstSerializableBoolParamRef mShowFftMidParam{"ShowFftMid", mShowFftMid};
  VstSerializableBoolParamRef mShowFftSideParam{"ShowFftSide", mShowFftSide};
  VstSerializableBoolParamRef mShowFftWidthParam{"ShowFftWidth", mShowFftWidth};
  VstSerializableBoolParamRef mShowFftInputParam{"ShowFftInput", mShowFftInput};
  VstSerializableBoolParamRef mShowFftOutputParam{"ShowFftOutput", mShowFftOutput};

  const char* kInputLeftFftColor = "5f7088";
  const char* kInputRightFftColor = "866572";
  const char* kInputStereoFftColor = "7c7a61";

  const char* kInputMidFftColor = "664444";
  const char* kInputSideFftColor = "446644";
  const char* kInputWidthFftColor = "444466";

  const char* kOutputLeftFftColor = "4f7ddb";
  const char* kOutputRightFftColor = "cc6b7a";
  const char* kOutputStereoFftColor = bandColors[3];
  const char* kOutputMidFftColor = bandColors[0];
  const char* kOutputSideFftColor = bandColors[1];
  const char* kOutputWidthFftColor = bandColors[2];
  const char* kScopeLinesColor = "66b3ff";
  const char* kScopePointsColor = "ffcc66";
  const char* kScopePolyColor = "66dd88";
  const char* kScopeScissorColor = "cc66ff";


  Maj7WidthEditor(AudioEffect* audioEffect)
      :  //
      VstEditor(audioEffect, 1140, 820)
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
    auto renderContext = BuildRenderContext();

    {
      ImGuiGroupScope _leftColumnGroup("imager_left_column");
      RenderLeftColumn(renderContext);
    }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    ImGui::SameLine(0, 20);
    {
      ImGuiGroupScope _rightColumnGroup("imager_right_column");
      RenderRightColumn();
    }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  }

  virtual std::vector<IVstSerializableParam*> GetVstOnlyParams() override
  {
    return {
        &mEditingBandParam,
        &mShowFftLeftParam,
        &mShowFftRightParam,
        &mShowFftStereoParam,
        &mShowFftMidParam,
        &mShowFftSideParam,
        &mShowFftWidthParam,
        &mShowFftInputParam,
        &mShowFftOutputParam,
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
    };
  }


private:
  struct RenderContext
  {
    bool multibandEnabled = false;
    std::array<bool, WaveSabreCore::Maj7Width::gBandCount> muteSoloEnabled = {true, true, true};
  };

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

  RenderContext BuildRenderContext()
  {
    using Params = WaveSabreCore::Maj7Width::ParamIndices;

    RenderContext context;
    bool mutes[WaveSabreCore::Maj7Width::gBandCount] = {
        mpMaj7Width->mBandConfig[0].mMute,
        mpMaj7Width->mBandConfig[1].mMute,
        mpMaj7Width->mBandConfig[2].mMute,
    };
    bool solos[WaveSabreCore::Maj7Width::gBandCount] = {
        mpMaj7Width->mBandConfig[0].mSolo,
        mpMaj7Width->mBandConfig[1].mSolo,
        mpMaj7Width->mBandConfig[2].mSolo,
    };
    bool muteSoloEnabled[WaveSabreCore::Maj7Width::gBandCount] = {true, true, true};
    Maj7Width::CalculateBandMuteSolo(mutes, solos, muteSoloEnabled);
    for (int i = 0; i < WaveSabreCore::Maj7Width::gBandCount; ++i)
    {
      context.muteSoloEnabled[i] = muteSoloEnabled[i];
    }

    float mbBacking = mpMaj7WidthVst->getParameter((int)Params::MultibandEnable);
    M7::ParamAccessor mbEnableParam{&mbBacking, 0};
    context.multibandEnabled = mbEnableParam.GetBoolValue(0);
    if (!context.multibandEnabled)
    {
      mEditingBand = 1;
    }

    mEditingBand = std::clamp(mEditingBand, 0, WaveSabreCore::Maj7Width::gBandCount - 1);
    return context;
  }

  void RenderLeftColumn(RenderContext& renderContext)
  {
    RenderFftDisplayControlsRow(renderContext);
    ImGui::Spacing();
    RenderFftVisualizationAndBandSelector(renderContext);
    ImGui::Spacing();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    RenderHistorySeriesSelectionButtons();
    ImGui::Spacing();
    RenderHistoryGraph();
    ImGui::Spacing();
#endif
    RenderPerBandControls(renderContext);
    ImGui::Spacing();
    RenderGlobalControls();
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  void RenderRightColumn()
  {
    RenderStereoImageVisualizer();
    ImGui::Spacing();
    RenderVuMeterBank();
  }
#endif

  void RenderFftDisplayControlsRow(RenderContext& renderContext)
  {
    using Params = WaveSabreCore::Maj7Width::ParamIndices;
    using CrossoverSlope = M7::CrossoverSlope;

    ImGuiGroupScope _row("fft_display_controls_row");
    CROSSOVER_SLOPE_CAPTIONS(crossoverSlopeCaptions);

    float mbBacking = mpMaj7WidthVst->getParameter((int)Params::MultibandEnable);
    M7::ParamAccessor mbEnableParam{&mbBacking, 0};
    bool singleBand = !renderContext.multibandEnabled;
    if (ToggleButton(&singleBand, "Single-band", {90, 20}))
    {
      mbEnableParam.SetBoolValue(0, false);
      mpMaj7WidthVst->setParameter((int)Params::MultibandEnable, mbBacking);
      renderContext.multibandEnabled = false;
      mEditingBand = 1;
    }
    ImGui::SameLine();
    if (ToggleButton(&renderContext.multibandEnabled, "Multi-band", {90, 20}))
    {
      mbEnableParam.SetBoolValue(0, true);
      mpMaj7WidthVst->setParameter((int)Params::MultibandEnable, mbBacking);
    }

    if (renderContext.multibandEnabled)
    {
      ImGui::SameLine(0, 20);
      Maj7ImGuiParamEnumToggleButtonArray<CrossoverSlope>(
          Params::CrossoverASlope,
          "Slope 1",
          {
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_12dB],
                                        CrossoverSlope::Slope_12dB,
                                        "8d6e63"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_24dB],
                                        CrossoverSlope::Slope_24dB,
                                        "8d6e63"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_36dB],
                                        CrossoverSlope::Slope_36dB,
                                        "8d6e63"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_48dB],
                                        CrossoverSlope::Slope_48dB,
                                        "8d6e63"},
          });
      if (ImGui::IsItemHovered())
      {
        ImGui::BeginTooltip();
        ImGui::Text("Crossover 1 Slope");
        ImGui::Separator();
        ImGui::Text("Linkwitz-Riley slope used at the first split point.");
        ImGui::EndTooltip();
      }

      ImGui::SameLine();
      Maj7ImGuiParamEnumToggleButtonArray<CrossoverSlope>(
          Params::CrossoverBSlope,
          "Slope 2",
          {
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_12dB],
                                        CrossoverSlope::Slope_12dB,
                                        "3f7a93"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_24dB],
                                        CrossoverSlope::Slope_24dB,
                                        "3f7a93"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_36dB],
                                        CrossoverSlope::Slope_36dB,
                                        "3f7a93"},
              EnumToggleButtonArrayItem{crossoverSlopeCaptions[(int)CrossoverSlope::Slope_48dB],
                                        CrossoverSlope::Slope_48dB,
                                        "3f7a93"},
          });
    }

                      ImGui::Spacing();
                      ButtonArray<6>(
                        "width_fft_series",
                        {
                          MakeButtonSpec("L", &mShowFftLeft, kOutputLeftFftColor, "Show the left-channel FFT overlay."),
                          MakeButtonSpec("R", &mShowFftRight, kOutputRightFftColor, "Show the right-channel FFT overlay."),
                          MakeButtonSpec("Stereo", &mShowFftStereo, kOutputStereoFftColor, "Show the combined stereo FFT overlay."),
                          MakeButtonSpec("Mid", &mShowFftMid, kOutputMidFftColor, "Show the mid-channel FFT overlay."),
                          MakeButtonSpec("Side", &mShowFftSide, kOutputSideFftColor, "Show the side-channel FFT overlay."),
                          MakeButtonSpec("Width", &mShowFftWidth, kOutputWidthFftColor, "Show the width FFT overlay."),
                        });

                      ImGui::SameLine(0, 40);
                      ButtonArray<2>(
                        "width_fft_groups",
                        {
                          MakeButtonSpec("Input", &mShowFftInput, kInputLeftFftColor, "Show enabled input FFT overlays."),
                          MakeButtonSpec("Output", &mShowFftOutput, kOutputLeftFftColor, "Show enabled output FFT overlays."),
                        });
  }

  void RenderFftVisualizationAndBandSelector(const RenderContext& renderContext)
  {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    ImGuiGroupScope _grp("fft_and_band_selector");
    EnsureFrequencyAnalysisEnabled();
    RenderFrequencyAnalysis(renderContext.multibandEnabled, renderContext.muteSoloEnabled);
    if (renderContext.multibandEnabled)
    {
      ImGui::TextDisabled("Drag crossover lines, click a band to edit it, or use M/S buttons on the graph.");
    }
#else
    (void)renderContext;
#endif
  }

  void RenderHistorySeriesSelectionButtons()
  {
    ImGuiGroupScope _grp("history_series_selection_buttons");
    mStereoHistory.RenderSeriesSelectionButtons();
  }

  void RenderHistoryGraph()
  {
    ImGuiGroupScope _grp("history_graph");
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    mStereoHistory.RenderGraph(mpMaj7Width->mInputImagingAnalysis, mpMaj7Width->mOutputImagingAnalysis);
#endif
  }

  void RenderPerBandControls(const RenderContext& renderContext)
  {
    ImGuiGroupScope _grp("per_band_controls");
    if (!renderContext.multibandEnabled)
    {
      RenderBandControlSection(1, false);
      return;
    }

    ImGui::Spacing();

    for (int bandIndex = 0; bandIndex < WaveSabreCore::Maj7Width::gBandCount; ++bandIndex)
    {
      RenderBandControlSection(bandIndex, true);
      if (bandIndex + 1 < WaveSabreCore::Maj7Width::gBandCount)
      {
        ImGui::SameLine(0, 14.0f);
      }
    }
  }

  void RenderGlobalControls()
  {
    using Params = WaveSabreCore::Maj7Width::ParamIndices;

    ImGuiGroupScope _grp("global_controls");

    ImGui::Separator();

    Maj7ImGuiParamFrequency((VstInt32)Params::SideHPFrequency, -1, "Side HPF", M7::gFilterFreqConfig, 0, {});
    // display a tooltip for the side HPF control
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Side High-pass Filter");
      ImGui::Separator();
      ImGui::Text(
          "High-pass filter applied to the side signal. Can help reduce muddiness when boosting width, especially on "
          "bass-heavy material.");
      ImGui::EndTooltip();
    }

    ImGui::SameLine();
    Maj7ImGuiBoolParamToggleButton((VstInt32)Params::LInvert, "L flip", "cc6666");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Left Channel polarity invert (utility)");
      ImGui::Separator();
      ImGui::Text("Inverts the left channel before processing.");
      ImGui::EndTooltip();
    }

    ImGui::SameLine();
    Maj7ImGuiBoolParamToggleButton((VstInt32)Params::RInvert, "R flip", "6699cc");
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Right Channel polarity invert (utility)");
      ImGui::Separator();
      ImGui::Text("Inverts the right channel before processing.");
      ImGui::EndTooltip();
    }

    ImGui::SameLine(0, 24);
    Maj7ImGuiParamVolume((VstInt32)Params::OutputGain, "Output", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
    if (ImGui::IsItemHovered())
    {
      ImGui::BeginTooltip();
      ImGui::Text("Overall output gain.");
      ImGui::EndTooltip();
    }
  }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  void EnsureFrequencyAnalysisEnabled()
  {
    if (!mpMaj7Width->mInputImagingAnalysis.IsFrequencyAnalysisEnabled())
    {
      mpMaj7Width->mInputImagingAnalysis.SetFrequencyAnalysisEnabled(true);
      mpMaj7Width->mOutputImagingAnalysis.SetFrequencyAnalysisEnabled(true);
    }
  }

  StereoImagingDisplayStyle MakeStereoImagingStyle() const
  {
    StereoImagingDisplayStyle stereoImagingStyle{};
    stereoImagingStyle.primarySeries.label = "Input";
    stereoImagingStyle.primarySeries.colors = {ColorFromHTML("#8FA6B3"),
                                               ColorFromHTML("#B6AA84"),
                                               ColorFromHTML("#B79298")};
    stereoImagingStyle.secondarySeries.label = "Output";
    stereoImagingStyle.secondarySeries.colors = {ColorFromHTML("#1FE0A8"),
                                                 ColorFromHTML("#FFD24D"),
                                                 ColorFromHTML("#FF623D")};
    return stereoImagingStyle;
  }

  VUMeterConfig MakeMainVUMeterConfig() const
  {
    static const std::vector<VUMeterTick> tickSet = {
        {-3.0f, "3db"},
        {-6.0f, "6db"},
        {-12.0f, "12db"},
        {-18.0f, "18db"},
        {-24.0f, "24db"},
        {-30.0f, "30db"},
        {-40.0f, "40db"},
    };

    return {
        {24, 300},
        VUMeterLevelMode::Audio,
        VUMeterUnits::Linear,
        -50,
        6,
        tickSet,
    };
  }

  void RenderStereoImageVisualizer()
  {
    ImGuiGroupScope _grp("stereo_image_visualizer");
    RenderStereoImagingDisplay("stereo_imaging_io",
                               mpMaj7Width->mInputImagingAnalysis,
                               &mpMaj7Width->mOutputImagingAnalysis,
                               {mShowPolarL, mShowGoniometerPoints, mShowGoniometerLines, mShowPhaseX},
                               300,
                               MakeStereoImagingStyle());

    ImGui::Spacing();
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

  void RenderVuMeterBank()
  {
    ImGuiGroupScope _grp("vu_meter_bank");
    const auto mainCfg = MakeMainVUMeterConfig();
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
#endif

  static const char* GetBandLabel(int bandIndex)
  {
    switch (bandIndex)
    {
      case 0:
        return "Low Band";
      case 1:
        return "Mid Band";
      case 2:
        return "High Band";
      default:
        return "Band";
    }
  }

  VstInt32 GetBandParamIndex(int bandIndex, WaveSabreCore::Maj7Width::FreqBand::BandParam bandParam) const
  {
    using Params = WaveSabreCore::Maj7Width::ParamIndices;
    using BandParam = WaveSabreCore::Maj7Width::FreqBand::BandParam;
    return (VstInt32)((int)Params::AWidth + bandIndex * (int)BandParam::Count__ + (int)bandParam);
  }

  ImRect GetBandButtonArea(const ImRect& bandRect) const
  {
    const float paddingY = 10.0f;
    const float paddingX = 3.0f;
    const float buttonWidth = 22.0f;
    const float buttonHeight = 16.0f;
    const float spacing = 1.5f;
    const float totalWidth = buttonWidth * 2 + spacing + paddingX * 2;
    const float startX = (bandRect.Min.x + bandRect.Max.x) * 0.5f - totalWidth * 0.5f;
    return ImRect(startX,
                  bandRect.Min.y + paddingY,
                  startX + totalWidth,
                  bandRect.Min.y + paddingY + buttonHeight + paddingX * 2);
  }

  bool HandleBandClick(int bandIndex, const ImRect& bandRect, bool multibandEnabled)
  {
    if (!multibandEnabled || bandIndex < 0 || bandIndex >= WaveSabreCore::Maj7Width::gBandCount)
    {
      return false;
    }

    const ImRect buttonArea = GetBandButtonArea(bandRect);
    if (bandRect.GetWidth() < 70.0f || !buttonArea.Contains(ImGui::GetIO().MousePos))
    {
      return false;
    }

    auto& bandConfig = mpMaj7Width->mBandConfig[bandIndex];
    const float buttonWidth = 22.0f;
    const float buttonHeight = 16.0f;
    const float spacing = 1.5f;
    const ImVec2 mutePos{buttonArea.Min.x + 3.0f, buttonArea.Min.y + 3.0f};
    const ImVec2 soloPos{mutePos.x + buttonWidth + spacing, mutePos.y};
    const ImRect muteRect(mutePos, {mutePos.x + buttonWidth, mutePos.y + buttonHeight});
    const ImRect soloRect(soloPos, {soloPos.x + buttonWidth, soloPos.y + buttonHeight});
    const ImVec2 mousePos = ImGui::GetIO().MousePos;

    if (muteRect.Contains(mousePos))
    {
      bandConfig.mMute = !bandConfig.mMute;
      return true;
    }
    if (soloRect.Contains(mousePos))
    {
      bandConfig.mSolo = !bandConfig.mSolo;
      return true;
    }
    return false;
  }

  bool RenderBandOverlay(int bandIndex,
                         const ImRect& bandRect,
                         bool isHovered,
                         bool isSelected,
                         ImDrawList* drawList,
                         bool muteSoloEnabled,
                         bool multibandEnabled)
  {
    if (!multibandEnabled || bandIndex < 0 || bandIndex >= WaveSabreCore::Maj7Width::gBandCount)
    {
      return false;
    }

    if (isSelected)
    {
      drawList->AddRect(bandRect.Min, bandRect.Max, ColorFromHTML(bandColors[bandIndex], 0.8f));
      drawList->AddRectFilled(bandRect.Min,
                              {bandRect.Max.x, bandRect.Min.y + 4.0f},
                              ColorFromHTML(bandColors[bandIndex], 0.85f),
                              2.0f);
    }

    const ImRect buttonArea = GetBandButtonArea(bandRect);
    if (bandRect.GetWidth() < 70.0f)
    {
      return false;
    }

    auto& bandConfig = mpMaj7Width->mBandConfig[bandIndex];
    const float buttonWidth = 22.0f;
    const float buttonHeight = 16.0f;
    const float spacing = 1.5f;
    const ImVec2 mutePos{buttonArea.Min.x + 3.0f, buttonArea.Min.y + 3.0f};
    const ImVec2 soloPos{mutePos.x + buttonWidth + spacing, mutePos.y};
    const ImRect muteRect(mutePos, {mutePos.x + buttonWidth, mutePos.y + buttonHeight});
    const ImRect soloRect(soloPos, {soloPos.x + buttonWidth, soloPos.y + buttonHeight});
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    const bool muteHovered = muteRect.Contains(mousePos);
    const bool soloHovered = soloRect.Contains(mousePos);
    const bool anyButtonHovered = muteHovered || soloHovered;

    const bool effectivelyAudible = muteSoloEnabled && !bandConfig.mMute;
    const ImColor muteColor = bandConfig.mMute ? ColorFromHTML("cc4444", 0.95f)
                                               : ColorFromHTML("444444", muteHovered ? 0.85f : 0.65f);
    const ImColor soloColor = bandConfig.mSolo ? ColorFromHTML("cccc44", 0.95f)
                                               : ColorFromHTML("444444", soloHovered ? 0.85f : 0.65f);
    const ImColor buttonBorder = ColorFromHTML(effectivelyAudible ? bandColors[bandIndex] : "666666", 0.8f);

    drawList->AddRectFilled(muteRect.Min, muteRect.Max, muteColor, 2.0f);
    drawList->AddRect(muteRect.Min, muteRect.Max, buttonBorder, 2.0f, 0, 1.0f);
    drawList->AddText({muteRect.Min.x + 7.0f, muteRect.Min.y + 1.0f}, ColorFromHTML("ffffff"), "M");

    drawList->AddRectFilled(soloRect.Min, soloRect.Max, soloColor, 2.0f);
    drawList->AddRect(soloRect.Min, soloRect.Max, buttonBorder, 2.0f, 0, 1.0f);
    drawList->AddText({soloRect.Min.x + 7.0f, soloRect.Min.y + 1.0f},
                      ColorFromHTML(bandConfig.mSolo ? "000000" : "ffffff"),
                      "S");

    if (anyButtonHovered)
    {
      ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    }

    return anyButtonHovered || isHovered || isSelected;
  }

  void ShowBandControlTooltip(const char* title, const char* description)
  {
    if (!ImGui::IsItemHovered())
    {
      return;
    }

    ImGui::BeginTooltip();
    ImGui::TextUnformatted(title);
    ImGui::Separator();
    ImGui::PushTextWrapPos(ImGui::GetFontSize() * 24.0f);
    ImGui::TextWrapped("%s", description);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
  }

  void RenderBandTabHeader(int bandIndex)
  {
    const bool isSelected = mEditingBand == bandIndex;
    const auto label = GetBandLabel(bandIndex);
    ImGui::PushStyleColor(ImGuiCol_Text,
                          (isSelected ? ColorFromHTML("000000") : ColorFromHTML("cccccc")).operator ImVec4());
    bool tabSelected = isSelected;
    if (ToggleButton(&tabSelected,
                     label,
                     {250.0f, 22.0f},
                     ButtonColorSpec{ColorFromHTML(bandColors[bandIndex], 0.8f),
                                     ColorFromHTML(bandColors[bandIndex], 0.12f),
                                     ColorFromHTML(bandColors[bandIndex], 0.5f)}))
    {
      mEditingBand = bandIndex;
    }
    ImGui::PopStyleColor();
  }

  void RenderBandControlSection(int bandIndex, bool multibandEnabled)
  {
    using BandParam = WaveSabreCore::Maj7Width::FreqBand::BandParam;

    ImGui::PushID(multibandEnabled ? bandIndex : -1);
    ImGui::BeginGroup();
    if (multibandEnabled)
    {
      RenderBandTabHeader(bandIndex);
      ImGui::Spacing();
    }
    else
    {
      ImGui::TextUnformatted("Broadband (Uses Mid Band Controls)");
      ImGui::TextDisabled("Switch to multi-band mode to edit the low and high bands separately.");
    }

    auto knobToken = KnobColorMod{bandColors[bandIndex], !multibandEnabled || mEditingBand == bandIndex}.Push();

    Maj7ImGuiParamFloatN11(GetBandParamIndex(bandIndex, BandParam::Width), "Width", 0.0f, 0, {});
    ShowBandControlTooltip("Width", "Scales stereo spread without moving the image center");
    ImGui::SameLine();
    Maj7ImGuiParamFloatN11(GetBandParamIndex(bandIndex, BandParam::Pan), "Pan", 0.0f, 0, {});
    ShowBandControlTooltip("Pan", "Scales L/R channels to rebalance the stereo image; does not mix channels");

    Maj7ImGuiParamFloatN11(GetBandParamIndex(bandIndex, BandParam::Asymmetry), "Asymmetry", 0.0f, 0, {});
    ShowBandControlTooltip("Asymmetry",
                           "Scales L/R to lean the center of the stereo image towards one channel or the other.");
    ImGui::SameLine();
    Maj7ImGuiParamVolume(
        GetBandParamIndex(bandIndex, BandParam::SideGain), "Side Gain", WaveSabreCore::Maj7Width::gVolumeCfg, 0, {});
    ShowBandControlTooltip(
        "Side Gain",
        multibandEnabled
            ? "Changes the gain of the side component. In multiband mode this also works as a frequency-selective "
              "shuffler."
            : "Changes the gain of the side component. In broadband mode this is similar in effect to 'Width'.");
    ImGui::SameLine();
    Maj7ImGuiParamScaledFloat(GetBandParamIndex(bandIndex, BandParam::Rotation),
                              "Rotation",
                              -WaveSabreCore::Maj7Width::gRotationExtent,
                              WaveSabreCore::Maj7Width::gRotationExtent,
                              0,
                              0,
                              0,
                              {});
    ShowBandControlTooltip("Rotation",
                           "Rotates the stereo image geometrically. Best used for small adjustments / corrections.");

    ImGui::EndGroup();
    ImGui::PopID();
  }

  // Render frequency analysis graph
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  void RenderFrequencyAnalysis(bool multibandEnabled,
                               const std::array<bool, WaveSabreCore::Maj7Width::gBandCount>& muteSoloEnabled)
  {
    const auto* analyzerIn = mpMaj7Width->mInputImagingAnalysis.GetFrequencyAnalyzer();
    const auto* analyzerOut = mpMaj7Width->mOutputImagingAnalysis.GetFrequencyAnalyzer();

    if (!analyzerIn || !analyzerOut)
    {
      ImGui::Text("Frequency analysis not available");
      return;
    }

    static constexpr float scaleMinDb = -90.0f;
    const float scaleMin = scaleMinDb;
    const float scaleMax = 0.0f;

    FrequencyResponseRendererConfig<0, (size_t)WaveSabreCore::Maj7Width::ParamIndices::NumParams> cfg{
        ColorFromHTML("222222", 1.0f),
        ColorFromHTML("ff00ff", 1.0f),
        0.0f,
        {},
        {},
        {},
        {},
        {},
        scaleMin,
        scaleMax,
        true};
    for (size_t i = 0; i < (size_t)WaveSabreCore::Maj7Width::ParamIndices::NumParams; ++i)
    {
      cfg.mParamCacheCopy[i] = mpMaj7Width->mParamCache[i];
    }

    cfg.fftOverlays.clear();

    auto widthAsDbScale = [](float v) -> float
    {
      if (v <= 0.0f)
      {
        return v;
      }
      const float a = 1.8f;
      float t01 = std::log1p(a * std::max(0.0f, v)) / std::log1p(a * 3.0f);
      return M7::math::lerp(scaleMinDb, 0.0f, t01);
    };

    const auto widthTransform = std::function<float(float)>(widthAsDbScale);
    auto addOverlay = [&cfg](bool showSignalPath,
                             bool showSeries,
                             const WaveSabreCore::IFrequencyAnalysis* spectrum,
                             const char* colorHtml,
                             const char* label,
                             const std::function<float(float)>& valueTransform = {})
    {
      if (!showSignalPath || !showSeries || !spectrum)
      {
        return;
      }

      cfg.fftOverlays.push_back(
          {spectrum,
           ColorFromHTML(colorHtml, 0.9f),
           ColorFromHTML(colorHtml, 0.25f),
           true,
           label,
           valueTransform});
    };

    addOverlay(mShowFftInput, mShowFftLeft, analyzerIn->GetLeftAnalyzer(), kInputLeftFftColor, "Input L");
    addOverlay(mShowFftInput, mShowFftRight, analyzerIn->GetRightAnalyzer(), kInputRightFftColor, "Input R");
    addOverlay(mShowFftInput, mShowFftStereo, analyzerIn->GetStereoAnalyzer(), kInputStereoFftColor, "Input Stereo");
    addOverlay(mShowFftInput, mShowFftMid, analyzerIn->GetMidAnalyzer(), kInputMidFftColor, "Input Mid");
    addOverlay(mShowFftInput, mShowFftSide, analyzerIn->GetSideAnalyzer(), kInputSideFftColor, "Input Side");
    addOverlay(mShowFftInput, mShowFftWidth, analyzerIn, kInputWidthFftColor, "Input Width", widthTransform);

    addOverlay(mShowFftOutput, mShowFftLeft, analyzerOut->GetLeftAnalyzer(), kOutputLeftFftColor, "Output L");
    addOverlay(mShowFftOutput, mShowFftRight, analyzerOut->GetRightAnalyzer(), kOutputRightFftColor, "Output R");
    addOverlay(
        mShowFftOutput, mShowFftStereo, analyzerOut->GetStereoAnalyzer(), kOutputStereoFftColor, "Output Stereo");
    addOverlay(mShowFftOutput, mShowFftMid, analyzerOut->GetMidAnalyzer(), kOutputMidFftColor, "Output Mid");
    addOverlay(mShowFftOutput, mShowFftSide, analyzerOut->GetSideAnalyzer(), kOutputSideFftColor, "Output Side");
    addOverlay(mShowFftOutput, mShowFftWidth, analyzerOut, kOutputWidthFftColor, "Output Width", widthTransform);

    const bool anyWidthVisible = mShowFftWidth && (mShowFftInput || mShowFftOutput);
    const bool anyDbVisible = (mShowFftLeft || mShowFftRight || mShowFftStereo || mShowFftMid || mShowFftSide) &&
                              (mShowFftInput || mShowFftOutput);

    mWidthGraph.ClearFFTDiffOverlay();
    mWidthGraph.ClearFFTDiffFlatOverlay();
    mWidthGraph.SetFFTScaleCaption(anyWidthVisible ? (anyDbVisible ? "dB / Width" : "Width") : "dB");
    if (multibandEnabled)
    {
      mWidthGraph.SetCrossoverDataSource(
          [this](int crossoverIndex) -> float
          {
            using Params = WaveSabreCore::Maj7Width::ParamIndices;
            const auto paramIndex = crossoverIndex == 0 ? Params::CrossoverAFrequency : Params::CrossoverBFrequency;
            float raw = mpMaj7Width->mParamCache[(int)paramIndex];
            M7::ParamAccessor param{&raw, 0};
            return param.GetFrequency(0, M7::gFilterFreqConfig);
          },
          [this](float freqHz) -> std::array<float, 3>
          {
            auto mags = mpMaj7Width->mMidSplitter.GetMagnitudesAtFrequency(freqHz);
            return {mags[0], mags[1], mags[2]};
          });
    }
    else
    {
      mWidthGraph.SetCrossoverDataSource(nullptr, nullptr);
    }

    mWidthGraph.mCrossoverLayer->mShowResponses = multibandEnabled;
    mWidthGraph.mCrossoverLayer->mGetBandColor = [multibandEnabled, muteSoloEnabled](size_t bandIndex,
                                                                                     bool hovered) -> ImColor
    {
      if (bandIndex >= WaveSabreCore::Maj7Width::gBandCount)
      {
        return ColorFromHTML("888888", 0.5f);
      }

      const bool active = multibandEnabled ? muteSoloEnabled[bandIndex] : bandIndex == 1;
      const char* baseColor = active ? bandColors[bandIndex] : "444444";
      return ColorFromHTML(baseColor, hovered ? 0.95f : (active ? 0.75f : 0.55f));
    };
    mWidthGraph.SetCurrentEditingBand(multibandEnabled ? mEditingBand : 1);

    auto bandRenderer = [this, multibandEnabled, muteSoloEnabled](int bandIndex,
                                                                  const ImRect& bandRect,
                                                                  bool isHovered,
                                                                  bool isSelected,
                                                                  ImDrawList* drawList) -> bool
    {
      if (!multibandEnabled || bandIndex < 0 || bandIndex >= WaveSabreCore::Maj7Width::gBandCount)
      {
        return false;
      }

      if (drawList == nullptr)
      {
        return GetBandButtonArea(bandRect).Contains(ImGui::GetIO().MousePos);
      }
      if (drawList == reinterpret_cast<ImDrawList*>(1))
      {
        return HandleBandClick(bandIndex, bandRect, multibandEnabled);
      }
      return RenderBandOverlay(
          bandIndex, bandRect, isHovered, isSelected, drawList, muteSoloEnabled[bandIndex], multibandEnabled);
    };
    mWidthGraph.SetBandRenderer(bandRenderer);

    if (multibandEnabled)
    {
      mWidthGraph.SetFrequencyChangeHandler(
          [this](float freqHz, int crossoverIndex)
          {
            M7::QuickParam freqParam;
            freqParam.SetFrequencyAssumingNoKeytracking(M7::gFilterFreqConfig, freqHz);
            const VstInt32 paramIndex = crossoverIndex == 0
                                            ? (VstInt32)WaveSabreCore::Maj7Width::ParamIndices::CrossoverAFrequency
                                            : (VstInt32)WaveSabreCore::Maj7Width::ParamIndices::CrossoverBFrequency;
            mpMaj7WidthVst->setParameterAutomated(paramIndex, M7::math::clamp01(freqParam.GetRawValue()));
          });
      mWidthGraph.SetBandChangeHandler(
          [this](int bandIndex)
          {
            if (bandIndex >= 0 && bandIndex < WaveSabreCore::Maj7Width::gBandCount)
            {
              mEditingBand = bandIndex;
            }
          });
    }
    else
    {
      mWidthGraph.SetFrequencyChangeHandler(nullptr);
      mWidthGraph.SetBandChangeHandler(nullptr);
    }

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

    void RenderGraph(const StereoImagingAnalysisStream& inputAnalysis,
                     const StereoImagingAnalysisStream& outputAnalysis)
    {
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
    }

    void RenderSeriesSelectionButtons()
    {
      ButtonArray<5>(
          "width_history_metrics",
          {
              MakeButtonSpec(
                  "Correllation", &mShowPhaseCorrelation, kInputCorrellationColor, "Show stereo correlation history."),
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

    void Render(bool showToggles,
                const StereoImagingAnalysisStream& inputAnalysis,
                const StereoImagingAnalysisStream& outputAnalysis)
    {
      ImGui::BeginGroup();
      if (showToggles)
      {
        RenderSeriesSelectionButtons();
        ImGui::Spacing();
      }
      RenderGraph(inputAnalysis, outputAnalysis);
      ImGui::EndGroup();
    }
  };

  StereoHistoryVis<800, 120> mStereoHistory;
};
