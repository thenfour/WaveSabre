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
  static constexpr auto gShearAngleLimit = M7::math::gPIQuarter;

  struct BandConfig
  {
    bool mMute = false;
    bool mSolo = false;
  };

  // roughly in order of processing,
  enum class ParamIndices
  {
    LeftSource,   // 0 = left, 1 = right
    RightSource,  // 0 = left, 1 = right
    LInvert,
    RInvert,
    RotationAngle,  // rotates L/R geometrically
    MSShear,
    SideHPFrequency,
    MidSideBalance,
    Pan,
    OutputGain,
    CrossoverAFrequency,
    CrossoverBFrequency,
    CrossoverASlope,
    CrossoverBSlope,
    Band1Gain,
    Band2Gain,
    Band3Gain,
    Band1Rotation,
    Band2Rotation,
    Band3Rotation,
    Band1Shear,
    Band2Shear,
    Band3Shear,
    Asymmetry,

    NumParams,
  };

  // clang-format off
// NB: max 8 chars per string.
#define MAJ7WIDTH_PARAM_VST_NAMES(symbolName)                                                                          \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Width::ParamIndices::NumParams]              \
  {                                                                                                                    \
    {"LSrc"},\
    {"RSrc"},\
    {"LInv"},\
    {"RInv"},\
    {"Rot"},\
    {"MSShear"},\
    {"SideHPF"},\
    {"MSBal"},\
    {"Pan"},\
    {"OutGain"},\
    {"xAFreq"},\
    {"xBFreq"},\
    {"xASlope"},\
    {"xBSlope"},\
    {"B1Gain"},\
    {"B2Gain"},\
    {"B3Gain"},\
    {"MS1Rot"},\
    {"MS2Rot"},\
    {"MS3Rot"},\
    {"MS1Shr"},\
    {"MS2Shr"},\
    {"MS3Shr"},\
    {"Asym"},\
  }
  // clang-format on

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams;
  M7::BandSplitter mMidSplitter;
  M7::BandSplitter mSideSplitter;
  BandConfig mBandConfig[gBandCount];

    static_assert((int)ParamIndices::NumParams == 24, "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
      -32767,  // LSrc = -1
      32767,   // RSrc = 1
      0,       // LInv = 0
      0,       // RInv = 0
      16383,   // Rot = 0.5
      16383,   // MSShear = 0.5
      0,       // SideHPF = 0
      0,       // MSBal = 0
      0,       // Pan = 0
      16422,   // OutGain = 0.50118720531463623047
      14347,   // xAFreq = 0.43785116076469421387
      22305,   // xBFreq = 0.68073546886444091797
      3,       // xASlope = 0.0001220703125
      3,       // xBSlope = 0.0001220703125
      16422,   // B1Gain = 0.50118720531463623047
      16422,   // B2Gain = 0.50118720531463623047
      16422,   // B3Gain = 0.50118720531463623047
      16383,   // MS1Rot = 0.5
      16383,   // MS2Rot = 0.5
      16383,   // MS3Rot = 0.5
      16383,   // MS1Shr = 0.5
      16383,   // MS2Shr = 0.5
      16383,   // MS3Shr = 0.5
        0,       // Asym = 0
  };

  M7::MoogOnePoleFilter mFilter;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  AnalysisStream mInputAnalysis[2];
  AnalysisStream mOutputAnalysis[2];
  StereoImagingAnalysisStream mInputImagingAnalysis;
  StereoImagingAnalysisStream mOutputImagingAnalysis;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  Maj7Width()
      : Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults)
      , mParams(mParamCache, 0)
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

