
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

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		if (GetMaj7()) GetMaj7()->SetChunk(data, byteSize);
		return byteSize;
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		// return a more compatible chunk than the minified one by the WaveSabre device.
		return GetMaj7() ? GetMaj7()->GetChunk(data) : 0;
	}


	WaveSabreCore::M7::Maj7 *GetMaj7() const;
};
