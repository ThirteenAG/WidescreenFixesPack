#include "stdafx.h"

//#define _LOG
#ifdef _LOG
#include <fstream>
ofstream logfile;
uint32_t logit;
#endif // _LOG

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fDynamicScreenFieldOfViewScale;
    float fHudOffset;
    float fAspectRatio;
    float fHUDScaleX;
    float fTextScaleX;
    int32_t FilmstripScaleX;
    int32_t FilmstripOffset;
} Screen;


DWORD WINAPI InitD3DDrv(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "8B 4B 04 03 C9 85 C9 89 4C 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x38C

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitD3DDrv, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("D3DDrv")).count_hint(1).empty()) { Sleep(0); };

    //Minimap
    struct RenderFilmstrip_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = Screen.FilmstripScaleX + Screen.FilmstripScaleX;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook>(pattern.get_first(0));


    pattern = hook::module_pattern(GetModuleHandle("D3DDrv"), "03 D2 85 D2 89 54 24 10 DB 44 24 10"); //?RenderFilmstrip@UD3DRenderDevice@@UAE_NXZ + 0x350
    struct RenderFilmstrip_Hook2
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = Screen.FilmstripOffset;
            *(int32_t*)(regs.esp + 0x10) = regs.edx;
        }
    }; injector::MakeInline<RenderFilmstrip_Hook2>(pattern.get_first(0), pattern.get_first(6));
    injector::WriteMemory<uint16_t>(pattern.get_first(6), 0xD285, true);     //test    edx, edx

    return 0;
}

DWORD WINAPI InitEngine(LPVOID bDelay)
{
    auto pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 11 15 ? ? ? ? 0F BF"); //10305753

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&InitEngine, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear(GetModuleHandle("Engine")).count_hint(1).empty()) { Sleep(0); };

    auto pWidthScale = *pattern.get_first<float*>(4); //?m_widthScale@ImageUnreal@magma@@0MA
    injector::MakeNOP(pattern.get_first(), 8, true); //?InitDraw@ImageUnreal@magma@@SAXXZ + 0x153
    injector::WriteMemory<float>(pWidthScale, Screen.fHUDScaleX, true);

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5E CA F3 0F 11 4C 24 14 83 C4 04"); //0x1030678D
    struct TextHook
    {
        void operator()(injector::reg_pack& regs)
        {
            auto regs_ecx = regs.ecx;
            _asm
            {
                cvtsi2ss xmm2, regs_ecx
                movss xmm2, Screen.fTextScaleX
                divss xmm1, xmm2
            }
        }
    }; injector::MakeInline<TextHook>(pattern.get_first(-4), pattern.get_first(4));


    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5C 1D ? ? ? ? 0F 28 FD F3 0F 59 FA"); //0x103069A5 + 0x4
    injector::WriteMemory(pattern.get_first(4), &Screen.fHudOffset, true);
    
    pattern = hook::module_pattern(GetModuleHandle("Engine"), "F3 0F 5C D8 F3 0F 58 DC"); //0x1030503C
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            static const float f1_0 = 1.0f;
            _asm
            {
                movss xmm0, Screen.fHudOffset
                subss xmm3, xmm0
                addss xmm3, xmm4
                movss xmm0, f1_0
            }
        }
    }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8));


    //FOV
    #undef SCREEN_FOV_HORIZONTAL
    #undef SCREEN_FOV_VERTICAL
    #define SCREEN_FOV_HORIZONTAL 75.0f
    #define SCREEN_FOV_VERTICAL (2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
    Screen.fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);

    pattern = hook::module_pattern(GetModuleHandle("Engine"), "8B 91 28 06 00 00 52 8B"); //?Draw@UGameEngine@@UAEXPAVUViewport@@HPAEPAH@Z  10530BD7
    struct UGameEngine_Draw_Hook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(float*)&regs.edx = *(float*)(regs.ecx + 0x628) * Screen.fDynamicScreenFieldOfViewScale;
        }
    }; injector::MakeInline<UGameEngine_Draw_Hook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("89 85 D8 61 00 00");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp + 0x61D8) = regs.eax;

            CIniReader iniReader("");
            Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
            Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
            bool bForceLL = iniReader.ReadInteger("MAIN", "ForceLL", 1) != 0;

            if (bForceLL)
            {
                auto pattern = hook::pattern("74 ? 68 ? ? ? ? 53 FF D7");
                injector::MakeNOP(pattern.get_first(), 2, true);
            }

            if (!Screen.Width || !Screen.Height)
                std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHUDScaleX = 2.0f / (600.0f * Screen.fAspectRatio);
            Screen.fTextScaleX = 600.0f * Screen.fAspectRatio;
            Screen.fHudOffset = ((4.0f / 3.0f) / Screen.fAspectRatio);
            Screen.FilmstripScaleX = static_cast<int32_t>(Screen.fWidth / (1280.0f / (368.0 * ((4.0 / 3.0) / (Screen.fAspectRatio)))));
            Screen.FilmstripOffset = static_cast<int32_t>((((Screen.fWidth / 2.0f) - ((Screen.fHeight * (4.0f / 3.0f)) / 2.0f)) * 2.0f) + ((float)Screen.FilmstripScaleX / 5.25f));

            if (Screen.Width > 0 && Screen.Height >> 0)
            {
                char UserIni[MAX_PATH];
                GetModuleFileName(GetModuleHandle(NULL), UserIni, (sizeof(UserIni)));
                *strrchr(UserIni, '\\') = '\0';
                strcat(UserIni, "\\SplinterCell4.ini");

                CIniReader iniWriter(UserIni);
                char ResX[20];
                char ResY[20];
                _snprintf(ResX, 20, "%d", Screen.Width);
                _snprintf(ResY, 20, "%d", Screen.Height);
                iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportX", ResX);
                iniWriter.WriteString("WinDrv.WindowsClient", "WindowedViewportY", ResY);
                iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportX", ResX);
                iniWriter.WriteString("WinDrv.WindowsClient", "FullscreenViewportY", ResY);
            }

            InitEngine(NULL);
            InitD3DDrv(NULL);
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(6));

    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        #ifdef _LOG
        logfile.open("SC4.WidescreenFix.log");
        #endif // _LOG
        Init(NULL);
    }
    return TRUE;
}