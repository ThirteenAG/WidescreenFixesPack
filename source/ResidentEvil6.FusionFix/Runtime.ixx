module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/WindowCursor.hpp"

export module Runtime;

import ComVars;
import InputDrain;
import DeveloperOverlay;
import ButtonIcons;

namespace
{
    BOOL WINAPI NativeClip(const RECT*)
    {
        return TRUE;
    }

    void Frame(IDirect3DDevice9* device)
    {
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (!device || FAILED(device->GetCreationParameters(&parameters))) return;
        DWORD process = 0;
        GetWindowThreadProcessId(parameters.hFocusWindow, &process);
        if (process != GetCurrentProcessId()) return;
        RE6Icons::Frame(device);
        RE6Input::window.store(parameters.hFocusWindow);
        RE6Overlay::AttachWindow(parameters.hFocusWindow);
        if (GetWindowThreadProcessId(parameters.hFocusWindow, nullptr) == GetCurrentThreadId())
            WindowCursor::Attach(parameters.hFocusWindow);
        else if (WindowCursor::Window != parameters.hFocusWindow)
            PostMessageW(parameters.hFocusWindow, WM_NULL, 0, 0);
        CursorConfinement::Suspend(RE6Overlay::BlocksInput());
        if (WindowCursor::Window == parameters.hFocusWindow)
            CursorConfinement::Frame(parameters.hFocusWindow);
        RE6Overlay::Keyboard(parameters.hFocusWindow);
        RE6Overlay::Draw(device);
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
            RE6Input::overlayBlocksInput = []() { return RE6Overlay::BlocksInput(); };
            D3D9Options::frame = Frame;
            D3D9Options::beforeReset = [](IDirect3DDevice9* device)
            {
                RE6Icons::BeforeReset(device);
                RE6Overlay::BeforeReset(device);
            };
            RE6Icons::Install(RE6Overlay::ThisModule());
            D3D9Options::HookCreation();
            IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", NativeClip));
            if (auto device = GetD3DDevice()) D3D9Options::HookDevice(device);
        };
        WFP::onShutdownEvent() += []() { RE6Icons::Disable(); WindowCursor::Shutdown(); };
    }
} Runtime;
