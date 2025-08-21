#include "Maj7AnalyzeVst.h"
#include "Maj7AnalyzeEditor.h"

#include <WaveSabreCore.h>

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData, int deltaFromDefaults)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<Maj7AnalyzeVst>(deviceName, inpSize, inpData, outpSize, outpData, deltaFromDefaults);
}

void __cdecl WaveSabreFreeChunk(void* p)
{
	M7::Serializer::FreeBuffer(p);
}
int __cdecl WaveSabreTestCompression(int inpSize, void* inpData)
{
	return TestCompression(inpSize, inpData);
}

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new Maj7AnalyzeVst(audioMaster);
}
