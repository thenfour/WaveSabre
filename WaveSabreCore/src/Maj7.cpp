#include <WaveSabreCore/Maj7.hpp>
#include <WaveSabreCore/Helpers.h>

#include <math.h>
#include <Windows.h>
//
//
void __cdecl operator delete[](void* p) {
	free(p);
}
void* __cdecl operator new[](unsigned __int64 bytes) {
	return malloc(bytes);
}

namespace WaveSabreCore
{

	namespace M7
	{
	} // namespace M7


} // namespace WaveSabreCore








