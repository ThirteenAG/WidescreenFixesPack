#include "stdafx.h"

struct Screen
{
    int32_t Width;
    int32_t Height;
    int32_t Width43;
    int32_t Height43;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    float fHudScale;
    int32_t nHudScale;
    float fHudOffset;
    float fCameraZoom;
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

uintptr_t off_5952C4, esp40, esp44;
uintptr_t dword_6733F0, dword_4C834B, dword_4C74EA;
uintptr_t gbh_DrawQuadAddr;
void __declspec(naked) gbh_DrawQuad()
{
    _asm mov ecx, [esp + 0x40]
    _asm mov esp40, ecx
    _asm mov ecx, [esp + 0x44]
    _asm mov esp44, ecx
    if (esp40 != dword_4C834B && esp44 != dword_4C74EA)
    {
        *(float*)(dword_6733F0 + 0x00) += Screen.fHudOffset;
        *(float*)(dword_6733F0 + 0x20) += Screen.fHudOffset;
        *(float*)(dword_6733F0 + 0x40) += Screen.fHudOffset;
        *(float*)(dword_6733F0 + 0x60) += Screen.fHudOffset;
    }
    gbh_DrawQuadAddr = *(uintptr_t*)off_5952C4;

    _asm jmp gbh_DrawQuadAddr;
}

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("83 EC 68 55 56 8B 74 24 74");

    if (pattern.empty() && !bDelay)
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
    auto szCameraZoom = iniReader.ReadString("MAIN", "CameraZoomFactor", "auto");
    auto bSkipMovie = iniReader.ReadInteger("MAIN", "SkipMovie", 1) != 0;
    static auto bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    auto bFixMenu = iniReader.ReadInteger("MAIN", "FixMenu", 1) != 0;

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScale = ((16384.0f * (Screen.fWidth / 640.0f)) / (Screen.fAspectRatio / (4.0f / 3.0f)));
    Screen.nHudScale = static_cast<int32_t>(Screen.fHudScale);
    Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    if (strncmp(szCameraZoom, "auto", 4) != 0)
        Screen.fCameraZoom = iniReader.ReadFloat("MAIN", "CameraZoomFactor", 1.0f);
    else
        Screen.fCameraZoom = (Screen.fAspectRatio / (4.0f / 3.0f)) * 2.25f;

