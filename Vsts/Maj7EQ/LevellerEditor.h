#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "LevellerVst.h"
#include <WaveSabreVstLib/FreqMagnitudeGraph/FrequencyResponseRendererLayered.hpp>


///////////////////////////////////////////////////////////////////////////////////////////////////
class LevellerEditor : public VstEditor
{
  using FFTSize = MonoFFTAnalysis::FFTSize;
  using WindowType = MonoFFTAnalysis::WindowType;
  Leveller* mpLeveller;
  LevellerVst* mpLevellerVST;

  ColorMod mEnabledColors{0, 1, 1, 0.9f, 0.0f};
  ColorMod mDisabledColors{0, .15f, .6f, 0.5f, 0.2f};

  FrequencyResponseRendererLayered<780, 400, Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams, true>
      mResponseGraph;
  const ImVec2 kvuMeterSize{20, 400};

  // FFT Analysis controls (not VST parameters, just UI state)
  //float mFFTSmoothingFactor;// = 0.0f;      // 0.0-0.9
  //int mFFTOverlapFactor;// = 2;             // 2, 4, 8, or 16 (more options for testing)
  //float mSpectrumPeakHoldTime;// = 300.0f;    // 0-1000 ms
  //float mSpectrumFalloffTime;//= 2000.0f;   // 50-5000 ms
  //FFTSize mFFTSizeSelection;// = 1;             // 0=512, 1=1024, 2=2048, 3=4096
  //WindowType mFFTWindowType;// = 1;                // 0=Rectangular, 1=Hanning, 2=Hamming, 3=Blackman
  float mFFTDisplayMinDB = -80.0f;  // Noise floor
  float mFFTDisplayMaxDB = 0.0f;    // Digital maximum

public:
  LevellerEditor(AudioEffect* audioEffect)
      : VstEditor(audioEffect, 1050, 730)
      , mpLevellerVST((LevellerVst*)audioEffect)  //,
  {
    mpLeveller =
        (Leveller*)mpLevellerVST
            ->getDevice();  // for some reason this doesn't work as initialization but has to be called in ctor body like this.

    // Initialize FFT control values to match current Leveller settings
    //mFFTSmoothingFactor = mpLeveller->mInputSpectrumSmoother.GetFFTSmoothing();
    //mFFTOverlapFactor = mpLeveller->mInputSpectrumSmoother.GetOverlapFactor();
    //mSpectrumPeakHoldTime = mpLeveller->mInputSpectrumSmoother.GetPeakHoldTime();
    //mSpectrumFalloffTime = mpLeveller->mInputSpectrumSmoother.GetFalloffTime();
    //mFFTSizeSelection = mpLeveller->mInputSpectrumSmoother.GetFFTSize();
    //mFFTWindowType = mpLeveller->mInputSpectrumSmoother.GetWindowType();
  }

  virtual void PopulateMenuBar() override
  {
    LEVELLER_PARAM_VST_NAMES(paramNames);
    PopulateStandardMenuBar(mCurrentWindow,
                            "Maj7 EQ",
                            mpLeveller,
                            mpLevellerVST,
                            "gLevellerDefaults16",
                            "ParamIndices::NumParams",
                            mpLeveller->mParamCache,
                            paramNames);
  }

  void RenderBand(int id,
                  const char* label,
                  Leveller::ParamIndices paramOffset,
                  float defaultFreqParamHz,
                  const char* colorRaw)
  {
    ImGuiGroupScope __group{id};
    //ImGui::PushID(id);

    //ImColor color = ColorFromHTML(colorRaw, 0.8f);

    M7::QuickParam enabledParam{GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Enable)};
    KnobColorMod colorMod{
        enabledParam.GetBoolValue() ? "222222" : "111111",        // inactiveTrackColor
        enabledParam.GetBoolValue() ? bandColors[id] : "444444",  // activeTrackColor
        enabledParam.GetBoolValue() ? bandColors[id] : "666666",  // thumbColor
        enabledParam.GetBoolValue() ? "ffffff" : "666666"         // textColor
    };

    auto knobToken = colorMod.Push();

    //ColorMod& cm = enabledParam.GetBoolValue() ? mEnabledColors : mDisabledColors;
    //auto token = cm.Push();

