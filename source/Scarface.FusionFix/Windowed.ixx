module;
#include "stdafx.h"
#include "FixSettings.hpp"
#include "RTTI.h"
#include "Frame.hpp"
#include "../Shared/CursorConfinement.hpp"
#include <d3d9.h>

export module Windowed;
import ComVars;

namespace
{
    int mode = 0;
    HWND* gameWindow;
    SafetyHookInline settingsHook;
    SafetyHookInline displayHook;
    SafetyHookMid resetHook;
    HWND boundWindow = nullptr;
    WNDPROC originalProc = nullptr;
    void* currentDisplay = nullptr;
    void* applyingDisplay = nullptr;
    std::optional<HRESULT> resetResult;
    uintptr_t resetResume = 0;
    std::atomic<int> pendingMode{ -1 };
    int lastWindowMode = 1;
    bool changing = false;
    bool haveSettings = false;
    std::array<uint32_t, 8> lastSettings{};
    RECT framedRect{};
    int framedWidth = 0, framedHeight = 0;
    bool haveFramedRect = false;
    std::function<void(int)> onModeChanged;

    LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        if (window == boundWindow)
            CursorConfinement::Message(message, wParam);
        if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_RETURN)
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)))
                pendingMode.exchange(3);
            return 0;
        }
        if (changing && (message == WM_SIZE || message == WM_ACTIVATE || message == WM_ACTIVATEAPP || message == WM_KILLFOCUS || message == WM_SETFOCUS))
            return DefWindowProcW(window, message, wParam, lParam);
        const auto original = originalProc;
        if (message == WM_NCDESTROY)
        {
            boundWindow = nullptr;
            originalProc = nullptr;
            currentDisplay = nullptr;
            pendingMode.exchange(-1);
        }
        return original ? CallWindowProcW(original, window, message, wParam, lParam) : DefWindowProcW(window, message, wParam, lParam);
    }

    void CaptureReset(SafetyHookContext& regs)
    {
        if (!applyingDisplay || reinterpret_cast<void*>(regs.ebp) != applyingDisplay)
            return;
        resetResult = static_cast<HRESULT>(regs.eax);
        if (changing && FAILED(*resetResult))
        {
            // The mode-change caller handles the failure and restores the previous device settings.
            regs.ebx = regs.eax;
            regs.eip = resetResume;
        }
    }

    BOOL NativeDisplay(void* display, const std::array<uint32_t, 8>& settings)
    {
        const auto previousDisplay = applyingDisplay;
        const auto previousResult = resetResult;
        applyingDisplay = display;
        resetResult.reset();
        const auto result = displayHook.unsafe_fastcall<BOOL>(display, nullptr, settings.data());
        const bool success = result && (!resetResult || SUCCEEDED(*resetResult));
        applyingDisplay = previousDisplay;
        resetResult = previousResult;
        if (success)
        {
            currentDisplay = display;
            lastSettings = settings;
            haveSettings = true;
        }
        return success;
    }

    MONITORINFO Monitor(HWND window)
    {
        MONITORINFO info{ sizeof(info) };
        GetMonitorInfoW(MonitorFromWindow(window, MONITOR_DEFAULTTONEAREST), &info);
        return info;
    }

    int __cdecl BuildSettings(void* settings)
    {
        auto result = settingsHook.unsafe_ccall<int>(settings);
        const auto width = Game::Field<int>(settings, 0);
        const auto height = Game::Field<int>(settings, 4);
        if (width < 640 || width > 16384 || height < 480 || height > 16384)
        {
            const auto info = Monitor(gameWindow ? *gameWindow : nullptr);
            Game::Field<int>(settings, 0) = std::max(info.rcMonitor.right - info.rcMonitor.left, 640L);
            Game::Field<int>(settings, 4) = std::max(info.rcMonitor.bottom - info.rcMonitor.top, 480L);
        }
        Game::Field<int>(settings, 0x18) = mode ? 1 : 0;
        if (mode == 2)
        {
            const auto info = Monitor(*gameWindow);
            Game::Field<int>(settings, 0) = info.rcMonitor.right - info.rcMonitor.left;
            Game::Field<int>(settings, 4) = info.rcMonitor.bottom - info.rcMonitor.top;
        }
        return result;
    }

    void SquareCorners(HWND window)
    {
        using SetAttribute = HRESULT(WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        static auto setAttribute = []() -> SetAttribute
        {
            auto module = LoadLibraryExW(L"dwmapi.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<SetAttribute>(GetProcAddress(module, "DwmSetWindowAttribute")) : nullptr;
        }();
        const int preference = 1;
        if (setAttribute && window)
            setAttribute(window, 33, &preference, sizeof(preference));
    }

    BOOL __fastcall ApplyDisplay(void* display, void*, const void* requested)
    {
        std::array<uint32_t, 8> settings;
        memcpy(settings.data(), requested, sizeof(settings));
        settings[6] = mode ? 1 : 0;
        auto window = Game::Field<HWND>(display, 0x1C8);
        const auto info = Monitor(window);
        SquareCorners(window);
        if (window && !boundWindow)
        {
            boundWindow = window;
            originalProc = reinterpret_cast<WNDPROC>(SetWindowLongPtrW(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProc)));
        }
        if (!mode)
            return NativeDisplay(display, settings);
        if (mode == 2)
        {
            settings[0] = info.rcMonitor.right - info.rcMonitor.left;
            settings[1] = info.rcMonitor.bottom - info.rcMonitor.top;
        }
        else
        {
            const int width = static_cast<int>(settings[0]), height = static_cast<int>(settings[1]);
            settings[0] = width >= 640 && width <= 16384 ? width : std::max(info.rcMonitor.right - info.rcMonitor.left, 640L);
            settings[1] = height >= 480 && height <= 16384 ? height : std::max(info.rcMonitor.bottom - info.rcMonitor.top, 480L);
        }

        // Fixed-size window: the game's resolution menu remains responsible for resizing.
        const DWORD style = mode == 1 ? WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX : WS_POPUP;
        const auto oldStyle = GetWindowLongW(window, GWL_STYLE);
        const auto newStyle = style | (oldStyle & (WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS));
        const auto exStyle = GetWindowLongW(window, GWL_EXSTYLE);
        RECT rect{ 0, 0, static_cast<LONG>(settings[0]), static_cast<LONG>(settings[1]) };
        WindowedModeWrapper::GameHWND = window;
        WindowedModeWrapper::AdjustFrame(&rect, newStyle, FALSE, exStyle);
        const auto width = rect.right - rect.left;
        const auto height = rect.bottom - rect.top;
        auto x = mode == 2 ? info.rcMonitor.left : info.rcMonitor.left + (info.rcMonitor.right - info.rcMonitor.left - static_cast<LONG>(settings[0])) / 2 + rect.left;
        auto y = mode == 2 ? info.rcMonitor.top : info.rcMonitor.top + (info.rcMonitor.bottom - info.rcMonitor.top - static_cast<LONG>(settings[1])) / 2 + rect.top;

        if (mode == 1 && haveFramedRect && framedWidth == static_cast<int>(settings[0]) && framedHeight == static_cast<int>(settings[1]))
        {
            x = framedRect.left;
            y = framedRect.top;
        }

        // WM_SIZE normally re-enters the reset path. Use the engine's own mode-change guard.
        auto& changingMode = Game::Field<uint8_t>(display, 0x1CF);
        const auto previousGuard = changingMode;
        changingMode = 1;
        SetWindowLongW(window, GWL_STYLE, newStyle);
        Game::Field<LONG>(display, 0x1F8) = newStyle;
        SetWindowPos(window, nullptr, x, y, width, height, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
        changingMode = previousGuard;

        // The native windowed branch selects desktop format and zero fullscreen refresh.
        const auto result = NativeDisplay(display, settings);
        changingMode = 1;
        SetWindowPos(window, HWND_NOTOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        changingMode = previousGuard;
        return result;
    }

    void ApplyPending()
    {
        if (pendingMode.load() < 0 || changing || !haveSettings || !boundWindow || GetForegroundWindow() != boundWindow || !currentDisplay)
            return;
        if (Game::Field<uint8_t>(currentDisplay, 0x1CF) || Game::Field<uint8_t>(currentDisplay, 0x1F5))
            return;
        auto device = Game::Field<IDirect3DDevice9*>(currentDisplay, 16);
        if (!device || device->TestCooperativeLevel() != D3D_OK)
            return;
        const auto requested = pendingMode.exchange(-1);
        if (requested < 0)
            return;
        const int previous = mode;
        const int next = requested == 3 ? (mode ? 0 : lastWindowMode) : std::clamp<int>(requested, 0, 2);
        if (next == previous)
            return;
        auto settings = lastSettings;
        settings[0] = Game::Field<int>(currentDisplay, 0x1EC);
        settings[1] = Game::Field<int>(currentDisplay, 0x1E8);
        settings[6] = previous ? 1 : 0;
        RECT previousRect{};
        GetWindowRect(boundWindow, &previousRect);
        const auto previousStyle = GetWindowLongW(boundWindow, GWL_STYLE);
        const auto previousNativeStyle = Game::Field<LONG>(currentDisplay, 0x1F8);
        if (previous == 1)
        {
            framedRect = previousRect;
            framedWidth = settings[0];
            framedHeight = settings[1];
            haveFramedRect = true;
        }
        auto requestedSettings = settings;
        if (next == 1 && haveFramedRect && framedWidth >= 640 && framedHeight >= 480)
        {
            requestedSettings[0] = framedWidth;
            requestedSettings[1] = framedHeight;
        }
        changing = true;
        mode = next;
        if (ApplyDisplay(currentDisplay, nullptr, requestedSettings.data()))
        {
            if (previous)
                lastWindowMode = previous;
            if (mode)
                lastWindowMode = mode;
        }
        else
        {
            OutputDebugStringA("Scarface: window mode change failed; restoring the previous mode.\n");
            mode = previous;
            if (!ApplyDisplay(currentDisplay, nullptr, settings.data()))
                OutputDebugStringA("Scarface: native display rollback failed.\n");
            auto& guard = Game::Field<uint8_t>(currentDisplay, 0x1CF);
            const auto previousGuard = guard;
            guard = 1;
            SetWindowLongW(boundWindow, GWL_STYLE, previousStyle);
            Game::Field<LONG>(currentDisplay, 0x1F8) = previousNativeStyle;
            SetWindowPos(boundWindow, nullptr, previousRect.left, previousRect.top,
                previousRect.right - previousRect.left, previousRect.bottom - previousRect.top,
                SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
            guard = previousGuard;
        }
        changing = false;
        if (onModeChanged)
            onModeChanged(mode);
    }


    void CompleteFrame(void* display)
    {
        if (currentDisplay != display)
            return;
        ApplyPending();
        CursorConfinement::Update(boundWindow);
        CursorConfinement::Bind();
    }
}

class Windowed
{
public:
    Windowed()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            auto nWindowedMode = std::clamp(settings.Int("MAIN.WindowMode"), 0, 2);

            mode = nWindowedMode;
            CursorConfinement::SetEnabled(settings.Bool("MAIN.ConfineCursor"));
            settings.OnChange("MAIN.ConfineCursor", []()
            {
                CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
            });
            const auto displayVtable = ScarfaceRTTI::FindVtable(".?AVd3dDisplay@pure3d@@");
            if (!displayVtable)
            {
                OutputDebugStringA("Scarface: native display table unavailable.\n");
                return;
            }

            auto pattern = hook::pattern("A3 ? ? ? ? 5B 74");
            auto buildSettings = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 8D 54 24 ? 52 E8");
            if (pattern.size() != 1 || buildSettings.size() != 1)
            {
                OutputDebugStringA("Scarface: native window patterns unavailable.\n");
                return;
            }
            gameWindow = *pattern.get_first<HWND*>(1);
            pattern = buildSettings;
            settingsHook = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BuildSettings);

            // d3dDisplay's display-settings method occupies vtable slot 10.
            const auto displaySlot = displayVtable + 10 * sizeof(void*);
            const auto displayMethod = injector::ReadMemory<uintptr_t>(displaySlot, true);
            auto reset = hook::range_pattern(displayMethod, displayMethod + 4096, "FF 51 40 8B D8 53 E8 ? ? ? ? 83 C4 04 3B DF 75 ? E8");
            if (reset.size() != 1)
            {
                settingsHook.reset();
                OutputDebugStringA("Scarface: native display reset status unavailable.\n");
                return;
            }
            resetResume = reinterpret_cast<uintptr_t>(reset.get_first(14));
            resetHook = safetyhook::create_mid(reset.get_first(3), CaptureReset);
            displayHook = safetyhook::create_inline(reinterpret_cast<void*>(displayMethod), ApplyDisplay);
            if (!settingsHook || !displayHook || !ScarfaceFrame::PresentHook || !resetHook)
            {
                settingsHook.reset();
                displayHook.reset();
                resetHook.reset();
                OutputDebugStringA("Scarface: native window hooks unavailable.\n");
                return;
            }
            ScarfaceFrame::Handlers.push_back(CompleteFrame);
            onModeChanged = [](int value)
            {
                FixSettings().Set("MAIN.WindowMode", std::to_string(value));
            };
            settings.OnChange("MAIN.WindowMode", []()
            {
                pendingMode.exchange(std::clamp(FixSettings().Int("MAIN.WindowMode"), 0, 2));
            });
        };
        WFP::onShutdownEvent() += []()
        {
            CursorConfinement::Shutdown();
        };
    }
} Windowed;
