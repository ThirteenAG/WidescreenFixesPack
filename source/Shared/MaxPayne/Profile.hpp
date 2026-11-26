#pragma once

#include "Shared/StartupImport.hpp"

// MFC stores native settings in the plugin cfg when its registry key is unset.
namespace MaxPayneProfile
{
    // MFC frees the profile name through its own CRT.
    inline char* (__cdecl* crtStrdup)(const char* szString) = nullptr;
    inline void(__cdecl* crtFree)(void* pBlock) = nullptr;
    inline std::string ProfilePath;
    inline decltype(&WritePrivateProfileStringA) NativeWriteProfile = WritePrivateProfileStringA;

    inline BOOL WINAPI WriteProfile(LPCSTR section, LPCSTR key, LPCSTR value, LPCSTR file)
    {
        if (!file || ProfilePath.empty() || _stricmp(file, ProfilePath.c_str()) != 0)
            return NativeWriteProfile(section, key, value, file);
        return FixSettings().EditCfgFile([&](const std::filesystem::path& path)
        {
            return NativeWriteProfile(section, key, value, path.string().c_str()) != FALSE;
        });
    }

    struct RegistryKey
    {
        HKEY value = nullptr;
        ~RegistryKey()
        {
            if (value)
                RegCloseKey(value);
        }
    };

    inline bool SeedSection(HKEY section, const char* name, const asi_settings::CfgFile& existing, const std::string& path)
    {
        DWORD maxName = 0, maxData = 0;
        if (RegQueryInfoKeyA(section, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
            nullptr, &maxName, &maxData, nullptr, nullptr) != ERROR_SUCCESS)
            return false;
        std::vector<char> key(size_t(maxName) + 1);
        std::vector<BYTE> data(size_t(maxData) + 1);
        for (DWORD index = 0; ; ++index)
        {
            DWORD nameSize = DWORD(key.size()), dataSize = maxData, type = REG_NONE;
            const auto status = RegEnumValueA(section, index, key.data(), &nameSize, nullptr, &type, data.data(), &dataSize);
            if (status == ERROR_NO_MORE_ITEMS)
                return true;
            if (status != ERROR_SUCCESS)
                return false;
            if (!nameSize || existing.Find(name, key.data()))
                continue;
            std::string value;
            if (type == REG_DWORD && dataSize == sizeof(int32_t))
            {
                int32_t number;
                std::memcpy(&number, data.data(), sizeof(number));
                value = std::to_string(number);
            }
            else if (type == REG_SZ)
            {
                data[dataSize] = 0;
                value.assign(reinterpret_cast<const char*>(data.data()));
            }
            else
                continue;
            if (!NativeWriteProfile(name, key.data(), value.c_str(), path.c_str()))
                return false;
        }
    }

    inline bool CopyRegistrySettings(const std::string& key, const std::string& path)
    {
        RegistryKey app;
        const auto opened = RegOpenKeyExA(HKEY_CURRENT_USER, key.c_str(), 0, KEY_READ, &app.value);
        if (opened == ERROR_FILE_NOT_FOUND || opened == ERROR_PATH_NOT_FOUND)
            return true;
        if (opened != ERROR_SUCCESS)
            return false;
        DWORD error = ERROR_SUCCESS;
        const auto content = asi_settings::ReadFileText(path, &error);
        if (!content && error != ERROR_FILE_NOT_FOUND)
            return false;
        const auto existing = asi_settings::ParseCfg(content.value_or(""));
        for (DWORD index = 0; ; ++index)
        {
            char name[256];
            DWORD size = sizeof(name);
            const auto status = RegEnumKeyExA(app.value, index, name, &size, nullptr, nullptr, nullptr, nullptr);
            if (status == ERROR_NO_MORE_ITEMS)
                return true;
            RegistryKey section;
            if (status != ERROR_SUCCESS || RegOpenKeyExA(app.value, name, 0, KEY_READ, &section.value) != ERROR_SUCCESS ||
                !SeedSection(section.value, name, existing, path))
                return false;
        }
    }

    inline SafetyHookInline shSetRegistryKey = {};
    template<size_t AppName, size_t ProfileName>
    void __fastcall SetRegistryKey(uint8_t* _this, void* edx, const char* szRegistryKey)
    {
        const bool seeded = FixSettings().EditCfgFile([&](const std::filesystem::path& path)
        {
            ProfilePath = path.string();
            return CopyRegistrySettings(std::string("Software\\") + szRegistryKey + "\\" + *(const char**)(_this + AppName), ProfilePath);
        });

        if (!seeded)
            OutputDebugStringA("Portable profile initialization incomplete.\n");
        auto& szProfileName = *(char**)(_this + ProfileName);
        crtFree(szProfileName);
        szProfileName = crtStrdup(ProfilePath.c_str());
    }
}

