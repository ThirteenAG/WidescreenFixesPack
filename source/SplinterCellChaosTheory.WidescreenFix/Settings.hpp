#pragma once
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <sstream>
#include <set>
#include "gameref.hpp"
#include "safetyhook.hpp"
#include "Hooking.Patterns.h"

namespace CTSettings
{
    template<class T> T Read(void* object, size_t offset)
    {
        return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
    }
    template<class T> void Set(void* object, size_t offset, T value)
    {
        ProtectedGameRef<T>{reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset)} = value;
    }

    inline void* (__fastcall* makeItem)(void*, void*, int) = nullptr;
    inline int (__fastcall* bindStyle)(void*, void*, void*, void*) = nullptr;
    inline unsigned (__fastcall* addChild)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* findChild)(void*, void*, uint32_t) = nullptr;
    inline int (__fastcall* setText)(void*, void*, const wchar_t*) = nullptr;
    inline void (__fastcall* addCallback)(void*, void*, void*) = nullptr;
    inline int (__fastcall* setFocus)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* getStack)(void*, void*) = nullptr;
    inline std::wstring (*caption)(void*, double) = nullptr;
    inline bool (*stockFits)(void*, double) = nullptr;
    inline bool (*current)(void*, void*) = nullptr;
    inline void (*open)(void*, void*) = nullptr;
    inline void (*log)(const std::string&) = nullptr;
    inline void (*retire)(void*) = nullptr;
    inline void (*controllerRetired)(void*) = nullptr;
    inline void (*frame)(void*) = nullptr;
    inline SafetyHookInline initHook, destructorHook, tickHook;
    inline bool ready = false;
    inline unsigned updateDepth = 0;
    inline bool servicing = false;
    inline uint64_t nextSerial = 0;
    inline uintptr_t pointType = 0;
    inline constexpr uint32_t PluginID = 0xC05DD40B;
    inline constexpr uint32_t SelectedCategoryID = 0x5CAD3B90;
    inline constexpr std::array<uint32_t, 3> CategoryIDs{ 0xC46B4A04, 0xB297004C, 0x9C4A21C3 };

    struct Binding
    {
        void* controller = nullptr, * page = nullptr, * manager = nullptr, * body = nullptr;
        std::array<void*, 4> tabs{};
        uint64_t serial = 0;
        std::wstring text;
    };
    inline std::map<void*, Binding> bindings;
    inline std::map<void*, uint64_t> stackTokens;
    struct Action
    {
        void* page = nullptr, * item = nullptr;
        uint64_t serial = 0;
        int direction = 0;
    };
    inline std::deque<Action> pending;

    inline int Kind(void* item)
    {
        if (!item || !Read<void*>(item, 8))
            return -1;
        auto body = Read<void*>(item, 8);
        return reinterpret_cast<int (__fastcall*)(void*, void*)>(Read<void**>(body, 0)[3])(body, nullptr);
    }
    inline bool Child(void* page, void* item)
    {
        if (!page || !item)
            return false;
        const auto begin = Read<uintptr_t>(page, 8), end = Read<uintptr_t>(page, 12);
        if (!begin || end < begin || end - begin > 4 * 128 || (end - begin) % 4)
            return false;
        for (auto at = begin; at != end; at += 4)
            if (*reinterpret_cast<void**>(at) == item)
                return true;
        return false;
    }
    inline bool Live(const Binding& binding)
    {
        return binding.controller && Read<void*>(binding.controller, 56) == binding.page &&
            Read<void*>(binding.controller, 20) == binding.manager && Child(binding.page, binding.tabs[3]) &&
            Read<uint32_t>(binding.tabs[3], 4) == PluginID;
    }
    inline Binding* Lookup(void* page, uint64_t serial)
    {
        const auto found = bindings.find(page);
        return found != bindings.end() && found->second.serial == serial && Live(found->second) ? &found->second : nullptr;
    }
    inline void Queue(void* page, uint64_t serial, void* item, int direction)
    {
        auto binding = Lookup(page, serial);
        if (!binding || !current || !current(page, binding->manager) || pending.size() >= 16)
            return;
        pending.push_back({ page, item, serial, direction });
    }

    struct Callback
    {
        void** vtable = nullptr;
        uint32_t references = 1;
        uint8_t enabled = 1;
        uint8_t padding[3]{};
        void* page = nullptr;
        uint64_t serial = 0;

        static void* __fastcall Destroy(Callback* self, void*, char flags)
        {
            if (flags & 1)
                delete self;
            return self;
        }
        static int __fastcall Type(Callback*, void*) { return 0; }
        static char __fastcall Notify(Callback*, void*, void*) { return 0; }
        static char __fastcall Event(Callback*, void*, void*, void*) { return 0; }
        static char __fastcall Accept(Callback* self, void*, void* item)
        {
            auto binding = Lookup(self->page, self->serial);
            if (!binding || binding->tabs[3] != item)
                return 0;
            if (log) log("Settings Plugins native accept received");
            Queue(self->page, self->serial, item, 0);
            return 1;
        }
        static char __fastcall Key(Callback* self, void*, void* item, uint32_t* event)
        {
            auto binding = Lookup(self->page, self->serial);
            if (!binding || !event || std::find(binding->tabs.begin(), binding->tabs.end(), item) == binding->tabs.end())
                return 0;
            const auto key = *event;
            if (item == binding->tabs[3] && (key == VK_UP || key == VK_DOWN))
            {
                Queue(self->page, self->serial, item, key == VK_DOWN ? 2 : 3);
                return 1;
            }
            if (key != VK_LEFT && key != VK_RIGHT && key != VK_TAB)
                return 0;
            Queue(self->page, self->serial, item, key == VK_LEFT || (key == VK_TAB && (GetKeyState(VK_SHIFT) & 0x8000)) ? -1 : 1);
            return 1;
        }
    };
    inline std::array<void*, 14> callbackVtable
    {
        reinterpret_cast<void*>(&Callback::Destroy), reinterpret_cast<void*>(&Callback::Type),
        reinterpret_cast<void*>(&Callback::Notify), reinterpret_cast<void*>(&Callback::Notify),
        reinterpret_cast<void*>(&Callback::Accept), reinterpret_cast<void*>(&Callback::Notify),
        reinterpret_cast<void*>(&Callback::Event), reinterpret_cast<void*>(&Callback::Key),
        reinterpret_cast<void*>(&Callback::Event), reinterpret_cast<void*>(&Callback::Event), reinterpret_cast<void*>(&Callback::Event),
        reinterpret_cast<void*>(&Callback::Event), reinterpret_cast<void*>(&Callback::Notify), reinterpret_cast<void*>(&Callback::Notify)
    };

    inline void Destroy(void* item)
    {
        if (item)
            reinterpret_cast<void* (__fastcall*)(void*, void*, char)>(Read<void**>(item, 0)[0])(item, nullptr, 1);
    }
    inline bool Position(void* body, uint32_t& position)
    {
        if (!body)
            return false;
        auto point = Read<void*>(body, 4);
        if (point != static_cast<uint8_t*>(body) + 8 || Read<uintptr_t>(point, 0) != pointType)
            return false;
        position = Read<uint32_t>(point, 8);
        return true;
    }
    inline bool Plan(const std::array<uint32_t, 3>& positions, std::array<uint32_t, 4>& output)
    {
        for (size_t i = 0; i < positions.size(); ++i)
            if (std::abs(int(int16_t(LOWORD(positions[i]))) - (52 + 179 * int(i))) > 2 ||
                int16_t(HIWORD(positions[i])) < 56 || int16_t(HIWORD(positions[i])) > 59)
                return false;
        std::copy(positions.begin(), positions.end(), output.begin());
        output[3] = LOWORD(positions[0]) | (uint32_t(HIWORD(positions[0]) + 24) << 16);
        return true;
    }
    inline void* AuthoredPoint(void* item, uint32_t position)
    {
        auto frames = Read<void*>(item, 12);
        if (!frames)
            return Read<void*>(Read<void*>(item, 8), 4);
        const auto begin = Read<uintptr_t>(frames, 4), end = Read<uintptr_t>(frames, 8);
        if (!begin || end - begin != 4)
            return nullptr;
        auto frame = *reinterpret_cast<void**>(begin);
        auto point = frame ? Read<void*>(frame, 4) : nullptr;
        return point && Read<uintptr_t>(point, 0) == pointType && Read<uint32_t>(point, 8) == position ? point : nullptr;
    }
    inline bool Join(void* controller)
    {
        if (!ready || !caption || !stockFits || !current || !open)
            return false;
        auto page = Read<void*>(controller, 56), manager = Read<void*>(controller, 20);
        if (!page || !manager)
            return false;
        if (auto found = bindings.find(page); found != bindings.end())
            return found->second.controller == controller && Live(found->second);
        if (findChild(page, nullptr, PluginID))
            return false;

        Binding binding;
        binding.controller = controller; binding.page = page; binding.manager = manager;
        binding.serial = ++nextSerial;
        std::array<uint32_t, 3> positions{};
        std::array<void*, 3> authored{};
        std::array<uint32_t, 4> planned{};
        void* source = nullptr;
        for (size_t i = 0; i < 3; ++i)
        {
            binding.tabs[i] = findChild(page, nullptr, CategoryIDs[i]);
            // Each ordinary page renames only its selected, plain caption.
            // The position preflight below identifies its category; advanced
            // pages keep all three buttons and use this ID in their body.
            if (!binding.tabs[i])
            {
                auto selected = findChild(page, nullptr, SelectedCategoryID);
                if (Kind(selected) == 0)
                    binding.tabs[i] = selected;
            }
            const auto kind = Kind(binding.tabs[i]);
            if ((kind != 0 && kind != 2) || !Position(Read<void*>(binding.tabs[i], 8), positions[i]) ||
                !stockFits(Read<void*>(binding.tabs[i], 8), 140))
            {
                static std::set<uint32_t> described;
                if (log && described.insert(Read<uint32_t>(page, 20)).second)
                {
                    std::ostringstream details;
                    details << "Settings category contract: id=" << CategoryIDs[i] << " kind=" << kind;
                    if (kind == 0 || kind == 2)
                    {
                        auto body = Read<void*>(binding.tabs[i], 8), point = Read<void*>(body, 4);
                        details << " point=" << point << " point-type=" << (point ? Read<void*>(point, 0) : nullptr)
                            << " expected=" << reinterpret_cast<void*>(pointType)
                            << " position=" << (point ? Read<uint32_t>(point, 8) : 0)
                            << " text=" << Read<uint32_t>(body, 60) << '/' << Read<uint32_t>(body, 64)
                            << " fits=" << stockFits(body, 140);
                    }
                    log(details.str());
                }
                return false;
            }
            authored[i] = AuthoredPoint(binding.tabs[i], positions[i]);
            if (!authored[i] || std::find(authored.begin(), authored.begin() + i, authored[i]) != authored.begin() + i)
                return false;
            if (kind == 2 && (Read<uint8_t>(binding.tabs[i], 76) & 2))
                source = binding.tabs[i];
        }
        if (!source || !Plan(positions, planned))
            return false;
        const std::array<void*, 2> nativeCallbacks{ Read<void*>(controller, 4), Read<void*>(manager, 204) };
        if (!nativeCallbacks[0] || !nativeCallbacks[1])
            return false;
        auto sourceBody = Read<void*>(source, 8);
        if (!Read<void*>(sourceBody, 24) || !Read<void*>(sourceBody, 28))
            return false;
        auto text = caption(sourceBody, 140);
        if (text.empty())
            return false;
        auto item = makeItem(nullptr, nullptr, 2);
        if (!item)
            return false;
        auto body = Read<void*>(item, 8);
        uint32_t unused = 0;
        if (!body || !Position(body, unused) || !Read<void*>(body, 36))
        {
            Destroy(item);
            return false;
        }
        auto callback = std::unique_ptr<Callback>(new (std::nothrow) Callback);
        if (!callback)
        {
            Destroy(item);
            return false;
        }
        callback->vtable = callbackVtable.data(); callback->page = page; callback->serial = binding.serial;
        Set<void*>(body, 24, Read<void*>(sourceBody, 24));
        bindStyle(static_cast<uint8_t*>(body) + 20, nullptr, Read<void*>(sourceBody, 32), Read<void*>(sourceBody, 28));
        Set<uint32_t>(Read<void*>(body, 4), 4, Read<uint32_t>(Read<void*>(sourceBody, 4), 4));
        Set<uint32_t>(Read<void*>(body, 4), 8, planned[3]);
        Set<uint32_t>(item, 4, PluginID);
        setText(body, nullptr, text.c_str());
        binding.tabs[3] = item; binding.body = body; binding.text = std::move(text);
        addChild(page, nullptr, item);
        if (!Child(page, item))
        {
            Destroy(item);
            return false;
        }
        bindings.emplace(page, binding);
        for (auto native : nativeCallbacks)
        {
            Set<uint32_t>(native, 4, Read<uint32_t>(native, 4) + 1);
            addCallback(item, nullptr, native);
        }
        for (size_t i = 0; i < 3; ++i)
        {
            Set<uint32_t>(Read<void*>(Read<void*>(binding.tabs[i], 8), 4), 8, planned[i]);
            Set<uint32_t>(authored[i], 8, planned[i]);
        }
        for (auto tab : binding.tabs)
        {
            if (Kind(tab) != 2)
                continue;
            ++callback->references;
            addCallback(tab, nullptr, callback.get());
        }
        --callback->references;
        callback.release();
        if (log) log("Plugins button joined to native Settings category strip");
        return true;
    }
    inline char __fastcall Initialize(void* controller, void*, void* package)
    {
        const auto result = initHook.thiscall<char>(controller, package);
        if (result && !Join(controller) && log)
            log("Settings Plugins join refused by native resource/geometry contract");
        return result;
    }
    inline void Forget(void* page)
    {
        bindings.erase(page);
        std::erase_if(pending, [page](const Action& action) { return action.page == page; });
    }
    inline void* __fastcall ControllerDestroyed(void* controller, void*)
    {
        if (controllerRetired) controllerRetired(controller);
        std::vector<void*> pages;
        for (const auto& [page, binding] : bindings)
            if (binding.controller == controller)
                pages.push_back(page);
        for (auto page : pages)
        {
            Forget(page);
            if (retire) retire(page);
        }
        return destructorHook.thiscall<void*>(controller);
    }
    inline void Service(void* updatingPage)
    {
        auto actions = std::move(pending);
        pending.clear();
        for (const auto& action : actions)
        {
            auto binding = Lookup(action.page, action.serial);
            if (!binding || updatingPage != action.page || !current(action.page, binding->manager))
                continue;
            if (!action.direction)
            {
                auto controller = binding->controller, page = binding->page;
                if (log) log("Settings Plugins deferred open dispatched");
                open(controller, page);
                continue;
            }
            auto at = std::find(binding->tabs.begin(), binding->tabs.end(), action.item);
            if (at == binding->tabs.end() || Read<void*>(binding->page, 36) != action.item)
                continue;
            if (action.direction > 1)
            {
                void* target = nullptr;
                int targetY = action.direction == 2 ? 481 : -1;
                const auto begin = Read<uintptr_t>(binding->page, 8), end = Read<uintptr_t>(binding->page, 12);
                for (auto child = begin; child < end; child += 4)
                {
                    auto item = *reinterpret_cast<void**>(child);
                    if (std::find(binding->tabs.begin(), binding->tabs.end(), item) != binding->tabs.end()) continue;
                    const auto kind = Kind(item);
                    if ((kind != 2 && kind != 3 && kind != 4 && kind != 9 && kind != 10 && kind != 12) ||
                        !(Read<uint8_t>(item, 76) & 2)) continue;
                    auto point = Read<void*>(Read<void*>(item, 8), 4);
                    if (!point) continue;
                    uint32_t position = 0;
                    reinterpret_cast<void (__fastcall*)(void*, void*, uint32_t*)>(Read<void**>(point, 0)[5])(point, nullptr, &position);
                    const int y = int16_t(HIWORD(position));
                    if (y < 90 || y > 450) continue;
                    if ((action.direction == 2 && y < targetY) || (action.direction == 3 && y > targetY))
                    {
                        target = item; targetY = y;
                    }
                }
                if (target) setFocus(binding->page, nullptr, target);
                continue;
            }
            int index = int(at - binding->tabs.begin());
            for (int n = 0; n < 4; ++n)
            {
                index = (index + action.direction + 4) % 4;
                auto target = binding->tabs[size_t(index)];
                if (Kind(target) == 2 && (Read<uint8_t>(target, 76) & 2))
                {
                    setFocus(binding->page, nullptr, target);
                    break;
                }
            }
        }
    }
    inline void ForgetStack(void* stack)
    {
        stackTokens.erase(stack);
        pending.clear();
    }
    inline void __fastcall Updated(void* context, void*, float elapsed)
    {
        auto manager = Read<void*>(context, 64);
        auto stack = manager ? getStack(manager, nullptr) : nullptr;
        uint64_t token = 0;
        if (stack)
        {
            auto found = stackTokens.find(stack);
            if (found == stackTokens.end()) found = stackTokens.emplace(stack, ++nextSerial).first;
            token = found->second;
        }
        struct Depth
        {
            Depth() { ++updateDepth; }
            ~Depth() { --updateDepth; }
        };
        {
            Depth depth;
            tickHook.thiscall<void>(context, elapsed);
        }
        if (!stack || updateDepth || servicing)
            return;
        auto found = stackTokens.find(stack);
        if (found == stackTokens.end() || found->second != token)
            return;
        const auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        if (!begin || end <= begin || end - begin > 8 * 64 || (end - begin) % 8)
            return;
        struct Scope
        {
            Scope() { servicing = true; }
            ~Scope() { servicing = false; }
        } scope;
        auto page = *reinterpret_cast<void**>(end - 8);
        if (frame) frame(page);
        found = stackTokens.find(stack);
        if (found != stackTokens.end() && found->second == token)
            Service(page);
    }
    inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        if (ready)
            return true;
        struct Rollback
        {
            ~Rollback()
            {
                if (ready) return;
                initHook.reset(); destructorHook.reset(); tickHook.reset();
            }
        } rollback;
        auto initialize = hook::pattern(game, "56 8B F1 8A 46 3C 84 C0 0F 85 84 00 00 00 8B 44 24 08 85 C0 89 46 34 74 09 8B 06 FF 50 44 84 C0");
        auto destroy = hook::pattern(game, "56 8B F1 8A 4E 3C 33 C0 3A C8 C7 06 ? ? ? ? C7 46 08 ? ? ? ? C7 46 10 ? ? ? ? 74 06 89 46 34 89 46 38 8D 4E 18 E8 82 01 00 00 8B CE 5E E9 2A 4B FC FF");
        auto lookup = hook::pattern(game, "8B 41 08 8B 49 0C 3B C1 56 74 13 8B 54 24 08 90 8B 30 39 56 04 74 0D 83 C0 04 3B C1 75 F2 33 C0");
        auto text = hook::pattern(game, "56 57 8B 7C 24 0C 57 8B F1 E8 72 9A 45 00 83 C4 04 50 57 8D 4E 28 E8 45 F5 FF FF 5F 5E C2 04 00");
        auto callback = hook::pattern(game, "8D 44 24 04 50 83 C1 2C E8 43 FE FF FF 8B 4C 24 04 85 C9 74 18 8B 41 04 83 F8 01 77 0C 8B 11 C7");
        auto focus = hook::pattern(game, "56 57 8B F9 8B 4F 24 85 C9 74 05 8B 01 FF 50 10 8B 74 24 0C 85 F6 74 07 8B 16 8B CE FF 52 0C 89");
        auto tick = hook::pattern(game, "56 8B 71 40 85 F6 74 2A D9 44 24 08 D8 0D ? ? ? ? E8 5D 3B 52 00 50 8B CE E8 71 6C 0C 00 8B C8 E8 AA 5E 31 00 8B 4C 24 08 8B 06 51 8B CE FF 50 20 5E C2 04 00");
        auto point = hook::pattern(game, "56 8B F1 E8 A8 E2 FF FF 33 C0 89 46 24 89 46 28 89 46 2C 89 46 30 C7 06 ? ? ? ? 89 46 38 89 46 3C");
        if (initialize.size() != 1 || destroy.size() != 1 || lookup.size() != 1 || text.size() != 1 ||
            callback.size() != 1 || focus.size() != 1 || tick.size() != 1 || point.size() != 1 ||
            !makeItem || !bindStyle || !addChild || !getStack || !caption || !stockFits || !current || !open)
            return false;
        pointType = reinterpret_cast<uintptr_t>(point.get_first()) + (0x1113A6B0 - 0x10D32A60);
        findChild = reinterpret_cast<decltype(findChild)>(lookup.get_first());
        setText = reinterpret_cast<decltype(setText)>(text.get_first());
        addCallback = reinterpret_cast<decltype(addCallback)>(callback.get_first());
        setFocus = reinterpret_cast<decltype(setFocus)>(focus.get_first());
        destructorHook = safetyhook::create_inline(destroy.get_first(), ControllerDestroyed);
        if (!destructorHook) return false;
        tickHook = safetyhook::create_inline(tick.get_first(), Updated);
        if (!tickHook) return false;
        initHook = safetyhook::create_inline(initialize.get_first(), Initialize);
        ready = bool(initHook);
        return ready;
    }
}
