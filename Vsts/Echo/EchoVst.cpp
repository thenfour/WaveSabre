#include "EchoVst.h"
#include "EchoEditor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	return WaveSabreDeviceVSTChunkToMinifiedChunk_Impl<EchoVst>(deviceName, inpSize, inpData, outpSize, outpData);
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

	//switch ((Echo::ParamIndices)index)
	//{
	//case Echo::ParamIndices::LeftDelayCoarse: vst_strncpy(text, "LDly Crs", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::LeftDelayFine: vst_strncpy(text, "LDly Fin", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::RightDelayCoarse: vst_strncpy(text, "RDly Crs", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::RightDelayFine: vst_strncpy(text, "RDly Fin", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::LowCutFreq: vst_strncpy(text, "LC Freq", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::HighCutFreq: vst_strncpy(text, "HC Freq", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::Feedback: vst_strncpy(text, "Feedback", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::Cross: vst_strncpy(text, "Cross", kVstMaxParamStrLen); break;
	//case Echo::ParamIndices::DryWet: vst_strncpy(text, "Dry/Wet", kVstMaxParamStrLen); break;
	//}
}

bool EchoVst::getEffectName(char *name)
{
	vst_strncpy(name, "WaveSabre - Echo", kVstMaxEffectNameLen);
	return true;
}

bool EchoVst::getProductString(char *text)
{
	vst_strncpy(text, "WaveSabre - Echo", kVstMaxProductStrLen);
	return true;
}
