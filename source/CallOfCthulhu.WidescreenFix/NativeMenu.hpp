#pragma once

#include "NativeMenuHost.hpp"
#include "NativeMenuContracts.hpp"
#include "safetyhook.hpp"
#include <set>
#include <intrin.h>

namespace CthulhuMenu
{
    inline SafetyHookInline mapperHook, mapperCleanupHook, targetHook, broadcastHook;
    inline SafetyHookInline messageHook, updateHook, serializationHook, drawHook;
    inline SafetyHookInline managerClearHook, managerDestroyHook, pageDestroyHook, rowDestroyHook;
    inline SafetyHookInline fontHook;
    inline SafetyHookInline actionHook;
    inline std::shared_ptr<Host> host;
    inline HMODULE module = nullptr;
    inline void* frameDispatchReturn = nullptr;
    inline bool installed = false, servicing = false, nativeResources = false;
    inline unsigned mapperDepth = 0, eventDepth = 0, messageDepth = 0, updateDepth = 0;
    inline unsigned serializationDepth = 0, drawDepth = 0, retirementDepth = 0, actionDepth = 0;

    struct Depth
    {
        unsigned& depth;
        explicit Depth(unsigned& value) : depth(value) { ++depth; }
        ~Depth() { --depth; }
    };

    inline bool OutsideNative()
    {
        return !mapperDepth && !eventDepth && !messageDepth && !updateDepth &&
            !serializationDepth && !drawDepth && !retirementDepth && !actionDepth;
    }

