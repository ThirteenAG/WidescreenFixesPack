module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/WindowCursor.hpp"

export module OverlayRuntime;

import InputDrain;
import DeveloperOverlay;
import Video;
import LightSync;
import IconBindings;
import ButtonIcons;

export namespace RE0OverlayRuntime
{
    inline BOOL WINAPI NativeClip(const RECT*)
    {
        // The physical-client service owns confinement, including native release requests.
        return TRUE;
    }

    inline void Frame(IDirect3DDevice9* device)
    {
        RE0Video::Frame();
        RE0Icons::Frame(device);
        D3DDEVICE_CREATION_PARAMETERS parameters{};
        if (!device || FAILED(device->GetCreationParameters(&parameters))) return;
        DWORD process = 0;
        GetWindowThreadProcessId(parameters.hFocusWindow, &process);
        if (process != GetCurrentProcessId()) return;
        RE0Overlay::AttachWindow(parameters.hFocusWindow);
        WindowCursor::Follow(parameters.hFocusWindow);
        CursorConfinement::Suspend(RE0Overlay::BlocksInput());
        if (!RE0Input::ready) return;
        RE0Overlay::Keyboard(parameters.hFocusWindow);
        RE0Overlay::Draw(device);
    }

    inline void BeforeReset(IDirect3DDevice9* device)
    {
        RE0Video::Frame();
        RE0Icons::BeforeReset(device);
        RE0Overlay::BeforeReset(device);
    }

    inline void Install()
    {
        auto update = hook::pattern("8B 0D ? ? ? ? E8 ? ? ? ? 8B 07 8B CF FF 50 18 8B 0D ? ? ? ? 80 B9 ? 26 00 00 00");
        static auto gameUpdate = safetyhook::create_mid(update.get_first(18), [](SafetyHookContext&)
        {
            RE0LightSync::Sample();
            RE0Video::Scene();
        });
        WindowCursor::Initialize(FixSettings());
        RE0Input::Install();
        RE0Input::overlayBlocksInput = []() { return RE0Overlay::BlocksInput(); };
        RE0Overlay::InputReady = []() { return RE0Input::ready; };
        D3D9Options::frame = Frame;
        D3D9Options::beforeReset = BeforeReset;
        D3D9Options::HookCreation();
        IATHook::Replace(GetModuleHandleW(nullptr), "USER32.dll", std::forward_as_tuple("ClipCursor", NativeClip));
    }
}
