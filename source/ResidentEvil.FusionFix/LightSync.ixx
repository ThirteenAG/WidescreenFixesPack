module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "LEDEffects.h"

export module LightSync;

// Logitech LIGHTSYNC (MAIN.LightSyncRGB): health as the keyboard colour and a cardiogram, purple when poisoned, the
// ammo left in the clip on the F-key row.

namespace
{
    // The equipped weapon: ammo in the clip at +0xFFC, clip size at +0x1000. Taken from the game's weapon functions
    // that set and check the clip.
    uint32_t* ammoInClip = nullptr;
    uint32_t* maxAmmoInClip = nullptr;

    void AmmoInClip()
    {
        if (!ammoInClip || !maxAmmoInClip)
            return;

        static std::vector<LogiLed::KeyName> keys = {
            LogiLed::KeyName::F1, LogiLed::KeyName::F2, LogiLed::KeyName::F3,
            LogiLed::KeyName::F4, LogiLed::KeyName::F5, LogiLed::KeyName::F6,
            LogiLed::KeyName::F7, LogiLed::KeyName::F8, LogiLed::KeyName::F9,
            LogiLed::KeyName::F10, LogiLed::KeyName::F11, LogiLed::KeyName::F12,
            LogiLed::KeyName::PRINT_SCREEN, LogiLed::KeyName::SCROLL_LOCK,
            LogiLed::KeyName::PAUSE_BREAK,
        };

        static auto oldAmmoInClip = -1;
        if (*ammoInClip != oldAmmoInClip)
        {
            auto maxAmmo = *maxAmmoInClip;
            if (maxAmmo == 255) // grenade launcher
                maxAmmo = *ammoInClip;

            float ammoInClipPercent = maxAmmo ? ((float)*ammoInClip / (float)maxAmmo) * 100.0f : 0.0f;

            for (size_t i = 0; i < keys.size(); i++)
            {
                auto indexInPercent = ((float)i / (float)keys.size()) * 100.0f;
                if (ammoInClipPercent > indexInPercent)
                    LogiLedSetLightingForKeyWithKeyName(keys[i], 100, 100, 100);
                else
                    LogiLedSetLightingForKeyWithKeyName(keys[i], 10, 10, 10);
            }
        }
        oldAmmoInClip = *ammoInClip;
    }

    void CaptureWeapon(SafetyHookContext& regs)
    {
        ammoInClip = reinterpret_cast<uint32_t*>(regs.ecx + 0xFFC);
        maxAmmoInClip = reinterpret_cast<uint32_t*>(regs.ecx + 0x1000);
    }
}

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        auto setClip = hook::pattern("8B 44 24 04 8B 91 ? ? ? ? 89 81 ? ? ? ? 3B C2");
        auto checkClip = find_pattern("56 8B F1 8B 46 04 83 E8 00 74 25", "56 8B F1 8B 46 04 83 E8 00 74 29");
        if (PatternAvailable(setClip, "ResidentEvil.FusionFix: LightSync"))
        {
            static SafetyHookMid SetClipHook = safetyhook::create_mid(setClip.get_first(), CaptureWeapon);
        }
        if (PatternAvailable(checkClip, "ResidentEvil.FusionFix: LightSync"))
        {
            static SafetyHookMid CheckClipHook = safetyhook::create_mid(checkClip.get_first(), CaptureWeapon);
        }

        auto player = find_pattern("8B D1 56 8B 42 24", "FF 71 24 FF 71 20 E8 ? ? ? ? C3");
        auto manager = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B D0 89 54 24 18");
        if (player.size() != 1 || manager.size() != 1)
        {
            OutputDebugStringA("Resident Evil: health lighting patterns unavailable.\n");
            return;
        }
        static auto sub_4898C0 = (int(__fastcall*)(void*, int))player.get_first();
        static auto dword_D7C938 = *manager.get_first<void**>(2);
        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (*dword_D7C938)
            {
                auto sPlayerPtr = sub_4898C0(*dword_D7C938, 0);
                if (sPlayerPtr)
                {
                    auto Player1Health = PtrWalkthrough<int32_t>(&sPlayerPtr, 0x13BC);
                    auto Player1Poisoned = PtrWalkthrough<int32_t>(&sPlayerPtr, 0x1584);

                    if (Player1Health)
                    {
                        auto health1 = *Player1Health;
                        auto poisoned = *Player1Poisoned;
                        if (health1 > 1)
                        {
                            if (health1 < 240) {
                                if (!poisoned)
                                    LEDEffects::SetLighting(26, 4, 4, true, false, true); //red
                                else
                                    LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                            }
                            else if (health1 < 720) {
                                if (!poisoned)
                                    LEDEffects::SetLighting(50, 30, 4, true, false, true); //orange
                                else
                                    LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                            }
                            else {
                                if (!poisoned)
                                    LEDEffects::SetLighting(10, 30, 4, true, false, true);  //green
                                else
                                    LEDEffects::SetLighting(51, 4, 53, true, false, true); //purple
                                LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                            }

                            AmmoInClip();
                        }
                        else
                        {
                            LEDEffects::SetLighting(26, 4, 4, false, true); //red
                            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                        }
                    }
                    else
                    {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(93, 12, 14); //umbrella red
                    }
                }
                else
                {
                    LogiLedStopEffects();
                    LEDEffects::SetLighting(93, 12, 14); //umbrella red
                }
            }
        });
    }
}
