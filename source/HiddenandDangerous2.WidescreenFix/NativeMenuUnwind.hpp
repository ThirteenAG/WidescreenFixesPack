#pragma once
#include "NativeMenuHost.hpp"
#include <safetyhook.hpp>
#include <string>
#include <unordered_set>
#include <vector>

namespace HD2Menu
{
    struct State
    {
        HMODULE module = nullptr;
        DWORD thread = 0;
        bool draining = false;
        bool enabled = false;
        unsigned worldDepth = 0;
        unsigned inputDepth = 0;
        unsigned dispatchDepth = 0;
        unsigned drawDepth = 0;
        unsigned destructionDepth = 0;
        void* driver = nullptr;
        std::vector<std::shared_ptr<Host>> hosts;
        std::unordered_set<void*> refused;
        std::unordered_set<std::string> logged;
        safetyhook::InlineHook world, input, keyboard, dispatch, queue, groupDraw, textDraw, windowDestroy, buttonDestroy, managerDestroy;

        void Log(const std::string& message) noexcept
        {
            try
            {
                // Once per reason; native callbacks never log, the frame and Init do.
                if (logged.size() >= 64 || !logged.insert(message).second) return;
                OutputDebugStringA(("HD2 native menu: " + message + "\n").c_str());
            }
            catch (...) {}
        }

        bool Outside() const
        {
            return !worldDepth && !inputDepth && !dispatchDepth && !drawDepth && !destructionDepth;
        }

        std::shared_ptr<Host> Find(void* manager)
        {
            for (auto& host : hosts) if (host->context.manager == manager && !host->queueCleared) return host;
            return {};
        }

        void Service()
        {
            if (!enabled || draining || !Outside() || GetCurrentThreadId() != thread) return;
            if (!driver)
            {
                auto candidate = reinterpret_cast<void*(__cdecl*)()>(Context::graphics + 0x68FB0)();
                uintptr_t table = 0;
                if (!Context::Read(candidate, 0, table) || table != Context::graphics + 0x9B640)
                { Log("native graphics driver readiness deferred"); return; }
                driver = candidate;
            }
            bool before = draining;
            draining = true;
            struct Restore { bool& value; bool before; ~Restore() { value = before; } } restore{ draining, before };
            // Snapshot ownership: discovery, settings and native focus calls may
            // synchronously enter a teardown hook or append another host.
            auto snapshot = hosts;
            for (auto& host : snapshot)
            {
                try
                {
                    host->notice = nullptr;
                    if (host->queueCleared) host->Cleanup();
                    else if (host->invalid) host->Recover();
                    else
                    {
                        host->Frame(module);
                        if (host->invalid) host->Recover();
                    }
                    if (host->notice) Log(host->notice);
                }
                catch (...)
                {
                    host->Retire();
                    Log("private Model/read/widget continuation refused; restoration deferred");
                }
            }
            hosts.erase(std::remove_if(hosts.begin(), hosts.end(), [](const auto& host) {
                return host->queueCleared;
            }), hosts.end());
            void* world = nullptr;
            void* manager = nullptr;
            if (Context::Read(reinterpret_cast<void*>(ABI::game), 0x4AED14, world)
                && Context::Read(world, 20, manager) && Find(manager)) return;
            Context context;
            if (!context.Discover()) { Log(context.refusal); return; }
            if (Find(context.manager) || refused.count(context.manager)) return;
            if (hosts.size() >= 64) { Log("native host capacity refused"); return; }
            // The first Options activation can precede its first native Draw,
            // which selects CText.font/properties. Defer this read-only readiness
            // phase; never permanently refuse a manager merely for that frame.
            auto current = [&] { return context.Alive() && Context::Top() == context.options; };
            for (auto button : context.buttons)
            {
                TextProbe probe;
                if (!probe.Capture(button, current) || !current())
                { Log("native source font/glyph/layout readiness deferred"); return; }
            }
            auto host = std::make_shared<Host>(context, Context::graphics, driver);
            hosts.push_back(host); // Stable callback ownership before factories.
            try
            {
                if (!host->Prepare())
                {
                    Log(std::string("category construction refused: ") + host->gate);
                    refused.insert(context.manager);
                    host->Retire();
                    host->Recover();
                }
            }
            catch (...)
            {
                refused.insert(context.manager);
                host->Retire();
                host->Recover();
                Log("category construction allocation/native-layout rollback");
            }
        }
    };

    // Supported native bridge pins its ASI before publishing callbacks. This
    // state intentionally lives until process exit; dynamic unload is unsupported.
    inline State*& Storage() { static State* state = nullptr; return state; }

    struct Depth
    {
        unsigned& depth;
        explicit Depth(unsigned& depth) : depth(depth) { ++depth; }
        ~Depth() { --depth; }
    };

    inline uint8_t __fastcall World(void* owner, void*, int32_t delta)
    {
        auto& state = *Storage();
        uint8_t result;
        {
            Depth depth(state.worldDepth);
            if (!state.thread) state.thread = GetCurrentThreadId();
            result = state.world.unsafe_fastcall<uint8_t>(owner, nullptr, delta);
        }
        try { state.Service(); } catch (...) { state.Log("frame service allocation refused"); }
        return result;
    }

    inline uint8_t __fastcall Input(void* owner, void*, int32_t first, int32_t second)
    {
        auto& state = *Storage();
        Depth depth(state.inputDepth);
        return state.input.unsafe_fastcall<uint8_t>(owner, nullptr, first, second);
    }

