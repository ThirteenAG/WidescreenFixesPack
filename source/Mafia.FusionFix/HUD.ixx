module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cmath>

export module HUD;

import ComVars;

export void InitHUD()
{
    static auto pOnePCurBaseWidth = find_pattern("D8 0D ? ? ? ? D9 5C 24 18 8B 4C 24 18 89 8D C0 90 00 00",
        "D8 0D ? ? ? ? D9 5C 24 20 8B 4C 24 20 89 8E C0 40 00 00 A1 ? ? ? ? 50 8B 08 FF 51 74 89 44 24 20 A1 ? ? ? ? DB 44 24 20 8B 10 50 D8 0D ? ? ? ? D9 5C 24 14 FF 52 70 89 44 24 20 DB 44 24 20 D8 0D ? ? ? ? D9 5C 24 20 E9 ? ? ? ?");
    static auto pOnePCurBaseWidth2 = find_pattern("D8 0D ? ? ? ? 8B 44 24 10 8D 9D E4 91 00 00",
        "D8 0D ? ? ? ? D9 5C 24 20 E9 ? ? ? ? FF 51 74");
    static auto pHudSpeedometerX = find_pattern("D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 44 24 20 D8 E2 89 44 24 1C",
        "D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 05 ? ? ? ? D8 E2 D9 5C 24 20 D9 05 ? ? ? ? D8 E1 8B 44 24 20");
    static auto pHudTachometerX = find_pattern("D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 44 24 20 D8 E2 D9 5C 24 18",
        "D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 05 ? ? ? ? D8 E2 D9 5C 24 20 D9 05 ? ? ? ? D8 E1 8B 54 24 20");
    static auto pHudSpeedLimiterX = find_pattern("C7 44 24 30 00 00 3E 44 C7 44 24 34 00 00 BE 43",
        "C7 44 24 30 00 00 20 C2 C7 44 24 34 00 00 5C C3");
    static auto pHudTimerX = find_pattern("C7 44 24 18 00 80 2D 44 8D 4C 24 28 53 51 8D 4C 24 20",
        "C7 44 24 20 00 00 D4 C2 C7 44 24 24 00 00 A0 41");
    static auto pHudCompasX = find_pattern("D8 0D ? ? ? ? 83 CE FF C7 84 24 84 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? D9 85 C4 40 00 00 D8 0D ? ? ? ? D9 C9 D8 05 ? ? ? ? 83 CE FF");
    static auto pHudMinimapX = find_pattern("D8 0D ? ? ? ? D9 54 24 18 D9 85 C4 90 00 00",
        "D8 0D ? ? ? ? D9 85 C4 40 00 00 D8 0D ? ? ? ? D9 C9 D8 05 ? ? ? ? D9 5C 24 20");
    static auto pHudTransmissionX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? 8A 85 FC 91 00 00",
        "D8 0D ? ? ? ? 8A 85 04 42 00 00 C7 84 24 E8 00 00 00 00 00 80 3F");
    static auto pHudFuelLedX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? C7 84 24 84 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? C7 84 24 94 00 00 00 00 00 00 00 C7 84 24 98 00 00 00 00 00 80 3F 89 9C 24 9C 00 00 00");
    static auto pHudActionBarX = find_pattern("D8 0D ? ? ? ? 8D 0C 76 89 7C 24 44 C1 E0 18",
        "D8 0D ? ? ? ? 8D 0C 76 89 7C 24 40 C1 E0 18");
    static auto pHudHealthBarX = find_pattern("D8 0D ? ? ? ? 2B DA BA 00 00 80 3F 89 5C 24 10");
    static auto pHudHealthBarTextX = find_pattern("D8 0D ? ? ? ? 8D 04 90 BA 3C 02 00 00 03 C0",
        "D8 0D ? ? ? ? 8D 14 40 57 33 FF D8 05 ? ? ? ?");
    static auto pHudAmmoBarX = find_pattern("D8 0D ? ? ? ? D9 81 C4 90 00 00 D8 0D ? ? ? ? 8B 54 24 08",
        "D8 0D ? ? ? ? 8B 54 24 08 8B 44 24 04 55 56");
    static auto pHudAmmoBarTextX = find_pattern("D8 0D ? ? ? ? D9 81 C4 90 00 00 D8 0D ? ? ? ? 53",
        "D8 0D ? ? ? ? 53 55 56 8B 74 24 24 D8 05 ? ? ? ?");
    static auto pHudSearchBarX = find_pattern("D8 0D ? ? ? ? 6A 01 C7 84 24 88 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? 6A 01 C7 84 24 98 00 00 00 00 00 00 00");
    static auto pHudProgressBarX = find_pattern("F6 C4 10 0F 84 ? ? ? ? D9 85 C0 90 00 00 D8 0D ? ? ? ?",
        "F6 C4 10 0F 84 ? ? ? ? D9 85 C0 40 00 00 D8 0D ? ? ? ?");
    static auto pHudProgressBar2X = find_pattern("F7 85 A4 90 00 00 00 00 04 00 0F 84 ? ? ? ?",
        "F7 85 A4 40 00 00 00 00 04 00 0F 84 ? ? ? ?");
    static auto pHudLeftPopupTextX = find_pattern("D8 0D ? ? ? ? 68 00 00 20 41 50 51 8D 8C 2E 74 92 00 00",
        "D8 0D ? ? ? ? 68 00 00 20 41 50 51 8D 8C 2E 7C 42 00 00");
    static auto pHudMoneyRightTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 8B 9D A4 90 00 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 8B 9D A4 40 00 00");
    static auto pHudRace321GoX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? D9 5C 24 44 D9 44 24 4C DC 0D ? ? ? ? DE E9 74 ? D9 05 ? ? ? ? EB ? D9 44 24 10",
        "68 00 00 C8 43 8D 8C 24 90 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 50");
    static auto pHudRace321GoX2 = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? D9 5C 24 44 D9 44 24 4C DC 0D ? ? ? ? DE E9 74 ? D9 05 ? ? ? ? EB ? D9 05 ? ? ? ?",
        "68 00 00 C8 43 8D 8C 24 90 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8D 4C 24 48");
    static auto pHudRaceWrongWayX = find_pattern("68 00 00 88 43 8D 8C 24 80 00 00 00 E8 ? ? ? ?",
        "68 00 00 88 43 8D 8C 24 90 00 00 00 E8 ? ? ? ?");
    static auto pHudRaceCheckpointX = find_pattern("C7 44 24 48 00 00 C3 43 C7 44 24 4C 00 00 16 43",
        "68 00 00 C3 43 8D 8C 24 90 00 00 00 E8 ? ? ? ?");
    static auto pHudRaceRightTextX = find_pattern("D8 0D ? ? ? ? 6A 00 6A 01 6A 04 6A FF 8D 95 CC 94 00 00",
        "D8 0D ? ? ? ? 6A 00 6A 01 6A 04 6A FF D8 05 ? ? ? ?");
    static auto pHudMapTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 8A 85 A4 90 00 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 8A 85 A4 40 00 00");
    static auto pHudBlackCentredTextX = find_pattern("D8 2D ? ? ? ? F7 EE D9 5C 24 14 D9 44 24 1C");
    static auto pHudCreditsText2X = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 5E",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 5E");
    static auto pHudCreditsText2X2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 83 EC 10 83 C6 0C D9 5C 24 0C D9 05 ? ? ? ?");
    static auto pHudCreditsText2X3 = find_pattern("D8 0D ? ? ? ? E9 ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 01");
    static auto pHudCreditsText1X = find_pattern("D8 0D ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 D9 05 ? ? ? ?");
    static auto pHudCreditsText3X = find_pattern("D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02",
        "D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 D9 5C 24 18 D9 05 ? ? ? ? D8 0D ? ? ? ? 8B 4C 24 18 D9 1C 24 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02");
    static auto pHudCreditsText3X2 = find_pattern("D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 83 C6 4C",
        "D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 D9 5C 24 18");
    static auto pHudCreditsText2X4 = find_pattern("D8 0D ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 5E",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 5E");
    static auto pHudCreditsText2X5 = find_pattern("68 00 00 C3 43 56 B9 ? ? ? ? E8 ? ? ? ?");
    static auto pHudCreditsText1X2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00");
    static auto pHudCreditsText4X = find_pattern("D8 0D ? ? ? ? D9 1C 24 56 B9 ? ? ? ? E8 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? B9 ? ? ? ?");
    static auto pHudLiveBarX = find_pattern("D8 0D ? ? ? ? C7 84 24 84 00 00 00 00 00 00 00 C7 84 24 88 00 00 00 00 00 80 3F 89 9C 24 8C 00 00 00 C7 84 24 94 00 00 00 00 00 00 00 C7 84 24 98 00 00 00 00 00 40 3E");
    static auto pHudCreditsText2X6 = find_pattern("68 00 00 C3 43 52 B9 ? ? ? ? E8 ? ? ? ?");

    if (pOnePCurBaseWidth.size() != 1 ||
        pOnePCurBaseWidth2.size() != 1 ||
        pHudSpeedometerX.size() != 1 ||
        pHudTachometerX.size() != 1 ||
        pHudSpeedLimiterX.size() != 1 ||
        pHudTimerX.size() != 1 ||
        pHudCompasX.size() != 1 ||
        pHudMinimapX.size() != 1 ||
        pHudTransmissionX.size() != 1 ||
        pHudFuelLedX.size() != 1 ||
        pHudActionBarX.size() != 1 ||
        pHudHealthBarX.size() != 1 ||
        pHudHealthBarTextX.size() != 1 ||
        pHudAmmoBarX.size() != 1 ||
        pHudAmmoBarTextX.size() != 1 ||
        pHudSearchBarX.size() != 1 ||
        pHudProgressBarX.size() != 1 ||
        pHudProgressBar2X.size() != 1 ||
        pHudLeftPopupTextX.size() != 1 ||
        pHudMoneyRightTextX.size() != 1 ||
        pHudRace321GoX.size() != 1 ||
        pHudRace321GoX2.size() != 1 ||
        pHudRaceWrongWayX.size() != 1 ||
        pHudRaceCheckpointX.size() != 1 ||
        pHudRaceRightTextX.size() != 1 ||
        pHudMapTextX.size() != 1 ||
        pHudBlackCentredTextX.size() != 1 ||
        pHudCreditsText2X.size() != 1 ||
        pHudCreditsText2X2.size() != 1 ||
        pHudCreditsText2X3.size() != 1 ||
        pHudCreditsText1X.size() != 1 ||
        pHudCreditsText3X.size() != 1 ||
        pHudCreditsText3X2.size() != 1 ||
        pHudCreditsText2X4.size() != 1 ||
        pHudCreditsText2X5.size() != 1 ||
        pHudCreditsText1X2.size() != 1 ||
        pHudCreditsText4X.size() != 1)
        return;
    if (nGameVersion == MAFIA_1_0_ENG && (pHudLiveBarX.size() != 1)) return;

    ProtectedGameRef<std::remove_cvref_t<decltype(&Screen.fInvBaseWidth)>>{pOnePCurBaseWidth.get_first<std::remove_cvref_t<decltype(&Screen.fInvBaseWidth)>>(0x2)} = &Screen.fInvBaseWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&Screen.fInvBaseWidth)>>{pOnePCurBaseWidth2.get_first<std::remove_cvref_t<decltype(&Screen.fInvBaseWidth)>>(0x2)} = &Screen.fInvBaseWidth;

    const auto ratio = ParseWidescreenHudOffset(FixSettings().String("MAIN.HudAspectRatioConstraint"));
    const float hudAspect = ratio ? ClampHudAspectRatio(*ratio, Screen.fAspectRatio) : Screen.fAspectRatio;
    const float inset = (Screen.fBaseWidth - 600.0f * hudAspect) * 0.5f;
    bWidescreenHUD = FixSettings().Bool("WIDESCREEN.WidescreenHUD");
    static const bool subscribed = []()
    {
        FixSettings().OnChange("MAIN.HudAspectRatioConstraint", InitHUD);
        FixSettings().OnChange("WIDESCREEN.WidescreenHUD", InitHUD);
        return true;
    }();
    (void)subscribed;

    float fixHudLeft;
    float fixHudCenter;
    float fixHudRight;
    if (!bWidescreenHUD)
    {
        fixHudLeft = (Screen.fBaseWidth - 800.0f) / 2.0f;
        fixHudCenter = (Screen.fBaseWidth - 800.0f) / 2.0f;
        fixHudRight = (Screen.fBaseWidth - 800.0f) / 2.0f;
    }
    else
    {
        fixHudLeft = inset;
        fixHudCenter = (Screen.fBaseWidth - 800.0f) / 2.0f;
        fixHudRight = Screen.fBaseWidth - 800.0f - inset;
    }

    static float hudSpeedometerX;
    hudSpeedometerX = 189.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudSpeedometerX)>>{pHudSpeedometerX.get_first<std::remove_cvref_t<decltype(&hudSpeedometerX)>>(0x2)} = &hudSpeedometerX;
    static float hudTachometerX;
    hudTachometerX = 259.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudTachometerX)>>{pHudTachometerX.get_first<std::remove_cvref_t<decltype(&hudTachometerX)>>(0x2)} = &hudTachometerX;
    float hudSpeedLimiterX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudSpeedLimiterX = 760.0f + fixHudRight;
    }
    else
    {
        hudSpeedLimiterX = -40.0f - fixHudLeft;
    }
    ProtectedGameRef<std::remove_cvref_t<decltype(hudSpeedLimiterX)>>{pHudSpeedLimiterX.get_first<std::remove_cvref_t<decltype(hudSpeedLimiterX)>>(0x4)} = hudSpeedLimiterX;
    float hudTimerX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudTimerX = 694.0f + fixHudRight;
    }
    else
    {
        hudTimerX = -106.0f - fixHudLeft;
    }
    ProtectedGameRef<std::remove_cvref_t<decltype(hudTimerX)>>{pHudTimerX.get_first<std::remove_cvref_t<decltype(hudTimerX)>>(0x4)} = hudTimerX;
    static float hudCompasX;
    hudCompasX = 0.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCompasX)>>{pHudCompasX.get_first<std::remove_cvref_t<decltype(&hudCompasX)>>(0x2)} = &hudCompasX;
    static float hudMinimapX;
    hudMinimapX = 29.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudMinimapX)>>{pHudMinimapX.get_first<std::remove_cvref_t<decltype(&hudMinimapX)>>(0x2)} = &hudMinimapX;
    static float hudTransmissionX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudTransmissionX = 758.0f + fixHudRight;
    }
    else
    {
        hudTransmissionX = 42.0f + fixHudLeft;
    }
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudTransmissionX)>>{pHudTransmissionX.get_first<std::remove_cvref_t<decltype(&hudTransmissionX)>>(0x2)} = &hudTransmissionX;
    static float hudFuelLedX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudFuelLedX = 668.0f + fixHudRight;
    }
    else
    {
        hudFuelLedX = 132.0f + fixHudLeft;
    }
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudFuelLedX)>>{pHudFuelLedX.get_first<std::remove_cvref_t<decltype(&hudFuelLedX)>>(0x2)} = &hudFuelLedX;
    static float hudActionBarX;
    hudActionBarX = 9.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudActionBarX)>>{pHudActionBarX.get_first<std::remove_cvref_t<decltype(&hudActionBarX)>>(0x2)} = &hudActionBarX;
    static float hudHealthBarX;
    hudHealthBarX = 7.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudHealthBarX)>>{pHudHealthBarX.get_first<std::remove_cvref_t<decltype(&hudHealthBarX)>>(0x2)} = &hudHealthBarX;
    static float hudHealthBarTextX;
    hudHealthBarTextX = 42.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudHealthBarTextX)>>{pHudHealthBarTextX.get_first<std::remove_cvref_t<decltype(&hudHealthBarTextX)>>(0x2)} = &hudHealthBarTextX;
    static float hudAmmoBarX;
    hudAmmoBarX = 62.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudAmmoBarX)>>{pHudAmmoBarX.get_first<std::remove_cvref_t<decltype(&hudAmmoBarX)>>(0x2)} = &hudAmmoBarX;
    static float hudAmmoBarTextX;
    hudAmmoBarTextX = 112.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudAmmoBarTextX)>>{pHudAmmoBarTextX.get_first<std::remove_cvref_t<decltype(&hudAmmoBarTextX)>>(0x2)} = &hudAmmoBarTextX;
    static float hudSearchBarX;
    hudSearchBarX = 336.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudSearchBarX)>>{pHudSearchBarX.get_first<std::remove_cvref_t<decltype(&hudSearchBarX)>>(0x2)} = &hudSearchBarX;
    static float hudLiveBarX;
    hudLiveBarX = 368.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(&hudLiveBarX)>>{pHudLiveBarX.get_first<std::remove_cvref_t<decltype(&hudLiveBarX)>>(0x2)} = &hudLiveBarX;
    }
    static float hudProgressBarX;
    hudProgressBarX = 250.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudProgressBarX)>>{pHudProgressBarX.get_first<std::remove_cvref_t<decltype(&hudProgressBarX)>>(0x11)} = &hudProgressBarX;
    static float hudProgressBar2X;
    hudProgressBar2X = 350.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudProgressBar2X)>>{pHudProgressBar2X.get_first<std::remove_cvref_t<decltype(&hudProgressBar2X)>>(0x18)} = &hudProgressBar2X;
    static float hudLeftPopupTextX;
    hudLeftPopupTextX = 9.0f + fixHudLeft;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudLeftPopupTextX)>>{pHudLeftPopupTextX.get_first<std::remove_cvref_t<decltype(&hudLeftPopupTextX)>>(0x2)} = &hudLeftPopupTextX;
    static float hudMoneyRightTextX;
    hudMoneyRightTextX = 760.0f + fixHudRight;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudMoneyRightTextX)>>{pHudMoneyRightTextX.get_first<std::remove_cvref_t<decltype(&hudMoneyRightTextX)>>(0x2)} = &hudMoneyRightTextX;

    static float hudRace321goX;
    hudRace321goX = 400.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(&hudRace321goX)>>{pHudRace321GoX.get_first<std::remove_cvref_t<decltype(&hudRace321goX)>>(0x2)} = &hudRace321goX;
        ProtectedGameRef<std::remove_cvref_t<decltype(&hudRace321goX)>>{pHudRace321GoX2.get_first<std::remove_cvref_t<decltype(&hudRace321goX)>>(0x2)} = &hudRace321goX;
    }
    else
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(hudRace321goX)>>{pHudRace321GoX.get_first<std::remove_cvref_t<decltype(hudRace321goX)>>(0x1)} = hudRace321goX;
        ProtectedGameRef<std::remove_cvref_t<decltype(hudRace321goX)>>{pHudRace321GoX2.get_first<std::remove_cvref_t<decltype(hudRace321goX)>>(0x1)} = hudRace321goX;
    }
    float hudRaceWrongWayX = 272.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(hudRaceWrongWayX)>>{pHudRaceWrongWayX.get_first<std::remove_cvref_t<decltype(hudRaceWrongWayX)>>(0x1)} = hudRaceWrongWayX;
    float hudRaceCheckpointX = 390.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(hudRaceCheckpointX)>>{pHudRaceCheckpointX.get_first<std::remove_cvref_t<decltype(hudRaceCheckpointX)>>(0x4)} = hudRaceCheckpointX;
    }
    else
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(hudRaceCheckpointX)>>{pHudRaceCheckpointX.get_first<std::remove_cvref_t<decltype(hudRaceCheckpointX)>>(0x1)} = hudRaceCheckpointX;
    }
    static float hudRaceRightTextX;
    hudRaceRightTextX = 780.0f + fixHudRight;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudRaceRightTextX)>>{pHudRaceRightTextX.get_first<std::remove_cvref_t<decltype(&hudRaceRightTextX)>>(0x2)} = &hudRaceRightTextX;

    static float hudMapTextX;
    hudMapTextX = 350.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudMapTextX)>>{pHudMapTextX.get_first<std::remove_cvref_t<decltype(&hudMapTextX)>>(0x2)} = &hudMapTextX;

    static float hudBlackCentredTextX;
    hudBlackCentredTextX = 400.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudBlackCentredTextX)>>{pHudBlackCentredTextX.get_first<std::remove_cvref_t<decltype(&hudBlackCentredTextX)>>(0x2)} = &hudBlackCentredTextX;
    static float hudCreditsText1X;
    hudCreditsText1X = 370.0f + fixHudCenter;
    fCreditsTextX = 390.0f + fixHudCenter;
    static float hudCreditsText3X;
    hudCreditsText3X = 400.0f + fixHudCenter;
    static float hudCreditsText4X;
    hudCreditsText4X = 410.0f + fixHudCenter;
    ProtectedGameRef<std::remove_cvref_t<decltype(&fCreditsTextX)>>{pHudCreditsText2X.get_first<std::remove_cvref_t<decltype(&fCreditsTextX)>>(0x2)} = &fCreditsTextX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&fCreditsTextX)>>{pHudCreditsText2X2.get_first<std::remove_cvref_t<decltype(&fCreditsTextX)>>(0x2)} = &fCreditsTextX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&fCreditsTextX)>>{pHudCreditsText2X3.get_first<std::remove_cvref_t<decltype(&fCreditsTextX)>>(0x2)} = &fCreditsTextX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCreditsText1X)>>{pHudCreditsText1X.get_first<std::remove_cvref_t<decltype(&hudCreditsText1X)>>(0x2)} = &hudCreditsText1X;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCreditsText3X)>>{pHudCreditsText3X.get_first<std::remove_cvref_t<decltype(&hudCreditsText3X)>>(0x2)} = &hudCreditsText3X;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCreditsText3X)>>{pHudCreditsText3X2.get_first<std::remove_cvref_t<decltype(&hudCreditsText3X)>>(0x2)} = &hudCreditsText3X;
    ProtectedGameRef<std::remove_cvref_t<decltype(&fCreditsTextX)>>{pHudCreditsText2X4.get_first<std::remove_cvref_t<decltype(&fCreditsTextX)>>(0x2)} = &fCreditsTextX;
    ProtectedGameRef<std::remove_cvref_t<decltype(fCreditsTextX)>>{pHudCreditsText2X5.get_first<std::remove_cvref_t<decltype(fCreditsTextX)>>(0x1)} = fCreditsTextX;
    if (nGameVersion == MAFIA_1_2_ENG)
    {
        ProtectedGameRef<std::remove_cvref_t<decltype(fCreditsTextX)>>{pHudCreditsText2X6.get_first<std::remove_cvref_t<decltype(fCreditsTextX)>>(0x1)} = fCreditsTextX;
    }
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCreditsText1X)>>{pHudCreditsText1X2.get_first<std::remove_cvref_t<decltype(&hudCreditsText1X)>>(0x2)} = &hudCreditsText1X;
    ProtectedGameRef<std::remove_cvref_t<decltype(&hudCreditsText4X)>>{pHudCreditsText4X.get_first<std::remove_cvref_t<decltype(&hudCreditsText4X)>>(0x2)} = &hudCreditsText4X;

}

