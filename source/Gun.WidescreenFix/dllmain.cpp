#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fHudScaleX;
    float fHudOffset;
    int Width43;
    float* DefaultAR;
} Screen;

float SetAspect()
{
    return Screen.fAspectRatio * ((*Screen.DefaultAR) / (4.0f / 3.0f));
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("6A 60 68 ? ? ? ? E8 ? ? ? ? BF 94 00 00 00");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    uint32_t nGameSpeed = iniReader.ReadInteger("MISC", "GameSpeed", 30);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio)));
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));

    pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 84 C0 74 38 8B 0D");
    static auto dword_6B75A0_Y = *pattern.count(1).get(0).get<uint32_t*>(1);
    static auto dword_6B759C_X = dword_6B75A0_Y - 1;
    struct ResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6B759C_X = Screen.Width;
            *dword_6B75A0_Y = Screen.Height;
        }
    }; injector::MakeInline<ResHook1>(pattern.get_first());

    pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 6A 00 68 ? ? ? ? 8D 4C 24 14");
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_6B759C_X = Screen.Width;
            *dword_6B75A0_Y = Screen.Height;
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first());
    
    auto pSetAR = injector::GetBranchDestination(hook::pattern("E8 ? ? ? ? D8 1D ? ? ? ? D9 44 24").count(1).get(0).get<uintptr_t>(0), true).as_int(); //0x4BA680
    Screen.DefaultAR = *(float**)(pSetAR + 2);
    injector::MakeJMP(pSetAR, SetAspect, true);

    if (bFixHUD)
    {
        Screen.fHudScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
        Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

        pattern = hook::pattern("D8 0D ? ? ? ? 8B 35 ? ? ? ? 85 F6 D9 1D");
        injector::WriteMemory<float>(*pattern.count(1).get(0).get<float*>(2), Screen.fHudScaleX, true); //0x6814CC

        struct TextHudHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float fCurOffset = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[fCurOffset]
                }
                *(float*)(regs.esp + 0x28) = fCurOffset + Screen.fHudOffset;

            }
        }; 
        pattern = hook::pattern("D8 C1 D9 5C 24 28 DB 05 ? ? ? ? D9 47 2C");  //0x4DC226
        injector::MakeInline<TextHudHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 C1 D9 5C 24 28 D9 05 ? ? ? ? D8");  //0x4F12DB
        injector::MakeInline<TextHudHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 C1 D9 5C 24 18 DD D8 DD D8 DD D8");  //0x4F12E9
        struct HudHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                float fCurOffset = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[fCurOffset]
                }
                *(float*)(regs.esp + 0x18) = fCurOffset + Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook2>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

        pattern = hook::pattern("D8 44 24 20 D9 5C 24 60 0F 84 7A 03 00 00");  //0x4F1BD7
        struct HudHook3
        {
            void operator()(injector::reg_pack& regs)
            {
                float temp1 = *(float*)(regs.esp + 0x20);
                float temp2 = 0.0f;
                _asm
                {
                    fadd    dword ptr[temp1]
                    fstp    dword ptr[temp2]
                }
                *(float*)(regs.esp + 0x60) = temp2;
                *(float*)(regs.esp + 0x5C) += Screen.fHudOffset;
                *(float*)(regs.esp + 0x6C) += Screen.fHudOffset;
                *(float*)(regs.esp + 0x7C) += Screen.fHudOffset;
                *(float*)(regs.esp + 0x8C) += Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook3>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(8));
        injector::WriteMemory<uint16_t>(pattern.count(1).get(0).get<uint32_t>(6), 0xC084, true); //test    al, al

        pattern = hook::pattern("D8 C1 D9 5C 24 14 DD D8");  //0x4F0D06
        struct HudHook4
        {
            void operator()(injector::reg_pack& regs)
            {
                float temp1 = 0.0f;
                _asm
                {
                    fadd    st, st(1)
                    fstp    dword ptr[temp1]
                }
                *(float*)(regs.esp + 0x14) = temp1 + Screen.fHudOffset;
                *(float*)(regs.esp + 0x18) += Screen.fHudOffset;
            }
        }; injector::MakeInline<HudHook4>(pattern.count(2).get(1).get<uint32_t>(0), pattern.count(2).get(1).get<uint32_t>(6));

        pattern = hook::pattern("D8 C2 8B F0 D8 05 ? ? ? ? D8 0D ? ? ? ? D8"); //main menu cursor fix
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uintptr_t>(2), 0xC0, true); //0x4FC837
        injector::WriteMemory<uint8_t>(pattern.count(1).get(0).get<uintptr_t>(17), 0xC0, true); //0x4FC847
    }

    if (bFixFOV)
    {
        #undef SCREEN_FOV_HORIZONTAL
        #undef SCREEN_FOV_VERTICAL
        #define SCREEN_FOV_HORIZONTAL		48.0f
        #define SCREEN_FOV_VERTICAL			(2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_HORIZONTAL * 0.5f)) / SCREEN_AR_NARROW)))
        float fDynamicScreenFieldOfViewScale = 2.0f * RADIAN_TO_DEGREE(atan(tan(DEGREE_TO_RADIAN(SCREEN_FOV_VERTICAL * 0.5f)) * Screen.fAspectRatio)) * (1.0f / SCREEN_FOV_HORIZONTAL);
        static float fFOVFactor = fDynamicScreenFieldOfViewScale * 114.59155f;
        pattern = hook::pattern("D8 0D ? ? ? ? D9 9E C4 00 00 00 E8 ? ? ? ? D9 86"); //0x498BA3
        injector::WriteMemory(pattern.count(1).get(0).get<uintptr_t>(2), &fFOVFactor, true);
    }

    if (nGameSpeed)
    {
        pattern = hook::pattern("C7 05 ? ? ? ? 1E 00 00 00"); //0x52E7CA
        injector::WriteMemory(pattern.count(1).get(0).get<uintptr_t>(6), nGameSpeed, true);
    }
    //43F8B8 - fps

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