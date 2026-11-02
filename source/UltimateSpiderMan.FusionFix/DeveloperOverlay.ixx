module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

export namespace USMOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline SafetyHookInline ActionHook, MouseHook;
    inline IDirect3DDevice9* Device = nullptr;
    inline LONG HotkeyDown = 0, Requested = 0;

    inline HMODULE Module()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Module), &module);
        return module;
    }

    inline std::string BuildTime()
    {
        std::array<wchar_t, 32768> path{};
        GetModuleFileNameW(Module(), path.data(), static_cast<DWORD>(path.size()));
        DWORD ignored = 0;
        const auto size = GetFileVersionInfoSizeW(path.data(), &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path.data(), 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    inline bool Message(HWND window, UINT message, WPARAM key, LPARAM parameter, LRESULT& result)
    {
        if (!ActionHook)
            return false;
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            key == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            if (message == WM_KEYUP || message == WM_SYSKEYUP)
                InterlockedExchange(&HotkeyDown, 0);
            else if (!(parameter & (1u << 30)) && GetForegroundWindow() == window &&
                !InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&Requested, 1);
            result = 0;
            return true;
        }
        if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !key))
        {
            InterlockedExchange(&Requested, 0);
            InterlockedExchange(&HotkeyDown, 0);
        }
        return Overlay.Message(window, message, key, parameter, result);
    }

    inline double __fastcall Action(uintptr_t bindings, void*, uint32_t action)
    {
        const auto value = ActionHook.thiscall<double>(bindings, action);
        return Overlay.BlocksInput() ? 0.0 : value;
    }

    inline char __fastcall Mouse(uintptr_t manager, void*, UINT message, WPARAM key, LPARAM parameter)
    {
        if (Overlay.BlocksInput() && ((message >= WM_KEYFIRST && message <= WM_KEYLAST) ||
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)))
            return 0;
        return MouseHook.thiscall<char>(manager, message, key, parameter);
    }

    inline void Init()
    {
        auto action = hook::pattern("51 8B 44 24 08 3B 01 72 0A D9 05 ? ? ? ? 59 C2 04 00 D9 05");
        ActionHook = safetyhook::create_inline(action.get_first(), Action);
        auto mouse = hook::pattern("8B 44 24 04 3D 02 02 00 00 77 30 3D 00 02 00 00 73 3E 83 F8 0F");
        MouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
        {
            static const auto build = BuildTime();
            ui.Value("Version", rsc_ProductVersion);
            ui.Value("BuildTimeUtc", build.c_str());
            ui.Value("Graphics API", "Direct3D 9");
            IDirect3DSurface9* surface = nullptr;
            if (Device && SUCCEEDED(Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &surface)))
            {
                D3DSURFACE_DESC desc{};
                surface->GetDesc(&desc);
                surface->Release();
                ui.Value("Resolution", std::to_string(desc.Width) + " x " + std::to_string(desc.Height));
            }
        });
    }

    inline void Frame(IDirect3DDevice9* device, HWND window)
    {
        if (!device || !window || !ActionHook)
            return;
        const bool down = GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down)
        {
            if (!InterlockedCompareExchange(&HotkeyDown, 1, 0))
                InterlockedExchange(&Requested, 1);
        }
        else
            InterlockedExchange(&HotkeyDown, 0);
        const bool toggle = InterlockedExchange(&Requested, 0) && GetForegroundWindow() == window;
        Device = device;
        if (toggle && Overlay.Initialize(window, Module()))
            Overlay.SetOpen(!Overlay.IsOpen());
        if (Overlay.IsOpen())
            Overlay.Present(device, Module());
    }
}
