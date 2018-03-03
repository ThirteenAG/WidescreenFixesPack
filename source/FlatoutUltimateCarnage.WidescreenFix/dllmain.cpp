#include "stdafx.h"

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("B9 ? ? ? ? E8 ? ? ? ? 8B 7C 24 10 6A 65 8B F0 57 89 74 24 28");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    static auto dword_7242B0 = *pattern.get_first<uint32_t>(1);
    auto nWidth = *(uint32_t*)(dword_7242B0 + 0x8);
    auto nHeight = *(uint32_t*)(dword_7242B0 + 0xC);
    injector::WriteMemory<float>(dword_7242B0 + 0x904, static_cast<float>(nWidth), true);
    injector::WriteMemory<float>(dword_7242B0 + 0x908, static_cast<float>(nHeight), true);

    pattern = hook::pattern("D9 44 24 04 D9 99 04 09 00 00 D9 44"); // 0x608970
    struct SetAR
    {
        void operator()(injector::reg_pack& regs)
        {
            float temp = 0.0f;
            _asm fstp dword ptr[temp]
            _asm fstp dword ptr[temp]

            auto nWidth = *(uint32_t*)(dword_7242B0 + 0x8);
            auto nHeight = *(uint32_t*)(dword_7242B0 + 0xC);
            *(float*)(regs.ecx + 0x904) = static_cast<float>(nWidth);
            *(float*)(regs.ecx + 0x908) = static_cast<float>(nHeight);
        }
    }; injector::MakeInline<SetAR>(pattern.get_first(0), pattern.get_first(20));

    //pattern = hook::pattern(""); //
    //struct Hook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        float temp = 0.0f;
    //        _asm fstp    dword ptr[temp]
    //
    //        *(float*)(regs.edx + 0x54) = temp / 2.0f;
    //        regs.edx = *(uint32_t*)(regs.ecx + 0x2C);
    //    }
    //}; injector::MakeInline<Hook>(0x46FF14, 0x46FF14+6);
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
