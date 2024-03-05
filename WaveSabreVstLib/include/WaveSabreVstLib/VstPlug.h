#ifndef __WAVESABREVSTLIB_VSTPLUG_H__
#define __WAVESABREVSTLIB_VSTPLUG_H__

#undef _WIN32_WINNT

#define _7ZIP_ST // single-threaded LZMA lib

#include "../7z/LzmaEnc.h"

#include "audioeffectx.h"
#include <WaveSabreCore.h>
#include "./Serialization.hpp"

// copies 16-bit static array of values to a float buffer. specifically intended to copy a 'defaults' array to a usable float parameter array.
template<size_t N>
inline void Copy16bitDefaults(float* dest, const int16_t(&src)[N])
{
	for (size_t i = 0; i < N; ++i) {
		dest[i] = M7::math::Sample16To32Bit(src[i]);
	}
}


// accepts the VST chunk, optimizes & minifies and outputs the wavesabre optimized chunk.
template<typename TVST>
inline int WaveSabreDeviceVSTChunkToMinifiedChunk_Impl(const char* deviceName, int inpSize, void* inpData, int* outpSize, void** outpData)
{
	*outpSize = 0;
	auto p = new TVST(nullptr); // assume too big for stack.
	p->setChunk(inpData, inpSize, false);
	p->OptimizeParams();
	*outpSize = p->GetMinifiedChunk(outpData);
	return *outpSize;
}


namespace WaveSabreVstLib
{

	// compresses the given data to test Squishy performance for a given buf.
	// just returns the compressed size
	inline int TestCompression(int inpSize, void* inpData)
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
		return (int)compressedSize;
	}

	//// generates a list of defaults from the specified device type.
	//template<typename TDevice>
	//inline std::vector<float> GetDefaultParamCache()
	//{
	//	auto tmpEffect = std::make_unique(new TDevice);
	//	CCASSERT(!!tmpEffect);
	//	if (!tmpEffect) {
	//		return {};
	//	}
	//	std::vector<float> ret;
	//	ret.reserve(numParams);
	//	for (int i = 0; i < numParams; ++i) {
	//		ret.push_back(tmpEffect->GetParam(i));
	//	}
	//	return ret;
	//}

	//// generates a list of defaults from the specified device type.
	////template<typename TDevice>
	//inline std::vector<float> GetDefaultParamCache(Device * p)
	//{
	//	std::vector<float> ret;
	//	ret.reserve(numParams);
	//	for (int i = 0; i < numParams; ++i) {
	//		ret.push_back(p->GetParam(i));
	//	}
	//	return ret;
	//}

	template <typename Treal, size_t N>
	class MovingAverage
	{
	public:
		void Update(Treal sample)
		{
			if (num_samples_ < N)
			{
				samples_[num_samples_++] = sample;
				total_ += sample;
			}
			else
			{
				Treal& oldest = samples_[num_samples_++ % N];
				total_ += sample - oldest;
				oldest = sample;
			}
		}

		Treal GetValue() const
		{
			return total_ / GetValidSampleCount();// std::min(num_samples_, N);
		}

		size_t GetValidSampleCount() const
		{
			return std::min(num_samples_, N);
		}
		size_t GetTotalSamplesTaken() const
		{
			return num_samples_;
		}

		void Clear()
		{
			total_ = 0;
			num_samples_ = 0;
		}

		Treal GetSample(size_t n) const
		{
			return samples_[n % N];
		}

	private:
		Treal samples_[N];
		size_t num_samples_ = 0;
		Treal total_ = 0;
	};

	enum class ChunkStatsResult {
		Nothing,
		Success,
		NotSupported,
	};

	struct ChunkStats
	{
		ChunkStatsResult result = ChunkStatsResult::Nothing;
		int nonZeroParams = 0;
		int defaultParams = 0;
		int uncompressedSize = 0;
		int compressedSize = 0;
	};

	class VstPlug : public AudioEffectX
	{
	public:
		VstPlug(audioMasterCallback audioMaster, int numParams, int numInputs, int numOutputs, VstInt32 id, WaveSabreCore::Device* device, bool synth = false);
		virtual ~VstPlug();

		virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
		virtual VstInt32 processEvents(VstEvents* ev);

		virtual void setProgramName(char* name);
		virtual void getProgramName(char* name);

		virtual void setSampleRate(float sampleRate);
		virtual void setTempo(int tempo);

		virtual void setParameter(VstInt32 index, float value);
		virtual float getParameter(VstInt32 index);
		virtual void getParameterLabel(VstInt32 index, char* label);
		virtual void getParameterDisplay(VstInt32 index, char* text);
		virtual void getParameterName(VstInt32 index, char* text);

		virtual VstInt32 getChunk(void** data, bool isPreset);
		virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset);

		virtual bool getEffectName(char* name);
		virtual bool getVendorString(char* text);
		virtual bool getProductString(char* text);
		virtual VstInt32 getVendorVersion();

		virtual VstInt32 canDo(char* text);
		virtual VstInt32 getNumMidiInputChannels();

		double GetCPUUsage01() const {
			return mCPUUsage.GetValue();
		}

		// takes the current patch, returns a binary blob containing the WaveSabre chunk.
		// this is where we serialize "diff" params, and save to 16-bit values.
		// and there's the opportunity to append other things; for example Maj7 Synth sampler devices.
		//
		// default implementation just does this for our param cache.
		virtual int GetMinifiedChunk(void** data);

		// looks at the current params and returns statistics related to size optimization.
		virtual ChunkStats AnalyzeChunkMinification();

		// default impl does nothing.
		virtual void OptimizeParams();

		virtual const char* GetJSONTagName() = 0;

		using ParamAccessor = ::WaveSabreCore::M7::ParamAccessor;

		template<typename Tenum, typename Toffset>
		inline void OptimizeEnumParam(ParamAccessor& params, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return; // invalid IDs exist for example in LFo

			ParamAccessor dp{ mDefaultParamCache.data(), params.mBaseParamID };
			if (dp.GetEnumValue<Tenum>(offset) == params.GetEnumValue<Tenum>(offset)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
		}

		template<typename Toffset>
		inline void OptimizeIntParam(ParamAccessor& params, const ::WaveSabreCore::M7::IntParamConfig& cfg, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return; // invalid IDs exist for example in LFo

			ParamAccessor dp{ mDefaultParamCache.data(), params.mBaseParamID };
			if (dp.GetIntValue(offset, cfg) == params.GetIntValue(offset, cfg)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
		}

		template<typename Toffset>
		inline bool OptimizeBoolParam(ParamAccessor& params, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return false; // invalid IDs exist for example in LFo
			ParamAccessor dp{ mDefaultParamCache.data(), params.mBaseParamID };
			bool ret = params.GetBoolValue(offset);
			if (ret == dp.GetBoolValue(offset)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
			return ret;
		}


		WaveSabreCore::Device* getDevice() const;

		std::vector<float> mDefaultParamCache;

	protected:
		void setEditor(class VstEditor *editor);

	private:
		int numParams, numInputs, numOutputs;
		bool synth;

		LARGE_INTEGER perfFreq;
		MovingAverage<double, 100> mCPUUsage;

		char programName[kVstMaxProgNameLen + 1];

		WaveSabreCore::Device *device;
	};
}

#endif
