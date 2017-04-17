#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
} Screen;

DWORD WINAPI WindowCheck(LPVOID hWnd)
{
    while (true)
    {
        Sleep(10);

        if (*(HWND*)hWnd && !IsWindow(*(HWND*)hWnd))
            ExitProcess(0);
    }
    return 0;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("83 EC 0C 33 C0 53 57 33 DB");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bEndProcess = iniReader.ReadInteger("MAIN", "EndProcessOnWindowClose", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.Width43 = static_cast<uint32_t>(Screen.Height * (4.0f / 3.0f));

    //wndmode crashfix
    pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00"); //0x49225B
    injector::MakeNOP(pattern.get_first(0), 5, true);

    //Player_a bugfix
    pattern = hook::pattern("88 1D ? ? ? ? 88 1D ? ? ? ? C6 05 ? ? ? ? 01 88 1D ? ? ? ? C7 05"); //0x42B512
    static auto byte_51029C = *pattern.get_first<uint8_t*>(2);
    struct patch_player_a
    {
        void operator()(injector::reg_pack& regs)
        {
            *byte_51029C = 0x06;
        }
    };
    injector::MakeInline<patch_player_a>(pattern.get_first(0), pattern.get_first(6));
    pattern = hook::pattern("A2 ? ? ? ? E8 ? ? ? ? 8B F0 83"); //0x4274FF
    injector::MakeInline<patch_player_a>(pattern.get_first(0));

    //Res change
    pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40"); //0x491E4C
    static auto nWidth = *pattern.get_first<uint32_t*>(1);
    static auto dword_787370 = *pattern.get_first<uint32_t*>(6);
    static auto nHeight = *pattern.get_first<uint32_t*>(20);
    static auto dword_787388 = *pattern.get_first<uint32_t*>(25);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *nWidth = Screen.Width;
            *dword_787370 = Screen.Width;
            regs.eax = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

    pattern = hook::pattern("89 B1 ? ? ? ? D1 F8 89 81"); //0x46453B
    static auto dword_74F168 = *pattern.get_first<uint32_t>(2);
    struct SetResHook43
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = Screen.Width43;
            *(uint32_t*)(regs.ecx + dword_74F168) = regs.esi;
        }
    }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 40 02 00 00"); //0x48C1A0
    struct SetResHookX
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Width - 1;
        }
    }; injector::MakeInline<SetResHookX>(pattern.get_first(0), pattern.get_first(19));

    pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 83 C4 08 40 83"); //0x414FF7
    static auto dword_504CC0 = *pattern.get_first<uint32_t*>(1);
    struct SetResHookX1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_504CC0 = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX1>(pattern.get_first(0));

    pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3"); //0x43B7CF
    static auto dword_5C0C00 = *pattern.get_first<uint32_t*>(2);
    struct SetResHookX2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_5C0C00 = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 44 02 00 00 8B 91"); //0x48C1C0
    struct SetResHookY
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Height - 1;
        }
    }; injector::MakeInline<SetResHookY>(pattern.get_first(0), pattern.get_first(19));

    //Menu fix
    pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC"); //0x4898C3
    injector::WriteMemory(pattern.get_first(1), Screen.Width, true);

    //FMV crashfix
    static int n480 = 480;
    pattern = hook::pattern("A1 ? ? ? ? 74 56 85 C0 0F");
    injector::WriteMemory(pattern.get_first(1), &n480, true); //42D83A
    pattern = hook::pattern("3B 15 ? ? ? ? 7C B7 5B 5F 5E C3");
    injector::WriteMemory(pattern.get_first(2), &n480, true); //42D88B
    pattern = hook::pattern("3B 15 ? ? ? ? 7C C7 5F 5E C3");
    injector::WriteMemory(pattern.get_first(2), &n480, true); //42D8CC

    if (bEndProcess)
    {
        pattern = hook::pattern("3B 05 ? ? ? ? 0F 94 C3 85 DB"); //0x4B4FB8
        auto hwnd = *pattern.get_first<HWND*>(2);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);
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