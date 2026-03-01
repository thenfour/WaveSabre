#include <Windows.h>

void __cdecl operator delete[](void* p) {
	free(p);// HeapFree(GetProcessHeap(), 0, p);
}
void* __cdecl operator new[](size_t bytes) {
	return malloc(bytes);// return HeapAlloc(GetProcessHeap(), 0, bytes);
}

#if defined(_M_IX86)
extern "C" long __declspec(naked) __cdecl _ftol2()
{
	__asm {
		sub esp, 4
		fistp dword ptr [esp]
		pop eax
		ret
	}
}

extern "C" long __declspec(naked) __cdecl _ftol2_sse()
{
	__asm {
		cvttss2si eax, xmm0
		ret
	}
}
#endif