export void InitMap()
{
    auto pMapLeftX = find_pattern("D8 0D ? ? ? ? 8B 4C 24 34 8B 54 24 38 89 8C 24 94 00 00 00",
        "D8 0D ? ? ? ? D9 44 24 14 D8 0D ? ? ? ? D9 44 24 10");
    auto pMapRightX = find_pattern("D8 0D ? ? ? ? 8B 54 24 50 C7 84 24 84 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? C7 84 24 94 00 00 00 00 00 00 00 C7 84 24 98 00 00 00 00 00 80 3F C7 84 24 D4 00 00 00 00 00 00 00");
    auto pMapPlayer = find_pattern("D8 8D B8 90 00 00 D9 C9 D8 8D BC 90 00 00 D9 5C 24 44",
        "D8 4C 24 10 D9 C9 D8 4C 24 14 D9 5C 24 58 DC C0");
    auto pMapWidth = find_pattern("D8 0D ? ? ? ? 84 C9 D9 5C 24 40 D8 0D ? ? ? ?",
        "D8 0D ? ? ? ? 84 C9 D9 5C 24 54 D8 0D ? ? ? ?");
    auto pMapScaleX = find_pattern("D8 05 ? ? ? ? D8 1D ? ? ? ? DF E0 25 00 41 00 00 75 ? C7 44 24 34 9A 99 19 3F",
        "D8 05 ? ? ? ? D8 1D ? ? ? ? DF E0 25 00 41 00 00 75 ? C7 44 24 18 9A 99 19 3F");
    auto pConstant10FMapScaleX = find_pattern("C7 44 24 34 9A 99 19 3F D9 44 24 38 D8 05 ? ? ? ?",
        "C7 44 24 18 9A 99 19 3F D9 44 24 1C D8 05 ? ? ? ?");
    auto pMapScaleX2 = find_pattern("D8 05 ? ? ? ? D9 94 24 BC 00 00 00 D9 9C 24 FC 00 00 00",
        "D8 05 ? ? ? ? 89 8C 24 F0 00 00 00 8B 8D 90 40 00 00");
    auto pMapPosX = find_pattern("D8 25 ? ? ? ? D9 5C 24 34 D9 44 24 14 D8 25 ? ? ? ?",
        "D8 25 ? ? ? ? D9 5C 24 18 D9 44 24 24 D8 25 ? ? ? ?");
    auto pMapPosX2 = find_pattern("D8 25 ? ? ? ? D9 5C 24 40 D8 25 ? ? ? ?",
        "D8 25 ? ? ? ? D9 5C 24 54 D8 25 ? ? ? ?");
    auto pMapPosX3 = find_pattern("D8 25 ? ? ? ? 32 C9 D9 5C 24 40 D8 25 ? ? ? ?",
        "D8 25 ? ? ? ? 32 C9 D9 5C 24 3C D8 25 ? ? ? ?");
    auto pMapTargetPosX = find_pattern("D8 0D ? ? ? ? D9 5C 24 4C 8B 44 24 4C D8 0D ? ? ? ?",
        "D8 0D ? ? ? ? D9 5C 24 48 8B 44 24 48 D8 0D ? ? ? ?");

    if (pMapLeftX.size() != 1 ||
        pMapRightX.size() != 1 ||
        pMapPlayer.size() != 1 ||
        pMapWidth.size() != 1 ||
        pMapScaleX.size() != 1 ||
        pConstant10FMapScaleX.size() != 1 ||
        pMapScaleX2.size() != 1 ||
        pMapPosX.size() != 1 ||
        pMapPosX2.size() != 1 ||
        pMapPosX3.size() != 1 ||
        pMapTargetPosX.size() != 1)
        return;

    float fixMapLeft;
    if (!bWidescreenMap)
    {
        fixMapLeft = (Screen.fBaseWidth - 800.0f) / 2.0f;
    }
    else
    {
        fixMapLeft = 0.0f;
    }
    static float mapLeftX = (800.0f * 0.1f + fixMapLeft) / Screen.fBaseWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapLeftX)>>{pMapLeftX.get_first<std::remove_cvref_t<decltype(&mapLeftX)>>(0x2)} = &mapLeftX;
    static float mapRightX = 1.0f - mapLeftX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapRightX)>>{pMapRightX.get_first<std::remove_cvref_t<decltype(&mapRightX)>>(0x2)} = &mapRightX;
    fMapPlayerWidth = Screen.fHeight * (4.0f / 3.0f);
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        // Replace FMUL [ebp+HUD.width] with FMUL [fMapPlayerWidth]. Both
        // encodings occupy six bytes; the following native FXCH stays intact.
        ProtectedGameRef<std::remove_cvref_t<decltype(uint16_t{0x0DD8})>>{pMapPlayer.get_first<std::remove_cvref_t<decltype(uint16_t{0x0DD8})>>()} = uint16_t{0x0DD8};
        ProtectedGameRef<std::remove_cvref_t<decltype(&fMapPlayerWidth)>>{pMapPlayer.get_first<std::remove_cvref_t<decltype(&fMapPlayerWidth)>>(0x2)} = &fMapPlayerWidth;
    }
    else
    {
        // Later versions multiply by a stack-local width. Preserve the native
        // FMUL/FXCH sequence and restore the width before the next operation.
        // Copy integer bits here: both hooks run with a live x87 stack.
        static thread_local uint32_t savedWidth;
        static SafetyHookMid MapPlayerWidthHook;
        static auto MapPlayerRestoreHook = safetyhook::create_mid(pMapPlayer.get_first(6), [](SafetyHookContext& regs)
        {
            Field<uint32_t>(regs.esp, 0x10) = savedWidth;
        });
        if (MapPlayerRestoreHook)
        {
            MapPlayerWidthHook = safetyhook::create_mid(pMapPlayer.get_first(), [](SafetyHookContext& regs)
            {
                savedWidth = Field<uint32_t>(regs.esp, 0x10);
                Field<uint32_t>(regs.esp, 0x10) = std::bit_cast<uint32_t>(fMapPlayerWidth);
            });
            if (!MapPlayerWidthHook) MapPlayerRestoreHook.reset();
        }
    }
    static float mapWidth = mapRightX - mapLeftX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapWidth)>>{pMapWidth.get_first<std::remove_cvref_t<decltype(&mapWidth)>>(0x2)} = &mapWidth;

    float mapAspect = (mapWidth / (((800.0f * 0.9f + fixMapLeft) / Screen.fBaseWidth) - mapLeftX));
    static float mapScaleX = 0.4f * mapAspect;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapScaleX)>>{pMapScaleX.get_first<std::remove_cvref_t<decltype(&mapScaleX)>>(0x2)} = &mapScaleX;
    ProtectedGameRef<std::remove_cvref_t<decltype(1.0f - mapScaleX)>>{pConstant10FMapScaleX.get_first<std::remove_cvref_t<decltype(1.0f - mapScaleX)>>(0x4)} = 1.0f - mapScaleX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapScaleX)>>{pMapScaleX2.get_first<std::remove_cvref_t<decltype(&mapScaleX)>>(0x2)} = &mapScaleX;
    static float mapPosX = 0.2f * mapAspect;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapPosX)>>{pMapPosX.get_first<std::remove_cvref_t<decltype(&mapPosX)>>(0x2)} = &mapPosX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapPosX)>>{pMapPosX2.get_first<std::remove_cvref_t<decltype(&mapPosX)>>(0x2)} = &mapPosX;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapPosX)>>{pMapPosX3.get_first<std::remove_cvref_t<decltype(&mapPosX)>>(0x2)} = &mapPosX;
    static float mapTargetPosX = 2.5f / mapAspect;
    ProtectedGameRef<std::remove_cvref_t<decltype(&mapTargetPosX)>>{pMapTargetPosX.get_first<std::remove_cvref_t<decltype(&mapTargetPosX)>>(0x2)} = &mapTargetPosX;
}

