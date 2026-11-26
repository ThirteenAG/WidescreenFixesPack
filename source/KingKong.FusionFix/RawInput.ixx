module;

#include <stdafx.h>
#include "Shared/CursorConfinement.hpp"
#include <d3d9.h>
#include "FixSettings.hpp"

export module RawInput;

import ComVars;
import DeveloperOverlay;

bool bNeedsRawInput = false;
std::atomic<float> RawMouseScale{ 0.0f };
SafetyHookInline shsub_45B1B0 = {};
int sub_45B1B0()
{
    bNeedsRawInput = true;
    auto ret = shsub_45B1B0.unsafe_call<int>();
    bNeedsRawInput = false;
    return ret;
}

SafetyHookInline shCameraRotationX = {};
void __cdecl CameraRotationX(void* a1, float a2)
{
    if (bNeedsRawInput && aMouseX.get())
    {
        float fMouseSensitivity = 1.0f;
        uintptr_t Settings = pSettings;
        if (Settings)
            fMouseSensitivity = *(float*)(Settings + 0x4DBC);
        auto deltaX = -aMouseX.get() * RawMouseScale.load(std::memory_order_relaxed) * fMouseSensitivity;
        return shCameraRotationX.unsafe_ccall(a1, deltaX);
    }
    return shCameraRotationX.unsafe_ccall(a1, a2);
}

SafetyHookInline shCameraRotationY = {};
void __cdecl CameraRotationY(void* a1, float a2)
{
    if (bNeedsRawInput && aMouseY.get())
    {
        int bMouseInverted = 0;
        float fMouseSensitivity = 1.0f;
        uintptr_t Settings = pSettings;
        if (Settings)
        {
            bMouseInverted = *(int*)(pSettings + 0x4DB8);
            fMouseSensitivity = *(float*)(pSettings + 0x4DBC);
        }
        auto deltaY = -aMouseY.get() * RawMouseScale.load(std::memory_order_relaxed) * (bMouseInverted ? -fMouseSensitivity : fMouseSensitivity);
        return shCameraRotationY.unsafe_ccall(a1, deltaY);
    }
    return shCameraRotationY.unsafe_ccall(a1, a2);
}

SafetyHookInline shWndProc = {};
std::atomic<HWND> inputWindow{ nullptr };
std::atomic<bool> pauseOnFocusLoss{ true };
UINT pausePolicyMessage = 0;
LRESULT WINAPI WndProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    if (pausePolicyMessage && Msg == pausePolicyMessage)
    {
        if (bAppInactive.is_initialized())
            bAppInactive = pauseOnFocusLoss.load(std::memory_order_relaxed) && GetForegroundWindow() != hWnd ? 1u : 0u;
        return 0;
    }
    if (Msg == WM_NCDESTROY)
    {
        auto destroyed = hWnd;
        inputWindow.compare_exchange_strong(destroyed, nullptr, std::memory_order_relaxed);
    }
    else if (Screen.Width > 0)
        inputWindow.store(hWnd, std::memory_order_relaxed);
    LRESULT overlayResult = 0;
    if (KongOverlay::Message(hWnd, Msg, wParam, lParam, overlayResult))
        return overlayResult;
    const auto result = shWndProc.unsafe_stdcall<LRESULT>(hWnd, Msg, wParam, lParam);
    if (Msg == WM_ACTIVATEAPP && bAppInactive.is_initialized() && !pauseOnFocusLoss.load(std::memory_order_relaxed))
        bAppInactive = 0;
    // Mouse moves, hit tests and raw input come thousands of times a second with a high polling rate mouse; the
    // confinement doesn't change with them
    const bool mouseTraffic = Msg == WM_MOUSEMOVE || Msg == WM_NCMOUSEMOVE || Msg == WM_INPUT || Msg == WM_NCHITTEST || Msg == WM_SETCURSOR;
    if (Msg == WM_KILLFOCUS || (Msg == WM_ACTIVATEAPP && !wParam))
        CursorConfinement::Release();
    else if (Msg != WM_NCDESTROY && !mouseTraffic)
        CursorConfinement::Frame(hWnd);
    return result;
}

