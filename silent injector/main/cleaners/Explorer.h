#pragma once
#include <shlobj.h>

#include "../../sdk/Logger.h"

class Explorer
{
public:
	static bool deleteFileFromPrefetch(const std::wstring& fileName);
	static bool deleteFileFromRecent(const std::wstring& fileName);

};