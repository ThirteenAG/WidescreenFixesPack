module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"


export module Window;

import ComVars;
import DeveloperOverlay;
import States;

export namespace SRSWindow
{
    inline int Mode = 0;
    inline int Width = 0, Height = 0;
    inline HWND Window = nullptr;
    inline WNDPROC PreviousProcedure = nullptr;
    inline SafetyHookInline ActivateHook;
    inline void(__fastcall* SetActive)(void*, void*, char) = nullptr;
    inline LRESULT(__fastcall* Default)(void*, void*) = nullptr;
    inline bool CursorShown = false;
    inline std::function<void()> FrameCallback;

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline bool PauseOnFocusLoss()
    {
        return FixSettings().Bool("MAIN.PauseOnFocusLoss");
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

    inline LRESULT CALLBACK Procedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        CursorConfinement::Message(message, wParam);
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
        return CallWindowProcA(PreviousProcedure, window, message, wParam, lParam);
    }

    inline void PresentParameters(D3DPRESENT_PARAMETERS* params, HWND)
    {
        if (!Mode)
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

    inline void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS creation{};
        if (FAILED(device->GetCreationParameters(&creation)) || !creation.hFocusWindow)
            return;
        const auto window = creation.hFocusWindow;
        if (window != Window)
        {
            Window = window;
            PreviousProcedure = reinterpret_cast<WNDPROC>(SetWindowLongPtrA(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(Procedure)));
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

        const auto updateCursor = []() { CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor")); };
        updateCursor();
        FixSettings().OnChange("MAIN.ConfineCursor", updateCursor);

        auto activate = hook::pattern("8B 44 24 04 85 C0 56 8B F1 8D 4C 24 08 74");
        Default = reinterpret_cast<decltype(Default)>(injector::GetBranchDestination(activate.get_first(0x4E)).as_int());
        ActivateHook = safetyhook::create_inline(activate.get_first(), Activate);
        auto setActive = hook::pattern("8A 44 24 04 38 81 AD 00 00 00 74");
        SetActive = reinterpret_cast<decltype(SetActive)>(setActive.get_first());
        auto renderer = hook::pattern("A1 ? ? ? ? 85 C0 75 ? 56 68 74 73 27 00");
        if (auto* rendererAddress = *renderer.get_first<uint8_t**>(1))
            WindowRenderer.SetAddress(rendererAddress);
        auto app = hook::pattern("53 33 DB 53 B9 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ? C7 05");
        App.SetAddress(*app.get_first<uint8_t*>(5));
        // Wakes the waiting game loop, which then follows the new setting
        FixSettings().OnChange("MAIN.PauseOnFocusLoss", []() { if (Window) PostMessageW(Window, WM_NULL, 0, 0); });

        SRSStates::Init();
        SRSOverlay::Init();

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "StreetRacingSyndicate.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return Window && GetForegroundWindow() == Window && SRSStates::Active("MainMenuState"); };
        update.show = [](const std::wstring& line)
        {
            MessageBoxW(Window, line.c_str(), L"Street Racing Syndicate Fusion Fix", MB_OK | MB_ICONINFORMATION);
        };
        RWGTA::UpdateCheck::Init(std::move(update));
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));

        D3D9Options::presentParameters = PresentParameters;
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = SRSOverlay::BeforeReset;
        D3D9Options::HookCreation();
    }
}
