module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

// Ctrl+F10 overlay
export namespace MatrixOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    bool HotkeyDown = false;
    SafetyHookInline KeyboardHook, MouseHook;
    uint32_t KeyboardState = 0, MouseState = 0;

    // The game reads the keyboard and mouse through DirectInput; while the overlay has them, the game sees nothing
    int __fastcall Keyboard(uint8_t* self)
    {
        const int result = KeyboardHook.thiscall<int>(self);
        if (Overlay.BlocksInput())
            std::memset(self + KeyboardState, 0, 256);
        return result;
    }

    int __fastcall Mouse(uint8_t* self)
    {
        const int result = MouseHook.thiscall<int>(self);
        if (Overlay.BlocksInput())
            std::memset(self + MouseState, 0, 20);
        return result;
    }

    void Init()
    {
        auto keyboard = hook::pattern("55 8B EC 83 EC 08 89 4D F8 8B 45 F8 05 ? ? 00 00 50 68 00 01 00 00");
        KeyboardState = *keyboard.get_first<uint32_t>(13);
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        auto mouse = hook::pattern("55 8B EC 83 EC 08 89 4D F8 8B 45 F8 05 ? ? 00 00 50 6A 14");
        MouseState = *mouse.get_first<uint32_t>(13);
        MouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
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

    bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        // The hotkey itself is read at the frame boundary: the game's DirectInput keyboard can take the key messages
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    void Hotkey(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !HotkeyDown)
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
    }

    void BeforeReset(IDirect3DDevice9* device)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
    }

    void Present(IDirect3DDevice9* device, HWND window)
    {
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
        (void)page;
        Hotkey(window);
        CurrentDevice = device;
        Overlay.Present(device, ThisModule());
        XINPUT_STATE state{};
        DWORD slot = 0;
        const bool present = PadInput::Read(state, slot);
        uint16_t mapped = 0;
        if (present)
        {
            const auto buttons = state.Gamepad.wButtons;
            const auto map = [&](WORD source, uint16_t target) { if (buttons & source) mapped |= target; };
            map(XINPUT_GAMEPAD_BACK, 0x1);
            map(XINPUT_GAMEPAD_START, 0x8);
            map(XINPUT_GAMEPAD_DPAD_UP, 0x10);
            map(XINPUT_GAMEPAD_DPAD_RIGHT, 0x20);
            map(XINPUT_GAMEPAD_DPAD_DOWN, 0x40);
            map(XINPUT_GAMEPAD_DPAD_LEFT, 0x80);
            map(XINPUT_GAMEPAD_LEFT_SHOULDER, 0x400);
            map(XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x800);
            map(XINPUT_GAMEPAD_Y, 0x1000);
            map(XINPUT_GAMEPAD_B, 0x2000);
            map(XINPUT_GAMEPAD_A, 0x4000);
            map(XINPUT_GAMEPAD_X, 0x8000);
        }
        Overlay.Gamepad(mapped, present ? static_cast<int8_t>(state.Gamepad.sThumbLX / 256) : 0,
            present ? static_cast<int8_t>(-state.Gamepad.sThumbLY / 256) : 0);
    }
}
