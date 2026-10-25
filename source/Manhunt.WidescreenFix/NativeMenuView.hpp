#pragma once
#include "NativeMenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace MHMenu
{
    struct View
    {
        enum class Kind { Page, Row, Previous, Next, Details, Back, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            size_t index = 0;
            RWGTAMenu::Row row{};
            bool enabled = false;
            std::wstring text;
        };

        size_t page = static_cast<size_t>(-1);
        size_t physical = 0;
        size_t selected = 0;
        size_t count = 1;
        bool details = false;
        RWGTAMenu::Row helpRow{};
        std::vector<Item> items;

        bool Build(RWGTAMenu::Model& model, bool paused)
        {
            if (!model.IsOpen()) return false;
            std::vector<Item> complete;
            auto add = [&](Item item, const std::string& value)
            {
                std::vector<std::wstring> lines;
                if (!Text::Lines(value, lines)) return false;
                for (auto& line : lines)
                {
                    auto shown = item;
                    shown.text = std::move(line);
                    complete.push_back(std::move(shown));
                }
                return true;
            };
            if (details)
            {
                if (!add(Item{}, model.Help(helpRow))) return false;
            }
            else if (page == static_cast<size_t>(-1))
            {
                const auto& pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                    if (!add(Item{ Kind::Page, i, {}, true }, pages[i].title)) return false;
            }
            else
            {
                if (page >= model.Pages().size()) return false;
                auto rows = model.VisibleRows(page);
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    if (!rows[i]) return false;
                    auto row = *rows[i];
                    auto label = model.Label(row);
                    if (row.kind == RWGTAMenu::RowKind::Preset && row.plugin)
                        label = OtherPlugins::Name(*row.plugin) + ": " + label;
                    auto value = model.Value(row);
                    if (!value.empty()) label += ": " + value;
                    if (!add(Item{ Kind::Row, i, row, model.Enabled(row, paused) }, label)) return false;
                }
            }
            if (complete.size() > 768) return false;
            const size_t capacity = details ? 5 : 4;
            count = (std::max)(size_t(1), (complete.size() + capacity - 1) / capacity);
            physical = (std::min)(physical, count - 1);
            auto first = physical * capacity;
            auto last = (std::min)(first + capacity, complete.size());
            std::vector<Item> shown;
            for (size_t i = first; i < last; ++i) shown.push_back(std::move(complete[i]));
            auto navigation = [&](Kind kind, const char* key)
            {
                std::wstring text;
                if (!Text::Convert(model.Text(key), text) || !Text::Fits(text)) return false;
                shown.push_back(Item{ kind, 0, {}, true, std::move(text) });
                return true;
            };
            if (physical && !navigation(Kind::Previous, "menu.previous")) return false;
            if (physical + 1 < count && !navigation(Kind::Next, "menu.next")) return false;
            if (!details && page != static_cast<size_t>(-1) && !navigation(Kind::Details, "menu.details")) return false;
            if (!navigation(Kind::Back, "menu.back") || shown.size() > 8) return false;
            items = std::move(shown);
            selected = (std::min)(selected, items.size() - 1);
            if (!items[selected].enabled) Move(1);
            Remember();
            return true;
        }

        void Remember()
        {
            if (selected < items.size() && items[selected].kind == Kind::Row)
                helpRow = items[selected].row;
        }

        void Move(int direction)
        {
            if (items.empty()) return;
            for (size_t attempt = 0; attempt < items.size(); ++attempt)
            {
                selected = (selected + items.size() + direction) % items.size();
                if (items[selected].enabled) break;
            }
            Remember();
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (details)
            {
                details = false;
                physical = 0;
                selected = 0;
            }
            else if (page != static_cast<size_t>(-1))
            {
                model.PageShown(page);
                *this = {};
            }
            else return false;
            return true;
        }

        bool Activate(RWGTAMenu::Model& model, bool paused, int direction)
        {
            if (selected >= items.size() || !items[selected].enabled) return true;
            auto item = items[selected];
            if (direction && item.kind != Kind::Row) return true;
            switch (item.kind)
            {
                case Kind::Page:
                    page = item.index;
                    physical = selected = 0;
                    helpRow = {};
                    model.PageShown(page);
                    break;
                case Kind::Row:
                    model.Change(item.row, direction, paused);
                    helpRow = item.row;
                    break;
                case Kind::Previous:
                    if (physical) --physical;
                    selected = 0;
                    break;
                case Kind::Next:
                    if (physical + 1 < count) ++physical;
                    selected = 0;
                    break;
                case Kind::Details:
                    details = true;
                    physical = selected = 0;
                    break;
                case Kind::Back:
                    if (!Back(model)) return false;
                    break;
                default:
                    break;
            }
            return true;
        }
    };
}
