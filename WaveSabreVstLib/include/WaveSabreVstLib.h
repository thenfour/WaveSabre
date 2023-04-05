
#pragma once

#include "WaveSabreVstLib/VstPlug.h"
#include "WaveSabreVstLib/VstEditor.h"

#include "WaveSabreVstLib/ImageManager.h"



inline void CopyTextToClipboard(const std::string& s) {

	if (!OpenClipboard(NULL)) return;
	HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
	if (!hMem) {
		CloseClipboard();
		return;
	}
	char* pMem = (char*)GlobalLock(hMem);
	if (!pMem) {
		GlobalFree(hMem);
		CloseClipboard();
		return;
	}
	strcpy(pMem, s.c_str());
	GlobalUnlock(hMem);

	EmptyClipboard();
	SetClipboardData(CF_TEXT, hMem);

	CloseClipboard();
}

inline std::string GetClipboardText() {
	if (!OpenClipboard(NULL)) return {};
	HANDLE hData = GetClipboardData(CF_TEXT);
	if (!hData) {
		CloseClipboard();
		return {};
	}

	char* pMem = (char*)GlobalLock(hData);
	if (pMem == NULL) {
		CloseClipboard();
		return {};
	}

	std::string text = pMem;

	// Unlock the global memory and close the clipboard
	GlobalUnlock(hData);
	CloseClipboard();

	return text;
}


inline std::string LoadContentsOfTextFile(const std::string& path)
{
	HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return "";

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize))
	{
		CloseHandle(hFile);
		return "";
	}

	char* fileContents = new char[(int)(fileSize.QuadPart + 1)];

	DWORD bytesRead;
	if (!ReadFile(hFile, fileContents, (DWORD)fileSize.QuadPart, &bytesRead, NULL))
	{
		delete[] fileContents;
		CloseHandle(hFile);
		return "";
	}

	fileContents[fileSize.QuadPart] = '\0';

	CloseHandle(hFile);
	std::string result(fileContents);
	delete[] fileContents;
	return result;
}

