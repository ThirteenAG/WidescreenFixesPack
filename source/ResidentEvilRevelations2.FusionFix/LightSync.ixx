module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <LEDEffects.h>

export module LightSync;

// Logitech LIGHTSYNC: the keyboard lights show the health of both players (MAIN.LightSyncRGB)

class LightSync
{
public:
    LightSync()
    {
        WFP::onInitEvent() += []()
        {
            if (!FixSettings().Bool("MAIN.LightSyncRGB"))
                return;

            auto playerPattern = hook::pattern("8B 0D ? ? ? ? 56 E8 ? ? ? ? 85 C0 74 7B 8B C8");
            if (!PatternAvailable(playerPattern, "ResidentEvilRevelations2.FusionFix: LightSync"))
                return;
            static auto sPlayerPtr = *playerPattern.get_first<void*>(2);

            LEDEffects::Inject([]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
                if (sPlayerPtr)
                {
                    static auto sub_6E6A70 = [](int* _this) -> void*
                    {
                        if (!_this)
                            return nullptr;
            
                        auto j = 0;
                        for (auto i = _this + 8; !*i || *(DWORD*)(*i + 0x7920); ++i)
                        {
                            if (++j >= 8)
                                return nullptr;
                        }
                        return (void*)_this[j + 8];
                    };
                    auto pPlayerPtr = sub_6E6A70(*(int**)sPlayerPtr);
            
                    if (pPlayerPtr)
                    {
                        auto Player1Health = PtrWalkthrough<int32_t>(&pPlayerPtr, 0x1A08);
                        auto Player2Health = PtrWalkthrough<int32_t>(&pPlayerPtr, 0x1A0C);
            
                        if (Player1Health && Player2Health)
                        {
                            auto health1 = *Player1Health;
                            auto health2 = *Player2Health;
                            if (health1 > 1)
                            {
                                if (health1 <= 250) {
                                    LEDEffects::SetLightingLeftSide(26, 4, 4, true, false); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                }
                                else if (health1 <= 350) {
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
                                if (health2 <= 250) {
                                    LEDEffects::SetLightingRightSide(26, 4, 4, true, false); //red
                                    LEDEffects::DrawCardiogramNumpad(100, 0, 0, 0, 0, 0); //red
                                }
                                else if (health2 <= 350) {
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
                            LEDEffects::SetLighting(90, 36, 3);
                        }
                    }
                    else
                    {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(90, 36, 3);
                    }
                }
            });
        };
    }
} LightSync;
