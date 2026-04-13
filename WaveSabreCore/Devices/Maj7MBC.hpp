#pragma once

#include "../Analysis/FFTAnalysis.hpp"
#include "../DSP/Maj7SaturationBase.hpp"
#include "../Filters/BandSplitter.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../WSCore/Device.h"
#include "Maj7Comp.hpp"


namespace WaveSabreCore
{
struct Maj7MBC : public Device
{
  enum class OutputStream  // : uint8_t
  {
    Normal,
    Delta,
    Sidechain,
    Count__,
  };

  enum class ChannelMode  // : uint8_t
  {
    Stereo,
    Mid,
    Side,
    Count__,
  };

  struct BandConfig
  {
    // let's just assume bool is atomic.
    bool mMute = false;
    bool mSolo = false;  // note that when output stream = delta or sidechain, solo is implicitly on.
    OutputStream mOutputStream = OutputStream::Normal;
  };

  enum class ParamIndices
  {
    InputGain,
    ChannelMode,
    MultibandEnable,
    CrossoverAFrequency,
    CrossoverBFrequency,
    CrossoverASlope,
    CrossoverBSlope,
    OutputGain,
    SoftClipEnable,
    SoftClipThresh,
    SoftClipOutput,

    AInputGain,
    AOutputGain,
    AThreshold,
    AAttack,
    ARelease,
    ARatio,
    AKnee,
    AChannelLink,
    AEnable,  // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
    ASidechainFilterEnable,
    AHighPassFrequency,  // biquad freq; default lo
    AHighPassQ,          // default 0.2
    ALowPassFrequency,   // biquad freq
    ALowPassQ,           // default 0.2
    ADrive,
    ASaturationModel,
    ASaturationThreshold,
    ASaturationEvenHarmonics,
    AMidSideMix,
    APan,
    ADryWet,

    BInputGain,
    BOutputGain,
    BThreshold,
    BAttack,
    BRelease,
    BRatio,
    BKnee,
    BChannelLink,
    BEnable,  // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
    BSidechainFilterEnable,
    BHighPassFrequency,  // biquad freq; default lo
    BHighPassQ,          // default 0.2
    BLowPassFrequency,   // biquad freq
    BLowPassQ,           // default 0.2
    BDrive,
    BSaturationModel,
    BSaturationThreshold,
    BEvenHarmonics,
    BMidSideMix,
    BPan,
    BDryWet,

    CInputGain,
    COutputGain,
    CThreshold,
    CAttack,
    CRelease,
    CRatio,
    CKnee,
    CChannelLink,
    CEnable,  // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
    CSidechainFilterEnable,
    CHighPassFrequency,  // biquad freq; default lo
    CHighPassQ,          // default 0.2
    CLowPassFrequency,   // biquad freq
    CLowPassQ,           // default 0.2
    CDrive,
    CSaturationModel,
    CSaturationThreshold,
    CEvenHarmonics,
    CMidSideMix,
    CPan,
    CDryWet,

    NumParams,
  };

  // clang-format off
		// NB: max 8 chars per string.
#define MAJ7MBC_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7MBC::ParamIndices::NumParams]{ \
    {"InGain"},\
    {"ChMode"},\
    {"MBEnable"},\
    {"xAFreq"},\
    {"xBFreq"},\
    {"xASlope"},\
    {"xBSlope"},\
    {"OutGain"},\
    {"SCEn"},\
    {"SCThr"},\
    {"SCOutp"},\
    {"AInVol"},\
    {"AOutVol"},\
    {"AThresh"},\
    {"AAttack"},\
    {"ARelease"},\
    {"ARatio"},\
    {"AKnee"},\
    {"AChanLnk"},\
    {"AEnable"},\
    {"ASCFEn"},\
    {"AHPF"},\
    {"AHPQ"},\
    {"ALPF"},\
    {"ALPQ"},\
    {"ADrive"}, \
    {"ASatMod"}, \
    {"ASatThr"}, \
    {"AAnalog"},\
    {"AWidth"},\
    {"APan"},\
    {"ADryWet"},\
    {"BInVol"},\
    {"BOutVol"},\
    {"BThresh"},\
    {"BAttack"},\
    {"BRelease"},\
    {"BRatio"},\
    {"BKnee"},\
    {"BChanLnk"},\
    {"BEnable"},\
    {"BSCFEn"},\
    {"BHPF"},\
    {"BHPQ"},\
    {"BLPF"},\
    {"BLPQ"},\
    {"BDrive"},\
    {"BSatMod"}, \
    {"BSatThr"}, \
    {"BAnalog"},\
    {"BWidth"}, \
    {"BPan"}, \
    {"BDryWet"},\
    {"CInVol"},\
    {"COutVol"},\
    {"CThresh"},\
    {"CAttack"},\
    {"CRelease"},\
    {"CRatio"},\
    {"CKnee"},\
    {"CChanLnk"},\
    {"CEnable"},\
    {"CSCFEn"},\
    {"CHPF"},\
    {"CHPQ"},\
    {"CLPF"},\
    {"CLPQ"},\
    {"CDrive"},\
    {"CSatMod"}, \
    {"CSatThr"}, \
    {"CAnalog"},\
    {"CWidth"}, \
    {"CPan"}, \
    {"CDryWet"}, \
  }
  // clang-format on

