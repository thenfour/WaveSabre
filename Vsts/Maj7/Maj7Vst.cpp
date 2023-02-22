#include "Maj7Vst.h"
#include "Maj7Editor.h"

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

AudioEffect *createEffectInstance(audioMasterCallback audioMaster)
{
	Helpers::Init();
	return new Maj7Vst(audioMaster);
}

Maj7Vst::Maj7Vst(audioMasterCallback audioMaster)
	: VstPlug(audioMaster, (int)Maj7::ParamIndices::NumParams, 0, 2, 'maj7', new Maj7(), true)
{
	setEditor(new Maj7Editor(this));
}

void Maj7Vst::getParameterName(VstInt32 index, char *text)
{
	switch ((Maj7::ParamIndices)index)
	{
	case Maj7::ParamIndices::Master: vst_strncpy(text, "Master", kVstMaxParamStrLen); break;
	}
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

Maj7* Maj7Vst::GetMaj7() const
{
	return (Maj7*)getDevice();
}
