module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include "LEDEffects.h"

export module LightSync;

// Logitech LIGHTSYNC (MAIN.LightSyncRGB): Rebecca's health on the left side of the keyboard, Billy's on the right, as
// colours and cardiograms, purple when poisoned.

namespace
{
    // The characters, taken from the game's health setter (health at +0x1030, character id at +0x100: 3 Rebecca,
    // 5 Billy)
    uint32_t* sPlayer1Ptr = nullptr;
    uint32_t* sPlayer2Ptr = nullptr;
}

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        auto pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 85 C0 75 09");
        if (!PatternAvailable(pattern, "ResidentEvil0.FusionFix: LightSync"))
            return;
        static SafetyHookMid HealthHook = safetyhook::create_mid(injector::GetBranchDestination(pattern.get_first()).get_raw<void>(), [](SafetyHookContext& regs)
        {
            auto character = reinterpret_cast<uint32_t*>(regs.ecx);
            if (character[0x40] == 3)
                sPlayer1Ptr = character;
            else if (character[0x40] == 5)
                sPlayer2Ptr = character;
        });

        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            auto Player1ID = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x100);
            auto Player2ID = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x100);
            auto Player1Health = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x1030);
            auto Player2Health = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x1030);
            auto Player1Poisoned = PtrWalkthrough<int32_t>(&sPlayer1Ptr, 0x68C4);
            auto Player2Poisoned = PtrWalkthrough<int32_t>(&sPlayer2Ptr, 0x68C4);

            if (Player1ID && *Player1ID == 3 && Player1Health)
            {
                auto health1 = *Player1Health;
                auto poisoned1 = *Player1Poisoned == 1;
                if (health1 > 1)
                {
                    if (health1 < 40)
                    {
                        if (!poisoned1)
                            LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                        else
                            LEDEffects::SetLightingLeftSide(51, 4, 53, true, false); //purple
                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                    }
                    else if (health1 < 130)
                    {
                        if (!poisoned1)
                            LEDEffects::SetLightingLeftSide(50, 30, 4, true, false); //orange
                        else
                            LEDEffects::SetLightingLeftSide(51, 4, 53, true, false); //purple
                        LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                    }
                    else
                    {
                        if (!poisoned1)
                            LEDEffects::SetLightingLeftSide(10, 30, 4, true, false);  //green
                        else
                            LEDEffects::SetLightingLeftSide(51, 4, 53, true, false);  //purple
                        LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                    }
                }
                else
                {
                    LEDEffects::SetLightingLeftSide(26, 4, 4, false, true); //red
                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                }

                if (Player2ID && *Player2ID == 5 && Player2Health)
                {
                    auto health2 = *Player2Health;
                    auto poisoned2 = *Player2Poisoned == 1;
                    if (health2 > 1)
                    {
                        if (health2 < 40)
                        {
                            if (!poisoned2)
                                LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                            else
                                LEDEffects::SetLightingRightSide(51, 4, 53, true, false); //purple
                            LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                        }
                        else if (health2 < 130)
                        {
                            if (!poisoned2)
                                LEDEffects::SetLightingRightSide(50, 30, 4, true, false); //orange
                            else
                                LEDEffects::SetLightingRightSide(51, 4, 53, true, false); //purple
                            LEDEffects::DrawCardiogramNumpad(67, 0, 0, 0, 0, 0); //orange
                        }
                        else
                        {
                            if (!poisoned2)
                                LEDEffects::SetLightingRightSide(10, 30, 4, true, false);  //green
                            else
                                LEDEffects::SetLightingRightSide(51, 4, 53, true, false);  //purple
                            LEDEffects::DrawCardiogramNumpad(0, 100, 0, 0, 0, 0); //green
                        }
                    }
                    else
                    {
                        LEDEffects::SetLightingRightSide(26, 4, 4, false, true); //red
                        LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0, true);
                    }
                }
                else
                {
                    LEDEffects::SetLightingRightSide(64, 20, 8); // rebecca's red cross
                }
            }
            else
            {
                LogiLedStopEffects();
                LEDEffects::SetLighting(64, 20, 8); // rebecca's red cross
            }
        });
    }
}