  static_assert((int)ParamIndices::NumParams == 74, "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
      8230,   // InGain = 0.25118863582611083984
      0,      // ChMode = 0
      0,      // MBEnable = 0
      13557,  // xAFreq = 0.4137503504753112793
      21576,  // xBFreq = 0.65849626064300537109
      3,      // xASlope = 0.0001220703125
      3,      // xBSlope = 0.0001220703125
      8230,   // OutGain = 0.25118863582611083984
      32767,  // SCEn = 1
      23197,  // SCThr = 0.70794582366943359375
      32205,  // SCOutp = 0.98287886381149291992
      8230,   // AInVol = 0.25118863582611083984
      8230,   // AOutVol = 0.25118863582611083984
      21844,  // AThresh = 0.6666666865348815918
      15268,  // AAttack = 0.46597486734390258789
      15909,  // ARelease = 0.48552104830741882324
      18938,  // ARatio = 0.57798200845718383789
      4368,   // AKnee = 0.13333334028720855713
      26213,  // AChanLnk = 0.80000001192092895508
      0,      // AEnable = 0
      0,      // ASCFEn = 0
      5949,   // AHPF = 0.18155753612518310547
      11176,  // AHPQ = 0.34108528494834899902
      26213,  // ALPF = 0.80000001192092895508
      11176,  // ALPQ = 0.34108528494834899902
      0,      // ADrive = 0
      7,      // ASatMod = 0.000244140625
      20674,  // ASatThr = 0.63095736503601074219
      0,      // AAnalog = 0
      0,      // AWidth = 0
      0,      // APan = 0
      32767,  // ADryWet = 1
      8230,   // BInVol = 0.25118863582611083984
      8230,   // BOutVol = 0.25118863582611083984
      21844,  // BThresh = 0.6666666865348815918
      15268,  // BAttack = 0.46597486734390258789
      15909,  // BRelease = 0.48552104830741882324
      18938,  // BRatio = 0.57798200845718383789
      4368,   // BKnee = 0.13333334028720855713
      26213,  // BChanLnk = 0.80000001192092895508
      32767,  // BEnable = 1
      0,      // BSCFEn = 0
      5949,   // BHPF = 0.18155753612518310547
      11176,  // BHPQ = 0.34108528494834899902
      26213,  // BLPF = 0.80000001192092895508
      11176,  // BLPQ = 0.34108528494834899902
      0,      // BDrive = 0
      7,      // BSatMod = 0.000244140625
      20674,  // BSatThr = 0.63095736503601074219
      0,      // BAnalog = 0
      0,      // BWidth = 0
      0,      // BPan = 0
      32767,  // BDryWet = 1
      8230,   // CInVol = 0.25118863582611083984
      8230,   // COutVol = 0.25118863582611083984
      21844,  // CThresh = 0.6666666865348815918
      15268,  // CAttack = 0.46597486734390258789
      15909,  // CRelease = 0.48552104830741882324
      18938,  // CRatio = 0.57798200845718383789
      4368,   // CKnee = 0.13333334028720855713
      26213,  // CChanLnk = 0.80000001192092895508
      0,      // CEnable = 0
      0,      // CSCFEn = 0
      5949,   // CHPF = 0.18155753612518310547
      11176,  // CHPQ = 0.34108528494834899902
      26213,  // CLPF = 0.80000001192092895508
      11176,  // CLPQ = 0.34108528494834899902
      0,      // CDrive = 0
      7,      // CSatMod = 0.000244140625
      20674,  // CSatThr = 0.63095736503601074219
      0,      // CAnalog = 0
      0,      // CWidth = 0
      0,      // CPan = 0
      32767,  // CDryWet = 1
  };


  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  struct FreqBand
  {
    enum class BandParam : uint8_t
    {
      InputGain,
      OutputGain,
      Threshold,
      Attack,
      Release,
      Ratio,
      Knee,
      ChannelLink,
      Enable,
      SidechainFilterEnable,
      HighPassFrequency,
      HighPassQ,
      LowPassFrequency,
      LowPassQ,
      Drive,
      SaturationModel,
      SaturationThreshold,
      SaturationEvenHarmonics,
      MidSideMix,
      Pan,
      DryWet,
      Count__,
    };

    static_assert((int)ParamIndices::BInputGain - (int)ParamIndices::AInputGain == (int)BandParam::Count__,
                  "bandparams need to be in sync with main params");

    M7::ParamAccessor mParams;
    MonoCompressor mComp[2];
    bool mEnable;
    float mDriveLin;
    float mDriveGainCompensationFact;
    M7::Maj7SaturationBase::Model mSaturationModel = M7::Maj7SaturationBase::Model::Thru;
    float mSaturationEvenHarmonics = 0.0f;
    float mSaturationCorrSlope = 1.0f;
    float mSaturationThresholdLin = 0.0f;
    float mInputGainLin;
    float mOutputGainLin;
    float mDryWetMix;
#ifdef MAJ7SAT_ENABLE_ANALOG
    M7::DCFilter mSaturationDC[2];
#endif
    // cached imaging params
    float mMidSideMixN11 = 0.0f;  // -1..+1
    M7::FloatPair mPanGains{1.0f, 1.0f};

    FreqBand(float* paramCache, ParamIndices baseParamID)
        :  //
        mParams(paramCache, baseParamID)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        , mInputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
        , mOutputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
        , mDetectorAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
        , mAttenuationAnalysis{AnalysisStream{gCompressionAttenuationFalloffMS},
                               AnalysisStream{gCompressionAttenuationFalloffMS}}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    {
    }


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    static constexpr float gAnalysisFalloffMS = 500;
    static constexpr float gCompressionAttenuationFalloffMS = 250;
    AnalysisStream mInputAnalysis[2];
    AnalysisStream mOutputAnalysis[2];
    AnalysisStream mDetectorAnalysis[2];
    AnalysisStream mAttenuationAnalysis[2];

    BandConfig mVSTConfig;
    bool mMuteSoloEnable;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    void Slider()
    {
      mEnable = mParams.GetBoolValue(BandParam::Enable);
      mDriveLin = M7::math::DecibelsToLinear(mParams.GetScaledRealValue(BandParam::Drive, 0, 30, 0));
      mSaturationModel = mParams.GetEnumValue<M7::Maj7SaturationBase::Model>(BandParam::SaturationModel);
      mSaturationThresholdLin = mParams.GetLinearVolume(BandParam::SaturationThreshold, M7::gUnityVolumeCfg);
      mSaturationThresholdLin = M7::math::clamp(mSaturationThresholdLin, 0.0f, 0.99f);
      mSaturationEvenHarmonics = mParams.GetScaledRealValue(BandParam::SaturationEvenHarmonics, 0, 2, 0);
      mInputGainLin = mParams.GetLinearVolume(BandParam::InputGain, M7::gVolumeCfg24db);
      mOutputGainLin = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg24db);
      mDryWetMix = mParams.Get01Value(BandParam::DryWet);

      mDriveGainCompensationFact = M7::Maj7SaturationBase::CalcAutoDriveCompensation(mDriveLin);
      mSaturationCorrSlope = M7::math::lerp(M7::Maj7SaturationBase::ModelNaturalSlopes[(int)mSaturationModel],
                                            1.0f,
                                            mSaturationThresholdLin);

      // cache imaging params
      mMidSideMixN11 = mParams.GetN11Value(BandParam::MidSideMix, 0);
      {
        float panN11 = mParams.GetN11Value(BandParam::Pan, 0);
        mPanGains = M7::math::PanToFactor(panN11);
      }

      for (auto& c : mComp)
      {
        c.SetParams(mParams.GetDivCurvedValue(BandParam::Ratio, MonoCompressor::gRatioCfg, 0),
                    mParams.GetScaledRealValue(BandParam::Knee, 0, 30, 0),
                    mParams.GetScaledRealValue(BandParam::Threshold, -60, 0, 0),
                    mParams.GetPowCurvedValue(BandParam::Attack, MonoCompressor::gAttackCfg, 0),
                    mParams.GetPowCurvedValue(BandParam::Release, MonoCompressor::gReleaseCfg, 0),
                    mParams.GetBoolValue(BandParam::SidechainFilterEnable),
                    mParams.GetFrequency(BandParam::HighPassFrequency, M7::gFilterFreqConfig)
            );
            //,
            //        M7::Decibels{mParams.GetDivCurvedValue(BandParam::HighPassQ, M7::gBiquadFilterQCfg)},
            //        mParams.GetFrequency(BandParam::LowPassFrequency, M7::gFilterFreqConfig),
            //        M7::Decibels{mParams.GetDivCurvedValue(BandParam::LowPassQ, M7::gBiquadFilterQCfg)});
      }
    }