// The game's own pause while its window is in the background (nothing of the game runs then)
export bool GameInactive()
{
    return bAppInactive.is_initialized() && bAppInactive.get() != 0;
}

// Menu cursor: every menu page updates it each frame. With raw input it follows the Windows cursor; menus are
// drawn in a centred box on wide screens, so the cursor is mapped into that box.
void SetMenuCursor(float* pMouseX, float* pMouseY)
{
    if (!aMouseX.get() && !aMouseY.get())
    {
        if (wasUsingLeftStick)
        {
            *pMouseX = 1.0f;
            *pMouseY = 0.0f;
            return;
        }
    }
    else
        wasUsingLeftStick = false;

    RECT clientRect{};
    const auto window = inputWindow.load(std::memory_order_relaxed);
    POINT cursor{};
    CursorConfinement::Frame(window);
    if (!window || GetForegroundWindow() != window || !GetClientRect(window, &clientRect) ||
        clientRect.right <= 0 || clientRect.bottom <= 0 || !GetCursorPos(&cursor) || !ScreenToClient(window, &cursor))
        return;

    const float x = static_cast<float>(cursor.x) / clientRect.right;
    const float box = Screen.fTextBox > 0.0f ? Screen.fTextBox : 1.0f;
    *pMouseX = std::clamp(0.5f + (x - 0.5f) / box, 0.0f, 1.0f);
    *pMouseY = std::clamp(static_cast<float>(cursor.y) / clientRect.bottom, 0.0f, 1.0f);
}

class RawInput
{
public:
    RawInput()
    {
        WFP::onInitEvent() += []()
        {
            const bool rawInput = FixSettings().Get("MAIN.RawInputMouse") == "1";
            const auto updateMouseSensitivity = [rawInput]()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                const float value = ReadNumericSetting("MAIN.MouseSensitivity", 1.0f);
                RawMouseScale.store(rawInput ? (std::isfinite(value) ? std::clamp(value, 0.1f, 5.0f) : 1.0f) : 0.0f,
                    std::memory_order_relaxed);
            };
            FixSettings().OnChange("MAIN.MouseSensitivity", updateMouseSensitivity);
            updateMouseSensitivity();

            if constexpr (WFP::DeveloperOverlay)
                KongOverlay::InitInput();
            auto procedure = hook::pattern("53 8B 5C 24 ? 55 8B 6C 24 ? 56 8B 74 24 ? 57 8B 7C 24 ? 8D 44 24 ? 50 56 57 53 55");
            auto inactive = hook::pattern("C7 05 ? ? ? ? 01 00 00 00 E8 ? ? ? ? EB ? C7 05 ? ? ? ? 00 00 00 00 E8");
            bAppInactive.SetAddress(*inactive.get_first<uint32_t*>(2));
            pausePolicyMessage = RegisterWindowMessageW(L"KingKong.FusionFix.PauseOnFocusLoss");
            shWndProc = safetyhook::create_inline(procedure.get_first(), WndProc);
            const auto updateConfinement = []()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                CursorConfinement::SetEnabled(FixSettings().Get("MAIN.ConfineCursor") == "1");
            };
            FixSettings().OnChange("MAIN.ConfineCursor", updateConfinement);
            updateConfinement();
            const auto updatePause = []()
            {
                static std::mutex mutex;
                const std::lock_guard lock(mutex);
                pauseOnFocusLoss.store(FixSettings().Get("MAIN.PauseOnFocusLoss") == "1", std::memory_order_relaxed);
                if (const auto window = inputWindow.load(std::memory_order_relaxed); window && pausePolicyMessage)
                    PostMessageW(window, pausePolicyMessage, 0, 0);
            };
            FixSettings().OnChange("MAIN.PauseOnFocusLoss", updatePause);
            updatePause();

