#pragma once

#include "NativeMenuSurface.hpp"
#include <deque>
#include <memory>

namespace CthulhuMenu
{
    struct Host : std::enable_shared_from_this<Host>
    {
        Context context;
        Entry entry;
        Page page;
        Surface surface;
        View view;
        FontMetrics font;
        HMODULE module = nullptr;
        void* escape = nullptr;
        const char* preparationFailure = nullptr;
        RWGTAMenu::Model model;
        uint64_t generation = 1, serial = 1;
        bool invalid = false, open = false, ready = false, busy = false, opening = false;
        bool expectedRows = false, restore = false, retiring = false;
        size_t selected = 0;
        std::array<std::array<uint32_t, 4>, 7> geometry{};
        std::array<uint32_t, 6> style{};

        struct Foreign
        {
            Host& host;
            bool previous;
            explicit Foreign(Host& owner) : host(owner), previous(owner.busy) { host.busy = true; }
            ~Foreign() { host.busy = previous; }
        };
        struct Flag
        {
            bool& flag;
            bool previous;
            explicit Flag(bool& value) : flag(value), previous(value) { flag = true; }
            ~Flag() { flag = previous; }
        };
        struct Pending
        {
            uint64_t generation, serial;
            int logical;
            size_t physical, selected;
            void* row;
            Item item;
            int direction;
            bool back;
        };
        std::deque<Pending> pending;

        bool CaptureStyle()
        {
            if (!context.Alive())
                return false;
            for (size_t i = 0; i < geometry.size(); ++i)
                for (size_t j = 0; j < 4; ++j)
                    geometry[i][j] = Read<uint32_t>(context.rows[i], 280 + j * 4);
            for (size_t i = 0; i < style.size(); ++i)
                style[i] = Read<uint32_t>(context.rows[0], std::array<size_t, 6>{332, 336, 344, 348, 352, 356}[i]);
            return true;
        }

        bool Alive() const
        {
            if (invalid || !context.Alive())
                return false;
            for (size_t i = 0; i < geometry.size(); ++i)
                for (size_t j = 0; j < 4; ++j)
                    if (geometry[i][j] != Read<uint32_t>(context.rows[i], 280 + j * 4))
                        return false;
            for (size_t i = 0; i < style.size(); ++i)
                if (style[i] != Read<uint32_t>(context.rows[0], std::array<size_t, 6>{332, 336, 344, 348, 352, 356}[i]))
                    return false;
            return true;
        }

        bool Current(bool privatePage) const
        {
            return Alive() && (privatePage ? page.Alive(context) && context.Current(page.object) : context.Current(context.options));
        }

        std::function<bool()> Snapshot(bool privatePage, bool selection = false)
        {
            auto lease = shared_from_this();
            const auto version = generation, rows = serial;
            const auto selectedRow = privatePage ? Read<int>(page.object, 540) : Read<int>(context.options, 540);
            return [lease, version, rows, privatePage, selection, selectedRow]
            {
                return lease->generation == version && lease->serial == rows && lease->Current(privatePage) &&
                    (!selection || Read<int>(privatePage ? lease->page.object : lease->context.options, 540) == selectedRow);
            };
        }

        float Width() const
        {
            float width = 1.0f;
            for (size_t i = 0; i < context.rows.size(); ++i)
                width = std::min(width, Read<float>(context.rows[i], 288) - Read<float>(context.rows[i], 280));
            return width - 0.01f;
        }

        void Cancel() noexcept
        {
            ++generation;
            ready = false;
            restore = true;
            pending.clear();
            // Shared discovery/read/preset snapshots observe Close immediately.
            // Native repaint/pop/deletion always waits for complete outer unwind.
            model.Close();
        }

        void Retire() noexcept
        {
            if (invalid)
                return;
            invalid = retiring = true;
            Cancel();
        }

