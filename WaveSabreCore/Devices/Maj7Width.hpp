#pragma once

#include "../Analysis/StereoImageAnalysis.hpp"
#include "../Filters/BandSplitter.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../WSCore/Device.h"


namespace WaveSabreCore
{
struct Maj7Width : public Device
{
  static constexpr int gBandCount = M7::kBandSplitterBands;
  static constexpr M7::VolumeParamConfig gVolumeCfg{3.9810717055349722f, 12.0f};
  static constexpr auto gRotationExtent = M7::math::gPIQuarter;
  //static constexpr auto gShearAngleLimit = M7::math::gPIQuarter;

  struct BandConfig
  {
    bool mMute = false;
    bool mSolo = false;
  };

  // roughly in order of processing,
  enum class ParamIndices
  {
    LInvert,
    RInvert,
    SideHPFrequency,
    MultibandEnable,
    CrossoverAFrequency,
    CrossoverBFrequency,
    CrossoverASlope,
    CrossoverBSlope,
    OutputGain,

    // ALeftSource,
    // ARightSource,
    AWidth,
    APan,
    AAsymmetry,
    ASideGain,
    ARotation,
    //AShear,

    // BLeftSource,
    // BRightSource,
    BWidth,
    BPan,
    BAsymmetry,
    BSideGain,
    BRotation,
    //BShear,

    // CLeftSource,
    // CRightSource,
    CWidth,
    CPan,
    CAsymmetry,
    CSideGain,
    CRotation,
    //CShear,

    NumParams,
  };

  // clang-format off
// NB: max 8 chars per string.
#define MAJ7WIDTH_PARAM_VST_NAMES(symbolName)                                                                          \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Width::ParamIndices::NumParams]              \
  {                                                                                                                    \
    {"LInv"},\
    {"RInv"},\
    {"SideHPF"},\
    {"MBEn"},\
    {"xAFreq"},\
    {"xBFreq"},\
    {"xASlope"},\
    {"xBSlope"},\
    {"OutGain"},\
    {"AWidth"},\
    {"APan"},\
    {"AAsym"},\
    {"ASideG"},\
    {"ARot"},\
    {"BWidth"},\
    {"BPan"},\
    {"BAsym"},\
    {"BSideG"},\
    {"BRot"},\
    {"CWidth"},\
    {"CPan"},\
    {"CAsym"},\
    {"CSideG"},\
    {"CRot"},\
  }
  // clang-format on

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams;
  BandConfig mBandConfig[gBandCount];

  struct FreqBand
  {
    enum class BandParam : uint8_t
    {
      Width,
      Pan,
      Asymmetry,
      SideGain,
      Rotation,
      Count__,
      First = Width,
    };

    static_assert((int)ParamIndices::BWidth - (int)ParamIndices::AWidth == (int)BandParam::Count__,
                  "band params need to be in sync with main params");

    M7::ParamAccessor mParams;
    M7::VanillaOnePoleFilter mSideFilter;
    //float mLeftSourceN11 = -1.0f;
    //float mRightSourceN11 = 1.0f;
    float mWidthN11 = 0.0f;
    float mAsymmetryN11 = 0.0f;
    float mSideGainLin = 1.0f;
    float mRotation = 0.0f;
    //float mShearFactor = 0.0f;
    bool mSideHpfActive = false;
    bool mMuteSoloEnable = true;
    M7::FloatPair mPanGains{1.0f, 1.0f};

    FreqBand(float* paramCache, ParamIndices baseParamID)
        : mParams(paramCache, baseParamID)
    {
    }

    void Slider(bool sideHpfActive, float sideHpfFreq)
    {
      //mLeftSourceN11 = mParams.GetN11Value(BandParam::LeftSource, 0);
      //mRightSourceN11 = mParams.GetN11Value(BandParam::RightSource, 0);
      mWidthN11 = mParams.GetN11Value(BandParam::Width, 0);
      mPanGains = M7::math::PanToFactor(mParams.GetN11Value(BandParam::Pan, 0));
      mAsymmetryN11 = mParams.GetN11Value(BandParam::Asymmetry, 0);
      mSideGainLin = mParams.GetLinearVolume(BandParam::SideGain, gVolumeCfg);
      mRotation = mParams.GetScaledRealValue(BandParam::Rotation, -gRotationExtent, gRotationExtent, 0);
      //mShearFactor = -M7::math::tan(mParams.GetScaledRealValue(BandParam::Shear, -gShearAngleLimit, gShearAngleLimit, 0));
      mSideHpfActive = sideHpfActive;
      if (mSideHpfActive)
      {
        mSideFilter.SetParams(
            //M7::FilterCircuit::OnePole,
            //                  M7::FilterSlope::Slope6dbOct,
                              M7::FilterResponse::Highpass,
                              sideHpfFreq,
                              M7::Param01{0},
                              0);
      }
    }

