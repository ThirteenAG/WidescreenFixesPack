module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "LEDEffects.h"

export module LightSync;

// Logitech LIGHTSYNC (MAIN.LightSyncRGB): the two characters' health on the left and right side of the keyboard, as
// colours and cardiograms.

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        auto player = hook::pattern("8B 0D ? ? ? ? 56 E8 ? ? ? ? A1 ? ? ? ? 8B 48 20 69 C9 ? ? ? ? 8B 94 01");
        if (player.size() != 1)
        {
            OutputDebugStringA("Resident Evil 5: health lighting pattern unavailable.\n");
            return;
        }
        static auto sPlayerPtr = *player.get_first<void*>(2);

        LEDEffects::Inject([]()
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));

            if (sPlayerPtr)
            {
                auto Player1Health = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0x2A74, 0x1364);
                auto Player2Health = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0x1364);

                if (Player1Health && Player2Health)
                {
                    auto health1 = *Player1Health;
                    auto health2 = *Player2Health;
                    if (health1 > 1)
                    {
                        if (health1 == 2) {
                            LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                            LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                        }
                        else if (health1 <= 250) {
                            LEDEffects::SetLightingLeftSide(50, 30, 4, true, false); //orange
                            LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
                        }
                        else {
                            LEDEffects::SetLightingLeftSide(10, 30, 4, true, false);  //green
                            LEDEffects::DrawCardiogram(0, 100, 0, 0, 0, 0); //green
                        }                                   
                    }
                    else
                    {
                        LEDEffects::SetLightingLeftSide(26, 4, 4, false, true); //red
                        LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0, true);
                    }

                    if (health2 > 1)
                    {
                        if (health2 == 2) {
                            LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                            LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                        }
                        else if (health2 <= 250) {
                            LEDEffects::SetLightingRightSide(50, 30, 4, true, false); //orange
                            LEDEffects::DrawCardiogramNumpad(67, 0, 0, 0, 0, 0); //orange
                        }
                        else {
                            LEDEffects::SetLightingRightSide(10, 30, 4, true, false);  //green
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
                    LogiLedStopEffects();
                    LEDEffects::SetLighting(98, 87, 24); //logo gold
                }
            }
        });
    }
}
