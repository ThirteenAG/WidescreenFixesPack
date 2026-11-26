module;

#include <stdafx.h>
#include <mmsystem.h>
#include <dsound.h>
#include "FixSettings.hpp"
#include "Shared/SilentHill/Display.hpp"

export module System;

import ComVars;

// Things that make the PC port behave: no registry needed, no "safe mode" reset of the display settings after a
// crash or Alt+F4, no long freeze every time Options opens, a steady 60 FPS instead of a jump between 30 and 60,
// no white window border, and an optional start straight at the title menu.

namespace Registry
{
    namespace
    {
        HKEY InstallKey()
        {
            static uint8_t key;
            return reinterpret_cast<HKEY>(&key);
        }

        bool IsInstallKey(HKEY root, LPCSTR name)
        {
            return root == HKEY_LOCAL_MACHINE && name && _stricmp(name, "SOFTWARE\\KONAMI\\SILENT HILL 3") == 0;
        }

        LSTATUS WINAPI Open(HKEY root, LPCSTR name, PHKEY key)
        {
            if (!IsInstallKey(root, name))
                return ::RegOpenKeyA(root, name, key);
            if (!key)
                return ERROR_INVALID_PARAMETER;
            *key = InstallKey();
            return ERROR_SUCCESS;
        }

        LSTATUS WINAPI OpenEx(HKEY root, LPCSTR name, DWORD options, REGSAM access, PHKEY key)
        {
            if (!IsInstallKey(root, name))
                return ::RegOpenKeyExA(root, name, options, access, key);
            return Open(root, name, key);
        }

        LSTATUS WINAPI Query(HKEY key, LPCSTR name, LPDWORD reserved, LPDWORD type, LPBYTE data, LPDWORD size)
        {
            if (key != InstallKey())
                return ::RegQueryValueExA(key, name, reserved, type, data, size);
            if (reserved || !name || !size)
                return ERROR_INVALID_PARAMETER;

            const bool save = _stricmp(name, "save") == 0;
            if (!save && _stricmp(name, "movie") != 0 && _stricmp(name, "sound") != 0 &&
                _stricmp(name, "data") != 0 && _stricmp(name, "installdir") != 0)
                return ERROR_FILE_NOT_FOUND;
            auto path = GetExeModulePath<std::string>();
            if (save)
                path += "savedata\\";
            const auto bytes = static_cast<DWORD>(path.size() + 1);
            const auto capacity = data ? *size : 0;
            *size = bytes;
            if (type)
                *type = REG_SZ;
            if (data && capacity < bytes)
                return ERROR_MORE_DATA;
            if (data)
                std::memcpy(data, path.c_str(), bytes);
            return ERROR_SUCCESS;
        }

        LSTATUS WINAPI Close(HKEY key)
        {
            return key == InstallKey() ? ERROR_SUCCESS : ::RegCloseKey(key);
        }
    }

    // Answer the game's installation path reads. The configuration utility still owns its native settings file.
    export bool Patch()
    {
        auto config = hook::pattern("50 53 53 FF 75 F4 C7 45 F0 10 00 00 00 FF 15");
        if (config.size() > 0)
        {
            auto iat = *config.get_first<uintptr_t*>(15);
            injector::WriteMemory(&iat[0], &Query, true);
            injector::WriteMemory(&iat[1], &OpenEx, true);
            injector::WriteMemory(&iat[4], &Open, true);
            injector::WriteMemory(&iat[8], &Close, true);
            return true;
        }

        auto game = hook::pattern("83 EC 14 53 57 8B D8 8D 44 24 0C 50 68 19 00 02 00 6A 00 68 ? ? ? ? 68 02 00 00 80 FF 15");
        auto iat = *game.get_first<uintptr_t*>(31);
        injector::WriteMemory(&iat[0], &OpenEx, true);
        injector::WriteMemory(&iat[1], &Query, true);
        injector::WriteMemory(&iat[2], &Open, true);
        injector::WriteMemory(&iat[3], &Close, true);
        return false;
    }
}

