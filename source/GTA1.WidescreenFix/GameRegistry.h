#pragma once

// The game's registry settings, kept per user: reads and writes of its machine-wide
// key go to the current user's copy, and values can be overridden from the ini.
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
    // Values the game reads from its registry keys can be overridden from the
    // fix's ini. Installers often leave the per-user keys read-only for the user,
    // so settings changed in game are kept there instead.
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
    inline LSTATUS WINAPI OpenKey(HKEY root, LPCSTR path, DWORD options, REGSAM access, PHKEY out)
    {
        if (root == HKEY_LOCAL_MACHINE && GameRegistryPath(path)) root = HKEY_CURRENT_USER;
        auto status = RegOpenKeyExA(root, path, options, access, out);
        TrackKey(status, path, out);
        return status;
    }
    inline LSTATUS WINAPI CreateKey(HKEY root, LPCSTR path, DWORD reserved, LPSTR cls,
        DWORD options, REGSAM access, const LPSECURITY_ATTRIBUTES attributes, PHKEY out, LPDWORD disposition)
    {
        if (root == HKEY_LOCAL_MACHINE && GameRegistryPath(path)) root = HKEY_CURRENT_USER;
        auto status = RegCreateKeyExA(root, path, reserved, cls, options, access, attributes, out, disposition);
        TrackKey(status, path, out);
        return status;
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
        return RegQueryValueExA(key, name, reserved, type, data, size);
    }
    inline LSTATUS WINAPI CloseKey(HKEY key)
    {
        {
            std::lock_guard lock(RegistryLock);
            GameKeys.erase(key);
        }
        return RegCloseKey(key);
    }
    inline void DefaultValue(const char* path, const char* name, DWORD value)
    {
        HKEY key;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
        DWORD size = sizeof(DWORD), type = 0, existing = 0;
        if (RegQueryValueExA(key, name, nullptr, &type, reinterpret_cast<BYTE*>(&existing), &size) == ERROR_FILE_NOT_FOUND)
            RegSetValueExA(key, name, 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(value));
        RegCloseKey(key);
    }
    inline void DefaultString(const char* path, const char* name, const char* value)
    {
        HKEY key;
        if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) return;
        DWORD size = 0;
        if (RegQueryValueExA(key, name, nullptr, nullptr, nullptr, &size) == ERROR_FILE_NOT_FOUND)
            RegSetValueExA(key, name, 0, REG_SZ, reinterpret_cast<const BYTE*>(value), static_cast<DWORD>(strlen(value) + 1));
        RegCloseKey(key);
    }
    inline void Install()
    {
        // On first use, retain the installation's machine-wide preferences.
        // Subsequent launches use only the current user's settings.
        for (auto path : { "Software\\DMA Design\\Grand Theft Auto" })
        {
            HKEY current = nullptr, machine = nullptr;
            auto existing = RegOpenKeyExA(HKEY_CURRENT_USER, path, 0, KEY_READ, &current);
            if (existing == ERROR_SUCCESS) { RegCloseKey(current); continue; }
            if (existing != ERROR_FILE_NOT_FOUND ||
                RegOpenKeyExA(HKEY_LOCAL_MACHINE, path, 0, KEY_READ, &machine) != ERROR_SUCCESS) continue;
            if (RegCreateKeyExA(HKEY_CURRENT_USER, path, 0, nullptr, 0, KEY_READ | KEY_WRITE, nullptr, &current, nullptr) == ERROR_SUCCESS)
            {
                RegCopyTreeA(machine, nullptr, current);
                RegCloseKey(current);
            }
            RegCloseKey(machine);
        }
        IATHook::Replace(GetModuleHandleW(nullptr), "advapi32.dll",
            std::make_tuple("RegOpenKeyExA", OpenKey), std::make_tuple("RegCreateKeyExA", CreateKey),
            std::make_tuple("RegQueryValueExA", QueryValue), std::make_tuple("RegCloseKey", CloseKey));
    }
}
