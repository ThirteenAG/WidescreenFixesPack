module;

#include <stdafx.h>
#include <string>
#include <vector>
#include <stdexcept>

export module Paths;

import VideoModes;

export namespace GodfatherPaths
{
    SafetyHookInline registryHook;
    void*(__thiscall* assign)(void*, const char*, size_t){};
    std::string directory;

    std::string Directory(std::string_view module)
    {
        const auto slash = module.find_last_of("\\/");
        return slash == std::string_view::npos ? std::string{} : std::string(module.substr(0, slash + 1));
    }

    void* __cdecl Read(void* output, const void* section, const void* key, const void* fallback)
    {
        if (GodfatherVideoModes::Text(section).empty() && GodfatherVideoModes::Text(key) == "Install Dir")
        {
            for (size_t offset = 0; offset < 28; offset += sizeof(uint32_t))
                *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(output) + offset) = 0;
            *reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(output) + 24) = 15;
            return assign(output, directory.c_str(), directory.size());
        }
        return registryHook.unsafe_ccall<void*>(output, section, key, fallback);
    }

    bool TitleKey(const char* key)
    {
        if (!key)
            return false;
        constexpr const char* prefixes[]{ "Software\\Electronic Arts\\The Godfather The Game",
            "Software\\WOW6432Node\\Electronic Arts\\The Godfather The Game" };
        for (const auto prefix : prefixes)
        {
            const auto length = std::strlen(prefix);
            if (!_strnicmp(key, prefix, length) && (!key[length] || key[length] == '\\'))
                return true;
        }
        return false;
    }

    LSTATUS WINAPI CreateKey(HKEY root, LPCSTR key, DWORD reserved, LPSTR type, DWORD options, REGSAM access,
        const LPSECURITY_ATTRIBUTES security, PHKEY output, LPDWORD disposition)
    {
        if (root == HKEY_LOCAL_MACHINE && TitleKey(key))
        {
            if (output)
                *output = nullptr;
            if (disposition)
                *disposition = 0;
            return ERROR_ACCESS_DENIED;
        }
        return RegCreateKeyExA(root, key, reserved, type, options, access, security, output, disposition);
    }

    void Init()
    {
        std::vector<char> module(32768);
        const auto length = GetModuleFileNameA(nullptr, module.data(), DWORD(module.size()));
        if (length && length < module.size())
            directory = Directory(std::string_view(module.data(), length));
        auto read = hook::pattern("51 8B 44 24 14 8B 4C 24 10 8B 54 24 0C 56 8B 74 24 0C 50 51 52 68 02 00 00 80 "
            "56 C7 44 24 18 00 00 00 00 E8 ? ? ? ? 83 C4 14 8B C6 5E 59 C3");
        auto text = hook::pattern("6A 0C 33 DB BE 0F 00 00 00 68 ? ? ? ? 8D 4C 24 50 89 74 24 68 89 5C 24 64 88 5C 24 54 E8");
        if (!directory.empty())
        {
            assign = reinterpret_cast<decltype(assign)>(injector::GetBranchDestination(text.get_first<uint8_t>(30)).as_int());
            registryHook = safetyhook::create_inline(read.get_first(), Read);
        }
        IATHook::Replace(GetModuleHandleW(nullptr), "ADVAPI32.dll", std::forward_as_tuple("RegCreateKeyExA", CreateKey));
    }
}
