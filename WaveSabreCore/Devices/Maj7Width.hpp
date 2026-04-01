#pragma once

#include "../Analysis/StereoImageAnalysis.hpp"
#include "../Filters/FilterOnePole.hpp"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../WSCore/Device.h"


namespace WaveSabreCore
{
struct Maj7Width : public Device
{
  // roughly in order of processing,
  enum class ParamIndices
  {
    LeftSource,   // 0 = left, 1 = right
    RightSource,  // 0 = left, 1 = right
    LInvert,
    RInvert,
    RotationAngle, // rotates L/R geometrically
    SideHPFrequency,
    MidSideBalance,
    Pan,
    OutputGain,

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
    {"SideHPF"},\
    {"MSBal"},\
    {"Pan"},\
    {"OutGain"},\
  }
// clang-format on

  static constexpr M7::VolumeParamConfig gVolumeCfg{3.9810717055349722f, 12.0f};

  float mParamCache[(int)ParamIndices::NumParams];
  M7::ParamAccessor mParams;

  static_assert((int)ParamIndices::NumParams == 9, "param count probably changed and this needs to be regenerated.");
  static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
      -32767,  // LSrc = -1
      32767,   // RSrc = 1
      0,       // LInv = false
      0,       // RInv = false
      16383,   // Rot = 0.5
      0,       // SideHPF = 0
      0,       // MSBal = 0
      0,       // Pan = 0
      16422,   // OutGain = 0.50118720531463623047
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

  // void RotateMS(M7::FloatPair& ms)
  // {
  //   float mid = ms.Mid();
  //   float side = ms.Side();
  //   Rotate2(side, mid, ParamIndices::MSRotationAngle, -M7::math::gPIQuarter, M7::math::gPIQuarter);
  //   ms.x[0] = mid;
  //   ms.x[1] = side;
  // }

  // interesting but it feels the same as rotation, but preserving L or R power.
  // void ShearMS(M7::FloatPair& ms)
  // {
  //   const float shearAngle =
  //       mParams.GetScaledRealValue(ParamIndices::MSShearAngle, -M7::math::gPIQuarter, M7::math::gPIQuarter, 0);
  //   const float shearFactor = M7::math::tan(shearAngle);
  //   ms.x[1] += ms.x[0] * shearFactor;
  // }
#endif  // MAJ7WIDTH_FULL_FEATURE

  virtual void Run(float** inputs, float** outputs, int numSamples) override
  {
    auto panGains = M7::math::PanToFactor(mParams.GetN11Value(ParamIndices::Pan, 0));
    mFilter.SetParams(M7::FilterCircuit::OnePole,
                      M7::FilterSlope::Slope6dbOct,
                      M7::FilterResponse::Highpass,
                      mParams.GetFrequency(ParamIndices::SideHPFrequency, M7::gFilterFreqConfig),
                      M7::Param01{0} /*reso*/,
                      0 /*gain*/);
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
      ms.x[1] = mFilter.ProcessSample(ms.Side());
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

      auto stereo = ms.MSDecode();

#ifdef MAJ7WIDTH_FULL_FEATURE
      Rotate2(stereo.x[0], stereo.x[1], ParamIndices::RotationAngle, -M7::math::gPIQuarter, M7::math::gPIQuarter);
#endif  // MAJ7WIDTH_FULL_FEATURE

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
