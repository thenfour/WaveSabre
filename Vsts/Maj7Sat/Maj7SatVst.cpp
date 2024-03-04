#include "Maj7SatVst.hpp"
#include "Maj7SatEditor.hpp"

#include <WaveSabreCore.h>

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	*outpSize = 0;
	//WaveSabreCore::Maj7Sat * tmpEffect = new WaveSabreCore::Maj7Sat(); // new for stack preservation
	//Maj7SetVstChunk(nullptr, tmpEffect, inpData, inpSize);

	//M7::OptimizeParams(tmpEffect, true);

	//*outpSize = GetMinifiedChunk(tmpEffect, outpData);
	//delete tmpEffect;
	return *outpSize;
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
	return new Maj7SatVst(audioMaster);
}
