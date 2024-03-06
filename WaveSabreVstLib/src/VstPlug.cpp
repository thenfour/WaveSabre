#include <string>
#include <WaveSabreVstLib/VstPlug.h>
#include <WaveSabreVstLib/VstEditor.h>

using namespace std;
using namespace WaveSabreCore;

namespace WaveSabreVstLib
{
	VstPlug::VstPlug(audioMasterCallback audioMaster, int numParams, int numInputs, int numOutputs, VstInt32 id, Device* device, bool synth)
		: AudioEffectX(audioMaster, 1, numParams)
	{
		// assume the device has default values populated; grab a copy.
		mDefaultParamCache.reserve(numParams);
		for (int i = 0; i < numParams; ++i) {
			mDefaultParamCache.push_back(device->GetParam(i));
		}

		QueryPerformanceFrequency(&this->perfFreq);

		this->numParams = numParams;
		this->numInputs = numInputs;
		this->numOutputs = numOutputs;
		this->device = device;
		this->synth = synth;

		setNumInputs(numInputs);
		setNumOutputs(numOutputs);
		setUniqueID(id);
		canProcessReplacing();
		canDoubleReplacing(false);
		programsAreChunks();
		if (synth) isSynth();

		vst_strncpy(programName, "Default", kVstMaxProgNameLen);
	}

	VstPlug::~VstPlug()
	{
		if (device) delete device;
	}

