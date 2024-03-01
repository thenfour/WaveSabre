#include "ScissorVst.h"
#include "ScissorEditor.h"

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
	return new ScissorVst(audioMaster);
}

ScissorVst::ScissorVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)0, 2, 2, 'Scsr', new Scissor())
{
	setEditor(new ScissorEditor(this));
}

void ScissorVst::getParameterName(VstInt32 index, char *text)
{
	vst_strncpy(text, "unused", kVstMaxParamStrLen);
}

bool ScissorVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Scissor", kVstMaxEffectNameLen);
	return true;
}

bool ScissorVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Scissor", kVstMaxProductStrLen);
	return true;
}
