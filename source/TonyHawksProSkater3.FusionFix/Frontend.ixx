module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <Windows.h>
#include <cstddef>
#include <safetyhook.hpp>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;

export
{
namespace THPS3Menu
{
    struct Component
    {
        uint8_t type = 0, flags = 0;
        uint16_t reserved = 0;
        uint32_t name = 0, value = 0;
        Component* next = nullptr;
    };
    struct Structure { Component* head = nullptr; Component* tail = nullptr; };
    struct Array { uint32_t data = 0; uint8_t type = 0, reserved = 0; uint16_t count = 0; };
    static_assert(sizeof(Component) == 16 && sizeof(Structure) == 8 && sizeof(Array) == 8);

    constexpr uint32_t Checksum(const char* text)
    {
        uint32_t value = 0xFFFFFFFF;
        for (; *text; ++text)
        {
            auto c = static_cast<uint8_t>(*text);
            if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (c == '/') c = '\\';
            value ^= c;
            for (int bit = 0; bit < 8; ++bit)
                value = (value >> 1) ^ ((value & 1) ? 0xEDB88320 : 0);
        }
        return value;
    }
    static_assert(Checksum("textmenuelement") == 0x6B7AE77F);
    static_assert(Checksum("verticalmenu") == 0x87FCAE56);

    // Borrowed synchronous inputs only. Native429630/4292B0 clone strings,
    // structures and arrays before retention; native destructors never own this.
    class Parameters
    {
    public:
        Structure value;
        Parameters() = default;
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;
        Parameters(Parameters&&) = delete;
        Parameters& operator=(Parameters&&) = delete;
        static uint32_t Pointer(const void* value) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(value)); }
        void Integer(const char* name, uint32_t number) { Add(1, Checksum(name), number); }
        void Float(const char* name, float number)
        {
            uint32_t bits;
            std::memcpy(&bits, &number, sizeof(bits));
            Add(2, Checksum(name), bits);
        }
        void Name(const char* name, uint32_t checksum) { Add(13, Checksum(name), checksum); }
        void Name(const char* name, const char* text) { Name(name, Checksum(text)); }
        void Flag(const char* name) { Add(13, 0, Checksum(name)); }
        void Text(const char* name, const std::string& text)
        {
            strings.push_back(text);
            Add(3, Checksum(name), Pointer(strings.back().c_str()));
        }
        void Nested(const char* name, Structure* nested) { Add(11, Checksum(name), Pointer(nested)); }
        void List(const char* name, Array* array) { Add(12, Checksum(name), Pointer(array)); }

    private:
        std::deque<Component> components;
        std::deque<std::string> strings;
        void Add(uint8_t type, uint32_t name, uint32_t data)
        {
            components.push_back({ type, 0, 0, name, data, nullptr });
            auto* next = &components.back();
            if (value.tail) value.tail->next = next;
            else value.head = next;
            value.tail = next;
        }
    };
}

namespace THPS3Menu
{
    template<class T> T Read(const void* base, size_t offset = 0)
    {
        T result{};
        std::memcpy(&result, static_cast<const uint8_t*>(base) + offset, sizeof(result));
        return result;
    }

    struct Addresses
    {
        uintptr_t initController, attachController, unlinkController, childById, childAt, subscribe, subscribed, launch,
            remove, create, command, drawer, select, setFocus, refresh, setText, updateText, setTextHeight, textWidth;
        uintptr_t recursive, parents, input, task, destroy, removeControllers, removeTree;
        uint32_t containerTable, menuTable, textTable, managerTable, mainMenuTable, gameMenuTable;
    };
    inline const Addresses& Game()
    {
        static const Addresses game = []
        {
            auto at = [](const char* bytes) { return reinterpret_cast<uintptr_t>(hook::pattern(bytes).get_first()); };
            auto operand = [](const char* bytes, ptrdiff_t offset) { return *hook::pattern(bytes).get_first<uint32_t>(offset); };
            Addresses a{};
            a.initController = at("8B C1 89 40 08 8D 50 04 C7 42 08 00");
            a.attachController = at("56 8B F1 8B 4C 24 08 57 8B BE 98 01");
            a.unlinkController = at("8D 41 04 C7 01 ? ? ? ? 8B 48 10 8B 50 0C C7 00 ? ? ? ? 89 51 0C 8B 48 0C 8B 50 10 89 51 10 89 40 0C 89 40 10 C3 90 90 90 90 90 90 90 90 56");
            a.childById = at("8B 41 6C 33 D2 56 83 78 04 FF 74 22");
            a.childAt = at("8B 41 6C 53 56 83 CE FF 8B 50 04 57");
            a.subscribe = at("53 8B 5C 24 08 56 57 83 CF FF 33 C0 8D B1 88 00 00 00 8B 16 3B D3 74 39");
            a.subscribed = at("56 8B 74 24 08 33 C0 8D 91 88 00 00 00 39 32 74 0F 40 83 C2 08 83 F8 20 7C F3");
            a.launch = at("8B 44 24 04 8B 89 30 01 00 00 6A 00 50 E8");
            a.remove = at("8B 44 24 04 56 6A 00 50 E8 ? ? ? ? 8B F0 85 F6 74 0E");
            a.create = at("83 EC 20 8D 44 24 14 53 8B 5C 24 28");
            a.command = at("51 53 56 8B F1 57 8B 0D ? ? ? ? E8");
            a.drawer = at("8B 44 24 08 8B 54 24 04 53 56 57 3D");
            a.select = at("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 18 56 57");
            a.setFocus = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 38 53 56 8B F1");
            a.refresh = at("56 8B F1 E8 ? ? ? ? 8B 46 18 D9");
            a.setText = at("8B 44 24 04 56 8B F1 50 8D 8E B4 01");
            a.updateText = at("56 8B F1 8B 8E C0 01 00 00 85 C9 74");
            a.setTextHeight = at("8B 44 24 04 89 41 2C C2 04 00 90 90");
            a.textWidth = at("8B 51 20 56 85 D2 75 0A D9 05 ? ? ? ? 5E");
            a.recursive = at("55 8B E9 56 57 8B 75 6C 8B 7C 24 10");
            a.parents = at("56 57 8B 7C 24 0C 8B F1 57 E8 ? ? ? ? 8B 46");
            a.input = at("8B 54 24 04 8B 42 2C 8B 48 3C 85 C9");
            a.task = at("64 A1 00 00 00 00 6A FF 8B 0D ? ? ? ? 68");
            a.destroy = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 53 55 8B E9 56 57 89 6C 24 ? ? ? ? ? ? ? ? 8B");
            a.removeControllers = at("53 56 57 8B F9 83 CB FF 8B 77 6C 39");
            a.removeTree = at("57 8B F9 8B 47 18 83 78 04 FF 74 21");
            NativeFrontend.SetAddress(operand("A1 ? ? ? ? 85 C0 75 3D 8A 4C 24 04 84 C9 74 35 6A 01 50 68 08", 1));
            NativeManager.SetAddress(operand("A1 ? ? ? ? 85 C0 75 3D 8A 4C 24 04 84 C9 74 35 6A 01 50 68 58", 1));
            NativeResources.SetAddress(operand("C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 5E C3 90 90 90 90 90 90 90 90 90 90 90 8B C1 33 C9 89 08 89 48 04 89 48 0C 89", 2));
            a.containerTable = operand("C7 06 ? ? ? ? C7 46 38 12 13 CC", 2);
            a.menuTable = operand("C7 06 ? ? ? ? C7 46 38 56 AE FC", 2);
            a.textTable = operand("C7 06 ? ? ? ? C7 46 38 7F E7 7A", 2);
            a.managerTable = operand("C7 06 ? ? ? ? C7 86 50 01 00 00", 2);
            a.mainMenuTable = operand("C7 06 ? ? ? ? 89 5E 08 89 5E 10", 2);
            a.gameMenuTable = operand("C7 06 ? ? ? ? 88 5E 18 C6 46 19", 2);
            return a;
        }();
        return game;
    }
}

namespace THPS3Menu
{
    inline constexpr uint32_t RootID = Checksum("wfp_thps3_advanced");
    inline constexpr uint32_t EntryID = Checksum("wfp_thps3_advanced_entry");
    inline constexpr uint32_t ParentID = Checksum("contain1");
    inline constexpr uint32_t TitleID = Checksum("options_main_menu");
    inline constexpr uint32_t PauseID = Checksum("game_menu");

