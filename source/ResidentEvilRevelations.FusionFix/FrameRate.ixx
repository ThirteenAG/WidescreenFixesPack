module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers VARIABLE (capped at 120 FPS), 60 and 30. With VARIABLE the fix caps
// the game at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number.

namespace
{
    ProtectedGameRef<int32_t> nVariableCap;  // the game's table of the option's caps: VARIABLE, 60, 30
    GameRef<uintptr_t> pMain;               // the game's main object: the frame rate cap in effect at +0x3C
    int32_t nCap = 120;
    std::atomic<int> requestedLimit{ 120 };
    std::atomic<bool> ready{ false };
    std::atomic<unsigned int> frameRateMode{ 3 };
    SafetyHookMid lookupModeHook, loadModeHook, menuModeHook;

    int32_t GetFrameRateCap(int32_t limit, HWND window)
    {
        if (limit == 0)
            return 1000;
        if (limit < 0)
        {
            const auto monitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFOEXW info{};
            info.cbSize = sizeof(info);
            DEVMODEW mode{};
            mode.dmSize = sizeof(mode);
            limit = GetMonitorInfoW(monitor, &info) && EnumDisplaySettingsW(info.szDevice, ENUM_CURRENT_SETTINGS, &mode) &&
                mode.dmDisplayFrequency > 1 ? static_cast<int32_t>(mode.dmDisplayFrequency) : 60;
        }
        return std::clamp(limit, 30, 1000);
    }

    void Apply(int32_t limit, HWND window)
    {
        nCap = GetFrameRateCap(limit, window);
        if (nVariableCap.is_initialized())
            nVariableCap = nCap;

        // VARIABLE is in effect: change the running cap too
        if (frameRateMode.load(std::memory_order_relaxed) == 0)
        {
            if (auto main = pMain.get())
                *reinterpret_cast<float*>(main + 0x3C) = static_cast<float>(nCap);
        }
    }
}

export namespace RevFrameRate
{
    void Frame(HWND window)
    {
        if (!ready.load(std::memory_order_acquire))
            return;
        static int applied = 0x7fffffff;
        static HMONITOR monitor = nullptr;
        static ULONGLONG lastCheck = 0;
        const int selected = requestedLimit.load(std::memory_order_relaxed);
        const auto currentMonitor = MonitorFromWindow(window, MONITOR_DEFAULTTOPRIMARY);
        const auto now = GetTickCount64();
        if (selected != applied || (selected < 0 && (monitor != currentMonitor || now - lastCheck >= 1000)))
        {
            Apply(selected, window);
            applied = selected;
            monitor = currentMonitor;
            lastCheck = now;
        }
    }
}

class FrameRate
{
public:
    FrameRate()
    {
        WFP::onInitEvent() += []()
        {
            auto pattern = hook::pattern("DB 04 85 ? ? ? ? 5E");
            nVariableCap.SetAddress(*pattern.get_first<int32_t*>(3));
            lookupModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
            {
                frameRateMode.store(context.eax, std::memory_order_relaxed);
            });
            pattern = hook::pattern("66 0F 6E 04 85 ? ? ? ? A1 ? ? ? ? 0F 5B C0 F3 0F 11 40 3C");
            loadModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
            {
                frameRateMode.store(context.eax, std::memory_order_relaxed);
            });
            pattern = hook::pattern("66 0F 6E 04 95 ? ? ? ? A1 ? ? ? ? 8B 54 24 ? 0F 5B C0 F3 0F 11 40 3C");
            menuModeHook = safetyhook::create_mid(pattern.get_first(), [](SafetyHookContext& context)
            {
                frameRateMode.store(context.edx, std::memory_order_relaxed);
            });

            pattern = hook::pattern("A1 ? ? ? ? 0F 5B C0 F3 0F 11 40 3C");
            pMain.SetAddress(*pattern.get_first<uintptr_t*>(1));

            requestedLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
            Apply(requestedLimit.load(std::memory_order_relaxed), nullptr);
            ready.store(true, std::memory_order_release);
            FixSettings().OnChange("MAIN.FPSLimit", []()
            {
                requestedLimit.store(FixSettings().Int("MAIN.FPSLimit"), std::memory_order_relaxed);
            });
        };
    }
} FrameRate;
