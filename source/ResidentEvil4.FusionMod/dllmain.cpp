#include "stdafx.h"
#include "LogitechLEDLib.h"
#pragma comment(lib, "LogitechLEDLib_x64.lib")

static bool bLogiLedInitialized = false;

template <typename T, typename PtrSize = uint64_t>
std::optional<T> PtrWalkthrough(auto addr, std::convertible_to<ptrdiff_t> auto&& ...offsets)
{
    auto list = std::vector<ptrdiff_t>{ offsets... };
    auto last = list.back(); list.pop_back();
    auto a = injector::ReadMemory<PtrSize>(addr, true);
    if (a)
    {
        for (auto v : list)
        {
            auto ptr = injector::ReadMemory<PtrSize>(a + v, true);
            if (ptr)
                a = ptr;
            else
            {
                a = 0;
                break;
            }
        }

        if (a)
            return injector::ReadMemory<T>(a + last, true);
    }
    return std::nullopt;
};

void Init()
{
    //CIniReader iniReader("");

    bLogiLedInitialized = LogiLedInit();

    if (bLogiLedInitialized)
    {
        static std::vector<LogiLed::KeyName> keys = {
            LogiLed::KeyName::CAPS_LOCK,
            LogiLed::KeyName::A,
            LogiLed::KeyName::S,
            LogiLed::KeyName::D,
            LogiLed::KeyName::F,
            LogiLed::KeyName::T,
            LogiLed::KeyName::SIX,
            LogiLed::KeyName::Y,
            LogiLed::KeyName::H,
            LogiLed::KeyName::N,
            LogiLed::KeyName::J,
            LogiLed::KeyName::I,
            LogiLed::KeyName::NINE,
            LogiLed::KeyName::O,
            LogiLed::KeyName::L,
            LogiLed::KeyName::SEMICOLON,
            LogiLed::KeyName::APOSTROPHE,
            LogiLed::KeyName::ENTER,
        };

        static std::vector<LogiLed::KeyName> keys_dead = {
            LogiLed::KeyName::CAPS_LOCK,
            LogiLed::KeyName::A,
            LogiLed::KeyName::S,
            LogiLed::KeyName::D,
            LogiLed::KeyName::F,
            LogiLed::KeyName::G,
            LogiLed::KeyName::H,
            LogiLed::KeyName::J,
            LogiLed::KeyName::K,
            LogiLed::KeyName::L,
            LogiLed::KeyName::SEMICOLON,
            LogiLed::KeyName::APOSTROPHE,
            LogiLed::KeyName::ENTER,
        };

        auto pattern = hook::pattern("48 8B 05 ? ? ? ? 48 8B F2 48 8B 5A 10");
        static auto sPlayerPtr = injector::ReadRelativeOffset(pattern.get_first(3), 4, true).as_int();

        std::thread t([]() 
        {
            while (true)
            {
                if (bLogiLedInitialized)
                {
                    if (sPlayerPtr)
                    {
                        auto PlayerHealth = PtrWalkthrough<int32_t>(sPlayerPtr, 0x78, 0x18, 0x30, 0x148, 0x14);
                        //auto PlayerHealthMax = PtrWalkthrough<int32_t>(sPlayerPtr, 0x78, 0x18, 0x30, 0x148, 0x10);

                        if (PlayerHealth)
                        {
                            auto Health = *PlayerHealth;
                            if (Health > 0)
                            {
                                if (Health < 400)
                                    LogiLedSetLighting(26, 4, 4); //red
                                else
                                    LogiLedSetLighting(5, 15, 2);  //green

                                for (auto& key : keys)
                                {
                                    auto s = 5 * keys.size();
                                    std::this_thread::sleep_for(std::chrono::milliseconds(s));
                                    if (Health < 400)
                                        LogiLedPulseSingleKey(key, 255, 0, 0, 0, 0, 0, s * 50, false); //red
                                    else
                                        LogiLedPulseSingleKey(key, 0, 255, 0, 0, 0, 0, s * 50, false); //green
                                }
                            }
                            else
                            {
                                for (auto& key : keys_dead)
                                {
                                    auto s = 5 * keys_dead.size();
                                    std::this_thread::sleep_for(std::chrono::milliseconds(s));
                                    LogiLedPulseSingleKey(key, 255, 0, 0, 0, 0, 0, s * 50, false);
                                }
                            }
                        }
                        else
                        {
                            LogiLedSetLighting(76, 12, 18); //logo red
                        }
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                }
                else
                    break;
            }
        });

        t.detach();
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("48 8B 05 ? ? ? ? 48 8B F2 48 8B 5A 10"));
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