    //Res change
    pattern = hook::pattern("8B 2D ? ? ? ? 56 8B 35 ? ? ? ? 57 8B 3D"); //0x4CB29F
    static auto dword_6732E0 = *pattern.get_first<uint32_t*>(2);
    pattern = hook::pattern("89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? EB 5F"); //0x4CB2D5
    static auto dword_673578 = *pattern.get_first<uint32_t*>(2);
    static auto dword_6732E8 = *pattern.get_first<uint32_t*>(7);
    static auto dword_6732E4 = *pattern.get_first<uint32_t*>(13);
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Height;
            *dword_673578 = Screen.Width;
            *dword_6732E8 = Screen.Height;
            *dword_6732E4 = Screen.Width;
        }
    }; injector::MakeInline<SetResHook>(pattern.get_first(-12), pattern.get_first(17));

    pattern = hook::pattern("74 49 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 84 C0");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CC61E
    injector::WriteMemory<uint8_t>(pattern.get_first(24), 0xEB, true); //0x4CC636
    pattern = hook::pattern("74 2B A1 ? ? ? ? B9 ? ? ? ? 50 68");
    injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CB692
    pattern = hook::pattern("B8 ? ? ? ? 3B C8 74 2B 6A 10 68");
    injector::WriteMemory(pattern.get_first(1), Screen.Width, true); //0x4CB59C + 1
    injector::WriteMemory<uint8_t>(pattern.get_first(10), 32, true); //0x4CB5A5+1
    injector::WriteMemory(pattern.get_first(12), Screen.Height, true); //0x4CB5A7 + 1


    pattern = hook::pattern("B9 2F 00 00 00 F3 A5 5F 8B CD"); //0x4A80CD
    struct CameraZoom
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x2F;
            if (bFixHud)
                *(int32_t*)(regs.ebp + 0x138) = Screen.nHudScale;
            if (Screen.fCameraZoom)
                *(int32_t*)(regs.esi + 0x8) *= Screen.fCameraZoom;
        }
    }; injector::MakeInline<CameraZoom>(pattern.get_first(0));


    if (bFixHud)
    {
        dword_6733F0 = *hook::pattern("68 ? ? ? ? 52 C7 05").get_first<uintptr_t>(1);

        pattern = hook::pattern("FF 15 ? ? ? ? 5F 5E 5B 8B E5 5D C2");
        off_5952C4 = *pattern.count(5).get(3).get<uintptr_t>(2);

        injector::MakeNOP(pattern.count(5).get(3).get<uintptr_t>(0), 6, true);
        injector::MakeCALL(pattern.count(5).get(3).get<uintptr_t>(0), gbh_DrawQuad, true); //0x4CBD9A

        injector::MakeNOP(pattern.count(5).get(4).get<uintptr_t>(0), 6, true);
        injector::MakeCALL(pattern.count(5).get(4).get<uintptr_t>(0), gbh_DrawQuad, true); //0x4CC0B1

        pattern = hook::pattern("FF 15 ? ? ? ? 8D 4C 24 10 51 8D 4C 24 7C");
        injector::MakeNOP(pattern.get_first(0), 6, true);
        injector::MakeCALL(pattern.get_first(0), gbh_DrawQuad, true); //0x4CC546

        dword_4C834B = (uintptr_t)hook::pattern("8B 46 20 48 83 F8 04").get_first(0);
        dword_4C74EA = (uintptr_t)hook::pattern("6A 06 E8 ? ? ? ? 5E 59 C3").get_first(7);

        //injector::MakeCALL(0x453590 + 0x175, test, true);
        //injector::MakeCALL(0x453590 + 0x1C3, test, true);
        //injector::MakeCALL(0x457920 + 0x9CE, test, true);
        //injector::MakeCALL(0x4B92B0 + 0x174, test, true);
        //injector::MakeCALL(0x4C71B0 + 0x5D, test, true);
        //injector::MakeCALL(0x4C74A0 + 0x45, test, true); // player arrow
        //injector::MakeCALL(0x4C82C0 + 0x86, test, true); // mission arrrows
        //injector::MakeCALL(0x4C82C0 + 0xED, test, true);
        //void __declspec(naked) test()
        //{
        //    _asm ret 0x2C
        //}
    }

    if (bFixMenu)
    {
        pattern = hook::pattern("C1 E1 04 03 C8 6A 00 6A 00"); //0x4A80CD
        struct MenuHook1
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)(regs.esp + 0x08) += Screen.fHudOffset;
                regs.ecx = (regs.ecx << 4) + regs.eax;
            }
        }; injector::MakeInline<MenuHook1>(pattern.get_first(0));

        pattern = hook::pattern("C1 E2 04 03 D0"); //4530FB
        struct MenuHook2
        {
            void operator()(injector::reg_pack& regs)
            {
                *(int32_t*)(regs.esp + 0x08) += Screen.fHudOffset;              
                regs.edx = (regs.edx << 4) + regs.eax;
            }
        }; injector::MakeInline<MenuHook2>(pattern.get_first(0));
    }

    if (bEndProcess)
    {
        pattern = hook::pattern("8B 15 ? ? ? ? 6A 06 52 8B 08 50"); //0x4B4FB8
        auto hwnd = *pattern.get_first<HWND*>(2);
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);
    }

    if (bSkipMovie)
    {
        //skip movie to prevent windowed crash
        pattern = hook::pattern("83 E7 F8 83 C7 08 EB 02");
        injector::WriteMemory<uint8_t>(pattern.get_first(5), 0, true); //0x4D145C + 2
        pattern = hook::pattern("6A 08 E8 ? ? ? ? 5E C3");
        injector::WriteMemory<uint8_t>(pattern.get_first(1), 0, true); //0x45969C + 1
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