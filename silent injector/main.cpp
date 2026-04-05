#include <windows.h>
#include <shlobj.h>
#include <winres.h>
#include <shellapi.h>

#include "main/cleaners/Explorer.h"
#include "main/cleaners/Registry.h"
#include "main/injector/Injector.h"
#include "sdk/Logger.h"
#include "sdk/Process.h"

#pragma comment(lib, "shell32.lib")

NOTIFYICONDATA nid;
HWND hwnd;

bool extractEmbeddedDll(std::wstring& outPath) {
    HRSRC hResource = FindResourceA(NULL, (LPCSTR)101, (LPCSTR)10);
    if (!hResource) {
        Logger::log(Logger::Type::Error, "DLL resource not found\n");
        return false;
    }

    HGLOBAL hMemory = LoadResource(NULL, hResource);
    if (!hMemory) {
        Logger::log(Logger::Type::Error, "Can't load DLL resource\n");
        return false;
    }

    DWORD size = SizeofResource(NULL, hResource);
    void* data = LockResource(hMemory);

    wchar_t tempPath[MAX_PATH];
    if (!GetTempPathW(MAX_PATH, tempPath)) {
        Logger::log(Logger::Type::Error, "Can't get temp path\n");
        return false;
    }

    std::wstring fullPath = std::wstring(tempPath) + L"Sphinx.dll";

    HANDLE hFile = CreateFileW(fullPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        Logger::log(Logger::Type::Error, "Can't create temp DLL file\n");
        return false;
    }

    DWORD written;
    WriteFile(hFile, data, size, &written, NULL);
    CloseHandle(hFile);

    outPath = fullPath;
    return true;
}

void cleanupEmbeddedDll(const std::wstring& path) {
    DeleteFileW(path.c_str());
}

void doInject() {
    if (!IsUserAnAdmin()) {
        return;
    }

    std::string processName = "GTA5.exe";

    std::wstring dllPath;
    if (!extractEmbeddedDll(dllPath)) {
        return;
    }

    Injector::adjustPrivileges();
    bool injectedDll = Injector::injectDll(dllPath, processName);

    cleanupEmbeddedDll(dllPath);

    if (injectedDll) {
        Logger::log(Logger::Type::Info, "Dll succesfly injected\n");
    } else {
        Logger::log(Logger::Type::Error, "Failed to inject dll\n");
        return;
    }

    Logger::log(Logger::Type::Info, "Starting cleaning routine\n");

    Sleep(1000);

    std::wstring currentProcessName = Process::getCurrentProcessName();
    std::wstring currentProcessLastFolder = Process::getCurrentProcessLastFolder();
    std::wstring dllFileName = L"Sphinx.dll";
    std::wstring dllFileNameOnly = L"Sphinx";

    if (injectedDll) {
        if (!Registry::deleteValueFromRecentDocs(dllFileName)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Docs\n");
    }
    
    if (!Registry::deleteValueFromRecentDocs(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Docs\n");
    if (!Explorer::deleteFileFromPrefetch(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside Prefetch\n");
    if (!Explorer::deleteFileFromRecent(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Files\n");
    if (!Registry::deleteValueFromUserAssist(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside User Assist\n");
    if (!Registry::deleteValueFromBAM(currentProcessName)) Logger::log(Logger::Type::Warning, "Nothing found inside BAM structures\n");
    if (!Registry::deleteValueFromShallBags(currentProcessLastFolder)) Logger::log(Logger::Type::Warning, "Nothing found inside Shell Bags\n");

    if (injectedDll) {
        if (!Explorer::deleteFileFromPrefetch(dllFileNameOnly)) Logger::log(Logger::Type::Warning, "Nothing found inside Prefetch\n");
        if (!Explorer::deleteFileFromRecent(dllFileNameOnly)) Logger::log(Logger::Type::Warning, "Nothing found inside Recent Files\n");
        if (!Registry::deleteValueFromUserAssist(dllFileNameOnly)) Logger::log(Logger::Type::Warning, "Nothing found inside User Assist\n");
        if (!Registry::deleteValueFromBAM(dllFileNameOnly)) Logger::log(Logger::Type::Warning, "Nothing found inside BAM structures\n");
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_DESTROY) {
        Shell_NotifyIcon(NIM_DELETE, &nid);
        PostMessageW(hwnd, WM_QUIT, 0, 0);
        return 0;
    }
    if (msg == WM_USER) {
        if (lParam == WM_LBUTTONDBLCLK) {
            doInject();
        }
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TrayClass";
    RegisterClassW(&wc);

    hwnd = CreateWindowW(wc.lpszClassName, L"Tray", WS_OVERLAPPEDWINDOW, 0, 0, 0, 0, NULL, NULL, hInstance, NULL);

    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_MESSAGE;
    nid.hIcon = NULL;
    nid.uCallbackMessage = WM_USER;

    Shell_NotifyIcon(NIM_ADD, &nid);

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}