#pragma once

#include "NativeMenuWidgets.hpp"
#include "NativeMenuView.hpp"

namespace CthulhuMenu
{
    // The native page owns every row and its renderer/literal allocations. This
    // record is only an identity snapshot, cleared before native row retirement.
    struct Surface
    {
        static constexpr size_t Count = 8;
        static constexpr int ActionID = 0x7F01;
        std::array<void*, Count> rows{};
        void** begin = nullptr;
        void* page = nullptr;
        char* helpBuffer = nullptr;

        void Forgotten()
        {
            rows.fill(nullptr);
            begin = nullptr;
            page = nullptr;
            helpBuffer = nullptr;
        }

        bool Alive() const
        {
            if (!page || !Readable(page, Native::PageSize) || Read<uintptr_t>(page, 0) != 0x62CAEC ||
                Read<void**>(page, 8) != begin || !Readable(begin, Count * sizeof(void*)) ||
                reinterpret_cast<uintptr_t>(Read<void**>(page, 12)) !=
                    reinterpret_cast<uintptr_t>(begin) + Count * sizeof(void*))
                return false;
            for (size_t i = 0; i < Count; ++i)
                if (!rows[i] || Read<void*>(begin, i * sizeof(void*)) != rows[i] ||
                    !Readable(rows[i], Native::TextRowSize) || Read<uintptr_t>(rows[i], 0) != 0x62C7AC ||
                    Read<void*>(rows[i], 268) != page)
                    return false;
            return true;
        }

        bool Selected(size_t item) const
        {
            return item < Count - 1 && Alive() && Read<int>(page, 540) == int(item + 1) &&
                Read<uint8_t>(rows[item + 1], 273) && Read<int>(rows[item + 1], 264) == ActionID;
        }

        static bool PlanRows(const std::array<std::array<float, 4>, 7>& source,
                             std::array<std::array<float, 4>, Count>& rectangles)
        {
            for (size_t i = 0; i < source.size(); ++i)
            {
                const auto& rect = source[i];
                for (auto value : rect) if (!std::isfinite(value) || value < 0 || value > 1) return false;
                if (rect[2] - rect[0] <= 0.05f || rect[3] - rect[1] < 0.012f || (i && rect[1] < source[i - 1][3])) return false;
                rectangles[i] = rect;
            }
            const auto& last = source.back();
            rectangles.back() = { last[0], last[3] + 0.005f, last[2], last[3] + 0.005f + last[3] - last[1] };
            return rectangles.back()[3] <= 0.90f;
        }

