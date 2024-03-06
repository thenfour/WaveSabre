#include "CathedralVst.h"
#include "CathedralEditor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

// no conversion required
int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<CathedralVst>(deviceName, inpSize, inpData, outpSize, outpData);
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
	return new CathedralVst(audioMaster);
}

CathedralVst::CathedralVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)Cathedral::ParamIndices::NumParams, 2, 2, 'Cath', new Cathedral())
{
	if (audioMaster)
		setEditor(new CathedralEditor(this));
}

void CathedralVst::getParameterName(VstInt32 index, char *text)
{
	switch ((Cathedral::ParamIndices)index)
	{
	case Cathedral::ParamIndices::Freeze: vst_strncpy(text, "Freeze", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::RoomSize: vst_strncpy(text, "Roomsize", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::Damp: vst_strncpy(text, "Damp", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::Width: vst_strncpy(text, "Width", kVstMaxParamStrLen); break; 
	case Cathedral::ParamIndices::LowCutFreq: vst_strncpy(text, "LC Freq", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::HighCutFreq: vst_strncpy(text, "HC Freq", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::DryWet: vst_strncpy(text, "Dry/Wet", kVstMaxParamStrLen); break;
	case Cathedral::ParamIndices::PreDelay: vst_strncpy(text, "Pre Dly", kVstMaxParamStrLen); break;
	}
}

bool CathedralVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Cathedral", kVstMaxEffectNameLen);
	return true;
}

bool CathedralVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Cathedral", kVstMaxProductStrLen);
	return true;
}
