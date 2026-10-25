module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module System;

// Things that make the PC port behave: no registry needed, no "safe mode" reset to 640x480 after a crash, no
// hardware check that fails on modern PCs (without its side effects: it also turned off DirectSound effects and
// realtime shadows and limited the resolution list), stereo sound that actually pans (the setting was never
// applied at startup, so "Stereo" played in mono), cutscenes at 30 FPS instead of 15, the four European languages
// the PC release hides, and an optional start straight at the title menu.

namespace
{
    // The game's movie-path initialiser tests the returned key handle for null, which RegistryWrapper uses
    // internally, so it gets a private non-null handle instead
    HKEY PortableKey = reinterpret_cast<HKEY>(UINT_PTR(0x534834));

    LSTATUS WINAPI RegOpen(HKEY root, LPCSTR path, PHKEY out)
    {
        auto result = RegistryWrapper::RegOpenKeyA(root, path, out);
        if (result == ERROR_SUCCESS && out && !*out)
            *out = PortableKey;
        return result;
    }

    LSTATUS WINAPI RegQuery(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
    {
        return RegistryWrapper::RegQueryValueExA(key == PortableKey ? nullptr : key, name, reserved, type, data, size);
    }

    LSTATUS WINAPI RegSet(HKEY key, LPCSTR name, DWORD reserved, DWORD type, const BYTE* data, DWORD size)
    {
        return RegistryWrapper::RegSetValueExA(key == PortableKey ? nullptr : key, name, reserved, type, data, size);
    }

    LSTATUS WINAPI RegClose(HKEY key)
    {
        return RegistryWrapper::RegCloseKey(key == PortableKey ? nullptr : key);
    }

    LSTATUS WINAPI RegDelete(HKEY key, LPCSTR name)
    {
        return key == PortableKey ? ERROR_SUCCESS : ::RegDeleteValueA(key, name);
    }

    // Languages: 1 English, 2 French, 3 German, 4 Italian, 5 Spanish (0 Japanese and 6 Korean need other fonts)
    uint8_t* Config = nullptr;      // the 36-byte options block
    uint8_t(__cdecl* PadTrigger)(int) = nullptr;
    int(__cdecl* PadAction)(int, int) = nullptr;
    int(__cdecl* MouseClick)() = nullptr;
    int(__cdecl* PlaySound)(int) = nullptr;
    int32_t* LeftArrowHovered = nullptr;
    int32_t* RightArrowHovered = nullptr;

    // The Language row of the options: the native one only toggles Japanese and English
    int __cdecl LanguageRow(uint8_t* config, int)
    {
        uint8_t trigger = PadTrigger(0);
        bool left = (trigger & 4) || (*LeftArrowHovered && MouseClick());
        bool right = !left && ((trigger & 8) || PadAction(0, 17) || (*RightArrowHovered && MouseClick()));
        if (!left && !right)
            return 0;
        int language = config[17];
        if (language < 1 || language > 5)
            language = 1;
        language += left ? -1 : 1;
        if (language > 5) language = 1;
        if (language < 1) language = 5;
        config[17] = uint8_t(language);
        PlaySound(40003);
        return 0;
    }
}

export namespace System
{
    void Init()
    {
        // Registry: the game only keeps its install paths, the setup language and a safe mode flag there
        {
            char language[64] = "English";
            HKEY installed = nullptr;
            if (::RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Konami\\SILENT HILL 4\\1.00.000", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &installed) == ERROR_SUCCESS)
            {
                char value[64] = {};
                DWORD size = sizeof(value), type = 0;
                if (::RegQueryValueExA(installed, "Install Language", nullptr, &type, reinterpret_cast<BYTE*>(value), &size) == ERROR_SUCCESS &&
                    type == REG_SZ && value[0] && size < sizeof(value))
                    strcpy_s(language, value);
                ::RegCloseKey(installed);
            }
            RegistryWrapper("Konami", GetThisModulePath<std::filesystem::path>() / "SilentHill4.registry.ini");
            RegistryWrapper::AddPathWriter("Install Path", "Movie Install");
            RegistryWrapper::AddDefault("Install Language", language);
            IATHook::Replace(GetModuleHandleW(nullptr), "advapi32.dll",
                std::forward_as_tuple("RegOpenKeyA", &RegOpen),
                std::forward_as_tuple("RegQueryValueExA", &RegQuery),
                std::forward_as_tuple("RegSetValueExA", &RegSet),
                std::forward_as_tuple("RegDeleteValueA", &RegDelete),
                std::forward_as_tuple("RegCloseKey", &RegClose));
            // The movie path may have been read before the plugin started: read it again
            auto moviePath = hook::pattern("83 EC 0C 8D 44 24 00 50 68 ? ? ? ? 68 02 00 00 80 C7 44 24 14 01 00 00 00 C7 44 24 10 04 01 00 00 FF 15");
            if (moviePath.size() == 1)
                reinterpret_cast<char(__cdecl*)()>(moviePath.get_first())();
        }

        // Safe mode: after a crash or Alt+F4 the game showed a warning and reset the resolution to 640x480
        auto safeMode = hook::pattern("83 C8 FF 83 C4 10 C3");
        if (safeMode.size() == 1)
            injector::MakeNOP(safeMode.get_first(-2), 2, true);

        // Hardware check: CheckSpec.dll fails on modern systems; the check is skipped and the results it would
        // give a modern PC filled in (top CPU class: DirectSound effects and realtime shadows; plenty of video memory)
        auto checkSpec = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 8B 0D ? ? ? ? 89 17 66");
        auto cpuClass = hook::pattern("3D DE 03 00 00 73 08 89 1D ? ? ? ? EB");
        auto videoMemory = hook::pattern("A1 ? ? ? ? 56 33 DB 33 F6 33 ED 3D 00 00 B0 03");
        if (checkSpec.size() == 1)
        {
            injector::MakeNOP(checkSpec.get_first(), 6, true);
            if (cpuClass.size() == 1)
                **cpuClass.get_first<int32_t*>(9) = 4;
            if (videoMemory.size() == 1)
                **videoMemory.get_first<int32_t*>(1) = 0x10000000;
        }

        // Stereo panning: the sound library caches "Stereo" as already applied, so the saved Stereo setting never
        // reached the engine and sounds played in mono
        auto soundMode = hook::pattern("68 00 00 80 3F C7 05 ? ? ? ? 01 00 00 00 C7 05 ? ? ? ? FF FF FF FF");
        if (soundMode.size() == 1)
            injector::WriteMemory<int32_t>(soundMode.get_first(11), -1, true);

        // Cutscenes at 30 FPS instead of 15
        auto cutsceneRate = hook::pattern("0F 94 C0 A3 ? ? ? ? EB ? C3");
        if (cutsceneRate.size() == 1)
            injector::WriteMemory<uint8_t>(cutsceneRate.get_first(1), 0x95, true);

        // Start at the title menu: the boot scene skips the logos and the opening
        auto boot = hook::pattern("A1 ? ? ? ? 85 C0 75 15 6A 01 E8 ? ? ? ?");
        auto gamma = hook::pattern("55 8B EC 81 EC 54 06 00 00 A1 ? ? ? ?");
        auto config = hook::pattern("B8 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 A1 ? ? ? ? C3 90 90 90 90 90 90 90 90 90 90 8B 54 24 04");
        if (config.size() == 1)
            Config = *config.get_first<uint8_t*>(1);
        if (FixSettings().Int("MAIN.SkipIntro") && boot.size() == 1)
        {
            // The boot scene goes to the title as if the logos had played (the flag itself is cleared at startup)
            injector::WriteMemory<uint8_t>(boot.get_first(7), 0xEB, true);
            // The logo sequence applied the saved gamma; do it when the title appears
            auto title = hook::pattern("83 EC 08 53 56 33 DB 53 33 F6 E8 ? ? ? ?");
            if (title.size() == 1 && gamma.size() == 1 && Config)
            {
                static auto ApplyGamma = reinterpret_cast<void(__cdecl*)(int)>(gamma.get_first());
                static SafetyHookInline TitleHook;
                TitleHook = safetyhook::create_inline(title.get_first(), static_cast<int(__cdecl*)()>([]() -> int
                {
                    static bool once = false;
                    if (!once)
                    {
                        once = true;
                        ApplyGamma(*reinterpret_cast<int32_t*>(Config + 28));
                    }
                    return TitleHook.ccall<int>();
                }));
            }
        }

        // The French, German, Italian and Spanish text is in the game but can't be chosen: the language is
        // limited to Japanese or English when loading and in the options
        auto clampLoad = hook::pattern("38 1D ? ? ? ? 7E 06 88 1D ? ? ? ?");
        auto clampCopy = hook::pattern("8A 0D ? ? ? ? B0 01 3A C8 5F 5E 7E 05");
        auto languageRow = hook::pattern("8A 41 11 84 C0 75 15 68 43 9C 00 00 C6 41 11 01");
        auto optionsExit = hook::pattern("56 57 8B F3 BF 03 00 00 00 8D A4 24 00 00 00 00 56 E8");
        if (clampLoad.size() == 1 && clampCopy.size() == 1 && languageRow.size() == 1 && optionsExit.size() == 1)
        {
            auto row = languageRow.get_first<uint8_t>(-0x47);
            if (memcmp(row, "\x53\x6A\x00\xE8", 4) == 0 && row[0x0E] == 0xE8 && row[0x1A] == 0xA1 && row[0x22] == 0xE8 && row[0x2C] == 0xA1 && row[0x57] == 0xE8)
            {
                PadTrigger = reinterpret_cast<decltype(PadTrigger)>(injector::GetBranchDestination(row + 3).as_int());
                PadAction = reinterpret_cast<decltype(PadAction)>(injector::GetBranchDestination(row + 0x0E).as_int());
                LeftArrowHovered = *reinterpret_cast<int32_t**>(row + 0x1B);
                MouseClick = reinterpret_cast<decltype(MouseClick)>(injector::GetBranchDestination(row + 0x23).as_int());
                RightArrowHovered = *reinterpret_cast<int32_t**>(row + 0x2D);
                PlaySound = reinterpret_cast<decltype(PlaySound)>(injector::GetBranchDestination(row + 0x57).as_int());

                // The row table holds the activate function; point it at the new one
                auto module = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
                auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(module + reinterpret_cast<IMAGE_DOS_HEADER*>(module)->e_lfanew);
                auto section = IMAGE_FIRST_SECTION(nt);
                for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
                {
                    if (!(section->Characteristics & IMAGE_SCN_MEM_WRITE) && !(section->Characteristics & IMAGE_SCN_CNT_INITIALIZED_DATA))
                        continue;
                    if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                        continue;
                    auto begin = reinterpret_cast<uintptr_t*>(module + section->VirtualAddress);
                    auto end = begin + section->Misc.VirtualSize / sizeof(uintptr_t);
                    for (auto it = begin; it < end; ++it)
                    {
                        // {activate, get, flash, values, count 7, label 0x2A, help}
                        if (*it == reinterpret_cast<uintptr_t>(row) && it + 5 < end && it[4] == 7 && it[5] == 0x2A)
                            injector::WriteMemory(it, &LanguageRow, true);
                    }
                }

                injector::WriteMemory<uint8_t>(clampLoad.get_first(6), 0xEB, true);     // jle -> jmp
                injector::WriteMemory<uint8_t>(clampCopy.get_first(12), 0xEB, true);

                // Leaving the options with another language loads that language's text
                static SafetyHookMid ExitHook = safetyhook::create_mid(optionsExit.get_first(), [](SafetyHookContext& ctx)
                {
                    auto options = reinterpret_cast<uint8_t*>(ctx.ebx);
                    if (options[176 + 17] != options[212 + 17])
                        *reinterpret_cast<int32_t*>(options + 268) = 1;
                });
            }
        }
    }
}
