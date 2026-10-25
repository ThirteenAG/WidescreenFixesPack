#pragma once

#include "FixSettings.hpp"
#include "NativeMenuUnwind.hpp"
#include <map>
#include <intrin.h>

namespace THPS2Menu
{
    // Native Options and pause hosts for the guarded installed executable.
    struct TitleRecord
    {
        TitleContext context;
        std::shared_ptr<Host> host;
        unsigned deleteFlags = 0;
    };
    struct PauseRecord
    {
        PauseContext context;
        std::shared_ptr<Host> host;
        bool deletionRequested = false;
        unsigned deleteFlags = 0;
    };
    inline HMODULE ownModule = nullptr;
    inline std::map<void*, std::shared_ptr<TitleRecord>> titles;
    inline std::map<void*, std::shared_ptr<PauseRecord>> pauses;
    inline std::vector<std::shared_ptr<TitleRecord>> retiredTitles;
    inline std::shared_ptr<PauseRecord> painting;
    inline SafetyHookInline titleCtorHook, titleDeleteHook, titleInputHook, titleUpdateHook;
    inline SafetyHookInline pauseCenterHook, pauseDeleteHook, pauseInputHook, pauseDrawHook, pauseReadHook, fontDrawHook, pauseDispatchHook;

    inline std::string Caption()
    {
        // No independently proved native locale getter: honest English fallback.
        return PlainText(RWGTAMenu::PluginText(FixSettings(), MenuLanguage(), "menu.plugins"));
    }

    inline std::shared_ptr<TitleRecord> Title(void* owner)
    {
        auto found = titles.find(owner);
        return found == titles.end() ? nullptr : found->second;
    }

    inline std::shared_ptr<PauseRecord> Pause(void* owner)
    {
        auto found = pauses.find(owner);
        return found == pauses.end() ? nullptr : found->second;
    }

    inline bool Pressed(uintptr_t a, uintptr_t b)
    {
        return *reinterpret_cast<const uint8_t*>(a) || *reinterpret_cast<const uint8_t*>(b);
    }

    inline void Consume(uintptr_t a, uintptr_t b)
    {
        NativeWrite(reinterpret_cast<uint8_t*>(a), uint8_t(0));
        NativeWrite(reinterpret_cast<uint8_t*>(b), uint8_t(0));
    }

    inline void* __fastcall TitleConstruct(void* owner, void*)
    {
        auto result = titleCtorHook.unsafe_thiscall<void*>(owner);
        if (!nativeReady || result != owner || Unwind::modalDepth != 1 || Unwind::worldDepth || titles.count(owner))
            return result;
        try
        {
            auto record = std::make_shared<TitleRecord>();
            auto host = std::make_shared<Host>(Backend::Title);
            record->host = host;
            host->owner = owner;
            host->module = ownModule;
            std::weak_ptr<TitleRecord> weak = record;
            host->nativeCurrent = [weak](const Host& value)
            {
                auto kept = weak.lock();
                return kept && Unwind::modalDepth == 1 && !Unwind::worldDepth && kept->context.Eligible(value.open && !value.entering);
            };
            host->enterNative = [weak](Host&) { auto kept = weak.lock(); return kept && kept->context.Enter(); };
            host->leaveNative = [weak](Host&) { if (auto kept = weak.lock()) kept->context.Leave(); };
            // Register C++ lifetime before publishing any engine-owned row.
            titles.emplace(owner, record);
            if (!record->context.Append(owner, Caption()))
            {
                titles.erase(owner);
                Diagnose(Diagnostic::TitleContext, "title category construction refused");
                return result;
            }
            host->titleStyle = record->context.style;
        }
        catch (...)
        {
            Diagnose(Diagnostic::Rollback, "title category allocation failed");
        }
        return result;
    }

    inline void* __fastcall TitleDelete(void* owner, void*, unsigned flags)
    {
        // Saved481959 CALL[eax] returns48195B, before all44B040 font unloads.
        // SafetyHook's entry JMP retains that original caller return address.
        const bool nativeFinal = reinterpret_cast<uintptr_t>(_ReturnAddress()) == 0x48195B && flags == 1 && Unwind::modalDepth == 1 && Unwind::OutsideNativeRows();
        auto record = Title(owner);
        if (record)
        {
            record->host->Retire();
            if (Unwind::modalDepth && !nativeFinal)
            {
                // A local owner pointer remains in4817E0 across its loop. Keep
                // native allocation intact until the next input returns exit;
                // normal native480160/save/deleting-dtor then owns teardown.
                record->context.deletionRequested = true;
                record->deleteFlags |= flags;
                Diagnose(Diagnostic::OwnerRetired, "title deletion deferred to native modal exit");
                return owner;
            }
            record->host->Close();
            titles.erase(owner);
        }
        return titleDeleteHook.unsafe_thiscall<void*>(owner, flags);
    }

