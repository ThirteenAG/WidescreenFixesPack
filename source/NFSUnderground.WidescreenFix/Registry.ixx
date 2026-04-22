module;

#include <stdafx.h>
#include <FunctionHookMinHook.hpp>

export module Registry;

import ComVars;

static std::filesystem::path CustomUserDir;
static bool bUseCustomUserDir = false;

static std::unique_ptr<FunctionHookMinHook> mhSHGetFolderPathA;
static HRESULT WINAPI SHGetFolderPathAHook(HWND hwnd, int csidl, HANDLE hToken, DWORD dwFlags, LPSTR pszPath)
{
    if (bUseCustomUserDir)
    {
        int folder = csidl & ~CSIDL_FLAG_CREATE;
        if (folder == CSIDL_COMMON_APPDATA)
        {
            CreateDirectoryW(CustomUserDir.c_str(), NULL);
            memcpy(pszPath, CustomUserDir.u8string().data(), CustomUserDir.u8string().size() + 1);
            return S_OK;
        }
    }

    return mhSHGetFolderPathA->get_original<decltype(SHGetFolderPathA)>()(hwnd, csidl, hToken, dwFlags, pszPath);
}

class Registry
{
public:
    Registry()
    {
        CIniReader iniReader("");
        auto szCustomUserFilesDirectoryInGameDir = iniReader.ReadString("MISC", "CustomUserFilesDirectoryInGameDir", "0");

        if (szCustomUserFilesDirectoryInGameDir.empty() || szCustomUserFilesDirectoryInGameDir == "0")
            szCustomUserFilesDirectoryInGameDir.clear();

        if (!szCustomUserFilesDirectoryInGameDir.empty())
        {
            CustomUserDir = GetExeModulePath<std::filesystem::path>();
            CustomUserDir.append(szCustomUserFilesDirectoryInGameDir);
            bUseCustomUserDir = true;
        }

        mhSHGetFolderPathA = std::make_unique<FunctionHookMinHook>((uintptr_t)SHGetFolderPathA, (uintptr_t)SHGetFolderPathAHook);
        mhSHGetFolderPathA->create();
    }
} Registry;