        bool Prepare(const std::string& title)
        {
            auto lease = shared_from_this();
            preparationFailure = "CoC native menu: Options context changed before preparation";
            if (!context.Capture())
                return false;
            preparationFailure = "CoC native menu: Options style or active-page authority unavailable";
            if (!CaptureStyle() || !Current(false))
                return false;
            escape = FindPage(context.manager, context.Pause() ? "INGAMEPAUSE" : "MAIN");
            preparationFailure = "CoC native menu: return page identity unavailable";
            if (!Readable(escape, Native::PageSize) || Read<uintptr_t>(escape, 0) !=
                (context.Pause() ? 0x62C818u : 0x62CA74u))
                return false;
            FontProbe probe;
            preparationFailure = "CoC native menu: Options source font unavailable";
            if (!probe.Capture(context.rows[0]) || !font.Capture(probe) || !probe.Current())
                return false;
            auto current = Snapshot(false);
            Foreign foreign(*this);
            preparationFailure = "CoC native menu: spare-row caption or font layout refused";
            if (!current() || !probe.Current() || !entry.Build(context, title, font, current))
                return false;
            preparationFailure = "CoC native menu: Options entry changed during publication";
            if (!current() || !entry.Matches(context))
                return false;
            preparationFailure = nullptr;
            return true;
        }

        bool Populate(View candidate, size_t focus, bool privatePage)
        {
            auto lease = shared_from_this();
            if (!open || !Current(privatePage))
                return false;
            ready = false;
            auto current = Snapshot(privatePage, true);
            FontProbe probe;
            auto fontRow = privatePage && surface.Alive() ? surface.rows[1] : context.rows[0];
            if (!probe.Capture(fontRow) || !font.Capture(probe) || !probe.Current())
                return false;
            {
                Foreign foreign(*this);
                if (!candidate.Prepare(model, font, context.Pause(), current, Width()) ||
                    !current() || !probe.Current() || !candidate.Footer(focus, font, Width(), candidate.help))
                    return false;
            }
            // Stop old-row replay before native invalidation. Failure closes the
            // entire private view; old widgets are never paired with new mappings.
            ++serial;
            pending.clear();
            auto continuing = Snapshot(privatePage);
            {
                Flag constructing(expectedRows);
                Foreign foreign(*this);
                surface.Forgotten();
                Native::ClearRows(page.object);
                if (!continuing())
                    return false;
                if (!surface.Build(page.object, context, candidate, probe.vertical, continuing) || !continuing())
                    return false;
                if (privatePage)
                {
                    Native::BuildRenderers(page.object);
                    if (!continuing() || !surface.Rendered(candidate, continuing))
                        return false;
                }
            }
            view = std::move(candidate);
            selected = std::min(focus, view.items.size() - 1);
            // Do not raw-write selection: native focus/secondary renderer state
            // is coupled to it. SelectDown finds the first enabled row (heading0
            // is disabled), then bounded native steps reach the retained index.
            Write<int>(page.object, 540, -1);
            for (size_t i = 0; i <= selected; ++i)
            {
                Native::Select(page.object, 1, 1);
                if (!continuing())
                    return false;
            }
            ready = privatePage;
            return continuing() && surface.Selected(selected);
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (open || !Current(false) || !entry.Matches(context) || !context.SelectedEntry())
                return false;
            ++generation;
            open = true;
            ready = false;
            restore = false;
            Flag construction(opening);
            auto current = Snapshot(false, true);
            {
                Foreign foreign(*this);
                model.OpenPlugins(module, language, 4096);
            }
            if (!current() || !context.SelectedEntry() || !model.IsOpen() || model.Pages().empty())
            {
                Cancel();
                return false;
            }
            if ((!page.object && !page.Create(context, current)) || !current() || !page.Alive(context) ||
                !Populate(View{}, 0, false) || !Current(false) || !context.SelectedEntry())
            {
                Cancel();
                return false;
            }
            {
                Foreign foreign(*this);
                Native::ShowPage(context.manager, Page::Name, 0, 1);
            }
            auto committed = Snapshot(true);
            if (!Current(true) || !surface.Rendered(view, committed) || !surface.Selected(0))
            {
                Cancel();
                return false;
            }
            selected = 0;
            ready = true;
            return true;
        }

