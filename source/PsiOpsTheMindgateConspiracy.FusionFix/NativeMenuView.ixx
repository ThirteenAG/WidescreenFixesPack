module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"

export module NativeMenuView;
import NativeMenuText;


export namespace PsiOpsMenu
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
        std::string notice;
        RWGTAMenu::Row helpRow{};
        std::vector<Item> items;

        bool Build(RWGTAMenu::Model& model, bool paused, size_t maximumRows)
        {
            if (!model.IsOpen() || maximumRows < 5 || maximumRows > 6) return false;
            std::vector<Item> complete;
            auto add = [&](Item item, const std::string& value)
            {
                std::vector<std::wstring> lines;
                if (!MenuText::Lines(value, lines)) return false;
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
            else if (details)
            {
                if (!add(Item{}, model.Help(helpRow))) return false;
            }
            else if (page == static_cast<size_t>(-1))
            {
                const auto& pages = model.Pages();
                for (size_t i = 0; i < pages.size(); ++i)
                {
                    std::wstring title;
                    if (!MenuText::Convert(pages[i].title, title) || !MenuText::Fits(title)) return false;
                    complete.push_back(Item{ Kind::Page, i, {}, true, std::move(title) });
                }
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
                    auto value = model.Value(row);
                    if (!value.empty()) label += ": " + value;
                    if (row.kind == RWGTAMenu::RowKind::Preset)
                    {
                        std::wstring text;
                        if (!MenuText::Convert(label, text) || !MenuText::Fits(text)) return false;
                        complete.push_back(Item{ Kind::Row, i, row, model.Enabled(row, paused), std::move(text) });
                    }
                    else if (!add(Item{ Kind::Row, i, row, model.Enabled(row, paused) }, label)) return false;
                }
            }
            if (complete.size() > 768) return false;
            const size_t capacity = (std::max)(size_t(1), maximumRows - (details || !notice.empty() ? 3 : 4));
            count = (std::max)(size_t(1), (complete.size() + capacity - 1) / capacity);
            physical = (std::min)(physical, count - 1);
            auto first = physical * capacity;
            auto last = (std::min)(first + capacity, complete.size());
            std::vector<Item> shown;
            for (size_t i = first; i < last; ++i) shown.push_back(std::move(complete[i]));
            auto navigation = [&](Kind kind, const char* key)
            {
                std::wstring text;
                auto label = model.Text(key);
                if (kind == Kind::Back) label += " (ESC)";
                if (!MenuText::Convert(label, text) || !MenuText::Fits(text)) return false;
                shown.push_back(Item{ kind, 0, {}, true, std::move(text) });
                return true;
            };
            if (physical && !navigation(Kind::Previous, "menu.previous")) return false;
            if (physical + 1 < count && !navigation(Kind::Next, "menu.next")) return false;
            if (!details && notice.empty() && page != static_cast<size_t>(-1) && !navigation(Kind::Details, "menu.details")) return false;
            if (!navigation(Kind::Back, "menu.back") || shown.size() > maximumRows) return false;
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
            if (!notice.empty()) return false;
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
