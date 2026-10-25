#pragma once

#include "NativeMenuView.hpp"
#include "gameref.hpp"
#include <deque>
#include <memory>

namespace KongMenu
{
    struct Host : std::enable_shared_from_this<Host>
    {
        Context context;
        HMODULE module = nullptr;
        RWGTAMenu::Model model;
        View view;
        uint64_t generation = 1, serial = 1;
        bool pause = true, invalid = false, open = false;
        bool busy = false, opening = false, ready = false, entryReady = false, restore = false, repaint = true;
        size_t selected = 0;

        struct Pending
        {
            uint64_t generation, serial;
            int page;
            size_t physical, selected;
            Item item;
            int direction;
        };
        std::deque<Pending> pending;

        struct Foreign
        {
            Host& host;
            bool previous;
            explicit Foreign(Host& owner) : host(owner), previous(owner.busy) { host.busy = true; }
            ~Foreign() { host.busy = previous; }
        };

        template<class T> static void Write(void* object, size_t offset, T value)
        {
            ProtectedGameRef<T>{ reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) } = value;
        }

        bool Current() const { return !invalid && context.Current(); }
        float Width() const { return std::min(context.style.x, 1.0f - context.style.x) - 0.025f; }

        void Retire()
        {
            if (invalid) return;
            invalid = true;
            ++generation;
            ready = entryReady = open = false;
            pending.clear();
            // Outstanding model reads survive Close, and presets abort
            // immediately. No native resource or UI is accessed here.
            model.Close();
        }

        void Cancel() noexcept
        {
            ++generation;
            ready = false;
            restore = !invalid;
            pending.clear();
            model.Close();
        }

        std::function<bool()> Snapshot(bool privatePage)
        {
            auto lease = shared_from_this();
            auto version = generation;
            auto rows = serial;
            const auto focus = Native::Read<int>(context.parameters, 60);
            return [lease, version, rows, privatePage, focus]
            {
                return lease->generation == version && lease->serial == rows &&
                    lease->Current() && (!privatePage || lease->open) &&
                    (!privatePage || Native::Read<int>(lease->context.parameters, 60) == focus);
            };
        }

        bool Paint(const View& candidate, size_t focus, const std::function<bool()>& current)
        {
            if (!current() || candidate.items.empty() || candidate.items.size() > 5 ||
                Width() <= 0 || focus >= candidate.items.size()) return false;
            Text text(context, current);
            // Clear only this Options page's verified thirty-slot range. The
            // cursor is a separate native object managed by the original VM.
            for (int i = 0; i < 29; ++i)
                if (!text.Hide(context.first + i)) return false;
            if (!text.Put(context.first, candidate.title, context.style.x, 0.25f,
                          Width(), 0.065f, 0xFFFFFFFF)) return false;
            for (size_t i = 0; i < candidate.items.size(); ++i)
            {
                const auto color = !candidate.items[i].enabled ? 0xFF606060 :
                    (i == focus ? 0xFFFFFFFF : 0xFFB0B0B0);
                if (!text.Put(context.first + 5 + int(i), candidate.items[i].text,
                              context.style.x, 0.40f + 0.08f * float(i),
                              Width(), 0.065f, color)) return false;
            }
            return current();
        }

        void Close()
        {
            auto lease = shared_from_this();
            const bool owned = Current();
            ++generation;
            ++serial;
            open = ready = false;
            pending.clear();
            model.Close();
            restore = !owned && context.Alive();
            view = {};
            selected = 0;
            if (owned && Current())
            {
                // Native Options will recreate every stock label on its next
                // call and preserve its normal title/pause return dispatch.
                Write<uint32_t>(context.parameters, 480, 1);
                Write<int>(context.parameters, 60, 4);
                repaint = true;
            }
        }

