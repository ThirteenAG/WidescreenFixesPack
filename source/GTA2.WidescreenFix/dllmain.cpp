#include "stdafx.h"

uint32_t* window_width;
uint32_t* window_height;

#define default_screen_width (640.0f)
#define default_screen_height (480.0f)
#define default_aspect_ratio (default_screen_width / default_screen_height)

#define screen_width ((float)*window_width)
#define screen_height ((float)*window_height)
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
float fZoom;
WNDPROC wndProcOld = NULL;

LRESULT APIENTRY WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_KEYDOWN:
        if (wParam == nZoomIncreaseKey) {
            fZoom += 1.0f;
        }
        else if (wParam == nZoomDecreaseKey) {
            fZoom -= 1.0f;
        }

        fZoom = max(0.0f, min(fZoom, 10.0f));
        break;
    case WM_KEYUP:
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

void RePositionElement(void* addr) {
    static injector::hook_back<int* (__fastcall*)(int*, int, int*, int*)> rePositionElement;
    auto f = [](int* in, int, int* out, int* scale) {
        out = rePositionElement.fun(in, 0, out, scale);

        float x = *out / fone;
        if (posType == 0) {
            // Shift this element approximatively.
            if (bExtendHud) {
                float f = screen_height / default_screen_height;
                if (x <= 140.0f * f) {
                }
                else if (x > 140.0f * f && x < 490.0f * f) {
                    x += hud_offset / 2;
                }
                else if (x >= 490.0f * f) {
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
    };
    rePositionElement.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (int*(__fastcall*)(int*, int, int*, int*))f);
}

void Rescale(int& scale) {
    float fs = (scale / fone) * hud_scale;
    scale = (int32_t)(fs * one);
}

void Repos(int& x, int& y) {
    float fx = (x / fone) * hud_scale;
    float fy = (y / fone) * hud_scale;
    fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

void Repos3d(int& x, int& y) {
    float fx = (x / fone) * hud_scale;
    float fy = (y / fone);
    //fx += ((int32_t)(hud_offset / 2));

    x = (int32_t)(fx * one);
    y = (int32_t)(fy * one);
}

template<int te>
void SetPosType(void* addr)
{
    static injector::hook_back<int* (__fastcall*)(int*, int, int)> encodeFlt;
    auto f = [](int* a, int, int b) {
        encodeFlt.fun(a, 0, b);
        posType = te;
    };
    encodeFlt.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__fastcall*)(int*, int, int))f);
}

void ReposAndScaleFontCall(void* addr)
{
    static injector::hook_back<void(__stdcall*)(const wchar_t*, int, int, int, int, const int*, int, bool, int)> printString;
    auto f = [](const wchar_t* str, int x, int y, int style, int scale, const int* mode, int palette, bool enableAlpha, int alpha) {
        Repos(x, y);
        Rescale(scale);
        printString.fun(str, x, y, style, scale, mode, palette, enableAlpha, alpha);
    };
    printString.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(const wchar_t*, int, int, int, int, const int*, int, bool, int))f);
}

void ReposAndScaleSpriteCall(void* addr)
{
    static injector::hook_back<void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int)> drawSprite;
    auto f = [](int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
        Repos(x, y);
        Rescale(scale);
        drawSprite.fun(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
    };
    drawSprite.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int))f);
}