export void InitText()
{
    auto pTextWidth = find_pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B F0 85 F6 74 ? 80 3E 00 74 ? 85 FF",
        "D8 0D ? ? ? ? 52 51 8B CB D9 1C 24 55 E8 ? ? ? ?");
    auto pTextWidth2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B F0 85 F6 75 ? 33 D2",
        "D8 0D ? ? ? ? 6A 00 50 51 8D 6E 01 8B CB D9 1C 24");
    auto pTextWidth3 = find_pattern("D8 0D ? ? ? ? D9 1C 24 57 E8 ? ? ? ? 8B F0 85 F6 74 ?",
        "D8 0D ? ? ? ? 50 51 8B CD D9 1C 24 57 E8 ? ? ? ?");
    auto pTextWidth4 = find_pattern("D8 0D ? ? ? ? D9 1C 24 57 E8 ? ? ? ? 8B F0 85 F6 75 ?",
        "D8 0D ? ? ? ? 6A 00 51 51 8D 7E 01 8B CD D9 1C 24");
    auto pTextWidth5 = find_pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B F0 85 F6 74 ? 80 3E 00 74 ? 8B 93 FC 93 00 00",
        "D8 0D ? ? ? ? F7 D1 49 6A 00 6A 00 8D 04 89");
    auto pTextWidth6 = find_pattern("D8 0D ? ? ? ? D9 1C 24 55 E8 ? ? ? ? 8B F0 85 F6 75 ? 8B 93 00 94 00 00",
        "D8 0D ? ? ? ? 6A 00 51 51 8D 6E 01 8B CB D9 1C 24");

    if (pTextWidth.size() != 1 ||
        pTextWidth2.size() != 1 ||
        pTextWidth3.size() != 1 ||
        pTextWidth4.size() != 1 ||
        pTextWidth5.size() != 1 ||
        pTextWidth6.size() != 1)
        return;

    static float textWidth;

    textWidth = (800.0f * 0.9f) / Screen.fBaseWidth;

    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth2.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth3.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth4.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth5.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
    ProtectedGameRef<std::remove_cvref_t<decltype(&textWidth)>>{pTextWidth6.get_first<std::remove_cvref_t<decltype(&textWidth)>>(0x2)} = &textWidth;
}