    inline int __fastcall TitleInput(void* owner, void*)
    {
        Unwind::Depth input(Unwind::inputDepth);
        auto record = Title(owner);
        if (!record)
            return titleInputHook.unsafe_thiscall<int>(owner);
        auto host = record->host;
        if (record->context.deletionRequested || !host->valid)
        {
            record->context.exiting = true;
            return 1;
        }
        const bool choose = Pressed(0x567C89, 0x567DF5);
        const bool back = Pressed(0x567C59, 0x567DC5);
        if (host->open && record->context.Eligible(true))
        {
            if (choose && Queue(host, host->Selected(), 0))
                Consume(0x567C89, 0x567DF5);
            else if (back && Queue(host, View::PhysicalBack, 0))
                Consume(0x567C59, 0x567DC5);
            return 0;
        }
        if (record->context.Eligible(false) && record->context.Selected() == CategoryCommand && choose)
        {
            Queue(host, CategoryCommand, 0, true);
            // Only our otherwise-unknown command is consumed. Every unknown
            // stock command and all stock modal/save behavior stay original.
            Consume(0x567C89, 0x567DF5);
            return 0;
        }
        const int result = titleInputHook.unsafe_thiscall<int>(owner);
        if (result)
            record->context.exiting = true;
        return result;
    }

    inline int __fastcall TitleUpdate(void* owner, void*)
    {
        Unwind::Depth update(Unwind::updateDepth);
        auto record = Title(owner);
        if (!record || !record->host->open || !record->context.Eligible(true))
            return titleUpdateHook.unsafe_thiscall<int>(owner);
        auto host = record->host;
        const int direction = FunctionAt<int (__cdecl*)()>(0x46F5E0)();
        if (direction == 0 || direction == 1)
            Queue(host, host->Selected(), direction == 0 ? -1 : 1);
        else if (host->title)
            host->title->Input(direction);
        if (host->title)
            host->title->Draw();
        // Preserve native base UI animation; no private state reaches the
        // stock options switch's unsupported-command assertion.
        return FunctionAt<int (__thiscall*)(void*)>(0x4AA010)(owner);
    }

    inline int __fastcall PauseCenter(Native::PauseMenu* owner, void*)
    {
        if (nativeReady && Unwind::worldDepth == 1 && !pauses.count(owner) && Contracts::Readable(reinterpret_cast<const void*>(0x55E8A0), 4) && *reinterpret_cast<Native::PauseMenu**>(0x55E8A0) == owner)
        {
            try
            {
                auto record = std::make_shared<PauseRecord>();
                auto host = std::make_shared<Host>(Backend::Pause);
                record->host = host;
                host->owner = owner;
                host->module = ownModule;
                host->pauseSource = reinterpret_cast<Native::Font*>(0x567600);
                std::weak_ptr<PauseRecord> weak = record;
                host->nativeCurrent = [weak](const Host&)
                {
                    auto kept = weak.lock();
                    return kept && !kept->deletionRequested && kept->context.Eligible();
                };
                host->enterNative = [weak](Host&)
                {
                    auto kept = weak.lock();
                    return kept && kept->context.Eligible() && kept->context.base->selected == kept->context.entry;
                };
                host->leaveNative = [weak](Host&)
                {
                    if (auto kept = weak.lock(); kept && kept->context.Eligible())
                        FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(0x44B710)(kept->context.base, static_cast<char>(kept->context.entry));
                };
                pauses.emplace(owner, record);
                if (!record->context.Append(owner, Caption()))
                {
                    pauses.erase(owner);
                    Diagnose(Diagnostic::PauseContext, "pause category construction refused");
                }
            }
            catch (...)
            {
                Diagnose(Diagnostic::Rollback, "pause category allocation failed");
            }
        }
        return pauseCenterHook.unsafe_thiscall<int>(owner);
    }

    inline void* __fastcall PauseDelete(Native::PauseMenu* owner, void*, unsigned flags)
    {
        auto record = Pause(owner);
        if (record)
        {
            record->host->Retire();
            if (Unwind::worldDepth || !Unwind::OutsideNativeRows() || Unwind::servicing)
            {
                record->deletionRequested = true;
                record->deleteFlags |= flags;
                return owner;
            }
            record->host->Close();
            pauses.erase(owner);
        }
        return pauseDeleteHook.unsafe_thiscall<void*>(owner, flags);
    }

