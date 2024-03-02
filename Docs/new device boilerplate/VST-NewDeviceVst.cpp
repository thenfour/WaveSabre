#include "Maj7CompVst.hpp"
#include "Maj7CompEditor.hpp"

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
	return new Maj7CompVst(audioMaster);
}
