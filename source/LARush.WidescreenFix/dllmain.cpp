#include "stdafx.h"

struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    int32_t nWidth43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fHudOffsetReal;
    float fTextScale;
} Screen;

void Init()
{
    CIniReader iniReader("");
    Screen.nWidth = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.nHeight = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bFix2D = iniReader.ReadInteger("MAIN", "Fix2D", 1) != 0;

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fTextScale = (1.0f / 640.0f) / Screen.fHudScale;

    //Registry
    auto pattern = hook::pattern("E8 ? ? ? ? 6A 20 E8 ? ? ? ? 8B E8 85 ED 89 6B 38");
    struct RegHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ecx + 0x37C) = 1;   // MovieInstalled
            *(uint32_t*)(regs.ecx + 0x380) = 0;   // IsCDROMVersion
            *(uint32_t*)(regs.ecx + 0x384) = 0;   // MovieCDNo
        }
    }; injector::MakeInline<RegHook>(pattern.get_first(0)); //0x54160C

    //Resolution
    pattern = hook::pattern("8B 44 24 04 85 C0 75 ? 8B 44 24");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x08) = Screen.nWidth;
            *(int32_t*)(regs.esp + 0x0C) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(6), pattern.get_first(28)); //0x53F6D6

    pattern = hook::pattern("89 BB 5C 03 00 00");
    struct ResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.ebx + 0x35C) = Screen.nWidth;
            *(int32_t*)(regs.ebx + 0x360) = Screen.nHeight;
        }
    }; injector::MakeInline<ResHook2>(pattern.get_first(0), pattern.get_first(6)); //0x53F6D6

    //Aspect Ratio
    pattern = hook::pattern("68 ? ? ? ? 8B CE E8 ? ? ? ? 51 D9 1C 24");
    injector::WriteMemory<float>(pattern.get_first(1), Screen.fAspectRatio, true);

    pattern = hook::pattern("F3 0F 10 05 ? ? ? ? 8B C7 99");
    injector::WriteMemory<float>(*pattern.get_first<void*>(4), Screen.fAspectRatio, true);

    if (bFix2D)
    {
        static auto ret_551F3B = (uint32_t)hook::get_pattern("83 C6 28 4D 75 F2 5F 5D 5E B8 01 00 00 00 5B 81 C4 98 00 00 00 C2 0C 00", 0);
        pattern = hook::pattern("F3 0F 11 56 ? F3 0F 11 4E ? E8 ? ? ? ? 8B E8 85 ED");
        struct TextHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esi + 0x0C) = regs.xmm2.f32[0];
                *(float*)(regs.esi + 0x10) = regs.xmm1.f32[0];

                auto ret = *(uint32_t*)(regs.esp + 0x28);
                if (ret == ret_551F3B)
                {
                    *(float*)(regs.esi + 0x04) /= Screen.fHudScale;
                    *(float*)(regs.esi + 0x08) /= Screen.fHudScale;
                }
            }
        }; injector::MakeInline<TextHook>(pattern.get_first(0), pattern.get_first(10)); //0x5504BA

        static auto ret_4ED9E4 = (uint32_t)hook::get_pattern("83 C4 04 8B 44 24 10 8B 0D ? ? ? ? 40 3B C1 89 44 24 10", 0);
        static auto ret_4D25FE = (uint32_t)hook::get_pattern("83 C4 04 5F 5B 83 C4 54", 0);
        static auto ret_4F62A6 = (uint32_t)hook::get_pattern("83 C4 40 5E 5D 83 C4 6C C3", 0);
        static auto ret_4ED33B = (uint32_t)hook::get_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 83 C4 04 3B C1 74 4B D9 05", 0);
        static auto ret_4FA384 = (uint32_t)hook::get_pattern("83 C4 34 5F 5D 5B 59 C2 08 00", 0);
        static auto ret_4EF78F = (uint32_t)hook::get_pattern("8B 44 24 34 83 C4 18 40 83 C7 02 83 F8 09", 0);
        pattern = hook::pattern("0F B6 46 42 0F B7 4E 40");
        struct BackgroundGraphicsHook
        {
            void operator()(injector::reg_pack& regs)
            {
                auto ret = *(uint32_t*)(regs.esp + 0x38);
                if (ret != ret_4ED9E4 && ret != ret_4D25FE && ret != ret_4F62A6 && ret != ret_4ED33B && ret != ret_4FA384 && ret != ret_4EF78F)
                {
                    *(float*)(regs.esi + 0x10) /= Screen.fHudScale;
                    *(float*)(regs.esi + 0x14) /= Screen.fHudScale;
                    *(float*)(regs.esi + 0x18) /= Screen.fHudScale;
                    *(float*)(regs.esi + 0x1C) /= Screen.fHudScale;
                }

                *(uint16_t*)&regs.ecx = *(uint16_t*)(regs.esi + 0x40);
                *(uint8_t*)&regs.eax = *(uint8_t*)(regs.esi + 0x42);
            }
        }; injector::MakeInline<BackgroundGraphicsHook>(pattern.get_first(0), pattern.get_first(8)); //550082

        //Mouse
        static auto ret_4C74E8 = (uint32_t)hook::get_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? F3 0F 10 05 ? ? ? ? 50 51 8D 54 24 40 F3 0F 11 44 24", 0);
        static auto ret_4C751D = (uint32_t)hook::get_pattern("83 C4 14 E9 ? ? ? ? 3B DF 75 43 F3 0F 10 44 24 ? F3 0F 58 44 24", 0);
        static auto ret_4F6CFC = (uint32_t)hook::get_pattern("68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? F3 0F 10 44 24 ? F3 0F 58 44 24", 0);
        pattern = hook::pattern("D9 58 0C DD D8 DD D8 83 C4 5C C3");
        struct MouseHook
        {
            void operator()(injector::reg_pack& regs)
            {
                if (*(uint32_t*)regs.esp != ret_4C74E8 && *(uint32_t*)regs.esp != ret_4C751D && *(uint32_t*)regs.esp != ret_4F6CFC)
                {
                    *(float *)(regs.eax + 0x0) /= Screen.fHudScale;
                    *(float *)(regs.eax + 0x0) += Screen.fHudOffsetReal / Screen.fWidth;
                    *(float *)(regs.eax + 0x8) /= Screen.fHudScale;
                }
            }
        }; injector::MakeInline<MouseHook>(pattern.get_first(10)); //4BF734
        injector::MakeRET(pattern.get_first(10 + 5));
    }
    else
    {
        //Text Scale
        pattern = hook::pattern("D8 0D ? ? ? ? D8 0D ? ? ? ? D9 1C 24 68");
        injector::WriteMemory<float>(*pattern.get_first<void*>(2), Screen.fTextScale, true); //0x5E0134
    }

    //Videos
    pattern = hook::pattern("E8 ? ? ? ? 89 44 24 60 E8 ? ? ? ? F3 0F 10 05");
    struct FMVHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x58) += (int32_t)Screen.fHudOffsetReal;
            regs.eax = Screen.nWidth43;
        }
    }; injector::MakeInline<FMVHook>(pattern.get_first(0)); //0x572C6F
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(Init, hook::pattern("8B 44 24 04 85 C0 75 ? 8B 44 24"));
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