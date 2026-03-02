#ifndef __WAVESABREVSTLIB_MAJ7PARAMCONVERTERS_H__
#define __WAVESABREVSTLIB_MAJ7PARAMCONVERTERS_H__

#include "Common.h"
#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <imgui-knobs.h>
#include <string>

using namespace WaveSabreCore;

namespace WaveSabreVstLib
{

// Utility function used by parameter converters
inline std::string FloatToString(float x, const char* suffix = nullptr)
{
  if (!suffix)
    suffix = "";
  char s[100] = {0};
  if (x >= 1000)
  {
    sprintf_s(s, "%0.0f%s", x, suffix);  // 3333
  }
  else if (x >= 100)
  {
    sprintf_s(s, "%0.1f%s", x, suffix);  // 333.2
  }
  else if (x >= 10)
  {
    sprintf_s(s, "%0.2f%s", x, suffix);  // 12.34
  }
  else
  {
    sprintf_s(s, "%0.3f%s", x, suffix);  // 12.34
  }
  return s;
}

static inline std::string midiNoteToString(int midiNote)
{
  static constexpr char const* const noteNames[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  int note = midiNote % 12;
  int octave = midiNote / 12 - 1;
  return std::string(noteNames[note]) + std::to_string(octave) + " (" + std::to_string(midiNote) + ")";
}

struct PowCurvedConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParam{&mBacking, 0};
  const M7::PowCurvedParamCfg& mConfig;

  PowCurvedConverter(const M7::PowCurvedParamCfg& cfg)
      : mConfig(cfg)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    M7::real_t ms = mParam.GetPowCurvedValue(0, mConfig, 0);
    return FloatToString(ms);
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetPowCurvedValue(mConfig, float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct BipolarPowCurvedConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParam{&mBacking, 0};
  const M7::PowCurvedParamCfg& mConfig;

  BipolarPowCurvedConverter(const M7::PowCurvedParamCfg& cfg)
      : mConfig(cfg)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    M7::real_t ms = mParam.GetBipolarPowCurvedValue(0, mConfig, 0);
    return FloatToString(ms);
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetBipolarPowCurvedValue(mConfig, float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct DivCurvedConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParam{&mBacking, 0};
  const M7::DivCurvedParamCfg& mConfig;

  DivCurvedConverter(const M7::DivCurvedParamCfg& cfg)
      : mConfig(cfg)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    M7::real_t v = mParam.GetDivCurvedValue(0, mConfig, 0);
    return FloatToString(v);
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetDivCurvedValue(mConfig, float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct M7VolumeConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParam{&mBacking, 0};
  M7::VolumeParamConfig mCfg;

  M7VolumeConverter(const M7::VolumeParamConfig& cfg)
      : mCfg(cfg)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};

    if (mParam.IsSilentVolume(0, mCfg))
    {
      return "-inf";
    }
    float db = mParam.GetDecibels(0, mCfg);
    if (inputContinuity)
    {
      sprintf_s(s, "%0.2f", db);
    }
    else
    {
      sprintf_s(s, "%c%0.2fdB", db < 0 ? '-' : '+', ::fabsf(db));
    }
    return s;
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetVolumeDB(mCfg, float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct ScaledFloatConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};
  float mMin;
  float mMax;

  ScaledFloatConverter(float v_min, float v_max)
      : mMin(v_min)
      , mMax(v_max)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    sprintf_s(s, "%0.2f", mParams.GetScaledRealValue(0, mMin, mMax, 0));
    return s;
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetScaledValue(mMin, mMax, float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct Maj7IntConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};
  const M7::IntParamConfig mCfg;

  Maj7IntConverter(const M7::IntParamConfig& cfg)
      : mCfg(cfg)
  {
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    int val = mParams.GetIntValue(0, mCfg);
    char s[100] = {0};
    sprintf_s(s, "%d", val);
    return s;
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      int d = std::stoi(s);
      M7::QuickParam p;
      p.SetIntValue(mCfg, d);
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct Maj7MidiNoteConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    return midiNoteToString(mParams.GetIntValue(0, M7::gKeyRangeCfg));
  }
};

// Forward declaration
class VstEditor;

struct Maj7FrequencyConverter : ImGuiKnobs::IValueConverter
{
  float mBacking[2]{0, 0};  // freq, kt
  //VstInt32 mKTParamID;
  M7::ParamAccessor mParams{mBacking, 0};
  const M7::FreqParamConfig mCfg;
  std::function<bool()> mGetKeytracking;

  Maj7FrequencyConverter(M7::FreqParamConfig cfg, std::function<bool()>&& getKeytracking)
      : mCfg(cfg)
      , mGetKeytracking(getKeytracking)
  {
  }

  bool InvokeGetKeytracking()
  {
    if (mGetKeytracking)
      return mGetKeytracking();
    return false;
  }

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    char s[100] = {0};
    mBacking[0] = (float)param;

    if (inputContinuity)
    {
      if (!InvokeGetKeytracking())
      {
        float hz = mParams.GetFrequency(0, 1, mCfg, 0, 0);
        return FloatToString(hz);
      }
      // with KT applied, the frequency doesn't really matter.
      return FloatToString(mBacking[0] * 100);
    }

    if (!InvokeGetKeytracking())
    {
      float hz = mParams.GetFrequency(0, 1, mCfg, 0, 0);
      return FloatToString(hz, "Hz");
    }
    // with KT applied, the frequency doesn't really matter.
    return FloatToString(mBacking[0] * 100, "%");
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      if (InvokeGetKeytracking())
      {
        return {std::stod(s) / 100, true};  // Hz directly.
      }
      float hz = float(std::stod(s));  // percent scaled by 100
      M7::QuickParam p;
      p.SetFrequencyAssumingNoKeytracking(mCfg, hz);
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct FloatN11Converter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    float n11 = mParams.GetN11Value(0, 0);
    char s[100] = {0};
    sprintf_s(s, "%0.3f", n11);
    return s;
  }

  virtual std::pair<ImGuiKnobs::variant, bool> DisplayValueToParam(const std::string& s, void* capture) override
  {
    try
    {
      double d = std::stod(s);
      M7::QuickParam p;
      p.SetN11Value(float(d));
      return {p.GetRawValue(), true};
    }
    catch (const std::invalid_argument&)
    {
      return {0, false};
    }
    catch (const std::out_of_range&)
    {
      return {0, false};
    }
  }
};

struct Float01Converter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    sprintf_s(s, "%0.3f", mParams.Get01Value(0));
    return s;
  }
};

struct FMValueConverter : ImGuiKnobs::IValueConverter
{
  float mBacking;
  M7::ParamAccessor mParams{&mBacking, 0};

  virtual std::string ParamToDisplayString(double param, void* capture, bool inputContinuity) override
  {
    mBacking = (float)param;
    char s[100] = {0};
    sprintf_s(s, "%d", int(mParams.Get01Value(0) * 100));
    return s;
  }
};

}  // namespace WaveSabreVstLib

#endif
