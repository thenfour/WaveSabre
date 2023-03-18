#include "CrusherVst.h"
#include "CrusherEditor.h"

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
	return new CrusherVst(audioMaster);
}

CrusherVst::CrusherVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)Crusher::ParamIndices::NumParams, 2, 2, 'Crsh', new Crusher())
{
	setEditor(new CrusherEditor(this));
}

void CrusherVst::getParameterName(VstInt32 index, char *text)
{
	switch ((Crusher::ParamIndices)index)
	{
	case Crusher::ParamIndices::Vertical: vst_strncpy(text, "Vert", kVstMaxParamStrLen); break;
	case Crusher::ParamIndices::Horizontal: vst_strncpy(text, "Hori", kVstMaxParamStrLen); break;
	case Crusher::ParamIndices::DryWet: vst_strncpy(text, "Dry/Wet", kVstMaxParamStrLen); break;
	}
}

bool CrusherVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Crusher", kVstMaxEffectNameLen);
	return true;
}

bool CrusherVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Crusher", kVstMaxProductStrLen);
	return true;
}
