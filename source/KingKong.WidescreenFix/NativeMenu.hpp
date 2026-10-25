#pragma once

#include "NativeMenuHost.hpp"
#include "NativeMenuContracts.hpp"
#include "FixSettings.hpp"
#include <intrin.h>
#include <set>

namespace KongMenu
{
    inline std::shared_ptr<Host> host;
    inline HMODULE module = nullptr;
    inline void* world = nullptr;
    inline unsigned outerDepth = 0, worldDepth = 0, optionDepth = 0;
    inline bool draining = false;
    inline std::atomic<bool> installed = false;
    inline std::atomic<bool> attempted = false;
    inline uint32_t inputMask = 0;
    inline SafetyHookInline wholeHook, worldHook, optionsHook, selectHook, mouseHook;
    inline SafetyHookInline titleHook, pauseHook, temporaryHook, objectHook, behaviorHook, languageHook, subtitleHook;

    struct Origin
    {
        void* world = nullptr;
        void* owner = nullptr;
        void* behavior = nullptr;
        void* parameters = nullptr;
        void* target = nullptr;
        bool pause = true;
    };
    inline Origin origin;
    inline std::weak_ptr<Host> requestedOpen;
    inline uint64_t requestedGeneration = 0;

    struct Depth
    {
        unsigned& value;
        explicit Depth(unsigned& counter) : value(counter) { ++value; }
        ~Depth() { --value; }
    };