        bool Build(void* destination, const Context& source, const View& view, float fontHeight,
                   const std::function<bool()>& current)
        {
            if (page || view.items.empty() || view.items.size() >= Count || !current() ||
                !Readable(destination, Native::PageSize) || Read<uintptr_t>(destination, 0) != 0x62CAEC ||
                Read<void**>(destination, 8) != Read<void**>(destination, 12))
                return false;
            // Keep all seven stock positions, with a disabled help row below.
            // The private page alone grows; stock Options geometry is borrowed.
            // Copy only proved scalar geometry/style, never native owner pointers.
            std::array<std::array<float, 4>, 7> borrowed{};
            std::array<std::array<float, 4>, Count> rectangles{};
            for (size_t i = 0; i < source.rows.size(); ++i)
            {
                if (!source.Alive()) return false;
                for (size_t j = 0; j < 4; ++j) borrowed[i][j] = Read<float>(source.rows[i], 280 + j * 4);
            }
            // The disabled help row never receives the native focus scale.
            // The active page's renderer was checked before ClearRows. Stock
            // Options renderers are released while the private page is shown.
            if (!PlanRows(borrowed, rectangles) || !std::isfinite(fontHeight) || fontHeight <= 0.0f ||
                fontHeight > rectangles.back()[3] - rectangles.back()[1]) return false;
            page = destination;
            // The constructor does not initialize these rectangles. Native
            // AddRow reads page588/596/currentY before assigning row geometry,
            // so establish its normal setter preconditions before any factory.
            std::array<float, 4> bounds{}, heading{};
            for (size_t j = 0; j < 4; ++j)
            {
                bounds[j] = Read<float>(source.options, 588 + j * 4);
                heading[j] = Read<float>(source.options, 572 + j * 4);
                if (!std::isfinite(bounds[j]) || !std::isfinite(heading[j]) ||
                    bounds[j] < 0.0f || bounds[j] > 1.0f || heading[j] < 0.0f || heading[j] > 1.0f)
                    return false;
            }
            if (bounds[2] <= bounds[0] || bounds[3] <= bounds[1] ||
                heading[2] <= heading[0] || heading[3] <= heading[1] || !current())
                return false;
            bounds[3] = std::max(bounds[3], rectangles.back()[3]);
            Native::Bounds(page, bounds.data());
            if (!current())
                return false;
            Native::HeadingBounds(page, heading.data());
            if (!current())
                return false;
            const auto style = Read<uint8_t>(source.options, 620);
            if (style > 1)
                return false;
            Write<uint8_t>(page, 620, style);
            for (int configuration = 0; configuration < 2; ++configuration)
            {
                Native::HeaderBack(page, configuration, 1, "OPTIONS");
                if (!current())
                    return false;
                // Stock Start goes through the current selected-row Accept.
                Native::HeaderStart(page, configuration, 23, "");
                if (!current())
                    return false;
            }
            // Constructor leaves native localized heading disabled (-1). Own
            // heading row preserves the complete shared page title instead.
            for (size_t i = 0; i < Count; ++i)
            {
                if (!current())
                    return false;
                rows[i] = Native::AddRow(page, "TEXTOPTION");
                if (!current() || !Readable(rows[i], Native::TextRowSize) ||
                    Read<uintptr_t>(rows[i], 0) != 0x62C7AC || Read<void*>(rows[i], 268) != page)
                    return false;
                auto row = rows[i];
                for (size_t j = 0; j < 4; ++j)
                    Write<float>(row, 280 + j * 4, rectangles[i][j]);
                // All private labels use the normal source font/scales and color.
                for (size_t offset : {size_t(332), size_t(336), size_t(344), size_t(348), size_t(352), size_t(356)})
                    Write<uint32_t>(row, offset, Read<uint32_t>(source.rows[0], offset));
                for (size_t offset : {size_t(276), size_t(277), size_t(340), size_t(341)})
                    Write<uint8_t>(row, offset, Read<uint8_t>(source.rows[0], offset));
                const auto footer = i == Count - 1;
                const auto active = i > 0 && !footer && i - 1 < view.items.size();
                const auto text = i == 0 ? view.title : footer ? view.help : active ? view.items[i - 1].text : std::string{};
                Native::Label(row, -1);
                if (!current())
                    return false;
                char* previous = nullptr;
                if (footer)
                {
                    if (text.size() >= 4096 || Read<void*>(row, 328)) return false;
                    helpBuffer = static_cast<char*>(Native::AllocateBuffer(4096, 7, ".\\CoCControlTextOption.cpp", 83, 1));
                    if (!helpBuffer || !current())
                    {
                        if (helpBuffer) Native::FreeBuffer(helpBuffer);
                        helpBuffer = nullptr; return false;
                    }
                    std::memcpy(helpBuffer, text.c_str(), text.size() + 1);
                    Write<char*>(row, 328, helpBuffer);
                }
                else if (!ReplaceLiteral(row, text, current, previous)) return false;
                // Newly constructed TEXTOPTION must not carry an old allocation.
                // If it does, retain the page for checked native deletion.
                if (previous)
                {
                    Native::FreeBuffer(previous);
                    return false;
                }
                Native::Action(row, active ? ActionID : 0, "", 0);
                if (!current())
                    return false;
                Write<uint8_t>(row, 273, active ? 1 : 0);
                // Availability affects activation, not navigation: disabled rows
                // remain readable and reachable, and an enabled Back always exists.
                const auto editable = active && view.items[i - 1].kind == Kind::Row &&
                    (view.items[i - 1].row.kind == RWGTAMenu::RowKind::Setting ||
                     view.items[i - 1].row.kind == RWGTAMenu::RowKind::Preset);
                Write<uint32_t>(row, 308, active ? (editable ? 0x1Cu : 0x10u) : 0u);
            }
            begin = Read<void**>(page, 8);
            return current() && Alive();
        }

        bool Help(const std::string& text, const std::function<bool()>& current) const
        {
            if (!current() || !Alive() || text.size() >= 4096 || !Readable(helpBuffer, 4096) || Read<char*>(rows.back(), 328) != helpBuffer) return false;
            auto renderer = Read<void*>(rows.back(), 320);
            if (!Readable(renderer, 352) || Read<uintptr_t>(renderer, 0) != 0x62FAF0) return false;
            std::memcpy(helpBuffer, text.c_str(), text.size() + 1);
            Native::CopyText(renderer, helpBuffer);
            if (!current() || !Alive()) return false;
            auto stored = Read<const char*>(renderer, 300);
            if (!Readable(stored, text.size() + 1) || std::memcmp(stored, text.c_str(), text.size() + 1)) return false;
            if (text.empty()) return true;
            Native::LayoutText(renderer);
            FontProbe probe; float width = 0.0f;
            return current() && Alive() && probe.Capture(rows.back()) && probe.Width(text, width) &&
                width < Read<float>(rows.back(), 288) - Read<float>(rows.back(), 280);
        }

        bool Rendered(const View& view, const std::function<bool()>& current) const
        {
            if (!Alive())
                return false;
            for (size_t i = 0; i < view.items.size() + 1; ++i)
            {
                if (!current())
                    return false;
                auto renderer = Read<void*>(rows[i], 320);
                auto backend = Read<void*>(reinterpret_cast<void*>(0x6F55B0), 0);
                if (!Readable(renderer, 352) || Read<uintptr_t>(renderer, 0) != 0x62FAF0 ||
                    !Readable(backend, 20) || Read<uintptr_t>(backend, 0) != 0x68AC3C)
                    return false;
                // Native BuildRenderers copies text with layout=false. Generate
                // the owning renderer's pool entry before measuring it; a fresh
                // empty pool is not evidence of unsupported font readiness.
                Native::LayoutText(renderer);
                if (!current() || !Alive())
                    return false;
                FontProbe probe;
                const auto& text = i ? view.items[i - 1].text : view.title;
                float width = 0.0f;
                if (!probe.Capture(rows[i]) || !probe.Width(text, width) ||
                    width >= Read<float>(rows[i], 288) - Read<float>(rows[i], 280))
                    return false;
            }
            return Help(view.help, current);
        }
    };
}
