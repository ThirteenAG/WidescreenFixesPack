module;

#include <stdafx.h>
#include "common.h"
#include <Zydis.h>

export module Frontend;

import Skeleton;
import Sprite2d;
import Draw;

auto INV_SCREEN_WIDTH = [](float fAspectRatio) { return (1.0f / 640.0f) / (fAspectRatio / (4.0f / 3.0f)); };

export ProtectedGameRef<float> ScaledResXRef;
export ProtectedGameRef<float> ResXInvRef;
export ProtectedGameRef<float> ResYInvRef;

std::array<std::pair<float, float>, 400> vHudScalePtrs;

class Frontend
{
public:
    Frontend()
    {
        WFP::onGameInitEvent() += []()
        {
            CIniReader iniReader("");

            auto bScalingMode = iniReader.ReadInteger("MAIN", "ScalingMode", 1) != 0;
            if (!bScalingMode)
                return;

            static float fHudWidthScale = iniReader.ReadFloat("MAIN", "HudWidthScale", 0.0f);
            static float fHudHeightScale = iniReader.ReadFloat("MAIN", "HudHeightScale", 0.0f);
            static float fRadarWidth = iniReader.ReadFloat("MAIN", "RadarWidth", 94.0f);
            static float fRadarHeight = iniReader.ReadFloat("MAIN", "RadarHeight", 76.0f);
            static float fSubtitlesScale = iniReader.ReadFloat("MAIN", "SubtitlesScale", 0.0f);
            static bool bProportionalWeaponIcon = iniReader.ReadInteger("MAIN", "ProportionalWeaponIcon", 0) != 0;
            g_noBorderAnim = iniReader.ReadInteger("MAIN", "NoCutsceneBorderAnimation", 0) != 0;

            constexpr auto INV_DEFAULT_SCREEN_HEIGHT = 1.0f / (float)DEFAULT_SCREEN_HEIGHT;
            constexpr auto INV_DEFAULT_SCREEN_HEIGHT_MENU = 1.0f / 448.0f;

            auto pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 6A 02 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 6A 02");
            ResXInvRef.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? DB 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 6A 02 E8 ? ? ? ? 6A 00 E8 ? ? ? ? 6A 02");
            ResYInvRef.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? 6A 02 E8 ? ? ? ? 83 C4 08 68 FF 00 00 00 6A 00");
            ScaledResXRef.SetAddress(*pattern.get_first<float*>(2));

            std::vector<uintptr_t> allHudScaleAddrs;
            std::vector<uintptr_t> excludeHudScaleAddrs;
            std::vector<uintptr_t> excludeResXInvAddrs;

            // excludes
            excludeHudScaleAddrs.push_back(0x57A88D + 2); // Slider 1
            excludeHudScaleAddrs.push_back(0x57AA81 + 2); // Slider 2
            excludeHudScaleAddrs.push_back(0x57ACBE + 2); // Slider 3
            excludeHudScaleAddrs.push_back(0x57AEB0 + 2); // Slider 4
            excludeHudScaleAddrs.push_back(0x57B0E4 + 2); // Slider 5

            // excludes for ResXInvRef
            excludeResXInvAddrs.push_back(0x57A1E3 + 2); // Main menu
            excludeResXInvAddrs.push_back(0x57A47D + 2); // Main menu entry sprite

            auto isExcluded = [&](uintptr_t a) -> bool
            {
                return std::find(excludeHudScaleAddrs.begin(), excludeHudScaleAddrs.end(), a) != excludeHudScaleAddrs.end();
            };

            // collect from patterns
            auto ptr = &RsGlobal->maximumWidth;
            pattern = hook::pattern(pattern_str(0xDB, 0x05, to_bytes(ptr)) + "D8 0D"); // fild + fmul
            pattern.for_each_result([&](hook::pattern_match match)
            {
                float* addr = match.get<float>(8);
                allHudScaleAddrs.push_back(reinterpret_cast<uintptr_t>(addr));
            });

            pattern = hook::pattern(pattern_str(0xDB, 0x05, to_bytes(ptr)) + "D8 C9 D8 0D"); // fild + fmul + fmul
            pattern.for_each_result([&](hook::pattern_match match)
            {
                float* addr = match.get<float>(10);
                allHudScaleAddrs.push_back(reinterpret_cast<uintptr_t>(addr));
            });

            // collect extra
            allHudScaleAddrs.push_back(0x58F92D + 2); // Weapon icons
            allHudScaleAddrs.push_back(0x58F9F5 + 2); // Ammo 1
            allHudScaleAddrs.push_back(0x58FA8E + 2); // Ammo 2
            allHudScaleAddrs.push_back(0x57A319 + 2); // Menu radio station text

            size_t index = 0;
            for (uintptr_t rawAddr : allHudScaleAddrs)
            {
                if (isExcluded(rawAddr))
                    continue;

                if (index >= vHudScalePtrs.size())
                    break;

                float** addr = reinterpret_cast<float**>(rawAddr);
                float value = **addr;

                vHudScalePtrs[index] = { value, value };
                injector::WriteMemory(addr, &vHudScalePtrs[index].first, true);
                ++index;
            }

            static float DefaultResXInv = 1.0f / 640.0f;
            for (const auto& addr : excludeResXInvAddrs)
            {
                injector::WriteMemory(addr, &DefaultResXInv, true);
            }

            onResChange() += [](int Width, int Height)
            {
                float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

                ResXInvRef = INV_SCREEN_WIDTH(fAspectRatio);
                ResYInvRef = INV_DEFAULT_SCREEN_HEIGHT_MENU;
                ScaledResXRef = 480.0f * fAspectRatio;

                for (auto& it : vHudScalePtrs)
                {
                    it.first = it.second / (CDraw::GetAspectRatio() / (4.0f / 3.0f));
                }
            };

            // Radar
            static ProtectedGameRef<float> radarWidthRef;
            static ProtectedGameRef<float> radarHeightRef;

            pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 D9 C1 D8 0D ? ? ? ? D8 E9");
            radarWidthRef.SetAddress(*pattern.get_first<float*>(2));

            pattern = hook::pattern("D9 05 ? ? ? ? D8 C9 D9 5C 24 ? D9 05 ? ? ? ? D8 C9 D8 6C 24");
            radarHeightRef.SetAddress(*pattern.get_first<float*>(2));

            radarWidthRef = fRadarWidth;// *(((float)DEFAULT_SCREEN_WIDTH / (float)DEFAULT_SCREEN_HEIGHT) / (640.0f / 448.0f));
            radarHeightRef = fRadarHeight;

            if (fSubtitlesScale)
            {
                fSubtitlesScale = std::clamp(fSubtitlesScale, 0.25f, 4.0f);
                static float SubtitlesWidth = 1.2f;
                static float SubtitlesHeight = 0.58f;

                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 51 DB 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24");
                injector::WriteMemory(pattern.get_first(2), &SubtitlesWidth, true);

                pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 ? DB 05 ? ? ? ? D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A0");
                injector::WriteMemory(pattern.get_first(2), &SubtitlesWidth, true);

                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? DB 05 ? ? ? ? A1");
                injector::WriteMemory(pattern.get_first(2), &SubtitlesHeight, true);

                pattern = hook::pattern("D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? A0");
                injector::WriteMemory(pattern.get_first(2), &SubtitlesHeight, true);

                pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 E8 ? ? ? ? DB 05 ? ? ? ? 83 C4 04");
                injector::WriteMemory(pattern.get_first(2), &SubtitlesHeight, true);

                SubtitlesWidth *= fSubtitlesScale;
                SubtitlesHeight *= fSubtitlesScale;
            }

            // Disable subtitle shift
            static float SubtitlesShift = 20.0f;
            pattern = hook::pattern("D8 0D ? ? ? ? D8 C1 DE EA D9 C9 D8 0D");
            injector::WriteMemory(pattern.get_first(2), &SubtitlesShift, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 ? D9 5C 24 ? D9 05 ? ? ? ? D8 C9 D8 EA D9 C9 DC C0 DE E9 D9 5C 24 ? DD D8 D9 44 24 ? D8 0D ? ? ? ? D8 6C 24 ? D8 64 24 ? D8 64 24 ? D8 0D ? ? ? ? D8 44 24 ? D9 44 24");
            injector::WriteMemory(pattern.get_first(2), &SubtitlesShift, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 C1 DE EA D9 C9 D9 1C 24");
            injector::WriteMemory(pattern.get_first(2), &SubtitlesShift, true);

            pattern = hook::pattern("D8 0D ? ? ? ? D8 44 24 ? D9 5C 24 ? D9 05 ? ? ? ? D8 C9 D8 EA D9 C9 DC C0 DE E9 D9 5C 24 ? DD D8 D9 44 24 ? D8 0D ? ? ? ? D8 6C 24 ? D8 64 24 ? D8 64 24 ? D8 0D ? ? ? ? D8 44 24 ? D9 1C 24");
            injector::WriteMemory(pattern.get_first(2), &SubtitlesShift, true);

            if (fHudWidthScale || fHudHeightScale)
            {
                fHudWidthScale = std::clamp(fHudWidthScale, 0.25f, 4.0f);
                fHudHeightScale = std::clamp(fHudHeightScale, 0.25f, 4.0f);

                uintptr_t m_dwHUDWidth[] =
                {
                    0x58EB3F, // 0 Clock
                    0x58EC0C, // 1 Clock
                    0x58F55C, // 2 Money
                    0x58F5F4, // 3 Money
                    0x5892CA, // 4 Info bars
                    0x58937E, // 5 Info bars
                    0x58EE7E, // 6 Info bars
                    0x58EEF4, // 7 Info bars
                    0x589155, // 8 Info bars
                    0x58EF50, // 9 Info bars
                    0x58EFC5, // 10 Info bars
                    0x58922D, // 11 Info bars
                    0x58F116, // 12 Info bars
                    0x58F194, // 13 Info bars
                    0x58D92D, // 14 Weapon icons
                    0x58D8C3, // 15 Weapon icons
                    0x58F91C, // 16 Weapon icons
                    0x58F92D, // 17 Weapon icons
                    0x5894C5, // 18 Ammo
                    0x5894E9, // 19 Ammo
                    0x58F9D0, // 20 Ammo
                    0x58FA5D, // 21 Ammo
                    0x58F9F5, // 22 Ammo
                    0x58FA8E, // 23 Ammo
                    0x58DCB8, // 24 Wanted
                    0x58DD00, // 25 Wanted
                    0x58DD7E, // 26 Wanted
                    0x58DF71, // 27 Wanted
                    0x58DFE5, // 28 Wanted
                    NULL,     // 29
                    0x58B09F, // 30 Vehicle names
                    0x58B13F, // 31 Vehicle names
                    0x58AD3A, // 32 Area names
                    0x58AD65, // 33 Area names
                    0x58AE4A, // 34 Area names
                    0x58C395, // 35 Subs
                    0x58C41D, // 36 Subs
                    0x58C4DC, // 37 Subs
                    0x5896D8, // 38 Stats box
                    0x589703, // 39 Stats box
                    0x58990C, // 40 Stats box
                    0x58986D, // 41 Stats box
                    0x5897C3, // 42 Stats box
                    0x589A16, // 43 Stats box
                    0x589B2D, // 44 Stats box
                    0x589C73, // 45 Stats box
                    0x589D61, // 46 Stats box
                    0x589E49, // 47 Stats box
                    0x589F31, // 48 Stats box
                    0x58A013, // 49 Stats box
                    0x58A090, // 50 Stats box
                    0x58A134, // 51 Stats box
                    NULL,     // 52
                    NULL,     // 53
                    NULL,     // 54
                    NULL,     // 55
                    NULL,     // 56
                    NULL,     // 57
                    NULL,     // 58
                    NULL,     // 59
                    NULL,     // 60
                    NULL,     // 61
                    0x58C863, // 62 SuccessFailed text
                    0x58D2DB, // 63 MissionTitle text
                    0x58D459, // 64 MissionTitle text
                    0x58CBC1, // 65 WastedBusted text
                    0x58B273, // 66 Timers
                    0x58B2A4, // 67 Timers
                    0x58B3AF, // 68 Timers
                    0x58B3FC, // 69 Timers
                    0x58B56A, // 70 Timers
                    0x58B5EE, // 71 Timers
                    0x58B67E, // 72 Timers
                    0x58B76F, // 73 Helptext
                    0x58B7D6, // 74 Helptext
                    0x58BA62, // 75 Helptext
                    0x58BAC6, // 76 Helptext
                    0x58BBDB, // 77 Helptext
                    0x58BCB0, // 78 Helptext
                    0x58BD58, // 79 Helptext
                    0x58BE8D, // 80 Helptext
                    0x58BF7E, // 81 Helptext
                    0x58BFFC, // 82 Helptext
                    0x580F16, // 83 Menu system
                    0x580F95, // 84
                    0x5810EF, // 85
                    0x581158, // 86
                    0x5811CD, // 87
                    0x58148A, // 88
                    0x5814F7, // 89
                    0x5815B1, // 90
                    0x5815EB, // 91
                    0x581633, // 92
                    0x47AD2A, // 93
                    0x5818CF, // 94
                    0x58CCDB, // 95 OddJob
                    0x58CDE6, // 96 OddJob
                    0x58CEE2, // 97 OddJob
                    0x58D15C, // 98 OddJob
                    0x58A178, // 99 TripSkip
                    0x58A21D, // 100 TripSkip
                    0x58A2C0, // 101 TripSkip
                    0x4E9F30, // 102 RadioStation
                    0x43CF57, // 103 CDarkel
                    0x4477CD, // 104 CGarages
                    0x4477F7, // 105 CGarages
                };

                uintptr_t m_dwHUDHeight[] =
                {
                    0x58EB29, // 0 Clock
                    0x58EBF9, // 1 Clock
                    0x58F546, // 2 Money
                    0x58F5CE, // 3 Money
                    0x589346, // 4 Info bars
                    0x58EE60, // 5 Info bars
                    0x588B9C, // 6 Info bars
                    0x58EEC8, // 7 Info bars
                    0x58913E, // 8 Info bars
                    0x58EF32, // 9 Info bars
                    0x58EF99, // 10 Info bars
                    0x589216, // 11 Info bars
                    0x58F0F8, // 12 Info bars
                    0x58F168, // 13 Info bars
                    0x58D945, // 14 Weapon icons
                    0x58D882, // 15 Weapon icons
                    0x58F90B, // 16 Weapon icons
                    NULL,     // 17
                    0x5894AF, // 18 Ammo
                    NULL,     // 19
                    0x58F9C0, // 20 Ammo
                    0x58FA4A, // 21 Ammo
                    NULL,     // 22
                    NULL,     // 23
                    0x58DCA2, // 24 Wanted
                    0x58DD68, // 25 Wanted
                    0x58DDF4, // 26 Wanted
                    0x58DF55, // 27 Wanted
                    0x58DF9B, // 28 Wanted
                    0x58DEE4, // 29 Wanted
                    0x58B089, // 30 Vehicle names
                    0x58B12D, // 31 Vehicle names
                    0x58AD24, // 32 Area names
                    0x58AE0D, // 33 Area names
                    NULL,     // 34
                    0x58C37F, // 35 Subs
                    0x58C407, // 36 Subs
                    0x58C4C6, // 37 Subs
                    //0x58C53B, // 38 Subs
                    //0x58C611, // 39 Subs
                    0x5898F6, // 40 Stats box text
                    //0x58C46E, // 41 Subs
                    NULL,     // 42
                    0x589735, // 43 Stats box
                    0x58978B, // 44 Stats box
                    0x589813, // 45 Stats box
                    0x58983F, // 46 Stats box
                    0x5898BD, // 47 Stats box
                    0x5899FF, // 48 Stats box
                    0x589A4B, // 49 Stats box
                    0x589B16, // 50 Stats box
                    0x589C5C, // 51 Stats box
                    0x589CA8, // 52 Stats box
                    0x589D4A, // 53 Stats box
                    0x589D92, // 54 Stats box
                    0x589E32, // 55 Stats box
                    0x589E7A, // 56 Stats box
                    0x589F1A, // 57 Stats box
                    0x589F62, // 58 Stats box
                    0x589FFC, // 59 Stats box
                    0x58A040, // 60 Stats box
                    0x58A07A, // 61 Stats box
                    0x58C84D, // 62 SuccessFailed text
                    0x58D2C5, // 63 MissionTitle text
                    NULL, // 0x58D447, // 64 MissionTitle text
                    0x58CBAB, // 65 WastedBusted text
                    NULL, // 0x58B1B7, // 66 Timers
                    0x58B263, // 67 Timers
                    NULL, //0x58B435, // 68 Timers
                    NULL, //0x58B536, // 69 Timers
                    0x58B5DE, // 70 Timers
                    NULL,     // 71
                    NULL,     // 72
                    0x58B7BD, // 73 Help text
                    0x58BA4C, // 74 Help text
                    0x58BBA7, // 75 Help text
                    0x58BD19, // 76 Help text
                    0x58BE2B, // 77 Help text
                    0x58BF1C, // 78 Help text
                    0x58BFCB, // 79 Help text
                    NULL,     // 80
                    NULL,     // 81
                    NULL,     // 82
                    0x580E11, // 83 Menu system
                    0x580F85, // 84
                    0x5810CC, // 85
                    0x581132, // 86
                    0x5811A1, // 87
                    0x58147A, // 88
                    0x5814E7, // 89
                    0x581699, // 90
                    NULL,     // 91
                    NULL,     // 92
                    NULL,     // 93
                    0x581889, // 94
                    0x58CCC5, // 95 OddJob
                    0x58CDD0, // 96 OddJob
                    0x58CECC, // 97 OddJob
                    0x58D146, // 98 OddJob
                    NULL, //0x58A199, // 99 TripSkip
                    NULL, //0x58A207, // 100 TripSkip
                    NULL, //0x58A2B0, // 101 TripSkip
                    0x4E9F1A, // 102 RadioStation
                    0x43CF47, // 103 CDarkel
                    0x4477B7, // 104 CGarages
                    0x4478AC, // 105 CGarages
                };

                static float fHUDWidth = 1.0f / 640.0f;
                static float fHUDHeight = 1.0f / 448.0f;

                for (int i = 0; i < sizeof(m_dwHUDWidth) / sizeof(const void*); i++)
                {
                    if (m_dwHUDWidth[i] != NULL)
                        injector::WriteMemory<const void*>(m_dwHUDWidth[i] + 0x2, &fHUDWidth, true);
                }

                for (int i = 0; i < sizeof(m_dwHUDHeight) / sizeof(const void*); i++)
                {
                    if (m_dwHUDHeight[i] != NULL)
                        injector::WriteMemory<const void*>(m_dwHUDHeight[i] + 0x2, &fHUDHeight, true);
                }

                static float fWeaponIconScaleX = 0.17343046f;
                injector::WriteMemory<const void*>(0x58F92D + 2, &fWeaponIconScaleX, true); // Weapon icons
                injector::WriteMemory<const void*>(0x58F9F5 + 2, &fWeaponIconScaleX, true); // Ammo 1
                injector::WriteMemory<const void*>(0x58FA8E + 2, &fWeaponIconScaleX, true); // Ammo 2

                if (bProportionalWeaponIcon)
                {
                    static float fWeaponIconScaleY = 1.0f / 640.0f;
                    injector::WriteMemory<const void*>(m_dwHUDHeight[14] + 0x2, &fWeaponIconScaleY, true);
                    injector::WriteMemory<const void*>(m_dwHUDHeight[15] + 0x2, &fWeaponIconScaleY, true);
                    injector::WriteMemory<const void*>(m_dwHUDHeight[16] + 0x2, &fWeaponIconScaleY, true);
                }

                onResChange() += [](int Width, int Height)
                {
                    float fAspectRatio = static_cast<float>(Width) / static_cast<float>(Height);

                    fHUDWidth = INV_SCREEN_WIDTH(fAspectRatio);
                    fHUDHeight = INV_DEFAULT_SCREEN_HEIGHT_MENU;

                    fHUDWidth *= fHudWidthScale;
                    fHUDHeight *= fHudHeightScale;

                    fWeaponIconScaleX = 0.17343046f / (CDraw::GetAspectRatio() / (4.0f / 3.0f));
                    fWeaponIconScaleX *= fHudWidthScale;
                };
            }
        };
    }
} Frontend;