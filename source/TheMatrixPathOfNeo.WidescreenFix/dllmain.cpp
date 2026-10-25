#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nAspectRatioDiff;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

const char* movies[] =
{
    "B10_Trns_B11_27",
    "B11_Trns_B12_028",
    "B12_Trns_B125_29",
    "B12_Trns_B13_30",
    "b13_end_033",
    "B13_Smithzilla_Con",
    "B13_Smithzilla_Des",
    "B1_Trns_B2_002",
    "B2_RM_003S",
    "B2_Trns_B3_005",
    "B2_Trns_RM_004",
    "B3_DO_006S",
    "B3_Trns_B4_012",
    "B4_LH_013S",
    "B4_Trns_B5_014",
    "B5_LO_016",
    "B5_Trns_B6_018",
    "B65_Trns_B7_021",
    "B6_Trns_B65_020s",
    "B6_TS_019S",
    "B7_022s",
    "B7_Trns_B8_023",
    "B8_Trns_B9_024",
    "B9_Trns_B10_025",
    "c_hand",
    "C_Staff",
    "C_Sword",
    "Extra_B3Bamboo_Finish",
    "Extra_B3_Gun_Intro",
    "Extra_B5_Rooftop_Showdown",
    "Extra_B6_SS_WallSmack",
    "Extra_B8AiTutSuperJump",
    "Extra_BLUR_makingof",
    "Extra_OUTTAKES",
    "pon_trailer",
    "PowerBubble",
    "serpahfight_256",
    "start_screen_512",
    "WB_Green"
};

const char* misc[] =
{
    "film_scratch",
    "oldfilm_noise04"
    //"Hud_Generic_box",
    //"Black_888"
    //"HelveticaNeue_BoldCond"
};

injector::hook_back<void(__cdecl*)(void* a1, void* a2, void* a3, void* a4, float a5, float a6, float a7, float a8, void* a9, float a10, float a11, float a12)> hb_538E80;
void __cdecl sub_538E80(void* a1, void* a2, void* a3, void* a4, float a5, float a6, float a7, float a8, void* a9, float a10, float a11, float a12)
{
    if (!a1 || !a2 || !a3 || !a4 || !a9 || Screen.fHudScale <= 0.0f || Screen.fWidth <= 0.0f)
        return hb_538E80.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);

    uint32_t text;
    std::memcpy(&text, static_cast<char*>(a9) + 4, sizeof(text));

    if (*(float*)a1 == 0.0f && *(float*)a2 == 1.0f && *(float*)a3 == 0.0f && *(float*)a4 == 1.0f && (a12 == 2.0f || a12 == 4.0f)) //fullscreen drawings
    {
        bool bFound = false;

        for (auto var : movies)
        {
            uint32_t prefix;
            std::memcpy(&prefix, var, sizeof(prefix));
            if (prefix == text)
            {
                bFound = true;
                break;
            }
        }

        if (!bFound)
            return hb_538E80.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    }

    uint32_t prefixes[2];
    std::memcpy(&prefixes[0], misc[0], sizeof(uint32_t));
    std::memcpy(&prefixes[1], misc[1], sizeof(uint32_t));
    if (prefixes[0] != text && prefixes[1] != text)
    {
        *static_cast<float*>(a1) = *static_cast<float*>(a1) / Screen.fHudScale + Screen.fHudOffsetReal / Screen.fWidth;
        *static_cast<float*>(a2) = *static_cast<float*>(a2) / Screen.fHudScale + Screen.fHudOffsetReal / Screen.fWidth;
    }

    return hb_538E80.fun(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
}

void Init()
{
    hook::pattern pattern;
    auto& settings = FixSettings();
    Screen.nWidth = settings.Int("MAIN.ResX");
    Screen.nHeight = settings.Int("MAIN.ResY");

    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 32767 || Screen.nHeight > 32767)
    {
        Screen.nWidth = 640;
        Screen.nHeight = 480;
    }

    if (wfp::PatchPreflight("Path of Neo resolution and aspect: signatures unavailable; feature disabled.\n",
    {
        { "68 ? ? ? ? 6A 01 68 ? ? ? ? 68 ? ? ? ? FF D6", 3 },
        { "C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 01 00 00 00", 2 },
        { "C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00", 1 },
        { "A3 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 6A 01", 1 },
        { "05 94 00 00 00 D9 5C 24 08", 1 },
    }))
    {
        auto pattern1 = hook::pattern("68 ? ? ? ? 6A 01 68 ? ? ? ? 68 ? ? ? ? FF D6");
        auto pattern2 = hook::pattern("C6 05 ? ? ? ? 01 C7 05 ? ? ? ? 01 00 00 00");
        static auto dword_6CACDC = *hook::get_pattern<int32_t*>("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00", 2);
        static auto dword_6CACE0 = *hook::get_pattern<int32_t*>("C7 05 ? ? ? ? 00 04 00 00 C7 05 ? ? ? ? 00 03 00 00", 12);
        struct ResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_6CACDC = Screen.nWidth;
                *dword_6CACE0 = Screen.nHeight;

                Screen.fWidth = static_cast<float>(Screen.nWidth);
                Screen.fHeight = static_cast<float>(Screen.nHeight);
                Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
                Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
                Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
                Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
                Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
                Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
                Screen.nAspectRatioDiff = static_cast<int32_t>(Screen.fWidth / (((4.0f / 3.0f)) / (Screen.fAspectRatio)));
            }
        }; injector::MakeInline<ResHook>(pattern1.count_hint(3).get(2).get<void>(0), pattern2.count_hint(2).get(1).get<void>(0)); //0x40DD33, 0x40DE21

        //forcing 4:3 aspect, maybe better to hook sub_4BC320
        pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 85 C0 0F 84 ? ? ? ? 6A 01");
        injector::MakeNOP(pattern.get_first(0), 5, true);
        ProtectedGameRef<uint32_t>{pattern2.count_hint(2).get(1).get<uint32_t>(13)} = 0;
        ProtectedGameRef<uint32_t>{reinterpret_cast<uint32_t*>(*pattern2.count_hint(2).get(1).get<uint32_t>(9))} = 0;

        pattern = hook::pattern("05 94 00 00 00 D9 5C 24 08");
        struct AspectHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax += 0x94;
                *reinterpret_cast<int32_t*>(regs.eax - 0x4C) = Screen.nAspectRatioDiff;
            }
        }; injector::MakeInline<AspectHook>(pattern.get_first(0)); //0x41F45A

    }

    if (settings.Bool("MAIN.FixHud"))
    {
        if (wfp::PatchPreflight("Path of Neo HUD: signatures unavailable; feature disabled.\n",
        {
            { "E8 ? ? ? ? 83 C4 30 5F 5E 5B 8B E5 5D C3", 1 },
        }))
        {
            pattern = hook::pattern("E8 ? ? ? ? 83 C4 30 5F 5E 5B 8B E5 5D C3"); //0x57A783
            hb_538E80.fun = injector::MakeCALL(pattern.get_first(0), sub_538E80, true).get();
        }


    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        WindowedModeWrapper::EnableDpiAwareness();
        FixSettings();
        asi_settings::LiveLink::Start();
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("33 84 24 08 04 00 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        if (!IsUALPresent()) { InitializeASI(); }
    }
    return TRUE;
}