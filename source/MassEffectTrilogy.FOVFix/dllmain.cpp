#include "stdafx.h"

void InitME1()
{
    auto pattern = hook::pattern("8B 35 ? ? ? ? 57 8B 3D ? ? ? ? 89 74 24 20 89 7C");
    if (pattern.empty())
        return;

    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(2));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(9));

    pattern = hook::pattern("D9 44 24 24 D9 1C 24 E8 ? ? ? ? B9 10 00 00 00 8B F0 8D BC 24 F0 00 00 00 F3 A5");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = *(float*)(regs.esp + 0x24);
            f *= 100.0f;
            f = AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight));
            f /= 100.0f;
            _asm {fld dword ptr[f]}
            _asm {fstp dword ptr[f]}
            *(float*)(regs.esp + 0x00) = f;
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(7));
}

void InitME2()
{
    auto pattern = hook::pattern("8B 35 ? ? ? ? 56 8B 35 ? ? ? ? 56 50 8D 43 3C");
    if (pattern.empty())
        return;

    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(9));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(2));

    pattern = hook::pattern("F3 0F 58 46 18 5F F3 0F 11 46 18 5B 8B E5 5D C2 04 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = (*(float *)(regs.ebx + 0x204) - *(float *)(regs.eax + 0x204)) + *(float *)(regs.esi + 0x18);
            if (f >= 60.0f)
                f = min(AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight)), 110.0f);
            _asm movss xmm0, dword ptr[f]
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(0));
}

void InitME3()
{
    auto pattern = hook::pattern("8B 0D ? ? ? ? 8B 15 ? ? ? ? 57 56 89 08 89 50 04 E8 ? ? ? ? 8B C6 5F 5E 5D C2 04 00 8B 0D ? ? ? ? 8B 15 ? ? ? ? 89 08 89 50 04");
    if (pattern.empty())
        return;

    static auto nWidth = std::ref(**pattern.get_first<uint32_t*>(2));
    static auto nHeight = std::ref(**pattern.get_first<uint32_t*>(8));

    pattern = hook::pattern("D9 45 FC 50 51 D9 1C 24 57 E8 ? ? ? ? 8B 4D 0C 5F F3 0F 11 01 5E 8B E5 5D C2 08 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = *(float*)(regs.ecx);
            if (f >= 60.0f)
                *(float*)(regs.ecx) = min(AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight)), 110.0f);
        }
    }; injector::MakeInline<FOVHook>(pattern.get_first(22));
    injector::WriteMemory<uint8_t>(pattern.get_first(22 + 5), 0x5E, true); //pop esi
    injector::WriteMemory<uint16_t>(pattern.get_first(22 + 6), 0xE58B, true); //mov esp, ebp
    injector::WriteMemory<uint8_t>(pattern.get_first(22 + 8), 0x5D, true); //pop ebp
    injector::WriteMemory<uint8_t>(pattern.get_first(22 + 9), 0xC2, true); //ret
    injector::WriteMemory<uint16_t>(pattern.get_first(22 + 10), 0x0008, true); //8
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(InitME1, hook::pattern("8B 41 44 83 F8 01 75 0F 8B 44 24 08 D9 00 8B 4C 24 04 D9 19 C2 14 00")/*.count_hint(1).empty(), 0x1100*/);
            CallbackHandler::RegisterCallback(InitME2, hook::pattern("8B 41 44 83 F8 01 75 0F 8B 44 24 08 D9 00 8B 4C 24 04 D9 19 C2 14 00")/*.count_hint(1).empty(), 0x1100*/);
            CallbackHandler::RegisterCallback(InitME3, hook::pattern("55 8B EC 8B 41 44 83 F8 01 75 0E 8B 45 0C D9 00 8B 4D 08 D9 19 5D C2")/*.count_hint(1).empty(), 0x1100*/);
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