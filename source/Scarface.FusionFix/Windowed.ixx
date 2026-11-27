module;
#include "stdafx.h"
#include "FixSettings.hpp"
#include "RTTI.h"
#include "Frame.hpp"
#include "OverlayInput.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/D3D9Options.hpp"
#include <d3d9.h>
#include <stdexcept>

export module Windowed;

import Focus;
import ComVars;

namespace
{
    int mode = 0;
    SafetyHookInline settingsHook;
    SafetyHookInline displayHook;
    SafetyHookMid resetHook;
    decltype(&Direct3DCreate9) originalCreate9 = nullptr;
    HWND boundWindow = nullptr;
    WNDPROC originalProc = nullptr;
    void* currentDisplay = nullptr;
    thread_local void* applyingDisplay = nullptr;
    thread_local std::optional<HRESULT> resetResult;
    uintptr_t resetResume = 0;
    constexpr int ToggleMode = 3;
    std::atomic<int> pendingMode{ -1 };
    std::atomic<bool> confineCursor{ true };
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
        {
            CursorConfinement::Message(message, wParam);
            if (message == WM_KILLFOCUS || (message == WM_ACTIVATEAPP && !wParam) ||
                (message == WM_ACTIVATE && LOWORD(wParam) == WA_INACTIVE))
            {
                int toggle = ToggleMode;
                pendingMode.compare_exchange_strong(toggle, -1);
            }
        }
        if ((message == WM_SYSKEYDOWN || message == WM_SYSKEYUP) && wParam == VK_RETURN)
        {
            if (message == WM_SYSKEYDOWN && (lParam & (1u << 29)) && !(lParam & (1u << 30)) &&
                window == boundWindow && GetForegroundWindow() == window)
            {
                int empty = -1;
                pendingMode.compare_exchange_strong(empty, ToggleMode);
            }
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

    [[noreturn]] void DisplayStartupError(HRESULT error)
    {
        wchar_t message[160]{};
        swprintf_s(message, L"Scarface could not initialize the Direct3D 9 display (0x%08X).", static_cast<unsigned>(error));
        MessageBoxW(boundWindow, message, L"Scarface", MB_OK | MB_ICONERROR);
        ExitProcess(static_cast<UINT>(error));
    }

    IDirect3D9* WINAPI CreateGraphics(UINT sdkVersion)
    {
        // The engine indexes the first adapter record even when enumeration produced no records.
        HRESULT result = D3DERR_NOTAVAILABLE;
        for (int attempt = 0; attempt < 21; ++attempt)
        {
            if (attempt)
                Sleep(100);
            if (auto api = originalCreate9(sdkVersion))
            {
                D3DCAPS9 caps{};
                const auto adapters = api->GetAdapterCount();
                for (UINT adapter = 0; adapter < adapters; ++adapter)
                {
                    result = api->GetDeviceCaps(adapter, D3DDEVTYPE_HAL, &caps);
                    if (SUCCEEDED(result))
                        return api;
                }
                api->Release();
            }
        }
        DisplayStartupError(result);
    }

    HRESULT WINAPI CreateDisplayDevice(IDirect3D9* api, UINT adapter, D3DDEVTYPE type, HWND window, DWORD flags,
        D3DPRESENT_PARAMETERS* parameters, IDirect3DDevice9** device)
    {
        auto requested = *parameters;
        // The presentation limiter owns pacing, including rates above the monitor refresh rate.
        requested.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        HRESULT result{};
        for (int attempt = 0; attempt < 21; ++attempt)
        {
            if (attempt)
                Sleep(100);
            *parameters = requested;
            result = api->CreateDevice(adapter, type, window, flags, parameters, device);
            if (SUCCEEDED(result))
            {
                D3D9Options::HookDevice(*device);
                return result;
            }
            if (result != D3DERR_DEVICELOST && result != D3DERR_NOTAVAILABLE)
                break;
        }
        // Native resource setup unconditionally calls AddRef on the returned device.
        DisplayStartupError(result);
    }

    void PrepareDisplayReset(IDirect3DDevice9* device)
    {
        if (applyingDisplay && Game::Field<IDirect3DDevice9*>(applyingDisplay, 16) == device)
            Game::Field<D3DPRESENT_PARAMETERS>(applyingDisplay, 20).PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    void CaptureReset(SafetyHookContext& regs)
    {
        if (!applyingDisplay || reinterpret_cast<void*>(regs.ebp) != applyingDisplay)
            return;
        if (!changing && FAILED(static_cast<HRESULT>(regs.eax)))
        {
            // A resize while the device is lost: the caller has released the device resources and the
            // frame would go on without them. Wait here until the device is back and reset.
            auto device = Game::Field<IDirect3DDevice9*>(applyingDisplay, 16);
            auto parameters = &Game::Field<D3DPRESENT_PARAMETERS>(applyingDisplay, 20);
            auto result = static_cast<HRESULT>(regs.eax);
            for (int attempt = 0; attempt < 20 && (result == D3DERR_DEVICELOST || result == D3DERR_DEVICENOTRESET); ++attempt)
            {
                MSG message{};
                if (PeekMessageW(&message, nullptr, WM_QUIT, WM_QUIT, PM_NOREMOVE))
                    break;
                Sleep(100);
                result = device->TestCooperativeLevel();
                if (SUCCEEDED(result) || result == D3DERR_DEVICENOTRESET)
                    result = device->Reset(parameters);
            }
            regs.eax = static_cast<uintptr_t>(result);
        }
        resetResult = static_cast<HRESULT>(regs.eax);
        if (changing && FAILED(*resetResult))
        {
            // The mode-change caller handles the failure and restores the previous device settings.
            regs.ebx = regs.eax;
            regs.eip = resetResume;
        }
    }

    BOOL NativeDisplay(void* display, const std::array<uint32_t, 8>& settings, HRESULT* status)
    {
        const auto previousDisplay = applyingDisplay;
        const auto previousResult = resetResult;
        applyingDisplay = display;
        resetResult.reset();
        const auto result = displayHook.unsafe_fastcall<BOOL>(display, nullptr, settings.data());
        const bool success = result && (!resetResult || SUCCEEDED(*resetResult));
        if (status)
            *status = resetResult && FAILED(*resetResult) ? *resetResult : (success ? S_OK : E_FAIL);
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
            const auto info = Monitor(hWndWindowed.is_initialized() ? hWndWindowed.get() : nullptr);
            Game::Field<int>(settings, 0) = std::max(info.rcMonitor.right - info.rcMonitor.left, 640L);
            Game::Field<int>(settings, 4) = std::max(info.rcMonitor.bottom - info.rcMonitor.top, 480L);
        }
        Game::Field<int>(settings, 0x18) = mode ? 1 : 0;
        if (mode == 2)
        {
            const auto info = Monitor(hWndWindowed.get());
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

    BOOL ApplyDisplaySettings(void* display, const void* requested, HRESULT* status)
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
            return NativeDisplay(display, settings, status);
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
        const auto result = NativeDisplay(display, settings, status);
        changingMode = 1;
        SetWindowPos(window, HWND_NOTOPMOST, x, y, width, height, SWP_NOACTIVATE | SWP_FRAMECHANGED | SWP_NOSENDCHANGING);
        changingMode = previousGuard;
        return result;
    }

    BOOL __fastcall ApplyDisplay(void* display, void*, const void* requested)
    {
        return ApplyDisplaySettings(display, requested, nullptr);
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
        const int next = requested == ToggleMode ? (mode ? 0 : lastWindowMode) : std::clamp<int>(requested, 0, 2);
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
        const bool confine = confineCursor.load(std::memory_order_relaxed) && !ScarfaceOverlayInput::BlocksInput();
        if (confine != CursorConfinement::Enabled)
            CursorConfinement::SetEnabled(confine);
        if (confine || CursorConfinement::Window.load(std::memory_order_relaxed) != boundWindow)
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
            const auto refreshCursor = []()
            {
                const bool confine = FixSettings().Bool("MAIN.ConfineCursor");
                confineCursor.store(confine, std::memory_order_relaxed);
                CursorConfinement::SetEnabled(confine && !ScarfaceOverlayInput::BlocksInput());
            };
            refreshCursor();
            settings.OnChange("MAIN.ConfineCursor", refreshCursor);
            const auto displayVtable = ScarfaceRTTI::FindVtable(".?AVd3dDisplay@pure3d@@");

            auto pattern = hook::pattern("A3 ? ? ? ? 5B 74");
            auto buildSettings = hook::pattern("E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 ? 8D 54 24 ? 52 E8");
            hWndWindowed.SetAddress(*pattern.get_first<HWND*>(1));
            pattern = buildSettings;
            settingsHook = safetyhook::create_inline(injector::GetBranchDestination(pattern.get_first()).as_int(), BuildSettings);

            // d3dDisplay's display-settings method occupies vtable slot 10.
            const auto displaySlot = displayVtable + 10 * sizeof(void*);
            const auto displayMethod = *reinterpret_cast<uintptr_t*>(displaySlot);
            auto factory = hook::pattern("6A 1F E8 ? ? ? ? 50 8B CE E8 ? ? ? ? EB 02");
            auto factoryCall = factory.get_first(2);
            originalCreate9 = reinterpret_cast<decltype(originalCreate9)>(injector::GetBranchDestination(factoryCall).as_int());
            injector::MakeCALL(factoryCall, CreateGraphics, true);

            auto creation = hook::range_pattern(displayMethod, displayMethod + 4096, "FF 52 40 50 E8 ? ? ? ? 8B ? ? E8 ? ? ? ? 8B 06 83 C4 08");
            // Replace the virtual call and its empty diagnostic call. The caller later pops the saved HRESULT.
            D3D9Options::beforeReset = PrepareDisplayReset;
            injector::MakeCALL(creation.get_first(), CreateDisplayDevice, true);
            injector::WriteMemory<uint8_t>(creation.get_first(5), 0x50, true);
            injector::MakeNOP(creation.get_first(6), 3);
            auto reset = hook::range_pattern(displayMethod, displayMethod + 4096, "FF 51 40 8B D8 53 E8 ? ? ? ? 83 C4 04 3B DF 75 ? E8");
            resetResume = reinterpret_cast<uintptr_t>(reset.get_first(14));
            resetHook = safetyhook::create_mid(reset.get_first(3), CaptureReset);
            displayHook = safetyhook::create_inline(reinterpret_cast<void*>(displayMethod), ApplyDisplay);
            ScarfaceFrame::Handlers.push_back(CompleteFrame);
            ScarfaceFocus::RestoreDisplay = []()
            {
                if (!currentDisplay || !haveSettings)
                    return ScarfaceFocus::DisplayRestore::Finished;
                HRESULT status = E_FAIL;
                changing = true;
                ApplyDisplaySettings(currentDisplay, lastSettings.data(), &status);
                changing = false;
                return status == D3DERR_DEVICELOST || status == D3DERR_DEVICENOTRESET ?
                    ScarfaceFocus::DisplayRestore::Retry : ScarfaceFocus::DisplayRestore::Finished;
            };
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
