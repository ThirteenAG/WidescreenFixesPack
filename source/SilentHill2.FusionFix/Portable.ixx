module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"


export module Portable;


export namespace SH2Portable
{
    inline int Key;
    inline decltype(&RegOpenKeyA) OpenKey = nullptr;
    inline decltype(&RegQueryValueExA) QueryValue = nullptr;
    inline decltype(&RegCloseKey) CloseKey = nullptr;

    inline LSTATUS WINAPI Open(HKEY key, LPCSTR name, PHKEY result)
    {
        if (name && asi_settings::text::ToLower(name).find("konami\\silent hill 2") != std::string::npos && result)
        {
            *result = reinterpret_cast<HKEY>(&Key);
            return ERROR_SUCCESS;
        }
        return OpenKey(key, name, result);
    }

    inline LSTATUS WINAPI Query(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
    {
        if (key != reinterpret_cast<HKEY>(&Key))
            return QueryValue(key, name, reserved, type, data, size);
        if (!name || !size)
            return ERROR_INVALID_PARAMETER;
        std::string value;
        if (_stricmp(name, "INSTALLDIR") == 0)
            value = GetExeModulePath<std::string>();
        else if (_stricmp(name, "LANGUAGE") == 0)
            value = "English";
        else if (_stricmp(name, "NO_DISC") == 0 || _stricmp(name, "WRONG_DISC") == 0)
            value = "Please insert the Silent Hill 2 disc.";
        else
            return ERROR_FILE_NOT_FOUND;
        const DWORD needed = DWORD(value.size() + 1);
        if (type)
            *type = REG_SZ;
        const DWORD capacity = *size;
        *size = needed;
        if (!data)
            return ERROR_SUCCESS;
        if (capacity < needed)
            return ERROR_MORE_DATA;
        memcpy(data, value.c_str(), needed);
        return ERROR_SUCCESS;
    }

    inline LSTATUS WINAPI Close(HKEY key)
    {
        return key == reinterpret_cast<HKEY>(&Key) ? ERROR_SUCCESS : CloseKey(key);
    }

    inline void Init()
    {
        auto hooks = IATHook::Replace(GetModuleHandleW(nullptr), "advapi32.dll",
            std::forward_as_tuple("RegOpenKeyA", &Open),
            std::forward_as_tuple("RegQueryValueExA", &Query),
            std::forward_as_tuple("RegCloseKey", &Close));
        OpenKey = reinterpret_cast<decltype(OpenKey)>(hooks.at("RegOpenKeyA").get());
        QueryValue = reinterpret_cast<decltype(QueryValue)>(hooks.at("RegQueryValueExA").get());
        CloseKey = reinterpret_cast<decltype(CloseKey)>(hooks.at("RegCloseKey").get());
    }
}
