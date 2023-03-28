#include "Maj7WidthVst.h"
#include "Maj7WidthEditor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

// no conversion required
int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	*outpSize = 0;
	return 0;
}

void __cdecl WaveSabreFreeChunk(void* p)
{
	M7::Serializer::FreeBuffer(p);
}

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new Maj7WidthVst(audioMaster);
}

Maj7WidthVst::Maj7WidthVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)Maj7Width::ParamIndices::NumParams, 2, 2, 'M7wd', new Maj7Width(), false)
{
	setEditor(new Maj7WidthEditor(this));
}

void Maj7WidthVst::getParameterName(VstInt32 index, char *text)
{
	switch ((Maj7Width::ParamIndices)index)
	{
	case Maj7Width::ParamIndices::Width: vst_strncpy(text, "Width", kVstMaxParamStrLen); break;
	case Maj7Width::ParamIndices::SideHPFrequency: vst_strncpy(text, "SideHPF", kVstMaxParamStrLen); break;
	case Maj7Width::ParamIndices::Pan: vst_strncpy(text, "Pan", kVstMaxParamStrLen); break;
	case Maj7Width::ParamIndices::OutputGain: vst_strncpy(text, "OutGain", kVstMaxParamStrLen); break;
	}
}



bool Maj7WidthVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Maj7 Width", kVstMaxEffectNameLen);
	return true;
}

bool Maj7WidthVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Maj7 Width", kVstMaxProductStrLen);
	return true;
}

Maj7Width* Maj7WidthVst::GetMaj7Width() const
{
	return (Maj7Width*)getDevice();
}