class System
{
public:
    System()
    {
        WFP::onInitEvent() += []()
        {
            // Safe mode: the game empties savedata\disp.ini at startup and only writes it again on a clean exit, so a
            // crash resets the display settings and shows a warning. The file is no longer emptied, and no warning.
            auto safeMode = hook::pattern("BF 01 00 00 00 68 ? ? ? ? 68 ? ? ? ? 6A 01 E8 ? ? ? ? 83 C4 08 50 E8 ? ? ? ? 83 C4 08 85 C0 74 09 50 E8 ? ? ? ? 83 C4 04 8B C7 5F");
            injector::WriteMemory(safeMode.get_first(1), 0, true);
            injector::MakeJMP(safeMode.get_first(5), safeMode.get_first(0x2F), true);

            // The window gets no white edge on Windows 10 and later
            auto windowStyle = hook::pattern("68 00 03 00 00 FF 15 ? ? ? ? 5E");
            injector::WriteMemory(windowStyle.get_first(1), 0, true);

            // Steady 60 FPS: the frame pacing counted a frame late and kept falling back to 30 FPS
            auto pacing = hook::pattern("4A 03 C8 A3 ? ? ? ? 89 15");
            injector::WriteMemory<uint8_t>(pacing.get_first(), 0x42, true);

            // Frame rate: the game advances in 1/60 s ticks and waits for its frame interval in ticks. 60 and 30 FPS
            // keep that native pacing (30 keeps the interval at two ticks at least, the rate the game falls back to by
            // itself). Any other limit skips the native wait and paces at Present; each frame then advances the game
            // by the time that really passed, in fractions of a tick, which nearly all of the game reads as a float.
            static std::atomic<int> limit{ 60 };
            const auto readLimit = []()
            {
                const int value = FixSettingNumber("MAIN.FPSLimit", 60);
                limit = value < 0 ? -1 : value == 0 ? 0 : std::clamp(value, 30, 1000);
            };
            readLimit();
            FixSettings().OnChange("MAIN.FPSLimit", readLimit);
            // In-engine cutscenes keep the native pacing: their animation, camera and voice timing were made for it
            static const auto nativePacing = []() { return limit == 60 || limit == 30 || nRenderMode == 2; };

            auto wait = hook::pattern("55 8B EC 83 E4 F8 83 EC 38 A1 ? ? ? ? 53 55 33 ED 3B C5 56 57 0F 84");
            nFrameTicks.SetAddress(*wait.get_first<int32_t*>(10));
            static SafetyHookMid WaitHook = safetyhook::create_mid(wait.get_first(9), [](SafetyHookContext&)
            {
                if (!nativePacing())
                    nFrameTicks = 0;    // no native wait; the game sets the interval again every frame
                else if (limit == 30 && nFrameTicks > 0 && nFrameTicks < 2)
                    nFrameTicks = 2;
            });

            // The tick setter: integer ticks of the frame in, the frame's tick count, tick float, seconds and rate out
            static GameRef<int32_t> nTicks, nTicksCopy;
            static GameRef<float> fTicks, fTicksCopy, fSeconds, fRate;
            auto ticks = hook::pattern("56 8B 74 24 08 83 FE 01 7F 09 BE 01 00 00 00 89 74 24 08 A1");
            nTicks.SetAddress(*ticks.get_first<int32_t*>(0x7C));
            nTicksCopy.SetAddress(*ticks.get_first<int32_t*>(0x82));
            fTicks.SetAddress(*ticks.get_first<float*>(0x88));
            fTicksCopy.SetAddress(*ticks.get_first<float*>(0x92));
            fSeconds.SetAddress(*ticks.get_first<float*>(0xAF));
            fRate.SetAddress(*ticks.get_first<float*>(0xBF));
            static SafetyHookInline TicksHook = safetyhook::create_inline(ticks.get_first(), static_cast<void(__cdecl*)(int)>([](int count)
            {
                static LARGE_INTEGER frequency{}, previous{};
                static double remainder = 0.0;
                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                if (!frequency.QuadPart)
                    QueryPerformanceFrequency(&frequency);
                const double elapsed = previous.QuadPart ? double(now.QuadPart - previous.QuadPart) / double(frequency.QuadPart) : 0.0;
                previous = now;

                if (nativePacing())
                {
                    remainder = 0.0;
                    TicksHook.unsafe_ccall(count);
                    return;
                }
                TicksHook.unsafe_ccall(1);  // the debug pause and the 50 Hz mode decide as before
                if (fTicks <= 0.0f)
                    return;                 // paused
                const double tick = fSeconds;   // one tick in seconds: 1/60, or 1/50 in the 50 Hz mode
                // A frame advances at most three ticks, as the game allows; a long stall doesn't make it jump ahead
                const double frameTicks = std::clamp(elapsed / tick, 0.01, 3.0);
                remainder += frameTicks;
                const int whole = int(remainder);
                remainder -= whole;
                nTicks = whole;             // whole-tick counters still add up to the real time
                nTicksCopy = whole;
                fTicks = float(frameTicks);
                fTicksCopy = float(frameTicks);
                fSeconds = float(frameTicks * tick);
                fRate = float(1.0 / (frameTicks * tick));
            }));

            SHDisplay::OnPresent().push_back([]()
            {
                // 30 also keeps the presentation rate capped if the native timer has a late frame to catch up
                SHDisplay::SetFrameLimit(limit == 60 || (limit != 30 && nativePacing()) ? 0.0 : limit < 0 ? -1.0 : double(limit));
            });

            // Start at the title menu: the first-boot flag that plays the logos, the content warning and the opening
            // movie stays off, so the game takes the path it takes when returning to the title
            auto firstBoot = hook::pattern("33 C0 A3 ? ? ? ? A3 ? ? ? ? C6 05 ? ? ? ? 01 66 A3");
            if (FixSettings().Int("MAIN.SkipIntro"))
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
            injector::WriteMemory<uint8_t>(reverb.get_first(10), 0xEB, true);

            // Give the async sound worker time to run during both synchronous startup waits.
            auto partition = hook::pattern("83 C4 10 83 F8 03 74 ? E8 ? ? ? ? 56 E8 ? ? ? ? 83 C4 04 83 F8 03 75");
            auto waveCache = hook::pattern("83 C4 14 83 F8 03 74 ? 8B FF E8 ? ? ? ? 56 E8 ? ? ? ? 83 C4 04 83 F8 03 75");
            static const auto ServiceSound = reinterpret_cast<unsigned(__cdecl*)()>(injector::GetBranchDestination(partition.get_first(8)).as_int());
            const auto serviceWhileWaiting = +[]() -> unsigned
            {
                Sleep(1);
                return ServiceSound();
            };
            injector::MakeCALL(partition.get_first(8), serviceWhileWaiting, true);
            injector::MakeCALL(waveCache.get_first(10), serviceWhileWaiting, true);

            // Keep the audio workers' affinity and let Windows schedule the game thread independently.
            auto audioAffinity = hook::pattern("8B 15 ? ? ? ? 6A 01 52 FF D6 85 C0 74 ? A1 ? ? ? ? 6A 01 50 FF D6 85 C0 75 ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 83 C8 FF 5E C3 33 C0 5E C3");
            injector::MakeJMP(audioAffinity.get_first(0x0F), audioAffinity.get_first(0x2F), true);

            // The new-game event gate returns the selected value regardless of installed games.
            auto reference = hook::pattern("A0 ? ? ? ? 84 C0 ? ? E8 ? ? ? ? A2 ? ? ? ? C6 05");
            static const int enableReference = FixSettings().Get("MAIN.SH2Reference") == "1";
            injector::MakeJMP(reference.get_first(), +[]() -> int { return enableReference; }, true);
        };
    }
} System;
