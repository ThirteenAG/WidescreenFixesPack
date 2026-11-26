module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <intrin.h>
#include <charconv>
#include <unordered_map>

export module Misc;

import ComVars;

// Skip intro: the logos at start-up
char __fastcall SkipLogo(const char*, void*, float, float)
{
    return 1;
}

void InitIntro()
{
    if (!FixSettings().Bool("MAIN.SkipIntro")) return;
    auto logos = hook::pattern("68 00 00 16 44 68 00 00 48 44 B9 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? 68 00 00 16 44 68 00 00 48 44 E8 ? ? ? ? B9 ? ? ? ? 68 00 00 C8 43 68 00 00 30 44 E8 ? ? ? ?");
    if (!logos.empty()) // 1.1 and 1.2
    {
        for (int offset : { 15, 35, 55 })
            injector::MakeCALL(logos.get_first(offset), SkipLogo, true);
    }
    else // 1.0
    {
        auto first = hook::pattern("84 C0 74 36 A1 ? ? ? ? 50 8B 08 FF 11 8B F0 8D 44 24 60");
        auto second = hook::pattern("84 C0 0F 84 8D 00 00 00 A1 ? ? ? ? 50 8B 10 FF 52 70 89 44 24 08");
        injector::WriteMemory<uint8_t>(first.get_first(2), 0xEB, true);
        injector::WriteMemory<uint16_t>(second.get_first(2), 0xE990, true);
    }
}

void RestoreGameOverSound()
{
    static constexpr char sound[] = "sounds\\music\\13 - Game Over.ogg";
    auto site = hook::pattern("85 FF 0F 84 ? ? ? ? 8B 0D ? ? ? ? 56 68 ? ? ? ? E8 ? ? ? ? 8B C8 E8");
    injector::WriteMemory<const char*>(site.get_first(16), sound, true);
}

void PreserveSniperBinding()
{
    auto update = hook::pattern("6A 3C 8D 86 50 03 00 00 53 89 8E F0 01 00 00 50 6A 1D 8B CE 89 96 9C 00 00 00 88 9E 91 00 00 00 E8 ? ? ? ? 85 C0 74 45 6A FF 8D 4C 24 14 53 51 6A 1D B9 ? ? ? ? 66 89 7C 24 24 66 C7 44 24 26 13 00 89 5C 24 28 89 5C 24 20 E8 ? ? ? ? 53 8B CE 85 C0 53 74 04 53 53 EB 03 6A 13 57 6A 1D E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 4C 24 20");
    if (!update.empty()) // 1.1 and 1.2
    {
        // Keep the saved binding and publish the profile without rewriting the sniper action.
        injector::WriteMemory<uint16_t>(update.get_first(39), 0x3EEB, true);
    }
    else // 1.0 already preserves the binding
    {
        auto native = hook::pattern("8B 46 70 8B 4E 64 8B 56 68 89 86 94 00 00 00 89 8E F0 01 00 00 89 96 9C 00 00 00 C6 86 91 00 00 00 00");
        native.get_first();
    }
}

// Mission programs run one command per frame and drop what a "wait" overshoots, so their loops speed up with the
// frame rate (Free Ride Extreme monster and bombs). Above 60 FPS, run them at 60 Hz and hand each
// program the time of the frames it sat out. Program starts and script events run unchanged.
namespace ScriptTiming
{
    constexpr double step = 1.0 / 60.0;
    SafetyHookInline runHook;
    uintptr_t startReturn = 0, eventReturn = 0;
    uint32_t presented = 0, decided = ~0u;
    void* first = nullptr;
    bool runFrame = true;
    double credit = step * 0.5;
    LARGE_INTEGER frequency{}, last{};
    std::unordered_map<void*, uint32_t> skipped;

    // Once per frame: time left over carries to the next frame, so 60 FPS and below run every frame
    void Decide()
    {
        if (runFrame)
            skipped.clear();
        LARGE_INTEGER now{};
        QueryPerformanceCounter(&now);
        const double real = last.QuadPart ? static_cast<double>(now.QuadPart - last.QuadPart) / frequency.QuadPart : step;
        last = now;
        credit = std::min(credit + real, step * 2.0);
        runFrame = credit >= step;
        if (runFrame)
            credit -= step;
    }

    char __fastcall Run(void* program, void*, uint32_t elapsed)
    {
        const auto caller = reinterpret_cast<uintptr_t>(_ReturnAddress());
        if (caller == startReturn || caller == eventReturn)
            return runHook.unsafe_thiscall<char>(program, elapsed);
        if (decided != presented || program == first)
        {
            decided = presented;
            first = program;
            Decide();
        }
        if (!runFrame)
        {
            skipped[program] += elapsed;
            return 0;
        }
        if (!skipped.empty())
        {
            if (auto it = skipped.find(program); it != skipped.end())
                elapsed += it->second;
        }
        return runHook.unsafe_thiscall<char>(program, elapsed);
    }

    void Init()
    {
        auto run = hook::pattern("56 57 8B 7C 24 0C 8B F1 8B 4E 10 85 C9 74 2B 8B 46 14 2B C1 8B 4E 1C C1 F8 03 3B C8 73 1C 8B 46 10 83 3C C8 06 75 04 C6 46 64 00");
        startReturn = reinterpret_cast<uintptr_t>(hook::get_pattern("6A 01 C7 41 44 00 00 00 00 C6 41 64 01 E8", 18));
        eventReturn = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 06 6A 01 8B CE FF 50 1C 6A 01 8B CE E8", 18));
        QueryPerformanceFrequency(&frequency);
        runHook = safetyhook::create_inline(run.get_first(), Run);
        WFP::onGameProcessEvent() += []()
        {
            ++presented;
        };
    }
}

