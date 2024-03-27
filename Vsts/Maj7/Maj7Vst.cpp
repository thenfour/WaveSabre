
#include "Maj7Vst.h"
#include "Maj7Editor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	return new Maj7Vst(audioMaster);
}

Maj7Vst::Maj7Vst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)M7::ParamIndices::NumParams, 0, 2, 'maj7', new M7::Maj7(), true)
{
	SetDefaultSettings();
	if (audioMaster)
		setEditor(new Maj7Editor(this));
}

void Maj7Vst::getParameterName(VstInt32 index, char* text)
{
	using vstn = const char[kVstMaxParamStrLen];
	static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

	vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
}

bool Maj7Vst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Maj7", kVstMaxEffectNameLen);
	return true;
}

bool Maj7Vst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Maj7", kVstMaxProductStrLen);
	return true;
}

M7::Maj7* Maj7Vst::GetMaj7() const
{
	return (M7::Maj7*)getDevice();
}

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char *deviceName, int inpSize, void* inpData, int* outpSize, void** outpData, int deltaFromDefaults)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<Maj7Vst>(deviceName, inpSize, inpData, outpSize, outpData, deltaFromDefaults);
}

void __cdecl WaveSabreFreeChunk(void* p)
{
	M7::Serializer::FreeBuffer(p);
}

// compresses the given data to test Squishy performance for a given buf.
// just returns the compressed size
int __cdecl WaveSabreTestCompression(int inpSize, void* inpData)
{
	return TestCompression(inpSize, inpData);
}