    inline char __fastcall PauseInput(Native::PauseMenu* owner, void*)
    {
        Unwind::Depth input(Unwind::inputDepth);
        auto record = Pause(owner);
        if (Unwind::pauseDispatchDepth != 1 || !record || !record->host->open || !record->host->valid || !record->context.Eligible() || !record->host->pause)
            return pauseInputHook.unsafe_thiscall<char>(owner);
        auto host = record->host;
        auto result = pauseInputHook.unsafe_thiscall<char>(host->pause->Get());
        const int direction = FunctionAt<int (__cdecl*)()>(0x46F5E0)();
        if (direction == 0 || direction == 1)
            Queue(host, host->Selected(), direction == 0 ? -1 : 1);
        return result;
    }

    inline char __fastcall PauseDraw(Native::PauseMenu* owner, void*)
    {
        Unwind::Depth draw(Unwind::drawDepth);
        auto record = Pause(owner);
        if (Unwind::worldDepth != 1 || Unwind::modalDepth || !record || !record->host->open || !record->host->valid || !record->context.Eligible() || !record->host->pause)
            return pauseDrawHook.unsafe_thiscall<char>(owner);
        struct Scope
        {
            std::shared_ptr<PauseRecord> previous;
            explicit Scope(std::shared_ptr<PauseRecord> current) : previous(painting) { painting = std::move(current); }
            ~Scope() { painting = previous; }
        } scope(record);
        return pauseDrawHook.unsafe_thiscall<char>(record->host->pause->Get());
    }

    inline char __fastcall FontDraw(Native::Font* source, void*, int x, int y, const char* text, int mode)
    {
        auto record = painting;
        if (record && source == reinterpret_cast<Native::Font*>(0x567600))
        {
            Native::Font copy;
            if (!record->host->valid || !record->context.Eligible() || !record->host->pauseFont.DrawCopy(*source, copy))
            {
                record->host->Retire();
                Diagnose(Diagnostic::PauseLayout, "private pause font identity changed");
                return 0;
            }
            return fontDrawHook.unsafe_thiscall<char>(&copy, x, y, text, mode);
        }
        return fontDrawHook.unsafe_thiscall<char>(source, x, y, text, mode);
    }

    inline int __cdecl ReadPauseInput(int* choose, int* back, int* any, int* start)
    {
        const int result = pauseReadHook.unsafe_ccall<int>(choose, back, any, start);
        if (Unwind::worldDepth != 1 || Unwind::pauseDispatchDepth != 1)
            return result;
        auto root = *reinterpret_cast<Native::PauseMenu**>(0x55E8A0);
        auto record = Pause(root);
        if (!record || !record->host->valid || !record->context.Eligible())
            return result;
        auto host = record->host;
        bool consumed = false;
        if (host->open)
        {
            if (*choose)
                consumed = Queue(host, host->Selected(), 0);
            else if (*back)
                consumed = Queue(host, View::PhysicalBack, 0);
        }
        else if (*choose && root->selected == record->context.entry)
            consumed = Queue(host, CategoryCommand, 0, true);
        if (!consumed)
            return result;
        NativeWrite(choose, 0);
        NativeWrite(back, 0);
        NativeWrite(any, 0);
        NativeWrite(start, 0);
        return 0;
    }

    inline int __cdecl DispatchPause()
    {
        Unwind::Depth scope(Unwind::pauseDispatchDepth);
        return pauseDispatchHook.unsafe_ccall<int>();
    }

    inline void ServiceTitle()
    {
        Drain(Backend::Title);
        std::vector<std::shared_ptr<TitleRecord>> snapshot;
        for (const auto& [owner, record] : titles)
            snapshot.push_back(record);
        const bool changed = RWGTAMenu::Model::Changed().exchange(false);
        for (const auto& record : snapshot)
        {
            auto host = record->host;
            if (host->closeRequested)
                host->Close();
            else if (host->open && host->valid)
            {
                if (!Host::Snapshot(host).Current())
                    host->Retire();
                else if (changed || host->Selected() != host->helpSelected)
                    host->Refresh(host->Selected());
            }
        }
    }

