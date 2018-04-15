#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("C7 47 14 ? ? ? ? C7 47 18");

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
    bool bFixFOV = iniReader.ReadInteger("MAIN", "FixFOV", 1) != 0;
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.Width43);

    //pattern = hook::pattern("C7 47 14 ? ? ? ? C7 47 18"); //0x4C81A3 0x4C81AA 
    injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(3), Screen.fWidth, true);
    injector::WriteMemory<float>(pattern.count(1).get(0).get<uint32_t>(10), Screen.fHeight, true);

    pattern = hook::pattern("89 4F 14 D8 0D ? ? ? ? D9 5F 18"); //0x4C8235
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float temp = 0.0f;
            _asm {fstp    dword ptr[temp]}

            *(float*)(regs.edi + 0x14) = Screen.fWidth;
            *(float*)(regs.edi + 0x18) = Screen.fHeight;
        }
    }; injector::MakeInline<SetResHook>(pattern.count(1).get(0).get<uint32_t>(3), pattern.count(1).get(0).get<uint32_t>(12));

    pattern = hook::pattern("84 DB 74 09 DB 87 ? ? ? ? D9 5F 14"); //0x4C8241
    injector::MakeNOP(pattern.count(1).get(0).get<uint32_t>(0), 13, true);

    //3D
    static float f3DScale = 1.0f / (480.0f * Screen.fAspectRatio);
    pattern = hook::pattern("D8 0D ? ? ? ? D9 5C 24 04 D9 46"); //0x4BC66B
    injector::WriteMemory(pattern.count(1).get(0).get<uint32_t>(2), &f3DScale, true);

    pattern = hook::pattern("C7 05 ? ? ? ? 00 00 20 44"); //0x553DFC //mov 
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        injector::WriteMemory<float>(pattern.get(i).get<uint32_t>(6), (480.0f * Screen.fAspectRatio), true);
    }

    pattern = hook::pattern("68 00 00 F0 43 68 00 00 20 44"); //0x4CA7E6 //push
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        injector::WriteMemory<float>(pattern.get(i).get<uint32_t>(6), (480.0f * Screen.fAspectRatio), true);
    }

    if (bFixHUD)
    {
        Screen.fHudScale = Screen.fAspectRatio / (4.0f / 3.0f);
        pattern = hook::pattern("8B 81 ? ? ? ? D8 40 6C 5E D9 58 6C"); //0x4CB3C5
        struct HudScaleHook
        {
            void operator()(injector::reg_pack& regs)
            {
                DBGONLY(spd::log->info("{} {} {} {} {}", *(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x00), *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x10), *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x14), *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x18), *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x1C)););

                if (*(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x08) != 90.0f && *(uintptr_t*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x18) != 235587392) //fading check
                {
                    *(float*)(*(uintptr_t*)(regs.ecx + 0x0A859C) + 0x00) /= Screen.fHudScale;
                }

                regs.eax = *(uintptr_t*)(regs.ecx + 0x0C3B40);
            }
        }; injector::MakeInline<HudScaleHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(6));

        pattern = hook::pattern("DB 44 24 08 8D 86 A8 00 00 00 8D"); //0x5288D2
        struct RearviewMirrorHook
        {
            void operator()(injector::reg_pack& regs)
            {
                uint32_t fMirrorOffset = *(uint32_t*)(regs.esp + 0x8) + (uint32_t)(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
                _asm {fild    dword ptr[fMirrorOffset]}
                regs.eax = regs.esi + 0xA8;
            }
        }; injector::MakeInline<RearviewMirrorHook>(pattern.count(1).get(0).get<uint32_t>(0), pattern.count(1).get(0).get<uint32_t>(10));

    }

    if (bFixFOV)
    {
        Screen.fFieldOfView = GetFOV2(0.55f, Screen.fAspectRatio);
        pattern = hook::pattern("D8 0D ? ? ? ? 8D B2 ? ? ? ? 33 C0 8B FE D9 F2"); //0x4BC4BB
        injector::WriteMemory(pattern.count(1).get(0).get<uintptr_t>(2), &Screen.fFieldOfView, true);
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