    M7::FloatPair ProcessSample(const M7::FloatPair& inputMs, bool leftInvert, bool rightInvert)
    {
      if (!mMuteSoloEnable)
      {
        return {};
      }

      auto stereoIn = inputMs.MSDecode();
      float left = stereoIn.Left();
      float right = stereoIn.Right();

      if (leftInvert)
      {
        left = -left;
      }
      if (rightInvert)
      {
        right = -right;
      }

      auto ms = M7::FloatPair{left, right}.MSEncode();
      if (mSideHpfActive)
      {
        ms.x[1] = mSideFilter.ProcessSample(ms.Side());
      }

      if (mWidthN11 < 0.0f)
      {
        ms.x[1] *= mWidthN11 + 1.0f;
      }
      else if (mWidthN11 > 0.0f)
      {
        ms.x[0] *= 1.0f - mWidthN11;
      }

      ms.x[1] *= mSideGainLin;

#ifdef MAJ7WIDTH_FULL_FEATURE
      //Shear2(ms.x[1], ms.x[0], mShearFactor);
      Rotate2(ms.x[1], ms.x[0], mRotation);
#endif  // MAJ7WIDTH_FULL_FEATURE

      auto stereo = ms.MSDecode();
      if (mAsymmetryN11 < 0.0f)
      {
        stereo.x[1] = M7::math::lerp(stereo.x[1], stereo.x[0], -mAsymmetryN11);
      }
      else if (mAsymmetryN11 > 0.0f)
      {
        stereo.x[0] = M7::math::lerp(stereo.x[0], stereo.x[1], mAsymmetryN11);
      }

      stereo = stereo.mul(mPanGains);
      stereo *= M7::math::gPanCompensationGainLin;
      return stereo;
    }
  };

  static_assert((int)ParamIndices::NumParams == 24, "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
      0,      // LInv = 0
      0,      // RInv = 0
      0,      // SideHPF = 0
      0,      // MBEn = 0
      12051,  // xAFreq = 0.36780720949172973633
      21576,  // xBFreq = 0.65849626064300537109
      3,      // xASlope = 0.0001220703125
      3,      // xBSlope = 0.0001220703125
      16422,  // OutGain = 0.50118720531463623047
      0,      // AWidth = 0
      0,      // APan = 0
      0,      // AAsym = 0
      16422,  // ASideG = 0.50118720531463623047
      16383,  // ARot = 0.5
      0,      // BWidth = 0
      0,      // BPan = 0
      0,      // BAsym = 0
      16422,  // BSideG = 0.50118720531463623047
      16383,  // BRot = 0.5
      0,      // CWidth = 0
      0,      // CPan = 0
      0,      // CAsym = 0
      16422,  // CSideG = 0.50118720531463623047
      16383,  // CRot = 0.5
  };

  M7::BandSplitter mMidSplitter;
  M7::BandSplitter mSideSplitter;
  FreqBand mBands[gBandCount];

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
  StereoImagingAnalysisStream mInputImagingAnalysis;
  StereoImagingAnalysisStream mOutputImagingAnalysis;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7Width()
      : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
      , mParams(mParamCache, 0)
      , mBands{
        FreqBand{mParamCache, ParamIndices::AWidth},
        FreqBand{mParamCache, ParamIndices::BWidth},
        FreqBand{mParamCache, ParamIndices::CWidth},
      }
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      ,
      // quite fast moving here because the user's looking for transients.
      mInputAnalysis{AnalysisStream{1200}, AnalysisStream{1200}}
      , mOutputAnalysis{AnalysisStream{1200}, AnalysisStream{1200}}
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  {
    LoadDefaults();
  }

  virtual void OnParamsChanged() override
  {
    const float crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
    const float crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
    const auto crossoverSlopeA = mParams.GetEnumValue<M7::CrossoverSlope>(ParamIndices::CrossoverASlope);
    const auto crossoverSlopeB = mParams.GetEnumValue<M7::CrossoverSlope>(ParamIndices::CrossoverBSlope);
    mMidSplitter.SetParams(crossoverFreqA, crossoverSlopeA, crossoverFreqB, crossoverSlopeB);
    mSideSplitter.SetParams(crossoverFreqA, crossoverSlopeA, crossoverFreqB, crossoverSlopeB);
  }

