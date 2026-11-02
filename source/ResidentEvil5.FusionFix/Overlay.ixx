module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/WindowCursor.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module Overlay;

import FrameRate;

// The developer overlay (Ctrl+F10, handled by the overlay in the window procedure) on the game's Direct3D 9 device. While it has the keyboard and mouse, the game's
// input sees its window as unfocused, as it does when another window is active: it reads nothing.

namespace
{
    asi_overlay::D3D9Overlay Instance;
    IDirect3DDevice9* CurrentDevice = nullptr;
    float FrameRate = 0.0f;
    std::atomic<HWND> CursorWindow{ nullptr };

    BOOL WINAPI NativeClip(const RECT*)
    {
        return TRUE;
    }

    HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    std::string BuildTime()
    {
        wchar_t path[MAX_PATH]{};
        GetModuleFileNameW(ThisModule(), path, DWORD(std::size(path)));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    constexpr wchar_t WindowProperty[] = L"ResidentEvil5.FusionFix.Overlay.Window";
    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto original = reinterpret_cast<WNDPROC>(GetPropW(window, WindowProperty));
        if (!original)
            return DefWindowProcW(window, message, wParam, lParam);
        if (message == WM_NULL && CursorWindow.load() != window)
        {
            WindowCursor::Attach(window);
            if (WindowCursor::Window == window)
                CursorWindow.store(window);
        }
        if (message == WM_NCDESTROY && CursorWindow.load() == window)
            CursorWindow.store(nullptr);
        LRESULT result = 0;
        if (Instance.Message(window, message, wParam, lParam, result))
            return result;
        if (message == WM_NCDESTROY)
            RemovePropW(window, WindowProperty);
        return CallWindowProcW(original, window, message, wParam, lParam);
    }

    void AttachWindow(HWND window)
    {
        if (!window || GetPropW(window, WindowProperty))
            return;
        auto original = GetWindowLongPtrW(window, GWLP_WNDPROC);
        if (!original || !SetPropW(window, WindowProperty, reinterpret_cast<HANDLE>(original)))
            return;
        SetLastError(0);
        if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
            RemovePropW(window, WindowProperty);
    }

    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (!device || FAILED(device->GetCreationParameters(&parameters)))
            return;
        const auto window = parameters.hFocusWindow;
        DWORD process = 0;
        GetWindowThreadProcessId(window, &process);
        if (process != GetCurrentProcessId())
            return;
        FrameRate::Frame(window);
        AttachWindow(window);
        if (CursorWindow.load() != window)
            PostMessageW(window, WM_NULL, 0, 0);
        CursorConfinement::Suspend(Instance.BlocksInput());

        static LARGE_INTEGER frequency{}, start{};
        static uint32_t frames = 0;
        LARGE_INTEGER now{};
        if (!frequency.QuadPart)
            QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&now);
        ++frames;
        if (!start.QuadPart)
            start = now, frames = 0;
        else if (now.QuadPart - start.QuadPart >= frequency.QuadPart)
        {
            FrameRate = float(frames * double(frequency.QuadPart) / double(now.QuadPart - start.QuadPart));
            start = now;
            frames = 0;
        }
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                ui.Value("Frame rate", std::to_string(int(std::lround(FrameRate))));
                IDirect3DSurface9* surface = nullptr;
                if (CurrentDevice && SUCCEEDED(CurrentDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
                {
                    D3DSURFACE_DESC desc{};
                    surface->GetDesc(&desc);
                    surface->Release();
                    ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
                }
            });
            return true;
        }();
        CurrentDevice = device;
        Instance.Present(device, ThisModule());
    }

    // The game's "window has focus" test (minimized, foreground, active); the input reads nothing without it
    bool(__fastcall* FocusTest)(uintptr_t, void*) = nullptr;
    SafetyHookInline HasFocusHook;
    bool __fastcall HasFocus(uintptr_t main, void*)
    {
        return !Instance.BlocksInput() && HasFocusHook.unsafe_thiscall<bool>(main);
    }
}

export namespace Overlay
{
    // The game's own focus test without the overlay's block (for the pause checks)
    bool HasFocus(uintptr_t main)
    {
        return HasFocusHook ? HasFocusHook.unsafe_thiscall<bool>(main) : FocusTest(main, nullptr);
    }

    bool BlocksInput()
    {
        return Instance.BlocksInput();
    }

    void Init()
    {
        auto focus = hook::pattern("80 B9 48 05 01 00 00 74 03 32 C0 C3 80 B9 49 05 01 00 00");
        FocusTest = focus.get_first<bool __fastcall(uintptr_t, void*)>();
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = [](IDirect3DDevice9* device)
        {
            if (device == CurrentDevice)
                Instance.BeforeReset();
        };
        if (!D3D9Options::HookCreation())
            return;
        WindowCursor::Initialize(FixSettings());
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", NativeClip));
        HasFocusHook = safetyhook::create_inline(focus.get_first(), ::HasFocus);
    }
}
