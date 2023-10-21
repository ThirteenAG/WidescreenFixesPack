#include "stdafx.h"
#include <assembly64.hpp>
#include <dolphin/dolphin.h>
#include <LEDEffects.h>

static bool bLogiLedInitialized = false;

void Init()
{
    CIniReader iniReader("");
    static bool bEnableDoorSkip = iniReader.ReadInteger("MAIN", "EnableDoorSkip", 1) != 0;
    static bool bUnthrottleEmuDuringDoorSkip = iniReader.ReadInteger("MAIN", "UnthrottleEmuDuringDoorSkip", 1) != 0;
    static bool bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;

    if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip && !bLightSyncRGB)
        return;

    if (bLightSyncRGB && !bLogiLedInitialized && false)
    {
        bLogiLedInitialized = LogiLedInit();
        if (bLogiLedInitialized) {
            LogiLedStopEffects();
            LEDEffects::SetLighting(52, 0, 0); //logo red

            IATHook::Replace(GetModuleHandleA(NULL), "KERNEL32.DLL",
                std::forward_as_tuple("ExitProcessImplementation", static_cast<void(__stdcall*)(UINT)>([](UINT uExitCode) {
                    if (bLogiLedInitialized) {
                        LogiLedShutdown();
                        bLogiLedInitialized = false;
                    }
                    ExitProcess(uExitCode); 
                }))
            );
        }
    }

    std::thread([]()
    {
        using namespace powerpc;
        using namespace std::chrono_literals;

        static auto swap16 = [](uint16_t n) -> uint16_t
        {
            return (n >> 8) | (n << 8);
        };

        static auto convert = [](uint32_t n) -> uint32_t
        {
            auto ptr = (unsigned char*)&n;
            return (ptr[1] << 24) | (ptr[0] << 16) | 0 | 0;
        };

        static uint32_t* data = nullptr;
        static int16_t* sPlayerHealth = nullptr;
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (Dolphin::MemoryValid())
            {
                if (Dolphin::GameID() == "GHAE08" || Dolphin::GameID() == "GHAP08")
                {
                    static uint32_t* memcheck = nullptr;
                    static uint32_t bytes = 0;
                    if (data == nullptr || (Dolphin::MemoryValid(memcheck) && *memcheck != bytes))
                    {
                        auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 a0 00 01 3c 60 ? ? 38 83 ? ? 90 a4 00 00 3c 60");
                        if (pattern.size() >= 1)
                        {
                            memcheck = pattern.get(0).get<uint32_t>(4);
                            bytes = injector::ReadMemory<uint32_t>(memcheck, true);
                            uint16_t a = swap16(*pattern.get(0).get<uint16_t>(6));
                            uint16_t b = swap16(*pattern.get(0).get<uint16_t>(10));
                            uint16_t c = swap16(*pattern.get(0).get<uint16_t>(14));
                            if (a != 0 && b != 0)
                                data = (uint32_t*)((convert(a) - int16_t(0 - b) + c) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                        }

                        if (bLogiLedInitialized)
                        {
                            LogiLedStopEffects();
                            LEDEffects::SetLighting(52, 0, 0); //logo red

                            auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "3c 60 ? ? 38 63 ? ? ? ? ? ? 2c 00 00 00 ? ? ? ? 38 00 ? ? 3c 60 ? ? 38 63 ? ? ? ? ? ? 3c 60 ? ? 38 63 ? ? ? ? ? ? 54 00 02 94");
                            if (pattern.size() >= 1)
                            {
                                uint16_t a = swap16(*pattern.get(0).get<uint16_t>(2));
                                uint16_t b = swap16(*pattern.get(0).get<uint16_t>(6));
                                uint16_t c = swap16(*pattern.get(0).get<uint16_t>(10));
                                if (a != 0 && b != 0)
                                    sPlayerHealth = (int16_t*)((convert(a) + b + c) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                            }
                        }

                        if (bEnableDoorSkip)
                        {
                            auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "28 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 54 00 03 9c 28 00 00 00 ? ? ? ? 38 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 48 00 ? ? 48 00");
                            if (pattern.size() >= 1)
                            {
                                injector::WriteMemory<uint16_t>(pattern.get(0).get<void>(4), 0x0048, true); // bne -> b
                                Dolphin::MenuBarClearCache();
                            }
                        }
                    }
                    else
                    {
                        while (bUnthrottleEmuDuringDoorSkip && Dolphin::MemoryValid(data) && data[0] == 0x01000000)
                        {
                            Dolphin::SetIsThrottlerTempDisabled(true);
                        }

                        if (bLogiLedInitialized)
                        {
                            if (Dolphin::MemoryValid(sPlayerHealth))
                            {
                                int16_t health1 = (int16_t)swap16(*sPlayerHealth);
                                auto poisoned = *(int8_t*)((uintptr_t)sPlayerHealth + 0xC2);
                                if (health1 > 1)
                                {
                                    if (health1 <= 20) {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(26, 4, 4, true, false); //red
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health1 <= 40) {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(100, 39, 13, true, false); //orange
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                    }
                                    else if (health1 <= 100) {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(52, 48, 0, true, false); //orange
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                                    }
                                    else {
                                        if (!poisoned)
                                            LEDEffects::SetLighting(0, 48, 0, true, false);  //green
                                        else
                                            LEDEffects::SetLighting(51, 4, 53, true, false); //purple
                                        LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                    }
                                }
                                else
                                {
                                    LEDEffects::SetLighting(26, 4, 4, false); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0);
                                }
                            }
                            else
                            {
                                LogiLedStopEffects();
                                LEDEffects::SetLighting(52, 0, 0); //logo red
                            }
                        }
                    }
                }
            }
            else
            {
                data = nullptr;
                sPlayerHealth = nullptr;
            }
        }
    }
    ).detach();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, Dolphin::Pattern());
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (bLogiLedInitialized) {
            LogiLedShutdown();
            bLogiLedInitialized = false;
        }
    }
    return TRUE;
}
