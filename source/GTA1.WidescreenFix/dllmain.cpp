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

void InitGtaClassicsSteam()
{
    // Patches:
    // - GTA1 Rockstar Classics & Steam: SHA256=6A5C313D9C0D87B4EFA527BAAC7097DD4A149B9B2DA5261ABB224083F5CDC1C2 (774144 bytes)

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bEndProcess = iniReader.ReadInteger("MAIN", "EndProcessOnWindowClose", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.Width43 = static_cast<uint32_t>(Screen.Height * (4.0f / 3.0f));

    //wndmode crashfix (caused by the ASI loader)
    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00"); //0x49225B
    injector::MakeNOP(pattern.get_first(0), 5, true);

    //Player_a bugfix (only in the Rockstar Classics / Steam version)
    //Fix by Daniel Marschall,
    //see technical details here: https://misc.daniel-marschall.de/spiele/gta1/player_a_bugfix.txt
    pattern = hook::pattern("3B C2 0F 9D C2 FE CA 83 E2 06");
    if (!pattern.empty()) { // This patch is optional; if pattern does not exist, then do nothing.
        struct patch_player_a_new
        {
            void operator()(injector::reg_pack& regs)
            {
                // mov     al, [ecx] ; ecx is a pointer to the saved resolution (values 0..5)
                // cmp     eax, edx  ; <-- Patch: mov edx, eax
                // setnl   dl        ; <-- Patch: nop
                // dec     dl        ; <-- Patch: nop
                // and     edx, 6    ; <-- Patch: nop (Bug is in this line: Resolution "6" does not exist!)
                // mov     byte ptr byte_00503224, dl  ; In byte_00503224, the to-be-applied resolution will be saved
                regs.edx = regs.eax;
            }
        };
        injector::MakeInline<patch_player_a_new>(pattern.get_first(0), pattern.get_first(10));
    }

    //Res change
    pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40"); //0x491E4C
    static auto nWidth = *pattern.get_first<uint32_t*>(1);
    static auto dword_787370 = *pattern.get_first<uint32_t*>(6);
    //static auto nHeight = *pattern.get_first<uint32_t*>(20);
    //static auto dword_787388 = *pattern.get_first<uint32_t*>(25);
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

    //Fix crash in FMV (cut scenes) due to the resolution change
    static int n480 = 480; // fix dword_785174
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
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);
    }
}

// TODO: GTA 1 Retail, London 1969 and London 1961:
//       The game crashes if a cut scene is replayed (space bar), most likely because of a timer issue:
//       They are heavily accelerated (like the menu).
//       The cut scenes do work when they are triggered via mission complete.

void InitLondonRetail()
{
    // Patches:
    // - GTA London 1969 Retail (CD): SHA256=1FA3934F330B0097A0CB8160E3968D78E93CB4A808703AD4854BDB1F87BAE39A (1231360 bytes)
    // - GTA London 1961 (Download):  SHA256=549020E49AAD17D1EF26BC5766A8374503C5A5936251B793BF8BE2A1D90700C5 (1235968 bytes)

    // Note: Variable names in this function (dword_...) are taken from GTA London 1969, not 1961

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bEndProcess = iniReader.ReadInteger("MAIN", "EndProcessOnWindowClose", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.Width43 = static_cast<uint32_t>(Screen.Height * (4.0f / 3.0f));

    //wndmode crashfix (caused by the ASI loader)
    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00");
    injector::MakeNOP(pattern.get_first(0), 5, true);

    //Res change
    pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40");
    static auto nWidth = *pattern.get_first<uint32_t*>(1);
    static auto dword_854C30 = *pattern.get_first<uint32_t*>(6);
    //static auto nHeight = *pattern.get_first<uint32_t*>(20);
    //static auto dword_854C48 = *pattern.get_first<uint32_t*>(25);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *nWidth = Screen.Width;
            *dword_854C30 = Screen.Width;
            regs.eax = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

    pattern = hook::pattern("8B 81 ? ? ? ? 99 2B C2 D1 F8");
    static auto dword_80CF48 = *pattern.get_first<uint32_t>(2);
    struct SetResHook43
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.ecx + dword_80CF48);
            regs.esi = Screen.Width43;
            *(uint32_t*)(regs.ecx + dword_80CF48) = regs.esi;
        }
    }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 40 02 00 00 8B 91 38 02 00 00 2B");
    struct SetResHookX
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Width - 1;
        }
    }; injector::MakeInline<SetResHookX>(pattern.get_first(0), pattern.get_first(19));

    pattern = hook::pattern("A1 ? ? ? ? 83 C4 08 40 83");
    static auto dword_55128C = *pattern.get_first<uint32_t*>(1);
    struct SetResHookX1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_55128C = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX1>(pattern.get_first(0));

    pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
    static auto dword_626638 = *pattern.get_first<uint32_t*>(2);
    struct SetResHookX2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_626638 = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 44 02 00 00 8B 91");
    struct SetResHookY
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Height - 1;
        }
    }; injector::MakeInline<SetResHookY>(pattern.get_first(0), pattern.get_first(19));

    //Menu fix
    pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC");
    injector::WriteMemory(pattern.get_first(1), Screen.Width, true);

    //Fix crash in FMV (cut scenes) due to the resolution change
    static int n480 = 480;
    pattern = hook::pattern("A1 ? ? ? ? 33 D2 85 C0 0F ? ? ? ?");
    injector::WriteMemory(pattern.get_first(1), &n480, true);
    pattern = hook::pattern("42 F3 A4 3B 15 ? ? ? ? 7C B7 5F 5E");
    injector::WriteMemory(pattern.get_first(5), &n480, true);
    pattern = hook::pattern("A1 ? ? ? ? 33 D2 85 C0 7E 39 8B FA");
    injector::WriteMemory(pattern.get_first(1), &n480, true);
    pattern = hook::pattern("42 F3 AA 3B 15 ? ? ? ? 7C C7 5F 5E");
    injector::WriteMemory(pattern.get_first(5), &n480, true);

    if (bEndProcess)
    {
        pattern = hook::pattern("3B 05 ? ? ? ? 0F 94 C3 85 DB");
        auto hwnd = *pattern.get_first<HWND*>(2);
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);
    }
}

