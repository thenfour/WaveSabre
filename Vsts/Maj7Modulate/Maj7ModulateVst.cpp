#include "Maj7ModulateVst.hpp"
#include "Maj7ModulateEditor.hpp"

#include <WaveSabreCore.h>

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData, int deltaFromDefaults)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<Maj7ModulateVst>(deviceName, inpSize, inpData, outpSize, outpData, deltaFromDefaults);
}

void __cdecl WaveSabreFreeChunk(void* p)
{
	FreeBuffer(p);
}
int __cdecl WaveSabreTestCompression(int inpSize, void* inpData)
{
	return TestCompression(inpSize, inpData);
}

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new Maj7ModulateVst(audioMaster);
}

WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect)
{
	return new Maj7ModulateEditor(audioEffect);
}
