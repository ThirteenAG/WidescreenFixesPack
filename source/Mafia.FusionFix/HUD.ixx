module;

#include <stdafx.h>
#include <cmath>

export module HUD;

import ComVars;

export void InitHUD()
{
    auto pOnePCurBaseWidth = find_pattern("D8 0D ? ? ? ? D9 5C 24 18 8B 4C 24 18 89 8D C0 90 00 00",
        "D8 0D ? ? ? ? D9 5C 24 20 8B 4C 24 20 89 8E C0 40 00 00 A1 ? ? ? ? 50 8B 08 FF 51 74 89 44 24 20 A1 ? ? ? ? DB 44 24 20 8B 10 50 D8 0D ? ? ? ? D9 5C 24 14 FF 52 70 89 44 24 20 DB 44 24 20 D8 0D ? ? ? ? D9 5C 24 20 E9 ? ? ? ?");
    auto pOnePCurBaseWidth2 = find_pattern("D8 0D ? ? ? ? 8B 44 24 10 8D 9D E4 91 00 00",
        "D8 0D ? ? ? ? D9 5C 24 20 E9 ? ? ? ? FF 51 74");
    auto pHudSpeedometerX = find_pattern("D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 44 24 20 D8 E2 89 44 24 1C",
        "D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 05 ? ? ? ? D8 E2 D9 5C 24 20 D9 05 ? ? ? ? D8 E1 8B 44 24 20");
    auto pHudTachometerX = find_pattern("D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 44 24 20 D8 E2 D9 5C 24 18",
        "D8 0D ? ? ? ? D9 43 04 D8 0D ? ? ? ? D9 05 ? ? ? ? D8 E2 D9 5C 24 20 D9 05 ? ? ? ? D8 E1 8B 54 24 20");
    auto pHudSpeedLimiterX = find_pattern("C7 44 24 30 00 00 3E 44 C7 44 24 34 00 00 BE 43",
        "C7 44 24 30 00 00 20 C2 C7 44 24 34 00 00 5C C3");
    auto pHudTimerX = find_pattern("C7 44 24 18 00 80 2D 44 8D 4C 24 28 53 51 8D 4C 24 20",
        "C7 44 24 20 00 00 D4 C2 C7 44 24 24 00 00 A0 41");
    auto pHudCompasX = find_pattern("D8 0D ? ? ? ? 83 CE FF C7 84 24 84 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? D9 85 C4 40 00 00 D8 0D ? ? ? ? D9 C9 D8 05 ? ? ? ? 83 CE FF");
    auto pHudMinimapX = find_pattern("D8 0D ? ? ? ? D9 54 24 18 D9 85 C4 90 00 00",
        "D8 0D ? ? ? ? D9 85 C4 40 00 00 D8 0D ? ? ? ? D9 C9 D8 05 ? ? ? ? D9 5C 24 20");
    auto pHudTransmissionX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? 8A 85 FC 91 00 00",
        "D8 0D ? ? ? ? 8A 85 04 42 00 00 C7 84 24 E8 00 00 00 00 00 80 3F");
    auto pHudFuelLedX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? C7 84 24 84 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? C7 84 24 94 00 00 00 00 00 00 00 C7 84 24 98 00 00 00 00 00 80 3F 89 9C 24 9C 00 00 00");
    auto pHudActionBarX = find_pattern("D8 0D ? ? ? ? 8D 0C 76 89 7C 24 44 C1 E0 18",
        "D8 0D ? ? ? ? 8D 0C 76 89 7C 24 40 C1 E0 18");
    auto pHudHealthBarX = find_pattern("D8 0D ? ? ? ? 2B DA BA 00 00 80 3F 89 5C 24 10");
    auto pHudHealthBarTextX = find_pattern("D8 0D ? ? ? ? 8D 04 90 BA 3C 02 00 00 03 C0",
        "D8 0D ? ? ? ? 8D 14 40 57 33 FF D8 05 ? ? ? ?");
    auto pHudAmmoBarX = find_pattern("D8 0D ? ? ? ? D9 81 C4 90 00 00 D8 0D ? ? ? ? 8B 54 24 08",
        "D8 0D ? ? ? ? 8B 54 24 08 8B 44 24 04 55 56");
    auto pHudAmmoBarTextX = find_pattern("D8 0D ? ? ? ? D9 81 C4 90 00 00 D8 0D ? ? ? ? 53",
        "D8 0D ? ? ? ? 53 55 56 8B 74 24 24 D8 05 ? ? ? ?");
    auto pHudSearchBarX = find_pattern("D8 0D ? ? ? ? 6A 01 C7 84 24 88 00 00 00 00 00 00 00",
        "D8 0D ? ? ? ? 6A 01 C7 84 24 98 00 00 00 00 00 00 00");
    auto pHudProgressBarX = find_pattern("F6 C4 10 0F 84 ? ? ? ? D9 85 C0 90 00 00 D8 0D ? ? ? ?",
        "F6 C4 10 0F 84 ? ? ? ? D9 85 C0 40 00 00 D8 0D ? ? ? ?");
    auto pHudProgressBar2X = find_pattern("F7 85 A4 90 00 00 00 00 04 00 0F 84 ? ? ? ?",
        "F7 85 A4 40 00 00 00 00 04 00 0F 84 ? ? ? ?");
    auto pHudLeftPopupTextX = find_pattern("D8 0D ? ? ? ? 68 00 00 20 41 50 51 8D 8C 2E 74 92 00 00",
        "D8 0D ? ? ? ? 68 00 00 20 41 50 51 8D 8C 2E 7C 42 00 00");
    auto pHudMoneyRightTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 8B 9D A4 90 00 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 8B 9D A4 40 00 00");
    auto pHudRace321GoX = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? D9 5C 24 44 D9 44 24 4C DC 0D ? ? ? ? DE E9 74 ? D9 05 ? ? ? ? EB ? D9 44 24 10",
        "68 00 00 C8 43 8D 8C 24 90 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 50");
    auto pHudRace321GoX2 = find_pattern("D8 0D ? ? ? ? D9 85 C4 90 00 00 D8 0D ? ? ? ? D9 5C 24 44 D9 44 24 4C DC 0D ? ? ? ? DE E9 74 ? D9 05 ? ? ? ? EB ? D9 05 ? ? ? ?",
        "68 00 00 C8 43 8D 8C 24 90 00 00 00 E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8D 4C 24 48");
    auto pHudRaceWrongWayX = find_pattern("68 00 00 88 43 8D 8C 24 80 00 00 00 E8 ? ? ? ?",
        "68 00 00 88 43 8D 8C 24 90 00 00 00 E8 ? ? ? ?");
    auto pHudRaceCheckpointX = find_pattern("C7 44 24 48 00 00 C3 43 C7 44 24 4C 00 00 16 43",
        "68 00 00 C3 43 8D 8C 24 90 00 00 00 E8 ? ? ? ?");
    auto pHudRaceRightTextX = find_pattern("D8 0D ? ? ? ? 6A 00 6A 01 6A 04 6A FF 8D 95 CC 94 00 00",
        "D8 0D ? ? ? ? 6A 00 6A 01 6A 04 6A FF D8 05 ? ? ? ?");
    auto pHudMapTextX = find_pattern("D8 0D ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 8A 85 A4 90 00 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 8A 85 A4 40 00 00");
    auto pHudBlackCentredTextX = find_pattern("D8 2D ? ? ? ? F7 EE D9 5C 24 14 D9 44 24 1C");
    auto pHudCreditsText2X = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 5E",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 5E");
    auto pHudCreditsText2X2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 83 EC 10 83 C6 0C D9 5C 24 0C D9 05 ? ? ? ?");
    auto pHudCreditsText2X3 = find_pattern("D8 0D ? ? ? ? E9 ? ? ? ? D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 01");
    auto pHudCreditsText1X = find_pattern("D8 0D ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 D9 05 ? ? ? ?");
    auto pHudCreditsText3X = find_pattern("D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02",
        "D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 D9 5C 24 18 D9 05 ? ? ? ? D8 0D ? ? ? ? 8B 4C 24 18 D9 1C 24 D9 05 ? ? ? ?",
        "D8 0D ? ? ? ? D9 5C 24 08 D9 05 ? ? ? ? D8 4C 24 2C D9 5C 24 04 D9 05 ? ? ? ? D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 02");
    auto pHudCreditsText3X2 = find_pattern("D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 83 C6 4C",
        "D8 0D ? ? ? ? 6A 00 6A 00 6A 02 6A FF 51 D9 5C 24 18");
    auto pHudCreditsText2X4 = find_pattern("D8 0D ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 5E",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 52 E8 ? ? ? ? DD D8 5E");
    auto pHudCreditsText2X5 = find_pattern("68 00 00 C3 43 56 B9 ? ? ? ? E8 ? ? ? ?");
    auto pHudCreditsText1X2 = find_pattern("D8 0D ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 51 B9 ? ? ? ? E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? D9 1C 24 50 E8 ? ? ? ? DD D8 D9 05 ? ? ? ? D8 0D ? ? ? ? 6A 00 6A 00 6A 00");
    auto pHudCreditsText4X = find_pattern("D8 0D ? ? ? ? D9 1C 24 56 B9 ? ? ? ? E8 ? ? ? ?",
        "D8 0D ? ? ? ? D8 05 ? ? ? ? B9 ? ? ? ?");
    auto pHudLiveBarX = find_pattern("D8 0D ? ? ? ? C7 84 24 84 00 00 00 00 00 00 00 C7 84 24 88 00 00 00 00 00 80 3F 89 9C 24 8C 00 00 00 C7 84 24 94 00 00 00 00 00 00 00 C7 84 24 98 00 00 00 00 00 40 3E");
    auto pHudCreditsText2X6 = find_pattern("68 00 00 C3 43 52 B9 ? ? ? ? E8 ? ? ? ?");

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

    injector::WriteMemory(pOnePCurBaseWidth.get_first(0x2), &Screen.fInvBaseWidth, true);
    injector::WriteMemory(pOnePCurBaseWidth2.get_first(0x2), &Screen.fInvBaseWidth, true);

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
        fixHudLeft = 0.0f;
        fixHudCenter = (Screen.fBaseWidth - 800.0f) / 2.0f;
        fixHudRight = Screen.fBaseWidth - 800.0f;
    }

    static float hudSpeedometerX = 189.0f + fixHudLeft;
    injector::WriteMemory(pHudSpeedometerX.get_first(0x2), &hudSpeedometerX, true);
    static float hudTachometerX = 259.0f + fixHudLeft;
    injector::WriteMemory(pHudTachometerX.get_first(0x2), &hudTachometerX, true);
    float hudSpeedLimiterX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudSpeedLimiterX = 760.0f + fixHudRight;
    }
    else
    {
        hudSpeedLimiterX = -40.0f - fixHudLeft;
    }
    injector::WriteMemory(pHudSpeedLimiterX.get_first(0x4), hudSpeedLimiterX, true);
    float hudTimerX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudTimerX = 694.0f + fixHudRight;
    }
    else
    {
        hudTimerX = -106.0f - fixHudLeft;
    }
    injector::WriteMemory(pHudTimerX.get_first(0x4), hudTimerX, true);
    static float hudCompasX = 0.0f + fixHudLeft;
    injector::WriteMemory(pHudCompasX.get_first(0x2), &hudCompasX, true);
    static float hudMinimapX = 29.0f + fixHudLeft;
    injector::WriteMemory(pHudMinimapX.get_first(0x2), &hudMinimapX, true);
    static float hudTransmissionX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudTransmissionX = 758.0f + fixHudRight;
    }
    else
    {
        hudTransmissionX = 42.0f + fixHudLeft;
    }
    injector::WriteMemory(pHudTransmissionX.get_first(0x2), &hudTransmissionX, true);
    static float hudFuelLedX;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        hudFuelLedX = 668.0f + fixHudRight;
    }
    else
    {
        hudFuelLedX = 132.0f + fixHudLeft;
    }
    injector::WriteMemory(pHudFuelLedX.get_first(0x2), &hudFuelLedX, true);
    static float hudActionBarX = 9.0f + fixHudLeft;
    injector::WriteMemory(pHudActionBarX.get_first(0x2), &hudActionBarX, true);
    static float hudHealthBarX = 7.0f + fixHudLeft;
    injector::WriteMemory(pHudHealthBarX.get_first(0x2), &hudHealthBarX, true);
    static float hudHealthBarTextX = 42.0f + fixHudLeft;
    injector::WriteMemory(pHudHealthBarTextX.get_first(0x2), &hudHealthBarTextX, true);
    static float hudAmmoBarX = 62.0f + fixHudLeft;
    injector::WriteMemory(pHudAmmoBarX.get_first(0x2), &hudAmmoBarX, true);
    static float hudAmmoBarTextX = 112.0f + fixHudLeft;
    injector::WriteMemory(pHudAmmoBarTextX.get_first(0x2), &hudAmmoBarTextX, true);
    static float hudSearchBarX = 336.0f + fixHudCenter;
    injector::WriteMemory(pHudSearchBarX.get_first(0x2), &hudSearchBarX, true);
    static float hudLiveBarX = 368.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        injector::WriteMemory(pHudLiveBarX.get_first(0x2), &hudLiveBarX, true);
    }
    static float hudProgressBarX = 250.0f + fixHudCenter;
    injector::WriteMemory(pHudProgressBarX.get_first(0x11), &hudProgressBarX, true);
    static float hudProgressBar2X = 350.0f + fixHudCenter;
    injector::WriteMemory(pHudProgressBar2X.get_first(0x18), &hudProgressBar2X, true);
    static float hudLeftPopupTextX = 9.0f + fixHudLeft;
    injector::WriteMemory(pHudLeftPopupTextX.get_first(0x2), &hudLeftPopupTextX, true);
    static float hudMoneyRightTextX = 760.0f + fixHudRight;
    injector::WriteMemory(pHudMoneyRightTextX.get_first(0x2), &hudMoneyRightTextX, true);

    static float hudRace321goX = 400.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        injector::WriteMemory(pHudRace321GoX.get_first(0x2), &hudRace321goX, true);
        injector::WriteMemory(pHudRace321GoX2.get_first(0x2), &hudRace321goX, true);
    }
    else
    {
        injector::WriteMemory(pHudRace321GoX.get_first(0x1), hudRace321goX, true);
        injector::WriteMemory(pHudRace321GoX2.get_first(0x1), hudRace321goX, true);
    }
    float hudRaceWrongWayX = 272.0f + fixHudCenter;
    injector::WriteMemory(pHudRaceWrongWayX.get_first(0x1), hudRaceWrongWayX, true);
    float hudRaceCheckpointX = 390.0f + fixHudCenter;
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        injector::WriteMemory(pHudRaceCheckpointX.get_first(0x4), hudRaceCheckpointX, true);
    }
    else
    {
        injector::WriteMemory(pHudRaceCheckpointX.get_first(0x1), hudRaceCheckpointX, true);
    }
    static float hudRaceRightTextX = 780.0f + fixHudRight;
    injector::WriteMemory(pHudRaceRightTextX.get_first(0x2), &hudRaceRightTextX, true);

    static float hudMapTextX = 350.0f + fixHudCenter;
    injector::WriteMemory(pHudMapTextX.get_first(0x2), &hudMapTextX, true);

    static float hudBlackCentredTextX = 400.0f + fixHudCenter;
    injector::WriteMemory(pHudBlackCentredTextX.get_first(0x2), &hudBlackCentredTextX, true);
    static float hudCreditsText1X = 370.0f + fixHudCenter;
    fCreditsTextX = 390.0f + fixHudCenter;
    static float hudCreditsText3X = 400.0f + fixHudCenter;
    static float hudCreditsText4X = 410.0f + fixHudCenter;
    injector::WriteMemory(pHudCreditsText2X.get_first(0x2), &fCreditsTextX, true);
    injector::WriteMemory(pHudCreditsText2X2.get_first(0x2), &fCreditsTextX, true);
    injector::WriteMemory(pHudCreditsText2X3.get_first(0x2), &fCreditsTextX, true);
    injector::WriteMemory(pHudCreditsText1X.get_first(0x2), &hudCreditsText1X, true);
    injector::WriteMemory(pHudCreditsText3X.get_first(0x2), &hudCreditsText3X, true);
    injector::WriteMemory(pHudCreditsText3X2.get_first(0x2), &hudCreditsText3X, true);
    injector::WriteMemory(pHudCreditsText2X4.get_first(0x2), &fCreditsTextX, true);
    injector::WriteMemory(pHudCreditsText2X5.get_first(0x1), fCreditsTextX, true);
    if (nGameVersion == MAFIA_1_2_ENG)
    {
        injector::WriteMemory(pHudCreditsText2X6.get_first(0x1), fCreditsTextX, true);
    }
    injector::WriteMemory(pHudCreditsText1X2.get_first(0x2), &hudCreditsText1X, true);
    injector::WriteMemory(pHudCreditsText4X.get_first(0x2), &hudCreditsText4X, true);

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
    injector::WriteMemory(pMapLeftX.get_first(0x2), &mapLeftX, true);
    static float mapRightX = 1.0f - mapLeftX;
    injector::WriteMemory(pMapRightX.get_first(0x2), &mapRightX, true);
    fMapPlayerWidth = Screen.fHeight * (4.0f / 3.0f);
    if (nGameVersion == MAFIA_1_0_ENG)
    {
        // Replace FMUL [ebp+HUD.width] with FMUL [fMapPlayerWidth]. Both
        // encodings occupy six bytes; the following native FXCH stays intact.
        injector::WriteMemory(pMapPlayer.get_first(), uint16_t{0x0DD8}, true);
        injector::WriteMemory(pMapPlayer.get_first(0x2), &fMapPlayerWidth, true);
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
    injector::WriteMemory(pMapWidth.get_first(0x2), &mapWidth, true);

    float mapAspect = (mapWidth / (((800.0f * 0.9f + fixMapLeft) / Screen.fBaseWidth) - mapLeftX));
    static float mapScaleX = 0.4f * mapAspect;
    injector::WriteMemory(pMapScaleX.get_first(0x2), &mapScaleX, true);
    injector::WriteMemory(pConstant10FMapScaleX.get_first(0x4), 1.0f - mapScaleX, true);
    injector::WriteMemory(pMapScaleX2.get_first(0x2), &mapScaleX, true);
    static float mapPosX = 0.2f * mapAspect;
    injector::WriteMemory(pMapPosX.get_first(0x2), &mapPosX, true);
    injector::WriteMemory(pMapPosX2.get_first(0x2), &mapPosX, true);
    injector::WriteMemory(pMapPosX3.get_first(0x2), &mapPosX, true);
    static float mapTargetPosX = 2.5f / mapAspect;
    injector::WriteMemory(pMapTargetPosX.get_first(0x2), &mapTargetPosX, true);
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

    injector::WriteMemory(pTextWidth.get_first(0x2), &textWidth, true);
    injector::WriteMemory(pTextWidth2.get_first(0x2), &textWidth, true);
    injector::WriteMemory(pTextWidth3.get_first(0x2), &textWidth, true);
    injector::WriteMemory(pTextWidth4.get_first(0x2), &textWidth, true);
    injector::WriteMemory(pTextWidth5.get_first(0x2), &textWidth, true);
    injector::WriteMemory(pTextWidth6.get_first(0x2), &textWidth, true);
}

export void InitOthers()
{
    auto pFrameDelay = find_pattern("6A 10 8B 08 50 FF 91 F0 00 00 00 33 ED 89 44 24 10",
        "6A 10 8B 08 50 FF 91 00 01 00 00 33 ED 89 44 24 10");
    auto pCreditsCaseCount = find_pattern("83 F8 06 0F 87 ? ? ? ? FF 24 85 ? ? ? ? D9 05 ? ? ? ?");

    if (nFPSLimit > 0 && pFrameDelay.size() == 1)
        injector::WriteMemory(pFrameDelay.get_first(0x1), static_cast<uint8_t>(std::clamp(1000 / nFPSLimit, 0, 127)), true);
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