    {
      ImGui::PushStyleColor(ImGuiCol_Text, ColorFromHTML("000000").operator ImVec4());
      bool boolTemp = enabledParam.GetBoolValue();
      if (ToggleButton(&boolTemp,
                       label,
                       {150, 20},
                       ButtonColorSpec{
                           ColorFromHTML(colorRaw, 0.8f),  // sel
                           ColorFromHTML(colorRaw, 0.1f),  // not selected
                           ColorFromHTML(colorRaw, 0.5f),  // hover
                       }))
      {
        enabledParam.SetBoolValue(boolTemp);
        GetEffectX()->setParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Enable,
                                   enabledParam.GetRawValue());
      }
      ImGui::PopStyleColor();

    }
    {
      const int filterCircuitParamID = (int)paramOffset + (int)Leveller::BandParamOffsets::Circuit;
      const int filterSlopeParamID = (int)paramOffset + (int)Leveller::BandParamOffsets::Slope;
      const int filterResponseParamID = (int)paramOffset + (int)Leveller::BandParamOffsets::Response;

      M7::QuickParam filterCircuitParam{GetEffectX()->getParameter(filterCircuitParamID)};
      M7::QuickParam filterSlopeParam{GetEffectX()->getParameter(filterSlopeParamID)};
      M7::QuickParam filterResponseParam{GetEffectX()->getParameter(filterResponseParamID)};

      M7::FilterCircuit selectedCircuit = filterCircuitParam.GetEnumValue<M7::FilterCircuit>();
      M7::FilterSlope selectedSlope = filterSlopeParam.GetEnumValue<M7::FilterSlope>();
      M7::FilterResponse selectedResponse = filterResponseParam.GetEnumValue<M7::FilterResponse>();
      float selectedCutoffRaw = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Freq);
      M7::ParamAccessor selectedCutoffPA{&selectedCutoffRaw, 0};
      const float selectedCutoffHz = selectedCutoffPA.GetFrequency(0, M7::gFilterFreqConfig);
      const float selectedReso01 = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Q);

      auto applySelection = [&](M7::FilterCircuit circuit, M7::FilterSlope slope, M7::FilterResponse response)
      {
        M7::QuickParam qp{};
        GetEffectX()->setParameter(filterCircuitParamID, qp.SetEnumValue(circuit));
        GetEffectX()->setParameter(filterSlopeParamID, qp.SetEnumValue(slope));
        GetEffectX()->setParameter(filterResponseParamID, qp.SetEnumValue(response));
      };

      RenderFilterSelectionWidget(
          "LevellerFilterSelector",
          id + 100,
          selectedCircuit,
          selectedSlope,
          selectedResponse,
          WaveSabreCore::M7::DoesFilterSupport,
          applySelection,
          [&](const ImRect& bb)
          {
            auto* dl = ImGui::GetWindowDrawList();
            dl->AddRect(bb.Min, bb.Max, ColorFromHTML("2a2a2a"), 3.0f, 0, 1.5f);
            dl->AddText(ImVec2(bb.Min.x + 6, bb.Min.y + 3), ColorFromHTML("cccccc"), LabelForFilterCircuit(selectedCircuit));
          });
    }

    ImGui::Spacing();

    M7::QuickParam fp{M7::gFilterFreqConfig};
    fp.SetFrequencyAssumingNoKeytracking(defaultFreqParamHz);


    Maj7ImGuiParamFrequency((int)paramOffset + (int)Leveller::BandParamOffsets::Freq,
                            -1,
                            "Freq",
                            M7::gFilterFreqConfig,
                            fp.GetRawValue(),
                            {});

    float typeB = GetEffectX()->getParameter((int)paramOffset + (int)Leveller::BandParamOffsets::Response);
    M7::ParamAccessor typePA{&typeB, 0};
    M7::FilterResponse type = typePA.GetEnumValue<M7::FilterResponse>(0);
    ImGui::BeginDisabled(type == M7::FilterResponse::Highpass || type == M7::FilterResponse::Lowpass ||
                         type == M7::FilterResponse::Allpass || type == M7::FilterResponse::Notch);
    ImGui::SameLine();
    Maj7ImGuiParamScaledFloat(
        (int)paramOffset + (int)Leveller::BandParamOffsets::Gain, "Gain(db)", -30.0f, 30.0f, 0, 0, 0, {});
    ImGui::EndDisabled();
    ImGui::SameLine();
    Maj7ImGuiParamFloat01((int)paramOffset + (int)Leveller::BandParamOffsets::Q, "Reso", 0.5f, 0);

    //ImGui::PopID();
  }


  virtual void renderImgui() override
  {
    mEnabledColors.EnsureInitialized();
    mDisabledColors.EnsureInitialized();

    // Capture bandIndex in individual lambdas for each band
    auto makeBandHandler = [this](uintptr_t bandIndex)
    {
      return [this, bandIndex](float freqHz, M7::Decibels gainDb, uintptr_t userData)
      {
        // Convert freqHz to the param value
        M7::QuickParam freqParam;
        freqParam.SetFrequencyAssumingNoKeytracking(M7::gFilterFreqConfig, freqHz);
        float freqParamValue = freqParam.GetRawValue();

        // Convert gainDb to the param value using scaled real value (-30 to +30 dB)
        M7::QuickParam gainParam;
        gainParam.SetScaledValue(M7::gEqBandGainMin, M7::gEqBandGainMax, gainDb.value);
        float gainParamValue = gainParam.GetRawValue();

        // Calculate the parameter indices based on band index
        // Each band has 5 parameters: Type, Freq, Gain, Q, Enable
        VstInt32 freqParamIndex = (VstInt32)Leveller::ParamIndices::Band1Freq +
                                  (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
        VstInt32 gainParamIndex = (VstInt32)Leveller::ParamIndices::Band1Gain +
                                  (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);

        // Set the parameters using VST automation
        GetEffectX()->setParameterAutomated(freqParamIndex, M7::math::clamp01(freqParamValue));
        GetEffectX()->setParameterAutomated(gainParamIndex, M7::math::clamp01(gainParamValue));
      };
    };

    // Create Q parameter change handlers for each band
    auto makeBandResoHandler = [this](uintptr_t bandIndex)
    {
      return [this, bandIndex](M7::Param01 value, uintptr_t userData)
      {
        //// Convert Q value to parameter value using the div curved parameter configuration
        //M7::QuickParam qParam;
        //qParam.SetDivCurvedValue(M7::gBiquadFilterQCfg, qValue);
        //float qParamValue = qParam.GetRawValue();

        VstInt32 qParamIndex = (VstInt32)Leveller::ParamIndices::Band1Q +
                               (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);

        GetEffectX()->setParameterAutomated(qParamIndex, M7::math::clamp01(value.value));
      };
    };

    auto makeBandFreqGetter = [this](uintptr_t bandIndex)
    {
      return [this, bandIndex]() -> float
      {
        VstInt32 freqParamIndex = (VstInt32)Leveller::ParamIndices::Band1Freq +
                                  (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
        float freqParamValue = GetEffectX()->getParameter(freqParamIndex);
        M7::ParamAccessor freqParamAccessor{&freqParamValue, 0};
        return freqParamAccessor.GetFrequency(0, M7::gFilterFreqConfig);
      };
    };

    auto makeBandResoGetter = [this](uintptr_t bandIndex)
    {
      return [this, bandIndex]() -> M7::Param01
      {
        VstInt32 qParamIndex = (VstInt32)Leveller::ParamIndices::Band1Q +
                               (VstInt32(bandIndex) * (int)Leveller::BandParamOffsets::Count__);
        return M7::Param01{GetEffectX()->getParameter(qParamIndex)};
      };
    };

    const std::array<FrequencyResponseRendererFilter, Leveller::gBandCount> filters = {
        FrequencyResponseRendererFilter{bandColors[0],
                                        (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band1Enable) >
                                         0.5f)
                        ? nullptr
                                            : nullptr,
                                        "A",
                                        makeBandHandler(0),
                                        makeBandResoHandler(0),
                      0,
                      (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band1Enable) >
                       0.5f)
                        ? mpLeveller->mBands[0].mFilters[0].mSelectedFilter
                        : nullptr,
                      makeBandFreqGetter(0),
                      makeBandResoGetter(0)},
        FrequencyResponseRendererFilter{bandColors[1],
                                        (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band2Enable) >
                                         0.5f)
                        ? nullptr
                                            : nullptr,
                                        "B",
                                        makeBandHandler(1),
                                        makeBandResoHandler(1),
                      1,
                      (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band2Enable) >
                       0.5f)
                        ? mpLeveller->mBands[1].mFilters[0].mSelectedFilter
                        : nullptr,
                      makeBandFreqGetter(1),
                      makeBandResoGetter(1)},
        FrequencyResponseRendererFilter{bandColors[2],
                                        (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band3Enable) >
                                         0.5f)
                        ? nullptr
                                            : nullptr,
                                        "C",
                                        makeBandHandler(2),
                                        makeBandResoHandler(2),
                      2,
                      (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band3Enable) >
                       0.5f)
                        ? mpLeveller->mBands[2].mFilters[0].mSelectedFilter
                        : nullptr,
                      makeBandFreqGetter(2),
                      makeBandResoGetter(2)},
        FrequencyResponseRendererFilter{bandColors[3],
                                        (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band4Enable) >
                                         0.5f)
                        ? nullptr
                                            : nullptr,
                                        "D",
                                        makeBandHandler(3),
                                        makeBandResoHandler(3),
                      3,
                      (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band4Enable) >
                       0.5f)
                        ? mpLeveller->mBands[3].mFilters[0].mSelectedFilter
                        : nullptr,
                      makeBandFreqGetter(3),
                      makeBandResoGetter(3)},
        FrequencyResponseRendererFilter{bandColors[4],
                                        (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band5Enable) >
                                         0.5f)
                        ? nullptr
                                            : nullptr,
                                        "E",
                                        makeBandHandler(4),
                                        makeBandResoHandler(4),
                      4,
                      (GetEffectX()->getParameter((VstInt32)Leveller::ParamIndices::Band5Enable) >
                       0.5f)
                        ? mpLeveller->mBands[4].mFilters[0].mSelectedFilter
                        : nullptr,
                      makeBandFreqGetter(4),
                      makeBandResoGetter(4)},
    };

    FrequencyResponseRendererConfig<Leveller::gBandCount, (size_t)Leveller::ParamIndices::NumParams> cfg{
        ColorFromHTML("222222", 1.0f),  // background
        ColorFromHTML("aaaa00", 1.0f),  // line
        11.0f,                          // thumbRadius
        filters,                        // filters array
        {},                             // mParamCacheCopy (will be filled below)
        {},                             // majorFreqTicks (empty = defaults)
        {},                             // minorFreqTicks (empty = defaults)
        {},                             // fftOverlays - will be populated below
        -60.0f,                         // fftDisplayMinDB - noise floor
        0.0f,                           // fftDisplayMaxDB - Digital maximum
        true                            // useIndependentFFTScale - Separate from EQ response scale
    };

    // Configure FFT overlays for input/output comparison
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    cfg.fftOverlays = {{
                           &mpLeveller->mInputSpectrumSmoother,  // Input signal (before EQ)
                           ColorFromHTML("888888", 0.8f),        // Orange - Input signal
                           ColorFromHTML("444444", 0.3f),        // Orange fill (more transparent)
                           true,                                 // Enable fill
                           "Input"                               // Label for legend
                       },
                       { &mpLeveller->mOutputSpectrumSmoother,  // Output signal (after EQ)
                         ColorFromHTML("007777", 0.8f),         // Green - Output signal
                         ColorFromHTML("005555", 0.3f),         // Green fill (more transparent)
                         true,                                  // Enable fill
                         "Output"                               // Label for legend
                       }};
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    for (size_t i = 0; i < (size_t)Leveller::ParamIndices::NumParams; ++i)
    {
      cfg.mParamCacheCopy[i] = GetEffectX()->getParameter((VstInt32)i);
    }

    // Apply current FFT scaling settings to renderer config
    cfg.fftDisplayMinDB = mFFTDisplayMinDB;
    cfg.fftDisplayMaxDB = mFFTDisplayMaxDB;

    mResponseGraph.OnRender(cfg);

    ImGui::SameLine();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    auto& iaL = mpLeveller->mInputAnalysis[0];
    auto& iaR = mpLeveller->mInputAnalysis[1];
    auto& oaL = mpLeveller->mOutputAnalysis[0];
    auto& oaR = mpLeveller->mOutputAnalysis[1];
