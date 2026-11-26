module;
#include <stdafx.h>
#include <d3d9.h>
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/Overlay/D3D9UpdateNotice.hpp"
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"

export module Window;
import ComVars;
import DeveloperOverlay;


export namespace LARushWindow
{
    inline int Mode = 0;
    inline std::atomic<HWND> Window{ nullptr };
    inline SafetyHookInline ModeHook, LoopHook;
    inline bool DeviceAttached = false;
    inline bool Focused = true;
    inline bool PausedByFocus = false;
    inline UINT FocusSettingsMessage = 0;
    inline std::atomic<bool> PauseWhenUnfocused{ true };
    inline std::atomic<int> MenuFrameLimit{ 0 };
    inline asi_overlay::D3D9UpdateNotice UpdateNotice;
    inline ULONGLONG NoticeUntil = 0;
    inline std::function<void()> CameraFrame;
    inline std::function<void()> HudFrame;
    inline std::function<bool()> IsLowrider;
    inline void (__cdecl* NativeExit)(int) = nullptr;

    inline void __cdecl Exit(int code)
    {
        RWGTA::UpdateCheck::OnExit();
        NativeExit(code);
    }

    inline bool FocusPauseRequested()
    {
        const int state = GameState;
        return !Focused && PauseWhenUnfocused.load(std::memory_order_relaxed) && state == 56;
    }