    M7::FloatPair ProcessSample(const M7::FloatPair& input, ChannelMode channelMode, bool isGuiVisible)
    {
      M7::FloatPair output{input};
      if (mEnable)
      {
        float channelLink01 = channelMode == ChannelMode::Stereo ? mParams.Get01Value(BandParam::ChannelLink) : 0;
        float inpAudio[2] = {
            input.x[0] * mInputGainLin,
            input.x[1] * mInputGainLin,
        };
        float detectorLevel[2] = {
            mComp[0].ComputeDetectorLevel(inpAudio[0]),
            mComp[1].ComputeDetectorLevel(inpAudio[1]),
        };
        float linkedDetectorLevel = (detectorLevel[0] + detectorLevel[1]) * 0.5f;
        for (size_t ich = 0; ich < 2; ++ich)
        {
          float detector = M7::math::lerp(detectorLevel[ich], linkedDetectorLevel, channelLink01);
          float wetSignal = mComp[ich].ApplyGainFromDetector(inpAudio[ich], detector);

          const bool doSaturation = (mDriveLin > 1.0f) || (mSaturationModel != M7::Maj7SaturationBase::Model::Thru) ||
                                    (mSaturationEvenHarmonics > 0.0f);

          if (doSaturation)
          {
            wetSignal *= mDriveLin;
            wetSignal = M7::Maj7SaturationBase::DistortSample(wetSignal,
                                                              ich,
                                                              mSaturationModel,
                                                              mSaturationThresholdLin,
                                                              mSaturationCorrSlope,
                                                              mDriveGainCompensationFact
#ifdef MAJ7SAT_ENABLE_ANALOG
                                                              ,
                                                              mSaturationDC,
                                                              mSaturationEvenHarmonics
#endif
            );
          }
          wetSignal *= mOutputGainLin;

          // Apply dry/wet mix
          float finalSignal = M7::math::lerp(inpAudio[ich], wetSignal, mDryWetMix);
          output.x[ich] = finalSignal;

          WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[ich], inpAudio[ich]);
          WRITE_ANALYSIS_SAMPLE(isGuiVisible, mDetectorAnalysis[ich], detector);
          WRITE_ANALYSIS_SAMPLE(isGuiVisible, mAttenuationAnalysis[ich], mComp[ich].mGainReduction);
        }

        if (channelMode == ChannelMode::Stereo)
        {
          output = output.MidSideMixOnStereo(mMidSideMixN11);

          // Apply equal-power pan with compensation (same as Maj7Width)
          output.x[0] *= mPanGains.x[0] * M7::math::gPanCompensationGainLin;
          output.x[1] *= mPanGains.x[1] * M7::math::gPanCompensationGainLin;
        }

        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[0], output.x[0]);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[1], output.x[1]);
      }  // ENABLE
      else
      {
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[0], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[0], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mDetectorAnalysis[0], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mAttenuationAnalysis[0], 0);

        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[1], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[1], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mDetectorAnalysis[1], 0);
        WRITE_ANALYSIS_SAMPLE(isGuiVisible, mAttenuationAnalysis[1], 0);
      }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (!mMuteSoloEnable)
      {
        return {0, 0};
      }
      switch (mVSTConfig.mOutputStream)
      {
        case OutputStream::Normal:
          break;
        case OutputStream::Delta:
        {
          if (!mEnable)
            return {0, 0};
          return {mComp[0].mDiff, mComp[1].mDiff};
        }
        case OutputStream::Sidechain:
        {
          if (!mEnable)
            return input;
          return {mComp[0].mSidechain, mComp[1].mSidechain};
        }
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

      return output;
    }
  };  // struct FreqBand
  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  float mParamCache[(int)ParamIndices::NumParams];

  M7::ParamAccessor mParams;

  static constexpr size_t gBandCount = 3;

  FreqBand mBands[gBandCount] = {
      {mParamCache, ParamIndices::AInputGain},
      {mParamCache, ParamIndices::BInputGain},
      {mParamCache, ParamIndices::CInputGain},
  };

  M7::BandSplitter splitter0;
  M7::BandSplitter splitter1;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static constexpr float gAnalysisFalloffMS = 500;
  static constexpr float gSoftclipAttenuationFalloffMS = 100;
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
  AttenuationAnalysisStream mClippingAnalysis[2];
  SmoothedStereoFFT mInputSpectrum;
  SmoothedStereoFFT mOutputSpectrum;
  SmoothedMonoFFT mInputMidSpectrum;
  SmoothedMonoFFT mInputSideSpectrum;
  SmoothedMonoFFT mOutputMidSpectrum;
  SmoothedMonoFFT mOutputSideSpectrum;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7MBC()
      : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
      , mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      , mInputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
      , mOutputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
      , mClippingAnalysis{AttenuationAnalysisStream{gSoftclipAttenuationFalloffMS},
                          AttenuationAnalysisStream{gSoftclipAttenuationFalloffMS}}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  {
    LoadDefaults();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    // Keep FFT visualization active while reducing analysis CPU cost.
    mInputSpectrum.SetInputDecimationFactor(2);
    mOutputSpectrum.SetInputDecimationFactor(2);
    mInputMidSpectrum.SetInputDecimationFactor(2);
    mInputSideSpectrum.SetInputDecimationFactor(2);
    mOutputMidSpectrum.SetInputDecimationFactor(2);
    mOutputSideSpectrum.SetInputDecimationFactor(2);
#endif
  }

  virtual void OnParamsChanged() override
  {
    for (auto& b : mBands)
    {
      b.Slider();
    }

    const float crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
    const float crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
    const auto crossoverSlopeA = mParams.GetEnumValue<M7::CrossoverSlope>(ParamIndices::CrossoverASlope);
    const auto crossoverSlopeB = mParams.GetEnumValue<M7::CrossoverSlope>(ParamIndices::CrossoverBSlope);
    splitter0.SetParams(crossoverFreqA, crossoverSlopeA, crossoverFreqB, crossoverSlopeB);
    splitter1.SetParams(crossoverFreqA, crossoverSlopeA, crossoverFreqB, crossoverSlopeB);
  }

  // returns {output sample, transfer gain}
  // The analysis value is the clipper's instantaneous transfer gain before makeup gain, where
  // 1.0 means no reduction and smaller values mean stronger soft-clip attenuation.
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static M7::FloatPair Softclip(float s, float thresh, float outputGain)
  {
    float transferGain = 1.0f;
    const float y = M7::Maj7SaturationBase::SoftClipSine(s, thresh, outputGain, &transferGain);
    return {y, transferGain};
  }
