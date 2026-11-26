module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/WindowCursor.hpp"

export module Runtime;

import ComVars;
import FrameRate;
import Camera;
import DeveloperOverlay;
import ControllerIcons;

// Once per frame, before the game's Present:
// - MAIN.CutsceneBorders: cutscenes are framed for 16:9. On wider screens Pillarbox (2) covers what lies outside the
//   16:9 picture with black bars, so the cutscene keeps its original framing; Off (0) shows the wider view.
// - The developer overlay (Ctrl+F10).

namespace
{
    void CutsceneBorders(IDirect3DDevice9* device)
    {
        if (!ShouldPillarboxCutscene())
            return;
        IDirect3DSurface9* backBuffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
            return;
        D3DSURFACE_DESC desc{};
        if (SUCCEEDED(backBuffer->GetDesc(&desc)))
        {
            const auto width = static_cast<LONG>(desc.Width), height = static_cast<LONG>(desc.Height);
            const auto picture = static_cast<LONG>(std::lround(height * defaultAspectRatio));
            const auto left = (width - picture) / 2;
            if (left > 0)
            {
                const RECT leftBar{ 0, 0, left, height };
                const RECT rightBar{ left + picture, 0, width, height };
                // Draw directly to the backbuffer without changing the game's active targets or state.
                if (SUCCEEDED(device->ColorFill(backBuffer, &leftBar, D3DCOLOR_XRGB(0, 0, 0))))
                    device->ColorFill(backBuffer, &rightBar, D3DCOLOR_XRGB(0, 0, 0));
            }
        }
        backBuffer->Release();
    }

    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (!device || FAILED(device->GetCreationParameters(&parameters)))
            return;
        DWORD process = 0;
        GetWindowThreadProcessId(parameters.hFocusWindow, &process);
        if (process != GetCurrentProcessId())
            return;
        RevIcons::Frame(device);
        RevOverlay::AttachWindow(parameters.hFocusWindow);
        if (RevOverlay::cursorWindow.load() != parameters.hFocusWindow)
            PostMessageW(parameters.hFocusWindow, WM_NULL, 0, 0);
        CursorConfinement::Suspend(RevOverlay::BlocksInput());
        CutsceneBorders(device);
        RevOverlay::Keyboard(parameters.hFocusWindow);
        RevOverlay::Draw(device);
        RevFrameRate::Frame(parameters.hFocusWindow);
    }
}

class Runtime
{
public:
    Runtime()
    {
        WFP::onInitEvent() += []()
        {
            WindowCursor::Initialize(FixSettings());
            D3D9Options::frame = Frame;
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                RevIcons::BeforeReset();
                RevOverlay::BeforeReset(device);
            };
            D3D9Options::HookCreation();
            if (auto device = GetD3DDevice())
                D3D9Options::HookDevice(device);
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Runtime;