    inline bool Object(const void* value, uint32_t table)
    {
        return value && Read<uint32_t>(value) == table;
    }
    inline bool ScreenObject(const void* value, uint32_t id)
    {
        if (id == Checksum("main_menu"))
            return Object(value, Game().mainMenuTable);
        if (id == Checksum("game_menu"))
            return Object(value, Game().gameMenuTable);
        return false;
    }
    inline void* Manager()
    {
        auto* value = NativeManager.get();
        return Object(value, Game().managerTable) ? value : nullptr;
    }
    inline void* Lookup(void* manager, uint32_t id)
    {
        if (!Object(manager, Game().managerTable)) return nullptr;
        // Do not call the native unbounded registry walk on damaged lists.
        auto* entry = Read<void*>(manager, 0x138);
        for (unsigned count = 0; entry && count < 4096; ++count)
        {
            uint32_t entryID;
            void* object;
            void* next;
            std::memcpy(&entryID, entry, 4);
            std::memcpy(&object, static_cast<uint8_t*>(entry) + 4, 4);
            std::memcpy(&next, static_cast<uint8_t*>(entry) + 8, 4);
            if (entryID == id) return object;
            if (next == entry) return nullptr;
            entry = next;
        }
        return nullptr;
    }
    inline void* SelectedChild(void* parent)
    {
        if (!Object(parent, Game().containerTable) || Read<uint8_t>(parent, 700)) return nullptr;
        const auto index = Read<int32_t>(parent, 420);
        if (index < 0 || index > 1024) return nullptr;
        // Container420 is an index; a vertical menu's field420 is a child ID.
        return reinterpret_cast<void*(__thiscall*)(void*, int32_t, uint32_t*)>(Game().childAt)(parent, index, nullptr);
    }

    struct Context
    {
        void* frontend = nullptr;
        void* screen = nullptr;
        void* manager = nullptr;
        void* parent = nullptr;
        void* base = nullptr;
        uint32_t screenID = 0, baseID = 0;
        bool pause = false;

        static Context Discover(const char** failure = nullptr)
        {
            Context result;
            auto refuse = [&](const char* reason) { if (failure) *failure = reason; return Context{}; };
            if (failure) *failure = nullptr;
            result.frontend = NativeFrontend.get();
            if (!result.frontend) return refuse("frontend unavailable");
            if (Read<uint32_t>(result.frontend, 60)) return refuse("frontend transition");
            result.screen = Read<void*>(result.frontend, 68);
            result.screenID = Read<uint32_t>(result.frontend, 64);
            if (!ScreenObject(result.screen, result.screenID)) return refuse("screen identity");
            if (!Read<uint8_t>(result.screen, 4)) return refuse("screen inactive");
            result.manager = Manager();
            if (!result.manager) return refuse("manager identity");
            result.parent = Lookup(result.manager, ParentID);
            if (!Object(result.parent, Game().containerTable) || !Read<uint8_t>(result.parent, 92)
                || !Read<uint8_t>(result.parent, 93) || !Read<uint8_t>(result.parent, 94)) return refuse("parent identity/show/active/focus");
            result.base = SelectedChild(result.parent);
            if (!Object(result.base, Game().menuTable) || !Read<uint8_t>(result.base, 92)
                || !Read<uint8_t>(result.base, 93)) return refuse("selected child identity/show/active");
            result.baseID = Read<uint32_t>(result.base, 52);
            if (result.baseID != TitleID && result.baseID != PauseID) return refuse("selected child is outside Options/pause");
            if (Lookup(result.manager, result.baseID) != result.base || Read<void*>(result.base, 4) != result.parent) return refuse("selected child ownership");
            result.pause = result.baseID == PauseID;
            return result;
        }
        bool Alive() const
        {
            return frontend && NativeFrontend.get() == frontend && !Read<uint32_t>(frontend, 60)
                && Read<void*>(frontend, 68) == screen && Read<uint32_t>(frontend, 64) == screenID
                && ScreenObject(screen, screenID) && Read<uint8_t>(screen, 4)
                && Manager() == manager
                && Object(parent, Game().containerTable) && Read<uint8_t>(parent, 92)
                && Read<uint8_t>(parent, 93) && Read<uint8_t>(parent, 94)
                && Object(base, Game().menuTable) && Read<uint32_t>(base, 52) == baseID
                && Read<void*>(base, 4) == parent;
        }
        bool StockCurrent() const
        {
            return Alive() && Read<uint32_t>(parent, 424) == baseID && Read<uint8_t>(base, 92) && Read<uint8_t>(base, 93);
        }
        bool PrivateCurrent(void* root, bool requireShown = true) const
        {
            return Alive() && Object(root, Game().menuTable) && Read<uint32_t>(root, 52) == RootID
                && Read<void*>(root, 4) == parent && Read<uint32_t>(parent, 424) == RootID
                && (!requireShown || (Read<uint8_t>(root, 92) && Read<uint8_t>(root, 93)));
        }
    };
}

namespace THPS3Menu
{
    struct Event { uint32_t type, target; Structure* params; };
    static_assert(sizeof(Event) == 12);
    struct Controller;
    using ControllerDestroy = void*(__fastcall*)(Controller*, void*, uint32_t);
    using ControllerInit = void(__fastcall*)(Controller*, void*);
    using ControllerExecute = uint8_t(__fastcall*)(Controller*, void*, Event*);
    struct ControllerTable { ControllerDestroy destroy; ControllerInit init; ControllerExecute execute; };
    static_assert(sizeof(ControllerTable) == 12);
    struct ControllerHeader
    {
        ControllerTable* table = nullptr;
        uint32_t nodeTable = 0;
        void* nodeValue = nullptr;
        uint32_t reserved = 0;
        void* next = nullptr;
        void* previous = nullptr;
        void* owner = nullptr;
    };
    static_assert(sizeof(ControllerHeader) == 28);
    static_assert(offsetof(ControllerHeader, nodeValue) == 8 && offsetof(ControllerHeader, owner) == 24);

    enum class Command { Open, Accept, Back, Left, Right, Focus, Enter };
    struct Controller
    {
        ControllerHeader native;
        uint64_t generation = 0;
        uint32_t row = 0;
        Command command = Command::Accept;
        bool retired = false;
        Controller() = default;
        Controller(const Controller&) = delete;
        Controller& operator=(const Controller&) = delete;
        Controller(Controller&&) = delete;
        Controller& operator=(Controller&&) = delete;
    };
    static_assert(offsetof(Controller, native) == 0);

    // The native UI borrows this embedded list node. Its owner destructor only
    // unlinks it; our storage stays alive through event, input and task unwind.
    // Native4274A0 treats an event target as QB code even for a CFn symbol, so
    // this bridge deliberately uses the proven native controller interface.
    inline void InitializeController(Controller& controller, ControllerTable& table)
    {
        // Fresh value-initialized storage only; never reinitialize an attached
        // node. Keep it at a stable address for its entire native membership.
        reinterpret_cast<void(__thiscall*)(ControllerHeader*)>(Game().initController)(&controller.native);
        controller.native.table = &table;
    }
    inline void AttachController(void* owner, Controller& controller)
    {
        // Caller proves owner identity immediately before attachment. The
        // manager's missing-ID branch would otherwise pass a null owner.
        reinterpret_cast<void(__thiscall*)(void*, ControllerHeader*)>(Game().attachController)(owner, &controller.native);
    }
    inline void UnlinkController(Controller& controller)
    {
        // Only after every dispatch/input/task depth is zero: unlinking changes
        // the next pointer that an outer native iterator still reads.
        auto* table = controller.native.table;
        reinterpret_cast<void(__thiscall*)(ControllerHeader*)>(Game().unlinkController)(&controller.native);
        controller.native.table = table;
        controller.native.owner = nullptr;
        controller.retired = true;
    }
}

namespace THPS3Menu
{
    // Every native caller below keeps touching its node/list after controller
    // Execute returns. Actions and storage reclamation run only after the full
    // recursive/parent event, input and frontend task call has unwound.
    struct Unwind
    {
        inline static SafetyHookInline recursive, parents, input, task, destroy, remove, removeControllers, removeTree;
        inline static uint32_t depth = 0;
        inline static bool draining = false;
        inline static void (*afterFrame)() = nullptr;
        inline static void (*beforeDestroy)(void*) = nullptr;
        inline static void (*observe)() = nullptr;

