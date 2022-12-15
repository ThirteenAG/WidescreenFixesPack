#include "stdafx.h"

int& window_width = *(int*)0x673578;
int& window_height = *(int*)0x6732E8;
int** gGame = (int**)0x5EB4FC;

#define default_window_width (640.0f)
#define default_window_height (480.0f)
#define default_aspect_ratio (default_window_width / default_window_height)

#define aspect_ratio (float)window_width / window_height

#define one (int)(1 << 14)
#define camera_scale (int)(((one * ((float)window_width / default_window_width)) / (default_aspect_ratio / (aspect_ratio))))
#define hud_scale (int)(((one * ((float)window_width / default_window_width)) / (aspect_ratio / (default_aspect_ratio))))
#define hud_offset ((window_width - window_height * (default_aspect_ratio)) / 2.0f);
#define menu_offsetx ((window_width / 2) - default_window_width / 2)
#define menu_offsety  ((window_height / 2) - default_window_height / 2)

int nResX;
int nResY;
bool bEndProcess;
int32_t nQuicksaveKey;
int32_t nZoomIncreaseKey;
int32_t nZoomDecreaseKey;
int32_t nZoom;
WNDPROC wndProcOld = NULL;
bool keyPressed = false;
LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (!keyPressed) {
            if (wParam == nZoomIncreaseKey)
                nZoom += one;
            else if (wParam == nZoomDecreaseKey)
                nZoom -= one;

            keyPressed = true;
        }

        nZoom = max(-one, min(nZoom, one * 25));
        break;
    case WM_KEYUP:
        keyPressed = false;
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
    _asm {mov ecx, [esp + 0x40]}
    _asm {mov esp40, ecx}
    _asm {mov ecx, [esp + 0x44]}
    _asm {mov esp44, ecx}
    _asm {mov ecx, [esp + 0x68]}
    _asm {mov esp68, ecx}

    if ((!(*gGame) || esp68 == dword_45379E || esp68 == dword_453A22 || esp68 == dword_4580C6 || esp68 == dword_4567E1 || esp68 == dword_458426 || esp40 == dword_4582F3))
    {
        *(float*)(dword_6733F0 + 0x00) += menu_offsetx;
        *(float*)(dword_6733F0 + 0x04) += menu_offsety;

        *(float*)(dword_6733F0 + 0x20) += menu_offsetx;
        *(float*)(dword_6733F0 + 0x24) += menu_offsety;

        *(float*)(dword_6733F0 + 0x40) += menu_offsetx;
        *(float*)(dword_6733F0 + 0x44) += menu_offsety;

        *(float*)(dword_6733F0 + 0x60) += menu_offsetx;
        *(float*)(dword_6733F0 + 0x64) += menu_offsety;
    }
    else if (((*gGame)))
    {
        if (esp44 != dword_4C834B && esp40 != dword_4C74EA && esp44 != dword_4C83B2 && esp40 != dword_4C834B && esp44 != dword_4C74EA && esp40 != dword_4C83B2)
        {
            *(float*)(dword_6733F0 + 0x00) += hud_offset;
            *(float*)(dword_6733F0 + 0x20) += hud_offset;
            *(float*)(dword_6733F0 + 0x40) += hud_offset;
            *(float*)(dword_6733F0 + 0x60) += hud_offset;
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
    *(int32_t*)(esp00 + 0x18) += (int32_t)menu_offsetx; //int dstX
    *(int32_t*)(esp00 + 0x1C) += (int32_t)menu_offsety; //int dstY

    gbh_BlitImageAddr = *(uintptr_t*)off_59533C;
    _asm jmp gbh_BlitImageAddr;
}

void Init()
{
    CIniReader iniReader("");
    nResX = iniReader.ReadInteger("MAIN", "ResX", 0);
    nResY = iniReader.ReadInteger("MAIN", "ResY", 0);

    auto bSkipMovie = iniReader.ReadInteger("MISC", "SkipMovie", 1) != 0;
    bEndProcess = iniReader.ReadInteger("MISC", "EndProcessOnWindowClose", 0) != 0;
    nQuicksaveKey = iniReader.ReadInteger("MISC", "QuicksaveKey", VK_F5);
    nZoomIncreaseKey = iniReader.ReadInteger("MISC", "ZoomIncreaseKey", VK_OEM_PLUS);
    nZoomDecreaseKey = iniReader.ReadInteger("MISC", "ZoomDecreaseKey", VK_OEM_MINUS);

    if (!nResX || !nResY)
        std::tie(nResX, nResY) = GetDesktopRes();

    // Res change
    if (nResX && nResY) {
        auto pattern = hook::pattern("8B 2D ? ? ? ? 56 8B 35 ? ? ? ? 57 8B 3D"); //0x4CB29F
        static auto dword_6732E0 = *pattern.get_first<uint32_t*>(2);
        pattern = hook::pattern("89 0D ? ? ? ? A3 ? ? ? ? 89 0D ? ? ? ? EB 5F"); //0x4CB2D5
        static auto dword_673578 = *pattern.get_first<uint32_t*>(2);
        static auto dword_6732E8 = *pattern.get_first<uint32_t*>(7);
        static auto dword_6732E4 = *pattern.get_first<uint32_t*>(13);
        struct SetResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = nResY;
                *dword_673578 = nResX;
                *dword_6732E8 = nResY;
                *dword_6732E4 = nResX;
                *dword_6732E0 = nResY;
            }
        }; injector::MakeInline<SetResHook>(pattern.get_first(-12), pattern.get_first(17));

        pattern = hook::pattern("74 49 A3 ? ? ? ? ? ? ? ? ? ? ? ? ? ? E8 ? ? ? ? 84 C0");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CC61E
        injector::WriteMemory<uint8_t>(pattern.get_first(24), 0xEB, true); //0x4CC636
        pattern = hook::pattern("74 2B A1 ? ? ? ? B9 ? ? ? ? 50 68");
        injector::WriteMemory<uint8_t>(pattern.get_first(0), 0xEB, true); //0x4CB692
        pattern = hook::pattern("B8 ? ? ? ? 3B C8 74 2B 6A 10 68");
        injector::WriteMemory(pattern.get_first(1), nResX, true); //0x4CB59C + 1
        injector::WriteMemory<uint8_t>(pattern.get_first(10), 32, true); //0x4CB5A5+1
        injector::WriteMemory(pattern.get_first(12), nResY, true); //0x4CB5A7 + 1
        pattern = hook::pattern("6A ? 50 51 52 32 DB"); //0x4CB583
        //injector::WriteMemory<uint8_t>(pattern.get_first(1), 32, true); // causes green menu
        injector::MakeNOP(pattern.get_first(32), 2, true); //0x4CB5A3
    }

    auto pattern = hook::pattern("B9 2F 00 00 00 F3 A5"); //0x4A80CD 0x4A6257
    struct CameraZoom
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = 0x2F;

            *(int32_t*)(regs.ebp + 0x138) = hud_scale;
            *(int32_t*)(regs.ebp + 0x2B0) = hud_scale; // for Zaibatsu [It was an Accident!] mission

            *(int32_t*)(regs.esi + 0x8) += camera_scale - one;
            *(int32_t*)(regs.esi + 0x8) += nZoom;

            *(int32_t*)(regs.esi + 0x8) = min(*(int32_t*)(regs.esi + 0x8), one * 25);
        }
    }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

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

    pattern = hook::pattern("FF 15 ? ? ? ? 83 F8 F6 ? ? 66 0F B6 C3");
    off_59533C = *pattern.count(1).get(0).get<uintptr_t>(2);

    pattern = hook::pattern(pattern_str(0xFF, 0x15, to_bytes(off_59533C)));
    for (size_t i = 0; i < pattern.size(); ++i)
    {
        injector::MakeNOP(pattern.get(i).get<uint32_t>(0), 6, true);
        injector::MakeCALL(pattern.get(i).get<uint32_t>(0), gbh_BlitImage, true);
    }

    pattern = hook::pattern("8B 15 ? ? ? ? 6A 06 52 8B 08 50"); //0x4B4FB8
    auto hwnd = *pattern.get_first<HWND*>(2);
    CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);

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
        static uint32_t dword_673E2C = *(uint32_t*)hook::get_pattern("A1 ? ? ? ? 85 C0 75 ? 8A 41 30", 1);

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
                    uint32_t missionFlag = **(uint32_t**)(*(uint32_t*)(dword_6644BC)+0x344);
                    uint32_t isMP = *(uint32_t*)dword_673E2C;
                    if (!missionFlag && !isMP)
                    {
                        //injector::thiscall<int(int, int)>::call(0x4105B0, 0x5D85A0, 0x3D); //sfx

                        auto i = injector::thiscall<uint32_t(uint32_t)>::call(dword_45E510, dword_5EC070); //save
                        injector::thiscall<uint32_t(uint32_t, uint32_t)>::call(dword_47EF40, *(uint32_t*)dword_6644BC, i);

                        injector::thiscall<uint32_t(uint32_t, uint32_t, const char*)>::call(dword_4C6750, *(uintptr_t*)dword_672F40 + 0xE4, 1, "svdone"); // text display
                    }
                }

                oldState = curState;
            }
        }; injector::MakeInline<QuicksaveHook>(pattern.get_first(0));
    }
}

void InitD3DDim()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim"), "B8 00 08 00 00 39");
    if (!pattern.count_hint(2).empty())
        injector::WriteMemory(pattern.get(0).get<void>(1), -1, true);
}

void InitD3DDim700()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3dim700"), "B8 00 08 00 00 39");
    if (!pattern.count_hint(2).empty())
        injector::WriteMemory(pattern.get(0).get<void>(1), -1, true);
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("83 EC 68 55 56 8B 74 24 74"));
        CallbackHandler::RegisterCallback(L"d3dim.dll", InitD3DDim);       // crash fix for
        CallbackHandler::RegisterCallback(L"d3dim700.dll", InitD3DDim700); // resolutions > 2048
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