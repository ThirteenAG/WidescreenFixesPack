module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <d3d9.h>
#include "Shared/D3D9Options.hpp"

export module Runtime;

import ComVars;
import FrameRate;
import DeveloperOverlay;
import Focus;
import MenuBindings;
import NativeMenu;
import InputDrain;
import WindowBindings;
import Window;
import Updates;
import IconBindings;
import ButtonIcons;
import SkipStart;

export namespace RERuntime
{
    IDirect3DDevice9* currentDevice = nullptr;
    HWND window = nullptr;

    void Frame(IDirect3DDevice9* device)
    {
        auto* native = sRender.get();
        if (!native || *reinterpret_cast<IDirect3DDevice9**>(native + 0x98) != device) return;
        if (currentDevice != device)
        {
            D3DDEVICE_CREATION_PARAMETERS parameters{};
            if (FAILED(device->GetCreationParameters(&parameters))) return;
            DWORD process = 0;
            GetWindowThreadProcessId(parameters.hFocusWindow, &process);
            if (process != GetCurrentProcessId()) return;
            currentDevice = device;
            window = parameters.hFocusWindow;
        }
        REOverlay::AttachWindow(window);
        REOverlay::Keyboard(window);
        REWindow::Frame(native, window, REOverlay::BlocksInput(), device);
        REIcons::Frame(device);
        REFrameRate::Frame(window);
        REOverlay::Draw(device);
    }

    void Install()
    {
        REWindow::Install();
        REInput::overlayBlocksInput = []() { return REOverlay::BlocksInput() || REWindow::BlocksInput(); };
        REInput::Install();
        RESkipStart::Install();
        REFocus::Install();
        REMenu::blocksInput = []() { return REOverlay::BlocksInput() || REWindow::BlocksInput(); };
        REMenu::Install(REOverlay::ThisModule());
        REUpdates::Install();
        REIcons::Install(REOverlay::ThisModule());
        auto current = hook::pattern("A1 ? ? ? ? 56 57 FF 71 04 66 0F 6E 88");
        auto original = hook::pattern("A1 ? ? ? ? 56 66 0F 6E 80");
        sRender.SetAddress(current.size() == 1 ? *current.get_first<uint8_t**>(1) : *original.get_first<uint8_t**>(1));
        REFrameRate::Install();
        {
            D3D9Options::frameLimit = 0;
            D3D9Options::frame = Frame;
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                REIcons::BeforeReset(device);
                REOverlay::BeforeReset(device);
            };
            D3D9Options::HookCreation();
        }
    }
}
