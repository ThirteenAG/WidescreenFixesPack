#pragma once
#include "NativeMenuRegistration.hpp"
#include "NativeMenuView.hpp"
#include "FixSettings.hpp"
#include <deque>
#include <memory>

namespace HD2Menu
{
    inline std::string MenuLanguage()
    {
        const auto& selected = FixSettings().String("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        // Automatic detection falls back to English.
        return "en";
    }

    inline std::string Caption()
    {
        auto translated = RWGTAMenu::PluginText(FixSettings(), MenuLanguage(), "menu.title");
        std::string encoded;
        return TextProbe::Convert(translated, encoded) ? encoded : std::string{};
    }

    struct Host;
    struct Binding
    {
        std::weak_ptr<Host> host;
        size_t index = 0;
        uint64_t serial = 0;
    };

    struct Pending
    {
        std::weak_ptr<Host> host;
        uint64_t generation = 0;
        uint64_t serial = 0;
        void* widget = nullptr;
        size_t index = 0;
        size_t page = 0;
        size_t physical = 0;
        int direction = 0;
        bool escape = false;
    };

    struct Host : std::enable_shared_from_this<Host>
    {
        Host(Context context, uintptr_t graphics, void* driver) : context(context), graphics(graphics),
            driver(driver), bank(graphics, driver), entry(graphics, driver) {}
        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;

        Context context;
        uintptr_t graphics = 0;
        void* driver = nullptr;
        FrameLease sourceAsset;
        FrameLease sourceGroup;
        void* sector = nullptr;
        FrameTree bank;
        Entry entry;
        Window window;
        Geometry geometry;
        RWGTAMenu::Model model;
        View view;
        std::array<Binding, 6> bindings{};
        Binding category;
        std::deque<Pending> pending;
        uint64_t generation = 1;
        uint64_t serial = 1;
        uint32_t index = 0;
        bool open = false;
        bool invalid = false;
        bool busy = false;
        bool queueCleared = false;
        bool building = false;
        bool refresh = false;
        bool recover = false;
        const char* gate = "source context/identity";
        const char* notice = nullptr;

        ~Host()
        {
            // Host storage is retired only after the complete owning-manager
            // teardown and all native frame/input/draw scopes have unwound.
            // Normal process termination does not destroy pinned State storage.
            if (queueCleared) Cleanup();
        }

        bool WindowIdentity() const
        {
            if (!window.Adopted()) return true;
            void** first = nullptr;
            void** last = nullptr;
            if (!window.Get() || !Context::Read(context.manager, 324, first)
                || !Context::Read(context.manager, 328, last)) return false;
            auto begin = reinterpret_cast<uintptr_t>(first);
            auto end = reinterpret_cast<uintptr_t>(last);
            if (!begin || end < begin || (end - begin) % 4 || end - begin > 256
                || index >= (end - begin) / 4 || !Contracts::Readable(first, end - begin)
                || first[index] != window.Get() || !Context::Table(window.Get(), 0x815540)) return false;
            void** children = nullptr;
            size_t count = 0;
            if (!Context::Children(window.Get(), children, count) || count != bindings.size()) return false;
            for (size_t i = 0; i < count; ++i)
            {
                uint32_t id = 0;
                if (children[i] != window.Cell(i) || !Context::Table(children[i], 0x814CB0)
                    || !Context::Read(children[i], 96, id) || id != window.ID(i)) return false;
            }
            return true;
        }
        bool Alive() const { return !invalid && context.Alive() && WindowIdentity(); }
        bool Private() const { return Alive() && open && window.Get() && Context::Top() == window.Get(); }

        bool ManagerAlive() const
        {
            void* world = nullptr;
            void* manager = nullptr;
            return Context::Read(reinterpret_cast<void*>(ABI::game), 0x4AED14, world) && world == context.world
                && Context::Read(world, 20, manager) && manager == context.manager
                && Context::Table(manager, context.pause ? 0x815750 : 0x815E30);
        }

        bool Registered(void* widget) const
        {
            if (!ManagerAlive() || !widget || !Context::Table(widget, 0x815540)) return false;
            uint32_t id = 0;
            void* scene = nullptr;
            void* managerScene = nullptr;
            void** first = nullptr;
            void** last = nullptr;
            if (!Context::Read(widget, 96, id) || (id & 0x03FFFFFF) || !Context::Read(widget, 104, scene)
                || !Context::Read(context.manager, 48, managerScene) || scene != managerScene
                || !Context::Read(context.manager, 324, first) || !Context::Read(context.manager, 328, last)) return false;
            auto begin = reinterpret_cast<uintptr_t>(first);
            auto end = reinterpret_cast<uintptr_t>(last);
            auto slot = id >> 26;
            return begin && end >= begin && (end - begin) % 4 == 0 && end - begin <= 256
                && slot < (end - begin) / 4 && Contracts::Readable(first, end - begin) && first[slot] == widget;
        }

        using GeometryStamp = std::array<uintptr_t, 42>;
        bool Stamp(GeometryStamp& stamp, bool dimensions) const
        {
            for (size_t i = 0; i < context.buttons.size(); ++i)
            {
                void* text = nullptr;
                uint32_t resolution = 0;
                uint32_t width = 0, height = 0;
                uintptr_t font = 0, properties = 0;
                if (!Context::Read(context.buttons[i], 108, text) || !Context::Read(text, 4, resolution)
                    || !Context::Read(text, 36, font) || !Context::Read(text, 44, properties)
                    || !Context::Read(text, 24, width) || !Context::Read(text, 28, height)) return false;
                stamp[i * 7] = reinterpret_cast<uintptr_t>(context.buttons[i]);
                stamp[i * 7 + 1] = reinterpret_cast<uintptr_t>(text);
                stamp[i * 7 + 2] = resolution;
                stamp[i * 7 + 3] = font;
                stamp[i * 7 + 4] = properties;
                stamp[i * 7 + 5] = dimensions ? width : 0;
                stamp[i * 7 + 6] = dimensions ? height : 0;
            }
            return true;
        }

        struct Snapshot
        {
            std::shared_ptr<Host> host;
            uint64_t generation;
            uint64_t serial;
            bool open;
            void* top;
            GeometryStamp geometry{};
            bool dimensions = true;
            bool measured = false;
            bool Current() const
            {
                GeometryStamp actual{};
                return host && measured && host->Alive() && host->generation == generation && host->serial == serial
                    && host->open == open && Context::Top() == top && host->Stamp(actual, dimensions) && actual == geometry;
            }
        };

        Snapshot Capture()
        {
            Snapshot result{ shared_from_this(), generation, serial, open, Context::Top() };
            // The construction transaction itself repositions known stock rows.
            // No Model foreign reads run there; retain font/screen identities
            // while allowing its explicitly owned rectangle updates.
            result.dimensions = !building;
            result.measured = Stamp(result.geometry, result.dimensions);
            return result;
        }

        static bool Selected(void* parent, void* widget)
        {
            int32_t selected = -1;
            void** children = nullptr;
            size_t count = 0;
            return Context::Read(parent, 108, selected) && selected >= 0
                && Context::Children(parent, children, count) && static_cast<size_t>(selected) < count
                && children[selected] == widget;
        }

        static void Focus(void* parent, uint32_t id)
        {
            ABI::Virtual<uint32_t(__thiscall*)(void*, uint32_t)>(parent, 48)(parent, id);
        }

        static void Request(void* manager, uint32_t event, uint32_t value = 0)
        {
            ABI::Message message{};
            message.event = event;
            message.value = value;
            ABI::Function<uint32_t(__thiscall*)(void*, ABI::Message*)>(0x66C950)(manager, &message);
        }

        static uint8_t __fastcall Accept(void* owner, void*, uintptr_t first, void* widget,
            uint32_t event, uint32_t)
        {
            auto binding = static_cast<Binding*>(owner);
            if (!binding || (first && first != reinterpret_cast<uintptr_t>(owner)) || event != 0x04001000) return 0;
            auto host = binding->host.lock();
            if (!host || !host->Alive()) return 0;
            if (binding == &host->category)
            {
                if (host->open || Context::Top() != host->context.options || widget != host->entry.Get()
                    || !Selected(host->context.group, widget) || host->pending.size() >= 32) return 0;
                try { host->pending.push_back({ host, host->generation, host->serial, widget,
                    static_cast<size_t>(-1), host->view.page, host->view.physical, 0 }); }
                catch (...) { return 0; }
                return 1;
            }
            return host->Queue(*binding, widget, 0) ? 1 : 0;
        }

        bool Queue(const Binding& binding, void* widget, int direction)
        {
            if (!Private() || binding.serial != serial || binding.index == 0 || binding.index >= bindings.size()
                || widget != window.Cell(binding.index) || !Selected(window.Get(), widget)
                || binding.index - 1 >= view.items.size() || pending.size() >= 32) return false;
            const auto& item = view.items[binding.index - 1];
            if (!item.enabled || (direction && item.kind != View::Kind::Row)) return false;
            try { pending.push_back({ shared_from_this(), generation, serial, widget, binding.index,
                view.page, view.physical, direction }); }
            catch (...) { return false; }
            // A synchronous foreign setting callback can send Back. Abort the
            // remaining preset burst now; native page restoration stays queued.
            if (busy && !direction && item.kind == View::Kind::Back)
            {
                model.Close();
                ++generation;
                pending.back().generation = generation;
            }
            return true;
        }

        bool QueueEscape()
        {
            if (!Private() || pending.size() >= 32) return false;
            int32_t selected = -1;
            if (!Context::Read(window.Get(), 108, selected) || selected < 1
                || static_cast<size_t>(selected) >= bindings.size()
                || static_cast<size_t>(selected - 1) >= view.items.size()
                || !Selected(window.Get(), window.Cell(selected))) return false;
            try
            {
                pending.push_back({ shared_from_this(), generation, serial, window.Cell(selected),
                    static_cast<size_t>(selected), view.page, view.physical, 0, true });
            }
            catch (...) { return false; }
            if (busy)
            {
                model.Close();
                ++generation;
                pending.back().generation = generation;
            }
            return true;
        }

        bool Prepare()
        {
            auto retained = shared_from_this();
            if (!Alive() || Context::Top() != context.options || sector || window.Get()) return false;
            building = true;
            struct End { bool& value; ~End() { value = false; } } end{ building };
            auto snapshot = Capture();
            auto current = [&] { return snapshot.Current(); };
            auto caption = Caption();
            if (caption.empty() || !current()) return false;
            gate = "source frame parents";
            void* groupFrame = nullptr;
            void* parent = nullptr;
            void* scene = nullptr;
            if (!Context::Read(context.group, 80, groupFrame) || !Context::Read(context.asset, 264, parent)
                || !Context::Read(context.manager, 48, scene) || !parent || !scene) return false;
            gate = "source asset frame lease";
            if (!sourceAsset.Acquire(context.asset, graphics, 0x1009BA50) || !current()) return false;
            gate = "source group frame lease";
            if (!sourceGroup.Acquire(groupFrame, graphics, 0x1009BBA8) || !current()) return false;
            gate = "source seventh-row geometry";
            if (!geometry.Capture(context, current)) return false;
            gate = "private sector factory";
            sector = ABI::CreateFrame(driver, 9);
            if (!sector || !current()) return false;
            // Copy only the proved common frame portion, not Sector216, which
            // would copy sector-owned registrations. Keep source transforms.
            auto copyBase = reinterpret_cast<int32_t(__stdcall*)(void*, void*)>(graphics + 0x1E150);
            gate = "private sector base copy";
            if (copyBase(sector, context.asset) < 0 || !current()) return false;
            ABI::NameFrame(sector, "FusionFixHubAdvanced");
            gate = "private sector parent link";
            if (!current() || ABI::LinkFrame(sector, parent) < 0 || !current()) return false;
            ABI::EnableFrame(sector, false);
            gate = "private group frame clone";
            auto cloned = bank.Clone(groupFrame);
            if (!cloned || !current()) return false;
            gate = "private group parent link";
            if (ABI::LinkFrame(cloned, sector) < 0 || !current()) return false;
            gate = "private group sector registration";
            if (!bank.Register(sector) || !current()) return false;
            std::vector<CellSpec> specifications;
            specifications.reserve(6);
            auto self = shared_from_this();
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                bindings[i] = { self, i, serial };
                void* frame = nullptr;
                const char* frameName = nullptr;
                uint16_t nameOffset = 0;
                if (!Context::Read(context.buttons[i], 80, frame) || !Context::Read(frame, 232, frameName)
                    || !Context::Read(frame, 230, nameOffset)
                    || !frameName || !Contracts::Readable(frameName, 64)) return false;
                size_t length = 0;
                while (length < 63 && frameName[length]) ++length;
                if (!length || length == 63 || nameOffset >= length) return false;
                // Native FindFrame compares the local name after WORD230's
                // asset prefix, while the stored name includes that prefix.
                specifications.push_back({ std::string(frameName + nameOffset, length - nameOffset), &bindings[i],
                    reinterpret_cast<ABI::Action>(&Accept), 20, 13 });
            }
            void** first = nullptr;
            void** last = nullptr;
            if (!Context::Read(context.manager, 324, first) || !Context::Read(context.manager, 328, last)
                || !first || last < first || last - first >= 64) return false;
            index = static_cast<uint32_t>(last - first);
            gate = "private native window/controls";
            if (!window.Build(scene, sector, index, specifications, current))
            { gate = window.Failure(); return false; }
            for (size_t i = 0; i < bindings.size(); ++i)
            {
                gate = "private native initial text/style";
                if (!window.Text(i, i ? "" : caption, false, current)) return false;
                gate = "private native initial text bounds";
                if (!Geometry::Bounds(window.Cell(i)) || !current()) return false;
            }
            TextProbe probe;
            gate = "private native glyph/rectangle fit";
            if (!probe.Capture(window.Cell(0), current) || !probe.Fits(caption) || !current()) return false;
            // Adopt only after all private factories and actual text geometry
            // have succeeded. Native manager teardown owns this window later.
            gate = "private native window registry";
            if (!AppendWindow(context, window, index, current)) return false;
            category = { self, static_cast<size_t>(-1), serial };
            gate = "seventh native category/actual layout rollback";
            if (!entry.Build(context, geometry, &category, reinterpret_cast<ABI::Action>(&Accept), caption, current))
            { gate = entry.Failure(); return false; }
            return current();
        }

        bool Populate(View candidate, size_t preferred = 0)
        {
            auto retained = shared_from_this();
            if (!Alive() || !open || !model.IsOpen()) return false;
            bool outerBusy = busy;
            busy = true;
            struct Restore { bool& value; bool before; ~Restore() { value = before; } } restore{ busy, outerBusy };
            auto snapshot = Capture();
            auto current = [&] { return snapshot.Current(); };
            TextProbe probe;
            if (!probe.Capture(window.Cell(1), current)) return false;
            TextProbe second;
            if (!second.Capture(window.Cell(2), current)) return false;
            if (!candidate.Prepare(model, context.pause, current, TextProbe::Convert,
                [&](const std::string& value) { return probe.Fits(value) && second.Fits(value); }) || !current()) return false;
            auto title = model.Text("menu.title");
            if (!current()) return false;
            std::string encodedTitle;
            if (!TextProbe::Convert(title, encodedTitle) || !probe.Fits(encodedTitle) || !current()) return false;
            // Invalidate old queued row identities before mutating any native
            // caption. On any failure the host closes; old mapping is not kept.
            ++serial;
            snapshot.serial = serial;
            for (auto& binding : bindings) binding.serial = serial;
            if (!window.Text(0, encodedTitle, false, current)) return false;
            for (size_t i = 1; i < bindings.size(); ++i)
            {
                const auto* item = i - 1 < candidate.items.size() ? &candidate.items[i - 1] : nullptr;
                if (!window.Text(i, item ? item->text : "", item && item->enabled, current)
                    || !Geometry::Bounds(window.Cell(i)) || !current()) return false;
                if (item)
                {
                    TextProbe actual;
                    if (!actual.Capture(window.Cell(i), current) || !actual.Fits(item->text) || !current()) return false;
                }
            }
            view = std::move(candidate);
            auto selected = (std::min)(preferred, view.items.empty() ? size_t(0) : view.items.size() - 1);
            for (size_t i = 0; i < view.items.size(); ++i)
            {
                auto row = (selected + i) % view.items.size();
                if (view.items[row].enabled)
                {
                    Focus(window.Get(), window.ID(row + 1));
                    return current();
                }
            }
            return false;
        }

        void Close(bool pop)
        {
            auto retained = shared_from_this();
            auto wasPrivate = Private();
            open = false;
            ++generation;
            ++serial;
            model.Close();
            pending.clear();
            if (pop && wasPrivate && Alive() && Context::Top() == window.Get())
                Request(context.manager, 0x02000005);
        }

        bool Open(HMODULE module)
        {
            auto retained = shared_from_this();
            if (!Alive() || open || Context::Top() != context.options || !window.Get()
                || !Selected(context.group, entry.Get())) return false;
            auto snapshot = Capture();
            model.OpenPlugins(module, MenuLanguage(), 5);
            if (!snapshot.Current() || !Selected(context.group, entry.Get()))
            { model.Close(); notice = "private discovery/source focus changed; open cancelled"; return false; }
            open = true;
            ++generation;
            view = {};
            if (!Populate(view) || !Alive() || Context::Top() != context.options || !Selected(context.group, entry.Get()))
            { Close(false); notice = "private row/glyph/layout or source focus refused; stock Options preserved"; return false; }
            auto pushed = Capture();
            Request(context.manager, 0x02000002, index);
            if (!Alive() || generation != pushed.generation || !open || Context::Top() != window.Get())
            { Close(Context::Top() == window.Get()); notice = "private native stack commit refused; recovery requested"; return false; }
            int32_t selected = -1;
            if (!Context::Read(window.Get(), 108, selected) || selected < 1
                || static_cast<size_t>(selected) > view.items.size())
            { Close(true); notice = "private native selected row refused; stock Options restored"; return false; }
            return true;
        }

        bool Activate(const Pending& action)
        {
            auto retained = shared_from_this();
            if (!Private() || action.generation != generation || action.serial != serial
                || action.page != view.page || action.physical != view.physical
                || action.index == 0 || action.index - 1 >= view.items.size()
                || action.widget != window.Cell(action.index) || !Selected(window.Get(), action.widget)) return false;
            if (action.escape && !model.IsOpen()) { Close(true); return true; }
            auto item = action.escape ? View::Item{ View::Kind::Back } : view.items[action.index - 1];
            if (!item.enabled || (action.direction && item.kind != View::Kind::Row)) return false;
            auto snapshot = Capture();
            View candidate = view;
            if (item.kind == View::Kind::Row)
            {
                bool outerBusy = busy;
                busy = true;
                struct Restore { bool& value; bool before; ~Restore() { value = before; } } restore{ busy, outerBusy };
                model.Change(item.row, action.direction, context.pause);
                if (!snapshot.Current()) return false;
            }
            else if (item.kind == View::Kind::Page)
            {
                candidate.page = item.index;
                candidate.physical = 0;
                model.PageShown(item.index);
                if (!snapshot.Current()) return false;
            }
            else if (item.kind == View::Kind::Previous) --candidate.physical;
            else if (item.kind == View::Kind::Next) ++candidate.physical;
            else if (candidate.page != static_cast<size_t>(-1))
            {
                model.PageShown(candidate.page);
                if (!snapshot.Current()) return false;
                candidate = {};
            }
            else { Close(true); return true; }
            size_t selected = item.kind == View::Kind::Row ? action.index - 1 : 0;
            if (!Populate(std::move(candidate), selected))
            { Close(true); notice = "private value/glyph/row layout refused; stock Options restored"; return false; }
            return true;
        }

        void Frame(HMODULE module)
        {
            auto retained = shared_from_this();
            if (!Alive()) { Retire(); return; }
            // A native modal covers the private pane without removing it from
            // the stack. Suspend actions until it returns, preserving stock
            // modal input/quit logic and the existing private Model session.
            if (open && !Context::Stacked(window.Get())) Close(false);
            refresh |= RWGTAMenu::Model::Changed().exchange(false);
            auto count = pending.size();
            while (count-- && !pending.empty() && Alive())
            {
                auto action = std::move(pending.front());
                pending.pop_front();
                if (action.index == static_cast<size_t>(-1))
                {
                    if (action.generation == generation && action.serial == serial && !open
                        && action.widget == entry.Get() && Context::Top() == context.options
                        && Selected(context.group, action.widget)) Open(module);
                }
                else if (!Activate(action) && open && !model.IsOpen()) Close(true);
            }
            if (refresh && Private())
            {
                refresh = false;
                int32_t selected = -1;
                Context::Read(window.Get(), 108, selected);
                if (!Populate(view, selected > 0 ? static_cast<size_t>(selected - 1) : 0))
                { Close(true); notice = "private live refresh/layout refused; stock Options restored"; }
            }
        }

        void Retire()
        {
            if (invalid) return;
            recover = ManagerAlive() && window.Get() && Context::Stacked(window.Get());
            invalid = true;
            open = false;
            ++generation;
            ++serial;
            model.Close();
            pending.clear();
            // The integration hook first restores a still-live source entry,
            // then retires input ownership. Native object and Binding storage
            // are reclaimed only after manager66B140 clears its message queue.
        }

        void Recover()
        {
            auto retained = shared_from_this();
            // This runs only at a complete outer frame boundary. A modal stays
            // authoritative until it leaves; then pop exactly our own pane.
            if (recover && Registered(window.Get()) && Context::Top() == window.Get())
            {
                if (context.Alive() && WindowIdentity())
                {
                    recover = false;
                    Request(context.manager, 0x02000005);
                }
                else
                {
                    std::vector<void*> stack;
                    if (!Context::Stack(stack) || stack.size() < 2 || stack.front() != window.Get()) return;
                    size_t remove = 1;
                    // Do not call Show/Hide on the destroyed source Options.
                    // Pop only exact stored aliases via native pointer-deque
                    // bookkeeping66E610, whose body never dereferences widgets.
                    while (!context.Alive() && remove < stack.size() && stack[remove] == context.options) ++remove;
                    if (remove == stack.size() || !Registered(stack[remove])) return;
                    auto destination = stack[remove];
                    ABI::EnableFrame(sector, false);
                    if (!ManagerAlive() || Context::Top() != window.Get()) return;
                    for (size_t i = 0; i < remove; ++i)
                    {
                        if (!ManagerAlive() || Context::Top() != stack[i]) return;
                        ABI::Function<uint32_t(__thiscall*)(void*)>(0x66E610)(reinterpret_cast<void*>(ABI::game + 0x4AE4B4));
                    }
                    if (!Registered(destination) || Context::Top() != destination) return;
                    Write(context.manager, 248, uint32_t(0));
                    ABI::Virtual<void*(__thiscall*)(void*, uint32_t)>(destination, 132)(destination, 0);
                    recover = false;
                }
            }
            if (context.Alive() && Context::Top() == context.options)
                entry.Remove(context, geometry);
        }

        void AfterManagerDestroyed()
        {
            Retire();
            queueCleared = true;
            if (window.Adopted()) window.NativeDestroyed();
            entry.NativeDestroyed();
        }

        void Cleanup()
        {
            if (!queueCleared) return;
            // Unadopted construction failures were not visited by66B140. Their
            // own native destructors do not read the now-invalid scene pointer.
            window.Reset();
            geometry.Restore(context);
            entry.ResetFrames();
            bank.Reset();
            if (auto value = std::exchange(sector, nullptr))
            {
                ABI::LinkFrame(value, nullptr);
                ABI::ReleaseFrame(value);
            }
            sourceGroup.Reset();
            sourceAsset.Reset();
        }
    };
}
