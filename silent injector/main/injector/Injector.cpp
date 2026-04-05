#include <fstream>

#include "Injector.h"
#include "../../sdk/Logger.h"
#include "../../sdk/Process.h"

bool Injector::standardInjectDll(HANDLE hProc, const std::wstring& dllPath) {
	SIZE_T dllPathSize = (dllPath.length() + 1) * sizeof(wchar_t);

	LPVOID remoteDllPath = VirtualAllocEx(hProc, nullptr, dllPathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!remoteDllPath) {
		Logger::log(Logger::Type::Error, "Can't allocate memory for dll path: 0x%X\n", GetLastError());
		return false;
	}

	if (!WriteProcessMemory(hProc, remoteDllPath, dllPath.c_str(), dllPathSize, nullptr)) {
		Logger::log(Logger::Type::Error, "Can't write dll path: 0x%X\n", GetLastError());
		VirtualFreeEx(hProc, remoteDllPath, 0, MEM_RELEASE);
		return false;
	}

	LPVOID loadLibraryAddr = reinterpret_cast<LPVOID>(GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryW"));
	if (!loadLibraryAddr) {
		Logger::log(Logger::Type::Error, "Can't get LoadLibraryW address\n");
		VirtualFreeEx(hProc, remoteDllPath, 0, MEM_RELEASE);
		return false;
	}

	HANDLE hThread = CreateRemoteThread(hProc, nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(loadLibraryAddr), remoteDllPath, 0, nullptr);
	if (!hThread) {
		Logger::log(Logger::Type::Error, "Can't create remote thread: 0x%X\n", GetLastError());
		VirtualFreeEx(hProc, remoteDllPath, 0, MEM_RELEASE);
		return false;
	}

	WaitForSingleObject(hThread, INFINITE);

	DWORD exitCode = 0;
	GetExitCodeThread(hThread, &exitCode);

	CloseHandle(hThread);
	VirtualFreeEx(hProc, remoteDllPath, 0, MEM_RELEASE);

	if (!exitCode) {
		Logger::log(Logger::Type::Error, "LoadLibraryW returned NULL\n");
		return false;
	}

	return true;
}

void Injector::adjustPrivileges() {
	TOKEN_PRIVILEGES priv = { 0 };
	HANDLE hToken = NULL;

	// ��������� ����� �������� �������� � ������� �� ��������� ����������
	if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
		priv.PrivilegeCount = 2;  // ������������� ���������� ���������� � 2

		// ���������� ��� �������
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &priv.Privileges[0].Luid)) {
			CloseHandle(hToken);
			return;
		}

		AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);

		// ���������� ��� ������ �� ���� ���� �������� ��������
		priv.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		if (!LookupPrivilegeValue(NULL, SE_TAKE_OWNERSHIP_NAME, &priv.Privileges[0].Luid)) {
			CloseHandle(hToken);
			return;
		}

		// ��������� ����������
		AdjustTokenPrivileges(hToken, FALSE, &priv, 0, NULL, NULL);

		// ��������� �����
		CloseHandle(hToken);
	}
}

bool Injector::injectDll(const std::wstring& dllPath, const std::string& processName) {
	DWORD PID = Process::findProcessByName(processName);
	if (PID == 0) {
		Logger::log(Logger::Type::Error, "Incorrect process name\n");
		return false;
	}

	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, PID);
	if (!hProc) {
		DWORD Err = GetLastError();
		Logger::log(Logger::Type::Error, "OpenProcess failed\n");
		return false;
	}

	std::ifstream File(dllPath);
	if (!File.good()) {
		Logger::log(Logger::Type::Error, "Dll file doesn't exist %ls \n", dllPath.c_str());
		CloseHandle(hProc);
		return false;
	}
	File.close();

	if (!standardInjectDll(hProc, dllPath)) {
		CloseHandle(hProc);
		Logger::log(Logger::Type::Error, "Error while injecting\n");
		return false;
	}

	CloseHandle(hProc);
	return true;
}