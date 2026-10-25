module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <dolphin/dolphin.h>
#include <LEDEffects.h>

export module LightSync;

import Game;

// Logitech LIGHTSYNC (MAIN.LightSyncRGB): the player's health as the keyboard colour and a cardiogram, purple when
// poisoned.

export namespace LightSync
{
    void Init()
    {
        if (!FixSettings().Bool("MAIN.LightSyncRGB"))
            return;

        LEDEffects::Inject([]()
        {
             [&]()
             {
                 __try
                 {
                     using namespace powerpc;

                     std::this_thread::sleep_for(std::chrono::milliseconds(100));
                     
                     auto gameVersion = Game::Detect(true);
                     
                     if (gameVersion)
                     {
                         if (Dolphin::MemoryAddrValid(Game::sPlayerHealth))
                         {
                             int16_t health1 = (int16_t)swap16(*Game::sPlayerHealth);
                             auto poisoned = *(int8_t*)((uintptr_t)Game::sPlayerHealth + 0xC2);
                     
                             if (gameVersion == RE3)
                                 poisoned = *(int8_t*)((uintptr_t)Game::sPlayerHealth + 0x6) % 4 != 0;
                     
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
                     else
                     {
                         LogiLedStopEffects();
                         LEDEffects::SetLighting(52, 0, 0); //logo red
                     }
                 }
                 __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
                 {
                 }
             }();
        });
    }
}
