#include "stdafx.h"
#include <thread>
#include <future>
#include <dolphin/dolphin.h>
#include <LEDEffects.h>

bool bEnableDoorSkip;
bool bUnthrottleEmuDuringDoorSkip;
bool bLightSyncRGB;

static bool bLogiLedInitialized = false;
std::promise<void> exitSignal;

void PluginThread(std::future<void> futureObj)
{
    [&]()
    {
        __try
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

            if (bLightSyncRGB && !bLogiLedInitialized)
            {
                int majorNum = 0;
                int minorNum = 0;
                int buildNum = 0;
                if (!LogiLedGetSdkVersion(&majorNum, &minorNum, &buildNum))
                {
                    bLogiLedInitialized = LogiLedInit();
                    if (bLogiLedInitialized) {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(52, 0, 0); //logo red
                    }
                }
            }
            
            static uint32_t* data = nullptr;
            static int16_t* sPlayerHealth = nullptr;
            while (futureObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (Dolphin::MemoryValid(futureObj))
                {
                    if (Dolphin::GameID() == "GHAE08" || Dolphin::GameID() == "GHAP08" || Dolphin::GameID() == "GLEE08"/* || Dolphin::GameID() == "GLEJ08"*/)
                    {
                        static uint32_t* memcheck = nullptr;
                        static uint32_t bytes = 0;
                        if (data == nullptr || (Dolphin::MemoryAddrValid(memcheck) && *memcheck != bytes))
                        {
                            [&]()
                            {
                                auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 a0 00 01 3c 60 ? ? 38 83 ? ? 90 a4 00 00 3c 60");
                                if (Dolphin::GameID() == "GLEE08" /* || Dolphin::GameID() == "GLEJ08"*/)
                                    pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "38 00 00 01 3c 60 ? ? 38 63 ? ? ? ? 00 00 3c 60");
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
                            }();
            
                            if (bLogiLedInitialized)
                            {
                                LogiLedStopEffects();
                                LEDEffects::SetLighting(52, 0, 0); //logo red
            
                                [&]()
                                {
                                    if (Dolphin::GameID() == "GLEE08" /* || Dolphin::GameID() == "GLEJ08"*/)
                                    {
                                        auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "7c 7e 1b 78 7c 9d 23 78 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 2c 03 00 00 ? ? ? ? 2c 1e 00 0a ? ? ? ? 2c 1e 00 64 ? ? ? ? 3b de 00 0a");
                                        if (pattern.size() >= 1)
                                        {
                                            uint16_t a = swap16(*pattern.get(0).get<uint16_t>(10));
                                            uint16_t b = swap16(*pattern.get(0).get<uint16_t>(14));
                                            uint16_t c = swap16(*pattern.get(0).get<uint16_t>(18));
                                            uint16_t d = swap16(*pattern.get(0).get<uint16_t>(54));
                                            if (a != 0 && b != 0)
                                                sPlayerHealth = (int16_t*)((convert(a) + b + c + d) - Dolphin::ImageBase + Dolphin::GameMemoryStart);
                                        }
                                    }
                                    else
                                    {
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
                                }();
                            }
            
                            if (bEnableDoorSkip)
                            {
                                [&]() {
                                    if (Dolphin::GameID() == "GLEE08" /* || Dolphin::GameID() == "GLEJ08"*/)
                                    {
                                        auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "94 21 ff f0 7c 08 02 a6 ? ? ? ? ? ? ? ? 38 60 00 01");
                                        if (pattern.size() > 1)
                                        {
                                            injector::WriteMemory(pattern.get(0).get<void>(0), blr(), true); //return
                                            Dolphin::MenuBarClearCache();
                                        }
                                    }
                                    else
                                    {
                                        auto pattern = hook::pattern(Dolphin::GameMemoryStart, Dolphin::GameMemoryEnd, "28 00 00 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 54 00 03 9c 28 00 00 00 ? ? ? ? 38 00 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? 48 00 ? ? 48 00");
                                        if (pattern.size() >= 1)
                                        {
                                            injector::WriteMemory<uint16_t>(pattern.get(0).get<void>(4), 0x0048, true); // bne -> b
                                            Dolphin::MenuBarClearCache();
                                        }
                                    }
                                }();
                            }
                        }
                        else
                        {
                            if (Dolphin::GameID() == "GLEE08" /* || Dolphin::GameID() == "GLEJ08"*/)
                            {
                                while (bUnthrottleEmuDuringDoorSkip && Dolphin::MemoryAddrValid(data) && data[0] == 1)
                                {
                                    Dolphin::SetIsThrottlerTempDisabled(true);
                                }
                            }
                            else
                            {
                                while (bUnthrottleEmuDuringDoorSkip && Dolphin::MemoryAddrValid(data) && data[0] == 0x01000000)
                                {
                                    Dolphin::SetIsThrottlerTempDisabled(true);
                                }
                            }
            
                            if (bLogiLedInitialized)
                            {
                                if (Dolphin::MemoryAddrValid(sPlayerHealth))
                                {
                                    int16_t health1 = (int16_t)swap16(*sPlayerHealth);
                                    auto poisoned = *(int8_t*)((uintptr_t)sPlayerHealth + 0xC2);

                                    if (Dolphin::GameID() == "GLEE08" /* || Dolphin::GameID() == "GLEJ08"*/)
                                        poisoned = *(int8_t*)((uintptr_t)sPlayerHealth + 0x6) == 1;

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
                                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
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
                    if (bLogiLedInitialized) {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(52, 0, 0); //logo red
                    }
                }
            }

            if (bLogiLedInitialized) {
                bLogiLedInitialized = false;
                []()
                {
                    auto future = std::async(std::launch::async, []() { LogiLedShutdown(); });
                    std::future_status status;
                    do {
                        switch (status = future.wait_for(0ms); status) {
                        case std::future_status::deferred: break;
                        case std::future_status::ready: break;
                        case std::future_status::timeout: ExitProcess(0);
                       }
                    } while (status != std::future_status::ready);
                }();
            }
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        {
        }
    }();
}

void Init()
{
    CIniReader iniReader("");
    bEnableDoorSkip = iniReader.ReadInteger("MAIN", "EnableDoorSkip", 1) != 0;
    bUnthrottleEmuDuringDoorSkip = iniReader.ReadInteger("MAIN", "UnthrottleEmuDuringDoorSkip", 1) != 0;
    bLightSyncRGB = iniReader.ReadInteger("MAIN", "LightSyncRGB", 1) != 0;

    if (!bEnableDoorSkip && !bUnthrottleEmuDuringDoorSkip && !bLightSyncRGB)
        return;

    std::thread(PluginThread, exitSignal.get_future()).detach();
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, Dolphin::Pattern());
        CallbackHandler::RegisterCallback(L"explorerframe.dll", []() { exitSignal.set_value(); }, true);
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

    }
    return TRUE;
}
