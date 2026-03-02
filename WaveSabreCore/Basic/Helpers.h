#ifndef __WAVESABRECORE_HELPERS_H__
#define __WAVESABRECORE_HELPERS_H__

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



namespace WaveSabreCore
{
	class Helpers
	{
	public:
#ifdef MIN_SIZE_REL
    static constexpr int CurrentSampleRateI = 44100;
    static constexpr float CurrentSampleRateF = CurrentSampleRateI;
    static constexpr double CurrentSampleRate = CurrentSampleRateI;
    static constexpr float CurrentSampleRateRecipF = 1.0f / CurrentSampleRateF;
#else
    static int CurrentSampleRateI;
    static double CurrentSampleRate;
    static float CurrentSampleRateF;
    static float CurrentSampleRateRecipF;
	static void SetSampleRate(int sampleRate);
#endif  // MIN_SIZE_REL

		static int CurrentTempo;

	};
}

#endif
