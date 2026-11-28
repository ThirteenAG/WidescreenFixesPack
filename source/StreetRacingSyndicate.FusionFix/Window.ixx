module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <commctrl.h>
#include <d3d9.h>
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

#pragma comment(lib, "comctl32.lib")

export module Window;

import ComVars;
import DeveloperOverlay;
import OptionsMenu;
import States;

export namespace SRSWindow
{
    inline int Mode = 0;
    inline int Width = 0, Height = 0;
    inline std::atomic<HWND> Window{};
    inline std::atomic<bool> PauseWhenUnfocused{ true };
    inline thread_local HWND DispatchWindow = nullptr;
    inline SafetyHookInline ActivateHook;
    inline void(__fastcall* SetActive)(void*, void*, char) = nullptr;
    inline LRESULT(__fastcall* Default)(void*, void*) = nullptr;
    inline bool CursorShown = false;
    inline std::function<void()> FrameCallback;
    inline void (*InputMessage)(UINT, LPARAM) = nullptr;
    inline void (__cdecl* NativeExit)(int) = nullptr;

    inline void __cdecl Exit(int code)
    {
        RWGTA::UpdateCheck::OnExit();
        NativeExit(code);
    }

    inline bool PauseOnFocusLoss()
    {
        return PauseWhenUnfocused.load(std::memory_order_relaxed);
    }

    // The game loop runs while the application's active flag (+173) is set and waits for messages otherwise
    inline void SyncActive(HWND window)
    {
        const bool active = !PauseOnFocusLoss() || GetForegroundWindow() == window;
        if (!!App.get()[173] != active)
            SetActive(App.get_ptr(), nullptr, active ? 1 : 0);
    }

