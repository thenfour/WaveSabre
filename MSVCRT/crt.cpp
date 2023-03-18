#include <Windows.h>

void __cdecl operator delete[](void* p) {
	free(p);// HeapFree(GetProcessHeap(), 0, p);
}
void* __cdecl operator new[](size_t bytes) {
	return malloc(bytes);// return HeapAlloc(GetProcessHeap(), 0, bytes);
}
