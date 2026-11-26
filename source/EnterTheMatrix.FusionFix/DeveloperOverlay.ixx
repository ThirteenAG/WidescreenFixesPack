module;

#include <stdafx.h>
#include <d3d9.h>
#include <dinput.h>
#include <float.h>
#include "FixSettings.hpp"
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D9.hpp"

#pragma comment(lib, "version.lib")

export module DeveloperOverlay;

namespace
{
    // The game leaves x87 and SSE rounding out of sync. The overlay's CRT conversions require both to agree.
    class OverlayRounding
    {
    public:
        OverlayRounding()
        {
            __control87_2(0, 0, &X87, &Sse);
            unsigned int x87{}, sse{};
            __control87_2(_RC_NEAR, _MCW_RC, &x87, &sse);
        }

        ~OverlayRounding()
        {
            unsigned int control{};
            __control87_2(X87, _MCW_RC, &control, nullptr);
            __control87_2(Sse, _MCW_RC, nullptr, &control);
        }

    private:
        unsigned int X87{}, Sse{};
    };
}

// Ctrl+F10 overlay
export namespace MatrixOverlay
{
    asi_overlay::D3D9Overlay Overlay;
    IDirect3DDevice9* CurrentDevice = nullptr;
    bool HotkeyDown = false;
    SafetyHookInline KeyboardHook, MouseHook, ControllerHook;
    uint32_t KeyboardState = 0, MouseState = 0;
    uint32_t ControllerState = 24, ControllerCount = 1, ControllerButtons = 588;
    DWORD OverlayPadSlot = XUSER_MAX_COUNT;
    bool ControllerSuppressed = false;

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

    bool OverlayChord(const XINPUT_GAMEPAD& pad)
    {
        constexpr WORD chord = XINPUT_GAMEPAD_BACK | XINPUT_GAMEPAD_START;
        return (pad.wButtons & chord) == chord;
    }

    bool Held(const XINPUT_GAMEPAD& pad)
    {
        return pad.wButtons != 0 || std::abs(int(pad.sThumbLX)) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbLY)) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbRX)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            std::abs(int(pad.sThumbRY)) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE ||
            pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD || pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
    }

    XINPUT_GAMEPAD ReadOverlayPad()
    {
        const auto getState = PadInput::XInput().getState;
        if (!getState)
            return {};
        XINPUT_STATE state{};
        if (OverlayPadSlot < XUSER_MAX_COUNT && getState(OverlayPadSlot, &state) == ERROR_SUCCESS)
            return state.Gamepad;
        OverlayPadSlot = XUSER_MAX_COUNT;

        for (DWORD slot = 0; slot < XUSER_MAX_COUNT; ++slot)
        {
            if (getState(slot, &state) == ERROR_SUCCESS &&
                (OverlayChord(state.Gamepad) || (Overlay.IsOpen() && Held(state.Gamepad))))
            {
                OverlayPadSlot = slot;
                return state.Gamepad;
            }
        }
        return {};
    }

    bool BlocksController()
    {
        const auto pad = ReadOverlayPad();
        ControllerSuppressed = Overlay.BlocksInput() || OverlayChord(pad) || (ControllerSuppressed && Held(pad));
        return ControllerSuppressed;
    }

    void UpdateOverlayPad()
    {
        const auto pad = ReadOverlayPad();
        PadInput::ToOverlay(Overlay, pad);
        if (!Overlay.BlocksInput() && !Held(pad))
            OverlayPadSlot = XUSER_MAX_COUNT;
    }

    int __fastcall Controller(uint8_t* self)
    {
        const int result = ControllerHook.thiscall<int>(self);
        if (BlocksController())
        {
            DIJOYSTATE2 neutral{};
            std::fill(std::begin(neutral.rgdwPOV), std::end(neutral.rgdwPOV), DWORD(-1));
            for (uint32_t i = 0; i < ControllerCount; ++i)
                std::memcpy(self + ControllerState + i * sizeof(neutral), &neutral, sizeof(neutral));
            std::memset(self + ControllerButtons, 0, sizeof(neutral.rgdwPOV));
        }
        return result;
    }

    void Init()
    {
        // The overlay's input hooks exist only in Debug builds
        if constexpr (!WFP::DeveloperOverlay)
            return;
        auto keyboard = hook::pattern("55 8B EC 83 EC 08 89 4D F8 8B 45 F8 05 ? ? 00 00 50 68 00 01 00 00");
        KeyboardState = *keyboard.get_first<uint32_t>(13);
        KeyboardHook = safetyhook::create_inline(keyboard.get_first(), Keyboard);
        auto mouse = hook::pattern("55 8B EC 83 EC 08 89 4D F8 8B 45 F8 05 ? ? 00 00 50 6A 14");
        MouseState = *mouse.get_first<uint32_t>(13);
        MouseHook = safetyhook::create_inline(mouse.get_first(), Mouse);
        auto controller = hook::pattern("55 8B EC 83 EC 10 89 4D F0 C7 45 FC 00 00 00 00 8B 45 F0 83 78 08 00 0F 84 ? ? 00 00 8B 4D F0 8B 51 08");
        if (controller.empty())
        {
            // The updated input object adds a second controller state and shifts the cached buttons.
            controller = hook::pattern("55 8B EC 83 EC 18 89 4D E8 C7 45 FC 00 00 00 00 8B 45 E8 83 78 08 00 0F 84 ? ? 00 00 8B 4D E8 8B 51 08");
            ControllerState = 28;
            ControllerCount = 2;
            ControllerButtons = 592;
        }
        ControllerHook = safetyhook::create_inline(controller.get_first(), Controller);
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
        if constexpr (!WFP::DeveloperOverlay)
            return false;
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
        if constexpr (!WFP::DeveloperOverlay)
            return;
        const OverlayRounding rounding;
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
        UpdateOverlayPad();
    }
}
