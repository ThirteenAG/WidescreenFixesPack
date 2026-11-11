module;
#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include "FixSettings.hpp"
#include "overlay/OverlayD3D9.hpp"

export module DeveloperOverlay;
import Window;


#pragma comment(lib, "version.lib")

export namespace PsiOpsOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
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
                ui.Value("Graphics API", "Direct3D 9");
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

    inline SafetyHookInline inputHook;

    inline int __fastcall Input(void* self, void*)
    {
        const auto result = inputHook.unsafe_thiscall<int>(self);
        if (BlocksInput())
            std::memset(static_cast<uint8_t*>(self) + 32, 0, 272);
        return result;
    }

    inline void Install()
    {
        auto input = hook::pattern("55 8B EC 83 EC 20 89 4D E0 8B 45 E0 BA 01 00 00 00 8B 48 08 D3 E2 89 55 FC");
        inputHook = safetyhook::create_inline(input.get_first(), Input);
        PsiOpsWindow::message = Message;
        PsiOpsWindow::blocksInput = BlocksInput;
        PsiOpsWindow::beforeReset = []() { Overlay.BeforeReset(); };
        PsiOpsWindow::frame = [](IDirect3DDevice9* device)
        {
            Keyboard(PsiOpsWindow::window);
            Draw(device);
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor") && !BlocksInput());
        };
    }
}

