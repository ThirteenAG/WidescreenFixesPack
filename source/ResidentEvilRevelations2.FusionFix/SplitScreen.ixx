module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>
#include <cstdlib>
#include <mutex>

export module SplitScreen;

import ComVars;

// Local split-screen co-op. The game gives each player an 800x360 viewport (of 1280x720) in a staggered layout and
// leaves the rest of the screen black. MAIN.SplitScreenLayout chooses between that layout and one half of the screen
// for each player: top and bottom, or left and right, which on a 32:9 screen gives each player a 16:9 view. The
// layout is set up in the 1280x720 space, which the game maps to a 16:9 area of the screen's height starting at the
// left edge; the fix's halves span 720 * aspect, and the game's own layout is centered on wider screens.

namespace
{
    struct Options
    {
        SplitScreen::Layout layout{ SplitScreen::Layout::TopBottom };
        bool swap{};
    };

    std::atomic<Options> requested{ Options{} };
    thread_local SplitScreen::ViewPair buildingPair;

    void UpdateOptions()
    {
        static std::mutex writers;
        const std::lock_guard lock(writers);
        const auto layout = std::atoi(FixSettings().Get("MAIN.SplitScreenLayout").c_str());
        const bool swap = FixSettings().Get("MAIN.SplitScreenSwapTopBottom") == "1";
        requested.store({ static_cast<SplitScreen::Layout>(std::clamp(layout, 0, 2)), swap }, std::memory_order_relaxed);
    }

    void(__stdcall* VirtualToScreenRect)(void* out, int32_t* rect) = nullptr;

    void SetHalf(int32_t* rect, int32_t half, const SplitScreen::ViewPair& pair)
    {
        if (pair.swap)
            half = 1 - half;
        const int32_t w = (int32_t)(720.0f * pair.screen.Aspect());
        if (pair.layout == SplitScreen::Layout::Original)
        {
            const int32_t offset = std::max(0, (w - 1280) / 2);
            rect[0] = offset + (half == 0 ? 136 : 344);
            rect[1] = half == 0 ? 0 : 360;
            rect[2] = rect[0] + SplitScreen::NativeResX;
            rect[3] = rect[1] + SplitScreen::NativeResY;
        }
        else if (pair.layout == SplitScreen::Layout::SideBySide)
        {
            rect[0] = half == 0 ? 0 : w / 2;
            rect[1] = 0;
            rect[2] = half == 0 ? w / 2 : w;
            rect[3] = 720;
        }
        else
        {
            rect[0] = 0;
            rect[1] = half == 0 ? 0 : 360;
            rect[2] = w;
            rect[3] = half == 0 ? 360 : 720;
        }
    }

    void __stdcall SplitScreenSetupTop(void* out, int32_t* rect)
    {
        const auto options = requested.load(std::memory_order_relaxed);
        buildingPair = { options.layout, options.swap, GetRenderSize() };
        // The native camera setup holds its lock throughout both conversions and assignments.
        for (int half = 0; half < 2; ++half)
        {
            SetHalf(rect, half, buildingPair);
            VirtualToScreenRect(&buildingPair.viewports[half], rect);
        }
        std::memcpy(out, &buildingPair.viewports[0], sizeof(RECT));
    }

    void __stdcall SplitScreenSetupBottom(void* out, int32_t*)
    {
        std::memcpy(out, &buildingPair.viewports[1], sizeof(RECT));
    }

    void Refresh(SafetyHookContext& context)
    {
        if (!IsSplitScreenActive()) return;
        const auto pair = SplitScreen::InstalledPair();
        const auto options = requested.load(std::memory_order_relaxed);
        if (pair.layout != options.layout || pair.swap != options.swap || pair.screen != GetRenderSize())
        {
            // Ask the camera manager to rebuild both viewports through its native setup path.
            *reinterpret_cast<uint8_t*>(context.esi + 0xD0C) = 1;
        }
    }

    void Publish(SafetyHookContext&)
    {
        SplitScreen::PublishPair(buildingPair);
    }
}

class SplitScreenFix
{
public:
    SplitScreenFix()
    {
        WFP::onInitEvent() += []()
        {
            auto& settings = FixSettings();
            settings.OnChange("MAIN.SplitScreenLayout", UpdateOptions);
            settings.OnChange("MAIN.SplitScreenSwapTopBottom", UpdateOptions);
            UpdateOptions();

            auto top = hook::pattern("E8 ? ? ? ? 6A 01 6A 00 8B CE E8 ? ? ? ? 8D 44 24 30");
            auto bottom = hook::pattern("E8 ? ? ? ? 6A 01 6A 01 8B CE E8 ? ? ? ? 8D 44 24 30");

            VirtualToScreenRect = (decltype(VirtualToScreenRect))injector::GetBranchDestination(top.get_first()).as_int();
            injector::MakeCALL(top.get_first(), SplitScreenSetupTop, true);
            injector::MakeCALL(bottom.get_first(), SplitScreenSetupBottom, true);
            auto dirty = hook::pattern("8A 9E 0C 0D 00 00 84 DB 75 ? 8B 0D");
            auto assigned = hook::pattern("C7 86 10 0D 00 00 02 00 00 00 C7 86 1C 0D 00 00 01 00 00 00");
            static auto refresh = safetyhook::create_mid(dirty.get_first(), Refresh);
            static auto publish = safetyhook::create_mid(assigned.get_first(), Publish);
        };
    }
} SplitScreenFix;