export void InitOthers()
{
    auto pFrameDelay = find_pattern("6A 10 8B 08 50 FF 91 F0 00 00 00 33 ED 89 44 24 10",
        "6A 10 8B 08 50 FF 91 00 01 00 00 33 ED 89 44 24 10");
    auto pCreditsCaseCount = find_pattern("83 F8 06 0F 87 ? ? ? ? FF 24 85 ? ? ? ? D9 05 ? ? ? ?");

    if (nFPSLimit > 0 && pFrameDelay.size() == 1)
        ProtectedGameRef<std::remove_cvref_t<decltype(static_cast<uint8_t>(std::clamp(1000 / nFPSLimit, 0, 127)))>>{pFrameDelay.get_first<std::remove_cvref_t<decltype(static_cast<uint8_t>(std::clamp(1000 / nFPSLimit, 0, 127)))>>(0x1)} = static_cast<uint8_t>(std::clamp(1000 / nFPSLimit, 0, 127));
    if (pCreditsCaseCount.size() != 1) return;
    auto dispatch = reinterpret_cast<uintptr_t>(pCreditsCaseCount.get_first()) + 9;
    static uintptr_t creditsEnd;
    static uintptr_t creditsHUD;
    static uintptr_t creditsDraw;
    creditsEnd = reinterpret_cast<uintptr_t>(pCreditsCaseCount.get_first()) + 9 + Field<int32_t>(reinterpret_cast<uintptr_t>(pCreditsCaseCount.get_first()), 5);
    auto pDraw = find_pattern("8B 44 24 24 8B 54 24 20 D9 44 24 14 D8 89 C4 90 00 00",
        "8B 44 24 24 8B 54 24 20 D9 44 24 14 D8 89 C4 40 00 00");
    auto draw = pDraw.size() == 1 ? reinterpret_cast<uintptr_t>(pDraw.get_first()) : 0;
    // Native case C uses the same HUD singleton as the missing line.
    auto table = Field<uintptr_t>(dispatch, 3);
    auto nativeCase = Field<uintptr_t>(table, 8);
    uintptr_t hud = 0;
    for (size_t i = 0; i < 100; ++i)
        if (Field<uint8_t>(nativeCase, i) == 0xB9 && Field<uint8_t>(nativeCase, i + 5) == 0xE8)
        { hud = Field<uintptr_t>(nativeCase, i + 1); break; }
    if (!draw || !hud) return;
    creditsDraw = draw;
    creditsHUD = hud;
    static auto CreditsHook = safetyhook::create_mid(reinterpret_cast<uintptr_t>(pCreditsCaseCount.get_first()), [](SafetyHookContext& regs)
    {
        if (regs.eax != 7) return;
        using DrawText = int(__thiscall*)(void*, int, float, float, float, float, int, int, int, int);
        reinterpret_cast<DrawText>(creditsDraw)(reinterpret_cast<void*>(creditsHUD), Field<int>(regs.esi, 8),
            fCreditsTextX, Field<float>(regs.esp, 0xC), 20.0f, 30.0f, -1, 2, 1, 0);
        regs.eip = creditsEnd;
    });
}
