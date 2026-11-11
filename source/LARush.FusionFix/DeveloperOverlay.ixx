module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"

export module DeveloperOverlay;
import ComVars;


#pragma comment(lib, "version.lib")

export namespace LARushOverlay
{
    inline asi_overlay::D3D9Overlay Overlay;
    inline IDirect3DDevice9* CurrentDevice = nullptr;
    inline bool HotkeyDown = false;
    inline bool PausedByOverlay = false;
    inline bool (*FocusPauseRequested)() = nullptr;
    inline void* SetGamePaused = nullptr;
    inline SafetyHookInline InputHook;

    inline int __cdecl Input(int player, uint8_t* state)
    {
        const int result = InputHook.ccall<int>(player, state);
        if (state && Overlay.BlocksInput())
            std::memset(state, 0, 36); // Preserve the native device-present byte.
        return result;
    }

    inline void Pause(bool enabled)
    {
        const int value = enabled ? 1 : 0;
        __asm
        {
            push esi
            mov esi, value
            call dword ptr [SetGamePaused]
            pop esi
        }
    }

    inline void Init()
    {
        auto pause = hook::pattern("56 89 35 ? ? ? ? E8 ? ? ? ? 83 C4 04 FF 15 ? ? ? ? 85 F6 F3 0F 10 05");
        SetGamePaused = pause.get_first();
        GamePaused.SetAddress(*pause.get_first<int32_t*>(3));
        auto input = hook::pattern("68 ? ? ? ? 56 89 35 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 83 C4 08 3C 28");
        const auto inputCall = input.get_first<uint8_t>(12);
        InputHook = safetyhook::create_inline(inputCall + 5 + *reinterpret_cast<int32_t*>(inputCall + 1), Input);
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
        // The hotkey itself is read at the frame boundary: the game's DirectInput keyboard can take the key messages
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

    inline void Keyboard(HWND window)
    {
        const bool down = window && GetForegroundWindow() == window &&
            (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_F10) & 0x8000);
        if (down && !HotkeyDown)
            Overlay.SetOpen(!Overlay.IsOpen());
        HotkeyDown = down;
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
        Keyboard(window);
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
        const bool blocking = Overlay.BlocksInput();
        if (blocking && !PausedByOverlay && GamePaused.is_initialized() && !GamePaused)
        {
            Pause(true);
            PausedByOverlay = true;
        }
        else if (!blocking && PausedByOverlay && !(FocusPauseRequested && FocusPauseRequested()))
        {
            if (GamePaused.is_initialized() && GamePaused)
                Pause(false);
            PausedByOverlay = false;
        }
    }
}