void ReposSpriteCall3D(void* addr)
{
    static injector::hook_back<void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int)> drawSprite;
    auto f = [](int id1, int id2, int x, int y, int angle, int scale, const int* mode, int enableAlpha, int alpha, int a10, int lightFlag) {
        Repos3d(x, y);
        drawSprite.fun(id1, id2, x, y, angle, scale, mode, enableAlpha, alpha, a10, lightFlag);
    };
    drawSprite.fun = injector::GetBranchDestination(addr).get();
    injector::MakeCALL(addr, (void(__stdcall*)(int, int, int, int, int, int, const int*, int, int, int, int))f);
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
    
    window_width = *hook::pattern("8B 3D ? ? ? ? 8B 4C 24 1C").get_first<uint32_t*>(2);
    window_height = *hook::pattern("8B 0D ? ? ? ? 2B F8").get_first<uint32_t*>(2);

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
            *(int32_t*)(regs.esi + 0x8) += (uint32_t)(fZoom * one);
        }
    }; injector::MakeInline<CameraZoom>(pattern.count(2).get(1).get<void*>(0));

    // Hud
    pattern = hook::pattern("E8 ? ? ? ? 2B FB");
    RePositionElement(pattern.get_first(-5962)); // 0x4C72AA

    pattern = hook::pattern("E8 ? ? ? ? 8B 76 54");
    RePositionElement(pattern.get_first(-3233)); // 0x4BA2F4

    pattern = hook::pattern("E8 ? ? ? ? 81 3B ? ? ? ?");
    RePositionElement(pattern.get_first(-7509)); // 0x4C71DA

    pattern = hook::pattern("E8 ? ? ? ? C6 44 24 ? ? EB 7C");
    RePositionElement(pattern.get_first(-359)); // 0x44B4BA

    pattern = hook::pattern("E8 ? ? ? ? 81 C6 ? ? ? ? 56");
    SetPosType<1>(pattern.get_first(0)); // Big messages 0x4C8A7D

    pattern = hook::pattern("E8 ? ? ? ? 66 0F B6 86 ? ? ? ?");
    SetPosType<1>(pattern.get_first(0)); // Subtitle sprite 0x4C946F

    pattern = hook::pattern("6A 40 E8 ? ? ? ? 56");
    SetPosType<1>(pattern.get_first(2)); // Subtitle text 0x4C94D7

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
    SetPosType<1>(pattern.get_first(450950)); // Quit text 1 0x4C87A5

    pattern = hook::pattern("E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0B 8B 0D ? ? ? ? E8 ? ? ? ? FF 15 ? ? ? ?");
    SetPosType<1>(pattern.get_first(451046)); // Quit text 2 0x4C8805

    pattern = hook::pattern("50 E8 ? ? ? ? 57 E8 ? ? ? ? 5F");
    SetPosType<1>(pattern.get_first(1)); // Quit text 3 0x4C8867

    pattern = hook::pattern("E8 ? ? ? ? 56 53");
    SetPosType<1>(pattern.get_first(0)); // Stats sprite 0x4CA1BC

    pattern = hook::pattern("57 E8 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 5F");
    SetPosType<1>(pattern.get_first(1)); // Stats text 0x4CA40C

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
    SetPosType<2>(pattern.get_first(-14347)); // 3d Text 0x4BAD41

    pattern = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? 8B CB E8 ? ? ? ? 5D");
    SetPosType<2>(pattern.get_first(-14202)); // 3d Text 0x4BADD2

    pattern = hook::pattern("E8 ? ? ? ? A1 ? ? ? ? 89 5C 24 18");
    SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAE7B

    pattern = hook::pattern("E8 ? ? ? ? 8B 15 ? ? ? ? 8B 44 24 1C");
    SetPosType<2>(pattern.get_first(0)); // 3d Text 0x4BAF3D

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2986)); // Zone name 0x4C98F0

    pattern = hook::pattern("55 E8 ? ? ? ? 68 ? ? ? ?");
    SetPosType<1>(pattern.get_first(1)); // Zone name 0x4C9933

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2844)); // Zone name 0x4C997E

    pattern = hook::pattern("E8 ? ? ? ? 8B CE E8 ? ? ? ? 8D 8E ? ? ? ? E8 ? ? ? ? 8D 8E ? ? ? ?");
    SetPosType<1>(pattern.get_first(-2774)); // Zone name 0x4C99C4

    pattern = hook::pattern("E8 ? ? ? ? 03 F5 56");
    SetPosType<1>(pattern.get_first(0)); // Zone name 0x4C9A29

    pattern = hook::pattern("E8 ? ? ? ? 8B 46 04 6A 00");
    ReposSpriteCall3D(pattern.get_first(-495)); // Money messages 0x4B9424

    pattern = hook::pattern("74 7B 68 ? ? ? ?");
    injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93AC

    pattern = hook::pattern("74 4D 8D 4C 24 14");
    injector::MakeNOP(pattern.get_first(0), 2, true); // 0x4B93DA

    // Frontend
    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 1C 8B 54 24 20");
    ReposAndScaleFontCall(pattern.get_first(0)); // 0x453799

    pattern = hook::pattern("E8 ? ? ? ? FE 44 24 11");
    ReposAndScaleFontCall(pattern.get_first(-9210)); // 0x453A1D

    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 30 8B 4C 24 18");
    ReposAndScaleFontCall(pattern.get_first(0)); // 0x4567DC
    
    pattern = hook::pattern("E8 ? ? ? ? 66 A1 ? ? ? ? 68 ? ? ? ?");
    ReposAndScaleFontCall(pattern.get_first(-4085)); // 0x456A17
    
    pattern = hook::pattern("E8 ? ? ? ? 8B 7C 24 1C 8A 44 24 30");
    ReposAndScaleFontCall(pattern.get_first(0)); // 0x4570A7
    
    pattern = hook::pattern("EB 7E 66 8B 6D 6C");
    ReposAndScaleFontCall(pattern.get_first(128)); // 0x4580C1
    
    pattern = hook::pattern("E8 ? ? ? ? 8B 44 24 18 40");
    ReposAndScaleFontCall(pattern.get_first(0)); // 0x458421
    
    pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 68 ? ? ? ? 57");
    ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x453705

    pattern = hook::pattern(" E8 ? ? ? ? EB 44 8B 44 24 10");
    ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x453753

    pattern = hook::pattern("E8 ? ? ? ? E9 ? ? ? ? 66 8B 5D 6A");
    ReposAndScaleSpriteCall(pattern.get_first(0)); // 0x4582EE

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
    pattern = hook::pattern("74 66 E8 ? ? ? ?");
    injector::MakeNOP(pattern.get_first(0), 2, true);

    static int mode = 0;
    pattern = hook::pattern("A1 ? ? ? ? 8B 0D ? ? ? ? EB 06"); // 0x4CAEF2
    struct ClearScreenHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = *window_width;
            regs.ecx = *window_height;
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

