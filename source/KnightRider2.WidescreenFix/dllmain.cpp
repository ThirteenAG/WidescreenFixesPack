#include "stdafx.h"
#include "FixSettings.hpp"
#include <ddraw.h>

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
    float fRadarBlipSize;
    float fRadarBlipPosition;
    float fPointSize;
    uint32_t PointSizeInstr;
} Screen;

HWND WINAPI CreateWindowExAHook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam)
{
    auto hWnd = CreateWindowExA(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
    SetWindowLong(hWnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
    SetWindowPos(hWnd, nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
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
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");

    pattern = hook::module_pattern(hD3DIM, "B8 00 08 00 00 39 85 ? ? ? ? 0F 87 ? ? ? ? 39 85 ? ? ? ? 0F 87"); //DIRECT3DDEVICEI::SetRenderTarget
    if (!pattern.empty())
        ProtectedGameRef<uint32_t>{pattern.get_first<uint32_t>(1)} = 16384;
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");
}

// DirectShow decodes into a DirectDraw surface; only the movie blit uses this path.
IDirectDrawSurface* WINAPI BltMovieFrame(IDirectDrawSurface* destination, RECT* destinationRect,
    IDirectDrawSurface* source, RECT* sourceRect, DWORD flags, DDBLTFX* effects)
{
    DDSURFACEDESC description{ sizeof(description) };
    if (SUCCEEDED(destination->GetSurfaceDesc(&description)) && description.dwWidth && description.dwHeight)
    {
        const auto width = static_cast<LONG>(description.dwWidth);
        const auto height = static_cast<LONG>(description.dwHeight);
        LONG movieWidth = width;
        LONG movieHeight = height;
        if (static_cast<int64_t>(width) * 3 > static_cast<int64_t>(height) * 4)
            movieWidth = std::max(1L, height * 4 / 3);
        else
            movieHeight = std::max(1L, width * 3 / 4);

        RECT movieRect{ (width - movieWidth) / 2, (height - movieHeight) / 2, 0, 0 };
        movieRect.right = movieRect.left + movieWidth;
        movieRect.bottom = movieRect.top + movieHeight;
        DDBLTFX clear{ sizeof(clear) };
        destination->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &clear);
        destination->Blt(&movieRect, source, sourceRect, flags, effects);
    }
    else
    {
        destination->Blt(destinationRect, source, sourceRect, flags, effects);
    }
    return destination;
}

void Init()
{
    auto& settings = FixSettings();
    asi_settings::LiveLink::Start();
    Screen.nWidth = settings.Int("MAIN.ResX");
    Screen.nHeight = settings.Int("MAIN.ResY");
    const bool bFixFMV = settings.Bool("MAIN.FixFMV");

    if (Screen.nWidth <= 0 || Screen.nHeight <= 0 || Screen.nWidth > 16384 || Screen.nHeight > 16384)
        std::tie(Screen.nWidth, Screen.nHeight) = GetDesktopRes();
    if (Screen.nWidth <= 0 || Screen.nHeight <= 0)
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
    auto pattern = hook::pattern("68 E0 01 00 00 8D 44 24 18 68 80 02 00 00 8D 4C 24 18 50 51 6A 04");
    if (!pattern.empty())
    {
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(1)} = Screen.nHeight;
        ProtectedGameRef<int32_t>{pattern.get_first<int32_t>(10)} = Screen.nWidth;
    }
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");

    //Menu res
    pattern = hook::pattern("8B 5C 24 30 89 03 8B 45 04 8B 75 08");
    struct ForceResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.ebx = *(uint32_t*)(regs.esp + 0x2C);
            *(uint32_t*)(regs.ebx) = Screen.nWidth;
            regs.ebx = *(uint32_t*)(regs.esp + 0x30);
            *(uint32_t*)(regs.ebx) = Screen.nHeight;
        }
    };
    if (!pattern.empty())
        injector::MakeInline<ForceResHook>(pattern.get_first(0), pattern.get_first(6));
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");

    //Aspect Ratio
    pattern = hook::pattern("AB AA AA 3F"); //1.3333334
    for (size_t i = 0; i < pattern.size(); i++)
    {
        if (i != 5) //PointTGA
            ProtectedGameRef<float>{pattern.get(i).get<float>(0)} = Screen.fAspectRatio;
    }
    pattern = hook::pattern("39 8E E3 3F"); //1.7777778
    for (size_t i = 0; i < pattern.size(); i++)
    {
        ProtectedGameRef<float>{pattern.get(i).get<float>(0)} = Screen.fAspectRatio;
    }

    //2D
    auto findAddress = [](const char* signature, ptrdiff_t offset, size_t index = 0) -> void*
    {
        auto match = hook::pattern(signature);
        if (match.empty() || match.size() <= index)
        {
            static bool reported = false;
            if (!reported)
            {
                reported = true;
                OutputDebugStringA("Knight Rider 2: HUD caller unavailable; HUD correction disabled.\n");
            }
            return nullptr;
        }
        return match.get(index).get<void>(offset);
    };
    static auto dword_7E2446 = findAddress("83 C4 0C 5D C3 55 8B CD E8 ? ? ? ? 5D C3", 13);
    static auto dword_63AFDC = findAddress("8B 44 24 04 85 C0 74 17 8B 70 04 8B C8 83 C0 04 4E", 0);
    static auto dword_65E46E = findAddress("80 88 CC 00 00 00 01 8B 41 68 8B 48 08 E8 ? ? ? ? 5E C2 08 00", 18);
    static auto dword_65E540 = findAddress("8B 56 64 8B 4A 08 33 D2 66 39 91 D6 00 00 00", 0, 1);
    static auto dword_532FF8 = findAddress("8B 4E 44 E8 ? ? ? ? 8A 46 79 84 C0 0F 84 ? ? ? ? 8B 46 2C", 0);
    static auto dword_5369D3 = findAddress("83 C4 10 8D 4C 24 04 E8 ? ? ? ? 8B 4E 0C E8 ? ? ? ? 5E 59 C2 08 00", 20);
    static auto dword_52E81A = findAddress("8B 4E 30 E8 ? ? ? ? 5E C2 08 00", 8);
    static auto dword_5D4B43 = findAddress("83 C6 04 4F 75 E7 5F 5E 5B C2 04 00", 0);
    static auto dword_538F62 = findAddress("8B 4F 7C E8 ? ? ? ? 5F 5E 5D C2 04 00", 8);
    static auto dword_52A984 = findAddress("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 0); //night vision 1
    static auto dword_52A98C = findAddress("8B 4E 28 E8 ? ? ? ? 5E 59 C2 08 00", 8); //night vision 2
    static auto dword_5330C3 = findAddress("8B 4E 40 E8 ? ? ? ? 5E 59 C2 08 00", 8); //dialog faces

    pattern = hook::pattern("C7 44 24 14 00 00 00 00 DA 4C 24 10 E8 ? ? ? ? 89 44 24 7C 8B 46 30");
    struct HUDHook
    {
        void operator()(injector::reg_pack& regs)
        {
            *(uint32_t*)(regs.esp + 0x14) = 0;

            auto ret1 = *(void**)(regs.esp + 0xC8);
            auto ret2 = *(void**)(regs.esp + 0xF4);
            auto ret3 = *(void**)(regs.esp + 0x100);

            if (ret2 == dword_7E2446 && ret3 != dword_63AFDC && ret3 != dword_65E46E && ret3 != dword_65E540 && ret3 != dword_532FF8 && ret3 != dword_5369D3 &&
                ret3 != dword_52E81A && ret3 != dword_5D4B43 && ret3 != dword_538F62 && ret3 != dword_52A984 && ret3 != dword_52A98C)
            {
                *(int32_t*)(regs.esp + 0x70) = (int32_t)(((((Screen.fWidth * (float)(*(int32_t*)(regs.esp + 0x70))) / Screen.fHudScale) + Screen.fHudOffsetReal) / Screen.fWidth) + Screen.fHudOffsetReal);
                *(int32_t*)(regs.esp + 0x78) = (int32_t)((float)(*(int32_t*)(regs.esp + 0x78)) / Screen.fHudScale);

                if (ret3 == dword_5330C3) //dialog faces
                {
                    *(int32_t*)(regs.esp + 0x70) = 20;
                }
            }
        }
    };
    if (!pattern.empty() && dword_7E2446 && dword_63AFDC && dword_65E46E && dword_65E540 && dword_532FF8 &&
        dword_5369D3 && dword_52E81A && dword_5D4B43 && dword_538F62 && dword_52A984 && dword_52A98C && dword_5330C3)
        injector::MakeInline<HUDHook>(pattern.get_first(0), pattern.get_first(8));

    pattern = hook::pattern("D8 0D ? ? ? ? 8B D9 8B 43 08 56 D9 F2");
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
    };
    if (!pattern.empty())
        injector::MakeInline<FOVHook>(pattern.get_first(0), pattern.get_first(6));
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");

    //Window style fix
    pattern = hook::pattern("FF 15 ? ? ? ? 8B F8 8B 46 24 85 C0");
    if (!pattern.empty())
        ProtectedGameRef<decltype(&CreateWindowExAHook)>{*pattern.get_first<decltype(&CreateWindowExAHook)*>(2)} = CreateWindowExAHook;
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n"); //0x8AB184
    pattern = hook::pattern("FF 15 ? ? ? ? A1 ? ? ? ? 8A 48 24 84 C9");
    if (!pattern.empty())
        ProtectedGameRef<decltype(&AdjustWindowRectHook)>{*pattern.get_first<decltype(&AdjustWindowRectHook)*>(2)} = AdjustWindowRectHook;
    else
        OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n"); //0x8AB188

    if (bFixFMV)
    {
        pattern = hook::pattern("6A 00 68 00 00 00 01 8D 4C 24 18 51 8B 4C 24 18 51 6A 00 50 FF 52 14 8B 44 24 24 8B 10 50 FF 52 08");
        if (!pattern.empty())
        {
            // Replace Blt and its discarded result with a call returning the surface for Release.
            auto blit = pattern.get_first<uint8_t>(20);
            injector::MakeCALL(blit, BltMovieFrame);
            injector::MakeNOP(blit + 5, 2);
        }
        else
            OutputDebugStringA("Knight Rider 2: patch site unavailable; affected feature disabled.\n");
    }
}

ASI_SETTINGS_EXPORT(FixSettings())

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

            CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("8D 4C 24 18 50 51 6A 04 8B CE"));
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