    // Failures only; each distinct message is reported once.
    inline void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> written;
            if (written.size() >= 64 || !written.emplace(message).second)
                return;
            OutputDebugStringA(message);
            OutputDebugStringA("\n");
        }
        catch (...) {}
    }

    inline void CancelFailure() noexcept
    {
        if (auto lease = host)
            lease->Cancel();
        Log("CoC native menu: private transaction refused; deferred stock restore");
    }

    // Cheap check on the static menu manager (no page walk, no VirtualQuery).
    inline bool MenuShown() noexcept
    {
        const auto manager = reinterpret_cast<const uint8_t*>(0x6C8560);
        const auto count = *reinterpret_cast<const uint32_t*>(0x6C8890);
        return manager[33] != 0 && count > 0 && count <= 10;
    }

    inline void Service() noexcept
    {
        if (!installed || servicing || !OutsideNative() || !nativeResources)
            return;
        Host::Flag guard(servicing);
        try
        {
            auto lease = host;
            if (lease && lease->invalid)
            {
                if (lease->Cleanup([] { return OutsideNative() && nativeResources; }) && host == lease)
                    host.reset();
                return;
            }
            if (lease)
            {
                // Idle between visits: nothing to do while no native menu is shown.
                if (!lease->open && !lease->restore && lease->pending.empty() && !MenuShown())
                    return;
                lease->Frame();
                return;
            }
            Context source;
            if (!MenuShown() || !source.Capture() || !source.Current(source.options))
                return;
            auto candidate = std::make_shared<Host>();
            candidate->module = module;
            host = candidate;
            const auto title = FixSettings().GetTexts().Resolve("@menu.title");
            if (!candidate->Prepare(title))
            {
                candidate->Retire();
                Log(candidate->preparationFailure ? candidate->preparationFailure : "CoC native menu: Options entry refused");
                return;
            }
        }
        catch (...) { CancelFailure(); }
    }

    inline bool Input(void* manager, uint16_t event) noexcept
    {
        if (!installed)
            return false;
        try
        {
            auto lease = host;
            if (!lease || lease->context.manager != manager)
                return false;
            if (lease->invalid)
            {
                // Never allow a private header to dereference a retired source.
                // Its copied input waits for source-independent safe recovery.
                return lease->OwnsNativePage() &&
                    (event == 0x243 || event == 0x244 || event == 0x245 || event == 0x246 ||
                     event == 0x248 || event == 0x251);
            }
            if (!lease->open)
            {
                if ((event == 0x245 || event == 0x248 || event == 0x251) &&
                    lease->Current(false) && lease->entry.Matches(lease->context) && lease->context.SelectedEntry())
                {
                    // Primitive opening request. Model discovery/UI work cannot
                    // run while manager/bus/mapper native continuations are live.
                    lease->restore = false;
                    lease->pending.clear();
                    lease->pending.push_back(Host::Pending{lease->generation, lease->serial, -2, 0, 0,
                        lease->entry.row, {}, 0, false});
                    return true;
                }
                return false;
            }
            if (event == 0x246)
                return lease->Queue(0, true);
            if (event == 0x245 || event == 0x248 || event == 0x251)
                return lease->Queue(0);
            if (event == 0x243 || event == 0x244)
                return lease->Queue(event == 0x243 ? -1 : 1);
            return false; // stock up/down/forced-close/quit always remain native
        }
        catch (...) { CancelFailure(); return false; }
    }

    inline void OpenRequested() noexcept
    {
        try
        {
            auto lease = host;
            if (!installed || !lease || lease->invalid || lease->open || lease->pending.empty())
                return;
            auto request = std::move(lease->pending.front());
            lease->pending.pop_front();
            if (request.logical != -2 || request.generation != lease->generation || request.serial != lease->serial ||
                request.row != lease->entry.row || !lease->Current(false) || !lease->context.SelectedEntry())
                return;
            // The native renderer uses western byte glyphs. Explicit supported
            // menu languages use checked conversion; auto keeps the English fallback.
            const auto chosen = FixSettings().String("MAIN.TextLanguage");
            const auto language = chosen == "fr" || chosen == "de" || chosen == "it" || chosen == "es" ? chosen : std::string("en");
            if (!lease->Open(language))
                Log("CoC native menu: private publication/layout refused");
        }
        catch (...) { CancelFailure(); }
    }

    inline void Completed() noexcept
    {
        if (!OutsideNative() || servicing)
            return;
        // Both operations have their own reentrancy guard and retained Host.
        Service();
        if (installed && nativeResources && OutsideNative() && !servicing)
        {
            Host::Flag guard(servicing);
            OpenRequested();
        }
    }

    inline int __fastcall Mapper(void* self, void*)
    {
        int result;
        { Depth scope(mapperDepth); result = mapperHook.unsafe_thiscall<int>(self); }
        Completed();
        return result;
    }
    inline int __fastcall MapperCleanup(void* self, void*)
    {
        Depth scope(mapperDepth);
        return mapperCleanupHook.unsafe_thiscall<int>(self);
    }
    inline uint8_t __fastcall Target(void* self, void*, void* event)
    {
        const auto frame = _ReturnAddress() == frameDispatchReturn && Readable(event, 20) &&
            Read<uint16_t>(event, 0) == 201 && Read<int>(event, 4) == -1 && Read<int>(event, 8) == 0;
        uint8_t result;
        { Depth scope(eventDepth); result = targetHook.unsafe_thiscall<uint8_t>(self, event); }
        // The native frame uses source -1 and target 0; finish dispatch before menu work.
        if (frame)
            Completed();
        return result;
    }
    inline uint8_t __fastcall Broadcast(void* self, void*, void* event)
    {
        Depth scope(eventDepth);
        return broadcastHook.unsafe_thiscall<uint8_t>(self, event);
    }
    inline uint8_t __fastcall Message(void* self, void*, void* event)
    {
        Depth scope(messageDepth);
        if (Readable(event, 2) && Input(self, Read<uint16_t>(event, 0)))
            return 1;
        return messageHook.unsafe_thiscall<uint8_t>(self, event);
    }
    inline int __fastcall Action(void* self, void*)
    {
        Depth scope(actionDepth);
        if (installed)
            try
            {
                if (auto lease = host)
                {
                    const auto source = !lease->open && self == lease->entry.row &&
                        lease->entry.Matches(lease->context) && lease->context.SelectedEntry();
                    bool own = false;
                    if (lease->open && lease->Current(true) && lease->surface.Alive())
                    {
                        const auto selected = Read<int>(lease->page.object, 540);
                        own = selected > 0 && size_t(selected) < Surface::Count &&
                            self == lease->surface.rows[size_t(selected)];
                    }
                    if ((source || own) && Input(lease->context.manager, 0x245))
                        return 0;
                }
            }
            catch (...) { CancelFailure(); }
        // Every unowned/stock/modal action retains its native confirmation and
        // dispatch; this callback never drains a primitive queued action.
        return actionHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall Update(void* self, void*)
    {
        Depth scope(updateDepth);
        return updateHook.unsafe_thiscall<int>(self);
    }
    inline int __cdecl Serialize()
    {
        int result;
        // Save/load serialization is not a menu frame boundary.
        { Depth scope(serializationDepth); result = serializationHook.unsafe_ccall<int>(); }
        return result;
    }
    inline int __fastcall Draw(void* self, void*, int target)
    {
        Depth scope(drawDepth);
        return drawHook.unsafe_thiscall<int>(self, target);
    }
    inline int __fastcall ManagerClear(void* self, void*)
    {
        Depth scope(retirementDepth);
        if (installed)
            try { if (auto lease = host; lease && lease->context.manager == self) lease->NativeManagerRetiring(); }
            catch (...) { CancelFailure(); }
        return managerClearHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall ManagerDestroy(void* self, void*)
    {
        Depth scope(retirementDepth);
        if (installed)
            try { if (auto lease = host; lease && lease->context.manager == self) lease->NativeManagerRetiring(); }
            catch (...) { CancelFailure(); }
        return managerDestroyHook.unsafe_thiscall<int>(self);
    }
    inline int __fastcall PageDestroy(void* self, void*)
    {
        const bool enclosingRetirement = retirementDepth != 0;
        Depth scope(retirementDepth);
        if (installed)
            try
            {
                if (auto lease = host)
                {
                    const auto tracked = self == lease->context.options || self == lease->page.object;
                    // A standalone source destruction must not leave a dead
                    // history entry for later ShowPage/deactivation to follow.
                    if (tracked && !enclosingRetirement && lease->ManagerAlive() &&
                        Read<uint32_t>(reinterpret_cast<void*>(0x6C8890), 0) <= 10)
                    {
                        Native::RemoveHistory(self);
                        for (size_t offset : {size_t(44), size_t(48), size_t(52)})
                            if (Read<void*>(lease->context.manager, offset) == self)
                                Write<void*>(lease->context.manager, offset, nullptr);
                    }
                    lease->NativePageRetiring(self);
                }
            }
            catch (...) { CancelFailure(); }
        return pageDestroyHook.unsafe_thiscall<int>(self);
    }
    inline void* __fastcall RowDestroy(void* self, void*, unsigned flags)
    {
        Depth scope(retirementDepth);
        if (installed)
            if (auto lease = host)
                lease->NativeRowRetiring(self);
        return rowDestroyHook.unsafe_thiscall<void*>(self, flags);
    }
    inline int __cdecl Font(void* backend)
    {
        Depth scope(retirementDepth);
        if (installed && !backend)
        {
            nativeResources = false;
            if (auto lease = host) lease->Retire();
        }
        const auto result = fontHook.unsafe_ccall<int>(backend);
        if (installed && backend)
            nativeResources = Readable(backend, 20) && Read<uintptr_t>(backend, 0) == 0x68AC3C;
        return result;
    }

    inline void Rollback() noexcept
    {
        installed = false;
        nativeResources = false;
        frameDispatchReturn = nullptr;
        if (auto lease = host) lease->Retire();
        actionHook.reset(); fontHook.reset(); rowDestroyHook.reset(); pageDestroyHook.reset();
        managerDestroyHook.reset(); managerClearHook.reset(); drawHook.reset(); serializationHook.reset();
        updateHook.reset(); messageHook.reset(); broadcastHook.reset(); targetHook.reset();
        mapperCleanupHook.reset(); mapperHook.reset();
    }

    inline void Init()
    {
        if (installed)
            return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        if (!module || !Contracts::Matches())
        {
            Log("CoC native menu: unsupported executable contracts");
            return;
        }
        auto frameDispatch = hook::pattern("8D 44 24 44 83 CE FF 50 66 C7 44 24 48 C9 00 89 5C 24 58 66 89 5C 24 54 89 74 24 4C 89 5C 24 50 E8 ? ? ? ? 8B C8 E8 ? ? ? ? E8 ? ? ? ? 8B C8 E8");
        auto targetDispatch = hook::pattern("56 8B 74 24 08 8B 56 08 83 FA FF 75 0A 56 E8 ? ? ? ? 5E C2 04 00 8B C2 25 FF 01 00 00 8B 04 81 85 C0 74 0B 39 10");
        if (frameDispatch.size() != 1 || targetDispatch.size() != 1 ||
            injector::GetBranchDestination(frameDispatch.get_first(39)).as_int() !=
                reinterpret_cast<uintptr_t>(targetDispatch.get_first()))
        {
            Log("CoC native menu: frame dispatch signature unavailable");
            return;
        }
        frameDispatchReturn = frameDispatch.get_first(44);
        HMODULE pinned = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned))
        {
            Log("CoC native menu: process lifetime pin refused");
            return;
        }
        // The module is pinned before any hook, so it cannot unload while hooked.
        try
        {
            mapperHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4CE6F0), Mapper, SafetyHookInline::StartDisabled);
            mapperCleanupHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4CD980), MapperCleanup, SafetyHookInline::StartDisabled);
            targetHook = safetyhook::create_inline(targetDispatch.get_first(), Target, SafetyHookInline::StartDisabled);
            broadcastHook = safetyhook::create_inline(reinterpret_cast<void*>(0x43A380), Broadcast, SafetyHookInline::StartDisabled);
            messageHook = safetyhook::create_inline(reinterpret_cast<void*>(0x40FB20), Message, SafetyHookInline::StartDisabled);
            updateHook = safetyhook::create_inline(reinterpret_cast<void*>(0x40F720), Update, SafetyHookInline::StartDisabled);
            serializationHook = safetyhook::create_inline(reinterpret_cast<void*>(0x40FF00), Serialize, SafetyHookInline::StartDisabled);
            drawHook = safetyhook::create_inline(reinterpret_cast<void*>(0x45B1E0), Draw, SafetyHookInline::StartDisabled);
            managerClearHook = safetyhook::create_inline(reinterpret_cast<void*>(0x40E9D0), ManagerClear, SafetyHookInline::StartDisabled);
            managerDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(0x40FA30), ManagerDestroy, SafetyHookInline::StartDisabled);
            pageDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(0x416FC0), PageDestroy, SafetyHookInline::StartDisabled);
            rowDestroyHook = safetyhook::create_inline(reinterpret_cast<void*>(0x41A350), RowDestroy, SafetyHookInline::StartDisabled);
            fontHook = safetyhook::create_inline(reinterpret_cast<void*>(0x43B290), Font, SafetyHookInline::StartDisabled);
            actionHook = safetyhook::create_inline(reinterpret_cast<void*>(0x419730), Action, SafetyHookInline::StartDisabled);
            if (!mapperHook || !mapperCleanupHook || !targetHook || !broadcastHook || !messageHook ||
                !updateHook || !serializationHook || !drawHook || !managerClearHook || !managerDestroyHook ||
                !pageDestroyHook || !rowDestroyHook || !fontHook || !actionHook)
            {
                Rollback();
                Log("CoC native menu: hook transaction refused");
                return;
            }
            // Publish every original trampoline before any patched entry is live.
            // Partial enable failure restores all native targets without exposure.
            if (!mapperHook.enable() || !mapperCleanupHook.enable() || !targetHook.enable() ||
                !broadcastHook.enable() || !messageHook.enable() || !updateHook.enable() || !serializationHook.enable() ||
                !drawHook.enable() || !managerClearHook.enable() || !managerDestroyHook.enable() ||
                !pageDestroyHook.enable() || !rowDestroyHook.enable() || !fontHook.enable() || !actionHook.enable())
            {
                Rollback();
                Log("CoC native menu: hook enable transaction refused");
                return;
            }
            auto backend = Read<void*>(reinterpret_cast<void*>(0x6F55B0), 0);
            nativeResources = Readable(backend, 20) && Read<uintptr_t>(backend, 0) == 0x68AC3C;
            installed = true;
        }
        catch (...)
        {
            Rollback();
            Log("CoC native menu: installation exception refused");
        }
    }
}
