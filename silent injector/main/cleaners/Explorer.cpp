#include "Explorer.h"
#include "../../sdk/Logger.h"

static const std::wstring prefetchPath = L"C:\\Windows\\Prefetch";

bool Explorer::deleteFileFromPrefetch(const std::wstring& fileName) {
    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((prefetchPath + L"\\*.pf").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        std::wstring currentFileName(findData.cFileName);
        std::wstring transformatedFileName = fileName;

        for (auto& c : transformatedFileName) c = std::toupper(c);

        std::wstring upperCurrent = currentFileName;
        for (auto& c : upperCurrent) c = std::toupper(c);

        if (upperCurrent.find(transformatedFileName) != std::wstring::npos) {
            std::wstring fullPath = prefetchPath + L"\\" + findData.cFileName;
            DeleteFileW(fullPath.c_str());
            Logger::log(Logger::Type::Info, "Removed file %ls from Prefetch\n", currentFileName.c_str());
            FindClose(hFind);
            return true;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return false;
}

bool Explorer::deleteFileFromRecent(const std::wstring& fileName) {
    PWSTR path = NULL;
    std::wstring appDataPath;

    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &path) == S_OK) {
        appDataPath = path;
        CoTaskMemFree(path);
    }

    appDataPath += L"\\Microsoft\\Windows\\Recent";

    WIN32_FIND_DATAW findData;
    HANDLE hFind = FindFirstFileW((appDataPath + L"\\*").c_str(), &findData);
    if (hFind == INVALID_HANDLE_VALUE) return false;

    do {
        if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

        std::wstring currentFileName(findData.cFileName);
        std::wstring transformatedFileName = fileName;

        for (auto& c : transformatedFileName) c = std::toupper(c);

        std::wstring upperCurrent = currentFileName;
        for (auto& c : upperCurrent) c = std::toupper(c);

        if (upperCurrent.find(transformatedFileName) != std::wstring::npos) {
            std::wstring fullPath = appDataPath + L"\\" + findData.cFileName;
            DeleteFileW(fullPath.c_str());
            Logger::log(Logger::Type::Info, "Removed file %ls from Recent\n", currentFileName.c_str());
            FindClose(hFind);
            return true;
        }
    } while (FindNextFileW(hFind, &findData));

    FindClose(hFind);
    return false;
}