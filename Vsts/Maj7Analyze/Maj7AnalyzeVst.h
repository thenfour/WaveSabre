#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7AnalyzeVst : public VstPlug
{
public:
  Maj7AnalyzeVst(audioMasterCallback audioMaster)
      : VstPlug(audioMaster, (int)Maj7Analyze::ParamIndices::NumParams, 2, 2, 'M7an', new Maj7Analyze(), false)
  {
    if (audioMaster)
      setEditor(createEditor(this));
  }

  virtual void getParameterName(VstInt32 index, char* text) override
  {
    MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
    vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
  }

  virtual bool getEffectName(char* name) override
  {
    vst_strncpy(name, "Maj7 - Analyze", kVstMaxEffectNameLen);
    return true;
  }

  virtual bool getProductString(char* text) override
  {
    vst_strncpy(text, "Maj7 - Analyze", kVstMaxProductStrLen);
    return true;
  }

  virtual const char* GetJSONTagName()
  {
    return "Maj7Analyze";
  }

  virtual VstInt32 getChunk(void** data, bool isPreset) override
  {
    MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
    return GetSimpleJSONVstChunk(GetJSONTagName(),
                                 data,
                                 GetMaj7Analyze()->mParamCache,
                                 paramNames,
                                 [this](clarinoid::JsonVariantWriter& elem)
                                 {
                                   auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
                                   auto map = editor.GetVstOnlyParams();
                                   for (auto& p : map)
                                   {
                                     p->Serialize(elem);
                                   }
                                 });
  }

  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
  {
    MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
    return SetSimpleJSONVstChunk(GetMaj7Analyze(),
                                 GetJSONTagName(),
                                 data,
                                 byteSize,
                                 GetMaj7Analyze()->mParamCache,
                                 paramNames,
                                 [this](clarinoid::JsonVariantReader& elem)
                                 {
                                   auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
                                   auto map = editor.GetVstOnlyParams();
                                   for (auto& p : map)
                                   {
                                     p->TryDeserialize(elem);
                                   }
                                 });
  }

  Maj7Analyze* GetMaj7Analyze() const
  {
    return (Maj7Analyze*)getDevice();
  }
};
