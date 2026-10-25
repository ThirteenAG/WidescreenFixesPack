#pragma once

#include "NativeMenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace DeerMenu
{
    enum class Kind { Link, Row, Previous, Next, Back };

    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
    };

    struct View
    {
        // Keep two content lines plus Previous/Next/Back within five rows.
        static constexpr size_t ContentRows = 2;
        int page = -1;
        size_t physical = 0;
        std::string title;
        std::vector<std::string> headings;
        std::vector<Item> items;

        // Caller stages a candidate View and publishes it only with the matching
        // native widget/text transaction. No stale visible-row mapping survives.
        bool Prepare(RWGTAMenu::Model& model, bool pause,
                     const std::function<bool()>& current, int width)
        {
            if (!current() || !model.IsOpen() || model.Pages().empty() || width < 80)
                return false;
            std::vector<Item> content;
            auto append = [&](Item item, const std::string& text)
            {
                if (!FitText(text, width, item.text) || !current() || content.size() >= 2048)
                    return false;
                content.push_back(std::move(item));
                return true;
            };
            if (page < 0)
            {
                title = model.Text("menu.plugins");
                const auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!append(Item{Kind::Link, {}, {}, i, true}, pages[i].title))
                        return false;
            }
            else
            {
                if (size_t(page) >= model.Pages().size())
                    return false;
                title = model.Pages()[size_t(page)].title;
                const auto visible = model.VisibleRows(size_t(page));
                if (!current())
                    return false;
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : visible)
                    copied.push_back(*row);
                for (auto row : copied)
                {
                    auto label = model.Label(row);
                    if (!current())
                        return false;
                    if (!row.plugin && (row.kind == RWGTAMenu::RowKind::Preset ||
                        row.kind == RWGTAMenu::RowKind::ApplyPreset))
                        continue;
                    auto value = model.Value(row);
                    if (!current())
                        return false;
                    const auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    std::string suffix = value.empty() ? std::string{} : ": " + value;
                    if (!enabled)
                        suffix += " " + model.Text("menu.unavailable");
                    std::string fittedSuffix, fittedLabel;
                    if (!FitText(suffix, width, fittedSuffix) ||
                        !FitText(label, std::max(0, width - int(fittedSuffix.size()) * Native::GlyphWidth), fittedLabel))
                        return false;
                    label = fittedLabel + fittedSuffix;
                    if (!append(Item{Kind::Row, {}, row, 0, enabled}, label))
                        return false;
                }
            }
            if (model.RestartRequired())
                title = model.Text("menu.restartwarning");
            if (!current() || !SplitText(title, width, headings) || headings.size() > 4)
                return false;
            const auto count = std::max(size_t(1), (content.size() + ContentRows - 1) / ContentRows);
            physical = std::min(physical, count - 1);
            items.clear();
            for (size_t i = physical * ContentRows; i < content.size() && i < (physical + 1) * ContentRows; ++i)
                items.push_back(std::move(content[i]));
            if (physical)
                items.push_back(Item{Kind::Previous, model.Text("menu.previous"), {}, 0, true});
            if (physical + 1 < count)
                items.push_back(Item{Kind::Next, model.Text("menu.next"), {}, 0, true});
            items.push_back(Item{Kind::Back, model.Text("menu.back"), {}, 0, true});
            for (auto& item : items)
            {
                item.text = PlainText(item.text);
                if (!TextFits(item.text, width, Native::GlyphHeight)) return false;
            }
            return current() && !items.empty() && items.size() <= 5;
        }
    };
}
