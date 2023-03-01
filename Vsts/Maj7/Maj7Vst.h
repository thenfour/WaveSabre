
#pragma once

#include <WaveSabreVstLib.h>
//using namespace WaveSabreVstLib;
//
//#include <WaveSabreCore.h>
//using namespace WaveSabreCore;

class Maj7Vst : public WaveSabreVstLib::VstPlug
{
public:
	Maj7Vst(audioMasterCallback audioMaster);
	
	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	// TODO: return all params for VST. Maj7::SetChunk / GetChunk operate on different, optimized, format which would clobber disabled params. that's annoying for DAW work but great for size-optimizing.
	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		if (GetMaj7()) GetMaj7()->SetChunk(data, byteSize);
		return byteSize;
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		return GetMaj7() ? GetMaj7()->GetChunk(data) : 0;
	}


	WaveSabreCore::M7::Maj7 *GetMaj7() const;
};
