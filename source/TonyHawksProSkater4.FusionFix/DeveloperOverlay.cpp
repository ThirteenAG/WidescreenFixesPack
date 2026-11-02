#include "stdafx.h"
#include "FixSettings.hpp"
#include "DeveloperOverlay.hpp"
#include "overlay/OverlayD3D9.hpp"

import ComVars;

#pragma comment(lib, "version.lib")

namespace
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    SafetyHookInline Create9Hook, CreateDeviceHook, ResetHook, KeyboardHook;
    bool HotkeyDown = false;

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

    HRESULT WINAPI CreateDevice(IDirect3D9* d3d, UINT adapter, D3DDEVTYPE type, HWND window, DWORD behavior,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        const auto result = CreateDeviceHook.unsafe_stdcall<HRESULT>(d3d, adapter, type, window, behavior, parameters, device);
        if (SUCCEEDED(result) && device && *device)
        {
            CurrentDevice = *device;
            if (!ResetHook)
                ResetHook = safetyhook::create_inline((*reinterpret_cast<void***>(*device))[16], Reset);
        }
        return result;
    }

    IDirect3D9* WINAPI Create9(UINT version)
    {
        auto result = Create9Hook.unsafe_stdcall<IDirect3D9*>(version);
        if (result && !CreateDeviceHook)
            CreateDeviceHook = safetyhook::create_inline((*reinterpret_cast<void***>(result))[16], CreateDevice);
        return result;
    }

    // The game reads the keyboard through DirectInput each frame; the open overlay keeps the keys for itself
    int __cdecl ReadKeyboard()
    {
        const auto result = KeyboardHook.unsafe_ccall<int>();
        if (Overlay.BlocksInput())
            std::memset(KeyBuffer.get().data() + 256 * (KeyBufferIndex.get() & 1), 0, 256);
        return result;
    }
}

void InitDeveloperOverlay()
{
    auto keyboard = hook::pattern("A1 ? ? ? ? 57 83 F8 02 C7 05 ? ? ? ? 00 00 00 00 75 18 B9 80 00 00 00 33 C0 BF");
    KeyBufferIndex.SetAddress(*keyboard.get_first<int32_t*>(1));
    KeyBuffer.SetAddress(*keyboard.get_first<uint8_t*>(29));
    KeyboardHook = safetyhook::create_inline(keyboard.get_first(), ReadKeyboard);
    CallbackHandler::RegisterCallback(L"d3d9.dll", []()
    {
        if (auto create = GetProcAddress(GetModuleHandleW(L"d3d9.dll"), "Direct3DCreate9"))
            Create9Hook = safetyhook::create_inline(create, Create9);
    });
}

void DrawDeveloperOverlay(HWND window)
{
    if (!CurrentDevice)
        return;
    static const bool page = []()
    {
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto buildTime = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", buildTime.c_str());
            ui.Value("Graphics API", "Direct3D 8 (on Direct3D 9)");
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
    // The hotkey is read here too: the game's DirectInput keyboard can take the key messages
    const bool down = window && GetForegroundWindow() == window &&
        (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
    if (down && !HotkeyDown)
        Overlay.SetOpen(!Overlay.IsOpen());
    HotkeyDown = down;
    Overlay.Present(CurrentDevice, ThisModule());
}

bool DeveloperOverlayMessage(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
{
    if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
        wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
    {
        result = 0;
        return true;
    }
    return Overlay.Message(window, message, wParam, lParam, result);
}