    inline uint8_t __fastcall Dispatch(void* owner, void*, uint32_t event, uint32_t value)
    {
        auto& state = *Storage();
        Depth depth(state.dispatchDepth);
        return state.dispatch.unsafe_fastcall<uint8_t>(owner, nullptr, event, value);
    }

    inline uint32_t __fastcall Queue(void* manager, void*)
    {
        //66C790 saves incomingECX inEBX and66C8FB restores it for66C950.
        auto& state = *Storage();
        Depth depth(state.dispatchDepth);
        return state.queue.unsafe_fastcall<uint32_t>(manager, nullptr);
    }

    inline uint8_t __fastcall GroupDraw(void* owner, void*)
    {
        auto& state = *Storage();
        Depth depth(state.drawDepth);
        return state.groupDraw.unsafe_fastcall<uint8_t>(owner, nullptr);
    }

    inline uint8_t __fastcall TextDraw(void* owner, void*)
    {
        auto& state = *Storage();
        Depth depth(state.drawDepth);
        return state.textDraw.unsafe_fastcall<uint8_t>(owner, nullptr);
    }

    inline uint8_t __fastcall Keyboard(void* window, void*, uint32_t scan)
    {
        auto& state = *Storage();
        Depth depth(state.inputDepth);
        if (state.enabled && state.thread == GetCurrentThreadId())
        {
            for (auto& stored : state.hosts)
            {
                auto host = stored;
                bool privatePane = host->Private() && host->window.Get() == window;
                bool sourcePane = host->Alive() && !host->open && host->context.options == window
                    && Context::Top() == window && host->entry.Get();
                if (!privatePane && !sourcePane) continue;
                auto parent = privatePane ? window : host->context.group;
                auto captured = host->Capture();
                // Native668670 does not consume Escape. Otherwise66B3E0
                // queues0200000A to a stock controller for this root index,
                // which does not exist for the owned extra window. Defer our
                // logical Back and return1 only when its exact queue accepts.
                if (privatePane && scan == 1 && host->QueueEscape()) return 1;
                void** children = nullptr;
                size_t count = 0;
                int32_t selected = -1;
                if (!Context::Children(parent, children, count) || !count || !Context::Read(parent, 108, selected)) break;
                // Up/down change focus through native Focus. All stock accepts,
                // Esc/modal routing and forced-close input still reach original.
                auto code = scan & 0x7F;
                if (code == 0x48 || code == 0x50)
                {
                    int direction = code == 0x48 ? -1 : 1;
                    int32_t next = selected;
                    for (size_t attempt = 0; attempt < count; ++attempt)
                    {
                        next = (next + direction + static_cast<int32_t>(count)) % static_cast<int32_t>(count);
                        uint8_t disabled = 1;
                        uint32_t id = 0;
                        if (!Context::Read(children[next], 100, disabled) || disabled
                            || !Context::Read(children[next], 96, id)) continue;
                        Host::Focus(parent, id);
                        return captured.Current() ? 1 : 0;
                    }
                }
                else if (privatePane && (code == 0x4B || code == 0x4D) && selected > 0
                    && static_cast<size_t>(selected) < host->bindings.size())
                {
                    if (host->Queue(host->bindings[selected], host->window.Cell(selected), code == 0x4B ? -1 : 1)) return 1;
                }
            }
        }
        return state.keyboard.unsafe_fastcall<uint8_t>(window, nullptr, scan);
    }

    inline uint32_t __fastcall WindowDestroyed(void* window, void*)
    {
        auto& state = *Storage();
        Depth depth(state.destructionDepth);
        for (auto& stored : state.hosts)
        {
            auto host = stored;
            if (host->context.options == window) host->Retire();
            if (host->window.Get() == window)
            {
                host->Retire();
                host->window.NativeDestroyed();
            }
        }
        return state.windowDestroy.unsafe_fastcall<uint32_t>(window, nullptr);
    }

    inline uint32_t __fastcall ButtonDestroyed(void* button, void*)
    {
        auto& state = *Storage();
        Depth depth(state.destructionDepth);
        for (auto& stored : state.hosts)
        {
            auto host = stored;
            bool owned = host->entry.Get() == button;
            for (size_t i = 0; i < host->window.Count(); ++i) owned |= host->window.Cell(i) == button;
            for (auto original : host->context.buttons) owned |= original == button;
            if (owned) host->Retire();
            if (host->entry.Get() == button) host->entry.NativeDestroyed();
        }
        return state.buttonDestroy.unsafe_fastcall<uint32_t>(button, nullptr);
    }

    inline uint8_t __fastcall ManagerDestroyed(void* manager, void*)
    {
        auto& state = *Storage();
        Depth depth(state.destructionDepth);
        auto host = state.Find(manager);
        if (host)
        {
            host->Retire();
            // Parent links do not retain the scene's native root. Detach our
            // separately owned sector before66B140 can clear/free that parent.
            // Native widget destructors do not need it linked or dereference
            // their borrowed scene; their frame/CPP storage remains retained.
            if (host->sector) ABI::LinkFrame(host->sector, nullptr);
        }
        auto result = state.managerDestroy.unsafe_fastcall<uint8_t>(manager, nullptr);
        //66B140 has now deleted registered windows before its scene and fully
        // cleared the native24-byte message queue. Free CPP cells only after
        // every outer input/draw/dispatch/world scope has returned as well.
        if (host) host->AfterManagerDestroyed();
        state.refused.erase(manager);
        return result;
    }
}