namespace NativeSettings
{
    constexpr char GameKey[] = "Software\\Illusion Softworks\\Mafia";
    struct Key
    {
        HKEY root;
    };
    Key currentUser{ HKEY_CURRENT_USER }, localMachine{ HKEY_LOCAL_MACHINE };

    Key* Match(HKEY root, LPCSTR path)
    {
        if (!path || _stricmp(path, GameKey)) return nullptr;
        if (root == HKEY_CURRENT_USER) return &currentUser;
        return root == HKEY_LOCAL_MACHINE ? &localMachine : nullptr;
    }

    Key* PortableKey(HKEY handle)
    {
        auto* key = reinterpret_cast<Key*>(handle);
        return key == &currentUser || key == &localMachine ? key : nullptr;
    }

    LSTATUS WINAPI Open(HKEY root, LPCSTR path, PHKEY out)
    {
        const auto key = Match(root, path);
        if (!key) return RegOpenKeyA(root, path, out);
        if (!out) return ERROR_INVALID_PARAMETER;
        *out = reinterpret_cast<HKEY>(key);
        return ERROR_SUCCESS;
    }

    LSTATUS WINAPI Create(HKEY root, LPCSTR path, DWORD reserved, LPSTR cls, DWORD options, REGSAM access,
        SECURITY_ATTRIBUTES* security, PHKEY out, DWORD* disposition)
    {
        if (!Match(root, path))
            return RegCreateKeyExA(root, path, reserved, cls, options, access, security, out, disposition);
        if (disposition) *disposition = REG_OPENED_EXISTING_KEY;
        return Open(root, path, out);
    }

    LSTATUS WINAPI Close(HKEY key)
    {
        return PortableKey(key) ? ERROR_SUCCESS : RegCloseKey(key);
    }

    LSTATUS WINAPI Query(HKEY handle, LPCSTR name, DWORD* reserved, DWORD* type, BYTE* data, DWORD* size)
    {
        const auto key = PortableKey(handle);
        if (!key) return RegQueryValueExA(handle, name, reserved, type, data, size);
        if (reserved || !name || (data && !size)) return ERROR_INVALID_PARAMETER;
        std::optional<std::string> hex;
        if (!FixSettings().EditCfgFile([&](const std::filesystem::path& path)
        {
            const auto file = asi_settings::ReadFileText(path);
            if (!file) return false;
            const auto cfg = asi_settings::ParseCfg(*file);
            if (const auto* value = cfg.Find("Mafia", name))
                hex = *value;
            return true;
        })) return ERROR_READ_FAULT;
        if (!hex)
        {
            // Setup.exe supplies the initial engine configuration.
            return RegGetValueA(key->root, GameKey, name, RRF_RT_ANY, type, data, size);
        }
        if (hex->size() % 2 || hex->size() / 2 > MAXDWORD) return ERROR_INVALID_DATA;
        const DWORD length = static_cast<DWORD>(hex->size() / 2);
        const DWORD capacity = data ? *size : 0;
        if (size) *size = length;
        if (type) *type = REG_BINARY;
        if (!data) return ERROR_SUCCESS;
        if (capacity < length) return ERROR_MORE_DATA;
        for (DWORD i = 0; i < length; ++i)
        {
            const auto* digit = hex->data() + size_t(i) * 2;
            const auto parsed = std::from_chars(digit, digit + 2, data[i], 16);
            if (parsed.ec != std::errc{} || parsed.ptr != digit + 2) return ERROR_INVALID_DATA;
        }
        return ERROR_SUCCESS;
    }

    LSTATUS WINAPI Write(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size)
    {
        if (!PortableKey(key)) return RegSetValueExA(key, name, reserved, type, data, size);
        if (!name || reserved || (!data && size)) return ERROR_INVALID_PARAMETER;
        constexpr wchar_t digits[] = L"0123456789abcdef";
        std::wstring hex;
        hex.reserve(size_t(size) * 2);
        for (DWORD i = 0; i < size; ++i)
        {
            hex.push_back(digits[data[i] >> 4]);
            hex.push_back(digits[data[i] & 15]);
        }
        return FixSettings().EditCfgFile([&](const std::filesystem::path& file)
        {
            const auto wideName = std::filesystem::path(name).wstring();
            return WritePrivateProfileStringW(L"Mafia", wideName.c_str(), hex.c_str(), file.c_str()) != FALSE;
        }) ? ERROR_SUCCESS : ERROR_WRITE_FAULT;
    }
}

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            InitIntro();
            PreserveSniperBinding();
            RestoreGameOverSound();
            ScriptTiming::Init();

            IATHook::Replace(GetModuleHandleW(nullptr), "ADVAPI32.DLL",
                std::forward_as_tuple("RegCloseKey", NativeSettings::Close),
                std::forward_as_tuple("RegOpenKeyA", NativeSettings::Open),
                std::forward_as_tuple("RegCreateKeyExA", NativeSettings::Create),
                std::forward_as_tuple("RegQueryValueExA", NativeSettings::Query),
                std::forward_as_tuple("RegSetValueExA", NativeSettings::Write));
        };
    }
} Misc;