        bool Populate(View candidate, size_t focus)
        {
            auto lease = shared_from_this();
            if (!Current() || !open) return false;
            ready = false;
            auto current = Snapshot(true);
            Text text(context, current);
            {
                Foreign foreign(*this);
                if (!candidate.Prepare(model, text, pause, current, Width(), module) || !current()) return false;
            }
            focus = std::min(focus, candidate.items.size() - 1);
            if (!Paint(candidate, focus, current) || !current()) return false;
            // View mappings and widget contents publish together. Advancing
            // the serial discards all input copied from the previous rows.
            view = std::move(candidate);
            selected = focus;
            Write<int>(context.parameters, 60, int(selected));
            ++serial;
            pending.clear();
            ready = true;
            repaint = false;
            return true;
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (!Current() || open || !entryReady ||
                Native::Read<int>(context.parameters, 60) != 4) return false;
            ++generation;
            open = true;
            ready = false;
            struct Construction
            {
                Host& host;
                bool previous;
                explicit Construction(Host& owner) : host(owner), previous(owner.opening) { host.opening = true; }
                ~Construction() { host.opening = previous; }
            } construction(*this);
            view = {};
            selected = 0;
            auto current = Snapshot(true);
            {
                Foreign foreign(*this);
                model.OpenPlugins(module, language, 5);
            }
            if (!current() || Native::Read<int>(context.parameters, 60) != 4 ||
                !Populate(view, 0))
            {
                if (!invalid) Close();
                return false;
            }
            return true;
        }

        bool Queue(int direction, bool back = false)
        {
            auto lease = shared_from_this();
            if (!Current() || !open || (!ready && !busy) || pending.size() >= 32) return false;
            if (opening && busy && back && direction == 0)
            {
                if (Native::Read<int>(context.parameters, 60) != 4) return false;
                Cancel();
                return true;
            }
            if (Native::Read<int>(context.parameters, 60) != int(selected)) return false;
            if (!back && direction == 0 && selected < view.items.size() &&
                view.items[selected].kind == Kind::Back) back = true;
            if (back && busy)
            {
                // An accepted native Back cancels a foreign preset/read burst
                // immediately; restoring native labels waits for full unwind.
                ++generation;
                ready = false;
                restore = true;
                pending.clear();
                model.Close();
                return true;
            }
            if (!ready || selected >= view.items.size()) return false;
            auto item = view.items[selected];
            if (back) item = Item{ Kind::Back, {}, {}, 0, true };
            pending.push_back(Pending{ generation, serial, view.page, view.physical,
                                       selected, std::move(item), direction });
            return true;
        }

        bool Activate(const Pending& event)
        {
            auto lease = shared_from_this();
            if (!Current() || !open || !ready || event.generation != generation ||
                event.serial != serial || event.page != view.page ||
                event.physical != view.physical || event.selected != selected) return false;
            auto current = Snapshot(true);
            View candidate = view;
            switch (event.item.kind)
            {
                case Kind::Link:
                    if (event.direction) return true;
                    if (event.item.target >= model.Pages().size()) return false;
                    candidate.page = int(event.item.target);
                    candidate.physical = 0;
                    model.PageShown(event.item.target);
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
                    if (candidate.page < 0) { Close(); return true; }
                    model.PageShown(size_t(candidate.page));
                    candidate.page = -1;
                    candidate.physical = 0;
                    break;
                case Kind::Row:
                {
                    Foreign foreign(*this);
                    if (!model.Enabled(event.item.row, pause) || !current()) return current();
                    model.Change(event.item.row, event.direction, pause);
                    if (!current()) return false;
                    break;
                }
            }
            const bool samePage = candidate.page == view.page && candidate.physical == view.physical;
            return current() && Populate(std::move(candidate), samePage ? selected : 0);
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (invalid) return;
            if (!context.Alive()) { Retire(); return; }
            if (!Current())
            {
                if (open) Close();
                entryReady = false;
                return;
            }
            if (restore) { Close(); return; }
            if (open)
            {
                while (!pending.empty() && Current() && open)
                {
                    auto event = std::move(pending.front());
                    pending.pop_front();
                    if (!Activate(event))
                    {
                        if (open && Current()) Close();
                        return;
                    }
                }
                if (Current() && open && (repaint || RWGTAMenu::Model::Changed().exchange(false)))
                    if (!Populate(view, selected) && Current()) Close();
            }
        }
    };
}
