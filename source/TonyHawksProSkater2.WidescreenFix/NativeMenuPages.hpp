#pragma once

#include "../Shared/RWGTA/FixMenu.hpp"
#include "NativeMenuWidgets.hpp"
#include <functional>
#include <limits>

namespace THPS2Menu
{
    enum class ItemKind { Page, Row, Back, Previous, Next };

    struct Item
    {
        ItemKind kind = ItemKind::Back;
        size_t page = 0;
        RWGTAMenu::Row row;
        std::string label;
        bool enabled = true;
    };

    struct View
    {
        static constexpr size_t Root = std::numeric_limits<size_t>::max();
        static constexpr size_t PageLinks = 3;
        static constexpr size_t ContentLines = 7;
        static constexpr int PhysicalPrevious = 0x7FF0;
        static constexpr int PhysicalNext = 0x7FF1;
        static constexpr int PhysicalBack = 0x7FF2;

        size_t page = Root;
        size_t rootStart = 0;
        size_t physical = 0;
        std::vector<Item> items;
        std::vector<DisplayLine> text;
        std::vector<size_t> starts;

        void Reset()
        {
            page = Root;
            rootStart = physical = 0;
            items.clear();
            text.clear();
            starts.clear();
        }

        // Foreign Visible/Get/GetState returns can retire or reopen the host.
        // Copy row names before reads, then validate the exact host snapshot
        // immediately after each foreign return before touching another row.
        bool Prepare(RWGTAMenu::Model& model, bool pause, const std::function<bool()>& current)
        {
            if (!current() || !model.IsOpen())
                return false;
            std::vector<Item> next;
            std::string heading = model.Text("menu.plugins");
            if (page == Root)
            {
                auto count = model.Pages().size();
                if (rootStart >= count)
                    rootStart = count ? ((count - 1) / PageLinks) * PageLinks : 0;
                for (size_t i = rootStart; i < std::min(rootStart + PageLinks, count); ++i)
                    next.push_back({ ItemKind::Page, i, {}, model.Pages()[i].title, true });
                if (rootStart)
                    next.push_back({ ItemKind::Previous, 0, {}, model.Text("menu.previous"), true });
                if (rootStart + PageLinks < count)
                    next.push_back({ ItemKind::Next, 0, {}, model.Text("menu.next"), true });
            }
            else
            {
                if (page >= model.Pages().size())
                    return false;
                heading = model.Pages()[page].title;
                auto rows = model.VisibleRows(page);
                if (!current())
                    return false;
                for (const auto* described : rows)
                {
                    auto row = *described;
                    auto label = model.Label(row);
                    auto value = model.Value(row);
                    if (!current())
                        return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    if (!value.empty())
                        label += ": " + value;
                    next.push_back({ ItemKind::Row, page, row, std::move(label), enabled });
                }
            }
            if (!current())
                return false;
            std::vector<DisplayLine> nextText;
            for (const auto& line : TextLines(PlainText(heading), 31))
                nextText.push_back({ line, -1, false, false, true });
            for (size_t i = 0; i < next.size(); ++i)
            {
                auto lines = TextLines(PlainText(next[i].label), 31);
                for (size_t k = 0; k < lines.size(); ++k)
                    nextText.push_back({ lines[k], k == 0 ? static_cast<int>(i) : -1, k == 0 && next[i].enabled, k != 0, false });
            }
            if (!current())
                return false;
            items = std::move(next);
            text = std::move(nextText);

            // Seven content lines, two help rows and previous/next/back stay
            // within the twelve native positions. Full values retain their
            // nonselectable continuation lines.
            starts.clear();
            for (size_t start = 0; start < text.size(); start += ContentLines)
                starts.push_back(start);
            if (starts.empty())
                starts.push_back(0);
            if (physical >= starts.size())
                physical = starts.size() - 1;
            return true;
        }

        std::vector<DisplayLine> Lines(RWGTAMenu::Model& model, int selected = -1) const
        {
            if (starts.empty() || physical >= starts.size())
                return {};
            auto start = starts[physical];
            auto count = std::min(ContentLines, text.size() - start);
            std::vector<DisplayLine> result(text.begin() + start, text.begin() + start + count);
            std::string help;
            if (selected >= 0 && static_cast<size_t>(selected) < items.size() && items[selected].kind == ItemKind::Row)
                help = PlainText(model.Help(items[selected].row));
            const bool restart = model.RestartRequired();
            if (restart) result.push_back({ FitText(PlainText(model.Text("menu.restartwarning")), 31), -1, false });
            if (!help.empty())
            {
                auto lines = TextLines(help, 31);
                auto maximum = restart ? size_t(1) : size_t(2);
                for (size_t i = 0; i < std::min(maximum, lines.size()); ++i)
                {
                    auto line = lines[i];
                    if (i + 1 == maximum && lines.size() > maximum) line = FitText(line + "...", 31);
                    result.push_back({ std::move(line), -1, false });
                }
            }
            if (physical)
                result.push_back({ PlainText(model.Text("menu.previous")), PhysicalPrevious, true });
            if (physical + 1 < starts.size())
                result.push_back({ PlainText(model.Text("menu.next")), PhysicalNext, true });
            result.push_back({ PlainText(model.Text("menu.back")), PhysicalBack, true });
            return result;
        }
    };
}
