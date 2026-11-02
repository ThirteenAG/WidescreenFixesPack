module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "RawMouse.hpp"
#include <d3d11.h>
#include "Shared/PadInput.hpp"
#include "overlay/OverlayD3D11.hpp"

#pragma comment(lib, "version.lib")

export module DevOverlay;

import ComVars;

// The developer overlay (Ctrl+F10, or Back + Start on a pad), drawn after the game's frame. It gets the window's
// messages before the game: while it's open, the game's raw keyboard and mouse input and its pad stay at rest.
namespace
{
    asi_overlay::D3D11Overlay Overlay;

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
        auto size = GetFileVersionInfoSizeW(path, &ignored);
        std::vector<uint8_t> data(size);
        if (!size || !GetFileVersionInfoW(path, 0, size, data.data()))
            return "Unknown";
        wchar_t* value = nullptr;
        UINT length = 0;
        if (!VerQueryValueW(data.data(), L"\\StringFileInfo\\040004b0\\BuildTimeUtc", reinterpret_cast<void**>(&value), &length) || !value)
            return "Unknown";
        return asi_settings::text::Narrow(value);
    }

    // The game window's procedure, after the overlay's
    HWND Window = nullptr;
    WNDPROC GameWindowProc = nullptr;

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        LRESULT result = 0;
        if (Overlay.Message(window, message, wParam, lParam, result))
            return result;
        RawMouse::Message(window, message, wParam, lParam);
        return CallWindowProcA(GameWindowProc, window, message, wParam, lParam);
    }

    void Subclass(HWND window)
    {
        if (!window || window == Window || !IsWindow(window))
            return;
        Window = window;
        GameWindowProc = reinterpret_cast<WNDPROC>(SetWindowLongA(window, GWL_WNDPROC, reinterpret_cast<LONG>(&WindowProc)));
    }

    // The game's pad: the overlay's buttons, and a pad at rest while it's open
    decltype(&XInputGetState) GameXInputGetState = nullptr;

    DWORD WINAPI XInputGetStateHook(DWORD userIndex, XINPUT_STATE* state)
    {
        if (!GameXInputGetState)
            return ERROR_DEVICE_NOT_CONNECTED;
        auto result = GameXInputGetState(userIndex, state);
        if (result != ERROR_SUCCESS || !state || userIndex != 0)
            return result;
        auto& pad = state->Gamepad;
        // A B X Y, LB RB, Back Start, LS RS and the d-pad in the overlay's bits
        static const std::pair<WORD, uint16_t> mapping[] =
        {
            { XINPUT_GAMEPAD_A, 0x4000 }, { XINPUT_GAMEPAD_B, 0x2000 }, { XINPUT_GAMEPAD_X, 0x8000 }, { XINPUT_GAMEPAD_Y, 0x1000 },
            { XINPUT_GAMEPAD_LEFT_SHOULDER, 0x400 }, { XINPUT_GAMEPAD_RIGHT_SHOULDER, 0x800 }, { XINPUT_GAMEPAD_BACK, 0x1 },
            { XINPUT_GAMEPAD_START, 0x8 }, { XINPUT_GAMEPAD_LEFT_THUMB, 0x2 }, { XINPUT_GAMEPAD_RIGHT_THUMB, 0x4 },
            { XINPUT_GAMEPAD_DPAD_UP, 0x10 }, { XINPUT_GAMEPAD_DPAD_RIGHT, 0x20 }, { XINPUT_GAMEPAD_DPAD_DOWN, 0x40 },
            { XINPUT_GAMEPAD_DPAD_LEFT, 0x80 },
        };
        uint16_t buttons = 0;
        for (auto& [button, bit] : mapping)
        {
            if (pad.wButtons & button)
                buttons |= bit;
        }
        Overlay.Gamepad(buttons, int8_t(std::clamp(pad.sThumbLX / 258, -127, 127)), int8_t(std::clamp(-pad.sThumbLY / 258, -127, 127)));
        if (Overlay.BlocksInput())
            pad = {};
        return result;
    }
}

export namespace DeveloperOverlay
{
    bool BlocksInput()
    {
        return Overlay.BlocksInput();
    }

    // After the game's frame, before it's presented
    void Present(IDXGISwapChain* swapChain)
    {
        Overlay.Present(swapChain, ThisModule());
    }
}

class DevOverlay
{
public:
    DevOverlay()
    {
        WFP::onInitEvent() += []()
        {
            asi_overlay::Overlay::AddPage("Debug", "Info", [](asi_overlay::OverlayUI& ui)
            {
                static const auto buildTime = BuildTime();
                ui.Value("Version", rsc_ProductVersion);
                ui.Value("BuildTimeUtc", buildTime.c_str());
                ui.Value("Graphics API", "Direct3D 11");
                RECT client{};
                if (Window && GetClientRect(Window, &client))
                    ui.Value("Resolution", std::to_string(client.right) + " x " + std::to_string(client.bottom));
            });

            GameXInputGetState = PadInput::XInput().getState;
            auto originals = IATHook::Replace(GetModuleHandleA(NULL), "xinput9_1_0.dll", std::forward_as_tuple("XInputGetState", XInputGetStateHook));
            if (originals.contains("XInputGetState"))
                GameXInputGetState = reinterpret_cast<decltype(GameXInputGetState)>(originals["XInputGetState"].get());

            WFP::onGameProcessEvent() += []()
            {
                Subclass(hWnd.get());
            };

            WFP::onShutdownEvent() += []()
            {
                Overlay.ShutdownRenderer();
            };
        };
    }
} DevOverlay;
