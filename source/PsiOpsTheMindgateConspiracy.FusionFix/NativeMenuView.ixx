module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"

export module NativeMenuView;
import NativeMenuText;


// The rows of the menu screen: the lines of a list (the plugins, a plugin's page, a category, a help text) in a window
// that scrolls a line at a time, then Details (on a list of settings) and Back. A setting whose text needs two lines
// takes two.
export namespace PsiOpsMenu
{
    struct View
    {
        enum class Kind { Open, Row, Details, Back, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            RWGTAMenu::Row row{};
            bool enabled = false;
            std::wstring text;
        };

        size_t page = RWGTAMenu::Model::RootPage;
        size_t first = 0;           // the first line shown
        size_t lines = 0;           // the lines of the list
        size_t capacity = 1;        // the lines shown at once
        size_t selected = 0;
        bool details = false;
        std::string notice;
        RWGTAMenu::Row helpRow{};
        std::vector<Item> items;

        // The rows of the list in the window
        size_t Shown() const
        {
            return (std::min)(capacity, lines > first ? lines - first : 0);
        }

        bool Above() const { return first > 0; }
        bool Below() const { return first + capacity < lines; }

        bool Build(RWGTAMenu::Model& model, bool paused, size_t maximumRows)
        {
            if (!model.IsOpen() || maximumRows < 5 || maximumRows > 6) return false;
            std::vector<Item> complete;
            auto add = [&](Item item, const std::string& value)
            {
                std::vector<std::wstring> wrapped;
                if (!MenuText::Lines(value, wrapped)) return false;
                for (auto& line : wrapped)
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
            else
            {
                if (page >= model.Pages().size()) return false;
                for (auto source : model.VisibleRows(page))
                {
                    if (!source) return false;
                    auto row = *source;
                    if (RWGTAMenu::Model::Opens(row))
                    {
                        std::wstring title;
                        if (!MenuText::Convert(model.Label(row), title) || !MenuText::Fits(title)) return false;
                        complete.push_back(Item{ Kind::Open, row, true, std::move(title) });
                        continue;
                    }
                    auto label = model.Label(row);
                    auto value = model.Value(row);
                    if (!value.empty()) label += ": " + value;
                    if (row.kind == RWGTAMenu::RowKind::Preset)
                    {
                        std::wstring text;
                        if (!MenuText::Convert(label, text) || !MenuText::Fits(text)) return false;
                        complete.push_back(Item{ Kind::Row, row, model.Enabled(row, paused), std::move(text) });
                    }
                    else if (!add(Item{ Kind::Row, row, model.Enabled(row, paused) }, label)) return false;
                }
            }
            if (complete.size() > 768) return false;
            const bool settings = !details && notice.empty() &&
                std::any_of(complete.begin(), complete.end(), [](const Item& item) { return item.kind == Kind::Row; });
            capacity = (std::max)(size_t(1), maximumRows - (settings ? 2 : 1));
            lines = complete.size();
            first = (std::min)(first, lines > capacity ? lines - capacity : 0);
            std::vector<Item> shown;
            for (size_t i = first; i < first + Shown(); ++i) shown.push_back(std::move(complete[i]));
            auto navigation = [&](Kind kind, const char* key)
            {
                std::wstring text;
                auto label = model.Text(key);
                if (kind == Kind::Back) label += " (ESC)";
                if (!MenuText::Convert(label, text) || !MenuText::Fits(text)) return false;
                shown.push_back(Item{ kind, {}, true, std::move(text) });
                return true;
            };
            if (settings && !navigation(Kind::Details, "menu.details")) return false;
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

        // The game moved the selection from row from to row to: past the first or last line shown the list scrolls
        // instead, and from Back down to the first line or from the first line up to Back it goes to the list's other end.
        // True when the window changed (the caller builds the rows again and selects the row of selected).
        bool Follow(size_t from, size_t to)
        {
            const size_t shown = Shown();
            if (from == to || from >= items.size() || to >= items.size()) return false;
            const bool up = (from == 0 && to == items.size() - 1) || to + 1 == from;
            if (up && from == 0 && Above())
            {
                --first;
                selected = 0;
                return true;
            }
            if (!up && from + 1 == shown && to == shown && Below())
            {
                ++first;
                selected = from;
                return true;
            }
            if (!up && from == items.size() - 1 && to == 0 && Above())
            {
                first = 0;
                selected = 0;
                return true;
            }
            if (up && from == 0 && to == items.size() - 1 && Below())
            {
                first = lines - capacity;
                selected = items.size() - 1;
                return true;
            }
            return false;
        }

        // The wheel: the window moves a line, the selection stays on the lines shown
        bool Scroll(int direction)
        {
            const size_t before = first, shown = Shown();
            if (direction < 0 && first > 0) --first;
            else if (direction > 0 && Below()) ++first;
            if (first == before) return false;
            // The selected line keeps its row while it stays in the window
            if (selected < shown)
                selected = direction > 0 ? (selected ? selected - 1 : 0) : (std::min)(selected + 1, shown - 1);
            return true;
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (!notice.empty()) return false;
            if (details)
            {
                details = false;
                selected = 0;
                return true;
            }
            const auto parent = model.Back(page);
            if (parent == SIZE_MAX) return false;
            const auto& rows = model.VisibleRows(parent);
            const auto found = std::find_if(rows.begin(), rows.end(), [&](const RWGTAMenu::Row* row)
                { return row && row->target == page; });
            const size_t line = found == rows.end() ? 0 : size_t(found - rows.begin());
            page = parent;
            first = line >= 4 ? line - 3 : 0;
            selected = line - first;
            return true;
        }

        bool Activate(RWGTAMenu::Model& model, bool paused, int direction)
        {
            if (selected >= items.size() || !items[selected].enabled) return true;
            auto item = items[selected];
            if (direction && item.kind != Kind::Row) return true;
            switch (item.kind)
            {
                case Kind::Open:
                {
                    const auto target = model.Enter(item.row);
                    if (target == SIZE_MAX) break;
                    page = target;
                    first = selected = 0;
                    helpRow = {};
                    break;
                }
                case Kind::Row:
                    model.Change(item.row, direction, paused);
                    helpRow = item.row;
                    break;
                case Kind::Details:
                    details = true;
                    first = selected = 0;
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
