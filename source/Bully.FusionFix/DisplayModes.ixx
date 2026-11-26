module;

#include <stdafx.h>

export module DisplayModes;

import ComVars;

export namespace DisplayModes
{
    std::vector<int32_t> Modes;
    std::vector<int32_t**> References;

    SafetyHookMid ValidationHook, ModeCountHook, ScreenListHook;

    void Init(HMODULE module = GetModuleHandle(nullptr))
    {
        auto validation = hook::module_pattern(module, "A1 ? ? ? ? 69 C0 E8 03 00 00 99 F7 3D ? ? ? ? 05 1E FB FF FF 3D 26 02 00 00 0F 96 C0 C3");
        DisplayWidth.SetAddress(*validation.get_first<int*>(1));
        DisplayHeight.SetAddress(*validation.get_first<int*>(14));
        static auto resume = reinterpret_cast<uintptr_t>(validation.get_first(31));
        ValidationHook = safetyhook::create_mid(validation.get_first(), [](SafetyHookContext& regs)
        {
            regs.eax = DisplayWidth >= 640 && DisplayHeight >= 480 && DisplayWidth <= 16384 && DisplayHeight <= 16384;
            regs.eip = resume;
        });
        auto count = hook::module_pattern(module, "8B 50 18 51 56 FF D2 8B D8 3B DD 75 ? 8B 0D");
        auto table = hook::module_pattern(module, "B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC B8 ? ? ? ? C3 CC CC CC CC CC CC CC CC CC CC 81 EC 8C 05 00 00 53 55 33 ED 56 57");
        auto original = *table.get_first<int32_t*>(1);
        constexpr std::pair<const char*, int> sites[] =
        {
            { "B8", 1 }, { "89 04 D5", 3 }, { "39 3C C5", 3 }, { "89 3C CD", 3 }, { "39 34 C5", 3 },
            { "89 0C C5", 3 }, { "39 14 C5", 3 }, { "89 14 C5", 3 }, { "8B 3C C5", 3 }
        };
        for (size_t i = 0; i < std::size(sites); ++i)
        {
            auto signature = std::string(sites[i].first) + " " + pattern_str(to_bytes(original + (i >= 5 ? 1 : 0)));
            auto reference = hook::module_pattern(module, signature);
            References.push_back(reference.get_first<int32_t*>(sites[i].second));
        }
        // The native Video screen has space for 128 width/height pairs followed by the count.
        auto screen = hook::module_pattern(module, "E8 ? ? ? ? 8B 88 00 04 00 00 89 8E 18 04 00 00 33 C9 39 88 00 04 00 00");
        NativeDisplayModeCount.SetAddress(original + 256);
        ScreenListHook = safetyhook::create_mid(screen.get_first(5), [](SafetyHookContext& regs)
        {
            static int32_t list[257] = {};
            int32_t count = std::clamp(NativeDisplayModeCount.get(), 0, int32_t(Modes.size() / 2));
            using Size = std::pair<int32_t, int32_t>;
            MONITORINFO monitor{ sizeof(monitor) };
            GetMonitorInfo(MonitorFromWindow(hGameWindow.get(), MONITOR_DEFAULTTOPRIMARY), &monitor);
            const Size desktop{ monitor.rcMonitor.right - monitor.rcMonitor.left, monitor.rcMonitor.bottom - monitor.rcMonitor.top };
            const Size active{ DisplayWidth.get(), DisplayHeight.get() };
            std::vector<Size> choices;
            choices.reserve(count);
            for (int32_t i = 0; i < count; ++i)
            {
                const Size size{ Modes[i * 2], Modes[i * 2 + 1] };
                if (std::find(choices.begin(), choices.end(), size) == choices.end())
                    choices.push_back(size);
            }
            size_t skip = choices.size() > 128 ? choices.size() - 128 : 0;
            int32_t shown = 0;
            for (const auto& size : choices)
            {
                if (skip && size != choices.front() && size != active && size != desktop)
                {
                    --skip;
                    continue;
                }
                list[shown * 2] = size.first;
                list[shown * 2 + 1] = size.second;
                ++shown;
            }
            list[256] = shown;
            regs.eax = reinterpret_cast<uintptr_t>(list);
        });
        ModeCountHook = safetyhook::create_mid(count.get_first(7), [](SafetyHookContext& regs)
        {
            // The native list includes one 640x480 entry in addition to the adapter's modes.
            Modes.resize((static_cast<size_t>(regs.eax) + 1) * 2);
            for (size_t i = 0; i < References.size(); ++i)
                injector::WriteMemory<int32_t*>(References[i], Modes.data() + (i >= 5 ? 1 : 0), true);
        });
    }
}
