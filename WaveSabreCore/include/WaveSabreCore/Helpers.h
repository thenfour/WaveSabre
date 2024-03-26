#ifndef __WAVESABRECORE_HELPERS_H__
#define __WAVESABRECORE_HELPERS_H__

//#include "StateVariableFilter.h"
//#include "Twister.h"
#include "SynthDevice.h"

#define _USE_MATH_DEFINES
#include <math.h>

#ifdef _DEBUG
#include <Windows.h>
#include <string>
#include <algorithm>
#include <iostream>
#include <fstream>
namespace cc
{
	thread_local extern size_t gBufferCount;

	thread_local extern size_t gLogIndent;
	template <typename ...Args>
	static inline void log(const std::string& format, Args && ...args)
	{
		std::string fmt = std::string("[%x:%x] ") + std::string(gLogIndent, ' ') + std::string(gLogIndent, ' ') + format + "\r\n";
		auto size = std::snprintf(nullptr, 0, fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
		std::string output(size + 2, '\0');// to ensure the null-terminator
		output.resize(size);// so the reported length is correct.
		std::snprintf(&output[0], size, fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
		OutputDebugStringA(output.c_str());
		std::string path("c:\\root\\cclog.txt");
		HANDLE h = CreateFileA(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
		DWORD bw;
		SetFilePointer(h, GetFileSize(h, 0), 0, FILE_BEGIN);
		WriteFile(h, output.c_str(), (DWORD)output.length(), &bw, 0);
		CloseHandle(h);
	}
	struct LogScope {
		LogScope(const std::string& msg) {
			log("{ " + msg);
			gLogIndent++;
		}
		LogScope() : LogScope("") { }
		~LogScope() {
			gLogIndent--;
			log("}");
		}
	};
}
#endif // _DEBUG


#ifdef _DEBUG
static inline void assert__(bool condition, const char* conditionStr, const char* file, int line) {
	if (condition) return;
	char s[1000];
	sprintf(s, "ASSERT failed: %s (%s:%d)", conditionStr, file, line);
	::OutputDebugStringA(s);
	::DebugBreak();
}

#define CCASSERT(x) assert__(x, #x, __FILE__, __LINE__)
#else // _DEBUG
#define CCASSERT(x)
#endif // _DEBUG



namespace WaveSabreCore
{
	struct ISampleSource
	{
		virtual ~ISampleSource()
		{}

		virtual const float* GetSampleData() const = 0;
		virtual int GetSampleLength() const = 0;
		virtual int GetSampleLoopStart() const = 0;
		virtual int GetSampleLoopLength() const = 0;
		virtual int GetSampleRate() const = 0;
	};

	class Helpers
	{
	public:
		static double CurrentSampleRate;
		static float CurrentSampleRateF;
		static float CurrentSampleRateRecipF;

		static int CurrentTempo;
		//static int RandomSeed;

		//static void Init();

		//static float RandFloat();

		static double Exp2(double x);
		static float Exp2F(float x);

		static inline float Exp10F(float x)
		{
			const float scale = (float)(M_LN10 / M_LN2);
			return Exp2F(x * scale);
		}

		static inline double Pow2(double x)
		{
			return x * x;
		}

		static inline double Pow4(double x)
		{
			return Pow2(Pow2(x));
		}

		static inline float Pow2F(float x)
		{
			return x * x;
		}

		//static double FastCos(double x);
		//static double FastSin(double x);

		//static double NoteToFreq(double note);

		//static double Square135(double phase);
		//static double Square35(double phase);

		//static float Mix(float v1, float v2, float mix);
		//static float Clamp(float f, float min, float max);

		//static float DbToScalar(float db);

		//static float EnvValueToScalar(float value);
		//static float ScalarToEnvValue(float scalar);

		//static float VolumeToScalar(float volume);
		//static float ScalarToVolume(float scalar);

		static bool ParamToBoolean(float value);
		static float BooleanToParam(bool b);

		static float ParamToFrequency(float param);
		static float FrequencyToParam(float freq);

		//static float ParamToQ(float param);
		//static float QToParam(float q);

		//static float ParamToDb(float param, float range = 24.0f);
		//static float DbToParam(float db, float range = 24.0f);

		//static float ParamToResonance(float param);
		//static float ResonanceToParam(float resonance);

		//static StateVariableFilterType ParamToStateVariableFilterType(float param);
		//static float StateVariableFilterTypeToParam(StateVariableFilterType type);

		//static int ParamToUnisono(float param);
		//static float UnisonoToParam(int unisono);

		//static double ParamToVibratoFreq(float param);
		//static float VibratoFreqToParam(double vf);

		//static float PanToScalarLeft(float pan);
		//static float PanToScalarRight(float pan);

		//static Spread ParamToSpread(float param);
		//static float SpreadToParam(Spread spread);

		//static VoiceMode ParamToVoiceMode(float param);
		//static float VoiceModeToParam(VoiceMode type);
	//private:
	//	static const int fastSinTabLog2Size = 9; // size = 512
	//	static const int fastSinTabSize = (1 << fastSinTabLog2Size);
	//	static const int adjustedFastSinTabSize = fastSinTabSize + 1;
	//	static double fastSinTab[adjustedFastSinTabSize];
	};
}

#endif
