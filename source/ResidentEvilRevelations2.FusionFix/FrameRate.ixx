module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>

export module FrameRate;

import ComVars;

// Frame rate. The game's Frame Rate option offers 30, 60 and VARIABLE, which caps the game at 120 FPS. With VARIABLE
// the fix caps it at MAIN.FPSLimit instead: 0 = unlimited (1000), -1 = the monitor's refresh rate, or a number. The
// game builds its table of the three caps whenever it reads the option.

namespace
{
    std::vector<ProtectedGameRef<int32_t>> VariableCaps;
    GameRef<uintptr_t> pMain;   // the game's main object: the frame rate cap in effect at +0x3C
    int32_t nCap = 120;
    std::atomic<int> requestedLimit{ 120 };
    std::atomic<bool> ready{ false };
    std::atomic<unsigned int> nFrameRateMode{ 3 };
    SafetyHookInline shLookupMode = {}, shSetMode = {};

    int __stdcall LookupMode(unsigned int mode)
    {
        nFrameRateMode.store(mode, std::memory_order_relaxed);
        return shLookupMode.unsafe_stdcall<int>(mode);
    }

    int __fastcall SetMode(void* object, void*, unsigned int mode)
    {
        nFrameRateMode.store(mode, std::memory_order_relaxed);
        return shSetMode.unsafe_thiscall<int>(object, mode);
    }

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
        for (auto& cap : VariableCaps)
            cap = nCap;

        // VARIABLE is in effect: change the running cap too
        if (nFrameRateMode.load(std::memory_order_relaxed) == 2 && pMain.is_initialized())
        {
            if (auto main = pMain.get())
                *(float*)(main + 0x3C) = (float)nCap;
        }
    }
}

export namespace Rev2FrameRate
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
            // mov [esp+8], 120: the VARIABLE entry of the table
            for (auto ref : { "C7 44 24 08 78 00 00 00 83 F8 03 72 0B", "C7 44 24 08 78 00 00 00 83 F8 03 72 07" })
            {
                auto pattern = hook::pattern(ref);
                VariableCaps.emplace_back(pattern.get_first<int32_t>(4));
            }

            auto pattern = hook::pattern("A1 ? ? ? ? 66 0F 5A C0 F3 0F 11 40 3C");
            pMain.SetAddress(*pattern.get_first<uintptr_t*>(1));

            pattern = hook::pattern("83 EC 0C 8B 44 24 10 C7 04 24 1E 00 00 00 C7 44 24 04 3C 00 00 00");
            shLookupMode = safetyhook::create_inline(pattern.get_first(), LookupMode);
            pattern = hook::pattern("83 EC 0C 8B 44 24 10 89 81 ? ? ? ? C7 04 24 1E 00 00 00");
            shSetMode = safetyhook::create_inline(pattern.get_first(), SetMode);

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