void InitD3DDLL() 
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"d3ddll"), "89 7C 24 28 89 74 24 2C");
    struct gbh_BlitImageHook
    {
        void operator()(injector::reg_pack& regs)
        {
            RECT& dest = (*(RECT*)(regs.esp + 0x30 - 0x10));
            dest.left = regs.ebp * (int32_t)hud_scale;
            dest.top = regs.ebx * (int32_t)hud_scale;
            dest.right = regs.edi * (int32_t)hud_scale;
            dest.bottom = regs.esi * (int32_t)hud_scale;

            dest.left += (int32_t)(hud_offset / 2);
            dest.right += (int32_t)(hud_offset / 2);
        }
    }; injector::MakeInline<gbh_BlitImageHook>(pattern.get_first(0), pattern.get_first(8));
}

void InitBinkw32()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"binkw32"), "8B 7D 1C 83 BE ? ? ? ? ?"); // 0x10007B49
    struct BinkCopyToBufferHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.ebp + 0x18) += (int32_t)((screen_width / 2) - default_screen_width / 2);
            *(uint32_t*)(regs.ebp + 0x1C) += (int32_t)((screen_height / 2) - default_screen_height / 2);
            //regs.ebx * (int32_t)hud_scale;

            _asm { cmp     dword ptr[esi + 0x1D8], 0x0FFFFFFFF };
        }
    }; injector::MakeInline<BinkCopyToBufferHook>(pattern.get_first(3), pattern.get_first(10));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("83 EC 68 55 56 8B 74 24 74"));
        CallbackHandler::RegisterCallback(L"d3dim.dll", InitD3DDim);       // crash fix for
        CallbackHandler::RegisterCallback(L"d3dim700.dll", InitD3DDim700); // resolutions > 2048
        CallbackHandler::RegisterCallback(L"d3ddll.dll", InitD3DDLL); // frontend background scale
        CallbackHandler::RegisterCallback(L"binkw32.dll", InitBinkw32); // intro video pos
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