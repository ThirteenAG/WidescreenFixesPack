module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

// Frame limit, cursor confinement, developer overlay and update check on the game's device; focus handling in the
// game's window.

export module Renderer;

import Window;
import DeveloperOverlay;

export namespace SecondSightRenderer
{
    inline std::string notice;

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

    inline void Init()
    {
        // The overlay gets the window's messages first. With MAIN.PauseOnFocusLoss off, a window mode stays active in
        // the background: the device setup otherwise marks the game inactive and slows its loop down.
        SecondSightWindow::Message = [](HWND window, UINT message, WPARAM wParam, LPARAM lParam, LRESULT& result)
        {
            if (SecondSightOverlay::Message(window, message, wParam, lParam, result))
                return true;
            if (message == WM_DESTROY)
                RWGTA::UpdateCheck::OnExit();
            if (message == WM_ACTIVATEAPP && !wParam && SecondSightWindow::Mode && !FixSettings().Bool("MAIN.PauseOnFocusLoss"))
            {
                CursorConfinement::Release();
                result = 0;
                return true;
            }
            return false;
        };

        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        D3D9Options::frame = [](IDirect3DDevice9* device)
        {
            const HWND window = SecondSightWindow::Window;
            WFP::onGameProcessEvent().executeAll();
            CursorConfinement::Suspend(SecondSightOverlay::BlocksInput());
            CursorConfinement::Frame(window);
            ApplyFrameLimit(window);
            RWGTA::UpdateCheck::Tick();
            SecondSightOverlay::Present(device, window);
        };
        D3D9Options::beforeReset = SecondSightOverlay::BeforeReset;

        RWGTA::UpdateCheck::Config update;
        update.pluginId = "SecondSight.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return true; };
        update.show = [](const std::wstring& line) { notice = asi_settings::text::Narrow(line); };
        RWGTA::UpdateCheck::Init(std::move(update));
        D3D9Options::HookCreation();
    }
}
