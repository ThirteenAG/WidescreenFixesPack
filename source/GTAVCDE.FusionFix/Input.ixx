module;

#include "stdafx.h"

export module Input;

import Build;
import Settings;
import RawMouse;
import Window;

namespace
{
    SafetyHookInline baseUpdatePadHook;
    SafetyHookMid mouseSampleHook;
    MouseDelta frameDelta;
    bool mousePathActive = false;

    uintptr_t BaseUpdatePad(void* pad, float time, float deltaTime, bool enableMouse)
    {
        // Drain on every pad update, including menus and controller frames.
        // An inactive frame must never replay its movement after resuming.
        if (Settings.rawMouseInput)
            RefreshRawMouseRegistration();
        frameDelta = ConsumeRawMouse();
        mousePathActive = false;
        const auto result = baseUpdatePadHook.ccall<uintptr_t>(pad, time, deltaTime, enableMouse);
        SetCursorCapture(mousePathActive && Settings.improveCameraPC);
        return result;
    }
}

export bool IsMouseCameraActive()
{
    return mousePathActive && IsGameFocused();
}

class InputModule
{
public:
    InputModule()
    {
        WFP::onInitEvent() += []()
        {
            // CPadBase::BaseUpdatePad, latest PC trilogy. This store is reached
            // only after native viewport/menu/wheel/device gates have passed.
            const auto sample = MouseSampleAddress();
            const auto start = PadUpdateAddress();
            if (!sample || !start)
                return;
            baseUpdatePadHook = safetyhook::create_inline(start, BaseUpdatePad);
            if (!baseUpdatePadHook)
                return;
            mouseSampleHook = safetyhook::create_mid(sample, [](SafetyHookContext& context)
            {
                mousePathActive = true;
                if (Settings.rawMouseInput && IsRawMouseReady())
                {
                    context.xmm0.f32[0] = frameDelta.x;
                    context.xmm0.f32[1] = frameDelta.y;
                }
            });
            if (!mouseSampleHook)
                baseUpdatePadHook = {};
        };
    }
} InputModuleInstance;