    inline void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> emitted;
            if (emitted.size() >= 24 || !emitted.emplace(message).second) return;
            OutputDebugStringA(("King Kong native menu: " + std::string(message) + "\n").c_str());
        }
        catch (...) {}
    }

    inline void Cancel(const char* reason) noexcept
    {
        auto lease = host;
        if (lease) lease->Cancel();
        requestedOpen.reset();
        Log(reason);
    }

    inline bool Global(const std::shared_ptr<Host>& owner)
    {
        return owner && owner == host && owner->Current() &&
            Native::Read<void*>(reinterpret_cast<void*>(0xB420DC), 0) == owner->context.behavior &&
            Native::Read<void*>(reinterpret_cast<void*>(0xB420E4), 0) == owner->context.owner &&
            Native::Read<int>(reinterpret_cast<void*>(0xB420D0), 0) == 3;
    }

    inline bool Once(uint32_t bit)
    {
        if (inputMask & bit) return false;
        inputMask |= bit;
        return true;
    }

    inline bool BackPressed() { return Native::Pressed(3) || Native::Key(259) || Native::Key(2); }
    inline bool AcceptPressed() { return Native::Pressed(0) || Native::Pressed(7); }

    inline std::string MenuLanguage()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (chosen == tag) return chosen;
        // 8FD260 returns this exact native locale field. Only the established
        // English enum has a known tag; other enums use the resource fallback.
        if (Native::Read<uint32_t>(reinterpret_cast<void*>(0xBE2D18), 0) != 1)
            Log("native locale tag unproved; using English menu resources");
        return "en";
    }

    inline void Invalidate(void* object, void* behavior = nullptr)
    {
        auto lease = host;
        if (lease && (object == lease->context.owner || object == lease->context.text ||
                      behavior == lease->context.behavior)) lease->Retire();
        if (object == origin.owner || behavior == origin.behavior) origin = {};
    }

    inline int OriginCall(SafetyHookInline& hook, bool pause)
    {
        if (!installed) return hook.unsafe_ccall<int>();
        auto owner = Native::Read<void*>(reinterpret_cast<void*>(0xB420E4), 0);
        auto behavior = Native::Read<void*>(reinterpret_cast<void*>(0xB420DC), 0);
        auto parameters = Native::Readable(behavior, 88) ? Native::Read<void*>(behavior, 64) : nullptr;
        auto target = Native::Read<void*>(reinterpret_cast<void*>(0xB99594), 0);
        const bool eligible = world && Native::Readable(parameters, 1396) &&
            Native::Readable(target, 24) && Native::Read<uintptr_t>(target, 20) == 0x623150 &&
            Native::Read<int>(reinterpret_cast<void*>(0xB420D0), 0) == 3;
        auto result = hook.unsafe_ccall<int>();
        if (eligible && Native::Read<void*>(reinterpret_cast<void*>(0xB420E4), 0) == owner &&
            Native::Read<void*>(reinterpret_cast<void*>(0xB420DC), 0) == behavior &&
            Native::Readable(behavior, 88) && Native::Read<void*>(behavior, 64) == parameters &&
            Native::Read<void*>(behavior, 32) == target)
            origin = Origin{ world, owner, behavior, parameters, target, pause };
        return result;
    }

    inline int __cdecl Title() { return OriginCall(titleHook, false); }
    inline int __cdecl Pause() { return OriginCall(pauseHook, true); }

    inline uintptr_t __cdecl Select(int count, int wrap)
    {
        auto lease = host;
        if (installed && _ReturnAddress() == reinterpret_cast<void*>(0x62354D) &&
            optionDepth && count == 4 && wrap == 1 && Global(lease) &&
            !lease->open && lease->entryReady)
            count = 5;
        return selectHook.unsafe_ccall<uintptr_t>(count, wrap);
    }

    inline void __cdecl Mouse(int style)
    {
        auto caller = _ReturnAddress();
        mouseHook.unsafe_ccall(style);
        auto lease = host;
        if (installed && caller == reinterpret_cast<void*>(0x623559) && optionDepth && Global(lease) &&
            !lease->open && lease->entryReady)
        {
            const auto y = Native::Read<float>(lease->context.parameters, 472);
            const auto x = Native::Read<float>(lease->context.parameters, 468);
            if ((style == 5 || style == 6) && std::isfinite(x) && std::isfinite(y) &&
                x > lease->context.style.x - lease->Width() && x < lease->context.style.x + lease->Width() &&
                y > 0.68f && y < 0.76f)
                Host::Write<int>(lease->context.parameters, 60, 4);
        }
    }

    inline void PrivateInput(const std::shared_ptr<Host>& lease)
    {
        if (!Global(lease) || !lease->open) return;
        // These globals-shaped common helpers run only in the real native
        // Options callback. Deferred publication never invokes either helper.
        const auto common = reinterpret_cast<int (__cdecl*)()>(0x4DAA80)();
        reinterpret_cast<int (__cdecl*)()>(0x4DACB0)();
        if (!common || !Global(lease)) return;
        if (BackPressed() && Once(1)) { lease->Queue(0, true); return; }
        if (lease->busy)
        {
            if (lease->selected < lease->view.items.size() &&
                Native::Read<int>(lease->context.parameters, 60) == int(lease->selected) &&
                AcceptPressed() && lease->view.items[lease->selected].kind == Kind::Back && Once(2))
                lease->Queue(0, true);
            return;
        }
        if (!lease->ready || lease->selected >= lease->view.items.size() ||
            Native::Read<int>(lease->context.parameters, 60) != int(lease->selected)) return;
        const auto count = lease->view.items.size();
        auto previous = lease->selected;
        if (Native::Read<int>(lease->context.parameters, 76) == 1 && Once(4))
            lease->selected = (lease->selected + 1) % count;
        if (Native::Read<int>(lease->context.parameters, 80) == 1 && Once(8))
            lease->selected = (lease->selected + count - 1) % count;
        const auto x = Native::Read<float>(lease->context.parameters, 468);
        const auto y = Native::Read<float>(lease->context.parameters, 472);
        const auto dx = Native::Read<float>(lease->context.parameters, 484);
        const auto dy = Native::Read<float>(lease->context.parameters, 488);
        if (std::isfinite(x) && std::isfinite(y) && std::isfinite(dx) && std::isfinite(dy) &&
            (dx != 0 || dy != 0 || Native::Pressed(7)) &&
            x > lease->context.style.x - lease->Width() && x < lease->context.style.x + lease->Width())
            for (size_t i = 0; i < count; ++i)
                if (y > 0.35f + 0.08f * float(i) && y < 0.44f + 0.08f * float(i)) lease->selected = i;
        if (previous != lease->selected)
        {
            Host::Write<int>(lease->context.parameters, 60, int(lease->selected));
            lease->repaint = true;
        }
        if (Native::Read<int>(lease->context.parameters, 84) == 1 && Once(16)) lease->Queue(-1);
        else if (Native::Read<int>(lease->context.parameters, 88) == 1 && Once(32)) lease->Queue(1);
        else if (AcceptPressed() && Once(2)) lease->Queue(0);
    }

    inline int __cdecl Options()
    {
        if (!installed) return optionsHook.unsafe_ccall<int>();
        auto lease = host;
        try
        {
        if (lease && lease->restore && lease->Current()) lease->Close();
        if (lease && lease->open && Global(lease))
        {
            PrivateInput(lease);
            return 0;
        }
        if (lease && lease->open && !lease->Current()) lease->Close();
        }
        catch (...)
        {
            Cancel("private input allocation failed; deferred stock restoration requested");
            if (lease && lease->open) return 0;
        }
        int result;
        {
            Depth depth(optionDepth);
            result = optionsHook.unsafe_ccall<int>();
        }
        try
        {
        Context context;
        if (!world || !Context::Capture(world, context))
        {
            Log("Options source context/range/committed owner unavailable");
            return result;
        }
        if (!host || host->invalid || host->context.owner != context.owner ||
            host->context.behavior != context.behavior || host->context.parameters != context.parameters ||
            host->context.text != context.text || host->context.model != context.model ||
            host->context.first != context.first)
        {
            if (host) host->Retire();
            host = std::make_shared<Host>();
            host->context = context;
            host->module = module;
            host->pause = !(origin.world == context.world && origin.owner == context.owner &&
                origin.behavior == context.behavior && origin.parameters == context.parameters &&
                origin.target == context.page && !origin.pause);
        }
        lease = host;
        lease->repaint = true;
        if (lease->entryReady && Global(lease) && Native::Read<int>(context.parameters, 60) == 4 &&
            !Native::Read<int>(context.parameters, 516) && !BackPressed() && AcceptPressed() && Once(2))
        {
            requestedOpen = lease;
            requestedGeneration = lease->generation;
        }
        return result;
        }
        catch (...)
        {
            Cancel("Options observation allocation failed; stock result preserved");
            return result;
        }
    }

    inline void Service()
    {
        auto lease = host;
        if (!lease || lease->invalid) return;
        auto request = requestedOpen.lock();
        requestedOpen.reset();
        if (request == lease && requestedGeneration == lease->generation && lease->Current())
        {
            // Read TextLanguage on reopen; unknown native locale enums use
            // the English resource fallback.
            if (!lease->Open(MenuLanguage())) Log("private Open discovery/font/layout transaction refused; stock retained");
        }
        lease->Frame();
        if (!lease->Current() || lease->open || lease->invalid) return;
        auto current = lease->Snapshot(false);
        Text text(lease->context, current);
        auto texts = FixSettings().GetTexts();
        texts.Select(MenuLanguage());
        const auto label = texts.Resolve("@menu.title");
        const auto color = Native::Read<int>(lease->context.parameters, 60) == 4 ? 0xFFFFFFFF : 0xFF606060;
        if (text.Put(lease->context.first + 9, label, lease->context.style.x, 0.72f,
                     lease->Width(), 0.065f, color))
        {
            lease->entryReady = true;
        }
        else
        {
            lease->entryReady = false;
            if (current())
            {
                text.Hide(lease->context.first + 9);
                if (Native::Read<int>(lease->context.parameters, 60) == 4)
                    Host::Write<int>(lease->context.parameters, 60, 3);
            }
            Log("category native glyph/font/scratch/capacity/layout refused");
        }
    }

    inline void __cdecl Whole()
    {
        if (!installed) { wholeHook.unsafe_ccall(); return; }
        const bool outer = outerDepth == 0;
        if (outer && !draining) inputMask = 0;
        {
            Depth depth(outerDepth);
            wholeHook.unsafe_ccall();
        }
        if (!outerDepth && !worldDepth && !optionDepth && !draining)
        {
            struct Drain { Drain() { draining = true; } ~Drain() { draining = false; } } drain;
            try { Service(); }
            catch (...) { Cancel("page transaction allocation failed; deferred stock restoration requested"); }
        }
    }

    inline uint32_t __cdecl World(void* value)
    {
        if (!installed) return worldHook.unsafe_ccall<uint32_t>(value);
        struct Scope
        {
            void* previous = world;
            Depth depth{ worldDepth };
            explicit Scope(void* next) { world = next; }
            ~Scope() { world = previous; }
        } scope(value);
        return worldHook.unsafe_ccall<uint32_t>(value);
    }

    inline void __cdecl Temporary(void* value, char mode)
    {
        if (installed) Invalidate(value);
        temporaryHook.unsafe_ccall(value, mode);
    }
    inline void __cdecl Object(void* value, char mode)
    {
        if (installed) Invalidate(value);
        objectHook.unsafe_ccall(value, mode);
    }
    inline void __cdecl Behavior(void* value)
    {
        if (installed) Invalidate(nullptr, value);
        behaviorHook.unsafe_ccall(value);
    }
    inline int __cdecl Language(unsigned value)
    {
        if (installed) { if (host) host->Retire(); origin = {}; }
        return languageHook.unsafe_ccall<int>(value);
    }
    inline int __cdecl Subtitle(unsigned value)
    {
        if (installed) { if (host) host->Retire(); origin = {}; }
        return subtitleHook.unsafe_ccall<int>(value);
    }

    inline void Rollback()
    {
        installed = false;
        if (host) host->Retire();
        host.reset(); requestedOpen.reset(); origin = {};
        // Restore targets but retain original trampolines for any forwarding
        // callback already on-stack. The supported module is process-pinned.
        (void)subtitleHook.disable(); (void)languageHook.disable(); (void)behaviorHook.disable();
        (void)objectHook.disable(); (void)temporaryHook.disable(); (void)pauseHook.disable();
        (void)titleHook.disable(); (void)mouseHook.disable(); (void)selectHook.disable();
        (void)optionsHook.disable(); (void)worldHook.disable(); (void)wholeHook.disable();
    }

    inline void Init()
    {
        if (attempted.exchange(true)) return;
        HMODULE own = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&Init), &own)) return;
        module = own;
        if (!SupportedImage() ||
            Native::Read<uint8_t>(reinterpret_cast<void*>(0xAEC0C8), 0) != 1)
        {
            Log("unsupported Gamer's Edition executable/native compiled dispatch contracts");
            return;
        }
        // Native retained callbacks require process lifetime. Dynamic unload is
        // deliberately unsupported; pin before the first hook or retained data.
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            Log("process lifetime pin failed; bridge not exposed");
            return;
        }
        try
        {
        wholeHook = safetyhook::create_inline(reinterpret_cast<void*>(0x904EF0), Whole, SafetyHookInline::StartDisabled);
        worldHook = safetyhook::create_inline(reinterpret_cast<void*>(0x904980), World, SafetyHookInline::StartDisabled);
        optionsHook = safetyhook::create_inline(reinterpret_cast<void*>(0x623150), Options, SafetyHookInline::StartDisabled);
        selectHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4DAEE0), Select, SafetyHookInline::StartDisabled);
        mouseHook = safetyhook::create_inline(reinterpret_cast<void*>(0x4E6450), Mouse, SafetyHookInline::StartDisabled);
        titleHook = safetyhook::create_inline(reinterpret_cast<void*>(0x62F3C0), Title, SafetyHookInline::StartDisabled);
        pauseHook = safetyhook::create_inline(reinterpret_cast<void*>(0x630BD0), Pause, SafetyHookInline::StartDisabled);
        temporaryHook = safetyhook::create_inline(reinterpret_cast<void*>(0x9016C0), Temporary, SafetyHookInline::StartDisabled);
        objectHook = safetyhook::create_inline(reinterpret_cast<void*>(0x901A70), Object, SafetyHookInline::StartDisabled);
        behaviorHook = safetyhook::create_inline(reinterpret_cast<void*>(0x405EA0), Behavior, SafetyHookInline::StartDisabled);
        languageHook = safetyhook::create_inline(reinterpret_cast<void*>(0x8FCF40), Language, SafetyHookInline::StartDisabled);
        subtitleHook = safetyhook::create_inline(reinterpret_cast<void*>(0x8FD0D0), Subtitle, SafetyHookInline::StartDisabled);
        }
        catch (...)
        {
            Rollback();
            Log("hook allocation failed; installation rolled back");
            return;
        }
        if (!wholeHook || !worldHook || !optionsHook || !selectHook || !mouseHook || !titleHook ||
            !pauseHook || !temporaryHook || !objectHook || !behaviorHook || !languageHook || !subtitleHook)
        {
            Rollback();
            Log("hook installation transaction failed; all hooks restored");
            return;
        }
        // Assign every original trampoline before enabling any branch. Partial
        // installation forwards originals while the committed flag is false.
        if (!wholeHook.enable() || !worldHook.enable() || !optionsHook.enable() || !selectHook.enable() ||
            !mouseHook.enable() || !titleHook.enable() || !pauseHook.enable() || !temporaryHook.enable() ||
            !objectHook.enable() || !behaviorHook.enable() || !languageHook.enable() || !subtitleHook.enable())
        {
            Rollback();
            Log("hook enable transaction failed; all original targets restored");
            return;
        }
        installed = true;
    }
}
