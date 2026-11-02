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
    const int* pCutsceneBorders = nullptr;

    void CutsceneBorders(IDirect3DDevice9* device)
    {
        if (!pCutsceneBorders || !(*pCutsceneBorders & 2) || !IsCutsceneCamera())
            return;
        IDirect3DSurface9* backBuffer = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)) || !backBuffer)
            return;
        D3DSURFACE_DESC desc{};
        backBuffer->GetDesc(&desc);
        const auto width = static_cast<LONG>(desc.Width), height = static_cast<LONG>(desc.Height);
        const auto picture = static_cast<LONG>(std::lround(height * defaultAspectRatio));
        if (picture + 1 >= width)
        {
            backBuffer->Release();
            return;
        }
        IDirect3DSurface9* target = nullptr;
        D3DVIEWPORT9 viewport{};
        DWORD scissor = FALSE;
        device->GetRenderTarget(0, &target);
        device->GetViewport(&viewport);
        device->GetRenderState(D3DRS_SCISSORTESTENABLE, &scissor);

        device->SetRenderTarget(0, backBuffer);
        const D3DVIEWPORT9 full{ 0, 0, desc.Width, desc.Height, 0.0f, 1.0f };
        device->SetViewport(&full);
        device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
        const auto left = (width - picture) / 2;
        const D3DRECT bars[2]{ { 0, 0, left, height }, { left + picture, 0, width, height } };
        device->Clear(2, bars, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.0f, 0);

        device->SetRenderState(D3DRS_SCISSORTESTENABLE, scissor);
        if (target)
        {
            device->SetRenderTarget(0, target);
            target->Release();
        }
        device->SetViewport(&viewport);
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
        RevFrameRate::Frame(parameters.hFocusWindow);
        RevIcons::Frame(device);
        RevOverlay::AttachWindow(parameters.hFocusWindow);
        if (RevOverlay::cursorWindow.load() != parameters.hFocusWindow)
            PostMessageW(parameters.hFocusWindow, WM_NULL, 0, 0);
        CursorConfinement::Suspend(RevOverlay::BlocksInput());
        CutsceneBorders(device);
        RevOverlay::Keyboard(parameters.hFocusWindow);
        RevOverlay::Draw(device);
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
            pCutsceneBorders = &FixSettings().Int("MAIN.CutsceneBorders");
            D3D9Options::frame = Frame;
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                RevIcons::BeforeReset();
                RevOverlay::BeforeReset(device);
            };
            if (D3D9Options::HookCreation())
            {
                if (auto device = GetD3DDevice())
                    D3D9Options::HookDevice(device);
            }
            else
            {
                D3D9Options::frame = nullptr;
                D3D9Options::beforeReset = nullptr;
                OutputDebugStringA("Resident Evil Revelations Fusion Fix: cutscene borders and developer overlay unavailable.\n");
            }
        };
        WFP::onShutdownEvent() += []() { WindowCursor::Shutdown(); };
    }
} Runtime;
