module;

#include <stdafx.h>
#include "common.h"
#include "callargs.h"

export module Frontend;

import Skeleton;
import Sprite2d;
import Draw;

auto INV_SCREEN_WIDTH = [](float fAspectRatio) { return (1.0f / 640.0f) / (fAspectRatio / (4.0f / 3.0f)); };

export enum FrontendClass
{
    eCDarkel,
    eCMenuManager,
    eLoadingScreen,
    eCRadar,
    eCHud,
    ecMusicManager,
    eConvertingTexturesScreen,
    eCReplay,

    FrontendClassCount
};

export std::array<ProtectedGameRef<float>, FrontendClassCount> ResXInvRefs;
export std::array<ProtectedGameRef<float>, FrontendClassCount> ResYInvRefs;

namespace CMenuManager
{
    SafetyHookInline shStretchX = {};
    float __fastcall StretchX(void* CMenuManager, void* edx, float fScaleFactor)
    {
        if (fScaleFactor == 225.0f || fScaleFactor == 415.0f) //logo
            fScaleFactor = ((640.0f * (CDraw::GetAspectRatio() / (4.0f / 3.0f))) / 2.0f) - (320.0f - fScaleFactor);

        if (fScaleFactor == 320.0f || fScaleFactor == 321.0f)
            return fScaleFactor * 0.0015625f * (float)RsGlobal->maximumWidth;
        else
            return fScaleFactor * INV_SCREEN_WIDTH(CDraw::GetAspectRatio()) * (float)RsGlobal->maximumWidth;
    }
}

ProtectedGameRef<CVector> playerSkinPos;