#else
  static float Softclip(float s, float thresh, float outputGain)
  {
    return M7::Maj7SaturationBase::SoftClipSine(s, thresh, outputGain);
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    const float inputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
    const float outputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
    const bool mbEnable = mParams.GetBoolValue(ParamIndices::MultibandEnable);

    const bool softClipEnabled = mParams.GetBoolValue(ParamIndices::SoftClipEnable);
    const float softClipThreshLin = mParams.GetLinearVolume(ParamIndices::SoftClipThresh, M7::gUnityVolumeCfg);
    const float softClipOutputLin = mParams.GetLinearVolume(ParamIndices::SoftClipOutput, M7::gUnityVolumeCfg);
    const ChannelMode channelMode = mParams.GetEnumValue<ChannelMode>(ParamIndices::ChannelMode);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    bool muteSoloEnabled[Maj7MBC::gBandCount] = {false, false, false};
    bool mutes[Maj7MBC::gBandCount] = {mBands[0].mVSTConfig.mMute,
                                       mBands[1].mVSTConfig.mMute,
                                       mBands[2].mVSTConfig.mMute};
    bool solos[Maj7MBC::gBandCount] = {
        mBands[0].mVSTConfig.mSolo || (mBands[0].mVSTConfig.mOutputStream != OutputStream::Normal),
        mBands[1].mVSTConfig.mSolo || (mBands[1].mVSTConfig.mOutputStream != OutputStream::Normal),
        mBands[2].mVSTConfig.mSolo || (mBands[2].mVSTConfig.mOutputStream != OutputStream::Normal),
    };
    M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);
    mBands[0].mMuteSoloEnable = muteSoloEnabled[0];
    mBands[1].mMuteSoloEnable = muteSoloEnabled[1];
    mBands[2].mMuteSoloEnable = muteSoloEnabled[2];

    // CPU optimization: only process FFT for visualization when GUI is visible
    const bool isGuiVisible = IsGuiVisible();
