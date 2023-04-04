#include "LevellerVst.h"
#include "LevellerEditor.h"

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
	return new LevellerVst(audioMaster);
}

LevellerVst::LevellerVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)LevellerParamIndices::NumParams, 2, 2, 'Lvlr', new Leveller())
{
	setEditor(new LevellerEditor(this));
}

void LevellerVst::getParameterName(VstInt32 index, char *text)
{
	using vstn = const char[kVstMaxParamStrLen];
	static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = LEVELLER_PARAM_VST_NAMES;
	vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
}

bool LevellerVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Leveller", kVstMaxEffectNameLen);
	return true;
}

bool LevellerVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Leveller", kVstMaxProductStrLen);
	return true;
}
