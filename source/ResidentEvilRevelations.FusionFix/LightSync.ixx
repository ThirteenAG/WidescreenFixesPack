module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "../Shared/PatternCheck.hpp"
#include <LEDEffects.h>

export module LightSync;

// Logitech LIGHTSYNC: the keyboard lights show the player's health (MAIN.LightSyncRGB)

class LightSync
{
public:
    LightSync()
    {
        WFP::onInitEvent() += []()
        {
            if (!FixSettings().Bool("MAIN.LightSyncRGB"))
                return;

            auto playerPattern = hook::pattern("8B 0D ? ? ? ? 85 C9 75 14 68 ? ? ? ? 6A 3E 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 8B 0D ? ? ? ? 8D 54 24 0A");
            auto valuePattern = hook::pattern("A1 ? ? ? ? 81 C6");
            if (!PatternAvailable(playerPattern, "ResidentEvilRevelations.FusionFix: LightSync") || !PatternAvailable(valuePattern, "ResidentEvilRevelations.FusionFix: LightSync"))
                return;
            static auto sPlayerPtr = *playerPattern.get_first<void*>(2);
            static auto dword_116F398 = *valuePattern.get_first<void*>(1);

            LEDEffects::Inject([]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
                if (sPlayerPtr)
                {
                    static auto sub_40EEE0 = [](int* _this) -> void*
                    {
                        auto sub_87F4E0 = [](uint32_t* _this) -> int
                        {
                            if ((_this[61] - 1) > 1)
                                return 0;
                            else
                                return _this[62];
                        };
            
                        if (!_this)
                            return nullptr;
            
                        uint32_t* v3; // esi
                        auto v2 = _this[12];
                        if (v2 == _this[13])
                            return 0;
                        while (true)
                        {
                            v3 = *(uint32_t**)(v2 + 16);
                            if (sub_87F4E0((uint32_t*)dword_116F398) == v3[907])
                            {
                                auto v5 = v3[3] & 7;
                                if (v5 == 2 || v5 == 1)
                                    break;
                            }
                            v2 = *(uint32_t*)(v2 + 8);
                            if (v2 == _this[13])
                                return 0;
                        }
                        return v3;
            
                    };
                    auto pPlayerPtr = sub_40EEE0(*(int**)sPlayerPtr);
            
                    if (pPlayerPtr)
                    {
                        auto PlayerHealth = PtrWalkthrough<float>(&pPlayerPtr, 0xE38);
            
                        if (PlayerHealth)
                        {
                            auto Health = *PlayerHealth;
                            if (Health > 0)
                            {
                                if (Health <= 400) {
                                    LEDEffects::SetLighting(26, 4, 4, true); //red
                                    LEDEffects::DrawCardiogram(100, 0, 0, 0, 0, 0); //red
                                }
                                else if (Health <= 600) {
                                    LEDEffects::SetLighting(50, 30, 4, true); //orange
                                    LEDEffects::DrawCardiogram(67, 0, 0, 0, 0, 0); //orange
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
                            LogiLedStopEffects();
                            LEDEffects::SetLighting(76, 12, 18); //logo red
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