        bool Queue(int direction, bool back = false)
        {
            auto lease = shared_from_this();
            if (!open || invalid || pending.size() >= 32)
                return false;
            if (opening && busy && back && !direction)
            {
                if (!Current(false) || !context.SelectedEntry())
                    return false;
                Cancel();
                return true;
            }
            if (!Current(true) || !surface.Alive())
                return false;
            const auto nativeSelected = Read<int>(page.object, 540);
            if (nativeSelected <= 0 || size_t(nativeSelected - 1) >= view.items.size())
                return false;
            selected = size_t(nativeSelected - 1);
            if (!surface.Selected(selected))
                return false;
            if (!direction && !back && view.items[selected].kind == Kind::Back)
                back = true;
            if (back && !direction && busy)
            {
                Cancel();
                return true;
            }
            if (!ready)
                return false;
            pending.push_back(Pending{generation, serial, view.page, view.physical, selected,
                surface.rows[selected + 1], view.items[selected], direction, back});
            return true;
        }

        bool Activate(const Pending& event)
        {
            auto lease = shared_from_this();
            if (!open || !ready || event.generation != generation || event.serial != serial ||
                event.logical != view.page || event.physical != view.physical ||
                !Current(true) || !surface.Selected(event.selected) ||
                surface.rows[event.selected + 1] != event.row)
                return true; // stale events are discarded, never retargeted
            selected = event.selected;
            auto current = Snapshot(true, true);
            auto candidate = view;
            const auto item = event.back ? Item{Kind::Back, {}, {}, 0, true} : event.item;
            switch (item.kind)
            {
                case Kind::Link:
                    if (event.direction) return true;
                    if (item.target >= model.Pages().size()) return false;
                    candidate.page = int(item.target);
                    candidate.physical = 0;
                    model.PageShown(item.target);
                    break;
                case Kind::Previous:
                    if (event.direction || !candidate.physical) return true;
                    --candidate.physical;
                    break;
                case Kind::Next:
                    if (event.direction) return true;
                    ++candidate.physical;
                    break;
                case Kind::Back:
                    if (event.direction) return true;
                    if (candidate.page < 0) { Cancel(); return true; }
                    model.PageShown(size_t(candidate.page));
                    candidate.page = -1;
                    candidate.physical = 0;
                    break;
                case Kind::Row:
                {
                    Foreign foreign(*this);
                    const auto enabled = model.Enabled(item.row, context.Pause());
                    if (!current()) return false;
                    if (!enabled) return true;
                    model.Change(item.row, event.direction, context.Pause());
                    if (!current()) return false;
                    break;
                }
            }
            const bool samePage = candidate.page == view.page && candidate.physical == view.physical;
            return current() && Populate(std::move(candidate), samePage ? selected : 0, true);
        }

        bool Restore()
        {
            auto lease = shared_from_this();
            model.Close();
            ready = open = false;
            pending.clear();
            if (!Alive())
                return false;
            if (Current(true))
            {
                Foreign foreign(*this);
                Native::ShowPage(context.manager, "OPTIONS", 0, 0);
            }
            if (!Current(false))
                return false; // temporary modal: retain pending restoration
            restore = false;
            view = {};
            ++serial;
            return entry.Matches(context);
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (invalid)
                return;
            if (!Alive()) { Retire(); return; }
            if (restore)
            {
                Restore();
                return;
            }
            if (!open)
                return;
            if (!Current(true))
            {
                // A temporary dialog loses input authority without discarding
                // borrowed page restoration or touching native hidden widgets.
                Cancel();
                return;
            }
            while (!pending.empty() && Current(true) && open)
            {
                auto event = std::move(pending.front());
                pending.pop_front();
                if (!Activate(event))
                {
                    Cancel();
                    return;
                }
            }
            if (open && Current(true) && RWGTAMenu::Model::Changed().exchange(false))
            {
                const auto index = Read<int>(page.object, 540);
                if (index <= 0 || size_t(index - 1) >= view.items.size() ||
                    !Populate(view, size_t(index - 1), true))
                    Cancel();
            }
            if (open && ready && Current(true))
            {
                const auto index = Read<int>(page.object, 540);
                std::string description;
                if (index > 0 && size_t(index - 1) < view.items.size() &&
                    view.Footer(size_t(index - 1), font, Width(), description) && description != view.help)
                {
                    auto current = Snapshot(true, true);
                    Foreign foreign(*this);
                    if (!surface.Help(description, current)) { Cancel(); return; }
                    if (current()) view.help = std::move(description);
                }
            }
        }

