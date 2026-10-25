#pragma once

#include "NativeMenuView.hpp"
#include <memory>

namespace DeerMenu
{
    struct Session : std::enable_shared_from_this<Session>
    {
        struct Pending
        {
            uint64_t generation = 0;
            uint64_t serial = 0;
            int page = -1;
            size_t physical = 0;
            size_t selected = 0;
            int direction = 0;
            bool back = false;
            Item item;
        };

        struct Busy
        {
            bool& flag;
            bool previous;
            explicit Busy(bool& value) : flag(value), previous(value) { flag = true; }
            ~Busy() { flag = previous; }
        };

        HMODULE module = nullptr;
        RWGTAMenu::Model model;
        View view;
        std::function<bool()> owner;
        std::vector<Pending> pending;
        uint64_t generation = 0;
        uint64_t serial = 0;
        size_t selected = 0;
        int width = 0;
        bool pause = true;
        bool open = false;
        bool opening = false;
        bool ready = false;
        bool busy = false;
        bool invalid = false;
        bool draining = false;

        bool Current(uint64_t expected) const
        {
            return !invalid && open && generation == expected && owner && owner();
        }

        void Close() noexcept
        {
            // Logical cancellation is immediate even inside a Get/Set/preset
            // burst; native window/font cleanup belongs to the outer bridge.
            ++generation;
            ++serial;
            open = opening = ready = false;
            pending.clear();
            try { model.Close(); } catch (...) { invalid = true; }
        }

        void Retire() noexcept
        {
            invalid = true;
            Close();
        }

        static bool Same(const Item& a, const Item& b)
        {
            return a.kind == b.kind && a.target == b.target && a.text == b.text &&
                a.row.kind == b.row.kind && a.row.plugin == b.row.plugin && a.row.setting == b.row.setting;
        }

        bool Populate(bool preserveSelection)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || !model.IsOpen())
                return false;
            const bool hadItem = preserveSelection && selected < view.items.size();
            const Item old = hadItem ? view.items[selected] : Item{};
            View candidate = view;
            Busy reading(busy);
            if (!candidate.Prepare(model, pause, [this, expected] { return Current(expected); }, width) ||
                !Current(expected))
                return false;
            size_t next = 0;
            if (hadItem)
            {
                for (size_t i = 0; i < candidate.items.size(); ++i)
                    if (Same(candidate.items[i], old))
                    {
                        next = i;
                        break;
                    }
                if (next == 0 && !Same(candidate.items[0], old))
                    next = std::min(selected, candidate.items.size() - 1);
            }
            // The text/widget bridge publishes this committed snapshot as one
            // unit. Old queued row identities are invalid from this point.
            view = std::move(candidate);
            selected = next;
            ++serial;
            ready = true;
            return true;
        }

        bool ReadHelp(std::string& text)
        {
            const auto expected = generation;
            const auto expectedSerial = serial;
            const auto expectedSelected = selected;
            if (!Current(expected) || !ready || selected >= view.items.size() || busy)
                return false;
            const Item item = view.items[selected];
            Busy reading(busy);
            text = item.kind == Kind::Row ? PlainText(model.Help(item.row)) : std::string{};
            return Current(expected) && serial == expectedSerial && selected == expectedSelected;
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (invalid || !module || !owner || !owner())
                return false;
            Close();
            if (invalid || !owner())
                return false;
            open = opening = true;
            ready = false;
            selected = 0;
            view = {};
            const auto expected = generation;
            Busy discovery(busy);
            model.OpenPlugins(module, language, 4096);
            if (!Current(expected) || !model.IsOpen() || !Populate(false))
            {
                if (generation == expected)
                    Close();
                return false;
            }
            opening = false;
            return Current(expected) && ready;
        }

        bool Queue(int direction, bool physicalBack, size_t actualSelected)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || (!opening && (!ready || actualSelected != selected ||
                selected >= view.items.size())))
                return false;
            // Physical Back is the only accepted construction-phase action.
            if (opening)
            {
                if (!physicalBack || direction)
                    return false;
                Close();
                return true;
            }
            const Item item = view.items[selected];
            const bool back = physicalBack || (direction == 0 && item.kind == Kind::Back);
            if (busy && back && direction == 0)
            {
                Close();
                return true;
            }
            if (pending.size() >= 32 || (physicalBack && direction))
                return false;
            pending.push_back(Pending{expected, serial, view.page, view.physical, selected, direction, back, item});
            return true;
        }

        bool Move(int direction)
        {
            if (!Current(generation) || !ready || busy || view.items.empty())
                return false;
            if (direction < 0 && selected)
                --selected;
            else if (direction > 0 && selected + 1 < view.items.size())
                ++selected;
            return true;
        }

        bool Activate(const Pending& command)
        {
            auto lease = shared_from_this();
            const auto expected = generation;
            if (!Current(expected) || !ready || selected >= view.items.size())
                return false;
            const Item item = command.item;
            if (command.back)
            {
                if (command.direction)
                    return true;
                if (view.page < 0)
                {
                    Close();
                    return true;
                }
                model.PageShown(size_t(view.page));
                if (!Current(expected))
                    return false;
                view.page = -1;
                view.physical = 0;
                return Populate(false);
            }
            if (item.kind != Kind::Row && command.direction)
                return true;
            if (item.kind == Kind::Link)
            {
                if (item.target >= model.Pages().size())
                    return false;
                view.page = int(item.target);
                view.physical = 0;
                model.PageShown(item.target);
                return Current(expected) && Populate(false);
            }
            if (item.kind == Kind::Previous || item.kind == Kind::Next)
            {
                if (item.kind == Kind::Previous && view.physical)
                    --view.physical;
                else if (item.kind == Kind::Next)
                    ++view.physical;
                return Populate(false);
            }
            if (item.kind == Kind::Row && item.enabled)
            {
                Busy changing(busy);
                const auto expectedSerial = serial;
                const auto expectedSelected = selected;
                const auto expectedPage = view.page;
                const auto expectedPhysical = view.physical;
                const bool enabled = model.Enabled(item.row, pause);
                if (!Current(expected) || serial != expectedSerial || selected != expectedSelected ||
                    view.page != expectedPage || view.physical != expectedPhysical)
                    return false;
                if (!enabled)
                    return Populate(true);
                model.Change(item.row, command.direction, pause);
                if (!Current(expected))
                    return false;
                return Populate(true);
            }
            return true;
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (draining)
                return;
            Busy drain(draining);
            if (open && (!owner || !owner()))
                Close();
            if (!Current(generation) || !ready)
                return;
            auto commands = std::move(pending);
            pending.clear();
            for (const auto& command : commands)
            {
                if (!Current(command.generation) || command.serial != serial ||
                    command.page != view.page || command.physical != view.physical ||
                    command.selected != selected || selected >= view.items.size() ||
                    !Same(command.item, view.items[selected]))
                    continue;
                if (!Activate(command))
                {
                    Close();
                    return;
                }
            }
            if (Current(generation) && RWGTAMenu::Model::Changed().exchange(false) && !Populate(true))
                Close();
        }
    };
}
