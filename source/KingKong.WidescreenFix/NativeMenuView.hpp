#pragma once

#include "NativeMenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace KongMenu
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
        static constexpr size_t ContentRows = 2;
        int page = -1;
        size_t physical = 0;
        std::string title;
        std::vector<Item> items;

        bool Prepare(RWGTAMenu::Model& model, Text& font, bool pause,
                     const std::function<bool()>& current, float width, HMODULE hostModule)
        {
            std::vector<Item> content;
            auto pageTitle = [&](const RWGTAMenu::Page& source)
            {
                if (!source.plugin || source.plugin->module != hostModule) return source.title;
                const auto name = OtherPlugins::Name(*source.plugin);
                return source.title.rfind(name, 0) == 0 ?
                    std::string("King Kong") + source.title.substr(name.size()) : source.title;
            };
            auto append = [&](Item item, const std::string& source)
            {
                std::vector<std::string> pieces;
                if (!font.Split(source, width, 0.065f, pieces) || !current()) return false;
                for (auto& piece : pieces)
                {
                    item.text = std::move(piece);
                    content.push_back(item);
                    if (content.size() > 2048) return false;
                }
                return true;
            };
            if (!current() || !model.IsOpen() || model.Pages().empty()) return false;
            if (page < 0)
            {
                title = model.Text("menu.title");
                const auto pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!append(Item{ Kind::Link, {}, {}, i, true }, pageTitle(pages[i]))) return false;
            }
            else
            {
                if (size_t(page) >= model.Pages().size()) return false;
                title = pageTitle(model.Pages()[size_t(page)]);
                auto rows = model.VisibleRows(size_t(page));
                if (!current()) return false;
                // Copy before any further plugin call can rebuild Model pages.
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : rows) copied.push_back(*row);
                for (auto row : copied)
                {
                    auto label = model.Label(row);
                    if (!current()) return false;
                    if (row.kind == RWGTAMenu::RowKind::Preset || row.kind == RWGTAMenu::RowKind::ApplyPreset)
                    {
                        auto scope = row.plugin && row.plugin->module == hostModule ? std::string("King Kong") :
                            row.plugin ? OtherPlugins::Name(*row.plugin) : model.Text("menu.gamepreset");
                        label = scope + ": " + label;
                    }
                    auto value = model.Value(row);
                    if (!current()) return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current()) return false;
                    if (!value.empty()) label += ": " + value;
                    if (!enabled) label += " " + model.Text("menu.unavailable");
                    if (!append(Item{ Kind::Row, {}, row, 0, enabled }, label)) return false;
                }
            }
            if (!current()) return false;
            const size_t pages = std::max(size_t(1), (content.size() + ContentRows - 1) / ContentRows);
            physical = std::min(physical, pages - 1);
            items.clear();
            const auto begin = physical * ContentRows;
            for (size_t i = begin; i < content.size() && i < begin + ContentRows; ++i)
                items.push_back(std::move(content[i]));
            if (physical) items.push_back(Item{ Kind::Previous, model.Text("menu.previous"), {}, 0, true });
            if (physical + 1 < pages) items.push_back(Item{ Kind::Next, model.Text("menu.next"), {}, 0, true });
            items.push_back(Item{ Kind::Back, model.Text("menu.back"), {}, 0, true });
            // Title itself also uses complete physical lines, never truncation.
            std::vector<std::string> headings;
            if (!font.Split(title, width, 0.065f, headings) || headings.size() != 1 || !current()) return false;
            title = std::move(headings[0]);
            return !items.empty() && items.size() <= 5;
        }
    };
}
