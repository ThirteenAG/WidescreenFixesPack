module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Renderer;

import Frontend;
import DeveloperOverlay;
import Xidi;

// Frame limit, cursor confinement, developer overlay and update check on the game's device; focus handling in the
// game's window procedure.
export namespace CondemnedRenderer
{
    inline SafetyHookInline windowProcedureHook;
    inline std::atomic<HWND> window{ nullptr };
    inline int windowMode = 0;

    inline std::atomic<int> requestedFrameLimit{ 0 };

    inline void UpdateFrameLimit()
    {
        requestedFrameLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
    }

    inline void ApplyFrameLimit(HWND target)
    {
        const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
        if (requested >= 0)
        {
            D3D9Options::frameLimit = requested;
            return;
        }
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const auto monitor = MonitorFromWindow(target, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (monitor != previous || now - lastCheck >= 2000)
        {
            previous = monitor;
            lastCheck = now;
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            refresh = GetMonitorInfoW(monitor, &info) &&
                EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) && mode.dmDisplayFrequency > 1 ?
                int(mode.dmDisplayFrequency) : 60;
        }
        D3D9Options::frameLimit = refresh;
    }

    // With MAIN.PauseOnFocusLoss off, a window mode keeps running in the background: the game's deactivation shuts
    // its renderer down and pauses the game. The overlay gets the window's messages first.
    inline LRESULT CALLBACK WindowProcedure(HWND target, UINT message, WPARAM wParam, LPARAM lParam)
    {
        window = target;
        LRESULT result = 0;
        if (CondemnedOverlay::Message(target, message, wParam, lParam, result))
            return result;
        if (message == WM_ACTIVATEAPP && !wParam && windowMode && !FixSettings().Bool("MAIN.PauseOnFocusLoss"))
        {
            CursorConfinement::Release();
            return 0;
        }
        return windowProcedureHook.unsafe_stdcall<LRESULT>(target, message, wParam, lParam);
    }

    inline void Init(int mode)
    {
        windowMode = mode;
        auto procedure = hook::pattern("A1 ? ? ? ? 81 EC 40 02 00 00 85 C0 55 8B AC 24 50 02 00 00");
        windowProcedureHook = safetyhook::create_inline(procedure.get_first(), WindowProcedure); //0x47C530

        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        D3D9Options::frame = [](IDirect3DDevice9* device)
        {
            const HWND target = window;
            UpdateXidi();
            ApplyFrameLimit(target);
            CursorConfinement::Suspend(CondemnedOverlay::BlocksInput());
            CursorConfinement::Frame(target);
            RWGTA::UpdateCheck::Tick();
            CondemnedOverlay::Present(device, target);
        };
        D3D9Options::beforeReset = CondemnedOverlay::BeforeReset;

        // Update notice: in the fix's page of the game's own menu
        RWGTA::UpdateCheck::Config update;
        update.pluginId = "Condemned.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return true; };
        update.show = [](const std::wstring& line) { CondemnedMenuNative::notice = asi_settings::text::Narrow(line); };
        RWGTA::UpdateCheck::Init(std::move(update));
        if (!D3D9Options::HookCreation())
            OutputDebugStringA("Condemned: Direct3D 9 hooks unavailable.\n");
    }
}
