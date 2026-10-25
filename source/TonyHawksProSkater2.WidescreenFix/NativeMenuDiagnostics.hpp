#pragma once

#include <array>
#include <cstdio>
#include <Windows.h>

namespace THPS2Menu
{
    enum class Diagnostic : size_t
    {
        UnsupportedImage, PinFailed, HookFailed,
        TitleContext, PauseContext, Style, Widget, Layout, Discovery,
        TitleStyle, TitleWidget, PauseStyle, PauseWidget, PauseLayout,
        Rollback, OwnerRetired, FrameReentry, Count
    };

    // Failure reasons only, each reported at most three times.
    inline void Diagnose(Diagnostic reason, const char* detail = nullptr, size_t rows = 0) noexcept
    {
        static std::array<uint8_t, static_cast<size_t>(Diagnostic::Count)> seen{};
        auto index = static_cast<size_t>(reason);
        if (index >= seen.size() || seen[index] >= 3)
            return;
        ++seen[index];
        static constexpr std::array<const char*, static_cast<size_t>(Diagnostic::Count)> names =
        {
            "unsupported image", "module pin failed", "hook transaction failed",
            "title context unavailable", "pause context unavailable", "style unavailable", "widget refused", "layout refused", "discovery retired",
            "title style unavailable", "title widget refused", "pause style unavailable", "pause widget refused", "pause geometry refused",
            "construction rollback", "native owner retired", "frame reentry rejected"
        };
        if (!detail)
            detail = names[index];
        char line[256] = {};
        auto count = std::snprintf(line, sizeof(line), "THPS2 native menu: %s; rows=%zu\n", detail, rows);
        if (count > 0)
            OutputDebugStringA(line);
    }
}
