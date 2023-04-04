#include "ThunderVst.h"
#include "ThunderEditor.h"

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
	return new ThunderVst(audioMaster);
}

ThunderVst::ThunderVst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, 0, 0, 2, 'Tndr', new Thunder(), true)
{
	setEditor(new ThunderEditor(this));
}

bool ThunderVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Thunder", kVstMaxEffectNameLen);
	return true;
}

bool ThunderVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Thunder", kVstMaxProductStrLen);
	return true;
}

Thunder *ThunderVst::GetThunder() const
{
	return (Thunder *)getDevice();
}
