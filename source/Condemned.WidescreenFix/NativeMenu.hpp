#pragma once
#include "NativeMenuLife.hpp"
#include "FixSettings.hpp"
#include <safetyhook.hpp>

namespace CondemnedMenuNative
{
    enum Hook : size_t { Update, PostUpdate, Render, KeyDown, KeyUp, Character, MouseDown, MouseUp, CommandOn, CommandOff,
        RemoveAll, ScreenTerm, ScreenDtor, ScreenScalar, ManagerTerm, ManagerScalar, ManagerDtor,
        InterfaceTerm, InterfaceScalar, InterfaceDtor, ShellDtor, ShellTerm, TextScalar, HookCount };
    struct Host
    {
        Bindings api;
        Traversals scopes;
        Lifecycle life;
        std::shared_ptr<Owner> source;
        std::array<SafetyHookInline, HookCount> hooks;
        HMODULE ownModule{};
        uint64_t nextOwner{};
        uint32_t diagnostics{}, written{};
        bool ready{}, acceptDown{}, repeatAccept{};
    };
    // Native vtables retain our callbacks. Supported bridge pins both modules;
    // bookkeeping deliberately survives CRT/static engine teardown. Dynamic
    // plugin unloading is unsupported. Ordinary native Back does not leak pools.
    inline Host* host{};
    inline constexpr uint32_t entryCommand = 0x57465001, rowCommand = 0x57465002;
    enum Diagnostic : uint32_t { Unsupported = 1, SourceRefused = 2, EntryPublished = 4, ViewRefused = 8,
        PagePublished = 16, OwnerRetired = 32, HookRefused = 64, Exception = 128 };
    inline void FlushLog()
    {
        if (!host) return;
        const auto fresh = host->diagnostics & ~host->written;
        if (!fresh) return;
        for (auto item : { std::pair<uint32_t, const char*>{ Unsupported, "unsupported executable" },
            { SourceRefused, "Options menu not recognised" }, { ViewRefused, "page could not be shown" },
            { OwnerRetired, "menu owner retired" }, { HookRefused, "hooks could not be installed" },
            { Exception, "menu operation failed" } })
        {
            if (fresh & item.first)
                OutputDebugStringA((std::string("Condemned native menu: ") + item.second + "\n").c_str());
        }
        host->written |= fresh;
    }
    inline std::string Language()
    {
        // Auto uses English until the native locale accessor is established.
        const auto& selected = FixSettings().String("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        return "en";
    }
    inline bool Admit(void* receiver, CommandKind kind)
    {
        if (!host || !host->ready || !host->scopes.Here()) return false;
        const auto owner = host->source;
        if (!owner || owner->ready.view.screen != receiver || !owner->Current()) return false;
        const auto selected = owner->ready.view.Selected();
        const auto row = owner->ready.view.SelectedRow();
        if (!selected || !row) return false;
        if (kind == CommandKind::Open)
        {
            if (owner->logical.settings.IsOpen() || owner->Private() || *selected != owner->entryIndex || row != owner->entry) return false;
        }
        else
        {
            const bool discoveryBack = kind == CommandKind::Back && owner->logical.settings.InForeignCall() &&
                owner->logical.settings.IsOpen() && owner->HasArray(owner->entryArray) &&
                *selected == owner->entryIndex && row == owner->entry;
            if (!discoveryBack && (!owner->logical.settings.IsOpen() || !owner->SelectedItem())) return false;
        }
        if (kind == CommandKind::Accept)
        {
            const auto item = owner->SelectedItem();
            if (item && owner->active->presented.items[*item].kind == ItemKind::Back) kind = CommandKind::Back;
        }
        const Request request{kind, owner->logical.identity, owner->logical.pool, *selected, reinterpret_cast<uintptr_t>(row)};
        return owner->logical.Queue(request, true);
    }
    inline int __fastcall SourceCommand(void* self, void*, uint32_t command, uint32_t parameter1, uint32_t parameter2)
    {
        const auto owner = host ? host->source : nullptr;
        auto scope = host ? host->scopes.Enter(Phase::Input) : Traversals::Scope{};
        try
        {
            if (owner && owner->ready.view.screen == self)
            {
                if (command == entryCommand && parameter2 == static_cast<uint32_t>(owner->logical.identity))
                    return Admit(self, CommandKind::Open) ? 1 : 0;
                if (command == rowCommand && parameter2 == static_cast<uint32_t>(owner->logical.pool) &&
                    owner->SelectedItem() == parameter1)
                    return host->repeatAccept ? 1 : (Admit(self, CommandKind::Accept) ? 1 : 0);
            }
        }
        catch (...) { if (host) host->diagnostics |= Exception; return 0; }
        return host->api.At<int (__thiscall*)(void*, uint32_t, uint32_t, uint32_t)>(0xCD470)(self, command, parameter1, parameter2);
    }
    inline char __fastcall SourceKey(void* self, void*, uint32_t key, uint32_t repeat)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try
        {
            if (key == VK_RETURN && host->source && host->source->ready.view.screen == self)
            {
                if (host->repeatAccept) return 1;
                const auto kind = host->source->logical.settings.IsOpen() ? CommandKind::Accept : CommandKind::Open;
                if (Admit(self, kind)) return 1;
            }
        }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*, uint32_t, uint32_t)>(0x7B20)(self, key, repeat);
    }
    inline int __fastcall SourceEscape(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Back)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<int (__thiscall*)(void*)>(0x5E00)(self);
    }
    inline char __fastcall SourceLeft(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Left)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*)>(0x7CE0)(self);
    }
    inline char __fastcall SourceRight(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        try { if (Admit(self, CommandKind::Right)) return 1; }
        catch (...) { host->diagnostics |= Exception; return 0; }
        return host->api.At<char (__thiscall*)(void*)>(0x7D40)(self);
    }
    inline int __fastcall SourceFocused(void* self, void*, char focused)
    {
        auto scope = host->scopes.Enter(Phase::Transition);
        const auto owner = host->source;
        if (!focused && host->ready && host->scopes.Here() && owner && owner->ready.view.screen == self)
        {
            // SetCurrent calls this before changing current screen. Close the
            // logical owner now so a foreign preset burst aborts subsequent Sets.
            owner->logical.settings.Cancel();
            owner->logical.returnPending = true;
            owner->logical.pending.Clear();
            ++owner->logical.pool;
        }
        return host->api.At<int (__thiscall*)(void*, char)>(0xCD520)(self, focused);
    }
    inline bool Build(const std::shared_ptr<Owner>& owner, Location location)
    {
        const auto identity = owner->logical.identity;
        const auto before = Array::Get(owner->ready.view.screen);
        if (!before) return false;
        auto current = [&]() { return host->source == owner && owner->logical.identity == identity &&
            owner->Current() && owner->HasArray(*before); };
        if (!current()) return false;
        // Clear before foreign reads: notifications arriving during preparation
        // remain set and trigger another safely scoped update afterward.
        owner->logical.settings.BeginRefresh();
        auto prepared = owner->logical.settings.Prepare(location, current);
        if (!prepared || !current()) return false;
        owner->staging = std::make_unique<Pool>();
        if (!PreparePool(host->api, owner->ready, *owner->staging, *prepared,
            owner->logical.settings.Text("menu.unavailable"), rowCommand,
            static_cast<uint32_t>(owner->logical.pool + 1), current) || !current())
        { owner->logical.settings.Reject(*prepared); return false; }
        if (!owner->PublishPool(host->api)) { owner->logical.settings.Reject(*prepared); return false; }
        host->diagnostics |= PagePublished;
        return host->source == owner && owner->logical.identity == identity && owner->Current() && owner->Private();
    }
    inline void Close(const std::shared_ptr<Owner>& owner)
    {
        if (owner->logical.settings.IsOpen()) owner->logical.settings.Cancel();
        owner->logical.pending.Clear();
        if (owner->Registered() && (Read<uint8_t>(owner->ready.view.interfaceManager, 10500).value_or(0) ||
            Read<void*>(owner->ready.view.screen, 216).value_or(nullptr)))
        { owner->logical.returnPending = true; return; }
        owner->logical.returnPending = false;
        ++owner->logical.pool;
        owner->ReturnToSource(host->api);
        host->life.CleanOwned([&]()
        {
            if (!owner->DestroyPools(host->api)) owner->logical.returnPending = true;
        });
    }
    inline void Act(const std::shared_ptr<Owner>& owner, Request request)
    {
        if (host->source != owner || !owner->Current() || request.owner != owner->logical.identity ||
            request.pool != owner->logical.pool || request.selected != owner->ready.view.Selected() ||
            request.sender != reinterpret_cast<uintptr_t>(owner->ready.view.SelectedRow())) return;
        auto current = [&]() { return host->source == owner && owner->Current() &&
            request.pool == owner->logical.pool && request.sender == reinterpret_cast<uintptr_t>(owner->ready.view.SelectedRow()) &&
            (request.kind == CommandKind::Open ? owner->HasArray(owner->entryArray) : owner->SelectedItem().has_value()); };
        if (request.kind == CommandKind::Open)
        {
            if (!owner->logical.settings.Open(host->ownModule, Language(), owner->ready.view.PauseContext(), current) || !current()) return;
            if (!Build(owner, {})) { host->diagnostics |= ViewRefused; Close(owner); }
            return;
        }
        const auto selected = owner->SelectedItem();
        if (!selected || !owner->active) return;
        const auto displayed = owner->active->presented;
        const auto item = displayed.items[*selected];
        auto location = displayed.location;
        if (request.kind == CommandKind::Back || (request.kind == CommandKind::Accept && item.kind == ItemKind::Back))
        {
            if (location.page) location = {};
            else { Close(owner); return; }
        }
        else if (item.kind == ItemKind::ModelRow)
        {
            const int direction = request.kind == CommandKind::Accept ? 0 : request.kind == CommandKind::Left ? -1 : 1;
            owner->logical.settings.Change(displayed, *selected, direction, current);
            if (!current() || !owner->logical.settings.IsOpen()) return;
        }
        else
        {
            if (request.kind != CommandKind::Accept) return;
            if (item.kind == ItemKind::Link) location = {item.destination, 0};
            else if (item.kind == ItemKind::Previous || item.kind == ItemKind::Next) location.first = item.destination;
            else return;
        }
        if (!Build(owner, location)) { host->diagnostics |= ViewRefused; Close(owner); }
    }
    inline void Capture()
    {
        const auto view = View::Get(reinterpret_cast<HMODULE>(host->api.Base()));
        if (!view || !view->ScreenMode() || view->Covered()) return;
        const auto array = Array::Get(view->screen);
        if (!array) return;
        auto owner = std::make_shared<Owner>();
        for (auto at = array->begin; at < array->end; ++at)
        {
            auto ready = Ready::Capture(host->api.calls, *view, *at);
            if (ready) { owner->ready = *ready; break; }
        }
        if (!owner->ready.sourceRow) { host->diagnostics |= SourceRefused; return; }
        auto current = [&]() { const auto now = View::Get(reinterpret_cast<HMODULE>(view->module));
            return !owner->retired && now && now->Same(*view) && now->ScreenMode() && !now->Covered(); };
        host->source = owner; // retain construction before any native callback
        if (!host->api.controls.Bind(host->api.calls, *view, owner->ready.sourceRow) ||
            !owner->ready.Probe(host->api.calls, current) ||
            !owner->PrepareEntry(host->api, ++host->nextOwner, entryCommand, rowCommand,
                RWGTAMenu::PluginText(FixSettings(), Language(), "menu.title"),
                {reinterpret_cast<uintptr_t>(&SourceCommand), reinterpret_cast<uintptr_t>(&SourceKey),
                 reinterpret_cast<uintptr_t>(&SourceEscape), reinterpret_cast<uintptr_t>(&SourceLeft), reinterpret_cast<uintptr_t>(&SourceRight),
                 reinterpret_cast<uintptr_t>(&SourceFocused)}, current) ||
            !current() || !owner->PublishEntry(host->api))
        { owner->Cancel(); host->diagnostics |= SourceRefused; return; }
        host->diagnostics |= EntryPublished;
    }
    inline void Describe(const std::shared_ptr<Owner>& owner)
    {
        const auto selected = owner->SelectedItem();
        const auto client = Client(owner->ready.view);
        if (!selected || !client || !owner->active || owner->active->count < 2) return;
        const auto pool = owner->active.get();
        const auto token = owner->logical.settings.Revision();
        const auto physical = owner->ready.view.Selected();
        auto text = owner->logical.settings.RestartRequired()
            ? owner->logical.settings.Text("menu.restartwarning")
            : pool->presented.items[*selected].description;
        std::replace(text.begin(), text.end(), '\r', ' ');
        std::replace(text.begin(), text.end(), '\n', ' ');
        auto current = [&]() { return host->source == owner && owner->Current() && owner->Private() &&
            owner->active.get() == pool && token == owner->logical.settings.Revision() && physical == owner->ready.view.Selected(); };
        const auto height = (client->bottom - client->top) / static_cast<LONG>(pool->presented.items.size() + 2);
        if (!FitControl(host->api, owner->ready, pool->nodes[1], text, client->right - client->left, height, false, current) && current())
            FitControl(host->api, owner->ready, pool->nodes[1], "", client->right - client->left, height, false, current);
    }
    inline uintptr_t LifeOriginal(DeleteJob job)
    {
        const auto slot = job.operation == LifeOp::RemoveAll ? RemoveAll : job.operation == LifeOp::ScreenTerm ? ScreenTerm :
            job.operation == LifeOp::ScreenDtor ? ScreenDtor : job.operation == LifeOp::ScreenScalar ? ScreenScalar :
            job.operation == LifeOp::ManagerTerm ? ManagerTerm : job.operation == LifeOp::ManagerDtor ? ManagerDtor :
            job.operation == LifeOp::ManagerScalar ? ManagerScalar : job.operation == LifeOp::InterfaceTerm ? InterfaceTerm :
            job.operation == LifeOp::InterfaceDtor ? InterfaceDtor : job.operation == LifeOp::InterfaceScalar ? InterfaceScalar :
            job.operation == LifeOp::ShellTerm ? ShellTerm : job.operation == LifeOp::ShellDtor ? ShellDtor : TextScalar;
        auto scope = host->scopes.Enter(Phase::Lifecycle);
        if (job.operation == LifeOp::RemoveAll)
            return host->hooks[slot].thiscall<uintptr_t>(job.receiver, static_cast<char>(job.flags));
        if (job.operation == LifeOp::ScreenScalar || job.operation == LifeOp::ManagerScalar ||
            job.operation == LifeOp::InterfaceScalar || job.operation == LifeOp::TextScalar)
            return host->hooks[slot].thiscall<uintptr_t>(job.receiver, job.flags);
        return host->hooks[slot].thiscall<uintptr_t>(job.receiver);
    }
    inline uintptr_t Destroy(DeleteJob job)
    {
        const auto owner = host->source;
        if (!host->ready || !host->scopes.Here() || !owner || !Lifecycle::Related(*owner, job.operation, job.receiver)) return LifeOriginal(job);
        if (host->life.Executing(job)) return reinterpret_cast<uintptr_t>(job.receiver);
        if (host->life.Authorized(*owner, job.operation, job.receiver)) return LifeOriginal(job);
        const bool quiet = host->scopes.Quiet();
        // Protect preparation too, not only original teardown: Select/StringText
        // may synchronously reenter the whole update before LifeOriginal starts.
        auto collection = quiet ? host->scopes.Enter(Phase::Lifecycle) : Traversals::Scope{};
        if (!host->life.Queue(*owner, job)) return reinterpret_cast<uintptr_t>(job.receiver);
        if (!quiet) return reinterpret_cast<uintptr_t>(job.receiver);
        uintptr_t result = reinterpret_cast<uintptr_t>(job.receiver);
        if (!host->life.Collect(*owner, [&](bool forced) { return owner->Dispose(host->api, forced); }, [&](DeleteJob pending)
        { const auto value = LifeOriginal(pending); if (pending.operation == job.operation && pending.receiver == job.receiver) result = value; }))
        { host->diagnostics |= ViewRefused; return result; }
        if (host->source == owner) host->source.reset();
        host->diagnostics |= OwnerRetired;
        return result;
    }
    inline void Drain()
    {
        auto service = host->scopes.Service();
        if (!service || !host->ready) return;
        DWORD foreground{};
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (foreground != GetCurrentProcessId()) host->acceptDown = host->repeatAccept = false;
        const auto owner = host->source;
        if (owner && owner->retired)
        {
            if (!host->life.Collect(*owner, [&](bool forced) { return owner->Dispose(host->api, forced); }, &LifeOriginal))
            { host->diagnostics |= ViewRefused; FlushLog(); return; }
            if (host->source == owner) host->source.reset();
            host->diagnostics |= OwnerRetired;
        }
        else if (owner)
        {
            if (!owner->Registered()) { owner->Cancel(); return; }
            const auto now = View::Get(reinterpret_cast<HMODULE>(host->api.Base()), owner->Table());
            // Preserve a covered session. No API/native mutations while a modal
            // or another native menu manager temporarily covers this screen.
            if (Read<uint8_t>(owner->ready.view.interfaceManager, 10500).value_or(0) ||
                Read<void*>(owner->ready.view.screen, 216).value_or(nullptr) ||
                (now && now->Covered())) return;
            if (!now || !now->Same(owner->ready.view) || !now->ScreenMode())
            { Close(owner); owner->Cancel(); return; }
            if (owner->logical.returnPending || (owner->logical.settings.IsOpen() && !owner->Private())) Close(owner);
            while (auto request = owner->logical.pending.Pop())
            {
                Act(owner, *request);
                if (host->source != owner || owner->retired || !owner->Current()) break;
            }
            if (owner->logical.settings.IsOpen() && owner->Current() && owner->Private() && owner->logical.settings.NeedsRefresh())
            {
                if (!Build(owner, owner->logical.settings.Shown())) { host->diagnostics |= ViewRefused; Close(owner); }
            }
            if (owner->logical.settings.IsOpen() && owner->Private()) Describe(owner);
            host->life.CleanOwned([&]()
            {
                for (auto pool : {&owner->retiring, &owner->staging})
                {
                    if (!*pool) continue;
                    if (owner->PoolReferenced(**pool)) { owner->logical.returnPending = true; continue; }
                    (*pool)->Destroy(host->api); pool->reset();
                }
            });
        }
        else Capture();
        FlushLog();
    }
    inline uintptr_t __fastcall Updated(void* self, void*)
    {
        uintptr_t result{};
        if (!host->scopes.BindUpdateThread()) return host->hooks[Update].thiscall<uintptr_t>(self);
        { auto scope = host->scopes.Enter(Phase::Update); result = host->hooks[Update].thiscall<uintptr_t>(self); }
        if (host->scopes.MarkCompleteUpdate())
            try { Drain(); } catch (...) { host->diagnostics |= Exception; if (host->source) host->source->logical.returnPending = true; }
        return result;
    }
    inline uintptr_t __fastcall PostUpdated(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Transition);
        const auto result = host->hooks[PostUpdate].thiscall<uintptr_t>(self);
        return result;
    }
    inline uintptr_t __fastcall Rendered(void* self, void*)
    {
        auto scope = host->scopes.Enter(Phase::Render);
        const auto result = host->hooks[Render].thiscall<uintptr_t>(self);
        return result;
    }
    inline char __fastcall KeyPressed(void* self, void*, uint32_t key, uint32_t repeat)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        if (key == VK_RETURN) { host->repeatAccept = host->acceptDown; host->acceptDown = true; }
        const auto result = host->hooks[KeyDown].thiscall<char>(self, key, repeat);
        host->repeatAccept = false;
        return result;
    }
    inline char __fastcall KeyReleased(void* self, void*, uint32_t key)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        if (key == VK_RETURN) { host->acceptDown = false; host->repeatAccept = false; }
        const auto result = host->hooks[KeyUp].thiscall<char>(self, key);
        return result;
    }
    inline char __fastcall CharacterReceived(void* self, void*, uint32_t character)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[Character].thiscall<char>(self, character);
        return result;
    }
    inline uintptr_t __fastcall MousePressed(void* self, void*, int x, int y)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[MouseDown].thiscall<uintptr_t>(self, x, y);
        return result;
    }
    inline uintptr_t __fastcall MouseReleased(void* self, void*, int x, int y)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[MouseUp].thiscall<uintptr_t>(self, x, y);
        return result;
    }
    inline char __fastcall CommandPressed(void* self, void*, uint32_t command)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[CommandOn].thiscall<char>(self, command);
        return result;
    }
    inline char __fastcall CommandReleased(void* self, void*, uint32_t command)
    {
        auto scope = host->scopes.Enter(Phase::Input);
        const auto result = host->hooks[CommandOff].thiscall<char>(self, command);
        return result;
    }
    inline uintptr_t __fastcall Removed(void* self, void*, char flags)
    {
        return Destroy({LifeOp::RemoveAll, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ScreenTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ScreenTerm, self, 0});
    }
    inline uintptr_t __fastcall ScreenDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ScreenDtor, self, 0});
    }
    inline uintptr_t __fastcall ScreenDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::ScreenScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ManagerTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ManagerTerm, self, 0});
    }
    inline uintptr_t __fastcall ManagerDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::ManagerScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall ManagerDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ManagerDtor, self, 0});
    }
    inline uintptr_t __fastcall InterfaceTerminated(void* self, void*)
    {
        return Destroy({LifeOp::InterfaceTerm, self, 0});
    }
    inline uintptr_t __fastcall InterfaceDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::InterfaceScalar, self, static_cast<uint32_t>(flags)});
    }
    inline uintptr_t __fastcall InterfaceDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::InterfaceDtor, self, 0});
    }
    inline uintptr_t __fastcall ShellDestroyed(void* self, void*)
    {
        return Destroy({LifeOp::ShellDtor, self, 0});
    }
    inline uintptr_t __fastcall ShellTerminated(void* self, void*)
    {
        return Destroy({LifeOp::ShellTerm, self, 0});
    }
    inline uintptr_t __fastcall TextDeleted(void* self, void*, uint32_t flags)
    {
        return Destroy({LifeOp::TextScalar, self, static_cast<uint32_t>(flags)});
    }
    inline void InitImpl(HMODULE image)
    {
        if (host) return;
        auto candidate = std::make_unique<Host>();
        HMODULE own{}, game{};
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&InitImpl), &own);
        candidate->ownModule = own;
        if (!candidate->api.Bind(image))
        {
            host = candidate.release(); host->diagnostics |= Unsupported; FlushLog(); return;
        }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&InitImpl), &own) ||
            !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(image), &game)) return;
        candidate->ownModule = own;
        host = candidate.release();
        host->hooks[0] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x51150), &Updated, safetyhook::InlineHook::StartDisabled);
        host->hooks[1] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x4AC90), &PostUpdated, safetyhook::InlineHook::StartDisabled);
        host->hooks[2] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6F60), &Rendered, safetyhook::InlineHook::StartDisabled);
        host->hooks[3] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x71F80), &KeyPressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[4] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6B390), &KeyReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[5] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6B430), &CharacterReceived, safetyhook::InlineHook::StartDisabled);
        host->hooks[6] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x721F0), &MousePressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[7] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x722F0), &MouseReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[8] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6F860), &CommandPressed, safetyhook::InlineHook::StartDisabled);
        host->hooks[9] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6B260), &CommandReleased, safetyhook::InlineHook::StartDisabled);
        host->hooks[10] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x8BC0), &Removed, safetyhook::InlineHook::StartDisabled);
        host->hooks[11] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x91A0), &ScreenTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[12] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x92D0), &ScreenDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[13] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0xCD4F0), &ScreenDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[14] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0xCC3B0), &ManagerTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[15] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0xCC690), &ManagerDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[16] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0xCC320), &ManagerDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[17] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x6AE10), &InterfaceTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[18] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x732F0), &InterfaceDeleted, safetyhook::InlineHook::StartDisabled);
        host->hooks[19] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x72E40), &InterfaceDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[20] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x50D70), &ShellDestroyed, safetyhook::InlineHook::StartDisabled);
        host->hooks[21] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x4E7D0), &ShellTerminated, safetyhook::InlineHook::StartDisabled);
        host->hooks[22] = safetyhook::create_inline(reinterpret_cast<void*>(host->api.Base() + 0x12BF40), &TextDeleted, safetyhook::InlineHook::StartDisabled);
        for (auto& hook : host->hooks) if (!hook)
        { for (auto& installed : host->hooks) installed.reset(); host->diagnostics |= HookRefused; FlushLog(); return; }
        // All trampolines exist before any hook can execute. Before committed
        // readiness, callbacks delegate through these complete originals.
        for (auto& hook : host->hooks) if (!hook.enable())
        { for (auto& installed : host->hooks) installed.reset(); host->diagnostics |= HookRefused; FlushLog(); return; }
        host->ready = true;
    }
    inline void Init(HMODULE image) noexcept
    {
        try { InitImpl(image); }
        catch (...)
        {
            if (!host) return;
            host->ready = false;
            for (auto& hook : host->hooks) hook.reset();
            host->diagnostics |= Exception;
            try { FlushLog(); } catch (...) {}
        }
    }
}