            auto pattern = hook::pattern("8B 55 ? 8B 82 ? ? ? ? 89 45 ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 4D ? 89 4D ? 8B 55 ? 8B 45 ? 89 82 ? ? ? ? 8B 4D");
            static auto MouseCursorHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                gMenuTick = GetTickCount();
                if (RawMouseScale.load(std::memory_order_relaxed) > 0.0f)
                    SetMenuCursor((float*)(regs.eax + 0x1D4), (float*)(regs.eax + 0x1D8));
            });

            pattern = hook::pattern("8B 45 ? 8B 88 ? ? ? ? 89 4D ? D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 55 ? 89 55 ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B 55");
            static int32_t offset = *pattern.get_first<int32_t>(-4);
            static auto MouseCursorHook2 = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& regs)
            {
                gMenuTick = GetTickCount();
                if (RawMouseScale.load(std::memory_order_relaxed) > 0.0f)
                    SetMenuCursor((float*)(regs.eax + offset - 4), (float*)(regs.eax + offset));
            });

            if (rawInput)
            {
                pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? EB 21 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8");
                aMouseX.SetAddress(*pattern.get_first<float*>(2));

                pattern = hook::pattern("C7 05 ? ? ? ? ? ? ? ? C3 D8 15 ? ? ? ? DF E0 F6 C4 41 75 ? DD D8 C7 05");
                aMouseY.SetAddress(*pattern.get_first<float*>(2));

                pattern = hook::pattern("8B 0D ? ? ? ? 8B 91 ? ? ? ? 89 95 ? ? ? ? 8B 85 ? ? ? ? 83 E8 06");
                pSettings.SetAddress(*pattern.get_first<uintptr_t*>(2));

                pattern = hook::pattern("55 8B EC B8 ? ? ? ? E8 ? ? ? ? A1 ? ? ? ? 8B 48 ? 89 8D ? ? ? ? 8B 15 ? ? ? ? 89 95 ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? ? DF E0");
                shsub_45B1B0 = safetyhook::create_inline(pattern.get_first(), sub_45B1B0);

                pattern = hook::pattern("55 8B EC 83 EC 34 D9 05 ? ? ? ? D9 45 ? DA E9 DF E0 F6 C4 44");
                shCameraRotationY = safetyhook::create_inline(pattern.count(6).get(0).get<void*>(0), CameraRotationY);
                shCameraRotationX = safetyhook::create_inline(pattern.count(6).get(5).get<void*>(0), CameraRotationX);

                static float f1 = 1.0f;
                pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 4D ? 89 4D ? 8B 55 ? 8B 45 ? 89 82 ? ? ? ? 8B 4D");
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f1, true);
                injector::WriteMemory<float>(pattern.get_first<float>(16), f1, true);

                pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 45 ? 89 45 ? 8B 4D ? 8B 55 ? 89 91 ? ? ? ? 8B 45");
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f1, true);
                injector::WriteMemory<float>(pattern.get_first<float>(16), f1, true);

                pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 55 ? 89 55 ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B 55 ? 8B 82 ? ? ? ? 89 45");
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f1, true);
                injector::WriteMemory<float>(pattern.get_first<float>(16), f1, true);

                pattern = hook::pattern("D8 1D ? ? ? ? DF E0 F6 C4 41 75 ? C7 45 ? ? ? ? ? EB 1F D9 45 ? D8 1D ? ? ? ? DF E0 F6 C4 05 7A ? C7 45 ? ? ? ? ? EB 06 8B 4D ? 89 4D ? 8B 55 ? 8B 45 ? 89 82 ? ? ? ? 68");
                injector::WriteMemory<float*>(pattern.get_first<float*>(2), &f1, true);
                injector::WriteMemory<float>(pattern.get_first<float>(16), f1, true);
            }
        };
    }
} RawInput;
