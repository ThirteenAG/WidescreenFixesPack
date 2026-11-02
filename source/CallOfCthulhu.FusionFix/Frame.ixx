module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/D3D9Options.hpp"
#include "Shared/CursorConfinement.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frame;

import Cinema;
import DeveloperOverlay;
import Frontend;

// Per-frame work on the game's device (frame rate limit, cursor confinement, the developer overlay, the update notice)
// and the window's messages
export namespace CoCFrame
{
    inline std::atomic<HWND> window{ nullptr };
    inline SafetyHookInline windowProcedureHook;

    inline LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        window = hwnd;
        LRESULT result = 0;
        if (CoCOverlay::Message(hwnd, message, wParam, lParam, result))
            return result;
        if (message == WM_ACTIVATEAPP && !wParam)
            CursorConfinement::Release();
        if (message == WM_DESTROY)
            RWGTA::UpdateCheck::OnExit();
        return windowProcedureHook.unsafe_stdcall<LRESULT>(hwnd, message, wParam, lParam);
    }

    // The game's device, once it draws: hooking device creation early upsets the game's display mode search
    inline void Attach(IDirect3DDevice9* device)
    {
        static bool attached = false;
        if (attached || !device)
            return;
        attached = true;
        D3D9Options::HookDevice(device);
    }

    // Frame rate limit at the start of each game frame. The game's present holds a lock its other work waits on, so
    // waiting inside the present leaves the picture black.
    inline std::atomic<int> requestedFrameLimit{ 0 };
    inline SafetyHookMid paceHook;

    inline void Pace()
    {
        static LONGLONG next = 0, frequency = 0;
        static HANDLE timer = CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        static HMONITOR previous = nullptr;
        static ULONGLONG lastCheck = 0;
        static int refresh = 60;
        const int requested = requestedFrameLimit.load(std::memory_order_relaxed);
        int frameLimit = requested;
        if (requested < 0)
        {
            const auto monitor = MonitorFromWindow(window.load(std::memory_order_relaxed), MONITOR_DEFAULTTOPRIMARY);
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
            frameLimit = refresh;
        }
        if (frameLimit <= 0)
        {
            next = 0;
            return;
        }
        if (!frequency)
        {
            LARGE_INTEGER f;
            QueryPerformanceFrequency(&f);
            frequency = f.QuadPart;
        }
        const LONGLONG period = frequency / std::clamp(frameLimit, 10, 1000);
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        // A late frame (loading, a hitch) starts the count anew instead of rushing the next ones
        if (next == 0 || now.QuadPart - next > period * 4)
        {
            next = now.QuadPart + period;
            return;
        }
        constexpr double SpinTime = 0.0015;
        const double seconds = double(next - now.QuadPart) / double(frequency);
        if (seconds > SpinTime)
        {
            LARGE_INTEGER due;
            due.QuadPart = -static_cast<LONGLONG>((seconds - SpinTime) * 10000000.0);
            if (timer && SetWaitableTimer(timer, &due, 0, nullptr, nullptr, FALSE))
                WaitForSingleObject(timer, INFINITE);
            else
                Sleep(static_cast<DWORD>((seconds - SpinTime) * 1000.0));
        }
        do
        {
            YieldProcessor();
            QueryPerformanceCounter(&now);
        } while (now.QuadPart < next);
        next += period;
    }

    inline void UpdateFrameLimit()
    {
        // The game is made for 60 frames per second; -1 follows the monitor
        requestedFrameLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
    }

    inline void Init()
    {
        UpdateFrameLimit();
        FixSettings().OnChange("MAIN.FPSLimit", UpdateFrameLimit);
        auto tick = hook::pattern("E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8B 44 24 20 8B 4C 24 08 89 08 B0 01 83 C4 1C C2 04 00");
        WFP::onGameProcessEvent() += []() { Pace(); };
        paceHook = safetyhook::create_mid(tick.get_first(), [](SafetyHookContext&)
        {
            static std::once_flag once;
            std::call_once(once, []() { WFP::onGameInitEvent().executeAll(); });
            WFP::onGameProcessEvent().executeAll();
        });
        if (!paceHook)
            OutputDebugStringA("Call of Cthulhu: frame loop hook installation failed.\n");
        CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        FixSettings().OnChange("MAIN.ConfineCursor", []()
        {
            CursorConfinement::SetEnabled(FixSettings().Bool("MAIN.ConfineCursor"));
        });
        D3D9Options::frame = [](IDirect3DDevice9* device)
        {
            CoCCinema::Frame();
            CursorConfinement::Suspend(CoCOverlay::BlocksInput());
            CursorConfinement::Frame(window);
            RWGTA::UpdateCheck::Tick();
            CoCOverlay::Present(device, window);
        };
        D3D9Options::beforeReset = CoCOverlay::BeforeReset;

        // Update notice: in the help line of the fix's page in Options
        RWGTA::UpdateCheck::Config update;
        update.pluginId = "CallOfCthulhu.FusionFix";
        update.fileVersion = rsc_FileVersion;
        update.settings = &FixSettings();
        update.playable = []() { return true; };
        update.show = [](const std::wstring& line) { CthulhuMenu::notice = asi_settings::text::Narrow(line); };
        RWGTA::UpdateCheck::Init(std::move(update));

        auto procedure = hook::pattern("83 EC 14 55 8B 6C 24 28 56 8B 74 24 28 57 8B 7C 24 28 83 FF 20 0F 87");
        windowProcedureHook = safetyhook::create_inline(procedure.get_first(), WindowProcedure);
        if (!windowProcedureHook)
            OutputDebugStringA("Call of Cthulhu: window message hook installation failed.\n");
    }
}
