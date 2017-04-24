#pragma once

#include <string>
#include <vector>

#if defined(WIN32) || defined(_WIN32)
#include "char_and_wchar.h"
#include <windows.h>
#endif // WIN32


inline void winGlob(const std::string & pattern, std::vector<std::string> &ret)
{
	WIN32_FIND_DATA FindData;
	HANDLE hError;
#ifdef UNICODE
	wchar_t *wpattern_ptr = NULL;
	wpattern_ptr = ctow(pattern.c_str(), wpattern_ptr);
	hError = FindFirstFile(wpattern_ptr, &FindData);
#else
	hError = FindFirstFile(pattern.c_str(), &FindData);
#endif // UNICODE

	if (hError == INVALID_HANDLE_VALUE)return;

	char *FileName = NULL;

#ifdef UNICODE
	wchar_t *wFileName = FindData.cFileName;
	FileName = wtoc(wFileName, FileName);
#else
	FileName = FindData.cFileName;
#endif // UNICODE
	
	ret.push_back(std::string(FileName));

	while (FindNextFile(hError, &FindData))
	{
#ifdef UNICODE
		*wFileName = FindData.cFileName;
		FileName = wtoc(wFileName, FileName);
#else
		FileName = FindData.cFileName;
#endif // UNICODE

		ret.push_back(std::string(FileName));

	}
}
