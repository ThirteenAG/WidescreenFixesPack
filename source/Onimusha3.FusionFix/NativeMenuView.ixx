module;
#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"

export module NativeMenuView;
import NativeMenuText;


export namespace OnimushaMenu
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
        size_t rootPhysical = 0, rootSelected = 0;
        size_t detailsPhysical = 0, detailsSelected = 0;
        size_t noticePhysical = 0, noticeSelected = 0;
        size_t count = 1;
        bool details = false;
        std::string notice;
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
                    auto value = model.Value(row);
                    if (!value.empty()) label += ": " + value;
                    if (!add(Item{ Kind::Row, i, row, model.Enabled(row, paused) }, label)) return false;
                    if (!add(Item{ Kind::Details, i, row, true }, model.Text("menu.details"))) return false;
                }
            }
            if (complete.size() > 768) return false;
            const size_t capacity = details || !notice.empty() ? 5 : 4;
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

        void ShowNotice(std::string text)
        {
            noticePhysical = physical;
            noticeSelected = selected;
            notice = std::move(text);
            physical = selected = 0;
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (!notice.empty())
            {
                notice.clear();
                physical = noticePhysical;
                selected = noticeSelected;
                return true;
            }
            if (details)
            {
                details = false;
                physical = detailsPhysical;
                selected = detailsSelected;
            }
            else if (page != static_cast<size_t>(-1))
            {
                model.PageShown(page);
                page = static_cast<size_t>(-1);
                physical = rootPhysical;
                selected = rootSelected;
                helpRow = {};
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
                    rootPhysical = physical;
                    rootSelected = selected;
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
                    helpRow = item.row;
                    detailsPhysical = physical;
                    detailsSelected = selected;
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
