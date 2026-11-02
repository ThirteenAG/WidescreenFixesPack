module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")


export module DeveloperOverlay;


export namespace SRSOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline bool HotkeyDown = false;
    inline bool Blocking = false;
    inline std::vector<uint8_t> HeldPads;
    inline SafetyHookInline PollHook;

    // The game's DirectInput poll: keyboard state at +144, mouse at +400, pads (80 bytes each) behind +420
    inline int __fastcall Poll(uint8_t* input, void*)
    {
        const int result = PollHook.thiscall<int>(input);
        const bool blocking = Overlay.BlocksInput();
        const int pads = std::max(0, *reinterpret_cast<int*>(input + 124) - 2);
        auto padStates = *reinterpret_cast<uint8_t**>(input + 420);
        if (blocking)
        {
            std::memset(input + 144, 0, 256);
            std::memset(input + 400, 0, 20);
            // Pads keep the state they had when the overlay opened, so axes stay centred
            if (padStates && HeldPads.size() == static_cast<size_t>(pads) * 80)
                std::memcpy(padStates, HeldPads.data(), HeldPads.size());
        }
        else if (padStates)
            HeldPads.assign(padStates, padStates + pads * 80);
        Blocking = blocking;
        return result;
    }

    inline void Init()
    {
        auto poll = hook::pattern("51 53 56 8B F1 8B 46 08 8B 08 57 50 FF 51 1C 8B 46 1C 8B 10 50 FF 52 1C");
        PollHook = safetyhook::create_inline(poll.get_first(), Poll);
    }

    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    inline std::string BuildTime()
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

    inline bool Message(HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
    {
        // The hotkey itself is read at the frame boundary
        if ((message == WM_KEYDOWN || message == WM_SYSKEYDOWN || message == WM_KEYUP || message == WM_SYSKEYUP) &&
            wParam == VK_F10 && ((GetAsyncKeyState(VK_CONTROL) & 0x8000) || HotkeyDown))
        {
            result = 0;
            return true;
        }
        return Overlay.Message(window, message, wParam, lParam, result);
    }

    inline bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        if (device == CurrentDevice)
            Overlay.BeforeReset();
    }

    inline void Present(IDirect3DDevice9* device, HWND window)
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
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !HotkeyDown)
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
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
