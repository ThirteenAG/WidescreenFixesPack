#include "stdafx.h"

void InitME12()
{
    auto ofs1 = 9, ofs2 = 2;
    auto pattern = hook::pattern("8B 35 ? ? ? ? 56 8B 35 ? ? ? ? 56 50 8D 43 3C");
    if (pattern.empty())
    {
        pattern = hook::pattern("8B 35 ? ? ? ? 57 8B 3D ? ? ? ? 89 74 24 20 89 7C");
        ofs1 = 2;
        ofs2 = 9;
        if (pattern.empty())
            return;
    }

    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(ofs1));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(ofs2));

    pattern = hook::pattern("8B 41 44 83 F8 01 75 0F 8B 44 24 08 D9 00 8B 4C 24 04 D9 19 C2 14 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esp + 0x08);
            float f = *(float*)regs.eax;
            int32_t n = static_cast<int32_t>(f);
            if (n == 70 || n == 80)
            {
                f = AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight));
                *(float*)regs.eax = f;
            }
            _asm fld dword ptr[f]
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(8), pattern.get_first(8 + 6));
}

void InitME3()
{
    auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 15 ? ? ? ? 57 56 89 08 89 50 04 E8 ? ? ? ? 8B C6 5F 5E 5D C2 04 00 8B 0D ? ? ? ? 8B 15 ? ? ? ? 89 08 89 50 04");
    if (pattern.empty())
        return;

    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(2));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(8));

    pattern = hook::pattern("55 8B EC 8B 41 44 83 F8 01 75 0E 8B 45 0C D9 00 8B 4D 08 D9 19 5D C2 14 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ebp + 0x0C);
            float f = *(float*)regs.eax;
            int32_t n = static_cast<int32_t>(f);
            if (n == 70 || n == 80)
            {
                f = AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight));
                *(float*)regs.eax = f;
            }
            _asm fld dword ptr[f]
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(11));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(InitME12, hook::pattern("8B 41 44 83 F8 01 75 0F 8B 44 24 08 D9 00 8B 4C 24 04 D9 19 C2 14 00")/*.count_hint(1).empty(), 0x1100*/);
        CallbackHandler::RegisterCallback(InitME3, hook::pattern("55 8B EC 8B 41 44 83 F8 01 75 0E 8B 45 0C D9 00 8B 4D 08 D9 19 5D C2 14 00")/*.count_hint(1).empty(), 0x1100*/);
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        InitializeASI();
    }
    return TRUE;
}