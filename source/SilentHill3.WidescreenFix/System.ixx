module;

#include <stdafx.h>
#include <mmsystem.h>
#include <dsound.h>
#include "FixSettings.hpp"

export module System;

// Things that make the PC port behave: no registry needed, no "safe mode" reset of the display settings after a
// crash or Alt+F4, no long freeze every time Options opens, a steady 60 FPS instead of a jump between 30 and 60,
// no white window border, and an optional start straight at the title menu.

export namespace System
{
    // The game (and SH3Config.exe) read their install paths from HKLM\SOFTWARE\KONAMI. They are answered from the
    // game folder instead. Returns true for SH3Config.exe, where nothing else of the fix applies.
    bool PatchRegistry()
    {
        auto config = hook::pattern("50 53 53 FF 75 F4 C7 45 F0 10 00 00 00 FF 15");
        RegistryWrapper("KONAMI", "");
        RegistryWrapper::AddPathWriter("movie", "sound", "data", "installdir");
        RegistryWrapper::AddPathWriterWithPath("save", "savedata");
        if (config.size() > 0)
        {
            auto iat = *config.get(0).get<uintptr_t*>(15);
            injector::WriteMemory(&iat[0], RegistryWrapper::RegQueryValueExA, true);
            injector::WriteMemory(&iat[1], RegistryWrapper::RegOpenKeyExA, true);
            injector::WriteMemory(&iat[2], RegistryWrapper::RegDeleteKeyA, true);
            injector::WriteMemory(&iat[3], RegistryWrapper::RegEnumKeyA, true);
            injector::WriteMemory(&iat[4], RegistryWrapper::RegOpenKeyA, true);
            injector::WriteMemory(&iat[5], RegistryWrapper::RegQueryValueA, true);
            injector::WriteMemory(&iat[6], RegistryWrapper::RegCreateKeyExA, true);
            injector::WriteMemory(&iat[7], RegistryWrapper::RegSetValueExA, true);
            injector::WriteMemory(&iat[8], RegistryWrapper::RegCloseKey, true);
            return true;
        }

        auto game = hook::pattern("68 02 00 00 80 FF 15");
        if (game.size() > 0)
        {
            auto iat = *game.get(0).get<uintptr_t*>(7);
            injector::WriteMemory(&iat[0], RegistryWrapper::RegOpenKeyExA, true);
            injector::WriteMemory(&iat[1], RegistryWrapper::RegQueryValueExA, true);
            injector::WriteMemory(&iat[2], RegistryWrapper::RegOpenKeyA, true);
            injector::WriteMemory(&iat[3], RegistryWrapper::RegCloseKey, true);
        }
        return false;
    }

    void Init()
    {
        // Safe mode: the game empties savedata\disp.ini at startup and only writes it again on a clean exit, so a
        // crash resets the display settings and shows a warning. The file is no longer emptied, and no warning.
        static const char* NoMode = "";
        auto safeMode = hook::pattern("BF 01 00 00 00 68 ? ? ? ? 68 ? ? ? ? 6A 01");
        if (safeMode.size() == 1)
        {
            injector::WriteMemory(safeMode.get_first(1), 0, true);
            injector::WriteMemory(safeMode.get_first(6), &NoMode, true);
        }

        // Options freeze: every time Options opens, the game test-switches the desktop to each of the hundreds of
        // display modes modern drivers report, twice. The list only fed the native Resolution row with 4:3 modes,
        // and the resolution is the fix's (the desktop's by default).
        auto modeList = hook::pattern("81 EC D8 00 00 00 55 8B AC 24 E0 00");
        if (modeList.size() == 1)
            injector::MakeRET(modeList.get_first());

        // The window gets no white edge on Windows 10 and later
        auto windowStyle = hook::pattern("68 00 03 00 00 FF 15 ? ? ? ? 5E");
        if (windowStyle.size() == 1)
            injector::WriteMemory(windowStyle.get_first(1), 0, true);

        // Steady 60 FPS: the frame pacing counted a frame late and kept falling back to 30 FPS
        auto pacing = hook::pattern("4A 03 C8 A3 ? ? ? ? 89 15");
        if (pacing.size() == 1)
            injector::WriteMemory<uint8_t>(pacing.get_first(), 0x42, true);

        // Start at the title menu: the first-boot flag that plays the logos, the content warning and the opening
        // movie stays off, so the game takes the path it takes when returning to the title
        auto firstBoot = hook::pattern("33 C0 A3 ? ? ? ? A3 ? ? ? ? C6 05 ? ? ? ? 01 66 A3");
        if (firstBoot.size() == 1 && FixSettings().Int("MAIN.SkipIntro"))
            injector::WriteMemory<uint8_t>(firstBoot.get_first(0x12), 0, true);

        // Sound effects that went missing on some systems (#421): every effect is created with DirectSound effects
        // for the area reverb, and when the buffer or the reverb can't be had the game dropped the sound. It now
        // plays without the reverb instead.
        static decltype(&DirectSoundCreate8) CreateSound = nullptr;
        static HRESULT(STDMETHODCALLTYPE* CreateBuffer)(IDirectSound8*, LPCDSBUFFERDESC, LPDIRECTSOUNDBUFFER*, LPUNKNOWN) = nullptr;
        auto sound = IATHook::Replace(GetModuleHandleW(nullptr), "dsound.dll",
            std::forward_as_tuple("DirectSoundCreate8@11", static_cast<decltype(&DirectSoundCreate8)>([](LPCGUID device, LPDIRECTSOUND8* out, LPUNKNOWN outer) -> HRESULT
            {
                HRESULT result = CreateSound(device, out, outer);
                if (SUCCEEDED(result) && out && *out)
                {
                    auto vtable = *reinterpret_cast<void***>(*out);
                    if (!CreateBuffer)
                    {
                        CreateBuffer = reinterpret_cast<decltype(CreateBuffer)>(vtable[3]);
                        injector::WriteMemory(&vtable[3], static_cast<decltype(CreateBuffer)>([](IDirectSound8* ds, LPCDSBUFFERDESC desc, LPDIRECTSOUNDBUFFER* buffer, LPUNKNOWN outer) -> HRESULT
                        {
                            HRESULT result = CreateBuffer(ds, desc, buffer, outer);
                            if (FAILED(result) && desc && (desc->dwFlags & DSBCAPS_CTRLFX))
                            {
                                DSBUFFERDESC dry = *desc;
                                dry.dwFlags &= ~DSBCAPS_CTRLFX;
                                result = CreateBuffer(ds, &dry, buffer, outer);
                            }
                            return result;
                        }), true);
                    }
                }
                return result;
            })));
        if (auto it = sound.find("DirectSoundCreate8@11"); it != sound.end())
            CreateSound = reinterpret_cast<decltype(CreateSound)>(it->second.get());
        auto reverb = hook::pattern("E8 ? ? ? ? 83 C4 04 85 C0 7D 19 53 68");
        if (reverb.size() == 1)
            injector::WriteMemory<uint8_t>(reverb.get_first(10), 0xEB, true);

        // Silent Hill 2 references of the PC release in new games (from sh3proxy)
        if (FixSettings().Bool("MAIN.SH2Reference"))
        {
            auto reference = hook::pattern("A0 ? ? ? ? 84 C0 ? ? E8 ? ? ? ? A2 ? ? ? ? C6 05");
            if (reference.size() == 1)
                injector::WriteMemory<uint8_t>(reference.get_first(), 0xC3, true);
        }
    }
}
