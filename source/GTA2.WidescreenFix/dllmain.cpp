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
    float fMenuOffset;
    float fCameraZoom;
} Screen;

bool bEndProcess;
int32_t nQuicksaveKey;
int32_t nZoom = 0;
WNDPROC wndProcOld = NULL;
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == VK_ADD && nZoom < 200000)
            nZoom += 4000;
        else if (wParam == VK_SUBTRACT && nZoom > 0)
            nZoom -= 4000;
        break;
    case WM_CLOSE:
        if (bEndProcess)
            ExitProcess(0);
        break;
    }

    return CallWindowProc(wndProcOld, hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI WindowCheck(LPVOID hWnd)
{
    while (*(HWND*)hWnd == NULL)
        Sleep(10);

    wndProcOld = (WNDPROC)GetWindowLong(*(HWND*)hWnd, GWL_WNDPROC);
    SetWindowLong(*(HWND*)hWnd, GWL_WNDPROC, (LONG)WndProc);
    return 0;
}

uintptr_t off_5952C4, esp40, esp44, esp68;
uintptr_t dword_6733F0, dword_4C834B, dword_4C74EA, dword_4582F3, dword_458426, dword_4C83B2;
uintptr_t dword_45379E, dword_453A22, dword_4580C6, dword_4567E1;
uintptr_t gbh_DrawQuadAddr;
void __declspec(naked) gbh_DrawQuad()
{
    _asm mov ecx, [esp + 0x40]
    _asm mov esp40, ecx
    _asm mov ecx, [esp + 0x44]
    _asm mov esp44, ecx
    _asm mov ecx, [esp + 0x68]
    _asm mov esp68, ecx

    if (esp68 == dword_45379E || esp68 == dword_453A22 || esp68 == dword_4580C6 || esp68 == dword_4567E1 || esp68 == dword_458426 || esp40 == dword_4582F3)
    {
        *(float*)(dword_6733F0 + 0x00) += Screen.fMenuOffset;
        *(float*)(dword_6733F0 + 0x20) += Screen.fMenuOffset;
        *(float*)(dword_6733F0 + 0x40) += Screen.fMenuOffset;
        *(float*)(dword_6733F0 + 0x60) += Screen.fMenuOffset;
    }
    else
    {
        if (esp40 != dword_4C834B && esp44 != dword_4C74EA && esp40 != dword_4C83B2)
        {
            *(float*)(dword_6733F0 + 0x00) += Screen.fHudOffset;
            *(float*)(dword_6733F0 + 0x20) += Screen.fHudOffset;
            *(float*)(dword_6733F0 + 0x40) += Screen.fHudOffset;
            *(float*)(dword_6733F0 + 0x60) += Screen.fHudOffset;
        }
    }

    gbh_DrawQuadAddr = *(uintptr_t*)off_5952C4;
    _asm jmp gbh_DrawQuadAddr;
}

uintptr_t esp00;
uintptr_t off_59533C;
uintptr_t gbh_BlitImageAddr;
void __declspec(naked) gbh_BlitImage()
{
    _asm mov esp00, esp

    //*(int32_t*)(esp00 + 0x04) += Screen.fHudOffset; //int imageIndex
    //*(int32_t*)(esp00 + 0x08) += Screen.fHudOffset; //int srcLeft
    //*(int32_t*)(esp00 + 0x0C) += Screen.fHudOffset; //int srcTop
    //*(int32_t*)(esp00 + 0x10) += Screen.fHudOffset; //int srcRight
    //*(int32_t*)(esp00 + 0x14) += Screen.fHudOffset; //int srcBottom
    *(int32_t*)(esp00 + 0x18) += (int32_t)Screen.fMenuOffset; //int dstX
    //*(int32_t*)(esp00 + 0x1C) += Screen.fHudOffset; //int dstY

    gbh_BlitImageAddr = *(uintptr_t*)off_59533C;
    _asm jmp gbh_BlitImageAddr;
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
    bEndProcess = iniReader.ReadInteger("MAIN", "EndProcessOnWindowClose", 0) != 0;
    auto szCameraZoom = iniReader.ReadString("MAIN", "CameraZoomFactor", "auto");
    auto bSkipMovie = iniReader.ReadInteger("MAIN", "SkipMovie", 1) != 0;
    static auto bFixHud = iniReader.ReadInteger("MAIN", "FixHud", 1) != 0;
    auto bFixMenu = iniReader.ReadInteger("MAIN", "FixMenu", 1) != 0;
    nQuicksaveKey = iniReader.ReadInteger("MAIN", "QuicksaveKey", VK_F5);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.Width43 = static_cast<int32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fHudScale = ((16384.0f * (Screen.fWidth / 640.0f)) / (Screen.fAspectRatio / (4.0f / 3.0f)));
    Screen.nHudScale = static_cast<int32_t>(Screen.fHudScale);
    Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fMenuOffset = (Screen.fWidth - 480.0f * (4.0f / 3.0f)) / 2.0f;
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
    pattern = hook::pattern("6A ? 50 51 52 32 DB"); //0x4CB583
    //injector::WriteMemory<uint8_t>(pattern.get_first(1), 32, true); // causes green menu
    injector::MakeNOP(pattern.get_first(32), 2, true); //0x4CB5A3

    pattern = hook::pattern("B9 2F 00 00 00 F3 A5"); //0x4A80CD 0x4A6257
    struct CameraZoom
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x2F;

            if (bFixHud)
                *(int32_t*)(regs.ebp + 0x138) = Screen.nHudScale;

            if (Screen.fCameraZoom)
            {
                *(int32_t*)(regs.esi + 0x8) *= (int32_t)Screen.fCameraZoom;
                *(int32_t*)(regs.esi + 0x8) += nZoom;
            }
        }
    }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

    if (bFixHud)
    {
        pattern = hook::pattern("8B 0D ? ? ? ? 53 8B 5D 08 56 57"); //4CBA59
        static auto dword_670684 = *pattern.get_first<uint32_t*>(2);
        struct HUDHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.ecx = *dword_670684;
                *(int32_t*)(regs.esi + 0x00) = Screen.nHudScale;
                //*(int32_t*)(regs.esi - 0xBC) = Screen.nHudScale;
            }
        }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(6));

        dword_6733F0 = *hook::pattern("68 ? ? ? ? 52 C7 05").get_first<uintptr_t>(1);

        pattern = hook::pattern("83 3D ? ? ? ? 00 75 ? 68 ? ? ? ? 68 ? ? ? ? 8D 85 B0 FC FF FF");
        off_5952C4 = *pattern.count(1).get(0).get<uintptr_t>(2);

        pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(off_5952C4)));
        for (size_t i = 3; i < pattern.size(); ++i) //0x4CBD9A 0x4CC0B1 0x4CC546
        { 
            injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 6, true);
            injector::MakeCALL(pattern.get(i).get<uint32_t>(0), gbh_DrawQuad, true);
        }

        dword_4C834B = (uintptr_t)hook::pattern("8B 46 20 48 83 F8 04").get_first(0);
        dword_4C83B2 = (uintptr_t)hook::pattern("5F 5E 83 C4 08 C3").count(10).get(9).get<uintptr_t>(0);
        dword_4C74EA = (uintptr_t)hook::pattern("6A 06 E8 ? ? ? ? 5E 59 C3").get_first(7);
        dword_4582F3 = (uintptr_t)hook::pattern("E9 ? ? ? ? 66 8B 5D 6A").get_first(0);
        dword_458426 = (uintptr_t)hook::pattern("8B 44 24 18 40 66 3B 47 02").get_first(0);

        dword_45379E = (uintptr_t)hook::pattern("8B 44 24 1C 8B 54 24 20 40").get_first(0);
        dword_453A22 = (uintptr_t)hook::pattern("C7 ? ? ? 02 00 00 00 E8 ? ? ? ? 59 C2 14 00").get_first(13);
        dword_4580C6 = (uintptr_t)hook::pattern("8B 4C 24 18 41 66 3B 0F").get_first(0);
        dword_4567E1 = (uintptr_t)hook::pattern("8B 44 24 30 8B 4C 24 18 03 F8").get_first(0);

        //menu (ret 24h)
        //injector::MakeCALL(0x453590 + 0x209, test, true); //credits
        //injector::MakeCALL(0x4539F0 + 0x2D , test, true); //main menu text
        //injector::MakeCALL(0x4566C0 + 0x11C, test, true); //red text
        //injector::MakeCALL(0x4568C0 + 0x157, test, true);
        //injector::MakeCALL(0x456FB0 + 0xF7 , test, true);
        //injector::MakeCALL(0x4580C1 + 0x00 , test, true); //main menu greyed out text
        //injector::MakeCALL(0x457920 + 0xB01, test, true); // player quit text
        //injector::MakeCALL(0x4BA2C0 + 0x57 , test, true);
        //injector::MakeCALL(0x4C7280 + 0x56 , test, true); //ingame text when press esc

        //injector::MakeCALL(0x453590 + 0x175, test, true);
        //injector::MakeCALL(0x453590 + 0x1C3, test, true);
        //injector::MakeCALL(0x457920 + 0x9CE, test, true); //red circle
        //injector::MakeCALL(0x4B92B0 + 0x174, test, true);
        //injector::MakeCALL(0x4C71B0 + 0x5D, test, true);
        //injector::MakeCALL(0x4C74A0 + 0x45, test, true); // player arrow
        //injector::MakeCALL(0x4C82C0 + 0x86, test, true); // mission arrrows
        //injector::MakeCALL(0x4C82C0 + 0xED, test, true); // small arrow when near mission
        //void __declspec(naked) test()
        //{
        //    _asm ret 0x2C
        //}
    }

    if (bFixMenu)
    {
        pattern = hook::pattern("FF 15 ? ? ? ? 83 F8 F6 ? ? 66 0F B6 C3");
        off_59533C = *pattern.count(1).get(0).get<uintptr_t>(2);

        pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(off_59533C)));
        for (size_t i = 0; i < pattern.size(); ++i)
        {
            injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 6, true);
            injector::MakeCALL(pattern.get(i).get<uint32_t>(0), gbh_BlitImage, true);
        }
    }

    pattern = hook::pattern("8B 15 ? ? ? ? 6A 06 52 8B 08 50"); //0x4B4FB8
    auto hwnd = *pattern.get_first<HWND*>(2);
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);

    if (bSkipMovie)
    {
        //skip movie to prevent windowed crash
        pattern = hook::pattern("8A 88 ? ? ? ? 88 88 ? ? ? ? 40 84 C9 ? ? B8 ? ? ? ? C3");
        injector::WriteMemory<uint8_t>(*pattern.get_first<void*>(2), '_', true); //0x459695+2
    }

    if (nQuicksaveKey)
    {
        //code from NTAuthority
        static uint16_t oldState = 0;
        static uint16_t curState = 0;

        //injector::WriteMemory(0x47FEDC, 0, true);
        //injector::WriteMemory(0x47FEF5, 500, true);

        static uint32_t dword_45E510 = (uint32_t)hook::get_pattern("8D 81 00 03 00 00 C3", 0);
        static uint32_t dword_5EC070 = *(uint32_t*)hook::get_pattern("B9 ? ? ? ? E8 ? ? ? ? 66 0F B6", 1);
        static uint32_t dword_47EF40 = (uint32_t)hook::get_pattern("83 EC 18 53 8B 5C 24 20 55 8B", 0);
        static uint32_t dword_6644BC = *(uint32_t*)hook::get_pattern("8B 15 ? ? ? ? 8B 82 38 03 00", 2);
        static uint32_t dword_4C6750 = (uint32_t)hook::get_pattern("8B 44 24 08 8B 54 24 04 6A FF 50 52", 0);
        static uint32_t dword_672F40 = *(uint32_t*)hook::get_pattern("8B 0D ? ? ? ? 56 68 ? ? ? ? 6A 01", 2);

        pattern = hook::pattern("8B 73 04 33 FF 3B F7 66 89 BB E8"); //0x481380
        struct QuicksaveHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = *(uint32_t*)(regs.ebx + 4);
                regs.edi = 0;

                curState = GetAsyncKeyState(nQuicksaveKey);

                if (!curState && oldState) 
                {
                    uint32_t missionFlag = **(uint32_t**)(*(uint32_t*)(dword_6644BC) + 0x344);
                    if (!missionFlag)
                    {
                        //injector::thiscall<int(int, int)>::call(0x4105B0, 0x5D85A0, 0x3D); //sfx

                        auto i = injector::thiscall<uint32_t(uint32_t)>::call(dword_45E510, dword_5EC070); //save
                        injector::thiscall<uint32_t(uint32_t, uint32_t)>::call(dword_47EF40, *(uint32_t*)dword_6644BC, i);

                        injector::thiscall<uint32_t(uint32_t, uint32_t, char*)>::call(dword_4C6750, *(uintptr_t*)dword_672F40 + 0xE4, 1, "svdone"); // text display
                    }
                }

                oldState = curState;
            }
        }; injector::MakeInline<QuicksaveHook>(pattern.get_first(0));
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