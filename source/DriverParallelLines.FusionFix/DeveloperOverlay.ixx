module;

#include "stdafx.h"
#include <d3d9.h>
#include <mutex>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")


export module DeveloperOverlay;

import ComVars;
import FrameRate;
import Rendering;

export namespace DPLOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    SafetyHookInline ResetHook, PresentHook, SwapChainHook;
    thread_local bool Presenting = false;
    std::mutex HotkeyMutex;
    HWND HotkeyWindow = nullptr;
    bool HotkeyFocused = false, HotkeyDown = false, HotkeyRequested = false;

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

    HRESULT WINAPI Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* parameters)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
        return ResetHook.unsafe_stdcall<HRESULT>(device, parameters);
    }

    bool ChordDown()
    {
        return (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
    }

    void HotkeyFocus(bool focused)
    {
        HotkeyFocused = focused;
        HotkeyDown = focused && ChordDown();
        HotkeyRequested = false;
    }

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if constexpr (!WFP::DeveloperOverlay)
            return false;
        {
            const std::lock_guard lock(HotkeyMutex);
            if (window == HotkeyWindow)
            {
                if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam) ||
                    (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE) || message == WM_NCDESTROY)
                {
                    HotkeyFocus(false);
                    if (message == WM_NCDESTROY)
                        HotkeyWindow = nullptr;
                }
                else if (!HotkeyFocused && (message == WM_SETFOCUS || message == WM_ACTIVATEAPP || message == WM_ACTIVATE))
                    HotkeyFocus(true);
                const bool pressed = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
                const bool released = message == WM_KEYUP || message == WM_SYSKEYUP;
                if (HotkeyFocused && GetForegroundWindow() == window && wParam == VK_F10 && (pressed || released) &&
                    ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
                {
                    if (released)
                        HotkeyDown = false;
                    else if (!(lParam & (1u << 30)) && !std::exchange(HotkeyDown, true))
                        HotkeyRequested = true;
                    result = 0;
                    return true;
                }
            }
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    void Keyboard(HWND window)
    {
        if constexpr (!WFP::DeveloperOverlay)
            return;
        bool toggle;
        {
            const std::lock_guard lock(HotkeyMutex);
            const bool focused = window && GetForegroundWindow() == window;
            if (window != HotkeyWindow || focused != HotkeyFocused)
            {
                HotkeyWindow = window;
                HotkeyFocus(focused);
            }
            const bool down = focused && ChordDown();
            if (down && !HotkeyDown)
                HotkeyRequested = true;
            HotkeyDown = down;
            toggle = std::exchange(HotkeyRequested, false) && focused;
        }
        if (toggle)
            Overlay.SetOpen(!Overlay.IsOpen());
    }

    constexpr wchar_t WindowProperty[] = L"DriverParallelLines.FusionFix.Overlay.Window";
    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto original = reinterpret_cast<WNDPROC>(GetPropW(window, WindowProperty));
        if (!original) return DefWindowProcW(window, message, wParam, lParam);
        LRESULT result = 0;
        if (Message(window, message, wParam, lParam, result)) return result;
        if (message == WM_NCDESTROY) RemovePropW(window, WindowProperty);
        return CallWindowProcW(original, window, message, wParam, lParam);
    }
    void AttachWindow(HWND window)
    {
        if constexpr (!WFP::DeveloperOverlay)
            return;
        if (!window || GetPropW(window, WindowProperty)) return;
        auto original = GetWindowLongPtrW(window, GWLP_WNDPROC);
        if (!original || !SetPropW(window, WindowProperty, reinterpret_cast<HANDLE>(original))) return;
        SetLastError(0);
        if (!SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)) && GetLastError())
            RemovePropW(window, WindowProperty);
    }

    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    void Draw(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        static const bool page = []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 9");
                ui.Value("Frame rate", std::to_string(static_cast<int>(std::lround(DPLFrameRate::actualFPS))));
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
        if constexpr (WFP::DeveloperOverlay)
            Overlay.Present(device, ThisModule());
    }

    HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (device == CurrentDevice)
        {
            DPLFrameRate::Frame(hWnd.get());
            DrawCutsceneBorders(device);
            Draw(device);
        }
        const bool previous = std::exchange(Presenting, true);
        const auto result = PresentHook.unsafe_stdcall<HRESULT>(device, source, destination, window, dirty);
        Presenting = previous;
        return result;
    }

    HRESULT WINAPI SwapChainPresent(IDirect3DSwapChain9* chain, const RECT* source, const RECT* destination,
        HWND window, const RGNDATA* dirty, DWORD flags)
    {
        IDirect3DDevice9* device = nullptr;
        if (!Presenting && SUCCEEDED(chain->GetDevice(&device)))
        {
            if (device == CurrentDevice)
            {
                DPLFrameRate::Frame(hWnd.get());
                DrawCutsceneBorders(device);
                Draw(device);
            }
            device->Release();
        }
        return SwapChainHook.unsafe_stdcall<HRESULT>(chain, source, destination, window, dirty, flags);
    }

    void Attach(IDirect3DDevice9* device)
    {
        if (!device)
            return;
        if (PresentHook)
        {
            CurrentDevice = device;
            return;
        }
        auto table = *reinterpret_cast<void***>(device);
        IDirect3DSwapChain9* chain = nullptr;
        if (FAILED(device->GetSwapChain(0, &chain)))
            return;
        auto chainTable = *reinterpret_cast<void***>(chain);
        if constexpr (WFP::DeveloperOverlay)
            ResetHook = safetyhook::create_inline(table[16], Reset);
        PresentHook = safetyhook::create_inline(table[17], Present);
        SwapChainHook = safetyhook::create_inline(chainTable[3], SwapChainPresent);
        chain->Release();
        CurrentDevice = device;
    }

}