#ifdef MAJ7WIDTH_FULL_FEATURE
  // 2D rotation around origin
  void Rotate2(float& a, float& b, ParamIndices param, float minAngle, float maxAngle)
  {
    float rot = mParams.GetScaledRealValue(param, minAngle, maxAngle, 0);
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

#endif  // MAJ7WIDTH_FULL_FEATURE

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    const float asymmetry = mParams.GetN11Value(ParamIndices::Asymmetry, 0);
    auto panGains = M7::math::PanToFactor(mParams.GetN11Value(ParamIndices::Pan, 0));
    const float sideBandGains[gBandCount] = {
        mParams.GetLinearVolume(ParamIndices::Band1Gain, gVolumeCfg),
        mParams.GetLinearVolume(ParamIndices::Band2Gain, gVolumeCfg),
        mParams.GetLinearVolume(ParamIndices::Band3Gain, gVolumeCfg),
    };
    constexpr float bandGainBypassThresholdLin = 0.005f;
    constexpr float bandRotationBypassThreshold = 0.001f;
    constexpr float bandShearBypassThreshold = 0.001f;
    bool muteSoloEnabled[gBandCount] = {true, true, true};
    const bool bandRoutingActive =
        mBandConfig[0].mMute || mBandConfig[0].mSolo || mBandConfig[1].mMute || mBandConfig[1].mSolo ||
        mBandConfig[2].mMute || mBandConfig[2].mSolo;
    if (bandRoutingActive)
    {
      bool mutes[gBandCount] = {mBandConfig[0].mMute, mBandConfig[1].mMute, mBandConfig[2].mMute};
      bool solos[gBandCount] = {mBandConfig[0].mSolo, mBandConfig[1].mSolo, mBandConfig[2].mSolo};
      CalculateBandMuteSolo(mutes, solos, muteSoloEnabled);
    }
  #ifdef MAJ7WIDTH_FULL_FEATURE
    const float broadbandShearAngle = mParams.GetScaledRealValue(ParamIndices::MSShear, -gShearAngleLimit, gShearAngleLimit, 0);
    const float broadbandShearFactor = -M7::math::tan(broadbandShearAngle);
    const float bandRotations[gBandCount] = {
      mParams.GetScaledRealValue(ParamIndices::Band1Rotation, -gRotationExtent, gRotationExtent, 0),
      mParams.GetScaledRealValue(ParamIndices::Band2Rotation, -gRotationExtent, gRotationExtent, 0),
      mParams.GetScaledRealValue(ParamIndices::Band3Rotation, -gRotationExtent, gRotationExtent, 0),
    };
    const float bandShearAngles[gBandCount] = {
      mParams.GetScaledRealValue(ParamIndices::Band1Shear, -gShearAngleLimit, gShearAngleLimit, 0),
      mParams.GetScaledRealValue(ParamIndices::Band2Shear, -gShearAngleLimit, gShearAngleLimit, 0),
      mParams.GetScaledRealValue(ParamIndices::Band3Shear, -gShearAngleLimit, gShearAngleLimit, 0),
    };
    const float bandShearFactors[gBandCount] = {
      -M7::math::tan(bandShearAngles[0]),
      -M7::math::tan(bandShearAngles[1]),
      -M7::math::tan(bandShearAngles[2]),
    };
    const bool broadbandShearActive = !M7::math::FloatEquals(broadbandShearAngle, 0.0f, bandShearBypassThreshold);
  #endif  // MAJ7WIDTH_FULL_FEATURE
    const bool multibandActive = !M7::math::FloatEquals(sideBandGains[0], 1.0f, bandGainBypassThresholdLin) ||
                                 !M7::math::FloatEquals(sideBandGains[1], 1.0f, bandGainBypassThresholdLin) ||
                   !M7::math::FloatEquals(sideBandGains[2], 1.0f, bandGainBypassThresholdLin) || bandRoutingActive
  #ifdef MAJ7WIDTH_FULL_FEATURE
                   || !M7::math::FloatEquals(bandRotations[0], 0.0f, bandRotationBypassThreshold) ||
                   !M7::math::FloatEquals(bandRotations[1], 0.0f, bandRotationBypassThreshold) ||
                   !M7::math::FloatEquals(bandRotations[2], 0.0f, bandRotationBypassThreshold) ||
                   !M7::math::FloatEquals(bandShearAngles[0], 0.0f, bandShearBypassThreshold) ||
                   !M7::math::FloatEquals(bandShearAngles[1], 0.0f, bandShearBypassThreshold) ||
                   !M7::math::FloatEquals(bandShearAngles[2], 0.0f, bandShearBypassThreshold)
  #endif  // MAJ7WIDTH_FULL_FEATURE
      ;
    const bool sideHpfActive = mParams.GetRawVal(ParamIndices::SideHPFrequency) > 0.001f;
    if (sideHpfActive)
    {
      mFilter.SetParams(M7::FilterCircuit::OnePole,
                        M7::FilterSlope::Slope6dbOct,
                        M7::FilterResponse::Highpass,
                        mParams.GetFrequency(ParamIndices::SideHPFrequency, M7::gFilterFreqConfig),
                        M7::Param01{0} /*reso*/,
                        0 /*gain*/);
    }
    float masterLinearGain = mParams.GetLinearVolume(ParamIndices::OutputGain, gVolumeCfg) *
                             M7::math::gPanCompensationGainLin;

    for (size_t i = 0; i < (size_t)numSamples; ++i)
    {
      float l = inputs[0][i];
      float r = inputs[1][i];

      float leftSrcN11 = mParams.GetN11Value(ParamIndices::LeftSource, 0);
      float rightSrcN11 = mParams.GetN11Value(ParamIndices::RightSource, 0);

      float left = M7::math::lerp(l, r, leftSrcN11 * 0.5f + 0.5f);    // rescale from -1,1 to 0,1 for lerp
      float right = M7::math::lerp(l, r, rightSrcN11 * 0.5f + 0.5f);  // rescale from -1,1 to 0,1 for lerp

      if (mParams.GetBoolValue(ParamIndices::LInvert))
      {
        left = -left;
      }
      if (mParams.GetBoolValue(ParamIndices::RInvert))
      {
        right = -right;
      }

      auto ms = M7::FloatPair{left, right}.MSEncode();

      // Width-shaping stage: side-only HPF and width amount both define the M/S image
      // before any later geometric rotations or shearing are applied.
      if (sideHpfActive)
      {
        ms.x[1] = mFilter.ProcessSample(ms.Side());
      }
      float width = mParams.GetN11Value(ParamIndices::MidSideBalance, 0);
      if (width < 0)
      {
        // reduce side toward mono
        ms.x[1] *= width + 1.0f;
      }
      if (width > 0)
      {
        // reduce mid toward side-only
        ms.x[0] *= 1.0f - width;
      }

      if (multibandActive)
      {
        const auto sideBands = mSideSplitter.Process(ms.Side());
        const auto midBands = mMidSplitter.Process(ms.Mid());

        M7::FloatPair multibandMs{};
        for (int iBand = 0; iBand < gBandCount; ++iBand)
        {
          if (!muteSoloEnabled[iBand])
          {
            continue;
          }

          float bandMid = midBands[iBand];
          float bandSide = sideBands[iBand] * sideBandGains[iBand];
#ifdef MAJ7WIDTH_FULL_FEATURE
          Shear2(bandSide, bandMid, bandShearFactors[iBand]);
          Rotate2(bandSide, bandMid, (ParamIndices)((int)ParamIndices::Band1Rotation + iBand), -gRotationExtent, gRotationExtent);
#endif  // MAJ7WIDTH_FULL_FEATURE
          multibandMs.x[0] += bandMid;
          multibandMs.x[1] += bandSide;
        }
        ms = multibandMs;
      }

#ifdef MAJ7WIDTH_FULL_FEATURE
      if (broadbandShearActive)
      {
        Shear2(ms.x[1], ms.x[0], broadbandShearFactor);
      }
#endif  // MAJ7WIDTH_FULL_FEATURE

      auto stereo = ms.MSDecode();

#ifdef MAJ7WIDTH_FULL_FEATURE
      Rotate2(stereo.x[0], stereo.x[1], ParamIndices::RotationAngle, -gRotationExtent, gRotationExtent);
#endif  // MAJ7WIDTH_FULL_FEATURE

      if (asymmetry < 0.0f)
      {
        stereo.x[1] = M7::math::lerp(stereo.x[1], stereo.x[0], -asymmetry);
      }
      else if (asymmetry > 0.0f)
      {
        stereo.x[0] = M7::math::lerp(stereo.x[0], stereo.x[1], asymmetry);
      }

      ms = stereo.MSEncode();

      auto outputPrePan = ms.MSDecode() * masterLinearGain;
      auto output = outputPrePan.mul(panGains);

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
