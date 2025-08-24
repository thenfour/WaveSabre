#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Maj7Comp.hpp"
#include <WaveSabreCore/BandSplitter.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>

namespace WaveSabreCore
{
struct Maj7MBC : public Device
{
  enum class OutputStream : uint8_t
  {
    Normal,
    Delta,
    Sidechain,
    Count__,
  };

  enum class ChannelMode : uint8_t
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
    bool mSolo = false;
    //DisplayStyle mDisplayStyle = DisplayStyle::Normal;
    OutputStream mOutputStream = OutputStream::Normal;
  };

  enum class ParamIndices
  {
    InputGain,
    ChannelMode,
    MultibandEnable,
    CrossoverAFrequency,
    CrossoverBFrequency,
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
    {"CWidth"}, \
    {"CPan"}, \
    {"CDryWet"}, \
  }
  // clang-format on

  static_assert((int)ParamIndices::NumParams == 63, "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
      8230,   // InGain = 0.25115966796875
      0,      // channel mode
      0,      // MBEnable = 0
      13557,  // xAFreq = 0.413726806640625
      21577,  // xBFreq = 0.658477783203125
      8230,   // OutGain = 0.25115966796875
      32767,  // SCEn = 0.999969482421875
      23197,  // SCThr = 0.707916259765625
      32206,  // SCOutp = 0.98284912109375
      8230,   // AInVol = 0.25115966796875
      8230,   // AOutVol = 0.25115966796875
      21845,  // AThresh = 0.666656494140625
      15269,  // AAttack = 0.465972900390625
      15909,  // ARelease = 0.485504150390625
      18939,  // ARatio = 0.577972412109375
      4369,   // AKnee = 0.133331298828125
      26214,  // AChanLnk = 0.79998779296875
      0,      // AEnable = 0
      0,      // ASCFEn = 0
      0,      // AHPF = 0
      14563,  // AHPQ = 0.444427490234375
      32767,  // ALPF = 0
      14563,  // ALPQ = 0.444427490234375
      0,      // ADrive = 0.125885009765625
      32767,  // ADryWet = 0.999969482421875
      16384,  // AMidSideMix = 0.5
      16384,  // APan = 0.5
      8230,   // BInVol = 0.25115966796875
      8230,   // BOutVol = 0.25115966796875
      21845,  // BThresh = 0.666656494140625
      15269,  // BAttack = 0.465972900390625
      15909,  // BRelease = 0.485504150390625
      18939,  // BRatio = 0.577972412109375
      4369,   // BKnee = 0.133331298828125
      26214,  // BChanLnk = 0.79998779296875
      32767,  // BEnable = 0.999969482421875
      0,      // BSCFEn = 0
      0,      // BHPF = 0
      14563,  // BHPQ = 0.444427490234375
      32767,  // BLPF = 0
      14563,  // BLPQ = 0.444427490234375
      0,      // BDrive = 0.125885009765625
      32767,  // BDryWet = 0.999969482421875
      16384,  // AMidSideMix = 0.5
      16384,  // APan = 0.5
      8230,   // CInVol = 0.25115966796875
      8230,   // COutVol = 0.25115966796875
      21845,  // CThresh = 0.666656494140625
      15269,  // CAttack = 0.465972900390625
      15909,  // CRelease = 0.485504150390625
      18939,  // CRatio = 0.577972412109375
      4369,   // CKnee = 0.133331298828125
      26214,  // CChanLnk = 0.79998779296875
      0,      // CEnable = 0
      0,      // CSCFEn = 0
      0,      // CHPF = 0
      14563,  // CHPQ = 0.444427490234375
      32767,  // CLPF = 0
      14563,  // CLPQ = 0.444427490234375
      0,      // CDrive = 0.125885009765625
      32767,  // CDryWet = 0.999969482421875
      16384,  // AMidSideMix = 0.5
      16384,  // APan = 0.5
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
      DryWet,
      MidSideMix,
      Pan,
      Count__,
    };

    static_assert((int)ParamIndices::BInputGain - (int)ParamIndices::AInputGain == (int)BandParam::Count__,
                  "bandparams need to be in sync with main params");

