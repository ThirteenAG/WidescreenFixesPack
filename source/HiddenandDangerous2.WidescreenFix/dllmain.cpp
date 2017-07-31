#include "stdafx.h"

//#define _LOG
#ifdef _LOG
#include <fstream>
std::ofstream logfile;
uint32_t logit;
#endif // _LOG

bool bSettingsApp;

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fCutOffArea;
    float fFMVScale;
    float fDynamicScreenFieldOfViewScale;
    float fCustomFieldOfView;
} Screen;

struct S_matrix
{
    float a1 = 1.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    uint32_t pad1 = 0;

    float a4 = 0.0f;
    float a5 = 1.0f;
    float a6 = 0.0f;

    uint32_t pad2 = 0;

    float a7 = 0.0f;
    float a8 = 0.0f;
    float a9 = 1.0f;

    float a10 = 0.0f;
    float a11 = 0.0f;
    float a12 = 0.0f;
    float a13 = 0.0f;
    float a14 = 1.0f;
} mat;

void __stdcall S_matrix_Identity(S_matrix *_this)
{
    memcpy((void *)_this, &mat, sizeof(S_matrix));
    _this->a1 = 0.75f;
    _this->a7 = 0.12f;
    _this->a11 = 0.12f;
}

DWORD WINAPI InitSettings(LPVOID)
{
    auto pattern = hook::pattern("0F ? ? ? ? ? 81 ? 80 02 00 00"); //0x4029AB 0x4029AB
    if (!pattern.count_hint(2).empty())
    {
        bSettingsApp = true;
        injector::MakeNOP(pattern.count_hint(2).get(0).get<void*>(0), 6, true);
        injector::MakeNOP(pattern.count_hint(2).get(1).get<void*>(0), 6, true);
    }
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    if (bSettingsApp)
        return 0;

    auto mpattern = hook::module_pattern(GetModuleHandle("LS3DF"), "D8 0D ? ? ? ? D9 82 38 01 00 00");

    if (mpattern.count_hint(2).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (mpattern.clear().count_hint(2).empty()) { Sleep(0); };

    CIniReader iniReader("");
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 0) != 0;
    Screen.fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);

    auto pattern = hook::pattern("68 ? ? ? ? 8B 80 5C"); //0x423E65 0x424077
    static auto ar1 = pattern.count(2).get(0).get<float>(1);
    static auto ar2 = pattern.count(2).get(1).get<float>(1);
    pattern = hook::pattern("C7 45 EC ? ? ? ? FF D6"); //0x6DB137
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.Width = regs.eax;
            Screen.Height = regs.edx;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudScale = (1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            Screen.fDynamicScreenFieldOfViewScale = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio))) * (Screen.fCustomFieldOfView ? Screen.fCustomFieldOfView : 1.0f);

            //aspect ratio
            injector::WriteMemory<float>(ar1, Screen.fAspectRatio, true);
            injector::WriteMemory<float>(ar2, Screen.fAspectRatio, true);
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(7));

    //FOV
    injector::WriteMemory(mpattern.count_hint(2).get(0).get<void*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);
    injector::WriteMemory(mpattern.count_hint(2).get(1).get<void*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);

    //2D
    mpattern = hook::module_pattern(GetModuleHandle("LS3DF"), "78 ? A1 ? ? ? ? C2 04 00");
    static auto dword_1000988F = (uint32_t)hook::module_pattern(GetModuleHandle("LS3DF"), "FF 51 18 89 44 24 0C A1").get_first();
    struct BrokenHudFix
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Width;

            auto callerAddr = *(uint32_t*)(regs.esp) - 3;
            //logfile << std::hex << callerAddr << std::endl;
            if (callerAddr == dword_1000988F)
                regs.eax = Screen.Width43;
        }
    }; injector::MakeInline<BrokenHudFix>(mpattern.count(2).get(0).get<void*>(0), mpattern.count(2).get(0).get<void*>(7));

    if (bFixHUD)
    {
        //breaks everything in the game, no way to align hud properly
        mpattern = hook::module_pattern(GetModuleHandle("LS3DF"), "89 86 34 01 00 00 89 86 30 01 00 00 5E C3");
        struct HudScale
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint32_t*)(regs.esi + 0x134) = 0;
                (*(S_matrix*)(regs.esi + 0x80)).a1 = Screen.fHudScale;
            }
        }; injector::MakeInline<HudScale>(mpattern.get_first(0), mpattern.get_first(6));

        //hud pos
        //injector::MakeCALL((DWORD)GetModuleHandle("LS3DF") + 0x0A268, S_matrix_Identity, true);


        //struct test
        //{
        //    void operator()(injector::reg_pack& regs)
        //    {
        //        float flt_80F524 = 1.0f;
        //        _asm fsubr   ds : flt_80F524
        //        //*(float*)(regs.esi + 0x08) += 110.0f;
        //    }
        //}; injector::MakeInline<test>(0x644B3F, 0x644B3F+6);
    }

    pattern = hook::pattern("56 57 BA 01 00 00 00 B9");
    auto ret_key = []() -> const char* { return "1234-5678-9abc-dddf"; };
    injector::MakeJMP(pattern.get_first(0), static_cast<const char*(*)()>(ret_key), true); //0x731C30
    
    return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        #ifdef _LOG
        logfile.open("HD2.WidescreenFix.log");
        #endif // _LOG

        InitSettings(NULL);
        Init(NULL);
    }
    return TRUE;
}