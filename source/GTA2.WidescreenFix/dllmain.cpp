#include "stdafx.h"

int& window_width = *(int*)0x673578;
int& window_height = *(int*)0x6732E8;

#define default_screen_width (640.0f)
#define default_screen_height (480.0f)
#define default_aspect_ratio (default_screen_width / default_screen_height)

#define screen_width ((float)window_width)
#define screen_height ((float)window_height)
#define screen_aspect_ratio (screen_width / screen_height)

#define fone (16384.0f)
#define one (16384)
#define camera_scale ((screen_width / default_screen_width) / (default_aspect_ratio / screen_aspect_ratio))
#define hud_scale (screen_height / default_screen_height)
#define default_hud_scale (screen_width / default_screen_width)

#define hud_offset (screen_width - screen_height * default_aspect_ratio)

int nResX;
int nResY;
bool bExtendHud;
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

int posType = 0; // 0 = default, 1 = centered, 2 = unscaled
injector::hook_back<int*(__fastcall*)(int*, int, int*, int*)> hbRePositionElement;
int* __fastcall RePositionElement(int* in, int, int* out, int* scale) {
    out = hbRePositionElement.fun(in, 0, out, scale);

    float x = *out / one;
    if (posType == 0) {
        // Shift this element approximatively.
        if (bExtendHud) {
            if (x <= screen_width * 0.15f) {
            }
            else if (x > screen_width * 0.15f && x < screen_width * 0.585f) {
                x += hud_offset / 2;
            }
            else if (x >= screen_width * 0.585f) {
                x += hud_offset;
            }
        }
        else
            goto centered;
    }
    else if (posType == 1) {
centered:
        x += (hud_offset / 2);
    }
    else if (posType == 2) {
        x /= hud_scale;
        x *= default_hud_scale;
    }
    posType = 0;

    *out = (uint32_t)(x * one);
    return out;
}

void Rescale(int& x, int& y, int& scale) {
    float fx = (x / fone) * hud_scale;
    float fy = (y / fone) * hud_scale;
    float fs = (scale / fone) * hud_scale;
    fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
    scale = (int32_t)(fs * one);
}

template<uintptr_t addr>
void SetPosType(int te)
{
    using func_hook = injector::function_hooker_thiscall<addr, void(int*, int)>;
    injector::make_static_hook<func_hook>([=](typename func_hook::func_type encodeFlt, int* a, int b) {
        encodeFlt(a, b);
        posType = te;
    });
}

template<uintptr_t addr>
void ScaleFontCall()
{
    using func_hook = injector::function_hooker_stdcall<addr, void(const wchar_t*, int, int, int, int, const int*, int, bool, int)>;
    injector::make_static_hook<func_hook>([=](typename func_hook::func_type printString, const wchar_t* str, int x, int y, int style, int scale, const int* mode, int palette, bool enableAlpha, int alpha) {
    Rescale(x, y, scale);
    printString(str, x, y, style, scale, mode, palette, enableAlpha, alpha);
        });
}

