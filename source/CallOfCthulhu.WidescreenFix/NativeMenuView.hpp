#pragma once

#include "NativeMenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace CthulhuMenu
{
    enum class Kind { Link, Row, Previous, Next, Back };

    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
        std::string help;
    };

    struct View
    {
        static constexpr size_t ContentRows = 3;
        int page = -1;
        size_t physical = 0;
        std::string title, help, restart;
        std::vector<Item> items;

        bool Footer(size_t selected, const FontMetrics& font, float width, std::string& output) const
        {
            const auto text = !restart.empty() ? restart : selected < items.size() ? items[selected].help : std::string{};
            return font.Fit(text, width, output);
        }

        bool Prepare(RWGTAMenu::Model& model, const FontMetrics& font, bool pause,
                     const std::function<bool()>& current, float width)
        {
            if (!current() || !model.IsOpen() || model.Pages().empty())
                return false;
            bool encodingValid = true;
            auto native = [&](const std::string& text)
            {
                std::string encoded;
                encodingValid &= font.Encode(text, encoded);
                return encoded;
            };
            std::vector<Item> content;
            auto append = [&](Item item, const std::string& text)
            {
                if (!font.Fit(text, width, item.text) || !current() || content.size() >= 2048)
                    return false;
                content.push_back(std::move(item));
                return true;
            };
            if (page < 0)
            {
                title = native(model.Text("menu.plugins"));
                const auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!append(Item{Kind::Link, {}, {}, i, true}, native(pages[i].title)))
                        return false;
            }
            else
            {
                if (size_t(page) >= model.Pages().size())
                    return false;
                title = native(model.Pages()[size_t(page)].title);
                auto visible = model.VisibleRows(size_t(page));
                if (!current())
                    return false;
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : visible)
                    copied.push_back(*row);
                for (auto row : copied)
                {
                    auto label = native(model.Label(row));
                    if (!current())
                        return false;
                    // Presets belong to their plugin; the page already names it.
                    if (!row.plugin && (row.kind == RWGTAMenu::RowKind::Preset ||
                        row.kind == RWGTAMenu::RowKind::ApplyPreset))
                        continue;
                    auto value = native(model.Value(row));
                    if (!current())
                        return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current())
                        return false;
                    std::string suffix = value.empty() ? std::string{} : ": " + value;
                    if (!enabled)
                        suffix += " " + native(model.Text("menu.unavailable"));
                    std::string fitted;
                    float suffixWidth = 0.0f;
                    if (!font.Fit(suffix, width, suffix) || !font.Width(suffix, suffixWidth) ||
                        !font.Fit(label, std::max(0.0f, width - suffixWidth), fitted))
                        return false;
                    label = fitted + suffix;
                    auto description = native(model.Help(row));
                    if (!current()) return false;
                    if (!append(Item{Kind::Row, {}, row, 0, enabled, std::move(description)}, label))
                        return false;
                }
            }
            if (!current())
                return false;
            restart = model.RestartRequired() ? native(model.Text("menu.restartwarning")) : std::string{};
            if (!font.Fit(title, width, title))
                return false;
            const auto count = std::max(size_t(1), (content.size() + ContentRows - 1) / ContentRows);
            physical = std::min(physical, count - 1);
            items.clear();
            for (size_t i = physical * ContentRows; i < content.size() && i < (physical + 1) * ContentRows; ++i)
                items.push_back(std::move(content[i]));
            if (physical)
                items.push_back(Item{Kind::Previous, native(model.Text("menu.previous")), {}, 0, true});
            if (physical + 1 < count)
                items.push_back(Item{Kind::Next, native(model.Text("menu.next")), {}, 0, true});
            items.push_back(Item{Kind::Back, native(model.Text("menu.back")), {}, 0, true});
            for (const auto& item : items)
            {
                float measured = 0.0f;
                if (!font.Width(item.text, measured) || measured >= width)
                    return false;
            }
            return encodingValid && current() && !items.empty() && items.size() <= 6;
        }
    };
}
