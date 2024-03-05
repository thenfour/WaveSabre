#include "Maj7SatVst.hpp"
#include "Maj7SatEditor.hpp"

#include <WaveSabreCore.h>

// accepts the VST chunk, optimizes & minifies and outputs the wavesabre optimized chunk.
int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<Maj7SatVst>(deviceName, inpSize, inpData, outpSize, outpData);
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
	return new Maj7SatVst(audioMaster);
}
