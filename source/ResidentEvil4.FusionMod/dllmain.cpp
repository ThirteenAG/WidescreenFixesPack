#include "stdafx.h"
#include "LEDEffects.h"

static bool bLogiLedInitialized = false;

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
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

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
                                if (Health < 400) {
                                    LEDEffects::SetLighting(26, 4, 4, true); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                }
                                else {
                                    LEDEffects::SetLighting(10, 30, 4, true);  //green
                                    LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                                }
                            }
                            else
                            {
                                LEDEffects::SetLighting(26, 4, 4, false, true); //red
                                LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                            }
                        }
                        else
                        {
                            LogiLedSetLighting(76, 12, 18); //logo red
                        }
                    }
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