	void VstPlug::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames)
	{
		if (device && sampleFrames)
		{
			VstTimeInfo *ti = getTimeInfo(0);
			if (ti)
			{
				if ((ti->flags & kVstTempoValid) > 0)
					setTempo((int)ti->tempo);
			}

			MxcsrFlagGuard mxcsrFlagGuard;

			LARGE_INTEGER timeStart, timeEnd;
			QueryPerformanceCounter(&timeStart);
			device->Run(ti ? ti->samplePos / ti->sampleRate : 0.0, inputs, outputs, sampleFrames);
			QueryPerformanceCounter(&timeEnd);

			// to calculate CPU, assume that 100% CPU usage would mean it runs at the same as realtime speed. a 3ms buffer took 3ms to process.
			double elapsedSeconds = (double)(timeEnd.QuadPart - timeStart.QuadPart) / (double)this->perfFreq.QuadPart;
			double frameSeconds = (double)sampleFrames / this->sampleRate;
			mCPUUsage.Update(elapsedSeconds / frameSeconds);
		}
		else {
			mCPUUsage.Clear();
		}
	}

	VstInt32 VstPlug::processEvents(VstEvents *ev)
	{
		if (device)
		{
			for (VstInt32 i = 0; i < ev->numEvents; i++)
			{
				if (ev->events[i]->type == kVstMidiType)
				{
					VstMidiEvent *midiEvent = (VstMidiEvent *)ev->events[i];
					char *midiData = midiEvent->midiData;
					int status = midiData[0] & 0xf0;

					//cc::log("Vst midi event; status=%02x, flags=%x, type=d", status, midiEvent->flags, midiEvent->type);

					if (status == 0xb0)
					{
						if (midiData[1] == 0x7e || midiData[1] == 0x7b) {
							device->AllNotesOff();
						}
						else {
							device->MidiCC(midiData[1], midiData[2], midiEvent->deltaFrames);
						}
					}
					else if (status == 0x90 || status == 0x80)
					{
						int note = midiData[1] & 0x7f;
						int vel = midiData[2] & 0x7f;
						if (vel == 0 || status == 0x80) device->NoteOff(note, midiEvent->deltaFrames);
						else device->NoteOn(note, vel, midiEvent->deltaFrames);
					}
					else if (status == 0xe0) {
						int msb = midiData[2];
						int lsb = midiData[1];
						device->PitchBend(lsb, msb, midiEvent->deltaFrames);
					}
				}
			}
		}
		return 1;
	}

	void VstPlug::setProgramName(char *name)
	{
		vst_strncpy(programName, name, kVstMaxProgNameLen);
	}

	void VstPlug::getProgramName(char *name)
	{
		vst_strncpy(name, programName, kVstMaxProgNameLen);
	}

	void VstPlug::setSampleRate(float sampleRate)
	{
		AudioEffect::setSampleRate(sampleRate);
		if (device) device->SetSampleRate(sampleRate);
	}

	void VstPlug::setTempo(int tempo)
	{
		if (device)	device->SetTempo(tempo);
	}

	void VstPlug::setParameter(VstInt32 index, float value)
	{
		if (device) device->SetParam(index, value);
		//if (editor) ((AEffGUIEditor *)editor)->setParameter(index, value);
	}

	float VstPlug::getParameter(VstInt32 index)
	{
		return device ? device->GetParam(index) : 0.0f;
	}

	void VstPlug::getParameterLabel(VstInt32 index, char *label)
	{
		vst_strncpy(label, "%", kVstMaxParamStrLen);
	}

	void VstPlug::getParameterDisplay(VstInt32 index, char *text)
	{
		vst_strncpy(text, to_string(device->GetParam(index) * 100.0f).c_str(), kVstMaxParamStrLen);
	}

	void VstPlug::getParameterName(VstInt32 index, char *text)
	{
		vst_strncpy(text, "Name", kVstMaxParamStrLen);
	}

	//VstInt32 VstPlug::getChunk(void **data, bool isPreset)
	//{
	//	//return device ? device->GetChunk(data) : 0;
	//}

	//VstInt32 VstPlug::setChunk(void *data, VstInt32 byteSize, bool isPreset)
	//{
	//	if (device) device->SetChunk(data, byteSize);
	//	return byteSize;
	//}

	bool VstPlug::getEffectName(char *name)
	{
		vst_strncpy(name, "I AM GOD, BITCH", kVstMaxEffectNameLen);
		return true;
	}

	bool VstPlug::getVendorString(char *text)
	{
		vst_strncpy(text, "Logicoma", kVstMaxVendorStrLen);
		return true;
	}

	bool VstPlug::getProductString(char *text)
	{
		vst_strncpy(text, "I AM GOD, BITCH", kVstMaxProductStrLen);
		return true;
	}

	VstInt32 VstPlug::getVendorVersion()
	{
		return 1337;
	}

	VstInt32 VstPlug::canDo(char *text)
	{
		if (synth && (!strcmp(text, "receiveVstEvents") || !strcmp(text, "receiveVstMidiEvents"))) return 1;
		return -1;
	}

	VstInt32 VstPlug::getNumMidiInputChannels()
	{
		return synth ? 1 : 0;
	}

	void VstPlug::setEditor(VstEditor *editor)
	{
		this->editor = editor;
	}

	Device *VstPlug::getDevice() const
	{
		return device;
	}

	void VstPlug::OptimizeParams()
	{
		// override this to optimize params.
	}

	// assumes that p has had its default param cache filled.
	// takes the current patch, returns a binary blob containing the WaveSabre chunk.
	// this is where we serialize "diff" params, and save to 16-bit values.
	// and there's the opportunity to append other things; for example Maj7 Synth sampler devices.
	//
	// default implementation just does this for our param cache.
	int VstPlug::GetMinifiedChunk(void** data)
	{
		M7::Serializer s;

		//auto defaultParamCache = GetDefaultParamCache();
		CCASSERT(defaultParamCache.size());

		for (int i = 0; i < numParams; ++i) {
			double f = getParameter(i);
			f -= mDefaultParamCache[i];
			static constexpr double eps = 0.000001; // NB: 1/65536 = 0.0000152587890625
			double af = f < 0 ? -f : f;
			if (af < eps) {
				f = 0;
			}
			s.WriteInt16NormalizedFloat((float)f);
		}

		auto ret = s.DetachBuffer();
		*data = ret.first;
		return (int)ret.second;
	}

	// looks at the current params and returns statistics related to size optimization.
	ChunkStats VstPlug::AnalyzeChunkMinification()
	{
		ChunkStats ret;
		void* data;
		int size = GetMinifiedChunk(&data);
		ret.uncompressedSize = size;

		for (size_t i = 0; i < numParams; ++i) {
			float d = getParameter((VstInt32)i) - mDefaultParamCache[i];
			if (fabsf(d) > 0.00001f) ret.nonZeroParams++;
			else ret.defaultParams++;
		}

		std::vector<uint8_t> compressedData;
		compressedData.resize(size);
		std::vector<uint8_t> encodedProps;
		encodedProps.resize(size);
		SizeT compressedSize = size;
		SizeT encodedPropsSize = size;

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

		int lzresult = LzmaEncode(&compressedData[0], &compressedSize, (const Byte*)data, size, &props, encodedProps.data(), &encodedPropsSize, 0, nullptr, &alloc, &alloc);
		ret.compressedSize = (int)compressedSize;

		delete[] data;
		return ret;
	}



} // namespace WaveSabreVstLib
