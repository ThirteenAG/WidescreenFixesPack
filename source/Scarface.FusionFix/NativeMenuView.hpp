#pragma once
#include "NativeMenuText.hpp"
#include "Shared/RWGTA/FixMenu.hpp"

namespace ScarfaceMenu
{
    // The plugins list, a plugin's settings or categories, or a category's settings, as the rows of the pause menu's
    // Options page under its section header (the list's title). Rows are indented under it like the game's options and
    // a setting's value is in the page's value column between its arrows. A label too long for the page continues on
    // the next lines. Each list is one list: the page's own list scrolls through it. Back returns one level, to the
    // row the list was opened from.
    struct View
    {
        enum class Kind { Row, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            const RWGTAMenu::Row* row = nullptr;
            bool enabled = false;
            std::wstring text;
            bool hasValue = false;          // the row's first line shows the setting's value
            std::wstring value;
        };

        size_t page = RWGTAMenu::Model::RootPage;
        size_t selected = 0;
        std::string notice;
        std::vector<Item> items;
        std::vector<size_t> selection;      // the line each list was left on, by page

        bool Build(RWGTAMenu::Model& model, bool paused)
        {
            if (!model.IsOpen() || page >= model.Pages().size()) return false;
            std::vector<Item> complete;
            // Labels beside a value keep left of the page's value column
            auto add = [&](Item item, const std::string& value, size_t width = 30)
            {
                std::vector<std::wstring> lines;
                if (!Text::Lines(value, lines, width)) return false;
                for (auto& line : lines)
                {
                    auto shown = item;
                    shown.text = std::move(line);
                    complete.push_back(std::move(shown));
                }
                return true;
            };
            if (!notice.empty())
            {
                if (!add(Item{}, notice)) return false;
            }
            else
            {
                for (auto row : model.VisibleRows(page))
                {
                    bool opens = RWGTAMenu::Model::Opens(*row);
                    bool enabled = opens || model.Enabled(*row, paused);
                    std::wstring value;
                    if (!opens && !Text::Convert(model.Value(*row), value)) return false;
                    const size_t first = complete.size();
                    if (!add(Item{ Kind::Row, row, enabled }, model.Label(*row), value.empty() ? 30 : 22)) return false;
                    if (!value.empty())
                    {
                        complete[first].hasValue = true;
                        complete[first].value = std::move(value);
                    }
                }
            }
            if (complete.empty() || complete.size() > 768) return false;
            items = std::move(complete);
            selected = (std::min)(selected, items.size() - 1);
            if (!items[selected].enabled) Move(1);
            return true;
        }

        void Move(int direction)
        {
            if (items.empty()) return;
            for (size_t attempt = 0; attempt < items.size(); ++attempt)
            {
                selected = (selected + items.size() + direction) % items.size();
                if (items[selected].enabled) break;
            }
        }

        // The line a list was left on, when it is shown again
        void Keep()
        {
            if (selection.size() <= page) selection.resize(page + 1);
            selection[page] = selected;
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (!notice.empty()) return false;
            size_t parent = model.Back(page);
            if (parent == SIZE_MAX) return false;
            page = parent;
            selected = page < selection.size() ? selection[page] : 0;
            return true;
        }

        bool Activate(RWGTAMenu::Model& model, bool paused, int direction)
        {
            if (selected >= items.size() || !items[selected].enabled) return true;
            auto item = items[selected];
            if (direction && item.kind != Kind::Row) return true;
            switch (item.kind)
            {
                case Kind::Row:
                    if (RWGTAMenu::Model::Opens(*item.row))
                    {
                        if (direction) return true;
                        size_t next = model.Enter(*item.row);
                        if (next != SIZE_MAX)
                        {
                            Keep();
                            page = next;
                            selected = 0;
                        }
                        break;
                    }
                    model.Change(*item.row, direction, paused);
                    break;
                default:
                    break;
            }
            return true;
        }
    };
}
