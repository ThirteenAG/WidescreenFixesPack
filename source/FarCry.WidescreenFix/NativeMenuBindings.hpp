#pragma once
#include "NativeMenu.hpp"
#include "NativeMenuGuards.hpp"
#include <safetyhook.hpp>
#include <set>

namespace FarCryMenu
{
    inline safetyhook::InlineHook updateHook, drawHook, inputHook, brokerHook, releaseHook, destroyHook,
        scriptReleaseHook, fontReleaseHook, switchHook;
    inline bool ready{}, preparing{}, bridgeReady{};
    inline unsigned releasing{}, scriptClosing{}, fontClosing{};
    inline void* frameUI{};
    inline void* bridgeScript{};
    inline void* bridgeState{};
    inline void* preparingUI{};
    inline std::string preparingEpoch;
    inline uint32_t uiLifetime{}, preparingLifetime{};
    inline size_t preparationDelay{};
    inline std::optional<uint32_t> blockedLifetime;
    inline std::set<std::string> logged;
    inline void Log(const std::string& reason) noexcept
    {
        try
        {
            if (!logged.insert(reason).second) return;
            OutputDebugStringA(("FarCry native menu: " + reason + "\n").c_str());
        }
        catch (...) {} // Diagnostics cannot unwind into native engine frames.
    }
    inline void Invalidate()
    {
        ++uiLifetime;
        if (context) { context->native.retired = true; Retire(context); }
        pending.clear();
    }
    inline bool BridgeAvailable(void* state)
    {
        return ready && bridgeReady && !scriptClosing && bridgeScript && bridgeState == state &&
            Table(bridgeScript, scriptModule, scriptTableRva) && State(bridgeScript) == state;
    }
    inline void* CallbackHandler(void* state)
    {
        if (!BridgeAvailable(state)) return nullptr;
        auto handler = Handler(bridgeScript);
        if (!Table(handler, scriptModule, handlerTableRva)) return nullptr;
        Attach(handler, state);
        return handler;
    }
    inline int BindCallbackBody(void* state)
    {
        Depth scope(nativeDepth);
        auto handler = CallbackHandler(state);
        if (!handler) return 0;
        bool accepted = false;
        std::string epoch;
        if (preparing && !releasing && preparingLifetime == uiLifetime && ParamCount(handler) == 14 &&
            String(handler, 1, epoch) && epoch == preparingEpoch && Table(preparingUI, gameModule, uiTableRva))
        {
            auto owner = std::make_shared<Context>();
            auto& pool = owner->native;
            pool.ui = preparingUI; pool.script = bridgeScript;
            pool.source = NativeParameter(bridgeScript, handler, 2, scriptModule);
            pool.page = NativeParameter(bridgeScript, handler, 3, scriptModule);
            pool.entry = NativeParameter(bridgeScript, handler, 4, scriptModule);
            pool.heading = NativeParameter(bridgeScript, handler, 5, scriptModule);
            pool.description = NativeParameter(bridgeScript, handler, 6, scriptModule);
            pool.prototype = NativeParameter(bridgeScript, handler, 7, scriptModule);
            for (size_t i = 0; i < pool.rows.size(); ++i)
                pool.rows[i] = NativeParameter(bridgeScript, handler, static_cast<int>(8 + i), scriptModule);
            const auto prototype = Field<NativeRect>(pool.prototype, widgetRectOffset);
            pool.entryBounds = {prototype.x, prototype.y + prototype.height, prototype.width, prototype.height};
            owner->epoch = owner->serial = epoch;
            owner->lifetime = preparingLifetime;
            if (preparingLifetime == uiLifetime && !releasing && pool.Live(gameModule, scriptModule) &&
                prototype.width >= 64 && prototype.height >= 16 && pool.entryBounds.y + pool.entryBounds.height <= 580)
            {
                std::set<void*> unique{pool.source, pool.page, pool.entry, pool.heading, pool.description, pool.prototype};
                unique.insert(pool.rows.begin(), pool.rows.end());
                auto system = Field<void*>(pool.ui, uiSystemOffset);
                auto font = Table(system, systemModule, systemTableRva) ? Field<void*>(system, systemFontOffset) : nullptr;
                auto texts = FixSettings().GetTexts(); texts.Select("en");
                const auto title = texts.Resolve("@menu.title");
                auto extent = MeasureLiteral(font, fontModule, title, 14);
                const auto scaleY = Field<double>(pool.ui, verticalScaleOffset);
                accepted = unique.size() == 13 && extent && extent->x <= prototype.width - 16 &&
                    extent->y <= prototype.height - 4 && std::isfinite(scaleY) && scaleY > 0 && extent->y * scaleY >= 8;
                if (accepted) context = owner;
            }
        }
        if (!accepted) Log("native source/pool/font/geometry binding refused");
        return Return(handler, accepted);
    }
    inline int QueueCallbackBody(void* state)
    {
        Depth scope(nativeDepth);
        auto handler = CallbackHandler(state);
        if (!handler) return 0;
        Pending action;
        bool accepted = ParamCount(handler) == 5 && String(handler, 1, action.serial) &&
            Integer(handler, 2, action.kind) && Integer(handler, 3, action.index) && Integer(handler, 4, action.key);
        if (accepted)
        {
            action.sender = NativeParameter(bridgeScript, handler, 5, scriptModule);
            accepted = action.kind >= 0 && action.kind <= 5 && Queue(std::move(action));
        }
        return Return(handler, accepted);
    }
    inline void CallbackFailure(const char* phase) noexcept
    {
        try
        {
            if (context) { context->returnPending = true; Retire(context); }
            pending.clear();
            Log(phase);
        }
        catch (...) {} // Only logical cancellation; native retirement stays deferred.
    }
    inline int __cdecl BindCallback(void* state) noexcept
    {
        try { return BindCallbackBody(state); }
        catch (...) { CallbackFailure("native Bind callback exception refused"); return 0; }
    }
    inline int __cdecl QueueCallback(void* state) noexcept
    {
        try { return QueueCallbackBody(state); }
        catch (...) { CallbackFailure("native Queue callback exception refused"); return 0; }
    }
    inline bool Bootstrap(void* ui)
    {
        if (preparing || releasing || scriptClosing || fontClosing || !ready ||
            !Table(ui, gameModule, uiTableRva) || !Readable(ui, uiBytes) ||
            !(Field<unsigned char>(ui, uiFlagsOffset) & 4)) return false;
        auto script = Field<void*>(ui, uiScriptOffset);
        if (!Table(script, scriptModule, scriptTableRva)) return false;
        if (!bridgeScript)
        {
            if (!NamespaceFree(script, scriptModule)) { Log("reserved Lua namespace occupied"); return false; }
            bridgeScript = script;
            bridgeState = State(script);
            if (!bridgeState || !Execute(script, nativeMenuLua)) { Log("native Lua definitions refused"); return false; }
            auto object = CreateGlobal(script, "WFP_Bridge");
            if (!Table(object, scriptModule, objectTableRva)) return false;
            const auto installed = AddFunction(object, "Bind", BindCallback, 1) &&
                AddFunction(object, "Queue", QueueCallback, 2);
            ReleaseObject(object);
            if (!installed) { Log("native retained Lua closures refused"); return false; }
            bridgeReady = true;
        }
        if (!bridgeReady || script != bridgeScript || State(script) != bridgeState) return false;
        preparingEpoch = Token();
        if (preparingEpoch.empty()) { Log("native generation exhausted"); return false; }
        preparingUI = frameUI = ui;
        preparingLifetime = uiLifetime;
        struct Preparing { Preparing() { preparing = true; } ~Preparing() { preparing = false; preparingUI = nullptr; } } guard;
        auto texts = FixSettings().GetTexts(); texts.Select("en");
        auto title = NativeLiteral(texts.Resolve("@menu.title"));
        if (!title) return false;
        {
            Depth depth(nativeDepth);
            if (!Execute(script, "WFP_Prepare(" + Quote(preparingEpoch) + "," + Quote(*title) + ",14);"))
            { Log("native Lua factory buffer refused"); return false; }
        }
        std::string status;
        const bool statusRead = Global(script, "g_WFPMenuStatus", status);
        const bool accepted = context && context->epoch == preparingEpoch && Available(context) &&
            statusRead && status == "ready";
        if (!accepted)
        {
            Log("native prepare refused: " + status);
            // A failed partial pool stays engine-owned and hidden until the
            // normal native Release. Never repeat a colliding factory round.
            if (status.rfind("source-not-ready", 0) != 0 && !status.empty()) blockedLifetime = uiLifetime;
            { Depth depth(nativeDepth); Execute(script, "WFP_Retire(" + Quote(preparingEpoch) + ");"); }
            if (context && context->epoch == preparingEpoch) { Retire(context); context.reset(); }
        }
        return accepted;
    }
    inline void FrameBody(void* ui)
    {
        if (!ready || nativeDepth || foreignDepth || draining || preparing || releasing || scriptClosing || fontClosing) return;
        frameUI = ui;
        if (!context)
        {
            if (blockedLifetime && *blockedLifetime == uiLifetime) return;
            if (preparationDelay) { --preparationDelay; return; }
            if (!Bootstrap(ui)) preparationDelay = 60;
            return;
        }
        auto before = context;
        Drain();
        if (before && !context) blockedLifetime = before->lifetime;
    }
    inline void Frame(void* ui) noexcept
    {
        try { FrameBody(ui); }
        catch (...)
        {
            CallbackFailure("native post-update menu exception refused");
            if (!context) blockedLifetime = uiLifetime;
        }
    }
    inline void ForeignTransition()
    {
        auto owner = context;
        if (!foreignDepth || !owner || owner->retired || !owner->model.IsOpen()) return;
        const auto still = NativeCurrent(owner) && !owner->native.Modal() &&
            (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4) &&
            (owner->open ? owner->native.Selected(owner->selected) &&
                Field<unsigned char>(owner->native.page, screenActiveOffset) != 0 :
                Field<void*>(owner->native.ui, focusOffset) == owner->native.entry &&
                Field<unsigned char>(owner->native.source, screenActiveOffset) != 0);
        if (!still) { owner->returnPending = true; Retire(owner); }
    }
    inline bool PrivateSwitchOwner(void* receiver, bool in)
    {
        auto owner = context;
        return !in && NativeCurrent(owner) && (owner->open || owner->returnPending) && !owner->native.Modal() &&
            Field<void*>(owner->native.ui, uiScriptObjectOffset) == receiver &&
            Field<void*>(receiver, scriptOwnerUiOffset) == owner->native.ui &&
            Field<void*>(receiver, scriptOwnerServiceOffset) == owner->native.script &&
            (Field<unsigned char>(owner->native.ui, uiFlagsOffset) & 4) &&
            Field<unsigned char>(owner->native.page, screenActiveOffset) != 0 &&
            std::find(owner->native.rows.begin(), owner->native.rows.end(),
                Field<void*>(owner->native.ui, focusOffset)) != owner->native.rows.end();
    }
#ifdef _WIN64
    inline int CanSwitch(void* receiver, bool in)
    { Depth scope(nativeDepth); return PrivateSwitchOwner(receiver, in) ? 0 : switchHook.call<int>(receiver, in); }
    inline uintptr_t Updated(void* ui) { const auto lifetime = uiLifetime; uintptr_t result; { Depth scope(nativeDepth); result = updateHook.call<uintptr_t>(ui); } if (lifetime == uiLifetime) Frame(ui); return result; }
    inline uintptr_t Drawn(void* ui) { Depth scope(nativeDepth); return drawHook.call<uintptr_t>(ui); }
    inline char Input(void* ui, void* event) { Depth scope(nativeDepth); return inputHook.call<char>(ui, event); }
    inline intptr_t Broker(void* ui, void* widget, unsigned message, uintptr_t wParam, intptr_t lParam)
    { Depth scope(nativeDepth); ForeignTransition(); auto result = brokerHook.call<intptr_t>(ui, widget, message, wParam, lParam); ForeignTransition(); return result; }
    inline int Released(void* ui)
    {
        Depth scope(nativeDepth); Depth phase(releasing); Invalidate();
        const auto result = releaseHook.call<int>(ui);
        preparationDelay = 0; return result;
    }
    inline uintptr_t Destroyed(void* ui, uintptr_t allocatorContext) { Depth scope(nativeDepth); Invalidate(); return destroyHook.call<uintptr_t>(ui, allocatorContext); }
    inline uintptr_t ScriptReleased(void* script)
    { Depth scope(nativeDepth); Depth phase(scriptClosing); Invalidate(); const auto result = scriptReleaseHook.call<uintptr_t>(script); bridgeScript = bridgeState = nullptr; bridgeReady = false; return result; }
    inline uintptr_t FontReleased(void* font)
    { Depth scope(nativeDepth); Depth phase(fontClosing); Invalidate(); return fontReleaseHook.call<uintptr_t>(font); }
#else
    inline int __fastcall CanSwitch(void* receiver, void*, bool in)
    { Depth scope(nativeDepth); return PrivateSwitchOwner(receiver, in) ? 0 : switchHook.thiscall<int>(receiver, in); }
    inline uintptr_t __fastcall Updated(void* ui, void*) { const auto lifetime = uiLifetime; uintptr_t result; { Depth scope(nativeDepth); result = updateHook.thiscall<uintptr_t>(ui); } if (lifetime == uiLifetime) Frame(ui); return result; }
    inline uintptr_t __fastcall Drawn(void* ui, void*) { Depth scope(nativeDepth); return drawHook.thiscall<uintptr_t>(ui); }
    inline char __fastcall Input(void* ui, void*, void* event) { Depth scope(nativeDepth); return inputHook.thiscall<char>(ui, event); }
    inline intptr_t __fastcall Broker(void* ui, void*, void* widget, unsigned message, uintptr_t wParam, intptr_t lParam)
    { Depth scope(nativeDepth); ForeignTransition(); auto result = brokerHook.thiscall<intptr_t>(ui, widget, message, wParam, lParam); ForeignTransition(); return result; }
    inline int __fastcall Released(void* ui, void*)
    { Depth scope(nativeDepth); Depth phase(releasing); Invalidate(); const auto result = releaseHook.thiscall<int>(ui); preparationDelay = 0; return result; }
    inline uintptr_t __fastcall Destroyed(void* ui, void*) { Depth scope(nativeDepth); Invalidate(); return destroyHook.thiscall<uintptr_t>(ui); }
    inline uintptr_t __fastcall ScriptReleased(void* script, void*)
    { Depth scope(nativeDepth); Depth phase(scriptClosing); Invalidate(); const auto result = scriptReleaseHook.thiscall<uintptr_t>(script); bridgeScript = bridgeState = nullptr; bridgeReady = false; return result; }
    inline uintptr_t __fastcall FontReleased(void* font, void*)
    { Depth scope(nativeDepth); Depth phase(fontClosing); Invalidate(); return fontReleaseHook.thiscall<uintptr_t>(font); }
#endif
    inline void Init()
    {
        if (ready) return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule);
        gameModule = GetModuleHandleW(L"CryGame.dll"); scriptModule = GetModuleHandleW(L"CryScriptSystem.dll");
        fontModule = GetModuleHandleW(L"CryFont.dll"); systemModule = GetModuleHandleW(L"CrySystem.dll");
        if (!gameModule || !scriptModule || !fontModule || !systemModule || !GetModuleHandleW(L"CryInput.dll")) return;
        if (!SupportedCode()) { Log("native per-architecture code contracts refused"); return; }
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &hostModule)) { Log("process lifetime pin refused"); return; }
#ifdef _WIN64
        constexpr uintptr_t update = 0x58EF0, draw = 0x5A070, input = 0x5FFA0, broker = 0x5B8E0,
            release = 0x5A740, destroy = 0x58440, scriptRelease = 0x11730, fontRelease = 0x38120, canSwitch = 0x5460;
