#pragma once

#include <WaveSabreVstLib.h>

using namespace WaveSabreVstLib;

class CathedralVst : public VstPlug
{
public:
  CathedralVst(audioMasterCallback audioMaster);

  virtual void getParameterName(VstInt32 index, char* text);

  virtual bool getEffectName(char* name);
  virtual bool getProductString(char* text);
  virtual const char* GetJSONTagName()
  {
    return "Cathedral";
  }

  Cathedral* GetCathedral() const
  {
    return (Cathedral*)getDevice();
  }

  virtual VstInt32 getChunk(void** data, bool isPreset) override
  {
    CATHEDRAL_PARAM_VST_NAMES(paramNames);
    return GetSimpleJSONVstChunk(
        GetJSONTagName(), data, GetCathedral()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
  }

  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
  {
    CATHEDRAL_PARAM_VST_NAMES(paramNames);
    return SetSimpleJSONVstChunk(GetCathedral(),
                                 GetJSONTagName(),
                                 data,
                                 byteSize,
                                 GetCathedral()->mParamCache,
                                 paramNames,
                                 [](clarinoid::JsonVariantReader&) {});
  }

  virtual void GenerateDefaults() override
  {
    auto& params = GetCathedral()->mParams;

    params.Set01Val(Cathedral::ParamIndices::RoomSize, 0.5f);
    params.Set01Val(Cathedral::ParamIndices::Damp, 0.15f);
    params.Set01Val(Cathedral::ParamIndices::Width, 0.9f);

    params.SetFrequencyAssumingNoKeytracking(Cathedral::ParamIndices::LowCutFreq, M7::gFilterFreqConfig, 145);
    params.SetFrequencyAssumingNoKeytracking(Cathedral::ParamIndices::HighCutFreq, M7::gFilterFreqConfig, 5500);

    params.SetDecibels(Cathedral::ParamIndices::DryOut, M7::gVolumeCfg12db, 0.0f);
    params.SetDecibels(Cathedral::ParamIndices::WetOut, M7::gVolumeCfg12db, -6.0f);

    params.Set01Val(Cathedral::ParamIndices::PreDelay, 0);
  }
};
