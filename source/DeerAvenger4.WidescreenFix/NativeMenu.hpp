#pragma once

#include "NativeMenuLauncher.hpp"
#include "NativeMenuPause.hpp"
#include "NativeMenuContracts.hpp"
#include "FixSettings.hpp"
#include <set>

namespace DeerMenu
{
    inline HMODULE module = nullptr;
    inline std::shared_ptr<Pause> pauseHost;
    inline SafetyHookInline titleHook, keyHook, menuHook, menuDrawHook, drawHook, frameHook, appHook;
    inline bool installed = false, attempted = false, draining = false, stopping = false;
    inline unsigned frameDepth = 0, drawDepth = 0, inputDepth = 0, menuDepth = 0, appDepth = 0, titleDepth = 0;

    struct Depth
    {
        unsigned& value;
        explicit Depth(unsigned& depth) : value(depth) { ++value; }
        ~Depth() { --value; }
    };

    // Failures only; each distinct message is reported once.
    inline void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> emitted;
            if (emitted.size() >= 24 || !emitted.emplace(message).second)
                return;
            OutputDebugStringA("Deer Avenger 4 native menu: ");
            OutputDebugStringA(message);
            OutputDebugStringA("\n");
        }
        catch (...) {}
    }

    inline bool Outside()
    {
        return frameDepth == 0 && drawDepth == 0 && inputDepth == 0 && menuDepth == 0 && appDepth == 0 && titleDepth == 0;
    }

    inline void Cancel(const char* reason) noexcept
    {
        auto host = pauseHost;
        if (host) host->Retire();
        Log(reason);
    }

    inline void ReleasePause()
    {
        auto host = pauseHost;
        pauseHost.reset();
        if (!host) return;
        // The stock pause menu has three items; never leave it on our fourth one.
        if (host->context.Alive() && Global<int>(0x5322F8) == 5 && Global<int>(0x5322B0) == 3)
            Pause::Write(0x5322B0, 2);
        host->Release();
    }

    inline void Drain()
    {
        if (!installed || !Outside() || draining) return;
        Session::Busy scope(draining);
        try
        {
            auto host = pauseHost;
            if (host && (host->retired || !host->context.Alive()))
            {
                ReleasePause();
                return;
            }
            if (host && !stopping)
                host->Frame(Language());
        }
        catch (...) { Cancel("deferred page failure; native pause restored"); }
    }

    inline INT_PTR __stdcall Title(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        Depth depth(titleDepth);
        std::shared_ptr<Launcher> host;
        const auto found = launchers.find(window);
        if (found != launchers.end()) host = found->second;
        if (installed && host)
        {
            try
            {
                if (message == Launcher::OpenMessage)
                {
                    host->Open();
                    return TRUE;
                }
                if (message == WM_COMMAND && LOWORD(wParam) == Launcher::EntryID && HIWORD(wParam) == BN_CLICKED &&
                    host->Request(reinterpret_cast<HWND>(lParam))) return TRUE;
                if (message == WM_NCDESTROY) host->Retire();
            }
            catch (...) { host->Retire(); Log("title transaction refused; stock controls preserved"); }
        }
        const auto result = titleHook.unsafe_stdcall<INT_PTR>(window, message, wParam, lParam);
        if (installed && message == WM_INITDIALOG)
        {
            std::shared_ptr<Launcher> created;
            try
            {
                created = std::make_shared<Launcher>();
                if (!launchers.emplace(window, created).second)
                    return result;
                if (!created->Build(window, module))
                {
                    created->Remove();
                    created->Retire();
                    launchers.erase(window);
                    Log(created->refusal);
                }
            }
            catch (...)
            {
                if (created)
                {
                    created->Remove();
                    created->Retire();
                    const auto current = launchers.find(window);
                    if (current != launchers.end() && current->second == created) launchers.erase(current);
                }
                Log("title allocation refused; exact entry rolled back");
            }
        }
        if (message == WM_NCDESTROY) launchers.erase(window);
        return result;
    }

    inline int __cdecl Key(int event, const int* data)
    {
        int result = 0;
        {
            Depth depth(inputDepth);
            auto host = pauseHost;
            bool consumed = false;
            if (installed && !stopping && event == 18 && host && Readable(data, sizeof(int) * 2))
            {
                try { consumed = host->Key(data[1]); }
                catch (...) { Cancel("input queue failure"); }
            }
            // Consumed keys return 1 like native handlers; everything else stays native.
            result = consumed ? 1 : keyHook.unsafe_ccall<int>(event, data);
        }
        Drain();
        return result;
    }

    inline int __cdecl Menu()
    {
        Depth depth(menuDepth);
        auto host = pauseHost;
        if (installed && !stopping && host && host->Private())
        {
            // Clear the pause menu's command slots and return 0, as the game does
            // when no item is chosen; our commands are handled in Key.
            for (uintptr_t address = 0x532308; address <= 0x532324; address += 4)
                Pause::Write(address, 0);
            return 0;
        }
        if (installed && Global<int>(0x5322F8) == 5 && Global<int>(0x5322B0) == 3 && (!host || !host->Current()))
            Pause::Write(0x5322B0, 2);
        return menuHook.unsafe_ccall<int>();
    }

    inline int __cdecl MenuDraw()
    {
        Depth depth(drawDepth);
        auto host = pauseHost;
        // This function draws only menu sprites; hide the pause pane while our page replaces it.
        if (installed && !stopping && host && host->Private() && host->session->ready)
            return 0;
        return menuDrawHook.unsafe_ccall<int>();
    }

    inline int __cdecl Draw()
    {
        Depth depth(drawDepth);
        const int result = drawHook.unsafe_ccall<int>();
        if (!installed || stopping) return result;
        try
        {
            if (!PauseContext::PauseShown())
                return result;
            PauseContext context;
            if (!context.Capture(Global<void*>(0x546998), Global<void*>(0x54699C)))
                return result;
            auto host = pauseHost;
            if (!host)
            {
                host = std::make_shared<Pause>();
                host->context = context;
                pauseHost = host;
            }
            if (!host->context.Alive())
            {
                host->Retire();
                return result;
            }
            if (!host->Prepare(module) || !host->Draw())
                Cancel("pause source/font/viewport/text contract refused");
        }
        catch (...) { Cancel("pause font/draw failure"); }
        return result;
    }

    inline void __cdecl Frame()
    {
        if (installed && Outside() && Global<int>(0x532B40) != 1)
        {
            try { ReleasePause(); } catch (...) { Cancel("pre-world retirement failure"); }
        }
        {
            Depth depth(frameDepth);
            frameHook.unsafe_ccall();
        }
        Drain();
    }

    inline int __cdecl App(int event, int* data)
    {
        if (installed && event == 13)
        {
            stopping = true;
            Cancel("native engine retirement");
            for (const auto& item : launchers) item.second->Retire();
            for (const auto& item : popups) item.second->Retire();
            if (!Outside())
            {
                // Nested shutdown: forward it now and drop our font without
                // calling its destructor after the engine is gone.
                auto host = pauseHost;
                if (host) host->font = nullptr;
                pauseHost.reset();
                Log("nested shutdown: owned font quarantined; native teardown forwarded");
            }
            else try { ReleasePause(); } catch (...) { Log("pre-engine font retirement failure"); }
        }
        int result;
        {
            Depth depth(appDepth);
            result = appHook.unsafe_ccall<int>(event, data);
        }
        Drain();
        return result;
    }

    inline void Rollback() noexcept
    {
        installed = false;
        titleHook.reset(); keyHook.reset(); menuHook.reset(); menuDrawHook.reset();
        drawHook.reset(); frameHook.reset(); appHook.reset();
    }

    inline void Init(HMODULE owner) noexcept
    {
        if (attempted) return;
        attempted = true;
        module = owner;
        try
        {
            if (!owner || !Contracts::Supported()) { Log("unsupported executable/layout; no exposure"); return; }
            HMODULE pinned = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                reinterpret_cast<LPCWSTR>(&Init), &pinned) || pinned != owner)
            { Log("process-lifetime pin failed; no exposure"); return; }
            titleHook = safetyhook::create_inline(reinterpret_cast<void*>(0x424CE0), Title, SafetyHookInline::StartDisabled);
            keyHook = safetyhook::create_inline(reinterpret_cast<void*>(0x426300), Key, SafetyHookInline::StartDisabled);
            menuHook = safetyhook::create_inline(reinterpret_cast<void*>(0x410220), Menu, SafetyHookInline::StartDisabled);
            menuDrawHook = safetyhook::create_inline(reinterpret_cast<void*>(0x411310), MenuDraw, SafetyHookInline::StartDisabled);
            drawHook = safetyhook::create_inline(reinterpret_cast<void*>(0x419260), Draw, SafetyHookInline::StartDisabled);
            frameHook = safetyhook::create_inline(reinterpret_cast<void*>(0x414E80), Frame, SafetyHookInline::StartDisabled);
            appHook = safetyhook::create_inline(reinterpret_cast<void*>(0x414950), App, SafetyHookInline::StartDisabled);
            if (!titleHook || !keyHook || !menuHook || !menuDrawHook || !drawHook || !frameHook || !appHook ||
                !titleHook.enable() || !keyHook.enable() || !menuHook.enable() || !menuDrawHook.enable() ||
                !drawHook.enable() || !frameHook.enable() || !appHook.enable())
            { Rollback(); Log("hook transaction failed; all original targets restored"); return; }
            installed = true;
        }
        catch (...) { Rollback(); Log("bridge allocation failed; originals restored"); }
    }
}
