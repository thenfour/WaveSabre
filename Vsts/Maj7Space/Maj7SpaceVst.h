#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7SpaceVst : public VstPlug
{
public:
  Maj7SpaceVst(audioMasterCallback audioMaster)
      : VstPlug(audioMaster, (int)Maj7Space::ParamIndices::NumParams, 2, 2, 'M7sp', new Maj7Space(), false)
  {
    if (audioMaster)
      setEditor(createEditor(this));
  }

  virtual void getParameterName(VstInt32 index, char* text) override
  {
    MAJ7SPACE_PARAM_VST_NAMES(paramNames);
    vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
  }

  virtual bool getEffectName(char* name) override
  {
    vst_strncpy(name, "Maj7 - Space", kVstMaxEffectNameLen);
    return true;
  }

  virtual bool getProductString(char* text) override
  {
    vst_strncpy(text, "Maj7 - Space", kVstMaxProductStrLen);
    return true;
  }

  virtual const char* GetJSONTagName()
  {
    return "Maj7Space";
  }

  virtual VstInt32 getChunk(void** data, bool isPreset) override
  {
    MAJ7SPACE_PARAM_VST_NAMES(paramNames);
    return GetSimpleJSONVstChunk(
        GetJSONTagName(), data, GetMaj7Space()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
  }

  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
  {
    MAJ7SPACE_PARAM_VST_NAMES(paramNames);
    return SetSimpleJSONVstChunk(GetMaj7Space(),
                                 GetJSONTagName(),
                                 data,
                                 byteSize,
                                 GetMaj7Space()->mParamCache,
                                 paramNames,
                                 [](clarinoid::JsonVariantReader&) {});
  }

  Maj7Space* GetMaj7Space() const
  {
    return (Maj7Space*)getDevice();
  }

  virtual void OptimizeParams() override
  {
    using Params = Maj7Space::ParamIndices;
    M7::ParamAccessor defaults{mDefaultParamCache.data(), 0};
    M7::ParamAccessor p{GetMaj7Space()->mParamCache, 0};

    if (!p.GetBoolValue(Params::DelayEnabled))
    {
      p.SetRawVal(Params::DelayLeftDelayCoarse, defaults.GetRawVal(Params::DelayLeftDelayCoarse));
      p.SetRawVal(Params::DelayLeftDelayFine, defaults.GetRawVal(Params::DelayLeftDelayFine));
      p.SetRawVal(Params::DelayLeftDelayMS, defaults.GetRawVal(Params::DelayLeftDelayMS));
      p.SetRawVal(Params::DelayRightDelayCoarse, defaults.GetRawVal(Params::DelayRightDelayCoarse));
      p.SetRawVal(Params::DelayRightDelayFine, defaults.GetRawVal(Params::DelayRightDelayFine));
      p.SetRawVal(Params::DelayRightDelayMS, defaults.GetRawVal(Params::DelayRightDelayMS));
      p.SetRawVal(Params::DelayLowCutFreq, defaults.GetRawVal(Params::DelayLowCutFreq));
      p.SetRawVal(Params::DelayLowCutQ, defaults.GetRawVal(Params::DelayLowCutQ));
      p.SetRawVal(Params::DelayHighCutFreq, defaults.GetRawVal(Params::DelayHighCutFreq));
      p.SetRawVal(Params::DelayHighCutQ, defaults.GetRawVal(Params::DelayHighCutQ));
      p.SetRawVal(Params::DelayFeedbackLevel, defaults.GetRawVal(Params::DelayFeedbackLevel));
      p.SetRawVal(Params::DelayFeedbackDriveDB, defaults.GetRawVal(Params::DelayFeedbackDriveDB));
      p.SetRawVal(Params::DelayCross, defaults.GetRawVal(Params::DelayCross));
      p.SetRawVal(Params::DelayVolume, defaults.GetRawVal(Params::DelayVolume));
    }
    if (!p.GetBoolValue(Params::ReverbEnabled))
    {
      p.SetRawVal(Params::ReverbRoomSize, defaults.GetRawVal(Params::ReverbRoomSize));
      p.SetRawVal(Params::ReverbDamp, defaults.GetRawVal(Params::ReverbDamp));
      p.SetRawVal(Params::ReverbWidth, defaults.GetRawVal(Params::ReverbWidth));
      p.SetRawVal(Params::ReverbLowCutFreq, defaults.GetRawVal(Params::ReverbLowCutFreq));
      p.SetRawVal(Params::ReverbHighCutFreq, defaults.GetRawVal(Params::ReverbHighCutFreq));
      p.SetRawVal(Params::ReverbPreDelay, defaults.GetRawVal(Params::ReverbPreDelay));
      p.SetRawVal(Params::ReverbVolume, defaults.GetRawVal(Params::ReverbVolume));
    }
  }

  virtual void GenerateDefaults() override
  {
    auto& p = GetMaj7Space()->mParams;
    using Params = Maj7Space::ParamIndices;

    p.SetIntValue(Params::DelayLeftDelayCoarse, 3);
    p.SetN11Value(Params::DelayLeftDelayFine, 0);
    p.SetBipolarPowCurvedValue(Params::DelayLeftDelayMS, M7::gEnvTimeCfg, 0.0f);

    p.SetIntValue(Params::DelayRightDelayCoarse, 4);
    p.SetN11Value(Params::DelayRightDelayFine, 0);
    p.SetBipolarPowCurvedValue(Params::DelayRightDelayMS, M7::gEnvTimeCfg, 0.0f);

    p.SetFrequencyAssumingNoKeytracking(Params::DelayLowCutFreq, M7::gFilterFreqConfig, 50.0f);
    p.SetDivCurvedValue(Params::DelayLowCutQ, M7::gBiquadFilterQCfg, 0.75f);
    p.SetFrequencyAssumingNoKeytracking(Params::DelayHighCutFreq, M7::gFilterFreqConfig, 8500.0f);
    p.SetDivCurvedValue(Params::DelayHighCutQ, M7::gBiquadFilterQCfg, 0.75f);

    p.SetDecibels(Params::DelayFeedbackLevel, M7::gVolumeCfg6db, -15.0f);
    p.SetDecibels(Params::DelayFeedbackDriveDB, M7::gVolumeCfg36db, 3.0f);
    p.Set01Val(Params::DelayCross, 0.25f);

    p.Set01Val(Params::ReverbRoomSize, 0.5f);
    p.Set01Val(Params::ReverbDamp, 0.15f);
    p.Set01Val(Params::ReverbWidth, 0.9f);
    p.SetFrequencyAssumingNoKeytracking(Params::ReverbLowCutFreq, M7::gFilterFreqConfig, 145.0f);
    p.SetFrequencyAssumingNoKeytracking(Params::ReverbHighCutFreq, M7::gFilterFreqConfig, 5500.0f);
    p.SetRangedValue(Params::ReverbPreDelay, 0.0f, 500.0f, 1.0f);

    p.SetBoolValue(Params::DelayEnabled, true);
    p.SetBoolValue(Params::ReverbEnabled, true);

    p.SetDecibels(Params::DryVolume, M7::gVolumeCfg12db, 0.0f);
    p.SetDecibels(Params::DelayVolume, M7::gVolumeCfg12db, -12.0f);
    p.SetDecibels(Params::ReverbVolume, M7::gVolumeCfg12db, -12.0f);
  }
};
