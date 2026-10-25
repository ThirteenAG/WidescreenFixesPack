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

            auto playerPattern = hook::pattern("8B 0D ? ? ? ? 85 C9 74 07 6A 01 E8 ? ? ? ? FE 86");
            if (!PatternAvailable(playerPattern, "ResidentEvil6.FusionFix: LightSync"))
                return;
            static auto sPlayerPtr = *playerPattern.get_first<void*>(2);

            LEDEffects::Inject([]()
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (sPlayerPtr)
                {
                    auto LeonHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x24, 0xF10);
                    auto HelenaHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x28, 0xF10);
                    auto ChrisHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x2C, 0xF10);
                    auto PiersHealthHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x30, 0xF10);
                    auto JakeHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x34, 0xF10);
                    auto SherryHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x38, 0xF10);
                    auto AdaHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x3C, 0xF10);
                    auto AgentHealth = PtrWalkthrough<int16_t>(sPlayerPtr, 0x40, 0xF10);

                    int FineR = 0x84, FineG = 0xDE, FineB = 0xFF;
                    int DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;

                    if ((LeonHealth && HelenaHealth) || (ChrisHealth && PiersHealthHealth) || (JakeHealth && SherryHealth) || AdaHealth)
                    {
                        int16_t health1 = 0;
                        int16_t health2 = 0;

                        if (LeonHealth && HelenaHealth)
                        {
                            health1 = *LeonHealth;
                            health2 = *HelenaHealth;
                        }
                        else if (ChrisHealth && PiersHealthHealth)
                        {
                            health1 = *ChrisHealth;
                            health2 = *PiersHealthHealth;

                            FineR = 0xA4, FineG = 0xB8, FineB = 0x39;
                            DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;
                        }
                        else if (JakeHealth && SherryHealth)
                        {
                            health1 = *JakeHealth;
                            health2 = *SherryHealth;

                            FineR = 0xC1, FineG = 0xD5, FineB = 0x42;
                            DangerR = 0x89, DangerG = 0x13, DangerB = 0x1D;
                        }
                        else
                        {
                            health1 = *AdaHealth;
                            health2 = AgentHealth ? *AgentHealth : -1;
                        }

                        auto [FineRP, FineGP, FineBP] = LEDEffects::RGBtoPercent(FineR, FineG, FineB);
                        auto [FineRPDimmed, FineGPDimmed, FineBPDimmed] = LEDEffects::RGBtoPercent(FineR, FineG, FineB, 0.5f);
                        auto [DangerRP, DangerGP, DangerBP] = LEDEffects::RGBtoPercent(DangerR, DangerG, DangerB);
                        auto [DangerRPDimmed, DangerGPDimmed, DangerBPDimmed] = LEDEffects::RGBtoPercent(DangerR, DangerG, DangerB, 0.5f);

                        if (health1 >= 1)
                        {
                            if (health1 <= 150)
                            {
                                LEDEffects::SetLightingLeftSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, true, false); //red
                                LEDEffects::DrawCardiogram(DangerRP, DangerGP, DangerBP, 0, 0, 0); //red
                            }
                            else
                            {
                                LEDEffects::SetLightingLeftSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, true, false);  //green
                                LEDEffects::DrawCardiogram(FineRP, FineGP, FineBP, 0, 0, 0); //green
                            }
                        }
                        else
                        {
                            LEDEffects::SetLightingLeftSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, true); //red
                            LEDEffects::DrawCardiogram(DangerRP, DangerGP, DangerBP, 0, 0, 0, true);
                        }

                        if (health2 >= 1)
                        {
                            if (health2 <= 150)
                            {
                                LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, true, false); //red
                                LEDEffects::DrawCardiogramNumpad(DangerRP, DangerGP, DangerBP, 0, 0, 0); //red
                            }
                            else
                            {
                                LEDEffects::SetLightingRightSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, true, false);  //green
                                LEDEffects::DrawCardiogramNumpad(FineRP, FineGP, FineBP, 0, 0, 0); //green
                            }
                        }
                        else if (health2 == 0)
                        {
                            LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, true); //red
                            LEDEffects::DrawCardiogramNumpad(DangerRP, DangerGP, DangerBP, 0, 0, 0, true);
                        }
                        else
                        {
                            if (health1 <= 150)
                                LEDEffects::SetLightingRightSide(DangerRPDimmed, DangerGPDimmed, DangerBPDimmed, false, false); //red
                            else
                                LEDEffects::SetLightingRightSide(FineRPDimmed, FineGPDimmed, FineBPDimmed, false, false);  //green
                        }
                    }
                    else
                    {
                        LogiLedStopEffects();
                        LEDEffects::SetLighting(31, 25, 70); //logo purple
                    }
                }
            });
        };
    }
} LightSync;