    inline void ServiceWorld()
    {
        Drain(Backend::Pause);
        std::vector<std::shared_ptr<PauseRecord>> snapshot;
        for (const auto& [owner, record] : pauses)
            snapshot.push_back(record);
        const bool changed = RWGTAMenu::Model::Changed().exchange(false);
        for (const auto& record : snapshot)
        {
            auto host = record->host;
            if (host->open && host->valid && !Host::Snapshot(host).Current())
                host->Retire();
            if (host->closeRequested)
                host->Close();
            else if ((changed || host->Selected() != host->helpSelected) && host->open && host->valid)
                host->Refresh(host->Selected());
            if (record->deletionRequested)
            {
                // Original native input/draw/HUD callers have all returned.
                // Retained record still keeps the appended label alive until
                // the original fixed-menu destructor has completed.
                pauses.erase(host->owner);
                pauseDeleteHook.unsafe_thiscall<void*>(host->owner, record->deleteFlags);
            }
            else if (!host->valid && record->context.Alive() && record->context.entry < record->context.base->count)
            {
                auto root = record->context.base;
                NativeWrite(&root->rows[record->context.entry].enabled, uint8_t(0));
                if (root->selected == record->context.entry)
                {
                    for (uint8_t i = 0; i < root->count; ++i)
                    {
                        if (root->rows[i].enabled && root->rows[i].visible)
                        {
                            FunctionAt<void* (__thiscall*)(Native::PauseMenu*, char)>(0x44B710)(root, static_cast<char>(i));
                            break;
                        }
                    }
                }
            }
        }
    }

    inline void ModalFinished()
    {
        auto snapshot = std::move(titles);
        titles.clear();
        for (const auto& [owner, record] : snapshot)
        {
            record->host->Retire();
            record->host->Close();
            if (record->context.deletionRequested)
            {
                // The supported final callsite already deletes before font
                // unload. Do not reorder an unrecognized escape path's native
                // destruction after its borrowed font resources were released.
                retiredTitles.push_back(record);
                Diagnose(Diagnostic::OwnerRetired, "unexpected modal escape; native storage retained without late destruction");
            }
        }
    }

    inline void ResetHooks()
    {
        Unwind::Reset();
        pauseDispatchHook.reset();
        fontDrawHook.reset();
        pauseReadHook.reset();
        pauseDrawHook.reset();
        pauseInputHook.reset();
        pauseDeleteHook.reset();
        pauseCenterHook.reset();
        titleUpdateHook.reset();
        titleInputHook.reset();
        titleDeleteHook.reset();
        titleCtorHook.reset();
    }

    inline bool Init()
    {
        if (nativeReady)
            return true;
        uintptr_t refused = 0;
        if (!Contracts::Matches(&refused))
        {
            char detail[96] = {};
            if (refused)
                std::snprintf(detail, sizeof(detail), "unsupported native prefix %08lX", static_cast<unsigned long>(refused));
            else
                std::snprintf(detail, sizeof(detail), "unsupported native image/header/layout");
            Diagnose(Diagnostic::UnsupportedImage, detail);
            return false;
        }
        // Native borrowed strings and callbacks require process lifetime.
        // Pin only this exact supported executable bridge, before any hook or
        // retained native pointer. Dynamic host unload is not supported.
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN, reinterpret_cast<LPCWSTR>(&Init), &ownModule))
        {
            Diagnose(Diagnostic::PinFailed);
            return false;
        }
        Unwind::titleFrame = ServiceTitle;
        Unwind::worldFrame = ServiceWorld;
        Unwind::modalFinished = ModalFinished;
        auto attach = [](SafetyHookInline& hook, uintptr_t address, void* callback)
        {
            hook = safetyhook::create_inline(reinterpret_cast<void*>(address), callback);
            return static_cast<bool>(hook);
        };
        if (!attach(titleCtorHook, 0x47EB00, reinterpret_cast<void*>(&TitleConstruct)) ||
            !attach(titleDeleteHook, 0x47FD50, reinterpret_cast<void*>(&TitleDelete)) ||
            !attach(titleInputHook, 0x480750, reinterpret_cast<void*>(&TitleInput)) ||
            !attach(titleUpdateHook, 0x480330, reinterpret_cast<void*>(&TitleUpdate)) ||
            !attach(pauseCenterHook, 0x44B6C0, reinterpret_cast<void*>(&PauseCenter)) ||
            !attach(pauseDeleteHook, 0x44B690, reinterpret_cast<void*>(&PauseDelete)) ||
            !attach(pauseInputHook, 0x44BB60, reinterpret_cast<void*>(&PauseInput)) ||
            !attach(pauseDrawHook, 0x44BA30, reinterpret_cast<void*>(&PauseDraw)) ||
            !attach(pauseReadHook, 0x44DD30, reinterpret_cast<void*>(&ReadPauseInput)) ||
            !attach(fontDrawHook, 0x44A010, reinterpret_cast<void*>(&FontDraw)) ||
            !attach(pauseDispatchHook, 0x44EA50, reinterpret_cast<void*>(&DispatchPause)) || !Unwind::Install())
        {
            ResetHooks();
            Diagnose(Diagnostic::HookFailed);
            return false;
        }
        nativeReady = true;
        return true;
    }
}