void InitGtaRetail()
{
    // Patches:
    // - GTA1 Retail (CD): SHA256=5AF3C1D90A85F694B184F3FC242E5F8F13B01BFE9BC706C655E97B7BC21599C8 (909824 bytes)

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);
    bool bEndProcess = iniReader.ReadInteger("MAIN", "EndProcessOnWindowClose", 0) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.Width43 = static_cast<uint32_t>(Screen.Height * (4.0f / 3.0f));

    //wndmode crashfix (caused by the ASI loader)
    auto pattern = hook::pattern("E8 ? ? ? ? 83 C4 04 B8 01 00 00 00 5D 5F 5E 5B 81 C4 70 05 00 00");
    injector::MakeNOP(pattern.get_first(0), 5, true);

    //Res change
    pattern = hook::pattern("A3 ? ? ? ? A3 ? ? ? ? 8B 83 B4 01 00 00 40");
    static auto nWidth = *pattern.get_first<uint32_t*>(1);
    static auto dword_77E990 = *pattern.get_first<uint32_t*>(6);
    //static auto nHeight = *pattern.get_first<uint32_t*>(20);
    //static auto dword_77E9A8 = *pattern.get_first<uint32_t*>(25);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *nWidth = Screen.Width;
            *dword_77E990 = Screen.Width;
            regs.eax = Screen.Height;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(0), pattern.get_first(17));

    pattern = hook::pattern("8B 86 ? ? ? ? 99 2B C2 D1 F8 8D 0C 49");
    static auto dword_6575E8 = *pattern.get_first<uint32_t>(2);
    struct SetResHook43
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *(uint32_t*)(regs.esi + dword_6575E8);
            regs.ecx = Screen.Width43;
            *(uint32_t*)(regs.esi + dword_6575E8) = regs.ecx;
        }
    }; injector::MakeInline<SetResHook43>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 40 02 00 00 8B 91 38 02 00 00 2B");
    struct SetResHookX
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Width - 1;
        }
    }; injector::MakeInline<SetResHookX>(pattern.get_first(0), pattern.get_first(19));

    pattern = hook::pattern("A1 ? ? ? ? 40 A3 10");
    static auto dword_4BF010 = *pattern.get_first<uint32_t*>(1);
    struct SetResHookX1
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_4BF010 = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX1>(pattern.get_first(0));

    pattern = hook::pattern("89 15 ? ? ? ? 8B 40 0C A3 ? ? ? ? C3");
    static auto dword_5F18E8 = *pattern.get_first<uint32_t*>(2);
    struct SetResHookX2
    {
        void operator()(injector::reg_pack& regs)
        {
            *dword_5F18E8 = Screen.Width;
        }
    }; injector::MakeInline<SetResHookX2>(pattern.get_first(0), pattern.get_first(6));

    pattern = hook::pattern("8B 4C 24 04 8B 81 44 02 00 00 8B 91");
    struct SetResHookY
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Height - 1;
        }
    }; injector::MakeInline<SetResHookY>(pattern.get_first(0), pattern.get_first(19));

    //Menu fix
    pattern = hook::pattern("BB ? ? ? ? 2B 5D 0C D1 E3 89 5D FC");
    injector::WriteMemory(pattern.get_first(1), Screen.Width, true);

    //Fix crash in FMV (cut scenes) due to the resolution change
    static int n480 = 480;
    pattern = hook::pattern("A1 ? ? ? ? 89 54 24 10 3B C2 0F 8E 95 00 00 00");
    injector::WriteMemory(pattern.get_first(1), &n480, true);
    pattern = hook::pattern("42 03 F3 F3 A5 3B 15 ? ? ? ? 7C B1");
    injector::WriteMemory(pattern.get_first(7), &n480, true);
    pattern = hook::pattern("33 D2 A1 ? ? ? ? 3B C2 7E 33");
    injector::WriteMemory(pattern.get_first(3), &n480, true);
    pattern = hook::pattern("F3 AB 3B 15 ? ? ? ? 7C CD");
    injector::WriteMemory(pattern.get_first(4), &n480, true);

    if (bEndProcess)
    {
        pattern = hook::pattern("3B 05 ? ? ? ? 0F 94 C3 85 DB");
        auto hwnd = *pattern.get_first<HWND*>(2);
        CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            CallbackHandler::RegisterCallback(InitGtaClassicsSteam, hook::pattern("83 EC 0C 33 C0 53 57 33 DB"));

            CallbackHandler::RegisterCallback(InitLondonRetail, hook::pattern("51 53 55 56 68 ? ? ? ? 33 DB"));

            CallbackHandler::RegisterCallback(InitGtaRetail, hook::pattern("53 56 57 55 33 ED 68 ? ? ? ? E8"));
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
