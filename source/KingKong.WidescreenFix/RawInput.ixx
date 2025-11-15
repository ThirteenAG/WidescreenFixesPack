module;

#include <stdafx.h>

export module RawInput;

import ComVars;

GameRef<float> aMouseX;
GameRef<float> aMouseY;
void* pSettings = nullptr;

SafetyHookInline shCameraRotationX = {};
void __cdecl CameraRotationX(void* a1, float a2)
{
    float fMouseSensitivity = 1.0f;
    auto Settings = *(uintptr_t*)pSettings;
    if (Settings)
        fMouseSensitivity = *(float*)(Settings + 0x4DBC);
    auto deltaX = -aMouseX.get() * Screen.fRawInputMouse * fMouseSensitivity;
    return shCameraRotationX.unsafe_ccall(a1, deltaX);
}

SafetyHookInline shCameraRotationY = {};
void __cdecl CameraRotationY(void* a1, float a2)
{
    int bMouseInverted = 0;
    float fMouseSensitivity = 1.0f;
    auto Settings = *(uintptr_t*)pSettings;
    if (Settings)
    {
        bMouseInverted = *(int*)(*(uintptr_t*)pSettings + 0x4DB8);
        fMouseSensitivity = *(float*)(*(uintptr_t*)pSettings + 0x4DBC);
    }
    auto deltaY = -aMouseY.get() * Screen.fRawInputMouse * (bMouseInverted ? -fMouseSensitivity : fMouseSensitivity);
    return shCameraRotationY.unsafe_ccall(a1, deltaY);
}

SafetyHookInline shWndProc = {};
LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (Screen.fRawInputMouse > 0.0f)
    {
        static bool bOnce = false;
        if (!bOnce)
        {
            if (Screen.Width)
            {
                RawCursorHandler<float>::Initialize(hWnd, Screen.fRawInputMouse);
                bOnce = true;
            }
        }
    }
    return shWndProc.unsafe_stdcall<LRESULT>(hWnd, Msg, wParam, lParam);
}

export void InitRawInput()
{
    if (Screen.fRawInputMouse > 0.0f)
    {
        auto pattern = hook::pattern("53 8B 5C 24 ? 55 8B 6C 24 ? 56 8B 74 24 ? 57 8B 7C 24 ? 8D 44 24 ? 50 56 57 53 55");
        shWndProc = safetyhook::create_inline(pattern.get_first(), WndProc);

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? EB 21 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8");
        aMouseX.SetAddress(*pattern.get_first<float*>(2));

        pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C3 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8 C7 05");
        aMouseY.SetAddress(*pattern.get_first<float*>(2));

        pattern = hook::pattern("8B 0D ? ? ? ? 8B 91 ? ? ? ? 89 95 ? ? ? ? 8B 85 ? ? ? ? 83 E8 06");
        pSettings = *pattern.get_first<void*>(2);

        pattern = hook::pattern("55 8B EC 83 EC 34 D9 05 ? ? ? ? D9 45 ? DA E9 DF E0 F6 C4 44");
        shCameraRotationY = safetyhook::create_inline(pattern.count(6).get(0).get<void*>(0), CameraRotationY);
        shCameraRotationX = safetyhook::create_inline(pattern.count(6).get(5).get<void*>(0), CameraRotationX);

        static auto SetCursor = [](float* pMouseX, float* pMouseY)
        {
            RECT clientRect;
            GetClientRect(RawCursorHandler<float>::UpdateMouseInput(true), &clientRect);

            // Normalize to 0.0 - 1.0 range
            float clientWidth = static_cast<float>(clientRect.right - clientRect.left);
            float clientHeight = static_cast<float>(clientRect.bottom - clientRect.top);

            *pMouseX = RawCursorHandler<float>::MouseCursorX / clientWidth;
            *pMouseY = RawCursorHandler<float>::MouseCursorY / clientHeight;
        };

        pattern = hook::pattern("8B 55 ? 8B 82 ? ? ? ? 89 45 ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 4D ? 89 4D ? 8B 55 ? 8B 45 ? 89 82 ? ? ? ? 8B 4D");
        static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            SetCursor((float*)(regs.eax + 0x1D4), (float*)(regs.eax + 0x1D8));
        });

        pattern = hook::pattern("8B 45 ? 8B 88 ? ? ? ? 89 4D ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 55 ? 89 55 ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B 55");
        static int32_t offset = *pattern.get_first<int32_t>(-4);
        static auto MouseCursorHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
        {
            SetCursor((float*)(regs.eax + offset - 4), (float*)(regs.eax + offset));
        });
    }
}