#else
        constexpr uintptr_t update = 0x34134, draw = 0x316A5, input = 0x30DAB, broker = 0x2FF62,
            release = 0x313AF, destroy = 0x31124, scriptRelease = 0x511F, fontRelease = 0x10A9, canSwitch = 0x5DA0;
#endif
        auto address = [](HMODULE module, uintptr_t rva) { return reinterpret_cast<unsigned char*>(module) + rva; };
        updateHook = safetyhook::create_inline(address(gameModule, update), &Updated);
        drawHook = safetyhook::create_inline(address(gameModule, draw), &Drawn);
        inputHook = safetyhook::create_inline(address(gameModule, input), &Input);
        brokerHook = safetyhook::create_inline(address(gameModule, broker), &Broker);
        releaseHook = safetyhook::create_inline(address(gameModule, release), &Released);
        destroyHook = safetyhook::create_inline(address(gameModule, destroy), &Destroyed);
        scriptReleaseHook = safetyhook::create_inline(address(scriptModule, scriptRelease), &ScriptReleased);
        fontReleaseHook = safetyhook::create_inline(address(fontModule, fontRelease), &FontReleased);
        switchHook = safetyhook::create_inline(address(gameModule, canSwitch), &CanSwitch);
        if (!updateHook || !drawHook || !inputHook || !brokerHook || !releaseHook || !destroyHook || !scriptReleaseHook || !fontReleaseHook || !switchHook)
        {
            switchHook.reset(); fontReleaseHook.reset(); scriptReleaseHook.reset(); destroyHook.reset(); releaseHook.reset();
            brokerHook.reset(); inputHook.reset(); drawHook.reset(); updateHook.reset();
            Log("native hook transaction refused"); return;
        }
        ready = true;
    }
}