        void NativePageRetiring(void* object) noexcept
        {
            if (object == escape)
            {
                escape = nullptr;
                Retire();
            }
            if (object == context.options)
            {
                Retire();
                entry.Forgotten();
            }
            if (object == page.object)
            {
                Retire();
                surface.Forgotten();
                page.Forgotten();
            }
        }

        void NativeRowRetiring(void* object) noexcept
        {
            if (object == entry.row)
            {
                Retire();
                entry.Forgotten();
            }
            if (!expectedRows)
                for (auto row : surface.rows)
                    if (object == row)
                    {
                        Retire();
                        surface.Forgotten();
                        break;
                    }
        }

        bool ManagerAlive() const
        {
            // Independent of the source Options owner: only used for deferred
            // escape/cleanup after that source's destructor has invalidated it.
            void* ignored = nullptr;
            return Readable(context.manager, 249) && Read<uintptr_t>(context.manager, 0) == 0x62BD68 &&
                FindPageNode(context.manager, Page::Name, ignored);
        }

        bool OwnsNativePage() const
        {
            if (!ManagerAlive() || !page.object || !Readable(page.object, Native::PageSize) ||
                Read<uintptr_t>(page.object, 0) != 0x62CAEC ||
                FindPage(context.manager, Page::Name) != page.object ||
                Read<void*>(context.manager, 48) != page.object || Read<void*>(context.manager, 44) ||
                Read<void*>(context.manager, 52) || Read<uint8_t>(context.manager, 12) ||
                !Read<uint8_t>(context.manager, 33))
                return false;
            const auto count = Read<uint32_t>(reinterpret_cast<void*>(0x6C8890), 0);
            return count > 0 && count <= 10 &&
                Read<void*>(reinterpret_cast<void*>(0x6C8840), (count - 1) * 8) == page.object &&
                !Read<uint8_t>(reinterpret_cast<void*>(0x6C8844), (count - 1) * 8);
        }

        bool Cleanup(const std::function<bool()>& authorized)
        {
            auto lease = shared_from_this();
            if (!authorized())
                return false;
            model.Close();
            ready = open = false;
            pending.clear();
            if (OwnsNativePage())
            {
                const auto sourceExists = entry.SameSource(context);
                const auto name = sourceExists ? "OPTIONS" : context.Pause() ? "INGAMEPAUSE" : "MAIN";
                const auto destination = sourceExists ? context.options : escape;
                if (!destination || FindPage(context.manager, name) != destination ||
                    !Readable(destination, Native::PageSize) || !authorized())
                    return false;
                Native::ShowPage(context.manager, name, 0, 0);
                if (!authorized() || !ManagerAlive() || OwnsNativePage())
                    return false;
            }
            // A modal may retain our page beneath it. Never erase/free a page
            // still referenced by current slots or native history.
            if (!page.Remove(context.manager, authorized))
                return false;
            surface.Forgotten();
            if (!entry.Restore(context, authorized))
                return false;
            entry.ReleaseOriginal(authorized);
            return authorized() && !page.object && !entry.row && !entry.original;
        }

        void NativeManagerRetiring()
        {
            Retire();
            entry.Forgotten();
            // A successfully published page is owned by the native map's full
            // deletion loop. Forget it before that loop, never delete it twice.
            if (page.object && FindPage(context.manager, Page::Name) == page.object)
            {
                page.Forgotten();
                surface.Forgotten();
            }
        }
    };
}
