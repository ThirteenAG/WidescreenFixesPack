#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fFieldOfView;
    float fHUDScaleX;
    float fHudOffset;
    float fHudOffsetReal;
    float fAspectRatioDiff;
} Screen;

bool isTTB;
uintptr_t* pMemory;
uint32_t off_288, off_290, off_2C0;
void __cdecl sub_44A390(uintptr_t a1, float a2, float a3, float a4, float a5, float a6, float a7, float a8, float a9, float a10, float a11, uintptr_t a12, uintptr_t a13, uintptr_t a14)
{
    uintptr_t Memory = *pMemory;

    if (a2 == 0.0f && a4 == Screen.fWidth)
    {
        if (a12)
        {
            if (*(uint32_t*)(a12 + 0x00) == 0xffffffff && *(uint32_t*)(a12 + 0x04) == 0xffffffff && *(uint32_t*)(a12 + 0x08) == 0xffffffff && *(uint32_t*)(a12 + 0x0C) == 0xffffffff)
            {
                a2 /= Screen.fAspectRatioDiff;
                a4 /= Screen.fAspectRatioDiff;
                a2 += Screen.fHudOffsetReal;
            }
        }
    }
    else
    {
        a2 /= Screen.fAspectRatioDiff;
        a4 /= Screen.fAspectRatioDiff;
        a2 += Screen.fHudOffsetReal;
    }

    float v14 = a6 + 0.5f;
    float v22 = a11 - a7 - 0.5f;
    float v15 = 1.0f / a10;
    float v23 = 1.0f / a11;

    if (a14)
    {
        *(float*)(a1 + 0x20) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288) + a4;
        *(float*)(a1 + 0x38) = a8;
        *(float*)(a1 + 0x24) = a3 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_290);
        *(float*)(a1 + 0x3C) = a7;
        *(float*)(a1 + 0x18) = a6;
        *(float*)(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288);
        *(float*)(a1 + 0x1C) = a9;
        *(float*)(a1 + 0x04) = a5 + *(float*)(a1 + 0x24);
    }
    else
    {
        *(float*)(a1 + 0x00) = a2 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_288);
        *(float*)(a1 + 0x04) = a3 + *(float*)(off_2C0 * *((uint32_t*)Memory + 0x13) + *((uint32_t*)Memory + 0x10) + off_290);
        *(float*)(a1 + 0x18) = v15 * v14;
        *(float*)(a1 + 0x1C) = v23 * v22;
        *(float*)(a1 + 0x20) = a4 + *(float*)a1;
        *(float*)(a1 + 0x24) = a5 + *(float*)(a1 + 4);
        *(float*)(a1 + 0x38) = (v14 + a8) * v15;
        *(float*)(a1 + 0x3C) = (v22 - a9) * v23;
    }
    *(uint32_t*)(a1 + 0x40) = *(uint32_t*)(a1 + 0x20);
    *(uint32_t*)(a1 + 0x44) = *(uint32_t*)(a1 + 0x24);
    *(uint32_t*)(a1 + 0x48) = *(uint32_t*)(a1 + 0x28);
    *(uint32_t*)(a1 + 0x4C) = *(uint32_t*)(a1 + 0x2C);
    *(uint32_t*)(a1 + 0x50) = *(uint32_t*)(a1 + 0x30);
    *(uint32_t*)(a1 + 0x54) = *(uint32_t*)(a1 + 0x34);
    *(uint32_t*)(a1 + 0x58) = *(uint32_t*)(a1 + 0x38);
    *(uint32_t*)(a1 + 0x5C) = *(uint32_t*)(a1 + 0x3C);
    *(uint32_t*)(a1 + 0x60) = *(uint32_t*)(a1 + 0x00);
    *(uint32_t*)(a1 + 0x64) = *(uint32_t*)(a1 + 0x04);
    *(uint32_t*)(a1 + 0x68) = *(uint32_t*)(a1 + 0x08);
    *(uint32_t*)(a1 + 0x6C) = *(uint32_t*)(a1 + 0x0C);
    *(uint32_t*)(a1 + 0x70) = *(uint32_t*)(a1 + 0x10);
    *(uint32_t*)(a1 + 0x74) = *(uint32_t*)(a1 + 0x14);
    *(uint32_t*)(a1 + 0x78) = *(uint32_t*)(a1 + 0x18);
    *(uint32_t*)(a1 + 0x7C) = *(uint32_t*)(a1 + 0x1C);
    *(uint32_t*)(a1 + 0x3C) = *(uint32_t*)(a1 + 0x1C);
    *(uint32_t*)(a1 + 0x24) = *(uint32_t*)(a1 + 0x04);
    *(uint32_t*)(a1 + 0x7C) = *(uint32_t*)(a1 + 0x5C);
    *(uint32_t*)(a1 + 0x64) = *(uint32_t*)(a1 + 0x44);
    *(uint32_t*)(a1 + 0x08) = 0x3727C5AC;
    *(uint32_t*)(a1 + 0x0C) = 0x3F800000;
    if (a12)
        *(uint32_t*)(a1 + 0x10) = *(uint32_t*)(a12 + 0x00);
    else
        *(uint32_t*)(a1 + 0x10) = 0xFFFFFFFF;
    *(uint32_t*)(a1 + 0x14) &= 0xFF000000;
    *(uint32_t*)(a1 + 0x14) = 0xFF000000;
    *(uint32_t*)(a1 + 0x28) = 0x3727C5AC;
    *(uint32_t*)(a1 + 0x2C) = 0x3F800000;
    if (a12)
        *(uint32_t*)(a1 + 0x30) = *(uint32_t*)(a12 + 0x04);
    else
        *(uint32_t*)(a1 + 0x30) = 0xFFFFFFFF;
    *(uint32_t*)(a1 + 0x34) &= 0xFF000000;
    *(uint32_t*)(a1 + 0x34) = 0xFF000000;
    *(uint32_t*)(a1 + 0x48) = 0x3727C5AC;
    *(uint32_t*)(a1 + 0x4C) = 0x3F800000;
    if (a12)
        *(uint32_t*)(a1 + 0x50) = *(uint32_t*)(a12 + 0x08);
    else
        *(uint32_t*)(a1 + 0x50) = 0xFFFFFFFF;
    *(uint32_t*)(a1 + 0x54) &= 0xFF000000;
    *(uint32_t*)(a1 + 0x54) = 0xFF000000;
    *(uint32_t*)(a1 + 0x68) = 0x3727C5AC;
    *(uint32_t*)(a1 + 0x6C) = 0x3F800000;
    if (a12)
        *(uint32_t*)(a1 + 0x70) = *(uint32_t*)(a12 + 0x0C);
    else
        *(uint32_t*)(a1 + 0x70) = 0xFFFFFFFF;
    *(uint32_t*)(a1 + 0x74) &= 0xFF000000;
    *(uint32_t*)(a1 + 0x74) = 0xFF000000;
    if (a13)
        *(uint32_t*)a13 = 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("75 4B D9 45 ? D8 5D ? DF E0 F6 C4 41");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    bool bDisableCutsceneBorders = iniReader.ReadInteger("MAIN", "DisableCutsceneBorders", 1) != 0;
    static float fFOVFactor = iniReader.ReadFloat("MAIN", "FOVFactor", 0.0f);

    //unlocking resolutions
    injector::MakeNOP(pattern.get_first(0), 2, true);
    injector::MakeNOP(pattern.get_first(13), 2, true);

    //unlocking windowed resolutions
    pattern = hook::pattern("8B 15 ? ? ? ? 83 E2 01"); //0x4B35E2
    injector::WriteMemory<uint16_t>(pattern.get_first(9), 0xE990i16, true);

    //The Game Play disk could not be found error
    pattern = hook::pattern("85 C0 ? ? 8B 4C 24 0C 51"); //0x43B989 in ttb
    injector::WriteMemory<uint8_t>(pattern.get_first(2), 0xEBi8, true); //0x43B959
    pattern = hook::pattern("75 ? 8D 44 24 08 50 8B 44 24 10");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    //FOV
    pattern = hook::pattern("D9 05 ? ? ? ? D8 B1 ? ? ? ? D9 E8 D9 F3");
    injector::WriteMemory(pattern.count(2).get(0).get<void*>(2), &Screen.fFieldOfView, true); //fld
    injector::WriteMemory(pattern.count(2).get(1).get<void*>(2), &Screen.fFieldOfView, true); //fld
    pattern = hook::pattern("D8 0D ? ? ? ? D8 B6 ? ? ? ? 5E D9 E8 D9 F3");
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true); //fmul
    pattern = hook::pattern("D8 0D ? ? ? ? D9 96");
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true); //fmul
    pattern = hook::pattern("D8 3D ? ? ? ? C2 04 00");
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true); //fdiv
    pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? FF");
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true); //fdiv
    pattern = hook::pattern("D8 3D ? ? ? ? D9 99 ? ? ? ? C2 04 00");
    static auto pFOV = *pattern.get_first<float*>(2);
    injector::WriteMemory(pattern.get_first(2), &Screen.fFieldOfView, true); //fdiv

    //HUD
    pMemory = *(uintptr_t**)hook::get_pattern("8B 0D ? ? ? ? DD D8 8B 51 4C DD D8", 2); //0x5C5D7C
    off_288 = *hook::get_pattern<uint32_t>("D8 84 0A ? ? ? ? D8 44 24 14 D9 58 ? 8B 0D", 3);
    off_290 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -4);
    off_2C0 = *hook::get_pattern<uint32_t>("8B 54 24 24 8B 4C 24 20 89 50 38 D9 58 24", -11);
    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 83 C4 38 50");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44A6C0
    pattern = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 38 51");
    injector::MakeCALL(pattern.get_first(0), sub_44A390, true); //0x44AB58

    //resolution
    static auto unk_5C6CEC = *(uintptr_t**)hook::get_pattern("68 ? ? ? ? 56 8D 4C 24", 1);
    pattern = hook::pattern("8B 52 08 89 50 08 C2 08 00"); //0x4460E7
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = *(uint32_t*)(regs.edx + 0x08);
            *(uint32_t*)(regs.eax + 0x08) = regs.edx;

            Screen.Width = *(uint32_t*)(unk_5C6CEC + 0);
            Screen.Height = *(uint32_t*)(unk_5C6CEC + 1);
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.fHUDScaleX = 1.0f / Screen.fWidth * (Screen.fHeight / 480.0f);
            Screen.fAspectRatioDiff = (Screen.fAspectRatio / (4.0f / 3.0f));
            Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
            Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fFieldOfView = (*pFOV / (Screen.fAspectRatio / (4.0f / 3.0f))) / (fFOVFactor ? fFOVFactor : 1.0f);
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(6));

    if (bDisableCutsceneBorders)
    {
        pattern = hook::pattern("51 53 55 56 8B F1 8B 46 34 85 C0"); //0x4B0B30
        injector::MakeRET(pattern.get_first());
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