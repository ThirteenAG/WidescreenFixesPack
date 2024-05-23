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
    static auto dword_126A26C = *hook::get_pattern<uint32_t*>("83 3D ? ? ? ? ? DC 0D ? ? ? ? B8 ? ? ? ? 0F 95 C2 89 46 28", 2);

    /*
    static bool bIsNotDialogue = false;
    pattern = hook::pattern("89 74 24 7C 89 44 24 10");
    struct DialogueCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x7C) = regs.esi;
            *(uint32_t*)(regs.esp + 0x10) = regs.eax;
            bIsNotDialogue = true;
        }
    }; injector::MakeInline<DialogueCheck>(pattern.get_first(0), pattern.get_first(8));
    */

    pattern = hook::pattern("F3 0F 58 46 18 5F F3 0F 11 46 18 5B 8B E5 5D C2 04 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = (*(float *)(regs.ebx + 0x204) - *(float *)(regs.eax + 0x204)) + *(float *)(regs.esi + 0x18);
            if (*dword_126A26C == 0 /*|| bIsNotDialogue*/)
                f = AdjustFOV(f, static_cast<float>(nWidth) / static_cast<float>(nHeight));
            regs.xmm0.f32[0] = f;
            //bIsNotDialogue = false;
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
    static auto off_16B0990 = *hook::get_pattern<uint32_t>("C7 45 ? ? ? ? ? 89 7D AC 89 4D B0 89 55 B8 89 45 BC", 3);
    static bool bIsDialogue1 = false;
    static bool bIsDialogue2 = false;

    pattern = hook::pattern("8A 45 08 56 88 47 2C FF 15 ? ? ? ? 5F 5E 5D C2 04 00");
    struct DialogueCheck1
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.edi + 0x2C) = *(uint8_t*)(regs.ebp + 0x8);
            bIsDialogue1 = *(uint32_t*)(regs.edi + 0x2C) != 0;
        }
    }; injector::MakeInline<DialogueCheck1>(pattern.get_first(0), pattern.get_first(7));
    injector::WriteMemory<uint8_t>(pattern.get_first(5), 0x56, true);

    pattern = hook::pattern("F3 0F 10 41 70 EB 05 F3 0F 10 46 74");
    struct DialogueCheck2
    {
        void operator()(injector::reg_pack& regs)
        {
            float f = *(float*)(regs.ecx + 0x70);
            regs.xmm0.f32[0] = f;
            bIsDialogue2 = (*(uint32_t*)(regs.ecx + 0x70) == 0x4039999A && bIsDialogue1); //2.900000095
        }
    }; injector::MakeInline<DialogueCheck2>(pattern.get_first(0));

    pattern = hook::pattern("D9 45 FC 50 51 D9 1C 24 57 E8 ? ? ? ? 8B 4D 0C 5F F3 0F 11 01 5E 8B E5 5D C2 08 00");
    struct FOVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (!bIsDialogue2 && *(uint32_t*)(regs.esp + 0x4C) == off_16B0990)
            {
                *(float*)(regs.ecx) = min(AdjustFOV(*(float*)(regs.ecx), static_cast<float>(nWidth) / static_cast<float>(nHeight)), 110.0f);
            }
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