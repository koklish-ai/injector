#pragma once
#include <Windows.h>
#include <string>

class Injector
{
public:

	static void adjustPrivileges();
	static bool injectDll(const std::wstring& dllPath, const std::string& processName);
	static bool standardInjectDll(HANDLE hProc, const std::wstring& dllPath);
};