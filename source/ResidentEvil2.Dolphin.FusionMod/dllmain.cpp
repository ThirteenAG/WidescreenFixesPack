#include "stdafx.h"
#include <assembly64.hpp>
#include <dolphin/dolphin.h>

void Init()
{
    CIniReader iniReader("");
    static bool bEnableDoorSkip = iniReader.ReadInteger("MAIN", "EnableDoorSkip", 1) != 0;
    static bool bUnthrottleEmuDuringDoorSkip = iniReader.ReadInteger("MAIN", "UnthrottleEmuDuringDoorSkip", 1) != 0;

    if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip)
        return;

    std::thread([]()
    {
        using namespace powerpc;
        using namespace std::chrono_literals;

        static uint32_t* data = nullptr;
        while (true)
        {
            std::this_thread::yield();
            if (Dolphin::MemoryValid())
            {
                if (Dolphin::GameID() == "GHAE08" || Dolphin::GameID() == "GHAP08")
                {
                    static uint32_t* memcheck = nullptr;
                    static uint32_t bytes = 0;
                    if (data == nullptr || (memcheck && *memcheck != bytes))
                    {
                        auto swap16 = [](uint16_t n) -> uint16_t
                        {
                            return (n >> 8) | (n << 8);
                        };
                        auto convert = [](uint32_t n) -> uint32_t
                        {
                            auto ptr = (unsigned char*)&n;
                            return (ptr[1] << 24) | (ptr[0] << 16) | 0 | 0;
                        };
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
                        if (bEnableDoorSkip)
                        {
                            pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "7f a0 22 14 ? ? ? ? ? ? ? ? 38 00 00 00 ? ? ? ? 38 00 00 00");
                            if (pattern.size() >= 1)
                            {
                                injector::WriteMemory(pattern.get(0).get<void>(4), li(r0, 10), true);
                                Dolphin::MenuBarClearCache();
                            }

                            pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "54 7d 04 3e 38 00 00 01 98 1f 00 01 38 00 00 00");
                            if (pattern.size() >= 1)
                            {
                                injector::WriteMemory(pattern.get(0).get<void>(4), li(r0, 0), true);
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
                    }
                }
            }
            else
            {
                data = nullptr;
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
    return TRUE;
}
