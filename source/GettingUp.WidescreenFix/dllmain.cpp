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
    int32_t g_res_w;
    int32_t video_mode;
    bool isMenu;
    int32_t* dword_A8F0C8;
    int32_t* dword_A8F0CC;

} Screen;

HWND g_hWnd;
HWND g_hWndInsertAfter;
UINT g_uFlags;
BOOL WINAPI SetWindowPosHook(HWND hWnd, HWND hWndInsertAfter, int X, int Y, int cx, int cy, UINT uFlags)
{
    g_hWnd = hWnd;
    g_hWndInsertAfter = hWndInsertAfter;
    g_uFlags = uFlags;
    tagRECT rc;
    auto [DesktopResW, DesktopResH] = GetDesktopRes();
    rc.left = (LONG)(((float)DesktopResW / 2.0f) - (Screen.fWidth / 2.0f));
    rc.top = (LONG)(((float)DesktopResH / 2.0f) - (Screen.nHeight / 2.0f));
    rc.right = Screen.nWidth;
    rc.bottom = Screen.nHeight;
    return ::SetWindowPos(hWnd, hWndInsertAfter, rc.left, rc.top, rc.right, rc.bottom, uFlags);
}

static auto GetRes = []()
{
    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.g_res_w = Screen.nWidth;
};

void Init()
{
    CIniReader iniReader(".\\videomode.ini");
    Screen.nWidth = iniReader.ReadInteger("Settings", "Width", 0);
    Screen.nHeight = iniReader.ReadInteger("Settings", "Height", 0);
    bool bFix2D = iniReader.ReadInteger("Settings", "WidescreenUIPatch", 1) != 0;

    if (!Screen.nWidth || !Screen.nHeight)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();

    GetRes();

    //Default Windowed Res Hook
    auto pattern = hook::pattern("FF 15 ? ? ? ? 8B 47 08 A3");
    injector::MakeNOP(pattern.get_first(0), 6, true);
    injector::MakeCALL(pattern.get_first(0), SetWindowPosHook, true);

    //Default Fullscreen Res Hook
    pattern = hook::pattern("A3 ? ? ? ? 8B 4F 0C");
    Screen.dword_A8F0C8 = *pattern.get_first<int32_t*>(1);
    struct FullscreenResXHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0C8 = Screen.nWidth;
        }
    }; injector::MakeInline<FullscreenResXHook>(pattern.get_first(0), pattern.get_first(5));
    pattern = hook::pattern("A3 ? ? ? ? 8B 86 ? ? ? ? 8D 14 40 8B 44 D1 04");
    injector::MakeInline<FullscreenResXHook>(pattern.get_first(0), pattern.get_first(5));

    pattern = hook::pattern("89 0D ? ? ? ? C7 05 ? ? ? ? ? ? ? ? 75 27 8B CE 88 1D");
    Screen.dword_A8F0CC = *pattern.get_first<int32_t*>(2);
    struct FullscreenResYHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *Screen.dword_A8F0CC = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResYHook>(pattern.get_first(0), pattern.get_first(6));
    pattern = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 85 C0 BF");
    injector::MakeInline<FullscreenResYHook>(pattern.get_first(0), pattern.get_first(5));

    //Default dimensions overwrite
    pattern = hook::pattern("8B 0D ? ? ? ? A1 ? ? ? ? 8D 5C 24 2C");
    injector::WriteMemory(pattern.get_first(2), &Screen.nHeight, true);
    injector::WriteMemory(pattern.get_first(7), &Screen.nWidth, true);

    //Hwnd
    pattern = hook::pattern("50 A3 ? ? ? ? FF 15");
    static auto hWnd = *pattern.get_first<HWND>(2);

    pattern = hook::pattern("8B 08 89 4C 24 2C 8B 48 04 89 4C 24 30 38 9E ? ? ? ? 75 17 B9");
    struct FullscreenResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(int32_t*)(regs.esp + 0x2C) = Screen.nWidth;
            *(int32_t*)(regs.esp + 0x30) = Screen.nHeight;
        }
    }; injector::MakeInline<FullscreenResHook>(pattern.get_first(0), pattern.get_first(13));

    //Do not pause on minimize
    pattern = hook::pattern("32 DB E8 ? ? ? ? 8B 4C 24 14 55 57 56 51");
    injector::WriteMemory<uint16_t>(pattern.get_first(0), 0x01B3, true); //mov bl,01


}