    M7::ParamAccessor mParams;
    MonoCompressor mComp[2];
    bool mEnable;
    float mDriveLin;
    float mDriveGainCompensationFact;
    float mInputGainLin;
    float mOutputGainLin;
    float mDryWetMix;
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
        , mAttenuationAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    {
    }


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    static constexpr float gAnalysisFalloffMS = 500;
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
      mInputGainLin = mParams.GetLinearVolume(BandParam::InputGain, M7::gVolumeCfg24db);
      mOutputGainLin = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg24db);
      mDryWetMix = mParams.Get01Value(BandParam::DryWet);

      // tanh gain compensation is not perfect, but experimentally this works quite well.
      mDriveGainCompensationFact = M7::math::CalcTanhGainCompensation(mDriveLin);

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
                    mParams.GetFrequency(BandParam::HighPassFrequency, M7::gFilterFreqConfig),
                    mParams.GetDivCurvedValue(BandParam::HighPassQ, M7::gBiquadFilterQCfg),
                    mParams.GetFrequency(BandParam::LowPassFrequency, M7::gFilterFreqConfig),
                    mParams.GetDivCurvedValue(BandParam::LowPassQ, M7::gBiquadFilterQCfg));
      }
    }

    M7::FloatPair ProcessSample(const M7::FloatPair& input, ChannelMode channelMode, bool isGuiVisible)
    {
      M7::FloatPair output{input};
      if (mEnable)
      {
        float monoDetector =
            (input.x[0] + input.x[1]) *
            0.5f;  // apparently averaging yields slightly more consistent sweeping between mono->stereo link
        float channelLink01 = channelMode == ChannelMode::Stereo ? mParams.Get01Value(BandParam::ChannelLink) : 0;
        for (size_t ich = 0; ich < 2; ++ich)
        {
          float inpAudio = input.x[ich] * mInputGainLin;
          float detector = M7::math::lerp(inpAudio, monoDetector, channelLink01);
          float wetSignal = mComp[ich].ProcessSample(inpAudio, detector);
          if (mDriveLin > 1)
          {
            wetSignal = M7::math::tanh(mDriveLin * wetSignal);
          }
          wetSignal *= mOutputGainLin * mDriveGainCompensationFact;

          // Apply dry/wet mix
          float finalSignal = M7::math::lerp(inpAudio, wetSignal, mDryWetMix);
          output.x[ich] = finalSignal;

          WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[ich], inpAudio);
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

  M7::FrequencySplitter splitter0;
  M7::FrequencySplitter splitter1;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static constexpr float gAnalysisFalloffMS = 500;
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
  AttenuationAnalysisStream mClippingAnalysis[2];
  SmoothedStereoFFT mInputSpectrum;
  SmoothedStereoFFT mOutputSpectrum;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7MBC()
      : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
      , mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      , mInputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
      , mOutputAnalysis{AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS}}
      , mClippingAnalysis{AttenuationAnalysisStream{4000}, AttenuationAnalysisStream{4000}}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  {
    LoadDefaults();
  }

  //#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  //		// Called by VST editor when GUI is opened/closed to optimize CPU usage
  //		void SetGuiVisible(bool visible) {
  //			mGuiVisible.store(visible, std::memory_order_relaxed);
  //		}
  //
  //		bool IsGuiVisible() const {
  //			return mGuiVisible.load(std::memory_order_relaxed);
  //		}
  //#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void OnParamsChanged() override
  {
    for (auto& b : mBands)
    {
      b.Slider();
    }
  }

  // returns {output sample, clip amount}
  // NOTE: clip amount is 0 for anything below clipping level, even though shaping actually does mess with amplitudes.
  // the visualizations just don't care about that area though; users only care about how much is being clipped.
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  static M7::FloatPair Softclip(float s, float thresh, float outputGain){
#else
  static float Softclip(float s, float thresh, float outputGain)
  {
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT                                                                            \
    // thresh of 1 (unity gain) means hard clipping. 0.99 is something like 0.001dB so this is an optimal way to approximate hard clipping
      // and a thresh of 0 would cause our attenuation calc to fail.
      thresh = M7::math::clamp(thresh, 0.01f, 0.99f);
  if (s < 0)
  {
    s = -s;
    outputGain = -outputGain;
  }
  static constexpr float naturalSlope = M7::math::gPIHalf;
  float corrSlope = M7::math::lerp(naturalSlope, 1, thresh);
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  float atten = 1;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  s *= corrSlope;
  if (s > thresh)
  {
    s = M7::math::lerp_rev(thresh, 1, s);  // pull down above thresh range to 0,1
    s /= naturalSlope;
    if (s > 1)
    {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      atten = (1.0f / s);
      // we are clipping. for analysis purposes, make it CLEAR.
      atten = std::min(0.95f, atten);  // this means attenuation is MINIMUM reported as .5dB
#endif                                 // SELECTABLE_OUTPUT_STREAM_SUPPORT
      s = 1;
    }
    else
    {
      s = M7::math::sin(s * M7::math::gPIHalf);
    }
    s = M7::math::lerp(thresh, 1, s);
  }
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  return {s * outputGain, atten};  // readd sign bit
#else
    return s * outputGain;  // readd sign bit
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
}

virtual void
Run(float** inputs, float** outputs, int numSamples) override
{
  const float inputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
  const float outputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
  const float crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
  const float crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
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
  bool solos[Maj7MBC::gBandCount] = {mBands[0].mVSTConfig.mSolo,
                                     mBands[1].mVSTConfig.mSolo,
                                     mBands[2].mVSTConfig.mSolo};
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

    // channel mode processing. everything internally is 2-channel, and there's the channel mix knob,
    // so for mid or side modes, we could
    // 1. set both channels to mid or side
    // 2. set one channel to mid/side and the other to 0.
    // 3. set one channel to mid/side and the other to the original opposite channel (side or mid).
    //    THIS is probably the "best" because it lets channel mix knob make sense (detector blend from mid-side), even if this is kinda odd and barely useful.
    // 4. don't set the other channel. in this case it's the original input signal but it just doesn't matter because chan link shall be 0 for mid/side modes.
    if (channelMode != ChannelMode::Stereo)
    {
      s = s.MSEncode();
    }
    //switch (channelMode)
    //{
    //  case ChannelMode::Mid:
    //  {
    //    s = inputMidSide;
    //    break;
    //  }
    //  case ChannelMode::Side:
    //  {
    //    s = inputMidSide.yx();
    //    break;
    //  }
    //}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    // to support solo/mute, because this gets clobbered later, if you are doing ms processing and solo/muting, need to accumulate this just like the main signal.
    M7::FloatPair msDrySignal;
#endif      // SELECTABLE_OUTPUT_STREAM_SUPPORT

    WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[0], s[0]);
    WRITE_ANALYSIS_SAMPLE(isGuiVisible, mInputAnalysis[1], s[1]);
    WRITE_SPECTRUM_SAMPLE(isGuiVisible, mInputSpectrum, s);

    if (mbEnable)
    {
      // split into 3 bands
      splitter0.frequency_splitter(s[0], crossoverFreqA, crossoverFreqB);
      splitter1.frequency_splitter(s[1], crossoverFreqA, crossoverFreqB);

      s.Clear();

      for (int iBand = 0; iBand < gBandCount; ++iBand)
      {
        auto& band = mBands[iBand];
        M7::FloatPair bandInput{splitter0.s[iBand], splitter1.s[iBand]};
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        if (band.mMuteSoloEnable)
        {
          msDrySignal += bandInput;
        }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
        auto r = band.ProcessSample(bandInput, channelMode, isGuiVisible);
        s.Accumulate(r * outputGainLin);
      }
    }
    else
    {
      auto& band = mBands[1];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
        if (band.mMuteSoloEnable)
        {
          msDrySignal += s;
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

    outputs[0][i] = s[0];
    outputs[1][i] = s[1];

  }  // for i < numSamples
}
};  // namespace WaveSabreCore
}  // namespace WaveSabreCore
