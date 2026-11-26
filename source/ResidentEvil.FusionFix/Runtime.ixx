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
import Video;

export namespace RERuntime
{
    void Frame(IDirect3DDevice9* device)
    {
        auto* native = sRender.get();
        if (!native || *reinterpret_cast<IDirect3DDevice9**>(native + 0x98) != device) return;
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (FAILED(device->GetCreationParameters(&parameters))) return;
        const auto window = parameters.hFocusWindow;
        DWORD process = 0;
        GetWindowThreadProcessId(window, &process);
        if (process != GetCurrentProcessId()) return;
        REVideo::Frame();
        REOverlay::AttachWindow(window);
        REOverlay::Keyboard(window);
        REWindow::Frame(native, window, REOverlay::BlocksInput());
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
        REVideo::Install();
        auto current = hook::pattern("A1 ? ? ? ? 56 57 FF 71 04 66 0F 6E 88");
        auto original = hook::pattern("A1 ? ? ? ? 56 66 0F 6E 80");
        sRender.SetAddress(current.size() == 1 ? *current.get_first<uint8_t**>(1) : *original.get_first<uint8_t**>(1));
        REFrameRate::Install();
        {
            D3D9Options::frameLimit = 0;
            D3D9Options::frame = Frame;
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                REVideo::Frame();
                REIcons::BeforeReset(device);
                REOverlay::BeforeReset(device);
            };
            D3D9Options::HookCreation();
        }
    }
}