#else
    AnalysisStream iaL{};  // mock
    AnalysisStream iaR{};  // mock
    AnalysisStream oaL{};  // mock
    AnalysisStream oaR{};  // mock

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    VUMeter("vu_inp", iaL, iaR, kvuMeterSize);
    ImGui::SameLine();
    VUMeter("vu_outp", oaL, oaR, kvuMeterSize);

    ImGui::Spacing();

    //ImGui::BeginGroup();

    RenderBand(0, "A", Leveller::ParamIndices::Band1Circuit, 90, bandColors[0]);
    ImGui::SameLine();
    RenderBand(1, "B", Leveller::ParamIndices::Band2Circuit, 250, bandColors[1]);
    ImGui::SameLine();
    RenderBand(2, "C", Leveller::ParamIndices::Band3Circuit, 1100, bandColors[2]);
    ImGui::SameLine();
    RenderBand(3, "D", Leveller::ParamIndices::Band4Circuit, 3000, bandColors[3]);
    ImGui::SameLine();
    RenderBand(4, "E", Leveller::ParamIndices::Band5Circuit, 8500, bandColors[4]);

    ImGui::Spacing();

    Maj7ImGuiParamVolume(
        (VstInt32)Leveller::ParamIndices::OutputVolume, "Output", WaveSabreCore::M7::gVolumeCfg12db, 0, {});
    ImGui::SameLine();
    Maj7ImGuiBoolParamToggleButton(Leveller::ParamIndices::EnableDCFilter, "DC Filter");

    //ImGui::EndGroup();
  }
};

#endif
