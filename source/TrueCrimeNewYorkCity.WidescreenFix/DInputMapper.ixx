module;

#include <stdafx.h>

export module DInputMapper;

export void InitDInputMapper()
{
    // Use bundled dlls for control mapping if needed (requires elevation)
    static auto runRegsvr32 = [](const std::wstring& args)
    {
        SHELLEXECUTEINFOW sei = { sizeof(sei) };
        sei.lpVerb = L"runas";
        sei.lpFile = L"regsvr32.exe";
        sei.lpParameters = args.c_str();
        sei.nShow = SW_HIDE;
        sei.fMask = SEE_MASK_NOCLOSEPROCESS;

        if (ShellExecuteExW(&sei))
        {
            WaitForSingleObject(sei.hProcess, 10000);
            CloseHandle(sei.hProcess);
        }
    };

    static auto getFullPath = []() -> std::wstring
    {
        wchar_t exePath[MAX_PATH];
        GetModuleFileNameW(NULL, exePath, MAX_PATH);
        std::wstring exeDir(exePath);
        exeDir = exeDir.substr(0, exeDir.find_last_of(L"\\/") + 1);
        return exeDir + L"diactfrm.dll";
    };

    static auto isDiactfrmRegistered = []() -> bool
    {
        HKEY hKey = NULL;
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, L"CLSID\\{9F34AF20-6095-11D5-964B-00C04FD6BE02}", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            RegCloseKey(hKey);
            return true;
        }
        return false;
    };

    static bool bRegisteredByUs = false;

    auto pattern = hook::pattern("8B 0E 89 44 24");
    static auto EnsureDInputMappingRegistered = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
    {
        if (isDiactfrmRegistered())
            return;

        std::wstring fullPath = getFullPath();
        if (GetFileAttributesW(fullPath.c_str()) == INVALID_FILE_ATTRIBUTES)
            return;

        runRegsvr32(L"/s \"" + fullPath + L"\"");
        bRegisteredByUs = true;
    });

    pattern = hook::pattern("8B C7 5F 5E 5D 5B 83 C4 44");
    static auto EnsureDInputMappingUnRegistered = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext& regs)
    {
        if (!bRegisteredByUs)
            return;

        std::wstring fullPath = getFullPath();
        runRegsvr32(L"/s /u \"" + fullPath + L"\"");
        bRegisteredByUs = false;
    });
}