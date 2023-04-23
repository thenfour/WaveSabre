
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


float* GenerateDefaultParamCache()
{
	static float* gCached = nullptr;
	if (gCached == nullptr) {
		M7::Maj7* tmpEffect = new M7::Maj7(); // consumes like 30kb of stack so new it.
		//auto ret = std::make_unique<float[]>((int)M7::ParamIndices::NumParams);
		gCached = new float[(int)M7::ParamIndices::NumParams];
		//std::copy(std::begin(tmpEffect->mParamCache), std::end(tmpEffect->mParamCache), ret.get());
		memcpy(gCached, tmpEffect->mParamCache, sizeof(tmpEffect->mParamCache));
		delete tmpEffect;
	}
	return gCached;
}

// assumes that p has had its default param cache filled.
int GetMinifiedChunk(M7::Maj7* p, void** data)
{
	// the base chunk is our big param dump, with sampler data appended. each sampler will append its data; we won't think too much about that data here.
	// the base chunk is a diff of default params, because that's way more compressible. the alternative would be to meticulously
	// serialize some optimal fields based on whether an object is in use or not.
	// if we just do diffs against defaults, then it's going to be almost all zeroes.
	// the DAW can also attempt to optimize by finding unused elements and setting them to defaults.
	//     0.20000000298023223877, // PortTm
	//     0.40000000596046447754, // O1ScFq
	//     0.0000001
	M7::Serializer s;

	auto defaultParamCache = GenerateDefaultParamCache();

	for (int i = 0; i < (int)M7::ParamIndices::NumParams; ++i) {
		double f = p->mParamCache[i];
		f -= defaultParamCache[i];
		static constexpr double eps = 0.0000001;
		double af = f < 0 ? -f : f;
		if (af < eps) {
			f = 0;
		}
		//s.WriteFloat((float)f);
		s.WriteInt16NormalizedFloat((float)f);
	}

	for (auto* sd : p->mpSamplerDevices) {
		sd->Serialize(s);
	}

	auto ret = s.DetachBuffer();
	*data = ret.first;
	return (int)ret.second;
}



// no conversion required
int __cdecl WaveSabreDeviceVSTChunkToMinifiedChunk(const char *deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	*outpSize = 0;

	M7::Maj7* tmpEffect = new M7::Maj7(); // consumes like 30kb of stack so new it.
	Maj7SetVstChunk(nullptr, tmpEffect, inpData, inpSize);
	
	M7::OptimizeParams(tmpEffect, true);

	*outpSize = GetMinifiedChunk(tmpEffect, outpData);
	delete tmpEffect;
	return *outpSize;
}

void __cdecl WaveSabreFreeChunk(void* p)
{
	M7::Serializer::FreeBuffer(p);
}

// compresses the given data to test Squishy performance for a given buf.
// just returns the compressed size
int __cdecl WaveSabreTestCompression(int inpSize, void* inpData)
{
	std::vector<uint8_t> compressedData;
	compressedData.resize(inpSize);
	std::vector<uint8_t> encodedProps;
	encodedProps.resize(inpSize);
	SizeT compressedSize = inpSize;
	SizeT encodedPropsSize = inpSize;

	ISzAlloc alloc;
	alloc.Alloc = [](ISzAllocPtr p, SizeT s) {
		return malloc(s);
	};
	alloc.Free = [](ISzAllocPtr p, void* addr) {
		free(addr);
	};

	CLzmaEncProps props;
	LzmaEncProps_Init(&props);
	props.level = 5;

	int lzresult = LzmaEncode(&compressedData[0], &compressedSize, (const Byte*)inpData, inpSize, &props, encodedProps.data(), &encodedPropsSize, 0, nullptr, &alloc, &alloc);
	//ret.compressedSize = ;

	//delete[] data;
	return (int)compressedSize;
}

