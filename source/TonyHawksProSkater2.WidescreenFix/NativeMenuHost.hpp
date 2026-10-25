#pragma once

#include "NativeMenuPages.hpp"
#include "NativeMenuPauseFont.hpp"
#include "NativeMenuDiagnostics.hpp"
#include "FixSettings.hpp"
#include <deque>

namespace THPS2Menu
{
    enum class Backend { Title, Pause };

    inline std::string MenuLanguage()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        // Auto uses English until a native locale getter is established.
        return language.empty() || language == "auto" ? "en" : language;
    }

    // The native hooks supply an exact owner predicate. This game-local
    // state machine uses the native hooks' exact owner and frame predicates.
    // C++ retention does not retain the native owner: deleting hooks must
    // invalidate immediately and defer native deletion through its full frame.
    struct Host : std::enable_shared_from_this<Host>
    {
        Backend backend;
        void* owner = nullptr;
        HMODULE module = nullptr;
        uint64_t generation = 1;
        bool valid = true;
        bool open = false;
        bool entering = false;
        bool busy = false;
        bool closeRequested = false;
        int helpSelected = -1;
        std::function<bool(const Host&)> nativeCurrent;
        std::function<bool(Host&)> enterNative;
        std::function<void(Host&)> leaveNative;
        Native::TextProperties* titleStyle = nullptr;
        Native::Font* pauseSource = nullptr;
        RWGTAMenu::Model model;
        View view;
        std::unique_ptr<TextList> title;
        std::unique_ptr<PauseList> pause;
        PauseFont pauseFont;

        explicit Host(Backend kind) : backend(kind) {}
        Host(const Host&) = delete;
        Host& operator=(const Host&) = delete;
        Host(Host&&) = delete;
        Host& operator=(Host&&) = delete;

        struct Snapshot
        {
            std::shared_ptr<Host> host;
            uint64_t generation;
            bool open;
            explicit Snapshot(const std::shared_ptr<Host>& value)
                : host(value), generation(value->generation), open(value->open) {}
            bool Current() const
            {
                return host->valid && host->generation == generation && host->open == open && host->nativeCurrent && host->nativeCurrent(*host);
            }
        };

        struct Busy
        {
            Host& host;
            bool previous;
            explicit Busy(Host& value) : host(value), previous(value.busy) { host.busy = true; }
            ~Busy() { host.busy = previous; }
        };

        int Selected() const
        {
            return backend == Backend::Title ? (title ? title->SelectedCommand() : -1) : (pause ? pause->SelectedCommand() : -1);
        }

        const void* Widget() const
        {
            return backend == Backend::Title ? static_cast<const void*>(title ? title->Get() : nullptr) : static_cast<const void*>(pause ? pause->Get() : nullptr);
        }

        void Retire()
        {
            // Native storage reclamation is deliberately separate. Close now
            // aborts a shared-model preset/read burst on owner destruction.
            valid = false;
            ++generation;
            model.Close();
            closeRequested = true;
        }

        bool Populate(int selected = -1)
        {
            auto retained = shared_from_this();
            ++generation; // Any old physical-row input is now obsolete.
            Snapshot snapshot(retained);
            if (!snapshot.Current() || !open || !model.IsOpen())
                return false;
            Busy scope(*this);
            auto nextView = view;
            if (!nextView.Prepare(model, backend == Backend::Pause, [&snapshot] { return snapshot.Current(); }))
                return false;
            if (selected < 0 && !nextView.starts.empty())
            {
                auto start = nextView.starts[nextView.physical];
                for (size_t i = start; i < std::min(nextView.text.size(), start + View::ContentLines); ++i)
                    if (nextView.text[i].enabled) { selected = nextView.text[i].command; break; }
            }
            auto lines = nextView.Lines(model, selected);
            if (!snapshot.Current())
                return false;
            if (backend == Backend::Title)
            {
                if (!titleStyle || !Contracts::Readable(titleStyle, sizeof(*titleStyle)))
                {
                    Diagnose(Diagnostic::TitleStyle);
                    return false;
                }
                auto next = std::make_unique<TextList>();
                if (!next->Build(lines, *titleStyle))
                {
                    Diagnose(Diagnostic::TitleWidget);
                    return false;
                }
                if (!snapshot.Current())
                    return false;
                next->Select(selected);
                title = std::move(next);
            }
            else
            {
                if (!pauseSource || !Contracts::Readable(pauseSource, sizeof(*pauseSource)))
                {
                    Diagnose(Diagnostic::PauseStyle);
                    return false;
                }
                PauseFont nextFont;
                if (!nextFont.Prepare(*pauseSource, lines))
                {
                    Diagnose(Diagnostic::PauseLayout);
                    return false;
                }
                auto next = std::make_unique<PauseList>();
                if (!next->Build(lines))
                {
                    Diagnose(Diagnostic::PauseWidget);
                    return false;
                }
                // 44B6C0 centers baselines in240 pixels. Width is centered512;
                // measured ascent/descent fit each16-pixel private row.
                const auto menu = next->Get();
                if (menu->x != 256 || menu->y < 16 || menu->rowSpacing != 16 || menu->y + 16 * (menu->count - 1) > 224 || !snapshot.Current())
                {
                    Diagnose(Diagnostic::PauseLayout);
                    return false;
                }
                next->Select(selected);
                pauseFont = nextFont;
                pause = std::move(next);
            }
            if (!snapshot.Current())
                return false;
            view = std::move(nextView);
            helpSelected = Selected();
            return snapshot.Current();
        }

        bool Refresh(int selected = -1)
        {
            if (Populate(selected))
                return true;
            // Activate may have navigated before population. Never leave old
            // visible commands bound to changed logical page/item state.
            Close();
            Diagnose(Diagnostic::Rollback, "private population refused; stock menu restored");
            return false;
        }

        bool Open()
        {
            auto retained = shared_from_this();
            Snapshot before(retained);
            if (busy || open || !before.Current())
                return false;
            Busy scope(*this);
            // Shared9c snapshots survive Close while discovery is in flight.
            model.OpenPlugins(module, MenuLanguage(), 4096);
            if (!before.Current() || !model.IsOpen())
            {
                model.Close();
                Diagnose(Diagnostic::Discovery);
                return false;
            }
            view.Reset();
            open = true;
            entering = true;
            ++generation;
            if (!Populate() || !valid || !enterNative || !enterNative(*this))
            {
                Close();
                Diagnose(Diagnostic::Rollback);
                return false;
            }
            entering = false;
            ++generation;
            Snapshot committed(retained);
            if (!committed.Current())
            {
                Close();
                Diagnose(Diagnostic::Rollback);
                return false;
            }
            return true;
        }

        void Close()
        {
            auto retained = shared_from_this();
            Busy scope(*this);
            ++generation;
            model.Close();
            if (open && valid && leaveNative)
                leaveNative(*this);
            open = false;
            entering = false;
            closeRequested = false;
            // Called only at complete modal iteration/world-frame unwind.
            title.reset();
            pause.reset();
            pauseFont.Clear();
            view.Reset();
        }

        bool Activate(int command, int direction)
        {
            auto retained = shared_from_this();
            Snapshot snapshot(retained);
            if (busy || !open || !snapshot.Current() || (command != View::PhysicalBack && Selected() != command))
                return false;
            Busy scope(*this);
            if (command == View::PhysicalBack)
            {
                if (direction)
                    return false;
                if (view.page == View::Root)
                {
                    Close();
                    return true;
                }
                model.PageShown(view.page);
                view.page = View::Root;
                view.physical = 0;
            }
            else if (command == View::PhysicalPrevious || command == View::PhysicalNext)
            {
                if (direction)
                    return false;
                if (command == View::PhysicalPrevious && view.physical)
                    --view.physical;
                else if (command == View::PhysicalNext && view.physical + 1 < view.starts.size())
                    ++view.physical;
                else
                    return false;
            }
            else
            {
                if (command < 0 || static_cast<size_t>(command) >= view.items.size())
                    return false;
                const auto item = view.items[command];
                if (!item.enabled)
                    return false;
                if (item.kind == ItemKind::Row)
                {
                    model.Change(item.row, direction, backend == Backend::Pause);
                    if (!snapshot.Current())
                        return false;
                    return Refresh(command);
                }
                if (direction)
                    return false;
                if (item.kind == ItemKind::Page)
                {
                    view.page = item.page;
                    model.PageShown(item.page);
                }
                else if (item.kind == ItemKind::Previous && view.rootStart >= View::PageLinks)
                    view.rootStart -= View::PageLinks;
                else if (item.kind == ItemKind::Next)
                    view.rootStart += View::PageLinks;
                else
                    return false;
                view.physical = 0;
            }
            return snapshot.Current() && Refresh();
        }
    };

    struct Pending
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        const void* widget;
        size_t page;
        size_t rootStart;
        size_t physical;
        int command;
        int selected;
        int direction;
        bool opening;

        bool Current() const
        {
            return Host::Snapshot(host).Current() && host->generation == generation && host->Widget() == widget && host->view.page == page && host->view.rootStart == rootStart && host->view.physical == physical && (opening ? !host->open : host->open && host->Selected() == selected);
        }
    };

    inline std::deque<Pending> pending;
    inline bool draining = false;

    inline bool Queue(const std::shared_ptr<Host>& host, int command, int direction, bool opening = false)
    {
        Host::Snapshot snapshot(host);
        if (!snapshot.Current() || pending.size() >= 32 || (opening ? host->open || host->busy : !host->open || (command != View::PhysicalBack && host->Selected() != command)))
            return false;
        if (!opening && command == View::PhysicalBack && host->busy && direction == 0)
        {
            // Abort foreign Set/preset bursts immediately; rebuild/stock return
            // stays deferred until the full native outer iteration unwinds.
            host->model.Close();
            ++host->generation;
            host->closeRequested = true;
            return true;
        }
        pending.push_back({ host, host->generation, host->Widget(), host->view.page, host->view.rootStart, host->view.physical, command, host->Selected(), direction, opening });
        return true;
    }

    inline void Drain(Backend backend)
    {
        if (draining)
            return;
        struct Scope { Scope() { draining = true; } ~Scope() { draining = false; } } scope;
        auto work = std::move(pending);
        pending.clear();
        for (auto& action : work)
        {
            if (action.host->backend != backend)
            {
                pending.push_back(std::move(action));
                continue;
            }
            if (!action.Current())
                continue;
            if (action.opening)
                action.host->Open();
            else
                action.host->Activate(action.command, action.direction);
        }
    }
}
