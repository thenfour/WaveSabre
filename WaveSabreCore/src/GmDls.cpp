#include <WaveSabreCore/GmDls.h>

#include <Windows.h>

namespace WaveSabreCore
{
	unsigned char *GmDls::Load()
	{
		static const char* gmDlsPaths[2] =
		{
			"drivers/gm.dls",
			"drivers/etc/gm.dls"
		};

		HANDLE gmDlsFile = INVALID_HANDLE_VALUE;
		for (int i = 0; gmDlsFile == INVALID_HANDLE_VALUE; i++)
		{
			OFSTRUCT reOpenBuff;
			gmDlsFile = (HANDLE)(UINT_PTR)OpenFile(gmDlsPaths[i], &reOpenBuff, OF_READ);
		}

		auto gmDlsFileSize = GetFileSize(gmDlsFile, NULL);
		auto gmDls = new unsigned char[gmDlsFileSize];
		unsigned int bytesRead;
		ReadFile(gmDlsFile, gmDls, gmDlsFileSize, (LPDWORD)&bytesRead, NULL);
		CloseHandle(gmDlsFile);

		return gmDls;
	}
}
