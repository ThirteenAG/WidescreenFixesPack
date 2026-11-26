module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/MaxPayne/Profile.hpp"

export module SettingsFile;

// Before the game sets its registry key
export void InitSettingsFile()
{
    auto& settings = FixSettings();
    if (!settings.Bool("MAIN.PortableSettings"))
        return;

    auto crt = GetModuleHandleA("msvcr71.dll");
    MaxPayneProfile::crtStrdup = (decltype(MaxPayneProfile::crtStrdup))GetProcAddress(crt, "_strdup");
    MaxPayneProfile::crtFree = (decltype(MaxPayneProfile::crtFree))GetProcAddress(crt, "free");

    auto mfc = GetModuleHandleA("mfc71.dll");
    ReplaceStartupImport(mfc, "KERNEL32.dll", "WritePrivateProfileStringA", &MaxPayneProfile::WriteProfile, MaxPayneProfile::NativeWriteProfile);
    MaxPayneProfile::shSetRegistryKey = safetyhook::create_inline(GetProcAddress(mfc, MAKEINTRESOURCEA(5975)), MaxPayneProfile::SetRegistryKey<0x50, 0x68>); // MaxPayneProfile::SetRegistryKey(LPCTSTR)
}
