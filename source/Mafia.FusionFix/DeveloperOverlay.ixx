module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

import FrameRate;

// Ctrl+F10 overlay on the Direct3D 9 device (d3d8to9)
export namespace MafiaOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    SafetyHookInline ResetHook, PresentHook, SwapChainHook;
    thread_local bool Presenting = false;
    LONG HotkeyDown = 0, HotkeyRequested = 0;

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

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                InterlockedExchange(&HotkeyDown, 0);
            else if (!(lParam & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    void Keyboard(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&HotkeyRequested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        if (InterlockedExchange(&HotkeyRequested, 0) && window && GetForegroundWindow() == window)
            Overlay.SetOpen(!Overlay.IsOpen());
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
                ui.Value("Graphics API", "Direct3D 8 / 9");
                ui.Value("Frame rate", std::to_string(static_cast<int>(std::lround(GetFrameRate()))));
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
        Overlay.Present(device, ThisModule());
    }

    HRESULT WINAPI Present(IDirect3DDevice9* device, const RECT* source, const RECT* destination, HWND window, const RGNDATA* dirty)
    {
        if (device == CurrentDevice)
            Draw(device);
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
                Draw(device);
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
        ResetHook = safetyhook::create_inline(table[16], Reset);
        PresentHook = safetyhook::create_inline(table[17], Present);
        SwapChainHook = safetyhook::create_inline(chainTable[3], SwapChainPresent);
        chain->Release();
        CurrentDevice = device;
    }
}