        struct Scope
        {
            Scope() { ++depth; }
            ~Scope() { --depth; }
            Scope(const Scope&) = delete;
            Scope& operator=(const Scope&) = delete;
        };
        static bool Idle() { return depth == 0 && !draining; }
        static uint8_t __fastcall Recursive(void* node, void*, Event* event, uint8_t skip)
        {
            Scope scope;
            auto result = recursive.unsafe_thiscall<uint8_t>(node, event, skip);
            return result;
        }
        static uintptr_t __fastcall Parents(void* node, void*, Event* event)
        {
            Scope scope;
            auto result = parents.unsafe_thiscall<uintptr_t>(node, event);
            return result;
        }
        static uintptr_t __cdecl Input(void* data)
        {
            Scope scope;
            auto result = input.unsafe_ccall<uintptr_t>(data);
            return result;
        }
        static uintptr_t __cdecl Task(void* data)
        {
            uintptr_t result;
            {
                Scope scope;
                result = task.unsafe_ccall<uintptr_t>(data);
                if (observe) observe();
            }
            if (Idle() && afterFrame)
            {
                struct DrainScope
                {
                    bool previous = draining;
                    DrainScope() { draining = true; }
                    ~DrainScope() { draining = previous; }
                } drain;
                afterFrame();
            }
            return result;
        }
        static void* __fastcall Destroy(void* node, void*)
        {
            Scope scope;
            if (beforeDestroy) beforeDestroy(node);
            return destroy.unsafe_thiscall<void*>(node);
        }
        static uintptr_t __fastcall Remove(void* manager, void*, uint32_t id)
        {
            Scope scope;
            return remove.unsafe_thiscall<uintptr_t>(manager, id);
        }
        static uintptr_t __fastcall RemoveControllers(void* node, void*)
        {
            Scope scope;
            return removeControllers.unsafe_thiscall<uintptr_t>(node);
        }
        static uintptr_t __fastcall RemoveTree(void* node, void*)
        {
            Scope scope;
            return removeTree.unsafe_thiscall<uintptr_t>(node);
        }
        static void Reset()
        {
            // Only startup rollback, before any native retained controllers.
            removeTree.reset(); removeControllers.reset(); remove.reset();
            destroy.reset(); task.reset(); input.reset(); parents.reset(); recursive.reset();
            afterFrame = nullptr;
            beforeDestroy = nullptr;
            observe = nullptr;
        }
        static bool Install(void (*frame)(), void (*retire)(void*), void (*check)())
        {
            afterFrame = frame;
            beforeDestroy = retire;
            observe = check;
            recursive = safetyhook::create_inline(reinterpret_cast<void*>(Game().recursive), Recursive);
            parents = safetyhook::create_inline(reinterpret_cast<void*>(Game().parents), Parents);
            input = safetyhook::create_inline(reinterpret_cast<void*>(Game().input), Input);
            task = safetyhook::create_inline(reinterpret_cast<void*>(Game().task), Task);
            destroy = safetyhook::create_inline(reinterpret_cast<void*>(Game().destroy), Destroy);
            remove = safetyhook::create_inline(reinterpret_cast<void*>(Game().remove), Remove);
            removeControllers = safetyhook::create_inline(reinterpret_cast<void*>(Game().removeControllers), RemoveControllers);
            removeTree = safetyhook::create_inline(reinterpret_cast<void*>(Game().removeTree), RemoveTree);
            return true;
        }
    };
}

namespace THPS3Menu
{
    struct Caption
    {
        std::string label, suffix, rendered;
        void* font = nullptr;
        void* render = nullptr;
        float width = 0.0f, height = 0.0f, spacing = 0.0f, border = 0.0f, nativeHeight = 0.0f;
    };
    inline std::map<void*, Caption> captions;

    // Measure the native glyph layout at a readable height. Only the label is
    // shortened; a value which cannot fit by itself refuses publication.
    template<class Measure> bool FitCaption(const std::string& label, const std::string& suffix,
        float width, float nativeHeight, Measure measure, std::string& text, float& height)
    {
        if (!std::isfinite(width) || width <= 0.0f || !std::isfinite(nativeHeight) || nativeHeight <= 0.0f) return false;
        const auto minimum = std::min(nativeHeight, 14.0f);
        text = label + suffix;
        height = nativeHeight;
        auto measured = measure(text, height);
        if (!std::isfinite(measured) || measured < 0.0f) return false;
        if (measured <= width) return true;
        height = std::max(minimum, nativeHeight * width / measured);
        measured = measure(text, height);
        if (!std::isfinite(measured) || measured < 0.0f) return false;
        if (measured <= width) return true;
        height = minimum;
        auto fits = [&](const std::string& candidate)
        {
            auto value = measure(candidate, height);
            return std::isfinite(value) && value >= 0.0f && value <= width;
        };
        if (!fits("..." + suffix)) return false;
        size_t low = 0, high = label.size();
        while (low < high)
        {
            auto middle = low + (high - low + 1) / 2;
            if (fits(label.substr(0, middle) + "..." + suffix)) low = middle;
            else high = middle - 1;
        }
        text = label.substr(0, low) + "..." + suffix;
        return fits(text);
    }

    inline bool Fit(void* node, float limit = 420.0f)
    {
        auto found = captions.find(node);
        if (found == captions.end() || !Object(node, Game().textTable) || !Read<uint8_t>(node, 92) || Read<uint8_t>(node, 475)) return false;
        reinterpret_cast<void(__thiscall*)(void*)>(Game().updateText)(node);
        auto render = Read<void*>(node, 448);
        auto drawer = Read<void*>(node, 452);
        if (!render || !drawer) return false;
        auto font = Read<void*>(render, 4);
        if (!font) return false;
        auto denominator = Read<float>(font, 272);
        auto nativeHeight = denominator * Read<float>(drawer, 8);
        auto spacing = Read<float>(node, 464), border = Read<float>(node, 468);
        if (!std::isfinite(denominator) || denominator <= 0.0f || !std::isfinite(spacing) || spacing < 0.0f
            || !std::isfinite(border) || border < 0.0f) return false;
        auto& caption = found->second;
        if (caption.font == font && caption.render == render && caption.width == limit && caption.spacing == spacing
            && caption.border == border && caption.nativeHeight == nativeHeight && caption.height == Read<float>(node, 80)) return true;
        auto measure = [&](const std::string& text, float height)
        {
            // Native SetText clones the string; no borrowed pointer survives.
            reinterpret_cast<void(__thiscall*)(void*, const char*)>(Game().setText)(node, text.c_str());
            reinterpret_cast<void(__thiscall*)(void*)>(Game().updateText)(node);
            reinterpret_cast<void(__thiscall*)(void*, float)>(Game().setTextHeight)(render, height);
            return reinterpret_cast<float(__thiscall*)(void*, int32_t)>(Game().textWidth)(render, -1);
        };
        std::string text;
        float height = 0.0f;
        if (!FitCaption(caption.label, caption.suffix, limit - 2.0f * border, nativeHeight, measure, text, height)) return false;
        // The last binary-search measurement need not be the chosen prefix.
        measure(text, height);
        *reinterpret_cast<float*>(static_cast<uint8_t*>(node) + 80) = height + spacing + 2.0f * border;
        caption.font = font;
        caption.render = render;
        caption.width = limit;
        caption.height = Read<float>(node, 80);
        caption.spacing = spacing;
        caption.border = border;
        caption.nativeHeight = nativeHeight;
        caption.rendered = std::move(text);
        return true;
    }
}

namespace THPS3Menu
{
    inline std::vector<std::string> HelpLines(const std::string& text)
    {
        std::vector<std::string> lines;
        for (size_t offset = 0; offset < text.size();)
        {
            size_t count = std::min(size_t(24), text.size() - offset);
            if (count < text.size() - offset)
            {
                auto space = text.rfind(' ', offset + count - 1);
                if (space != std::string::npos && space > offset)
                    count = space - offset;
                else
                {
                    while (count && (static_cast<uint8_t>(text[offset + count]) & 0xC0) == 0x80)
                        --count;
                }
            }
            if (!count)
                return {};
            lines.push_back(text.substr(offset, count));
            offset += count;
            if (offset < text.size() && text[offset] == ' ')
                ++offset;
        }
        return lines;
    }
}

