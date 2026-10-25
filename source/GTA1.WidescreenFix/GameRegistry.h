#pragma once
#include <set>

// Read installed game settings without changing registry keys or values.
// The fix's options persist to CFG; native registry writes stay in memory.
namespace GameRegistry
{
    inline bool GameRegistryPath(LPCSTR path)
    {
        if (!path) return false;
        for (auto prefix : { "Software\\DMA Design\\Grand Theft Auto" })
        {
            auto length = strlen(prefix);
            if (_strnicmp(path, prefix, length) == 0 && (path[length] == '\0' || path[length] == '\\')) return true;
        }
        return false;
    }
    // CFG overrides and native writes are served from memory.
    inline std::mutex RegistryLock;
    inline std::map<HKEY, std::string> GameKeys;
    inline std::map<std::string, DWORD> RegistryOverrides;
    inline std::string RegistryName(std::string path, std::string name)
    {
        for (auto& c : path) c = char(tolower(uint8_t(c)));
        for (auto& c : name) c = char(tolower(uint8_t(c)));
        return path + "|" + name;
    }
    inline void SetOverride(const char* path, const char* name, DWORD value)
    {
        std::lock_guard lock(RegistryLock);
        RegistryOverrides[RegistryName(path, name)] = value;
    }
    inline bool Override(const char* path, const char* name, DWORD& value)
    {
        std::lock_guard lock(RegistryLock);
        auto found = RegistryOverrides.find(RegistryName(path, name ? name : ""));
        if (found == RegistryOverrides.end()) return false;
        value = found->second;
        return true;
    }
    inline void TrackKey(LSTATUS status, LPCSTR path, PHKEY out)
    {
        if (status != ERROR_SUCCESS || !out || !GameRegistryPath(path)) return;
        std::lock_guard lock(RegistryLock);
        GameKeys[*out] = path;
    }
    inline std::set<HKEY> VirtualKeys;
    inline std::string FullPath(HKEY root, LPCSTR path)
    {
        std::lock_guard lock(RegistryLock);
        const auto parent = GameKeys.find(root);
        if (parent != GameKeys.end())
            return parent->second + (path && *path ? "\\" + std::string(path) : "");
        return path ? path : "";
    }
    inline LSTATUS WINAPI OpenKey(HKEY root, LPCSTR path, DWORD options, REGSAM access, PHKEY out)
    {
        const auto fullPath = FullPath(root, path);
        if (!GameRegistryPath(fullPath.c_str()))
            return RegOpenKeyExA(root, path, options, access, out);
        if (!out)
            return ERROR_INVALID_PARAMETER;
        // Installed values are read-only. Missing game keys exist in memory.
        auto status = RegOpenKeyExA(HKEY_CURRENT_USER, fullPath.c_str(), options, KEY_READ, out);
        if (status != ERROR_SUCCESS)
            status = RegOpenKeyExA(HKEY_LOCAL_MACHINE, fullPath.c_str(), options, KEY_READ, out);
        if (status != ERROR_SUCCESS)
        {
            auto token = CreateEventW(nullptr, FALSE, FALSE, nullptr);
            if (!token)
                return ERROR_NOT_ENOUGH_MEMORY;
            *out = reinterpret_cast<HKEY>(token);
            std::lock_guard lock(RegistryLock);
            VirtualKeys.insert(*out);
            status = ERROR_SUCCESS;
        }
        TrackKey(status, fullPath.c_str(), out);
        return status;
    }
    inline LSTATUS WINAPI CreateKey(HKEY root, LPCSTR path, DWORD reserved, LPSTR cls,
        DWORD options, REGSAM access, const LPSECURITY_ATTRIBUTES attributes, PHKEY out, LPDWORD disposition)
    {
        const auto fullPath = FullPath(root, path);
        if (!GameRegistryPath(fullPath.c_str()))
            return RegCreateKeyExA(root, path, reserved, cls, options, access, attributes, out, disposition);
        const auto status = OpenKey(root, path, options, KEY_READ, out);
        if (status == ERROR_SUCCESS && disposition)
        {
            std::lock_guard lock(RegistryLock);
            *disposition = VirtualKeys.count(*out) ? REG_CREATED_NEW_KEY : REG_OPENED_EXISTING_KEY;
        }
        return status;
    }
    inline LSTATUS WINAPI SetValue(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size)
    {
        std::string path;
        {
            std::lock_guard lock(RegistryLock);
            const auto found = GameKeys.find(key);
            if (found != GameKeys.end())
                path = found->second;
        }
        if (path.empty())
            return RegSetValueExA(key, name, reserved, type, data, size);
        if (!data)
            return ERROR_INVALID_PARAMETER;
        if (type == REG_DWORD && size == sizeof(DWORD))
        {
            DWORD value{};
            memcpy(&value, data, sizeof(value));
            SetOverride(path.c_str(), name ? name : "", value);
            return ERROR_SUCCESS;
        }
        return ERROR_UNSUPPORTED_TYPE;
    }
    inline LSTATUS WINAPI QueryValue(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
    {
        std::string path;
        {
            std::lock_guard lock(RegistryLock);
            auto found = GameKeys.find(key);
            if (found != GameKeys.end()) path = found->second;
        }
        DWORD value = 0;
        if (!path.empty() && Override(path.c_str(), name, value))
        {
            if (type) *type = REG_DWORD;
            if (!size) return data ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS;
            if (!data) { *size = sizeof(DWORD); return ERROR_SUCCESS; }
            if (*size < sizeof(DWORD)) { *size = sizeof(DWORD); return ERROR_MORE_DATA; }
            memcpy(data, &value, sizeof(value)); *size = sizeof(DWORD);
            return ERROR_SUCCESS;
        }
        {
            std::lock_guard lock(RegistryLock);
            if (VirtualKeys.count(key))
                return ERROR_FILE_NOT_FOUND;
        }
        return RegQueryValueExA(key, name, reserved, type, data, size);
    }
    inline LSTATUS WINAPI CloseKey(HKEY key)
    {
        {
            std::lock_guard lock(RegistryLock);
            GameKeys.erase(key);
            if (VirtualKeys.erase(key))
            {
                CloseHandle(reinterpret_cast<HANDLE>(key));
                return ERROR_SUCCESS;
            }
        }
        return RegCloseKey(key);
    }
    inline void DefaultValue(const char* path, const char* name, DWORD fallback)
    {
        DWORD value = 0, size = sizeof(value);
        if (Override(path, name, value)) return;
        if (RegGetValueA(HKEY_CURRENT_USER, path, name, RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS ||
            RegGetValueA(HKEY_LOCAL_MACHINE, path, name, RRF_RT_REG_DWORD, nullptr, &value, &size) == ERROR_SUCCESS) return;
        SetOverride(path, name, fallback);
    }
    inline void Install()
    {
        IATHook::Replace(GetModuleHandleW(nullptr), "advapi32.dll",
            std::make_tuple("RegOpenKeyExA", OpenKey), std::make_tuple("RegCreateKeyExA", CreateKey),
            std::make_tuple("RegQueryValueExA", QueryValue), std::make_tuple("RegSetValueExA", SetValue), std::make_tuple("RegCloseKey", CloseKey));
    }
}
