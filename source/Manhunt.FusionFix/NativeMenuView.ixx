module;

#include <stdafx.h>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/MenuList.hpp"

export module NativeMenuView;

import NativeMenuABI;
import NativeMenuText;

export namespace MHMenu
{
    // The plugins list, a plugin's settings or categories, or a category's settings, as the rows of the Settings screen. A row too long for the screen continues on the next lines. Every list is one
    // list that scrolls by a line through the rows the screen shows; the footer's Back returns one level.
    struct View
    {
        static constexpr size_t Slots = 8;  // lines shown at once

        enum class Kind { Row, Text };
        struct Item
        {
            Kind kind = Kind::Text;
            const RWGTAMenu::Row* row = nullptr;
            bool enabled = false;
            std::wstring text;
        };

        size_t page = RWGTAMenu::Model::RootPage;
        std::string notice;
        std::vector<Item> all;              // every line of the list
        std::vector<Item> items;            // the lines shown
        MenuList::Window* list = nullptr;   // the page's window (the model keeps one per page)
        MenuList::Window text;              // the notice

        MenuList::Window& Current() { return !notice.empty() || !list ? text : *list; }

        bool Build(RWGTAMenu::Model& model, bool paused)
        {
            if (!model.IsOpen() || page >= model.Pages().size()) return false;
            list = &model.Window(page);
            std::vector<Item> complete;
            std::vector<MenuList::Key> keys;
            auto add = [&](Item item, const std::string& value)
            {
                std::vector<std::wstring> lines;
                if (!Text::Lines(value, lines)) return false;
                for (size_t i = 0; i < lines.size(); ++i)
                {
                    auto shown = item;
                    shown.text = std::move(lines[i]);
                    // Every line of a row selects that row; the lines after the first carry it along
                    keys.push_back(MenuList::Key(item.row ? reinterpret_cast<uintptr_t>(item.row) : complete.size() + 1) * 8 + i);
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
                    auto label = model.Label(*row);
                    if (!RWGTAMenu::Model::Opens(*row))
                    {
                        auto value = model.Value(*row);
                        if (!value.empty()) label += ": " + value;
                    }
                    bool enabled = RWGTAMenu::Model::Opens(*row) || model.Enabled(*row, paused);
                    if (!add(Item{ Kind::Row, row, enabled }, label)) return false;
                }
            }
            if (complete.size() > 768) return false;
            auto& current = Current();
            current.SetSlots(Slots);
            current.Update(keys);
            items.assign(complete.begin() + current.First(), complete.begin() + current.First() + current.Shown());
            all = std::move(complete);
            if (!items.empty() && !items[current.Slot()].enabled) Move(1);
            return true;
        }

        size_t Selected() { return Current().Slot(); }

        // The mouse over a shown line (while it moves): its row
        void Point(size_t slot)
        {
            auto& current = Current();
            if (current.First() + slot < all.size() && all[current.First() + slot].enabled)
                current.Point(slot);
        }

        // Page Up/Page Down, Home/End and the wheel
        void Page(int direction) { Current().Page(direction); }
        void Edge(int direction) { Current().Edge(direction); }
        void Scroll(int direction) { Current().Scroll(direction); }

        // Up/Down: the next row that can be changed, the list following it; wrapping at the ends
        void Move(int direction)
        {
            auto& current = Current();
            if (all.empty()) return;
            for (size_t attempt = 0; attempt < all.size(); ++attempt)
            {
                current.Move(direction, true);
                if (all[current.Selected()].enabled) break;
            }
        }

        bool Back(RWGTAMenu::Model& model)
        {
            if (!notice.empty()) return false;
            size_t parent = model.Back(page);
            if (parent == SIZE_MAX) return false;
            page = parent;
            list = &model.Window(page);
            return true;
        }

        bool Activate(RWGTAMenu::Model& model, bool paused, int direction)
        {
            auto& current = Current();
            if (current.Selected() >= all.size() || !all[current.Selected()].enabled) return true;
            auto item = all[current.Selected()];
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
                            page = next;
                            list = &model.Window(page);
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