    inline void UpdateFocusPause()
    {
        if (!LARushOverlay::GamePaused.is_initialized())
            return;
        if (FocusPauseRequested())
        {
            if (!LARushOverlay::GamePaused)
            {
                LARushOverlay::Pause(true);
                PausedByFocus = true;
            }
        }
        else if (PausedByFocus && !LARushOverlay::Overlay.BlocksInput())
        {
            if (LARushOverlay::GamePaused)
                LARushOverlay::Pause(false);
            PausedByFocus = false;
        }
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, id);
            auto expected = window;
            if (Window.compare_exchange_strong(expected, nullptr))
                CursorConfinement::Release();
            return DefSubclassProc(window, message, wParam, lParam);
        }
        if (window != Window.load())
            return DefSubclassProc(window, message, wParam, lParam);
        if (message == WM_ACTIVATE || message == WM_ACTIVATEAPP)
        {
            Focused = message == WM_ACTIVATE ? LOWORD(wParam) != WA_INACTIVE : wParam != 0;
            UpdateFocusPause();
        }
        else if (FocusSettingsMessage && message == FocusSettingsMessage)
        {
            UpdateFocusPause();
            return 0;
        }
        CursorConfinement::Message(message, wParam);
        LRESULT result = 0;
        if (LARushOverlay::Message(window, message, wParam, lParam, result))
            return result;
        return DefSubclassProc(window, message, wParam, lParam);
    }

    inline int __cdecl SetMode(int requested)
    {
        return ModeHook.ccall<int>(Mode ? 1 : requested);
    }

    inline int __cdecl Loop()
    {
        static std::once_flag gameInit;
        std::call_once(gameInit, []()
        {
            WFP::onGameInitEvent().executeAll();
        });
        WFP::onGameProcessEvent().executeAll();
        return LoopHook.ccall<int>();
    }

    inline bool NoticePlayable()
    {
        const auto window = Window.load();
        const int state = GameState;
        return window && (state == 14 || state == 170) && GetForegroundWindow() == window;
    }

    inline bool NoticeTargetReady(IDirect3DDevice9* device)
    {
        using Microsoft::WRL::ComPtr;
        ComPtr<IDirect3DSurface9> backbuffer, target;
        D3DCAPS9 caps{};
        if (FAILED(device->GetDeviceCaps(&caps)) ||
            FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, backbuffer.GetAddressOf())) ||
            FAILED(device->GetRenderTarget(0, target.GetAddressOf())) || target.Get() != backbuffer.Get())
            return false;
        for (DWORD i = 1; i < caps.NumSimultaneousRTs; ++i)
        {
            ComPtr<IDirect3DSurface9> extra;
            if (device->GetRenderTarget(i, extra.GetAddressOf()) != D3DERR_NOTFOUND)
                return false;
        }
        return true;
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (FAILED(device->GetCreationParameters(&creation)) || !creation.hFocusWindow)
            return;
        const auto window = creation.hFocusWindow;
        if (CameraFrame)
            CameraFrame();
        const int state = GameState;
        const int limit = std::clamp(MenuFrameLimit.load(std::memory_order_relaxed), 0, 1000);
        // The game has its own 30 Hz simulation. Loading and Bink playback keep their native timing.
        D3D9Options::frameLimit = state == 56 || state == 12 || state == 29 || state == 30
            ? 0 : (limit == 0 ? 0 : std::max(30, limit));
        if (window != Window.load())
        {
            if (!SetWindowSubclass(window, Procedure, 1, 0))
                return;
            Window.store(window);
            Focused = GetForegroundWindow() == window;
            if (Mode)
            {
                IDirect3DSurface9* surface = nullptr;
                if (SUCCEEDED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    const auto success = SUCCEEDED(surface->GetDesc(&desc));
                    surface->Release();
                    if (success)
                    {
                        const DWORD style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
                        SetWindowLongW(window, GWL_STYLE, static_cast<LONG>(style | WS_VISIBLE));
                        const DWORD exStyle = static_cast<DWORD>(GetWindowLongW(window, GWL_EXSTYLE));
                        RECT frame{ 0, 0, static_cast<LONG>(desc.Width), static_cast<LONG>(desc.Height) };
                        using AdjustForDpi = BOOL(WINAPI*)(LPRECT, DWORD, BOOL, DWORD, UINT);
                        using GetDpi = UINT(WINAPI*)(HWND);
                        static const auto user = GetModuleHandleW(L"user32.dll");
                        static const auto adjust = reinterpret_cast<AdjustForDpi>(GetProcAddress(user, "AdjustWindowRectExForDpi"));
                        static const auto dpi = reinterpret_cast<GetDpi>(GetProcAddress(user, "GetDpiForWindow"));
                        if (!adjust || !dpi || !adjust(&frame, style, FALSE, exStyle, dpi(window)))
                            AdjustWindowRectEx(&frame, style, FALSE, exStyle);
                        MONITORINFO monitor{ sizeof(monitor) };
                        GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &monitor);
                        const auto& bounds = monitor.rcMonitor;
                        const int x = bounds.left + (bounds.right - bounds.left - static_cast<int>(desc.Width)) / 2 + frame.left;
                        const int y = bounds.top + (bounds.bottom - bounds.top - static_cast<int>(desc.Height)) / 2 + frame.top;
                        WindowStyle::detail::AllowSize(window, frame.right - frame.left, frame.bottom - frame.top);
                        SetWindowPos(window, nullptr, x, y, frame.right - frame.left, frame.bottom - frame.top,
                            SWP_NOACTIVATE | SWP_NOZORDER | SWP_FRAMECHANGED);
                        using Attribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
                        static const auto setAttribute = []()
                        {
                            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
                            return module ? reinterpret_cast<Attribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
                        }();
                        const DWORD square = 1;
                        if (setAttribute)
                            setAttribute(window, 33, &square, sizeof(square));
                    }
                }
            }
        }
        CursorConfinement::Suspend(LARushOverlay::BlocksInput() || CursorDisplayCount >= 0);
        CursorConfinement::Update(window);
        UpdateFocusPause();
        LARushOverlay::Present(device, window);
        RWGTA::UpdateCheck::Tick();
        if (NoticeUntil && (GetTickCount64() >= NoticeUntil || FixSettings().Get("MAIN.CheckForUpdates") == "0"))
        {
            NoticeUntil = 0;
            UpdateNotice.BeforeReset();
        }
        if (NoticeUntil && NoticePlayable() && NoticeTargetReady(device))
            UpdateNotice.Present(device, LARushOverlay::ThisModule(), true);
        UpdateFocusPause();
        if (HudFrame)
            HudFrame();
    }

    inline void Init(int32_t& width, int32_t& height)
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (Mode == 2)
            std::tie(width, height) = GetDesktopRes();
        const auto updateCursor = []()
        {
            static std::mutex mutex;
            std::lock_guard lock(mutex);
            CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
        };
        updateCursor();
        FixSettings().OnChange("MAIN.ConfineCursor", updateCursor);
        auto mode = hook::pattern("8B 0D ? ? ? ? 51 E8 ? ? ? ? 8D 54 24 18 52 8D 44 24 18 50 E8");
        const auto call = mode.get_first<uint8_t>(7);
        ModeHook = safetyhook::create_inline(call + 5 + *reinterpret_cast<int32_t*>(call + 1), SetMode);
        LARushOverlay::Init();
        FocusSettingsMessage = RegisterWindowMessageA("LARush.FusionFix.FocusSettingsChanged");
        LARushOverlay::FocusPauseRequested = FocusPauseRequested;
        BindLiveSetting("MAIN.PauseOnFocusLoss", PauseWhenUnfocused, []()
        {
            if (const auto window = Window.load(); window && FocusSettingsMessage)
                PostMessageW(window, FocusSettingsMessage, 0, 0);
        });
        BindLiveSetting("MAIN.FPSLimitMenus", MenuFrameLimit);
        auto state = hook::pattern("51 A1 ? ? ? ? 53 55 56 57 33 FF 3D 84 00 00 00");
        GameState.SetAddress(*state.get_first<int32_t*>(2));
        auto cursor = hook::pattern("A1 ? ? ? ? 85 C0 7D ? 6A 01 FF 15 ? ? ? ? A3 ? ? ? ? B8 01 00 00 00 C3");
        CursorDisplayCount.SetAddress(*cursor.get_first<int32_t*>(1));
        RWGTA::UpdateCheck::Config update;
        update.pluginId = "LARush.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []()
        {
            const auto device = Device.get();
            return NoticePlayable() && device && NoticeTargetReady(device);
        };
        update.show = [](const std::wstring& line)
        {
            UpdateNotice.Show(line);
            NoticeUntil = GetTickCount64() + 12000;
        };
        RWGTA::UpdateCheck::Init(std::move(update));
        // The game's normal WinMain return goes through the MSVC 7 runtime.
        if (const auto runtime = GetModuleHandleW(L"MSVCR71.dll"))
        {
            NativeExit = reinterpret_cast<decltype(NativeExit)>(GetProcAddress(runtime, "exit"));
            if (NativeExit)
                IATHook::Replace(GetModuleHandleW(nullptr), "MSVCR71.dll", std::forward_as_tuple("exit", Exit));
        }
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = [](IDirect3DDevice9* device)
        {
            UpdateNotice.BeforeReset();
            LARushOverlay::BeforeReset(device);
        };
        D3D9Options::HookCreation();
        auto device = hook::pattern("A1 ? ? ? ? F3 0F 11 44 24 6C 0F 57 C0 8D 4C 24 58");
        auto deviceAddress = *device.get_first<IDirect3DDevice9**>(1);
        if (deviceAddress)
            Device.SetAddress(deviceAddress);
        auto loop = hook::pattern("E8 ? ? ? ? 85 C0 0F 84 ? ? ? ? 8A 0D ? ? ? ? 53 B0 01 33 DB 3A C8");
        LoopHook = safetyhook::create_inline(loop.get_first(), Loop);
    }
}

class MainLoopEvents
{
public:
    MainLoopEvents()
    {
        WFP::onGameProcessEvent() += []()
        {
            if (!LARushWindow::DeviceAttached && LARushWindow::Device.is_initialized() && LARushWindow::Device.get())
            {
                D3D9Options::HookDevice(LARushWindow::Device.get());
                LARushWindow::DeviceAttached = true;
            }
        };
    }
} mainLoopEvents;