template<uintptr_t addr>
void ScaleSpriteCall()
{
    using func_hook = injector::function_hooker_stdcall<addr, void(int, int, int, int, int, int, const int*, int, int, int, int)>;
    injector::make_static_hook<func_hook>([=](typename func_hook::func_type drawSprite, int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
    Rescale(x, y, scale);
    drawSprite(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
        });
}

void Init()
{
    SetProcessDPIAware();

    CIniReader iniReader("");
    nResX = iniReader.ReadInteger("MAIN", "ResX", 0);
    nResY = iniReader.ReadInteger("MAIN", "ResY", 0);
    bExtendHud = iniReader.ReadBoolean("MAIN", "ExtendHud", 0);

    auto bSkipMovie = iniReader.ReadInteger("MISC", "SkipMovie", 1) != 0;
    auto bSkipCredits = iniReader.ReadInteger("MISC", "SkipCredits", 1) != 0;
    auto bNoSampManDelay = iniReader.ReadInteger("MISC", "NoSampManDelay", 1) != 0;

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

            *(int32_t*)(regs.ebp + 0x138) = (uint32_t)(hud_scale * one);
            *(int32_t*)(regs.ebp + 0x2B0) = (uint32_t)(hud_scale * one); // for Zaibatsu [It was an Accident!] mission

            *(int32_t*)(regs.esi + 0x8) += (uint32_t)((camera_scale - 1.0f) * one);
            *(int32_t*)(regs.esi + 0x8) += nZoom;

            *(int32_t*)(regs.esi + 0x8) = min(*(int32_t*)(regs.esi + 0x8), one * 25);
        }
    }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

    // Hud
    hbRePositionElement.fun = injector::MakeCALL(0x4C72AA, RePositionElement).get();
    injector::MakeCALL(0x4C72AA, RePositionElement);
    
    hbRePositionElement.fun = injector::MakeCALL(0x4BA2F4, RePositionElement).get();
    injector::MakeCALL(0x4BA2F4, RePositionElement);
    
    hbRePositionElement.fun = injector::MakeCALL(0x4C71DA, RePositionElement).get();
    injector::MakeCALL(0x4C71DA, RePositionElement);
    
    hbRePositionElement.fun = injector::MakeCALL(0x44B4BA, RePositionElement).get();
    injector::MakeCALL(0x44B4BA, RePositionElement);
    
    SetPosType<(0x4C8A7D)>(1); // Big messages
    SetPosType<(0x4C946F)>(1); // Subtitle sprite
    SetPosType<(0x4C94D7)>(1); // Subtitle text
    SetPosType<(0x4C87A5)>(1); // Quit text 1
    SetPosType<(0x4C8805)>(1); // Quit text 2
    SetPosType<(0x4C8867)>(1); // Quit text 3
    SetPosType<(0x4CA1BC)>(1); // Stats sprite
    SetPosType<(0x4CA40C)>(1); // Stats text

    SetPosType<(0x4BAD41)>(2); // 3d Text
    SetPosType<(0x4BADD2)>(2); // 3d Text
    SetPosType<(0x4BAE7B)>(2); // 3d Text
    SetPosType<(0x4BAF3D)>(2); // 3d Text

    SetPosType<(0x4C98F0)>(1); // Zone name
    SetPosType<(0x4C9933)>(1); // Zone name
    SetPosType<(0x4C997E)>(1); // Zone name
    SetPosType<(0x4C99C4)>(1); // Zone name
    SetPosType<(0x4C9A29)>(1); // Zone name

    // Frontend
    ScaleFontCall<(0x453799)>();
    ScaleFontCall<(0x453A1D)>();
    ScaleFontCall<(0x4567DC)>();
    ScaleFontCall<(0x456A17)>();
    ScaleFontCall<(0x4570A7)>();
    ScaleFontCall<(0x4580C1)>();
    ScaleFontCall<(0x458421)>();

    ScaleSpriteCall<(0x453705)>();
    ScaleSpriteCall<(0x453753)>();
    ScaleSpriteCall<(0x4582EE)>();

    // Frontend
    //pattern = hook::pattern("8B 4C 24 18 8B 54 24 14"); // 0x4539F1
    //struct PrintStringHook
    //{
    //    void operator()(injector::reg_pack& regs)
    //    {
    //        int32_t& nx = *(int32_t*)(regs.esp + 0x4 + 0x8);
    //        int32_t& ny = *(int32_t*)(regs.esp + 0x4 + 0xC);
    //        int32_t& ns = *(int32_t*)(regs.esp + 0x4 + 0x14);
    //
    //        float fx = (nx / one) * hud_scale;
    //        float fy = (ny / one) * hud_scale;
    //        float fs = (ns / one) * hud_scale;
    //        fx += ((int32_t)(hud_offset / 2));
    //
    //        regs.ecx = (int32_t)(fs * one);
    //        regs.edx = *(uint32_t*)(regs.esp + 0x4 + 0x10);
    //
    //        nx = (int32_t)fx * one;
    //        ny = (int32_t)fy * one;
    //    }
    //}; injector::MakeInline<PrintStringHook>(pattern.count(11).get(5).get<void*>(0), pattern.count(11).get(5).get<void*>(8));

    pattern = hook::pattern("8B 15 ? ? ? ? 6A 06 52 8B 08 50"); //0x4B4FB8
    auto hwnd = *pattern.get_first<HWND*>(2);
    CreateThreadAutoClose(0, 0, (LPTHREAD_START_ROUTINE)&WindowCheck, (LPVOID)hwnd, 0, NULL);

    if (bSkipMovie)
    {
        //skip movie to prevent windowed crash
        pattern = hook::pattern("8A 88 ? ? ? ? 88 88 ? ? ? ? 40 84 C9 ? ? B8 ? ? ? ? C3");
        injector::WriteMemory<uint8_t>(*pattern.get_first<void*>(2), '_', true); //0x459695+2
    }

    if (bSkipCredits)
    {
        pattern = hook::pattern("66 C7 86 ? ? ? ? ? ? A1 ? ? ? ? 33 FF");
        injector::WriteMemory<uint16_t>(pattern.get_first(7), 258, true); //0x453EFB+7
    }

    if (bNoSampManDelay)
    {
        pattern = hook::pattern("FF D5 6A 01");
        injector::MakeNOP(pattern.get_first(2), 8, true); //0x4B6821+2
    }

    // Intro crash fix
    injector::MakeNOP(0x481E28, 2, true);

    static int mode = 0;
    pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? EB 06"); // 0x4CAEF2
    struct ClearScreenHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = window_width;
            regs.ecx = window_height;
            mode = -2;
        }
    }; injector::MakeInline<ClearScreenHook>(pattern.get_first(0), pattern.get_first(11));

    pattern = hook::pattern("8B 15 ? ? ? ? A1 ? ? ? ? 81 CA ? ? ? ?"); // 0x481E2F
    struct IntroMovieColorHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.edx = 3;
    
            if (mode == -2)
                regs.edx = 1;

            mode = 0;
        }
    }; injector::MakeInline<IntroMovieColorHook>(pattern.get_first(0), pattern.get_first(6));

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

void InitD3DDLL() {
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3ddll"), "89 7C 24 28 89 74 24 2C");
    struct gbh_BlitImageHook
    {
        void operator()(injector::reg_pack& regs)
        {
            RECT& dest = (*(RECT*)(regs.esp + 0x30 - 0x10));
            dest.left = regs.ebp * hud_scale;
            dest.top = regs.ebx * hud_scale;
            dest.right = regs.edi * hud_scale;
            dest.bottom = regs.esi * hud_scale;

            dest.left += (int32_t)(hud_offset / 2);
            dest.right += (int32_t)(hud_offset / 2);
        }
    }; injector::MakeInline<gbh_BlitImageHook>(pattern.get_first(0), pattern.get_first(8));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("83 EC 68 55 56 8B 74 24 74"));
        CallbackHandler::RegisterCallback(L"d3dim.dll", InitD3DDim);       // crash fix for
        CallbackHandler::RegisterCallback(L"d3dim700.dll", InitD3DDim700); // resolutions > 2048
        CallbackHandler::RegisterCallback(L"d3ddll.dll", InitD3DDLL); // frontend background scale
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