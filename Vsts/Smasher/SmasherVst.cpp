#include "SmasherVst.h"
#include "SmasherEditor.h"

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
int __cdecl WaveSabreTestCompression(int inpSize, void* inpData)
{
	// implemented in maj7.dll
	return 0;
}


AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new SmasherVst(audioMaster);
}

SmasherVst::SmasherVst(audioMasterCallback audioMaster) :
	VstPlug(audioMaster, (int)Smasher::ParamIndices::NumParams, 4, 2, 'Smsh', new Smasher()),
	mpSmasher(static_cast<Smasher*>(this->getDevice()))
{
	setEditor(new SmasherEditor(this));
}

void SmasherVst::getParameterName(VstInt32 index, char *text)
{
	switch ((Smasher::ParamIndices)index)
	{
	case Smasher::ParamIndices::Sidechain: vst_strncpy(text, "Sidchain", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::InputGain: vst_strncpy(text, "In Gain", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::Threshold: vst_strncpy(text, "Thres", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::Ratio: vst_strncpy(text, "Ratio", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::Attack: vst_strncpy(text, "Attack", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::Release: vst_strncpy(text, "Release", kVstMaxParamStrLen); break;
	case Smasher::ParamIndices::OutputGain: vst_strncpy(text, "Out Gain", kVstMaxParamStrLen); break;
	}
}

bool SmasherVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Smasher", kVstMaxEffectNameLen);
	return true;
}

bool SmasherVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Smasher", kVstMaxProductStrLen);
	return true;
}
