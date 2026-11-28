module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"
#include <map>

export module NativeMenuView;
import NativeMenuText;


export namespace OnimushaMenu
{
    // A page of the menu as the game's Options rows: every list (plugins, categories, settings) is one list that
    // scrolls by a row through the rows the screen shows, Details and Back last; a help text scrolls by a line
    struct View
    {
        enum class Kind { Page, Row, Details, Back, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            const RWGTAMenu::Row* row = nullptr;
            bool enabled = false;
            size_t head = 0;            // the first line of the row this line belongs to
            std::wstring text, value;
        };

        size_t Lines = 8;               // the rows the screen shows

        size_t page = RWGTAMenu::Model::RootPage;
        size_t first = 0, selected = 0;
        size_t detailsFirst = 0, detailsSelected = 0, noticeFirst = 0, noticeSelected = 0;
        std::map<size_t, std::pair<size_t, size_t>> left;     // the line each page was left from, by page
        bool details = false;
        std::string notice;
        const RWGTAMenu::Row* helpRow = nullptr;
        std::vector<Item> all, items;

        bool Build(RWGTAMenu::Model& model, bool paused)
        {
            if (!model.IsOpen() || page >= model.Pages().size()) return false;
            std::vector<Item> complete;
            bool settings = false;
            auto add = [&](Item item, const std::string& value)
            {
                std::vector<std::wstring> lines;
                if (!Text::Lines(value, lines, item.kind == Kind::Text ? 30 : 64)) return false;
                item.head = complete.size();
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    auto shown = item;
                    shown.text = std::move(lines[i]);
                    if (i) shown.enabled = false;
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
                if (!helpRow || !add(Item{}, model.Help(*helpRow))) return false;
            }
            else
            {
                for (auto row : model.ListRows(page, false))
                {
                    if (!row) return false;
                    auto label = model.Label(*row);
                    if (RWGTAMenu::Model::Opens(*row))
                    {
                        if (!add(Item{ Kind::Page, row, true }, label)) return false;
                        continue;
                    }
                    Item item{ Kind::Row, row, model.Enabled(*row, paused) };
                    if (!Text::Convert(model.Value(*row), item.value)) return false;
                    if (!add(std::move(item), label)) return false;
                    settings = true;
                }
                if (settings && !add(Item{ Kind::Details, nullptr, true }, model.Text("menu.details"))) return false;
            }
            if (!add(Item{ Kind::Back, nullptr, true }, model.Text("menu.back"))) return false;
            if (complete.size() > 768) return false;
            all = std::move(complete);
            selected = (std::min)(selected, all.size() - 1);
            selected = all[selected].head;
            const bool text = details || !notice.empty();
            if (text) selected = all.size() - 1;
            else if (!all[selected].enabled) Move(1);
            // A help text scrolls by Up/Down (Scroll); Back stays selected below it
            if (text) first = (std::min)(first, all.size() > Lines ? all.size() - Lines : size_t(0));
            else Follow();
            items.assign(all.begin() + first, all.begin() + (std::min)(all.size(), first + Lines));
            Remember();
            return true;
        }

        // The window shows the selected row whole, scrolled by as little as needed
        void Follow()
        {
            size_t last = selected;
            while (last + 1 < all.size() && all[last + 1].head == selected && !all[last + 1].enabled) ++last;
            if (selected < first) first = selected;
            if (last >= first + Lines) first = last + 1 - Lines;
            first = (std::min)(first, all.size() > Lines ? all.size() - Lines : size_t(0));
        }

        bool Above() const { return first > 0; }
        bool Below() const { return first + Lines < all.size(); }

        // The selected line in the shown lines, or none
        size_t Slot(size_t line) const { return line >= first && line < first + items.size() ? line - first : SIZE_MAX; }

        void Remember()
        {
            if (selected < all.size() && all[selected].kind == Kind::Row)
                helpRow = all[selected].row;
        }

        void Move(int direction)
        {
            if (all.empty()) return;
            if (details || !notice.empty())
            {
                if (direction < 0 && first) --first;
                else if (direction > 0 && Below()) ++first;
                return;
            }
            for (size_t attempt = 0; attempt < all.size(); ++attempt)
            {
                selected = (selected + all.size() + direction) % all.size();
                if (all[selected].enabled) break;
            }
            Follow();
            Remember();
        }

        void ShowNotice(std::string text)
        {
            noticeFirst = first;
            noticeSelected = selected;
            notice = std::move(text);
            first = selected = 0;
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (!notice.empty())
            {
                notice.clear();
                first = noticeFirst;
                selected = noticeSelected;
                return true;
            }
            if (details)
            {
                details = false;
                first = detailsFirst;
                selected = detailsSelected;
                return true;
            }
            const auto parent = model.Back(page);
            if (parent == SIZE_MAX) return false;
            page = parent;
            std::tie(first, selected) = left[page];
            helpRow = nullptr;
            return true;
        }

        bool Activate(RWGTAMenu::Model& model, bool paused, int direction)
        {
            if (selected >= all.size() || !all[selected].enabled) return true;
            auto item = all[selected];
            if (direction && item.kind != Kind::Row) return true;
            switch (item.kind)
            {
                case Kind::Page:
                {
                    const auto target = model.Enter(*item.row);
                    if (target == SIZE_MAX) break;
                    left[page] = { first, selected };
                    page = target;
                    first = selected = 0;
                    helpRow = nullptr;
                    break;
                }
                case Kind::Row:
                    model.Change(*item.row, direction, paused);
                    helpRow = item.row;
                    break;
                case Kind::Details:
                    if (!helpRow) break;
                    detailsFirst = first;
                    detailsSelected = selected;
                    details = true;
                    first = 0;
                    selected = 0;
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
