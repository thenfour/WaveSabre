#ifndef __WAVESABREVSTLIB_H__
#define __WAVESABREVSTLIB_H__

#include "WaveSabreVstLib/VstPlug.h"
#include "WaveSabreVstLib/VstEditor.h"

#include "WaveSabreVstLib/ImageManager.h"




#include <Windows.h>
#undef max
#undef min
#ifdef _DEBUG
inline bool WSAssert(bool condition, const char *msg)
{
	if (condition) return condition;
	//std::string fmt = std::string("[%x:%x] FAILED ASSERTION: ") + format + "\r\n";
	//auto size = std::snprintf(nullptr, 0, fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
	//std::string output(size + 2, '\0');// to ensure the null-terminator
	//output.resize(size);// so the reported length is correct.
	//std::sprintf(&output[0], fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
	OutputDebugStringA("FAILED ASSERTION\r\n");
	OutputDebugStringA(msg);
	return condition;
}


#include <string>

namespace cc
{
	template <typename ...Args>
	static void log(const std::string& format, Args && ...args)
	{
		std::string fmt = std::string("[%x:%x] ") + format + "\r\n";
		auto size = std::snprintf(nullptr, 0, fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
		std::string output(size + 2, '\0');// to ensure the null-terminator
		output.resize(size);// so the reported length is correct.
		std::sprintf(&output[0], fmt.c_str(), GetCurrentProcessId(), GetCurrentThreadId(), std::forward<Args>(args)...);
		OutputDebugStringA(output.c_str());
	}

}
#else
#define WSAssert(a, b)
#endif


#endif
