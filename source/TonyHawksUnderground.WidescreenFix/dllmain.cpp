#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    float fAspectRatioDiff;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("6A 01 6A 01 ? 8B 15");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        LoadLibrary("TonyHawksUnderground.SafeDiscCheck.dll");
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fAspectRatioDiff = 1.0f / (Screen.fAspectRatio / (4.0f / 3.0f));
    Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    //addresses is not based on drm free exe
    //Resolution
    static int32_t* dword_6D3608 = *pattern.get_first<int32_t*>(7);
    static int32_t* dword_6D360C = *pattern.get_first<int32_t*>(-4);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6D3608 = Screen.Width;
            *dword_6D360C = Screen.Height;
        }
    };
    pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D3608)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0));

    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D360C)));
    for (size_t i = 0; i < pattern.size(); ++i)
        injector::MakeInline<SetResHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
    
    pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_6D3608)));
    if (!pattern.count_hint(1).empty())
        injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern(pattern_str(0xA3, to_bytes(dword_6D360C)));
    if (!pattern.count_hint(1).empty())
        injector::MakeInline<SetResHook>(pattern.get_first(0));

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? E8 ? ? ? ? D9 5C 24 08");
    if (!pattern.count_hint(1).empty())
    {
        injector::WriteMemory<float>(pattern.get_first(1), Screen.fAspectRatio, true);
    }
    else
    {
        pattern = hook::pattern("D9 05 ? ? ? ? 51 D9 1C 24 E8 ? ? ? ? 6A 00");
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fAspectRatio, true);
    }

    //FOV
    pattern = hook::pattern("? 0D ? ? ? ? D9 9E ? ? ? ? 5E 59 C3");
    if (*pattern.get_first<uint8_t>(0) == (uint8_t)0xD8) //fmul flt
    {
        #undef SCREEN_FOV_HORIZONTAL
        #undef SCREEN_FOV_VERTICAL
        #define SCREEN_FOV_HORIZONTAL 114.59155f
        #define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
        float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
        injector::WriteMemory<float>(*pattern.get_first<float*>(2), SCREEN_FOV_HORIZONTAL * fDynamicScreenFieldOfViewScale, true);
    }
    else //fmul dbl
    {
        #undef SCREEN_FOV_HORIZONTAL
        #undef SCREEN_FOV_VERTICAL
        #define SCREEN_FOV_HORIZONTAL 57.29577791868205f
        #define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
        float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
        static double dbl_62D630 = SCREEN_FOV_HORIZONTAL * fDynamicScreenFieldOfViewScale;
        injector::WriteMemory(pattern.get_first(2), &dbl_62D630, true);
    }

    //HUD
    if (bFixHUD)
    {
        static int32_t nHudOffset = static_cast<int32_t>(Screen.fHudOffsetReal);
        static int32_t* dword_72DFC4;
        struct SetOffsetHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *dword_72DFC4 = nHudOffset;
            }
        };

        pattern = hook::pattern("D8 0D ? ? ? ? 03 C2 C1 F8 02");
        if (!pattern.count_hint(1).empty())
        {
            injector::WriteMemory<float>(*pattern.get_first<float*>(2), Screen.fHUDScaleX, true);
            dword_72DFC4 = *hook::get_pattern<int32_t*>("DA 05 ? ? ? ? 0F BF 88", 2);
        }
        else
        {
            Screen.fHUDScaleX = 1.0f / Screen.fHUDScaleX;
            pattern = hook::pattern("DC 0D ? ? ? ? DE FB D9");
            injector::WriteMemory<double>(*pattern.get_first<double*>(2), Screen.fHUDScaleX, true);

            dword_72DFC4 = *hook::get_pattern<int32_t*>("DA 05 ? ? ? ? 89 4C 24 20 89 54 24 18", 2);
        }

        pattern = hook::pattern(pattern_str(0x89, '?', to_bytes(dword_72DFC4)));
        for (size_t i = 0; i < pattern.size(); ++i)
            injector::MakeInline<SetOffsetHook>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(6));
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}