  static void CalculateBandMuteSolo(bool (&mutes)[gBandCount], bool (&solos)[gBandCount], bool (&outputs)[gBandCount])
  {
    bool soloExists = false;
    for (int iBand = 0; iBand < gBandCount; ++iBand)
    {
      if (solos[iBand])
      {
        soloExists = true;
        break;
      }
    }

    for (int iBand = 0; iBand < gBandCount; ++iBand)
    {
      outputs[iBand] = soloExists ? solos[iBand] : !mutes[iBand];
    }
  }

#ifdef MAJ7WIDTH_FULL_FEATURE
  // 2D rotation around origin
  static void Rotate2(float& a, float& b, float rot)
  {
    float s = M7::math::sin(rot);
    float c = M7::math::cos(rot);
    float t = a * c - b * s;
    b = a * s + b * c;
    a = t;
  }

  static void Shear2(float& side, float mid, float shearFactor)
  {
    side += mid * shearFactor;
  }

#endif  // MAJ7WIDTH_FULL_FEATURE

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    const bool multibandEnable = mParams.GetBoolValue(ParamIndices::MultibandEnable);
    const bool leftInvert = mParams.GetBoolValue(ParamIndices::LInvert);
    const bool rightInvert = mParams.GetBoolValue(ParamIndices::RInvert);
    const bool sideHpfActive = mParams.GetRawVal(ParamIndices::SideHPFrequency) > 0.001f;
    const float sideHpfFreq = mParams.GetFrequency(ParamIndices::SideHPFrequency, M7::gFilterFreqConfig);
    const float outputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, gVolumeCfg);

    bool muteSoloEnabled[gBandCount] = {true, true, true};
    bool mutes[gBandCount] = {mBandConfig[0].mMute, mBandConfig[1].mMute, mBandConfig[2].mMute};
    bool solos[gBandCount] = {mBandConfig[0].mSolo, mBandConfig[1].mSolo, mBandConfig[2].mSolo};
    CalculateBandMuteSolo(mutes, solos, muteSoloEnabled);

    for (int iBand = 0; iBand < gBandCount; ++iBand)
    {
      mBands[iBand].mMuteSoloEnable = muteSoloEnabled[iBand];
      mBands[iBand].Slider(sideHpfActive, sideHpfFreq);
    }
#ifdef MAJ7WIDTH_FULL_FEATURE
    // constexpr float bandShearBypassThreshold = 0.001f;
    // const float broadbandShearAngle = mParams.GetScaledRealValue(ParamIndices::MSShear, -gShearAngleLimit, gShearAngleLimit, 0);
    // const float broadbandShearFactor = -M7::math::tan(broadbandShearAngle);
    // const float broadbandRotation = mParams.GetScaledRealValue(ParamIndices::RotationAngle, -gRotationExtent, gRotationExtent, 0);
    // const bool broadbandShearActive = !M7::math::FloatEquals(broadbandShearAngle, 0.0f, bandShearBypassThreshold);
#endif  // MAJ7WIDTH_FULL_FEATURE

    for (size_t i = 0; i < (size_t)numSamples; ++i)
    {
      const M7::FloatPair inputStereo{inputs[0][i], inputs[1][i]};
      const M7::FloatPair inputMs = inputStereo.MSEncode();
      M7::FloatPair stereo{};

      if (multibandEnable)
      {
        const auto sideBands = mSideSplitter.Process(inputMs.Side());
        const auto midBands = mMidSplitter.Process(inputMs.Mid());
        for (int iBand = 0; iBand < gBandCount; ++iBand)
        {
          stereo.Accumulate(
              mBands[iBand].ProcessSample({midBands.s[iBand], sideBands.s[iBand]}, leftInvert, rightInvert));
        }
      }
      else
      {
        stereo = mBands[1].ProcessSample(inputMs, leftInvert, rightInvert);
      }

      auto ms = stereo.MSEncode();

      // #ifdef MAJ7WIDTH_FULL_FEATURE
      //       if (broadbandShearActive)
      //       {
      //         Shear2(ms.x[1], ms.x[0], broadbandShearFactor);
      //       }
      // #endif  // MAJ7WIDTH_FULL_FEATURE

      stereo = ms.MSDecode();

      // #ifdef MAJ7WIDTH_FULL_FEATURE
      //       Rotate2(stereo.x[0], stereo.x[1], broadbandRotation);
      // #endif  // MAJ7WIDTH_FULL_FEATURE

      auto output = stereo * outputGainLin;

      outputs[0][i] = output.Left();
      outputs[1][i] = output.Right();

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
      if (IsGuiVisible())
      {
        mInputAnalysis[0].WriteSample(inputs[0][i]);
        mInputAnalysis[1].WriteSample(inputs[1][i]);
        mOutputAnalysis[0].WriteSample(outputs[0][i]);
        mOutputAnalysis[1].WriteSample(outputs[1][i]);

        // Feed stereo imaging analysis
        mInputImagingAnalysis.WriteStereoSample(inputs[0][i], inputs[1][i]);
        mOutputImagingAnalysis.WriteStereoSample(outputs[0][i], outputs[1][i]);
      }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
    }
  }
};
}  // namespace WaveSabreCore