class Frontend
{
public:
    Frontend()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");
            static float fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.0f); fHudWidthScale == 0.0f ? fHudWidthScale = 1.0f : fHudWidthScale;
            static float fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.0f); fHudHeightScale == 0.0f ? fHudHeightScale = 1.0714285f : fHudHeightScale;
            static float fRadarWidthScale = iniReader.ReadFloat("MAIN", "RadarWidthScale", 0.0f); fRadarWidthScale == 0.0f ? fRadarWidthScale = 1.0f : fRadarWidthScale;
            static float fRadarHeightScale = iniReader.ReadFloat("MAIN", "RadarHeightScale", 0.0f); fRadarHeightScale == 0.0f ? fRadarHeightScale = 1.0f : fRadarHeightScale;
            static float fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.0f); fSubtitlesScale == 0.0f ? fSubtitlesScale = 1.0f : fSubtitlesScale;
            auto bIVRadarScaling = iniReader.ReadInteger("MISC", "IVRadarScaling", 0) != 0;

            constexpr auto INV_DEFAULT_SCREEN_HEIGHT = 1.0f / (float)DEFAULT_SCREEN_HEIGHT;
            constexpr auto INV_DEFAULT_SCREEN_HEIGHT_MENU = 1.0f / 448.0f;

            auto SetResX = [](hook::pattern pattern, FrontendClass id)
            {
                if (pattern.empty()) return;
                ResXInvRefs[id].SetAddress(*pattern.get_first<float*>(2));
            };

            auto SetResY = [](hook::pattern pattern, FrontendClass id)
            {
                if (pattern.empty()) return;
                ResYInvRefs[id].SetAddress(*pattern.get_first<float*>(2));
            };

            auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DA 6C 24 ? D9 1C 24 E8 ? ? ? ? 83 C4 0C 8D 4C 24 ? 68 FF 00 00 00 68 FF 00 00 00");
            SetResX(pattern, FrontendClass::eCDarkel);
            pattern = hook::pattern("D8 0D ? ? ? ? 50 D8 0D ? ? ? ? D9 1C 24 A1 ? ? ? ? 89 44 24 ? DB 44 24");
            SetResY(pattern, FrontendClass::eCDarkel);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 E8 ? ? ? ? 8D 4C 24");
            SetResX(pattern, FrontendClass::eCMenuManager);
            pattern = hook::pattern("D8 0D ? ? ? ? 89 04 24 DA 0C 24 83 C4 10 C2 04 00");
            if (!pattern.empty())
                ResYInvRefs[FrontendClass::eCMenuManager].SetAddress(*pattern.count(2).get(1).get<float*>(2));

            pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA DA 6C 24 ? D9 05");
            SetResX(pattern, FrontendClass::eLoadingScreen);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? 50 D9 1C 24 DB 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? 50 D9 1C 24 E8 ? ? ? ? 59 59 68");
            SetResY(pattern, FrontendClass::eLoadingScreen);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DD DA D9 00");
            SetResX(pattern, FrontendClass::eCRadar);
            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 DD DA");
            SetResY(pattern, FrontendClass::eCRadar);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? FF 35 ? ? ? ? E8 ? ? ? ? 59 E8 ? ? ? ? E8");
            SetResX(pattern, FrontendClass::eCHud);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? FF 35 ? ? ? ? E8 ? ? ? ? 59 E8 ? ? ? ? E8");
            SetResY(pattern, FrontendClass::eCHud);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A 02");
            SetResX(pattern, FrontendClass::ecMusicManager);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? 6A 02");
            SetResY(pattern, FrontendClass::ecMusicManager);

            pattern = hook::pattern("D8 0D ? ? ? ? DD D9 D9 05 ? ? ? ? D8 C9 D9 1C 24");
            SetResX(pattern, FrontendClass::eConvertingTexturesScreen);
            pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 05 ? ? ? ? D8 CA D9 1C 24 DB 05 ? ? ? ? 50");
            SetResY(pattern, FrontendClass::eConvertingTexturesScreen);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1");
            SetResX(pattern, FrontendClass::eCReplay);
            pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 59 59 E8 ? ? ? ? E8 ? ? ? ? A1");
            SetResY(pattern, FrontendClass::eCReplay);

            pattern = hook::pattern("BE ? ? ? ? 8D 7C 24 ? A5 A5 A5 BE ? ? ? ? A1");
            playerSkinPos.SetAddress(*pattern.get_first<CVector*>(1));

            static float fHeadRadioOffset = 0.0f;
            static float fDoubRadioOffset = 0.0f;
            static float fKjahRadioOffset = 0.0f;
            static float fRiseRadioOffset = 0.0f;
            static float fLipsRadioOffset = 0.0f;
            static float fGameRadioOffset = 0.0f;
            static float fMsxxRadioOffset = 0.0f;
            static float fFlasRadioOffset = 0.0f;
            static float fChatRadioOffset = 0.0f;
            static float fMp33RadioOffset = 0.0f;

            onResChange() += [](int Width, int Height)
            {
                float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

                for (auto [index, resXInv] : std::views::enumerate(ResXInvRefs))
                {
                    resXInv = INV_SCREEN_WIDTH(fAspectRatio);
                }

                for (auto [index, resYInv] : std::views::enumerate(ResYInvRefs))
                {
                    if (index != FrontendClass::eCMenuManager)
                        resYInv = INV_DEFAULT_SCREEN_HEIGHT;
                    else
                        resYInv = INV_DEFAULT_SCREEN_HEIGHT_MENU;
                }

                constexpr float f1 = 1.35f / (640.0f / 2.0f - 110.0f) * (640.0f / 2.0f);
                constexpr float f2 = f1 - 1.35f;
                playerSkinPos->x = f1 * (fAspectRatio / (4.0f / 3.0f)) - f2;

                float fBaseOffset = (640.0f - (30.0f + 30.0f)) * (fAspectRatio / (4.0f / 3.0f)) / 10.0f;
                fHeadRadioOffset = fBaseOffset * 1.0f;
                fDoubRadioOffset = fBaseOffset * 2.0f;
                fKjahRadioOffset = fBaseOffset * 3.0f;
                fRiseRadioOffset = fBaseOffset * 4.0f;
                fLipsRadioOffset = fBaseOffset * 5.0f;
                fGameRadioOffset = fBaseOffset * 6.0f;
                fMsxxRadioOffset = fBaseOffset * 7.0f;
                fFlasRadioOffset = fBaseOffset * 8.0f;
                fChatRadioOffset = fBaseOffset * 9.0f;
                fMp33RadioOffset = fBaseOffset * 10.0f;
            };

            pattern = hook::pattern("FF 35 ? ? ? ? E8 ? ? ? ? 8D 84 24 ? ? 00 00 50");
            injector::WriteMemory(pattern.count(10).get(0).get<uint32_t>(2), &fHeadRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(1).get<uint32_t>(2), &fDoubRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(2).get<uint32_t>(2), &fKjahRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(3).get<uint32_t>(2), &fRiseRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(4).get<uint32_t>(2), &fLipsRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(5).get<uint32_t>(2), &fGameRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(6).get<uint32_t>(2), &fMsxxRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(7).get<uint32_t>(2), &fFlasRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(8).get<uint32_t>(2), &fChatRadioOffset, true);
            injector::WriteMemory(pattern.count(10).get(9).get<uint32_t>(2), &fMp33RadioOffset, true);

            pattern = hook::pattern("E8 ? ? ? ? 8B 8B ? ? ? ? 8D 84 24");
            CMenuManager::shStretchX = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), CMenuManager::StretchX);

            if (fHudWidthScale || fHudHeightScale)
            {
                static float fCustomWideScreenWidthScaleDown = 1.0f / 640.0f;
                static float fCustomWideScreenHeightScaleDown = 1.0f / 480.0f;

                onResChange() += [](int Width, int Height)
                {
                    float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);
                    fCustomWideScreenWidthScaleDown = INV_SCREEN_WIDTH(fAspectRatio) * fHudWidthScale;
                    fCustomWideScreenHeightScaleDown = INV_DEFAULT_SCREEN_HEIGHT * fHudHeightScale;
                };

                auto temp1 = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? DD D9 D9 44 24 1C D8 C1 D9 1C 24").count(1).get(0).get<uint32_t*>(2);
                auto DrawHudHorScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp1)));

                auto temp2 = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? DD DA D9 44 24 1C D8 C2 D9 1C 24").count(1).get(0).get<uint32_t*>(2);
                auto DrawHudVerScalePattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp2)));

                for (size_t i = 0; i < DrawHudHorScalePattern.size(); i++)
                {
                    if (i > 2 && i != 5 && i != 6 && i != 7 && i != 8 && i != 60 && i != 65 && i != 81) //0 1 2 - crosshair / 5 6 7 8 - sniper scope / 60 - radar disc / 65 - subs / 81 - IntroTextLines
                    {
                        uint32_t* pCustomScaleHor = DrawHudHorScalePattern.get(i).get<uint32_t>(2);
                        injector::WriteMemory(pCustomScaleHor, &fCustomWideScreenWidthScaleDown, true);
                    }
                }

                for (size_t i = 0; i < DrawHudVerScalePattern.size(); i++)
                {
                    if (i > 2 && i != 5 && i != 6 && i != 7 && i != 8 && i != 57 && i != 60 && i != 73) //0 1 2 - crosshair / 5 6 7 8 - sniper scope / 57 - radar disc / 60 - subs / 73 - IntroTextLines
                    {
                        uint32_t* pCustomScaleVer = DrawHudVerScalePattern.get(i).get<uint32_t>(2);
                        injector::WriteMemory(pCustomScaleVer, &fCustomWideScreenHeightScaleDown, true);
                    }
                }

                auto pattern = hook::pattern("50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8"); //0x57E954 radio text
                injector::WriteMemory(pattern.count(28).get(24).get<uint32_t>(3), &fCustomWideScreenWidthScaleDown, true);
                //injector::WriteMemory(pattern.count(28).get(25).get<uint32_t>(3), &fCustomWideScreenWidthScaleDown, true);
                injector::WriteMemory(pattern.count(28).get(27).get<uint32_t>(3), &fCustomWideScreenWidthScaleDown, true);//0x595F45 replay

                pattern = hook::pattern("50 D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 DB 05 ? ? ? ? 50"); //0x57E93E radio text
                injector::WriteMemory(pattern.count(19).get(16).get<uint32_t>(3), &fCustomWideScreenHeightScaleDown, true);
                injector::WriteMemory(pattern.count(19).get(18).get<uint32_t>(3), &fCustomWideScreenHeightScaleDown, true);//0x620C2F replay

                auto temp3 = *hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? DA 6C 24 0C D9 1C 24 ").count(1).get(0).get<uint32_t*>(2);
                auto CDarkelDrawMessagesPattern = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp3))); //0x5ECD00

                auto temp4 = *hook::pattern("D8 0D ? ? ? ? 50 D8 0D ? ? ? ? D9 1C 24 A1 ? ? ? ? 89 44 24 08").count(1).get(0).get<uint32_t*>(2);
                auto CDarkelDrawMessagesPattern2 = hook::pattern(pattern_str(0xD8, 0x0D, to_bytes(temp4))); //0x5ECCF8

                CDarkelDrawMessagesPattern.for_each_result([&](const hook::pattern_match& match)
                {
                    injector::WriteMemory(match.get<void*>(2), &fCustomWideScreenWidthScaleDown, true);
                });

                CDarkelDrawMessagesPattern2.for_each_result([&](const hook::pattern_match& match)
                {
                    injector::WriteMemory(match.get<void*>(2), &fCustomWideScreenHeightScaleDown, true);
                });
            }

            static std::vector<ProtectedGameRef<float>> RadarWidthRefs;
            static std::vector<ProtectedGameRef<float>> RadarHeightRefs;

            {
                auto pattern = hook::pattern("D8 0D ? ? ? ? DD DA D9 00");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarWidthRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D8 0D ? ? ? ? D8 05 ? ? ? ? D8 05 ? ? ? ? D9 1C 24");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarWidthRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 DD DA D9 40");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarHeightRefs.push_back(std::move(ref));
                }

                pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 D9 05 ? ? ? ? D8 CA DA 6C 24 ? DD DA D8 C1 D8 05");
                {
                    ProtectedGameRef<float> ref;
                    ref.SetAddress(*pattern.get_first<float*>(2));
                    RadarHeightRefs.push_back(std::move(ref));
                }
            }

            if (!bIVRadarScaling)
            {
                for (auto& radarWidth : RadarWidthRefs)
                {
                    radarWidth = 86.0f;
                }

                for (auto& radarHeight : RadarHeightRefs)
                {
                    radarHeight = 86.0f;
                }
            }

            if (fRadarWidthScale && !bIVRadarScaling)
            {
                for (auto& radarWidth : RadarWidthRefs)
                {
                    radarWidth *= fRadarWidthScale;
                }
            }

            if (fRadarHeightScale && !bIVRadarScaling)
            {
                for (auto& radarHeight : RadarHeightRefs)
                {
                    radarHeight *= fRadarHeightScale;
                }
            }

            if (bIVRadarScaling)
            {
                float fCustomRadarWidthIV = 92.0f;
                float fCustomRadarHeightIV = 92.0f;

                for (auto& radarWidth : RadarWidthRefs)
                {
                    radarWidth = fCustomRadarWidthIV;
                }

                for (auto& radarHeight : RadarHeightRefs)
                {
                    radarHeight = fCustomRadarHeightIV;
                }
            }

            RadarWidthRefs.back() += 1.5f; // expand radardisc a bit
        };
    }
} Frontend;