namespace THPS3Menu
{
    inline constexpr uint32_t ShowEvent = uint32_t(-839833858);
    inline constexpr uint32_t PrepareEvent = 1234828114;
    inline constexpr uint32_t BackEvent = uint32_t(-1842266424);
    inline constexpr uint32_t ChooseEvent = 1880869774;
    inline constexpr uint32_t PressEvent = 281135239;
    inline constexpr uint32_t HandledEvent = 316323547;
    inline constexpr size_t RootPage = SIZE_MAX;
    inline constexpr size_t PageRows = 6;
    inline constexpr size_t SettingRows = 3;
    inline constexpr uint32_t HelpID = Checksum("wfp_thps3_help");
    enum class ItemKind { Page, Previous, Next, Back, Value, Details, Text };
    struct Item
    {
        ItemKind kind = ItemKind::Back;
        size_t page = 0;
        RWGTAMenu::Row row;
        uint32_t id = 0;
        void* widget = nullptr;
        std::string label;
        bool enabled = true;
        std::string suffix;
    };
    struct Pending
    {
        Command command;
        uint64_t generation;
        size_t page, start, rowStart, row;
        uint32_t selectedID;
        void* selectedWidget;
    };
    struct Created { uint32_t id, table; void* node; void* parent; };
    struct Host
    {
        Context context;
        void* root = nullptr;
        void* entry = nullptr;
        void* expectedDestroy = nullptr;
        RWGTAMenu::Model model;
        uint64_t generation = 1;
        size_t page = RootPage, start = 0, rowStart = 0;
        bool open = false, busy = false, invalid = false, modelOpening = false, nativeBackPending = false, recovering = false;
        bool baseRetiredWhilePrivate = false;
        uint32_t foreignDepth = 0;
        uint64_t enterGeneration = 0;
        std::vector<Item> items;
        std::vector<uint32_t> leaves;
        std::vector<Created> created;
        std::vector<std::unique_ptr<Controller>> controllers;
        std::deque<Pending> pending;
        bool details = false, restoreSelection = false;
        size_t detailStart = 0;
        std::vector<std::string> detailLines;
        Item detailItem;
    };
    Host host;
    inline HMODULE module = nullptr;
    inline bool installed = false;
    inline std::function<void()> UpdateFrame;
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartBaseline;
    inline bool restartPending = false;

    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        // This installed script set has no proved native locale getter.
        return "en";
    }

    inline void Log(const char* message)
    {
        std::string line = "THPS3 native menu: ";
        line += message;
        line += '\n';
        OutputDebugStringA(line.c_str());
    }

    inline void Diagnostic(const char* reason)
    {
        static std::set<std::string> seen;
        if (reason && seen.size() < 32 && seen.insert(reason).second) Log(reason);
    }

    struct Busy
    {
        bool previous;
        Busy() : previous(host.busy) { host.busy = true; }
        ~Busy() { host.busy = previous; }
    };
    struct ForeignScope
    {
        ForeignScope() { ++host.foreignDepth; }
        ~ForeignScope() { --host.foreignDepth; }
    };
    struct Snapshot
    {
        uint64_t generation = host.generation;
        bool open = host.open;
        void* root = host.root;
        bool Current(bool shown = true) const
        {
            return generation == host.generation && open == host.open && root == host.root && !host.invalid
                && host.context.PrivateCurrent(root, shown);
        }
    };
    inline bool UpdateRestart()
    {
        Snapshot snapshot;
        bool pending = false;
        for (const auto& page : host.model.Pages())
            for (const auto& row : page.rows)
            {
                if (!row.plugin || !row.setting || row.setting->apply != "restart" || !row.setting->HasValue()) continue;
                auto key = std::make_pair(row.plugin->module, row.setting->id);
                asi_settings::LoadedPlugin plugin;
                plugin.api = row.plugin->api;
                asi_settings::Setting setting;
                setting.id = row.setting->id;
                std::string value;
                { ForeignScope call; value = OtherPlugins::Get(plugin, setting); }
                if (!snapshot.Current()) return false;
                auto baseline = restartBaseline.emplace(std::move(key), value).first;
                pending |= !asi_settings::text::IEquals(baseline->second, value);
            }
        restartPending = pending;
        return true;
    }
    inline bool RecoveryOwner()
    {
        const auto& context = host.context;
        return host.root && Manager() == context.manager && context.frontend
            && NativeFrontend.get() == context.frontend
            && !Read<uint32_t>(context.frontend, 60) && Read<void*>(context.frontend, 68) == context.screen
            && Read<uint32_t>(context.frontend, 64) == context.screenID && ScreenObject(context.screen, context.screenID)
            && Read<uint8_t>(context.screen, 4) && Lookup(context.manager, ParentID) == context.parent
            && Object(context.parent, Game().containerTable) && Read<uint8_t>(context.parent, 92)
            && Read<uint8_t>(context.parent, 93) && Read<uint8_t>(context.parent, 94)
            && !Read<uint8_t>(context.parent, 700) && Read<uint32_t>(context.parent, 424) == RootID
            && Lookup(context.manager, RootID) == host.root && Object(host.root, Game().menuTable)
            && Read<void*>(host.root, 4) == context.parent
            && Read<uint8_t>(host.root, 92) && Read<uint8_t>(host.root, 93);
    }
    inline bool RecoveryCurrent() { return RecoveryOwner() && SelectedChild(host.context.parent) == host.root; }
    inline void RepairRecoveryIndex()
    {
        if (!host.baseRetiredWhilePrivate || !RecoveryOwner() || RecoveryCurrent()) return;
        int32_t index = -1;
        auto* child = reinterpret_cast<void*(__thiscall*)(void*, uint32_t, int32_t*)>(Game().childById)(host.context.parent, RootID, &index);
        if (child != host.root || index < 0 || index > 1024 || !RecoveryOwner()) return;
        // Removing the earlier base leaves the parent's cached child index stale; restore it from the ID lookup.
        *reinterpret_cast<int32_t*>(reinterpret_cast<uint8_t*>(host.context.parent) + 420) = index;
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        *reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(object) + offset) = value;
    }
    inline void Emit(uint32_t type, uint32_t id)
    {
        Parameters params;
        Event event{ type, id, &params.value };
        reinterpret_cast<void(__thiscall*)(void*, Event*)>(Game().launch)(host.context.manager, &event);
    }
    inline bool Queue(Command command, size_t row = 0, bool beforeShow = false)
    {
        if (host.busy || host.invalid || !host.context.PrivateCurrent(host.root, !beforeShow)) return false;
        if (command != Command::Open && !host.open) return false;
        if (command == Command::Back && host.foreignDepth)
        {
            host.model.Close();
            host.open = false;
            ++host.generation;
            host.pending.clear();
            host.nativeBackPending = true;
            return true;
        }
        if (host.pending.size() >= 32) return false;
        auto selected = Read<uint32_t>(host.root, 420);
        auto* widget = (row < host.items.size()) ? host.items[row].widget : nullptr;
        host.pending.push_back({ command, host.generation, host.page, host.start, host.rowStart, row, selected, widget });
        return true;
    }
    inline void* __fastcall ControllerDestroyed(Controller* value, void*, uint32_t)
    {
        if (!value) return nullptr;
        // This node is DLL-owned. Native deleting flags do not transfer its
        // allocator ownership. Keep both embedded links alive until unwind.
        if (!value->retired && value->native.owner && value->native.owner != host.expectedDestroy
            && value->generation == host.generation)
        {
            ++host.generation;
            host.invalid = true;
            if (!host.modelOpening) host.model.Close();
            host.open = false;
        }
        value->retired = true;
        value->native.owner = nullptr;
        return value;
    }
    inline void __fastcall ControllerInitialized(Controller* value, void*)
    {
        const uint32_t rootEvents[] = { ShowEvent, PrepareEvent, BackEvent, PressEvent };
        const uint32_t rowEvents[] = { ChooseEvent };
        auto subscribe = [&](uint32_t type)
        {
            reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, uint8_t)>(Game().subscribe)(value->native.owner, type, 1);
            if (!reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(Game().subscribed)(value->native.owner, type))
                host.invalid = true;
        };
        if (value->command == Command::Open)
            for (auto event : rootEvents) subscribe(event);
        else
            for (auto event : rowEvents) subscribe(event);
    }
    inline uint8_t __fastcall ControllerExecuted(Controller* value, void*, Event* event)
    {
        if (!value || value->retired || value->generation != host.generation || !event) return 0;
        // Native widgets accept their own target or broadcast1. A subscribed
        // controller sees foreign-target events too, so retain that same scope
        // before any private root routing or event consumption.
        if (value->command == Command::Open && event->target != RootID && event->target != 1) return 0;
        if (value->command == Command::Enter)
        {
            if (!host.busy && !host.invalid && event->type == ChooseEvent && (event->target == EntryID || event->target == 1)
                && host.entry && value->native.owner == host.entry && host.context.StockCurrent()
                && Lookup(host.context.manager, EntryID) == host.entry
                && Read<void*>(host.entry, 4) == host.context.base
                && Read<uint32_t>(host.context.base, 420) == EntryID
                && Read<uint8_t>(host.entry, 92) && Read<uint8_t>(host.entry, 93) && Read<uint8_t>(host.entry, 94))
                host.enterGeneration = host.generation;
            return 0;
        }
        if (!host.root || !value->native.owner) return 0;
        if (host.recovering)
        {
            // Model ownership was lost, but this exact native page remains
            // current. Only its existing Back control can request the ordinary
            // native history escape; no old setting/preset command survives.
            if (value->command == Command::Accept && value->row < host.items.size()
                && host.items[value->row].kind == ItemKind::Back && event->type == ChooseEvent
                && (event->target == host.items[value->row].id || event->target == 1) && host.items[value->row].widget == value->native.owner
                && Read<uint32_t>(host.root, 420) == host.items[value->row].id && RecoveryCurrent())
                host.nativeBackPending = true;
            return 0; // Physical Back goes through the original native route.
        }
        if (value->command == Command::Open)
        {
            if (event->type == ShowEvent || event->type == PrepareEvent)
                Queue(Command::Open, 0, true);
            else if (event->type == BackEvent && Queue(Command::Back))
                Write(event, 0, HandledEvent);
            else if (event->type == PressEvent && host.open && event->params)
            {
                // A bounded borrowed integer lookup; no native allocation,
                // model calls or borrowed argument retention in this callback.
                auto* component = Read<Component*>(event->params);
                for (size_t count = 0; component && count < 64; ++count)
                {
                    if (component->type == 1 && component->name == Checksum("press"))
                    {
                        auto selected = Read<uint32_t>(host.root, 420);
                        for (size_t row = 0; row < host.items.size(); ++row)
                            if (host.items[row].id == selected && component->value < 2)
                                if (Queue(component->value ? Command::Right : Command::Left, row))
                                    Write(event, 0, HandledEvent);
                        break;
                    }
                    component = component->next;
                }
            }
        }
        else if (event->type == ChooseEvent && (event->target == Read<uint32_t>(value->native.owner, 52) || event->target == 1)
            && Read<uint32_t>(host.root, 420) == Read<uint32_t>(value->native.owner, 52) && Read<uint8_t>(value->native.owner, 92)
            && Read<uint8_t>(value->native.owner, 93) && Read<uint8_t>(value->native.owner, 94))
            Queue(Command::Accept, value->row);
        return 0;
    }
    inline ControllerTable controllerTable{ ControllerDestroyed, ControllerInitialized, ControllerExecuted };

    inline void Attach(void* owner, Command command, size_t row = 0)
    {
        auto value = std::make_unique<Controller>();
        value->generation = host.generation;
        value->row = uint32_t(row);
        value->command = command;
        InitializeController(*value, controllerTable);
        auto* pointer = value.get();
        host.controllers.push_back(std::move(value));
        AttachController(owner, *pointer);
    }
    inline void Retire(void* node)
    {
        captions.erase(node);
        if (node == host.context.base && Manager() == host.context.manager
            && Lookup(host.context.manager, ParentID) == host.context.parent && Object(host.context.parent, Game().containerTable)
            && Lookup(host.context.manager, RootID) == host.root && Object(host.root, Game().menuTable)
            && Read<void*>(host.root, 4) == host.context.parent && SelectedChild(host.context.parent) == host.root
            && Read<uint32_t>(host.context.parent, 424) == RootID)
            host.baseRetiredWhilePrivate = true;
        for (auto& value : host.controllers)
        {
            if (value->native.owner != node) continue;
            value->retired = true;
            value->native.owner = nullptr;
        }
        if (node != host.expectedDestroy && (node == host.root || node == host.entry || node == host.context.base
            || node == host.context.parent || node == host.context.screen
            || std::any_of(host.items.begin(), host.items.end(), [&](const Item& item) { return item.widget == node; })))
        {
            ++host.generation;
            host.invalid = true;
            // Abort an in-flight preset burst immediately. Native node/list
            // storage stays deferred, but the shared model's generation must
            // change before another foreign plugin Set can be issued.
            if (!host.modelOpening) host.model.Close();
            host.open = false;
        }
    }
    inline void Collect()
    {
        if (Unwind::depth) return;
        for (auto& value : host.controllers)
            if (value->retired) UnlinkController(*value);
        std::erase_if(host.controllers, [](const auto& value) { return value->retired; });
    }

    inline std::string FontText(const std::string& utf8)
    {
        if (utf8.empty()) return {};
        if (utf8.size() > 8192) return "?";
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), nullptr, 0);
        if (count <= 0 || count > 2048) return "?";
        std::wstring wide(size_t(count), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), wide.data(), count);
        std::string result(size_t(count) * 2, '\0');
        int bytes = WideCharToMultiByte(1252, 0, wide.data(), count, result.data(), int(result.size()), "?", nullptr);
        if (bytes <= 0 || bytes > 2048) return "?";
        result.resize(size_t(bytes));
        for (auto& c : result) if (static_cast<uint8_t>(c) < 32 || c == 127) c = ' ';
        return result;
    }
    inline void DestroyLeaf(uint32_t id)
    {
        auto* node = Lookup(host.context.manager, id);
        if (!node || Read<void*>(node, 4) != host.root) return;
        auto* previous = host.expectedDestroy;
        host.expectedDestroy = node;
        reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(Game().remove)(host.context.manager, id);
        host.expectedDestroy = previous;
        std::erase_if(host.created, [&](const Created& value) { return value.id == id && value.node == node; });
    }

    inline void* Create(Parameters& params, uint32_t id, uint32_t table, uint32_t parentID, void* parent)
    {
        if (Lookup(host.context.manager, id) || Lookup(host.context.manager, parentID) != parent) return nullptr;
        // The native command's CreateAttach branch dereferences an unchecked
        // factory result. Use the independently proved factory return first,
        // validate identity, then issue its ordinary parent/child attachment.
        auto* node = reinterpret_cast<void*(__thiscall*)(void*, Structure*)>(Game().create)(host.context.manager, &params.value);
        if (!Object(node, table) || Read<uint32_t>(node, 52) != id
            || Lookup(host.context.manager, id) != node) return nullptr;
        // Record publication before a callback-capable attachment. Rollback
        // can reclaim an exact registered unattached node as well as children.
        host.created.push_back({ id, table, node, parent });
        if (Lookup(host.context.manager, parentID) != parent) return nullptr;
        Parameters attachment;
        attachment.Name("parent", parentID);
        attachment.Name("child", id);
        reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, Structure*, void*)>(Game().command)(host.context.manager, 0xF1E6688E, &attachment.value, nullptr);
        return Read<void*>(node, 4) == parent ? node : nullptr;
    }
    inline bool DefaultDrawer()
    {
        auto* resources = NativeResources.get();
        if (!resources) return false;
        auto* drawer = reinterpret_cast<void*(__thiscall*)(void*, uint32_t, uint32_t)>(Game().drawer)(resources, uint32_t(-161245377), Checksum("main"));
        if (!drawer) return false;
        // Type compatibility is established by identity with an existing stock
        // text element's drawer, not by assuming an unchecked RTTI cast.
        for (int32_t index = 0; index < 128; ++index)
        {
            auto* child = reinterpret_cast<void*(__thiscall*)(void*, int32_t, uint32_t*)>(Game().childAt)(host.context.base, index, nullptr);
            if (!child) break;
            if (Object(child, Game().textTable) && Read<void*>(child, 452) == drawer
                && Read<void*>(child, 448)) return true;
        }
        return false;
    }
    inline void* Text(uint32_t id, const std::string& label, uint32_t parentID, void* parent, bool enabled,
        bool heading = false, uint32_t link = 0, const std::string& suffix = {})
    {
        if (!parent) return nullptr;
        Parameters params;
        params.Name("type", "textmenuelement");
        params.Name("id", id);
        params.Text("text", FontText(label + suffix));
        params.Name("drawer", "main");
        // The private menu briefly has no children during a rebuild. Its
        // intended width stays fixed while stock entries use their own list.
        const float width = parentID == RootID ? 420.0f : Read<float>(parent, 76);
        if (!std::isfinite(width) || width <= 0.0f || width > 4096.0f) return nullptr;
        params.Float("w", width);
        params.Flag("lock_width");
        if (!enabled || heading) params.Flag("static");
        if (heading) params.Flag("dont_gray");
        if (link) params.Name("link", link);
        auto node = Create(params, id, Game().textTable, parentID, parent);
        if (node) captions[node] = { FontText(label), FontText(suffix) };
        return node;
    }
    inline void RefreshHelp()
    {
        auto created = std::find_if(host.created.begin(), host.created.end(), [](const Created& value) { return value.id == HelpID; });
        auto node = created == host.created.end() ? nullptr : created->node;
        auto found = captions.find(node);
        if (found == captions.end() || !host.root) return;
        std::string text;
        auto selected = Read<uint32_t>(host.root, 420);
        for (auto& item : host.items)
            if (item.id == selected && item.kind == ItemKind::Value) { text = host.model.Help(item.row); break; }
        if (restartPending) text = host.model.Text("menu.restartwarning") + "  " + text;
        if (text.empty()) text = " ";
        text = FontText(text);
        if (found->second.label != text)
        {
            found->second.label = std::move(text);
            found->second.font = nullptr;
        }
    }
    inline void Close(bool nativeBack)
    {
        const bool owned = host.context.PrivateCurrent(host.root);
        host.model.Close();
        host.open = false;
        ++host.generation;
        host.pending.clear();
        host.nativeBackPending = false;
        for (auto& value : host.controllers)
            if (!value->retired) value->generation = host.generation;
        if (nativeBack && owned)
        {
            Busy busy;
            Emit(BackEvent, RootID);
        }
    }
    inline bool SameItem(const Item& a, const Item& b)
    {
        return a.kind == b.kind && a.page == b.page && ((a.kind != ItemKind::Value && a.kind != ItemKind::Details)
            || (a.row.kind == b.row.kind && a.row.plugin == b.row.plugin && a.row.setting == b.row.setting));
    }
    inline bool Populate()
    {
        Snapshot snapshot;
        if (!host.open || !snapshot.Current()) return false;
        std::vector<Item> desired;
        std::string heading = host.model.Text("menu.plugins");
        if (host.details)
        {
            heading = host.model.Text("menu.details");
            const auto end = std::min(host.detailLines.size(), host.detailStart + PageRows);
            for (size_t line = host.detailStart; line < end; ++line)
                desired.push_back({ ItemKind::Text, 0, {}, 0, nullptr, host.detailLines[line], false });
            if (host.detailStart) desired.push_back({ ItemKind::Previous, 0, {}, 0, nullptr, host.model.Text("menu.previous") });
            if (end < host.detailLines.size()) desired.push_back({ ItemKind::Next, 0, {}, 0, nullptr, host.model.Text("menu.next") });
        }
        else if (host.page == RootPage)
        {
            auto count = host.model.Pages().size();
            if (host.start >= count) host.start = 0;
            for (size_t page = host.start; page < std::min(count, host.start + PageRows); ++page)
            {
                Item item;
                item.kind = ItemKind::Page;
                item.page = page;
                item.label = host.model.Pages()[page].title;
                desired.push_back(std::move(item));
            }
            if (host.start) desired.push_back({ ItemKind::Previous, 0, {}, 0, nullptr, host.model.Text("menu.previous") });
            if (host.start + PageRows < count) desired.push_back({ ItemKind::Next, 0, {}, 0, nullptr, host.model.Text("menu.next") });
        }
        else
        {
            if (host.page >= host.model.Pages().size()) return false;
            heading = host.model.Pages()[host.page].title;
            std::vector<const RWGTAMenu::Row*> visible;
            { ForeignScope call; visible = host.model.VisibleRows(host.page); }
            if (!snapshot.Current()) return false;
            std::vector<RWGTAMenu::Row> rows;
            for (auto* row : visible) rows.push_back(*row);
            auto end = std::min(rows.size(), host.rowStart + SettingRows);
            for (size_t i = host.rowStart; i < end; ++i)
            {
                const auto& row = rows[i];
                std::string value;
                { ForeignScope call; value = host.model.Value(row); }
                if (!snapshot.Current()) return false;
                bool enabled;
                { ForeignScope call; enabled = host.model.Enabled(row, host.context.pause); }
                if (!snapshot.Current()) return false;
                auto label = host.model.Label(row);
                if (!snapshot.Current()) return false;
                std::string suffix;
                if (!value.empty()) suffix = ": " + value;
                if (!enabled) suffix += " [" + host.model.Text("menu.unavailable") + "]";
                desired.push_back({ ItemKind::Value, host.page, row, 0, nullptr, std::move(label), enabled, std::move(suffix) });
                auto help = host.model.Help(row);
                if (!snapshot.Current()) return false;
                if (!help.empty())
                    desired.push_back({ ItemKind::Details, host.page, row, 0, nullptr, host.model.Text("menu.details"), true });
            }
            if (host.rowStart) desired.push_back({ ItemKind::Previous, 0, {}, 0, nullptr, host.model.Text("menu.previous") });
            if (end < rows.size()) desired.push_back({ ItemKind::Next, 0, {}, 0, nullptr, host.model.Text("menu.next") });
        }
        desired.push_back({ ItemKind::Back, 0, {}, 0, nullptr, host.model.Text("menu.back") });
        if (desired.size() > PageRows + 3) return false;
        Item selected;
        bool haveSelection = false;
        const auto selectedID = Read<uint32_t>(host.root, 420);
        for (const auto& item : host.items)
            if (item.id == selectedID) { selected = item; haveSelection = true; break; }
        if (host.restoreSelection)
        {
            selected = host.detailItem;
            haveSelection = true;
            host.restoreSelection = false;
        }
        Busy busy;
        ++host.generation;
        snapshot.generation = host.generation;
        host.pending.clear();
        for (auto& controller : host.controllers)
            if (!controller->retired && controller->native.owner == host.root) controller->generation = host.generation;
        auto oldLeaves = std::move(host.leaves);
        for (auto id : oldLeaves)
        {
            DestroyLeaf(id);
            if (!snapshot.Current()) return false;
        }
        host.items.clear();
        constexpr uint32_t headingID = Checksum("wfp_thps3_heading");
        auto* title = Text(headingID, heading, RootID, host.root, false, true);
        if (!title || !snapshot.Current()) { Diagnostic("private heading construction/owner refused"); return false; }
        host.leaves.push_back(headingID);
        for (size_t row = 0; row < desired.size(); ++row)
        {
            auto& item = desired[row];
            item.id = Checksum(("wfp_thps3_row_" + std::to_string(row)).c_str());
            item.widget = Text(item.id, item.kind == ItemKind::Text ? "" : item.label, RootID, host.root, item.enabled,
                false, 0, item.kind == ItemKind::Text ? item.label : item.suffix);
            if (!item.widget || !snapshot.Current()) { Diagnostic("private item construction/owner refused"); return false; }
            host.leaves.push_back(item.id);
            Attach(item.widget, Command::Accept, row);
            if (!snapshot.Current()) return false;
        }
        auto help = Text(HelpID, " ", RootID, host.root, false, true);
        if (!help || !snapshot.Current()) { Diagnostic("private help construction/owner refused"); return false; }
        host.leaves.push_back(HelpID);
        host.items = std::move(desired);
        Emit(PrepareEvent, RootID);
        if (!snapshot.Current()) return false;
        // These children were created after the root's original Show. Prepare
        // enables them, but only Show gives them native visible/render state.
        Emit(ShowEvent, RootID);
        if (!snapshot.Current()) return false;
        uint32_t choice = 0;
        for (const auto& item : host.items)
        {
            if (!item.enabled) continue;
            if (!choice) choice = item.id;
            if (haveSelection && SameItem(item, selected)) { choice = item.id; break; }
        }
        if (choice)
            reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(Game().select)(host.root, choice);
        if (!snapshot.Current()) return false;
        RefreshHelp();
        for (auto id : host.leaves)
        {
            auto* node = Lookup(host.context.manager, id);
            if (!Fit(node))
            {
                const auto reason = std::string("private row layout refused; visible=") + std::to_string(node ? Read<uint8_t>(node, 92) : 0)
                    + ", native mode=" + std::to_string(node ? Read<uint8_t>(node, 475) : 0);
                Diagnostic(reason.c_str());
                return false;
            }
            if (!snapshot.Current()) return false;
        }
        reinterpret_cast<void(__thiscall*)(void*)>(Game().refresh)(host.root);
        return snapshot.Current();
    }
    inline void Open()
    {
        if (host.open || host.modelOpening || !host.context.PrivateCurrent(host.root)) return;
        Snapshot snapshot;
        {
            struct Opening
            {
                Opening() { host.modelOpening = true; }
                ~Opening() { host.modelOpening = false; }
            } opening;
            // Discovery calls foreign FindLoadedPlugins/Subscribe while the
            // shared model builds its member vector. Retire invalidates native
            // ownership immediately, but defers Close for this one operation.
            host.model.OpenPlugins(module, Language(), 4096);
        }
        if (!snapshot.Current()) { host.model.Close(); return; }
        host.open = true;
        host.page = RootPage;
        host.start = 0;
        host.rowStart = 0;
        host.details = host.restoreSelection = false;
        host.detailStart = 0;
        host.detailLines.clear();
        ++host.generation;
        for (auto& value : host.controllers)
            if (!value->retired) value->generation = host.generation;
        if (!UpdateRestart()) return;
        if (!Populate()) { Diagnostic("private row publication refused; native return retained"); Close(true); }
    }
    inline void Activate(size_t index, int direction)
    {
        Snapshot snapshot;
        if (!snapshot.Current() || !host.open || index >= host.items.size()) return;
        Item item = host.items[index];
        if (!item.enabled) return;
        if (item.kind != ItemKind::Value && direction) return;
        switch (item.kind)
        {
            case ItemKind::Page:
                host.page = item.page;
                host.rowStart = 0;
                host.model.PageShown(host.page);
                break;
            case ItemKind::Previous:
                if (host.details) host.detailStart -= std::min(host.detailStart, PageRows);
                else if (host.page == RootPage) host.start -= std::min(host.start, PageRows);
                else host.rowStart -= std::min(host.rowStart, SettingRows);
                break;
            case ItemKind::Next:
                if (host.details) host.detailStart += PageRows;
                else if (host.page == RootPage) host.start += PageRows;
                else host.rowStart += SettingRows;
                break;
            case ItemKind::Back:
                if (host.details)
                {
                    host.details = false;
                    host.restoreSelection = true;
                    break;
                }
                if (host.page == RootPage) { Close(true); return; }
                host.model.PageShown(host.page);
                host.page = RootPage;
                host.rowStart = 0;
                break;
            case ItemKind::Value:
                { ForeignScope call; host.model.Change(item.row, direction, host.context.pause); }
                if (!snapshot.Current()) return;
                if (!UpdateRestart()) return;
                break;
            case ItemKind::Details:
            {
                auto text = host.model.Label(item.row) + ": ";
                if (!snapshot.Current()) return;
                text += host.model.Help(item.row);
                if (!snapshot.Current()) return;
                host.detailItem = item;
                host.detailLines = HelpLines(text);
                if (host.detailLines.empty()) return;
                host.details = true;
                host.detailStart = 0;
                break;
            }
            case ItemKind::Text:
                return;
        }
        if (!Populate()) Close(true);
    }

    inline void Observe()
    {
        if (!host.root || host.busy) return;
        // A transient desktop focus loss is not native owner replacement. The
        // committed owner/selected child checks below do not use screen.active
        // or parent focus bytes; input/actions still require the full predicate.
        auto& context = host.context;
        const bool identity = Manager() == context.manager && context.frontend
            && NativeFrontend.get() == context.frontend
            && !Read<uint32_t>(context.frontend, 60) && Read<void*>(context.frontend, 68) == context.screen
            && Read<uint32_t>(context.frontend, 64) == context.screenID
            && Object(context.parent, Game().containerTable)
            && Object(context.base, Game().menuTable) && Read<uint32_t>(context.base, 52) == context.baseID
            && Object(host.root, Game().menuTable) && Read<uint32_t>(host.root, 52) == RootID
            && Read<void*>(host.root, 4) == context.parent;
        if (!identity)
        {
            ++host.generation;
            host.invalid = true;
            if (!host.modelOpening) host.model.Close();
            host.open = false;
            return;
        }
        if (host.open && (Read<uint32_t>(context.parent, 424) != RootID || !Read<uint8_t>(context.parent, 94))) Close(false);
    }
    inline void CleanupCreated()
    {
        if (Unwind::depth || Manager() != host.context.manager) return;
        // Do not erase the contents of a selected private root before a safe
        // native escape. This remains true behind a modal/inactive window too.
        if (Lookup(host.context.manager, RootID) == host.root && Object(host.root, Game().menuTable)
            && Object(host.context.parent, Game().containerTable)
            && (SelectedChild(host.context.parent) == host.root || Read<uint32_t>(host.context.parent, 424) == RootID)) return;
        // Exact registered identities only, in child-before-parent order. A
        // foreign replacement under the same checksum is never ours to delete.
        auto created = host.created;
        for (auto it = created.rbegin(); it != created.rend(); ++it)
        {
            if (Lookup(host.context.manager, it->id) != it->node
                || !Object(it->node, it->table)) continue;
            auto* parent = Read<void*>(it->node, 4);
            if (parent && parent != it->parent) continue;
            if (it->table == Game().menuTable && parent && SelectedChild(parent) == it->node) continue;
            auto* selected = Object(parent, Game().containerTable) ? SelectedChild(parent) : nullptr;
            const auto selectedID = selected ? Read<uint32_t>(selected, 52) : 0;
            const bool preserveIndex = selected != it->node && Object(selected, Game().menuTable)
                && Read<uint32_t>(parent, 424) == selectedID && Lookup(host.context.manager, selectedID) == selected;
            auto* previous = host.expectedDestroy;
            host.expectedDestroy = it->node;
            reinterpret_cast<void(__thiscall*)(void*, uint32_t)>(Game().remove)(host.context.manager, it->id);
            host.expectedDestroy = previous;
            // Removing our earlier sibling must not shift the current stock
            // child's index. Preserve its exact pointer/logical ID; the native
            // pure lookup supplies the same index used by4CF380 itself.
            if (preserveIndex && Lookup(host.context.manager, ParentID) == parent && Object(parent, Game().containerTable)
                && Read<uint32_t>(parent, 424) == selectedID && Lookup(host.context.manager, selectedID) == selected
                && Object(selected, Game().menuTable) && Read<void*>(selected, 4) == parent)
            {
                int32_t index = -1;
                if (reinterpret_cast<void*(__thiscall*)(void*, uint32_t, int32_t*)>(Game().childById)(parent, selectedID, &index) == selected
                    && index >= 0 && index <= 1024)
                    Write(parent, 420, index);
            }
        }
        std::erase_if(host.created, [](const Created& value) { return Lookup(host.context.manager, value.id) != value.node; });
    }
    inline void Prepare(const Context& context)
    {
        if (!context.StockCurrent()) return;
        if (host.root || host.entry) return;
        host.context = context;
        host.invalid = false;
        host.recovering = false;
        host.baseRetiredWhilePrivate = false;
        ++host.generation;
        if (!DefaultDrawer()) { Diagnostic("stock default drawer/font identity unavailable"); return; }
        if (!context.StockCurrent()) { Diagnostic("stock owner changed before preparation"); return; }
        if (Lookup(context.manager, RootID) || Lookup(context.manager, EntryID)) { Diagnostic("private ID already occupied"); return; }
        Busy busy;
        Parameters params;
        params.Name("type", "verticalmenu");
        params.Name("id", RootID);
        params.Float("x", 110.0f);
        params.Float("y", 60.0f);
        params.Float("w", 420.0f);
        params.Float("h", 400.0f);
        host.root = Create(params, RootID, Game().menuTable, ParentID, context.parent);
        if (!host.root || host.invalid || !context.StockCurrent())
        {
            Diagnostic("private root construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        Attach(host.root, Command::Open);
        Emit(1533424852, RootID);
        constexpr uint32_t placeholder = Checksum("wfp_thps3_loading");
        auto* back = Text(placeholder, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.back"), RootID, host.root, true);
        if (back) host.leaves.push_back(placeholder);
        if (!back || host.invalid || !context.StockCurrent())
        {
            Diagnostic("placeholder construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        auto label = RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins");
        host.entry = Text(EntryID, label, context.baseID, context.base, true);
        if (!host.entry || host.invalid || !context.StockCurrent())
        {
            Diagnostic("Plugins entry construction refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        Attach(host.entry, Command::Enter);
        Emit(ShowEvent, EntryID);
        if (host.invalid || !context.StockCurrent() || !Fit(host.entry, Read<float>(context.base, 76)))
        {
            Diagnostic("Plugins entry fitting refused");
            CleanupCreated();
            host.root = host.entry = nullptr;
            return;
        }
        if (!host.invalid && context.StockCurrent())
            reinterpret_cast<void(__thiscall*)(void*)>(Game().refresh)(context.base);
    }
    inline void Frame()
    {
        if (!installed || Unwind::depth) return;
        Collect();
        if (host.invalid)
        {
            RepairRecoveryIndex();
            if (RecoveryCurrent())
            {
                if (!host.recovering || host.nativeBackPending)
                {
                    host.nativeBackPending = false;
                    Busy busy;
                    Emit(BackEvent, RootID);
                }
                if (RecoveryCurrent())
                {
                    if (!host.recovering)
                    {
                        host.recovering = true;
                        for (auto& controller : host.controllers)
                            if (!controller->retired) controller->generation = host.generation;
                        uint32_t back = 0;
                        for (auto& item : host.items)
                        {
                            if (Lookup(host.context.manager, item.id) != item.widget || !Object(item.widget, Game().textTable)
                                || Read<void*>(item.widget, 4) != host.root) continue;
                            if (item.kind == ItemKind::Back) back = item.id;
                            else { Write(item.widget, 95, uint8_t(1)); Write(item.widget, 93, uint8_t(0)); }
                        }
                        if (back) reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t)>(Game().select)(host.root, back);
                        Log("stock owner retired; native Back retained, settings disabled");
                    }
                    Collect();
                    return;
                }
            }
            if (!host.recovering) Close(false);
            if (Manager() != host.context.manager) host.created.clear();
            CleanupCreated();
            if (host.created.empty())
            {
                host.root = host.entry = nullptr;
                host.items.clear(); host.leaves.clear();
                host.context = {};
                host.invalid = false;
                host.recovering = false;
                host.baseRetiredWhilePrivate = false;
            }
            return;
        }
        if (host.root && !host.context.Alive())
        {
            Observe();
            return;
        }
        if (!host.root)
        {
            static ULONGLONG nextDiscovery = 0;
            const auto now = GetTickCount64();
            if (now < nextDiscovery) return;
            nextDiscovery = now + 250;
            const char* failure = nullptr;
            auto context = Context::Discover(&failure);
            Diagnostic(failure);
            Prepare(context);
            return;
        }
        if (host.enterGeneration)
        {
            const auto generation = host.enterGeneration;
            host.enterGeneration = 0;
            if (generation == host.generation && host.context.StockCurrent()
                && Lookup(host.context.manager, EntryID) == host.entry && Object(host.entry, Game().textTable)
                && Read<void*>(host.entry, 4) == host.context.base && Read<uint32_t>(host.context.base, 420) == EntryID
                && Lookup(host.context.manager, RootID) == host.root && Object(host.root, Game().menuTable)
                && Read<void*>(host.root, 4) == host.context.parent)
            {
                // Navigate only after the Choose dispatch unwinds. The native
                // container owns history, visibility and focus for both pages.
                Busy busy;
                if (!reinterpret_cast<uint8_t(__thiscall*)(void*, uint32_t, uint8_t, uint8_t, uint8_t)>(Game().setFocus)
                    (host.context.parent, RootID, 0, 0, 1)) Diagnostic("native Plugins navigation refused");
            }
        }
        if (!host.context.PrivateCurrent(host.root))
        {
            Observe();
            host.pending.clear();
            return;
        }
        if (host.nativeBackPending) { Close(true); return; }
        if (!host.open) Open();
        if (!host.open || !host.context.PrivateCurrent(host.root)) return;
        auto pending = std::move(host.pending);
        host.pending.clear();
        for (const auto& request : pending)
        {
            if (request.generation != host.generation || request.page != host.page || request.start != host.start || request.rowStart != host.rowStart
                || !host.context.PrivateCurrent(host.root)) continue;
            if ((request.command == Command::Accept || request.command == Command::Left || request.command == Command::Right)
                && (request.row >= host.items.size() || host.items[request.row].widget != request.selectedWidget
                    || host.items[request.row].id != request.selectedID || Read<uint32_t>(host.root, 420) != request.selectedID
                    || !Object(request.selectedWidget, Game().textTable) || !Read<uint8_t>(request.selectedWidget, 94))) continue;
            switch (request.command)
            {
                case Command::Accept: Activate(request.row, 0); break;
                case Command::Left: Activate(request.row, -1); break;
                case Command::Right: Activate(request.row, 1); break;
                case Command::Back:
                    if (host.details)
                    {
                        host.details = false;
                        host.restoreSelection = true;
                        if (!Populate()) Close(true);
                    }
                    else if (host.page == RootPage) Close(true);
                    else
                    {
                        host.model.PageShown(host.page);
                        host.page = RootPage;
                        host.rowStart = 0;
                        if (!Populate()) Close(true);
                    }
                    break;
                default: break;
            }
        }
        if (host.nativeBackPending) { Close(true); return; }
        if (host.open && host.context.PrivateCurrent(host.root) && RWGTAMenu::Model::Changed().exchange(false))
            if (UpdateRestart() && !Populate()) Close(true);
        if (host.open && host.context.PrivateCurrent(host.root))
        {
            Snapshot snapshot;
            RefreshHelp();
        }
        if (UpdateFrame)
            UpdateFrame();
        Collect();
    }
    inline void Init()
    {
        if (installed) return;
        HMODULE pinned = nullptr;
        // Native intrusive controller nodes retain our vtable/function pointers.
        // Pin before any hook or allocation publishes them; dynamic unload is
        // unsupported for this verified host, including an explicit FreeLibrary.
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &pinned);
        module = pinned;
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartBaseline.emplace(std::make_pair(module, setting.id), FixSettings().Get(setting.id));
        Unwind::Install(Frame, Retire, Observe);
        installed = true;
    }
}

namespace THPS3Updates
{
    inline bool Current()
    {
        const auto& host = THPS3Menu::host;
        return host.open && !host.busy && !host.details && !host.foreignDepth && THPS3Menu::Snapshot().Current();
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        if (!THPS3Menu::installed)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TonyHawksProSkater3.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = Current;
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(THPS3Menu::host.model.Text(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            if (!Current())
                return;
            auto& host = THPS3Menu::host;
            host.detailLines = THPS3Menu::HelpLines(asi_settings::text::Narrow(text));
            if (host.detailLines.empty())
                return;
            const auto selected = THPS3Menu::Read<uint32_t>(host.root, 420);
            for (const auto& item : host.items)
            {
                if (item.id == selected)
                    host.detailItem = item;
            }
            host.details = true;
            host.detailStart = 0;
            if (!THPS3Menu::Populate())
                THPS3Menu::Close(true);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        THPS3Menu::UpdateFrame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
}