#else
    constexpr bool isGuiVisible = false;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

    for (size_t i = 0; i < (size_t)numSamples; ++i)
    {
      M7::FloatPair s{inputs[0][i], inputs[1][i]};
      s *= inputGainLin;
      const M7::FloatPair inputStereo = s;
      const M7::FloatPair inputMs = inputStereo.MSEncode();

      // channel mode processing. everything internally is 2-channel, and there's the channel mix knob,
      // so for mid or side modes, we could
      // 1. set both channels to mid or side
      // 2. set one channel to mid/side and the other to 0.
      // 3. set one channel to mid/side and the other to the original opposite channel (side or mid).
      //    THIS is probably the "best" because it lets channel mix knob make sense (detector blend from mid-side), even if this is kinda odd and barely useful.
      // 4. don't set the other channel. in this case it's the original input signal but it just doesn't matter because chan link shall be 0 for mid/side modes.
      if (channelMode != ChannelMode::Stereo)
      {
        s = inputMs;
      }

      // accumulates either mid or side.
      // for mid mode, side is processed but ignored.
      // and vice-versa.
      M7::FloatPair msDrySignal{};

      WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[0], s[0]);
      WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[1], s[1]);
      WRITE_SPECTRUM_SAMPLE(isGuiVisible, mInputSpectrum, inputStereo);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (isGuiVisible)
      {
        mInputMidSpectrum.ProcessSample(inputMs[0]);
        mInputSideSpectrum.ProcessSample(inputMs[1]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

      if (mbEnable)
      {
        // split into 3 bands
        const auto splitter0Output = splitter0.Process(s[0]);
        const auto splitter1Output = splitter1.Process(s[1]);

        s.Clear();

        for (int iBand = 0; iBand < gBandCount; ++iBand)
        {
          auto& band = mBands[iBand];
          M7::FloatPair bandInput{splitter0Output.s[iBand], splitter1Output.s[iBand]};
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          if (band.mMuteSoloEnable)
          {
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
            msDrySignal += bandInput;
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
          auto r = band.ProcessSample(bandInput, channelMode, isGuiVisible);
          s.Accumulate(r * outputGainLin);
        }
      }
      else
      {
        // single wide band
        auto& band = mBands[1];  // use middle band for processing
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        if (band.mMuteSoloEnable)
        {
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
          msDrySignal = s;
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
        auto r = band.ProcessSample(s, channelMode, isGuiVisible);
        s = r * outputGainLin;
      }

      switch (channelMode)
      {
        case ChannelMode::Mid:
        {
          s = M7::FloatPair{s[0], msDrySignal[1]}.MSDecode();
          break;
        }
        case ChannelMode::Side:
        {
          s = M7::FloatPair{msDrySignal[0], s[1]}.MSDecode();
          break;
        }
      }

      if (softClipEnabled)
      {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        auto sc0 = Softclip(s[0], softClipThreshLin, softClipOutputLin);
        auto sc1 = Softclip(s[1], softClipThreshLin, softClipOutputLin);
        s[0] = sc0[0];
        s[1] = sc1[0];
        mClippingAnalysis[0].WriteSample(sc0[1]);
        mClippingAnalysis[1].WriteSample(sc1[1]);
#else
        s[0] = Softclip(s[0], softClipThreshLin, softClipOutputLin);
        s[1] = Softclip(s[1], softClipThreshLin, softClipOutputLin);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
      }

      WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[0], s[0]);
      WRITE_ANALYSIS_SAMPLE(isGuiVisible, mOutputAnalysis[1], s[1]);
      WRITE_SPECTRUM_SAMPLE(isGuiVisible, mOutputSpectrum, s);
      if (isGuiVisible)
      {
        const M7::FloatPair outputMs = s.MSEncode();
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        mOutputMidSpectrum.ProcessSample(outputMs[0]);
        mOutputSideSpectrum.ProcessSample(outputMs[1]);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
      }

      outputs[0][i] = s[0];
      outputs[1][i] = s[1];

    }  // for i < numSamples
  }
};  // namespace WaveSabreCore
}  // namespace WaveSabreCore
