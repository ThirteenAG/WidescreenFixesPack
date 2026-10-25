#include "stdafx.h"
#include "../Shared/PatchPreflight.hpp"
#include "FixSettings.hpp"

ASI_SETTINGS_EXPORT(FixSettings())

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
    float fFOVFactor;
} Screen;

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    if (hWnd)
    {
        SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
        SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
    return hWnd;
}

BOOL WINAPI AdjustWindowRectHook(LPRECT lpRect, DWORD dwStyle, BOOL bMenu)
{
    return AdjustWindowRect(lpRect, dwStyle & ~WS_OVERLAPPEDWINDOW, bMenu);
}

void RaiseD3DIMRenderTargetLimit()
{
    // d3dim700.dll refuses render targets wider or taller than 2048 (CreateDevice and SetRenderTarget return an error),
    // so the renderer is never created above that; loaded here first so ddraw picks up the patched module later
    auto hD3DIM = LoadLibraryExW(L"d3dim700.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!hD3DIM)
        return;

    auto pattern = hook::module_pattern(hD3DIM, "B8 00 08 00 00 39 45 ? 0F 87 ? ? ? ? 39 45 ? 0F 87"); //DIRECT3DDEVICEI::Init
    if (!pattern.empty())
        ProtectedGameRef<uint32_t>{pattern.get_first<uint32_t>(1)} = 16384;

    pattern = hook::module_pattern(hD3DIM, "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87"); //DIRECT3DDEVICEI::SetRenderTarget
    if (!pattern.empty())
        ProtectedGameRef<uint32_t>{pattern.get_first<uint32_t>(1)} = 16384;
}

void Init()
{
    hook::pattern pattern;
    Screen.nWidth = FixSettings().Int("MAIN.ResX");
    Screen.nHeight = FixSettings().Int("MAIN.ResY");

    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 32767 || Screen.nHeight > 32767)
        std::tie(Screen.nWidth, Screen.nHeight) = std::make_tuple(640, 480);

    Screen.fWidth = static_cast<float>(Screen.nWidth);
    Screen.fHeight = static_cast<float>(Screen.nHeight);
    Screen.fAspectRatio = Screen.fWidth / Screen.fHeight;
    Screen.nWidth43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
    Screen.fWidth43 = static_cast<float>(Screen.nWidth43);
    Screen.fHudScale = 1.0f / (((4.0f / 3.0f)) / (Screen.fAspectRatio));
    Screen.fHudOffset = ((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f;
    Screen.fHudOffsetReal = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;

    RaiseD3DIMRenderTargetLimit();

    //Default res
    if (wfp::PatchPreflight("Knight Rider default resolution: signatures unavailable; feature disabled.\n",
    {
        { "68 E0 01 00 00 8D 44 24 1C 68 80 02 00 00 8D 4C 24 1C 50 51 6A 03 8B CE", 1 },
    }))
    {
        pattern = hook::pattern("68 E0 01 00 00 8D 44 24 1C 68 80 02 00 00 8D 4C 24 1C 50 51 6A 03 8B CE");
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(1)} = Screen.nHeight;
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(10)} = Screen.nWidth;

    }

    //Menu res
    if (wfp::PatchPreflight("Knight Rider selected resolution: signatures unavailable; feature disabled.\n",
    {
        { "8B 44 24 28 89 30 8B 55 04 8B 6D 08", 1 },
    }))
    {
        pattern = hook::pattern("8B 44 24 28 89 30 8B 55 04 8B 6D 08");
        struct ForceResHook
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.eax = *(uint32_t*)(regs.esp + 0x24);
                *reinterpret_cast<uint32_t*>(regs.eax) = Screen.nWidth;
                regs.eax = *(uint32_t*)(regs.esp + 0x28);
                *reinterpret_cast<uint32_t*>(regs.eax) = Screen.nHeight;
            }
        }; injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6));

    }

    //Aspect Ratio
    if (wfp::PatchPreflight("Knight Rider aspect: signatures unavailable; feature disabled.\n",
    {
        { "AB AA AA 3F", 0 },
        { "A1 ? ? ? ? 83 C4 0C D8 C9 50 51", 1 },
    }))
    {
        pattern = hook::pattern("AB AA AA 3F"); //1.3333334
        for (size_t i = 0; i < pattern.size(); i++)
        {
            ProtectedGameRef<float>{pattern.get(i).get<float>(0)} = Screen.fAspectRatio;
        }

        pattern = hook::pattern("A1 ? ? ? ? 83 C4 0C D8 C9 50 51");
        ProtectedGameRef<float>{*pattern.get_first<float*>(1)} = Screen.fAspectRatio;

    }

    //2D
    if (wfp::PatchPreflight("Knight Rider HUD and cursor: signatures unavailable; feature disabled.\n",
    {
        { "8B 8E B0 01 00 00 8B 01 FF 90 B4 00 00 00 8B 8E 7C 01 00 00 85 C9", 1 },
        { "85 F6 74 11 8B 46 04 48 89 46 04 75 08 8B 06 6A 01 8B CE FF 10 5E C2 08 00", 1 },
        { "8B 4E 2C E8 ? ? ? ? 8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0", 1 },
        { "8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0 74 10 8B 46 14 8B 4E", 1 },
        { "8B 96 C4 00 00 00 33 C9 8B 52 08 8B 82 EC 00 00 00 85 C0", 1 },
        { "83 7C 24 08 01 75 08 8B 49 10 E8 ? ? ? ? C2 08 00", 2 },
        { "8B 4B 20 E8 ? ? ? ? 5F 5E", 1 },
        { "5F 5E 5B C2 08 00 81 EC C0 00 00 00", 1 },
        { "C6 86 B4 00 00 00 00 8B 43 0C 8B 48 4C 89 8E B8 00 00 00", 1 },
        { "6A 04 E8 ? ? ? ? 8B 4E 28 E8 ? ? ? ? 5E C2 08 00", 1 },
        { "C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C", 1 },
    }))
    {
        static auto dword_56F775 = hook::get_pattern("8B 8E B0 01 00 00 8B 01 FF 90 B4 00 00 00 8B 8E 7C 01 00 00 85 C9", 0);
        static auto dword_4AF6E1 = hook::get_pattern("85 F6 74 11 8B 46 04 48 89 46 04 75 08 8B 06 6A 01 8B CE FF 10 5E C2 08 00", 0);
        static auto dword_40AC69 = hook::get_pattern("8B 4E 2C E8 ? ? ? ? 8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0", 0);
        static auto dword_40AC71 = hook::get_pattern("8B 4E 1C E8 ? ? ? ? 8B 4E 20 E8 ? ? ? ? 8A 46 58 84 C0 74 10 8B 46 14 8B 4E", 0);
        static auto dword_4BD9A5 = hook::get_pattern("8B 96 C4 00 00 00 33 C9 8B 52 08 8B 82 EC 00 00 00 85 C0", 0);
        static auto dword_4081EF = hook::pattern("83 7C 24 08 01 75 08 8B 49 10 E8 ? ? ? ? C2 08 00").count_hint(2).get(1).get<void>(15); //objective marker
        static auto dword_404D92 = hook::get_pattern("8B 4B 20 E8 ? ? ? ? 5F 5E", 0); //night vision 1
        static auto dword_404D9A = hook::get_pattern("5F 5E 5B C2 08 00 81 EC C0 00 00 00", 0); //night vision 2
        static auto dword_4B514B = hook::get_pattern("C6 86 B4 00 00 00 00 8B 43 0C 8B 48 4C 89 8E B8 00 00 00", 0);
        static auto dword_40ACAE = hook::get_pattern("6A 04 E8 ? ? ? ? 8B 4E 28 E8 ? ? ? ? 5E C2 08 00", 15); //dialog faces

        pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C");
        struct HUDHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *reinterpret_cast<uint32_t*>(regs.esp + 0x14) = 0;

                auto ret1 = *(void**)(regs.esp + 0xC8);
                auto ret2 = *(void**)(regs.esp + 0xF0);
                auto ret3 = *(void**)(regs.esp + 0xF4);

                if (ret1 == dword_56F775 && ret2 != dword_4AF6E1 && ret2 != dword_40AC69 && ret2 != dword_40AC71 && ret2 != dword_4BD9A5 && ret2 != dword_4081EF && ret2 != dword_404D92 && ret2 != dword_404D9A && ret3 != dword_4B514B)
                {
                    *reinterpret_cast<int32_t*>(regs.esp + 0x70) = (int32_t)(((((Screen.fWidth * (float)(*(int32_t*)(regs.esp + 0x70))) / Screen.fHudScale) + Screen.fHudOffsetReal) / Screen.fWidth) + Screen.fHudOffsetReal);
                    *reinterpret_cast<int32_t*>(regs.esp + 0x78) = (int32_t)((float)(*(int32_t*)(regs.esp + 0x78)) / Screen.fHudScale);

                    if (ret2 == dword_40ACAE)
                    {
                        *reinterpret_cast<int32_t*>(regs.esp + 0x70) = 20;
                    }
                }
            }
        }; injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8)); //0x5C0EB1

    }

    if (wfp::PatchPreflight("Knight Rider FOV: signatures unavailable; feature disabled.\n",
    {
        { "D8 0D ? ? ? ? 53 8B D9 56 57 D9 F2 8B 53 08", 1 },
    }))
    {
        pattern = hook::pattern("D8 0D ? ? ? ? 53 8B D9 56 57 D9 F2 8B 53 08");
        struct FOVHook
        {
            void operator()(injector::reg_pack& regs)
            {
                float t = 0.0087266462f;
                float f = 0.0f;
                _asm {fstp[f]}
                static const auto& factor = FixSettings().Float("MAIN.FOVFactor");
                f = AdjustFOV(std::min(f * std::clamp(factor, 0.5f, 2.0f), 170.0f), Screen.fAspectRatio);
                _asm {fld[f]}
                _asm {fmul[t]}
            }
        }; injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6)); //0x485717

    }

    //Window style fix
    if (wfp::PatchPreflight("Knight Rider window style: signatures unavailable; feature disabled.\n",
    {
        { "8B 2D ? ? ? ? 50 68 ? ? ? ? 57 FF D5", 1 },
        { "FF 15 ? ? ? ? A1 ? ? ? ? BB 00 00 00 40 8A 48 24 84 C9", 1 },
    }))
    {
        pattern = hook::pattern("8B 2D ? ? ? ? 50 68 ? ? ? ? 57 FF D5");
        ProtectedGameRef<decltype(&CreateWindowExAHook)>{reinterpret_cast<decltype(&CreateWindowExAHook)*>(*pattern.get_first<uint32_t*>(2))} = &CreateWindowExAHook; //60E178
        pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? BB 00 00 00 40 8A 48 24 84 C9");
        ProtectedGameRef<decltype(&AdjustWindowRectHook)>{reinterpret_cast<decltype(&AdjustWindowRectHook)*>(*pattern.get_first<uint32_t*>(2))} = &AdjustWindowRectHook; //60E184
    }


}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
        {
            // Per monitor DPI awareness, so the game gets real pixels on scaled desktops (the desktop resolution and a
            // full screen picture that isn't scaled up from its top left part)
            using SetContext = BOOL(WINAPI*)(HANDLE);
            if (auto set = reinterpret_cast<SetContext>(GetProcAddress(GetModuleHandleW(L"user32.dll"), "SetProcessDpiAwarenessContext")))
                set(reinterpret_cast<HANDLE>(-4));
            else
                SetProcessDPIAware();

            FixSettings();
            asi_settings::LiveLink::Start();
            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("68 E0 01 00 00 8D 44 24 1C 68 80 02"));
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