void InitGCore()
{
    static std::vector<std::string> list;
    GetResolutionsList(list);
    auto iniRes = format("%dx%d", Screen.nWidth, Screen.nHeight);
    Screen.video_mode = 0;
    int32_t i = 0;
    for (auto& res : list)
    {
        if (res == iniRes)
        {
            Screen.video_mode = i;
            break;
        }
        ++i;
    }

    auto pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "75 09 8B F0 BF ? ? ? ? EB 3E 3D");
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            if (regs.eax != Screen.g_res_w)
            {
                if (Screen.isMenu)
                {
                    if (regs.ebp == 0x26)
                        Screen.video_mode--;
                    else if (regs.ebp == 0x27)
                        Screen.video_mode++;
                }

                if (Screen.video_mode >= (int32_t)list.size())
                    Screen.video_mode = 0;
                else if (Screen.video_mode < 0)
                    Screen.video_mode = (int32_t)list.size() - 1;

                sscanf_s(list[Screen.video_mode].c_str(), "%dx%d", &Screen.nWidth, &Screen.nHeight);
                *Screen.dword_A8F0C8 = Screen.nWidth;
                *Screen.dword_A8F0CC = Screen.nHeight;
                regs.esi = Screen.nWidth;
                regs.edi = Screen.nHeight;
                Screen.g_res_w = regs.eax;
                SetWindowPosHook(g_hWnd, g_hWndInsertAfter, 0, 0, 0, 0, g_uFlags);
                GetRes();
                CIniReader iniWriter(".\\videomode.ini");
                iniWriter.WriteInteger("Settings", "Width", Screen.nWidth);
                iniWriter.WriteInteger("Settings", "Height", Screen.nHeight);
                Screen.isMenu = false;
            }
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(9));

    //Fullscreen res change to windowed (hack!), but otherwise it's kinda weird
    pattern = hook::module_pattern(GetModuleHandle(L"GCore.dll"), "74 0B 57 56 FF 52 38 5F 5E 83 C4 0C C3 8D 44 24 08 50 8D 44 24 10");
    injector::MakeNOP(pattern.get_first(0), 2, true);
}

void Initg_Rhapsody()
{
    auto pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8A 8E ? ? ? ? 88 88 ? ? ? ? 8B 0D ? ? ? ? E8 ? ? ? ? 8B CE E8 ? ? ? ? 8B 56 24 8B 4A 04 8B 01 6A 00 6A 05 FF 90 ? ? ? ? 8B CE FF 15 ? ? ? ? 8D 4C 24 10 68");
    struct MenuCheck
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint8_t*)&regs.ecx = *(uint8_t*)(regs.esi + 0xB0);
            Screen.isMenu = true;
        }
    }; injector::MakeInline<MenuCheck>(pattern.get_first(0), pattern.get_first(6));

    //skip confirm dialogue
    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8B CE FF 15 ? ? ? ? 8D 4C 24 10 68");
    injector::WriteMemory(pattern.get_first(0), 0x835B5E5F, true);
    injector::WriteMemory(pattern.get_first(4), 0x90C314C4, true);

    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "75 46 A1 ? ? ? ? 8B 48 38 8B 91 ? ? ? ? 8B 3A 6A 00");
    injector::MakeNOP(pattern.count(2).get(1).get<void>(0), 2, true);

    pattern = hook::module_pattern(GetModuleHandle(L"g_Rhapsody.sgl"), "8B 8E ? ? ? ? 8B 11 EB 73");
    struct ResTextOverwrite
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ecx = *(uint32_t*)(regs.esi + 0xC0);
            swprintf((wchar_t*)(*(uint32_t*)(regs.eax) + 2), 18, L"%d x %d", Screen.nWidth, Screen.nHeight);
        }
    }; injector::MakeInline<ResTextOverwrite>(pattern.get_first(0), pattern.get_first(6));
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(Init, hook::pattern("8B 47 0C 8B 4F 08"));
        CallbackHandler::RegisterCallback(L"GCore.dll", InitGCore);
        CallbackHandler::RegisterCallback(L"g_Rhapsody.sgl", Initg_Rhapsody);
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
