module;

#include <stdafx.h>
#include <minidx8/d3d8.h>
#include "FixSettings.hpp"
#include "FrameRate.hpp"
#include "DeveloperOverlay.hpp"
#include "Shared/TonyHawk/Display.hpp"

export module Window;

import ComVars;

export namespace THPS4Window
{
    inline SafetyHookInline FactoryHook, PresentHook;
    inline char(__cdecl* Reset)() = nullptr;
    inline LONG Pending = 0;
    inline bool Changing = false;
    inline int LastWindowMode = 1;
    inline RECT FramedRect{};
    inline bool HaveFramedRect = false;
    inline UINT FullscreenInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    inline UINT FullscreenRefresh = 0;
    constexpr wchar_t Property[] = L"THPS4.WidescreenFix.Window";

    struct WindowState
    {
        WNDPROC original;
    };

    inline void SetMode(int mode, int width, int height)
    {
        TonyDisplay::Window(mode, width, height);
        WindowedModeWrapper::bStretchWindow = false;
        WindowedModeWrapper::bScaleWindow = mode == 2;
    }

    inline LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto state = reinterpret_cast<WindowState*>(GetPropW(window, Property));
        if (!state)
            return DefWindowProcA(window, message, wParam, lParam);
        LRESULT result = 0;
        if (DeveloperOverlayMessage(window, message, wParam, lParam, result))
            return result;
        // The game stops (rendering, sound and play) while its window is inactive; in a window mode it can keep running
        static const int& pauseOnFocusLoss = FixSettings().Int("MAIN.PauseOnFocusLoss");
        if (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE && TonyDisplay::Mode != 0 && !pauseOnFocusLoss)
            return DefWindowProcA(window, message, wParam, lParam);
        if (wParam == VK_RETURN && (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP))
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                InterlockedExchange(&Pending, 1);
            return 0;
        }
        // Device reset can synchronously deactivate the window while rebuilding resources.
        if (Changing && message == WM_ACTIVATE)
            return DefWindowProcA(window, message, wParam, lParam);
        auto original = state->original;
        if (message == WM_NCDESTROY)
        {
            RemovePropW(window, Property);
            InterlockedExchange(&Pending, 0);
            delete state;
        }
        return CallWindowProcA(original, window, message, wParam, lParam);
    }

    inline void StoreParameters(const D3DPRESENT_PARAMETERS& value)
    {
        static_assert(sizeof(value) == 52);
        auto source = reinterpret_cast<const uint32_t*>(&value);
        auto target = reinterpret_cast<uint32_t*>(GamePresentation.get_ptr());
        for (size_t i = 0; i < sizeof(value) / sizeof(uint32_t); ++i)
            target[i] = source[i];
    }

    inline void Place(HWND window, int mode)
    {
        if (mode)
        {
            WindowedModeWrapper::afterCreateWindow();
            if (mode == 2)
                WindowedModeWrapper::CenterWindowPosition(GamePresentation->BackBufferWidth, GamePresentation->BackBufferHeight);
            if (mode == 1 && HaveFramedRect)
                SetWindowPos(window, HWND_NOTOPMOST, FramedRect.left, FramedRect.top,
                    FramedRect.right - FramedRect.left, FramedRect.bottom - FramedRect.top,
                    SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        else
        {
            MONITORINFO monitor{ sizeof(monitor) };
            if (!GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor))
                return;
            auto style = GetWindowLongA(window, GWL_STYLE);
            SetWindowLongA(window, GWL_STYLE, (style & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
            SetWindowPos(window, HWND_TOP, monitor.rcMonitor.left, monitor.rcMonitor.top,
                GamePresentation->BackBufferWidth, GamePresentation->BackBufferHeight,
                SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
        }
        TonyDisplay::SquareCorners(window);
    }

    inline void Toggle()
    {
        if (!Pending || Changing || !GameDevice.is_initialized() || !GameDevice.get() || !GameInitialized.get() || GameLost.get())
            return;
        auto window = GamePresentation->hDeviceWindow;
        if (!window || GetForegroundWindow() != window || (GameDevice.get())->TestCooperativeLevel() != D3D_OK)
            return;
        const LONG request = InterlockedExchange(&Pending, 0);
        if (!request)
            return;
        const auto saved = GamePresentation.get();
        const int previousMode = TonyDisplay::Mode;
        const int nextMode = request == 1 ? (previousMode ? 0 : LastWindowMode) : static_cast<int>(request - 2);
        if (nextMode == previousMode)
            return;
        const auto style = GetWindowLongA(window, GWL_STYLE);
        const auto exStyle = GetWindowLongA(window, GWL_EXSTYLE);
        RECT previousRect{};
        GetWindowRect(window, &previousRect);
        if (previousMode == 1)
        {
            FramedRect = previousRect;
            HaveFramedRect = true;
        }
        if (!previousMode)
        {
            FullscreenInterval = saved.FullScreen_PresentationInterval;
            FullscreenRefresh = saved.FullScreen_RefreshRateInHz;
        }
        auto requested = saved;
        requested.Windowed = nextMode != 0;
        requested.FullScreen_RefreshRateInHz = nextMode ? 0 : FullscreenRefresh;
        requested.FullScreen_PresentationInterval = nextMode ? 0 : FullscreenInterval;
        Changing = true;
        WindowedModeWrapper::GameHWND = window;
        SetMode(nextMode, saved.BackBufferWidth, saved.BackBufferHeight);
        Place(window, nextMode);
        StoreParameters(requested);
        GameLost = 1;
        Reset();
        if (!GameLost.get())
        {
            if (nextMode)
                LastWindowMode = nextMode;
            else if (previousMode)
                LastWindowMode = previousMode;
            Place(window, nextMode);
        }
        else
        {
            OutputDebugStringA("THPS4: window mode reset failed; restoring the previous mode.\n");
            SetMode(previousMode, saved.BackBufferWidth, saved.BackBufferHeight);
            StoreParameters(saved);
            SetWindowLongA(window, GWL_STYLE, style);
            SetWindowLongA(window, GWL_EXSTYLE, exStyle);
            SetWindowPos(window, (exStyle & WS_EX_TOPMOST) ? HWND_TOPMOST : HWND_NOTOPMOST,
                previousRect.left, previousRect.top, previousRect.right - previousRect.left,
                previousRect.bottom - previousRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
            Reset();
            if (GameLost.get())
                OutputDebugStringA("THPS4: previous mode reset failed; native device recovery remains active.\n");
        }
        Changing = false;
    }

    inline HRESULT WINAPI Present(IDirect3DDevice8* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (GameDevice.is_initialized() && device == GameDevice.get())
        {
            DrawDeveloperOverlay(GamePresentation->hDeviceWindow);
            SyncFrameRateLimit();
            Toggle();
        }
        auto result = PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
        if (result == D3DERR_INVALIDCALL && GameDevice.is_initialized() && device == GameDevice.get() && SUCCEEDED(device->EndScene()))
        {
            // The native frame loop presents before EndScene, which D3D9 rejects.
            result = PresentHook.stdcall<HRESULT>(device, source, destination, window, dirty);
            device->BeginScene();
        }
        return result;
    }

    inline void Attach()
    {
        if (!GameDevice.is_initialized() || !GameDevice.get() || !GameInitialized.get())
            return;
        if (!PresentHook)
        {
            auto table = *reinterpret_cast<void***>(GameDevice.get());
            PresentHook = safetyhook::create_inline(table[15], Present);
        }
        auto window = GamePresentation->hDeviceWindow;
        if (!PresentHook || !window || GetPropW(window, Property))
            return;
        auto state = new WindowState{ reinterpret_cast<WNDPROC>(GetWindowLongPtrA(window, GWLP_WNDPROC)) };
        if (!SetPropW(window, Property, state))
        {
            delete state;
            return;
        }
        SetLastError(0);
        if (!SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
        {
            RemovePropW(window, Property);
            delete state;
        }
    }

    inline int __cdecl CreateDevice()
    {
        auto result = FactoryHook.ccall<int>();
        Attach();
        return result;
    }

    inline void Install(int mode, HMODULE module = GetModuleHandleW(nullptr))
    {
        auto factory = hook::module_pattern(module, "83 EC 30 53 55 56 57 68 DC 00 00 00 E8 ? ? ? ? 33 ED A3");
        auto device = hook::module_pattern(module, "A1 ? ? ? ? 68 ? ? ? ? 50 8B 10 FF 92 80 00 00 00 A1 ? ? ? ? 68 ? ? ? ? 50 8B 08 FF 91 84 00 00 00");
        auto reset = hook::module_pattern(module, "A0 ? ? ? ? 53 33 DB 3A C3 0F 84 ? ? ? ? 38 1D ? ? ? ? 0F 84 ? ? ? ? E8 ? ? ? ? E8");
        auto parameters = hook::module_pattern(module, "89 2D ? ? ? ? 89 0D ? ? ? ? 89 1D ? ? ? ? A3 ? ? ? ? 89 1D ? ? ? ? 89 2D ? ? ? ?");
        GamePresentation.SetAddress(*parameters.get_first<uintptr_t>(2) - offsetof(D3DPRESENT_PARAMETERS, Windowed));
        GameDevice.SetAddress(*device.get_first<IDirect3DDevice8**>(1));
        GameInitialized.SetAddress(*reset.get_first<uint8_t*>(1));
        GameLost.SetAddress(*reset.get_first<uint8_t*>(18));
        Reset = reinterpret_cast<decltype(Reset)>(reset.get_first());
        LastWindowMode = mode ? mode : 1;
        FixSettings().OnChange("MAIN.WindowMode", []()
        {
            InterlockedExchange(&Pending, std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2) + 2);
        });
        FactoryHook = safetyhook::create_inline(factory.get_first(), CreateDevice);
    }
}

export namespace THPS4Display
{
    void Init(int width, int height)
    {
        auto& settings = FixSettings();
        auto mode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);
        WindowedModeWrapper::EnableDpiAwareness();
        THPS4Window::SetMode(0, width, height);
        THPS4Window::Install(mode);
        if (mode != 0)
        {
            auto pattern = hook::pattern("89 2D ? ? ? ? 89 0D ? ? ? ? 89 1D ? ? ? ? A3 ? ? ? ? 89 1D ? ? ? ? 89 2D ? ? ? ?");
            auto interval = hook::pattern("C7 05 ? ? ? ? 00 00 00 80 C7 05 ? ? ? ? 16 00 00 00 C7 05 ? ? ? ? 4D 00 00 00 8B 10");
            NativeWindowed.SetAddress(*pattern.get_first<int*>(2));
            static auto windowedHook = safetyhook::create_mid(pattern.get_first(6), [](SafetyHookContext&)
            {
                NativeWindowed = 1;
            });
            PresentationInterval.SetAddress(*interval.get_first<int*>(2));
            static auto intervalHook = safetyhook::create_mid(interval.get_first(30), [](SafetyHookContext&)
            {
                PresentationInterval = 0;
            });
            THPS4Window::SetMode(mode, width, height);
        }
        if (settings.Bool("MAIN.SkipIntro"))
        {
            // The startup logos and the intro movie play once before the main menu; Options > Movies still plays them
            auto pattern = hook::pattern("8B 4C 24 04 8D 44 24 04 6A 01 50 6A 00 E8 ? ? ? ? 84 C0 74 10 8B 4C 24 04 51 E8 ? ? ? ? 83 C4 04 B0 01 C3 32 C0 C3");
            static const uintptr_t resume = reinterpret_cast<uintptr_t>(pattern.get_first(32));
            static auto movieHook = safetyhook::create_mid(pattern.get_first(27), [](SafetyHookContext& regs)
            {
                static std::array<std::pair<const char*, bool>, 4> startup{ { { "atvi", false }, { "aspyr", false }, { "nslogo", false }, { "intro", false } } };
                std::string_view name(*reinterpret_cast<const char**>(regs.esp));
                if (auto slash = name.find_last_of("/\\"); slash != std::string_view::npos)
                    name.remove_prefix(slash + 1);
                for (auto& [movie, skipped] : startup)
                {
                    if (!skipped && name.size() == std::strlen(movie) && _strnicmp(name.data(), movie, name.size()) == 0)
                    {
                        skipped = true;
                        regs.eip = resume;
                        return;
                    }
                }
            });
        }
    }
}


export void InstallResolution()
{
    hook::pattern pattern("");
    //Resolution
    pattern = hook::pattern("8B 4C B4 20 89 15");
    dword_5A0280.SetAddress(*pattern.get_first<int32_t*>(6));
    dword_5A0284.SetAddress(*pattern.get_first<int32_t*>(12));
    struct SetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            dword_5A0280 = Screen.Width;
            dword_5A0284 = Screen.Height;
        }
    };
    const char* patterns[] = {
        "89 15 ? ? ? ? 89 0D ? ? ? ? 3B C5",
        "89 0D ? ? ? ? 8B 0D ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? A3 ? ? ? ?",
        "89 0D ? ? ? ? 89 15 ? ? ? ? 68 ? ? ? ? 55",
        "89 0D ? ? ? ? 3B C5",
        "89 15 ? ? ? ? 68 ? ? ? ? 55"
    };
    std::array<uint8_t*, 5> sites{};
    for (size_t i = 0; i < sites.size(); ++i)
        sites[i] = hook::pattern(patterns[i]).get_first<uint8_t>();
    auto accumulator = hook::pattern("A3 ? ? ? ? A1 ? ? ? ? 68 ? ? ? ? 6A 44 51 89 2D ? ? ? ?").get_first<uint8_t>();
    for (auto* site : sites)
        injector::MakeInline<SetResHook>(site, site + 6);
    injector::MakeInline<SetResHook>(accumulator);
}