    // The main frame's WM_ACTIVATEAPP handler minimizes the game and stops its loop. In window modes the window stays
    // as it is; the loop stops only with MAIN.PauseOnFocusLoss.
    inline void __fastcall Activate(uint8_t* frame, void* edx, BOOL active, DWORD thread)
    {
        if (DispatchWindow && DispatchWindow != Window.load())
        {
            Default(frame, nullptr);
            return;
        }
        if (!Mode)
            return ActivateHook.thiscall<void>(frame, active, thread);
        if (active || PauseOnFocusLoss())
            SetActive(App.get_ptr(), nullptr, active ? 1 : 0);
        // The game hides the cursor while it is active and shows it again when it is not
        if (CursorShown == !!active)
        {
            ShowCursor(active ? FALSE : TRUE);
            CursorShown = !active;
        }
        Default(frame, nullptr);
    }

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR)
    {
        const auto forward = [&]()
        {
            const auto previous = std::exchange(DispatchWindow, window);
            const auto result = DefSubclassProc(window, message, wParam, lParam);
            DispatchWindow = previous;
            return result;
        };
        if (message == WM_NCDESTROY)
        {
            RemoveWindowSubclass(window, Procedure, id);
            auto expected = window;
            if (Window.compare_exchange_strong(expected, nullptr))
                CursorConfinement::Suspend(true);
            return forward();
        }
        if (window != Window.load())
            return forward();

        CursorConfinement::Message(message, wParam);
        if (InputMessage)
            InputMessage(message, lParam);
        SRSMenu::Message(message, wParam, lParam);
        LRESULT result = 0;
        if (SRSOverlay::Message(window, message, wParam, lParam, result))
            return result;
        if (Mode && message != WM_ACTIVATEAPP)
            SyncActive(window);
        if (Mode && message == WM_SIZE && WindowRenderer.is_initialized() && WindowRenderer.get())
        {
            // The renderer asks for one minimize/restore after start and after a resolution change, to reset the
            // fullscreen device; a window keeps its device.
            WindowRenderer.get()[0x58] = 0;
        }
        if (Mode && message == WM_SYSCOMMAND && (wParam & 0xFFF0) == SC_MAXIMIZE)
            return 0;
        return forward();
    }

    inline bool IsGameDevice(IDirect3DDevice9* device);

    // Set by BeforeReset for the Reset that follows on the same thread
    inline thread_local bool ResettingOtherDevice = false;

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        ResettingOtherDevice = !IsGameDevice(device);
        SRSOverlay::BeforeReset(device);
    }

    inline void PresentParameters(D3DPRESENT_PARAMETERS* params, HWND window)
    {
        if (!Mode || (!window && std::exchange(ResettingOtherDevice, false)))
            return;
        params->Windowed = TRUE;
        params->FullScreen_RefreshRateInHz = 0;
        params->BackBufferWidth = Width;
        params->BackBufferHeight = Height;
    }

    inline void Place(HWND window)
    {
        const DWORD style = Mode == 2 ? WS_POPUP : WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
        SetWindowLongW(window, GWL_STYLE, static_cast<LONG>(style | WS_VISIBLE));
        const DWORD exStyle = static_cast<DWORD>(GetWindowLongW(window, GWL_EXSTYLE)) & ~WS_EX_TOPMOST;
        SetWindowLongW(window, GWL_EXSTYLE, static_cast<LONG>(exStyle));
        RECT frame{ 0, 0, Width, Height };
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
        // Centred client area; a client as big as the monitor covers it exactly with the frame outside
        const int x = bounds.left + (bounds.right - bounds.left - Width) / 2 + frame.left;
        const int y = bounds.top + (bounds.bottom - bounds.top - Height) / 2 + frame.top;
        WindowStyle::detail::AllowSize(window, frame.right - frame.left, frame.bottom - frame.top);
        SetWindowPos(window, HWND_NOTOPMOST, x, y, frame.right - frame.left, frame.bottom - frame.top, SWP_NOACTIVATE | SWP_FRAMECHANGED);
        using Attribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static const auto setAttribute = []()
        {
            const auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<Attribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const DWORD square = 1; // DWMWCP_DONOTROUND
        if (setAttribute)
            setAttribute(window, 33, &square, sizeof(square));
    }

    inline bool IsGameDevice(IDirect3DDevice9* device)
    {
        auto* renderer = WindowRenderer.is_initialized() ? WindowRenderer.get() : nullptr;
        if (!renderer)
            return false;
        // The renderer's D3D wrapper holds the current device used by its native Reset.
        auto* d3d = *reinterpret_cast<uint8_t**>(renderer + 0x0C);
        return device && d3d && device == *reinterpret_cast<IDirect3DDevice9**>(d3d + 0x1C);
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        if (!IsGameDevice(device))
            return;
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (FAILED(device->GetCreationParameters(&creation)) || !creation.hFocusWindow)
            return;
        const auto window = creation.hFocusWindow;
        if (window != Window.load())
        {
            if (!SetWindowSubclass(window, Procedure, 1, 0))
                return;
            Window.store(window);
            if (Mode)
                Place(window);
        }
        if (Mode)
            SyncActive(window);
        RWGTA::UpdateCheck::Tick();
        CursorConfinement::Suspend(SRSOverlay::BlocksInput() || GetForegroundWindow() != window);
        CursorConfinement::Update(window);
        if (FrameCallback)
            FrameCallback();
        SRSOverlay::Present(device, window);
    }

    inline void Init(int32_t& width, int32_t& height)
    {
        Mode = std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2);
        if (Mode == 2)
            std::tie(width, height) = GetDesktopRes();
        Width = width;
        Height = height;

        const auto updateCursor = []()
        {
            static std::mutex mutex;
            std::lock_guard lock(mutex);
            CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
        };
        FixSettings().OnChange("MAIN.ConfineCursor", updateCursor);
        updateCursor();

        auto activate = hook::pattern("8B 44 24 04 85 C0 56 8B F1 8D 4C 24 08 74");
        Default = reinterpret_cast<decltype(Default)>(injector::GetBranchDestination(activate.get_first(0x4E)).as_int());
        ActivateHook = safetyhook::create_inline(activate.get_first(), Activate);
        auto setActive = hook::pattern("8A 44 24 04 38 81 AD 00 00 00 74");
        SetActive = reinterpret_cast<decltype(SetActive)>(setActive.get_first());
        auto renderer = hook::pattern("A1 ? ? ? ? 85 C0 75 ? 56 68 74 73 27 00");
        WindowRenderer.SetAddress(*renderer.get_first<uint8_t**>(1));
        auto app = hook::pattern("53 33 DB 53 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? C7 05");
        App.SetAddress(*app.get_first<uint8_t*>(5));
        // Wakes the waiting game loop, which then follows the new setting
        const auto updatePause = []()
        {
            static std::mutex mutex;
            std::lock_guard lock(mutex);
            const auto pause = FixSettings().Get("MAIN.PauseOnFocusLoss");
            PauseWhenUnfocused.store(pause == "1", std::memory_order_relaxed);
            if (const auto window = Window.load())
                PostMessageW(window, WM_NULL, 0, 0);
        };
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", updatePause);
        updatePause();

        SRSStates::Init();
        SRSOverlay::Init();

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "StreetRacingSyndicate.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []()
        {
            const auto window = Window.load();
            return window && GetForegroundWindow() == window && SRSMenu::NoticeReady();
        };
        update.show = SRSMenu::ShowNotice;
        update.format = [](const std::string& latest, bool hub)
        {
            auto texts = FixSettings().GetTexts();
            texts.Select(SRSMenu::Language());
            auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            line = RWGTA::UpdateCheck::detail::Replace(line, "{name}", texts.Resolve(FixSettings().GetDescription().name));
            line = RWGTA::UpdateCheck::detail::Replace(line, "{version}", latest);
            return asi_settings::text::Widen(line);
        };
        RWGTA::UpdateCheck::Init(std::move(update));
        // The game's normal WinMain return goes through the MSVC 7 runtime.
        if (const auto runtime = GetModuleHandleW(L"MSVCR71.dll"))
        {
            NativeExit = reinterpret_cast<decltype(NativeExit)>(GetProcAddress(runtime, "exit"));
            if (NativeExit)
                IATHook::Replace(GetModuleHandleW(nullptr), "MSVCR71.dll", std::forward_as_tuple("exit", Exit));
        }

        D3D9Options::presentParameters = PresentParameters;
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = BeforeReset;
        D3D9Options::HookCreation();
    }
}
