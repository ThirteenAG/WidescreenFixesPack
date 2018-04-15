#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    int32_t Width43;
    float fAspectRatio;
    int32_t FOV;
    float fHudOffset;
    float fHudOffsetReal;
} Screen;

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("53 56 57 89 65 E8 33 F6 39");

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

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 8B 15 ? ? ? ? A1 ? ? ? ? 6A 10"); //4F3FEE
    static int32_t* dword_29D6FE8 = *pattern.get_first<int32_t*>(1);
    static int32_t* dword_29D6FE4 = *pattern.get_first<int32_t*>(17);

    struct SetResHook1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_29D6FE4 = Screen.Width;
            *dword_29D6FE8 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook1>(pattern.get_first(0));

    pattern = hook::pattern("A3 ? ? ? ? 8B 0D ? ? ? ? 6A 10 51 50"); //4F54A4
    struct SetResHook2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_29D6FE4 = Screen.Width;
            *dword_29D6FE8 = Screen.Height;
        }
    }; injector::MakeInline<SetResHook2>(pattern.get_first(0));

    pattern = hook::pattern("89 0D ? ? ? ? 8D 34 49"); //46463F
    static int32_t* dword_55ED00 = *pattern.get_first<int32_t*>(2);
    pattern = hook::pattern("A3 ? ? ? ? ? ? 8D 04 49 33 D2");
    static int32_t* dword_55ED18 = *pattern.get_first<int32_t*>(1);
    pattern = hook::pattern("C7 05 ? ? ? ? 00 10 00 00 5E A3");
    static int32_t* dword_5606CC = *pattern.get_first<int32_t*>(2);
    static int32_t* dword_5606D0 = *pattern.get_first<int32_t*>(12);
    struct AspectRatioHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_55ED00 = Screen.Width;
            *dword_55ED18 = Screen.Height;
            *dword_5606D0 = 4096;
            *dword_5606CC = 4096;
        }
    };
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 5E 5B 83 C4 08 C3"); //0x430466
    injector::MakeInline<AspectRatioHook>(pattern.get_first(0));
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 08 53 56"); //0x4645B8
    injector::MakeInline<AspectRatioHook>(pattern.get_first(0));

    //FOV
    Screen.FOV = static_cast<int32_t>(4096.0f / (Screen.fAspectRatio / (4.0f / 3.0f)));
    pattern = hook::pattern("F7 3D ? ? ? ? 33 C9 66 8B 4E"); //45E9E9
    injector::WriteMemory(pattern.get_first(2), &Screen.FOV, true);

    //Main menu wheel fix
    auto sub_4F0A70 = [](int16_t a1) -> int32_t
    {
        return (int32_t)(cos((float)(a1 & 0xFFF) * 0.00024414062f * 6.2831855f) * (4096.0f / (Screen.fHeight / 480.0f)));
    };
    pattern = hook::pattern("E8 ? ? ? ? 53 89 44 24 1C 89");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0A70), true); //0x457939
    pattern = hook::pattern("E8 ? ? ? ? 0F AF 44 24 20 C1");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0A70), true); //0x457992

    auto sub_4F0AA0 = [](int16_t a1) -> int32_t
    {
        return (int32_t)(sin((float)(a1 & 0xFFF) * 0.00024414062f * 6.2831855f) * (4096.0f / (Screen.fHeight / 480.0f)));
    };
    pattern = hook::pattern("E8 ? ? ? ? 8B F0 53 0F AF");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0AA0), true); //0x457947
    pattern = hook::pattern("E8 ? ? ? ? 8B 5D 0C 89 44");
    injector::MakeCALL(pattern.get_first(0), static_cast<int32_t(*)(int16_t)>(sub_4F0AA0), true); //0x45795D

    if (bFixHUD)
    {
        //menu backgrounds
        pattern = hook::pattern("A1 ? ? ? ? 81 EC 98"); //4D4BA0
        auto sub_4D4BA0 = pattern.get_first(0);
        static auto dword_29D6FF0 = *pattern.get_first<uint32_t*>(1);
        struct MenuHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *dword_29D6FF0;
                *(float*)(regs.esp + 0x8) += Screen.fHudOffset;
            }
        }; injector::MakeInline<MenuHook1>(sub_4D4BA0);

        auto rpattern = hook::range_pattern((uintptr_t)sub_4D4BA0, (uintptr_t)sub_4D4BA0 + 0x414, pattern_str(0xA1, to_bytes(dword_29D6FE4))); //mov     eax, dword_29D6FE4
        for (size_t i = 0; i < rpattern.size(); ++i)
        {
            //0x4D4CEF, 0x4D4D51, 0x4D4D8A, 0x4D4DBD, 0x4D4E4F, 0x4D4E82, 0x4D4EBB
            injector::WriteMemory(rpattern.get(i).get<uint32_t>(1), &Screen.Width43, true);
        }

        //some menu things
        pattern = hook::pattern("C7 41 28 00 00 80 BF"); //457C0E
        struct MenuHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int16_t*)(regs.ecx + 0x08) += (int16_t)Screen.fHudOffsetReal;
                *(int16_t*)(regs.ecx + 0x10) += (int16_t)Screen.fHudOffsetReal;
                *(int16_t*)(regs.ecx + 0x18) += (int16_t)Screen.fHudOffsetReal;
                *(int16_t*)(regs.ecx + 0x20) += (int16_t)Screen.fHudOffsetReal;
                *(float*)(regs.ecx + 0x28) = -1.0f;
            }
        };

        for (size_t i = 0; i < pattern.size(); ++i)
        {
            injector::MakeInline<MenuHook2>(pattern.get(i).get<uint32_t>(0), pattern.get(i).get<uint32_t>(7));

            rpattern = hook::range_pattern((uintptr_t)pattern.get(i).get<uint32_t>(-0x170), (uintptr_t)pattern.get(i).get<uint32_t>(0), pattern_str(0x0F, 0xAF, '?', to_bytes(dword_29D6FE4))); //imul    e?x, dword_29D6FE4
            for (size_t i = 0; i < rpattern.size(); ++i)
            {
                injector::WriteMemory(rpattern.get(i).get<uint32_t>(3), &Screen.Width43, true);
            }
        }
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