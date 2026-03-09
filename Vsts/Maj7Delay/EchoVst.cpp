#include "EchoVst.h"
#include "EchoEditor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData, int deltaFromDefaults)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<EchoVst>(deviceName, inpSize, inpData, outpSize, outpData, deltaFromDefaults);
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
	return new EchoVst(audioMaster);
}

EchoVst::EchoVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)Echo::ParamIndices::NumParams, 2, 2, 'Echo', new Echo())
{
	if (audioMaster)
		setEditor(new EchoEditor(this));
}

void EchoVst::getParameterName(VstInt32 index, char *text)
{
	ECHO_PARAM_VST_NAMES(paramNames);
	vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
}

bool EchoVst::getEffectName(char *name)
{
	vst_strncpy(name, "Maj7 - Delay", kVstMaxEffectNameLen);
	return true;
}

bool EchoVst::getProductString(char *text)
{
	vst_strncpy(text, "Maj7 - Delay", kVstMaxProductStrLen);
	return true;
}
