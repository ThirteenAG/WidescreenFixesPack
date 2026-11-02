module;

#include <stdafx.h>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <Windows.h>
#include <array>
#include <cmath>
#include <algorithm>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <memory>
#include <map>
#include <set>
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;

export
{
namespace THPS4Menu
{
    // Script structure layout: component type, head at 0, array data/type/count at 0/4/8, pair of 8 bytes.
    // Built only for synchronous calls; the game never frees this arena.
    struct Component
    {
        uint8_t type = 0, flags = 0;
        uint16_t reserved = 0;
        uint32_t name = 0, value = 0;
        Component* next = nullptr;
    };
    struct Structure
    {
        Component* head = nullptr;
        uint32_t flags = 0;
    };
    struct Array
    {
        uint32_t data = 0;
        uint8_t type = 0;
        uint8_t padding[3]{};
        uint32_t count = 0;
    };
    struct Pair { float x = 0, y = 0; };
    struct WeakSlot { void* element = nullptr; WeakSlot* previous = nullptr; WeakSlot* next = nullptr; };
    static_assert(sizeof(Component) == 16 && sizeof(Structure) == 8);
    static_assert(sizeof(Array) == 12 && sizeof(Pair) == 8 && sizeof(WeakSlot) == 12);

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
    static_assert(Checksum("TextElement") == 0x5200DFB6);
    static_assert(Checksum("ContainerElement") == 0x5B9DA842);
    static_assert(Checksum("VMenu") == 0x130EF802);

    class ArrayView
    {
    public:
        Array value;
        explicit ArrayView(uint8_t type, std::vector<uint32_t> values) : items(std::move(values))
        {
            value.type = items.empty() ? 0 : type;
            value.count = static_cast<uint32_t>(items.size());
            value.data = items.size() == 1 ? items.front() : static_cast<uint32_t>(reinterpret_cast<uintptr_t>(items.data()));
        }
        ArrayView(const ArrayView&) = delete;
        ArrayView& operator=(const ArrayView&) = delete;
        ArrayView(ArrayView&&) = delete;
        ArrayView& operator=(ArrayView&&) = delete;
    private:
        std::vector<uint32_t> items;
    };

    class Parameters
    {
    public:
        Structure value;
        Parameters() = default;
        Parameters(const Parameters&) = delete;
        Parameters& operator=(const Parameters&) = delete;
        Parameters(Parameters&&) = delete;
        Parameters& operator=(Parameters&&) = delete;

        void Integer(const char* name, uint32_t data) { Add(1, Checksum(name), data); }
        void Number(const char* name, float data)
        {
            static_assert(sizeof(data) == sizeof(uint32_t));
            uint32_t bits;
            std::memcpy(&bits, &data, sizeof(bits));
            Add(2, Checksum(name), bits);
        }
        void Name(const char* name, uint32_t data) { Add(13, Checksum(name), data); }
        void Name(const char* name, const char* data) { Name(name, Checksum(data)); }
        void Flag(const char* name) { Add(13, 0, Checksum(name)); }
        void Text(const char* name, const std::string& text)
        {
            strings.push_back(text);
            Add(3, Checksum(name), Pointer(strings.back().c_str()));
        }
        void XY(const char* name, float x, float y)
        {
            pairs.push_back({ x, y });
            Add(5, Checksum(name), Pointer(&pairs.back()));
        }
        void Nested(const char* name, Structure* structure) { Add(10, Checksum(name), Pointer(structure)); }
        void List(const char* name, Array* array) { Add(12, Checksum(name), Pointer(array)); }
        static uint32_t Pointer(const void* data) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data)); }

    private:
        std::deque<Component> components;
        std::deque<std::string> strings;
        std::deque<Pair> pairs;
        Component* tail = nullptr;
        void Add(uint8_t type, uint32_t name, uint32_t data)
        {
            components.push_back({ type, 0, 0, name, data, nullptr });
            auto* item = &components.back();
            if (tail) tail->next = item;
            else value.head = item;
            tail = item;
        }
    };

    // SetText47A3D0 preserves both doubled-slash bytes. Only font438D50
    // consumes the escape pair, so setter and metric use the same two bytes.
    inline std::string EscapeNativeText(const std::string& text, bool)
    {
        std::string result;
        result.reserve(text.size());
        for (char c : text)
        {
            if (c == '\\') result.append(1, '\\');
            result += c;
        }
        return result;
    }
}

namespace THPS4Menu
{
    // Reads a field of a game object; a missing object reads as zero
    template<class T> T Read(const void* value, size_t offset = 0)
    {
        T result{};
        if (value)
            std::memcpy(&result, static_cast<const uint8_t*>(value) + offset, sizeof(result));
        return result;
    }

    struct Addresses
    {
        uintptr_t symbol, run, spawn, unlinkWeak, findWeak, alias, setProps, create, destroy, broker, getFont, measure,
            elementDestroyed, source, task;
        uintptr_t textTable, containerTable, menuTable, fontTable;
    };
    inline const Addresses& Game()
    {
        static const Addresses game = []
        {
            auto at = [](const char* bytes) { return reinterpret_cast<uintptr_t>(hook::pattern(bytes).get_first()); };
            auto operand = [](const char* bytes, ptrdiff_t offset) { return *hook::pattern(bytes).get_first<uintptr_t>(offset); };
            Addresses a{};
            a.symbol = at("56 8B 35 ? ? ? ? 57 85 F6 74 7D");
            a.run = at("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 81 EC 0C");
            a.spawn = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 53 8B 5C 24 18 56 57 53");
            a.unlinkWeak = at("8B 44 24 04 56 33 F6 8B 50 04 3B D6");
            a.findWeak = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 10 8B 44 24 24");
            a.alias = at("56 8B 71 0C 8B 46 04 85 C0 75 18 A1");
            a.setProps = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 8B 0D ? ? ? ? 56 8B 74 24 20 6A 01 68 ? ? ? ? 8D 44 24 0C 56 50 E8 ? ? ? ? 8B 4C 24 04 C7");
            a.create = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 1C 53 55 56 57 33");
            a.destroy = at("53 55 56 8B 74 24 10 57 8B 3D ? ? ? ? 68");
            a.broker = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 18 53 8B D9");
            a.getFont = at("A1 ? ? ? ? 85 C0 75 18 A1 ? ? ? ? 50 68 13 01 00 00 68 ? ? ? ? E8 ? ? ? ? 83 C4 0C 8B 0D ? ? ? ? B8 01 00 00 00 D3 E0 8B 4C 24 04 48 23 C1 8D 14 40 A1 ? ? ? ? 8D 04 90 85 C0 74 12 39 08 75 07 8B 50 04 85 D2 75 0A 8B 40 08 85 C0 75 EE 33 C0 C3 8B 40 04 85");
            a.measure = at("8B 01 FF 60 20 90 90 90 90 90 90 90 90 90");
            a.elementDestroyed = injector::GetBranchDestination(hook::pattern("E8 ? ? ? ? 8B 4C 24 08 5E 64 89 0D 00 00 00 00 83 C4 10 C3 90 90 90 51 56").get_first()).as_int();
            a.source = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 18 8B 0D");
            a.task = at("56 8B 74 24 08 85 F6 75 18 A1 ? ? ? ? 50 68 2E");
            NativeLanguage.SetAddress(operand("A1 ? ? ? ? 48 83 F8 07 77 37 FF", 1));
            NativeSymbols.SetAddress(operand("89 35 ? ? ? ? 5E 8B 4C 24 04 64", 2));
            NativeRegistry.SetAddress(operand("8B 0D ? ? ? ? 56 C7 44 24 14 00", 2));
            NativeManager.SetAddress(operand("8B 35 ? ? ? ? 68 ? ? ? ? 53", 2));
            NativeFonts.SetAddress(operand("A1 ? ? ? ? 83 C4 04 85 C0 75 19", 1));
            NativeFontBits.SetAddress(operand("B9 ? ? ? ? 6A 01 50 E8 ? ? ? ? 85", 1));
            NativeFontsLoaded.SetAddress(operand("A0 ? ? ? ? 83 EC 10 84 C0 53 55", 1));
            a.textTable = operand("C7 06 ? ? ? ? 8B 86 5C 01 00 00", 2);
            a.containerTable = operand("C7 06 ? ? ? ? 89 86 44 01 00 00 E8", 2);
            a.menuTable = operand("C7 06 ? ? ? ? E8 ? ? ? ? 8B 4C 24 08", 2);
            a.fontTable = operand("C7 06 ? ? ? ? 8B 46 08 C7 44 24", 2);
            return a;
        }();
        return game;
    }
}

namespace THPS4Menu
{
    struct Event { uint32_t type, source, target, flags; Structure* data; };
    static_assert(sizeof(Event) == 20);
    // All calls require the native UI thread.
    // No owned CFunction is published as an event script:422D00 requires the
    // nonzero script result which4137E0 deliberately does not return for type8.
    inline void* Symbol(uint32_t name)
    {
        if (!NativeSymbols.get()) return nullptr;
        return reinterpret_cast<void* (__cdecl*)(uint32_t)>(Game().symbol)(name);
    }
    inline bool ScriptAvailable(const char* name)
    {
        auto* symbol = Symbol(Checksum(name));
        return Read<uint8_t>(symbol, 1) == 7 && Read<uintptr_t>(symbol, 12) != 0;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!ScriptAvailable(name)) return false;
        //413090's result is allocator cleanup, not a semantic helper result.
        // Only proved no-wait helpers; validate every created native identity.
        reinterpret_cast<int (__cdecl*)(uint32_t, const Structure*, void*, char, const char*)>(Game().run)(Checksum(name), &parameters.value, nullptr, 0, name);
        return true;
    }
    inline bool Spawn(const char* name, const Parameters& parameters, uint32_t id)
    {
        if (!ScriptAvailable(name)) return false;
        return reinterpret_cast<void* (__cdecl*)(uint32_t, const Structure*, uint32_t, const Structure*, uint32_t, uint32_t, char, char, char)>(Game().spawn)(Checksum(name), &parameters.value, 0, nullptr, 0, id, 0, 0, 0) != nullptr;
    }
    class Weak
    {
    public:
        WeakSlot slot;
        Weak() = default;
        Weak(const Weak&) = delete;
        Weak& operator=(const Weak&) = delete;
        Weak(Weak&&) = delete;
        Weak& operator=(Weak&&) = delete;
        ~Weak() { Reset(); }
        void Reset()
        {
            if (slot.element)
                reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(Game().unlinkWeak)(slot.element, &slot);
            slot = {};
        }
        void Find(uint32_t id)
        {
            Reset();
            //4748F0 explicitly replaces ECX with native5CD25C internally,
            // unlike the manager-thiscall named-param lookup4749B0.
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(Game().findWeak)(&slot, id, 0);
            if (slot.element) return;
            // AssignAlias publishes into the registry's separate alias table.
            // Resolve that name as the stock named-param lookup does, then let
            // the ordinary constructor register a weak reference to its ID.
            auto* registry = NativeRegistry.get();
            auto* aliases = Read<void*>(registry, 12);
            if (!aliases || Read<uint32_t>(aliases) > 24 || !Read<void*>(aliases, 4)) return;
            auto* alias = reinterpret_cast<void* (__thiscall*)(void*, uint32_t)>(Game().alias)(registry, id);
            if (!alias) return;
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(Game().findWeak)(&slot, Read<uint32_t>(alias, 20), 0);
            if (slot.element != alias) Reset();
        }
        void* Get() const { return slot.element; }
    };
    inline bool Name(const Structure* parameters, const char* name, uint32_t& value)
    {
        if (!parameters) return false;
        auto* component = parameters->head;
        auto hash = *name ? Checksum(name) : 0;
        for (size_t n = 0; component && n < 256; ++n)
        {
            if (component->type == 13 && component->name == hash) { value = component->value; return true; }
            component = component->next;
        }
        return false;
    }
    inline bool Properties(const Parameters& parameters)
    {
        uint32_t id;
        if (!Name(&parameters.value, "id", id)) return false;
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        return reinterpret_cast<char (__cdecl*)(const Structure*)>(Game().setProps)(&parameters.value) != 0;
    }
    inline bool Destroy(uint32_t id);
    inline bool Create(uint32_t type, uint32_t id, const Parameters& parameters)
    {
        auto* manager = NativeManager.get();
        if (!manager || (type != Checksum("TextElement") && type != Checksum("ContainerElement"))) return false;
        Weak existing, parent;
        existing.Find(id);
        uint32_t parentId;
        if (existing.Get() || !Name(&parameters.value, "parent", parentId)) return false;
        parent.Find(parentId);
        if (!parent.Get()) return false;
        WeakSlot slot;
        reinterpret_cast<WeakSlot* (__thiscall*)(void*, WeakSlot*, uint32_t, uint32_t, const Structure*)>(Game().create)(manager, &slot, type, id, &parameters.value);
        Weak registered;
        registered.Find(id);
        auto exact = slot.element && registered.Get() == slot.element;
        auto result = exact && parent.Get() && Read<uint32_t>(slot.element, 20) == id
            && Read<uint32_t>(slot.element, 24) == type && Read<void*>(slot.element, 64) == parent.Get();
        if (!result && exact) Destroy(id); // Only the exact newly-created node.
        if (slot.element)
            reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(Game().unlinkWeak)(slot.element, &slot);
        return result;
    }
    inline bool Destroy(uint32_t id)
    {
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        Parameters parameters;
        parameters.Name("id", id);
        //475BC0 ->4746E0 ->4754D0 recursion only uses the optional script in
        // guarded non-null4115A0 detachment; engine owns every node deletion.
        return reinterpret_cast<char (__cdecl*)(const Structure*, void*)>(Game().destroy)(&parameters.value, nullptr) != 0;
    }
    inline bool Fire(const char* event, uint32_t target, Structure* data = nullptr)
    {
        auto* manager = NativeRegistry.get();
        Weak object;
        object.Find(target);
        if (!manager || !object.Get()) return false;
        //Direct manager Event dispatch: CFN428070 always dereferences script.
        //427C10 builds a synchronous20-byte event and invokes target virtual16.
        return reinterpret_cast<bool (__thiscall*)(void*, uint32_t, uint32_t, uint32_t, Structure*)>(Game().broker)(manager, Checksum(event), target, target, data);
    }
    struct Extent { float width = 0, height = 0; };
    inline bool Measure(const std::string& literal, Extent& extent)
    {
        if (literal.size() > 4096 || NativeFontBits.get() > 16 || !NativeFonts.get() || !NativeFontsLoaded.get()) return false;
        auto* font = reinterpret_cast<void* (__cdecl*)(uint32_t)>(Game().getFont)(Checksum("small"));
        if (Read<uintptr_t>(font) != Game().fontTable) return false;
        auto* platform = Read<void*>(font, 8);
        if (!platform || !Read<uintptr_t>(platform, 8)) return false;
        auto escaped = EscapeNativeText(literal, false);
        reinterpret_cast<void (__thiscall*)(void*, const char*, float*, float*)>(Game().measure)(font, escaped.c_str(), &extent.width, &extent.height);
        return std::isfinite(extent.width) && std::isfinite(extent.height) && extent.width >= 0 && extent.height > 0;
    }
}

namespace THPS4Menu
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

namespace THPS4Menu
{
    inline SafetyHookInline taskHook, sourceHook, destructorHook, brokerHook;
    inline HMODULE module = nullptr;
    inline bool ready = false, draining = false;
    inline unsigned taskDepth = 0, foreignDepth = 0;
    inline thread_local unsigned nativeDepth = 0;
    struct DepthScope
    {
        unsigned& depth;
        explicit DepthScope(unsigned& value) : depth(value) { ++depth; }
        ~DepthScope() { --depth; }
    };
    struct DrainScope
    {
        bool previous = draining;
        DrainScope() { draining = true; }
        ~DrainScope() { draining = previous; }
    };
    template<class F> auto Foreign(F&& call)
    {
        DepthScope scope(foreignDepth);
        return call();
    }
    inline DWORD nativeThread = 0;
    inline uint32_t nextSerial = 0;
    constexpr uint32_t privateAnchor = Checksum("wfp_thps4_anchor");
    constexpr uint32_t privateMenu = Checksum("wfp_thps4_vmenu");
    constexpr uint32_t entryId = Checksum("wfp_thps4_entry");
    constexpr uint32_t titleId = Checksum("wfp_thps4_title");
    constexpr uint32_t descriptionId = Checksum("wfp_thps4_description");
    constexpr uint32_t restartId = Checksum("wfp_thps4_restart");

    inline void LogOnce(const std::string& message)
    {
        static std::set<std::string> seen;
        if (seen.size() >= 32 || !seen.insert(message).second) return;
        OutputDebugStringA(("THPS4 native menu: " + message + "\n").c_str());
    }
    inline uint32_t Serial()
    {
        nextSerial = (nextSerial + 1) & 0x7FFFFFFF;
        if (!nextSerial) ++nextSerial;
        return nextSerial;
    }
    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        switch (NativeLanguage.get())
        {
            case 1: return "ja";
            case 2: return "en";
            case 3: return "fr";
            case 4: return "es";
            case 5: return "de";
            case 6: return "it";
            case 7: return "nl";
            case 8: return "pt";
            default:
                LogOnce("native language has no verified tag; auto falls back to English");
                return "en";
        }
    }
    inline std::string NativeText(const std::string& utf8)
    {
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, nullptr, 0);
        if (count <= 0 || count > 4096) return "?";
        std::wstring wide(size_t(count), L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(), -1, wide.data(), count)) return "?";
        for (auto& c : wide) if (c < 32 && c) c = L' ';
        // The installed fan font uses the same single-byte Russian glyph slots
        // as its original menu strings. A locale9 tag is still unknown above.
        auto page = NativeLanguage.get() == 9 ? 1251u : 1252u;
        auto bytes = WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, nullptr, 0, "?", nullptr);
        if (bytes <= 0 || bytes > 4096) return "?";
        std::string result(size_t(bytes), '\0');
        if (!WideCharToMultiByte(page, WC_NO_BEST_FIT_CHARS, wide.c_str(), -1, result.data(), bytes, "?", nullptr)) return "?";
        result.pop_back();
        return result;
    }
    template<class MeasureText> std::string FitLiteral(std::string left, std::string suffix, float width, MeasureText measure)
    {
        if (!std::isfinite(width) || width <= 0.0f) return {};
        Extent measured;
        auto full = left + suffix;
        if (!measure(full, measured)) return {};
        if (measured.width <= width) return full;
        if (!measure("..." + suffix, measured) || measured.width > width) return {};
        size_t low = 0, high = left.size();
        while (low < high)
        {
            auto middle = low + (high - low + 1) / 2;
            if (!measure(left.substr(0, middle) + "..." + suffix, measured)) return {};
            if (measured.width <= width) low = middle;
            else high = middle - 1;
        }
        return left.substr(0, low) + "..." + suffix;
    }
    inline std::string Fit(const std::string& label, const std::string& value, float width)
    {
        return FitLiteral(NativeText(label), value.empty() ? std::string() : ": " + NativeText(value), width, Measure);
    }

    struct Host
    {
        Weak anchor, menu, window, entry;
        uint32_t serial = 0;
        bool pause = false;
    };
    enum class Kind { Link, Previous, Next, Row, Back, Details, Text };
    struct Item { Kind kind; size_t index = 0; bool enabled = true; std::string label, value, description; uint32_t id = 0; std::shared_ptr<Weak> native; };
    struct Context
    {
        RWGTAMenu::Model model;
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false, retired = false, rebuilding = false, constructing = false, returnPending = false;
        size_t page = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
        std::string language;
        std::vector<Item> items;
        bool details = false;
        size_t detailStart = 0, detailSelection = 0;
        std::vector<std::string> detailLines;
    };
    inline std::unique_ptr<Host> host;
    inline std::shared_ptr<Context> context;
    enum class Action { Open, Accept, Left, Right, Back, Focus, Unfocus };
    struct Pending { Action action; uint32_t serial; size_t index; };
    inline std::deque<Pending> pending;
    inline std::function<void()> UpdateFrame;
    inline bool focusEntry = false;

    inline bool Modal()
    {
        // Test actual native modal anchors; ordinary menu presence is allowed.
        for (auto name : { "keyboard_anchor", "dialog_box_anchor", "quit_dialog_anchor" })
        {
            Weak object;
            object.Find(Checksum(name));
            if (object.Get()) return true;
        }
        return false;
    }
    inline bool NativeOwned(const std::shared_ptr<Context>& owner)
    {
        if (!owner || context != owner || !owner->anchor.Get() || !owner->menu.Get() || !owner->window.Get()) return false;
        Weak window;
        window.Find(Checksum("root_window"));
        return window.Get() == owner->window.Get()
            && Read<void*>(owner->anchor.Get(), 64) == owner->window.Get()
            && Read<void*>(owner->menu.Get(), 64) == owner->anchor.Get();
    }
    inline bool Owned(const std::shared_ptr<Context>& owner)
    {
        return NativeOwned(owner) && !owner->retired && owner->model.IsOpen();
    }
    inline bool Current(const std::shared_ptr<Context>& owner)
    {
        if (!Owned(owner) || Modal()) return false;
        Weak menu, anchor;
        menu.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        return menu.Get() == owner->menu.Get() && anchor.Get() == owner->anchor.Get();
    }
    inline void Retire(const std::shared_ptr<Context>& owner);
    inline bool Queue(Action action, uint32_t serial, size_t index = 0)
    {
        if (!ready || nativeThread != GetCurrentThreadId() || pending.size() >= 32) return false;
        pending.push_back({ action, serial, index });
        return true;
    }
    inline bool Active(const std::shared_ptr<Context>& owner, size_t index);
    inline char __fastcall ControlEvent(void* object, void*, Event* event);
    struct ControlTable
    {
        uintptr_t original;
        // Keep all nine native slots and the exact preceding original word.
        // Storage remains process-long even after a logical Context retires.
        std::array<uintptr_t, 10> words{};
        uintptr_t Pointer() const { return reinterpret_cast<uintptr_t>(words.data() + 1); }
        bool Prepare()
        {
            std::memcpy(words.data(), reinterpret_cast<void*>(original - 4), sizeof(words));
            words[5] = reinterpret_cast<uintptr_t>(&ControlEvent); // vfptr+16
            return true;
        }
        bool Bind(void* object) const
        {
            if (Read<uintptr_t>(object) != original) return false;
            *reinterpret_cast<uintptr_t*>(object) = Pointer();
            return true;
        }
        char Delegate(void* object, Event* event) const
        {
            return reinterpret_cast<char (__thiscall*)(void*, Event*)>(Read<uintptr_t>(reinterpret_cast<void*>(original), 16))(object, event);
        }
    };
    inline ControlTable textTable{}, containerTable{}, menuTable{};

    inline void Color(uint32_t id, bool selected, bool enabled)
    {
        // Native skutils do_scale_up/down palette, without their spawned blur
        // loop. These remain the game's own text-control colors and renderer.
        ArrayView rgba(1, enabled ? selected ? std::vector<uint32_t>{127,102,0,100} : std::vector<uint32_t>{88,105,112,128} : std::vector<uint32_t>{70,70,70,120});
        Parameters parameters;
        parameters.Name("id", id);
        parameters.List("rgba", &rgba.value);
        Properties(parameters);
    }

    inline void Retire(const std::shared_ptr<Context>& owner)
    {
        if (!owner || owner->retired) return;
        owner->retired = true;
        // Logical retirement is safe even inside a foreign API callback: the
        // shared Model holds its plugin snapshot and aborts later preset sets.
        // Native controls are destroyed only after the outer task has returned.
        owner->model.Close();
    }
    inline void __fastcall ElementDestroyed(void* object, void*)
    {
        DepthScope native(nativeDepth);
        auto owner = context;
        if (owner && (object == owner->window.Get() || (!owner->rebuilding && (object == owner->anchor.Get() || object == owner->menu.Get())))) Retire(owner);
        destructorHook.thiscall<void>(object);
    }
    // A long caption line gets a little smaller first (evenly, never squeezed), then shortened to 440 units
    inline std::string FitCaption(const std::string& text, float& scale)
    {
        Extent full;
        if (!text.empty() && Measure(NativeText(text), full) && full.width * scale > 440)
            scale = std::max(scale * 0.75f, 440 / full.width);
        return Fit(text, "", 440 / scale);
    }
    inline bool Caption(uint32_t id, const std::string& text, float x, float y, float scale)
    {
        auto native = FitCaption(text, scale);
        Extent extent;
        if ((!text.empty() && native.empty()) || !Measure(native, extent)) return false;
        ArrayView just(13, { Checksum("left"), Checksum("center") });
        Parameters parameters;
        parameters.Name("parent", privateAnchor);
        parameters.Name("font", "small");
        parameters.Text("text", EscapeNativeText(native, true));
        parameters.XY("pos", x, y);
        parameters.List("just", &just.value);
        parameters.XY("scale", scale, scale);
        parameters.Flag("not_focusable");
        return Create(Checksum("TextElement"), id, parameters);
    }
    // Startup settings as the game started with them: the restart line shows while one of them differs
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartBaseline;
    inline bool RestartPending(const std::shared_ptr<Context>& owner)
    {
        bool pending = false;
        for (const auto& page : owner->model.Pages())
        {
            for (const auto& row : page.rows)
            {
                if (!row.plugin || !row.setting || row.setting->apply != "restart" || !row.setting->HasValue())
                    continue;
                auto value = Foreign([&] { return OtherPlugins::Get(*row.plugin, *row.setting); });
                auto baseline = restartBaseline.emplace(std::make_pair(row.plugin->module, row.setting->id), value).first;
                pending |= !asi_settings::text::IEquals(baseline->second, value);
            }
        }
        return pending;
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner)) return false;
        RWGTAMenu::Model::Changed().exchange(false);
        std::vector<Item> items;
        std::string heading = owner->model.Text("menu.plugins");
        if (owner->details)
        {
            heading = owner->model.Text("menu.details");
            const auto end = std::min(owner->detailLines.size(), owner->detailStart + 5);
            for (size_t i = owner->detailStart; i < end; ++i)
                items.push_back({ Kind::Text, 0, false, owner->detailLines[i] });
            if (owner->detailStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (end < owner->detailLines.size()) items.push_back({ Kind::Next, 5, true, owner->model.Text("menu.next") });
        }
        else if (owner->page == SIZE_MAX)
        {
            auto count = std::min(size_t(5), owner->model.Pages().size() - std::min(owner->rootStart, owner->model.Pages().size()));
            for (size_t i = 0; i < count; ++i) items.push_back({ Kind::Link, owner->rootStart + i, true, owner->model.Pages()[owner->rootStart + i].title });
            if (owner->rootStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (owner->rootStart + count < owner->model.Pages().size()) items.push_back({ Kind::Next, count, true, owner->model.Text("menu.next") });
        }
        else
        {
            if (owner->page >= owner->model.Pages().size()) return false;
            heading = owner->model.Pages()[owner->page].title;
            auto rows = Foreign([&] { return owner->model.VisibleRows(owner->page); });
            if (!Current(owner)) return false;
            auto end = std::min(rows.size(), owner->rowStart + 3);
            for (size_t i = owner->rowStart; i < end; ++i)
            {
                auto row = *rows[i];
                auto enabled = Foreign([&] { return owner->model.Enabled(row, owner->pause); });
                if (!Current(owner)) return false;
                auto label = owner->model.Label(row);
                if (!Current(owner)) return false;
                auto value = enabled ? Foreign([&] { return owner->model.Value(row); }) : owner->model.Text("menu.unavailable");
                if (!Current(owner)) return false;
                auto description = owner->model.Help(row);
                if (!Current(owner)) return false;
                items.push_back({ Kind::Row, i, enabled, std::move(label), std::move(value), std::move(description) });
                if (!items.back().description.empty())
                    items.push_back({ Kind::Details, i, true, owner->model.Text("menu.details") });
            }
            if (owner->rowStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (end < rows.size()) items.push_back({ Kind::Next, 3, true, owner->model.Text("menu.next") });
        }
        items.push_back({ Kind::Back, 0, true, owner->model.Text("menu.back") });
        const bool restart = RestartPending(owner);
        if (!Current(owner) || items.empty() || items.size() > 10) return false;
        owner->serial = Serial(); // Row-pool generation rejects queued old-page input.
        owner->rebuilding = true;
        owner->constructing = true;
        auto oldAnchor = Read<uint32_t>(owner->anchor.Get(), 20);
        Destroy(oldAnchor);
        if (context != owner || owner->retired || !owner->window.Get() || Modal())
        {
            owner->rebuilding = false;
            return false;
        }
        Weak replacement;
        replacement.Find(Checksum("current_menu_anchor"));
        if (replacement.Get())
        {
            owner->rebuilding = false;
            return false;
        }
        for (auto id : { privateAnchor, privateMenu, titleId, descriptionId, restartId })
        {
            Weak occupied;
            occupied.Find(id);
            if (occupied.Get()) return false;
        }
        Parameters root;
        root.Name("menu_id", privateAnchor);
        root.Name("vmenu_id", privateMenu);
        root.Name("parent", "root_window");
        root.XY("pos", 120, 108);
        root.XY("dims", 440, 280);
        root.Text("menu_title", "");
        auto issued = Run("make_new_menu", root);
        owner->anchor.Find(privateAnchor);
        owner->menu.Find(privateMenu);
        owner->rebuilding = false;
        if (!issued || !Current(owner) || Read<uint32_t>(owner->menu.Get(), 24) != Checksum("VMenu") || Read<uintptr_t>(owner->menu.Get()) != Game().menuTable) return false;
        // Install escape before population; a later font/row failure can still
        // return through the native parent even with only a partial page.
        if (!containerTable.Bind(owner->anchor.Get()) || !menuTable.Bind(owner->menu.Get())) return false;
        owner->items = std::move(items);
        owner->selected = std::min(owner->selected, owner->items.size() - 1);
        if (owner->details && !owner->items[owner->selected].enabled)
        {
            const auto selected = std::find_if(owner->items.begin(), owner->items.end(), [](const Item& item) { return item.enabled; });
            if (selected == owner->items.end()) return false;
            owner->selected = static_cast<size_t>(selected - owner->items.begin());
        }
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            auto idText = "wfp_thps4_row" + std::to_string(i);
            item.id = Checksum(idText.c_str());
            auto fitted = item.kind == Kind::Text
                ? FitLiteral("", NativeText(item.label), 440, Measure) : Fit(item.label, item.value, 440);
            Extent extent;
            if (fitted.empty() || !Measure(fitted, extent)) { LogOnce("complete native row value cannot fit"); return false; }
            auto xScale = extent.width > 440 ? 440 / extent.width : 1.0f;
            Weak occupied;
            occupied.Find(item.id);
            if (occupied.Get()) return false;
            Parameters row;
            row.Name("id", item.id);
            row.Name("parent_menu_id", privateMenu);
            row.Name("focus_script", "nullscript");
            row.Name("unfocus_script", "nullscript");
            row.Name("pad_choose_script", "nullscript");
            row.Text("text", EscapeNativeText(fitted, true));
            row.Flag("no_choose_sound");
            if (item.kind == Kind::Text) row.Flag("not_focusable");
            auto issued = Run("make_text_sub_menu_item", row);
            Weak created;
            created.Find(item.id);
            if (!issued || !created.Get() || Read<uintptr_t>(created.Get()) != Game().textTable || Read<uint32_t>(created.Get(), 24) != Checksum("TextElement")
                || Read<void*>(created.Get(), 64) != owner->menu.Get())
            {
                if (created.Get() && Read<uint32_t>(created.Get(), 20) == item.id) Destroy(item.id);
                return false;
            }
            if (!Current(owner)) return false;
            Parameters scale;
            scale.Name("id", item.id);
            scale.XY("scale", xScale, xScale);
            if (item.kind == Kind::Text) scale.Flag("not_focusable");
            if (!Properties(scale)) return false;
            item.native = std::make_shared<Weak>();
            item.native->Find(item.id);
            if (!textTable.Bind(item.native->Get())) return false;
            Color(item.id, i == owner->selected, item.enabled);
        }
        if (!Caption(titleId, heading, 120, 78, 1.2f) || !Current(owner)
            || !Caption(descriptionId, owner->items[owner->selected].description, 120, 392, 0.8f) || !Current(owner)
            || !Caption(restartId, restart ? owner->model.Text("menu.restartwarning") : " ", 120, 420, 0.8f)
            || !Current(owner)) return false;
        Parameters focus;
        focus.Name("child_id", owner->items[owner->selected].id);
        if (!Fire("focus", privateMenu, &focus.value) || !Current(owner)) return false;
        owner->constructing = false;
        return true;
    }

    inline bool Active(const std::shared_ptr<Context>& owner, size_t index)
    {
        return Current(owner) && !Modal() && index < owner->items.size()
            && owner->items[index].native && owner->items[index].native->Get()
            && Read<uint32_t>(owner->items[index].native->Get(), 20) == owner->items[index].id
            && Read<void*>(owner->items[index].native->Get(), 64) == owner->menu.Get()
            && Read<uint8_t>(owner->menu.Get(), 332) != 0
            && Read<uint32_t>(owner->menu.Get(), 328) == owner->items[index].id;
    }
    inline bool QueueBack(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner) || Read<uint8_t>(owner->menu.Get(), 332) == 0) return false;
        auto selected = Read<uint32_t>(owner->menu.Get(), 328);
        auto alive = std::any_of(owner->items.begin(), owner->items.end(), [&](const Item& item) {
            return item.id == selected && item.native && item.native->Get()
                && Read<uint32_t>(item.native->Get(), 20) == selected
                && Read<void*>(item.native->Get(), 64) == owner->menu.Get();
        });
        if (!alive || !Queue(Action::Back, owner->serial)) return false;
        if (foreignDepth)
        {
            // Accepted Back must abort an in-flight getter/preset API burst.
            // Only logical cancellation happens here: no native UI deletion,
            // factory, restoration or vtable mutation until the outer task.
            owner->returnPending = true;
            Retire(owner);
        }
        return true;
    }
    inline char __fastcall ControlEvent(void* object, void*, Event* event)
    {
        auto pointer = Read<uintptr_t>(object);
        const ControlTable* table = pointer == textTable.Pointer() ? &textTable
            : pointer == containerTable.Pointer() ? &containerTable
            : pointer == menuTable.Pointer() ? &menuTable : nullptr;
        if (!table) return 1; // Native destruction already reset its vtable.
        if (!ready || nativeThread != GetCurrentThreadId() || !event) return table->Delegate(object, event);
        Weak lifetime;
        lifetime.Find(Read<uint32_t>(object, 20));
        auto handled = [&]() { event->flags |= 2; return char(lifetime.Get() == object); };
        if (host && object == host->entry.Get()
            && (event->type == Checksum("pad_choose") || event->type == Checksum("pad_start")))
        {
            return Queue(Action::Open, host->serial) ? handled() : table->Delegate(object, event);
        }
        auto owner = context;
        if (!Current(owner) || owner->rebuilding || owner->constructing) return table->Delegate(object, event);
        if (object == owner->anchor.Get() || object == owner->menu.Get())
        {
            if (event->type == Checksum("pad_back") && Read<uint8_t>(owner->menu.Get(), 332))
            {
                return QueueBack(owner) ? handled() : table->Delegate(object, event);
            }
            return table->Delegate(object, event); // Original VMenu navigation/focus.
        }
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            if (!item.native || item.native->Get() != object) continue;
            Action action;
            if (event->type == Checksum("unfocus")) action = Action::Unfocus;
            else
            {
                if (!Active(owner, i)) return table->Delegate(object, event);
                if (event->type == Checksum("focus")) action = Action::Focus;
                else if (event->type == Checksum("pad_choose") || event->type == Checksum("pad_start")) action = Action::Accept;
                else if (event->type == Checksum("pad_left")) action = Action::Left;
                else if (event->type == Checksum("pad_right")) action = Action::Right;
                else if (event->type == Checksum("pad_back")) action = Action::Back;
                else return table->Delegate(object, event);
            }
            if (action == Action::Accept && item.kind == Kind::Back) action = Action::Back;
            if (!(action == Action::Back ? QueueBack(owner) : Queue(action, owner->serial, i))) return table->Delegate(object, event);
            // Never pass handled owned actions to423830's native script handler.
            // Only primitives are queued; accepted Back during a foreign API
            // may logically cancel Model. Native UI mutation remains deferred.
            return handled();
        }
        return table->Delegate(object, event);
    }
    inline void Entry(Structure* parameters)
    {
        if (!ready || nativeThread != GetCurrentThreadId() || context) return;
        uint32_t id = 0;
        if (!Name(parameters, "id", id)) return;
        const bool optionsCall = id == Checksum("ss_setup_options_menu") || id == Checksum("options_menu");
        uint32_t scriptName = 0;
        if (!Name(parameters, "", scriptName) || scriptName != Checksum("animate_in"))
        {
            if (optionsCall) LogOnce("Options dispatch has no inline animate_in checksum");
            return;
        }
        Weak currentAnchor;
        currentAnchor.Find(Checksum("current_menu_anchor"));
        if (!currentAnchor.Get()) { if (optionsCall) LogOnce("Options current anchor unavailable"); return; }
        auto sourceId = Read<uint32_t>(currentAnchor.Get(), 20);
        auto pause = sourceId == Checksum("options_menu");
        auto title = sourceId == Checksum("ss_setup_options_menu");
        if ((!pause && !title) || (id != sourceId && id != Checksum("current_menu_anchor")))
        { if (optionsCall) LogOnce("Options animate_in anchor identity unavailable"); return; }
        if (!ScriptAvailable("make_new_menu") || !ScriptAvailable("make_text_sub_menu_item")
            || !ScriptAvailable("nullscript") || !ScriptAvailable(title ? "main_menu_add_item" : "make_text_sub_menu_item"))
        { LogOnce("Options row helper scripts unavailable"); return; }
        Weak existing;
        existing.Find(entryId);
        if (existing.Get()) return;
        auto source = std::make_unique<Host>();
        source->menu.Find(title ? Checksum("ss_setup_options_vmenu") : Checksum("options_vmenu"));
        source->anchor.Find(sourceId);
        source->window.Find(Checksum("root_window"));
        if (!source->menu.Get() || !source->anchor.Get() || !source->window.Get()
            || Read<uint32_t>(source->menu.Get(), 24) != Checksum("VMenu")
            || Read<uintptr_t>(source->menu.Get()) != Game().menuTable || Modal())
        { LogOnce("Options native menu/window/type authority unavailable"); return; }
        Weak alias;
        alias.Find(Checksum("current_menu"));
        if (alias.Get() != source->menu.Get() || currentAnchor.Get() != source->anchor.Get())
        { LogOnce("Options current menu alias unavailable"); return; }
        auto sourceAnchor = source->anchor.Get();
        auto sourceMenu = source->menu.Get();
        auto sourceWindow = source->window.Get();
        source->serial = Serial();
        source->pause = pause;
        Parameters row;
        row.Name("id", entryId);
        auto label = Fit(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"), "", title ? 190.0f : 440.0f);
        Extent extent;
        if (!Measure(label, extent)) { LogOnce("Options entry font measurement unavailable"); return; }
        row.Text("text", EscapeNativeText(label, true));
        auto available = title ? 190.0f : 440.0f;
        const float entryScale = extent.width > available ? available / extent.width : 1.0f;
        row.XY("scale", entryScale, entryScale);
        row.Name("pad_choose_script", "nullscript");
        row.Flag("no_sound");
        if (!title) row.Name("parent_menu_id", Read<uint32_t>(sourceMenu, 20));
        auto issued = Run(title ? "main_menu_add_item" : "make_text_sub_menu_item", row);
        source->entry.Find(entryId);
        alias.Find(Checksum("current_menu"));
        currentAnchor.Find(Checksum("current_menu_anchor"));
        Weak window;
        window.Find(Checksum("root_window"));
        auto sameSource = source->anchor.Get() == sourceAnchor && source->menu.Get() == sourceMenu
            && source->window.Get() == sourceWindow && currentAnchor.Get() == sourceAnchor
            && alias.Get() == sourceMenu && window.Get() == sourceWindow && !Modal()
            && Read<void*>(sourceAnchor, 64) == sourceWindow && Read<void*>(sourceMenu, 64) == sourceAnchor;
        auto correctEntry = source->entry.Get() && Read<uint32_t>(source->entry.Get(), 20) == entryId
            && Read<uint32_t>(source->entry.Get(), 24) == Checksum(title ? "ContainerElement" : "TextElement")
            && Read<void*>(source->entry.Get(), 64) == sourceMenu;
        if (!issued || !sameSource || !correctEntry || !((title ? containerTable : textTable).Bind(source->entry.Get())))
        {
            // The id was absent before this synchronous helper. Remove only
            // its exact observed result; never leave a visible inert category.
            if (source->entry.Get() && Read<uint32_t>(source->entry.Get(), 20) == entryId) Destroy(entryId);
            LogOnce("native Advanced Options entry rejected");
            return;
        }
        host = std::move(source);
    }
    inline char __cdecl Source(Structure* parameters, void* script)
    {
        // Keep complete wrapper/helper dispatch above a reentrant Task alive.
        DepthScope native(nativeDepth);
        // Exact source roots' final animate-in call: append before original.
        Entry(parameters);
        const auto result = sourceHook.call<char>(parameters, script);
        // Back from Plugins: the rebuilt Options page selects the Plugins row again
        if (focusEntry && host && host->entry.Get() && host->menu.Get())
        {
            focusEntry = false;
            Parameters focus;
            focus.Name("child_id", entryId);
            Fire("focus", Read<uint32_t>(host->menu.Get(), 20), &focus.value);
        }
        return result;
    }
    inline char __fastcall Broker(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* data)
    {
        //427C10 continues waiting-script notifications after target virtual16.
        // A reentrant503960 must not drain/delete while that traversal resumes.
        DepthScope native(nativeDepth);
        return brokerHook.thiscall<char>(manager, type, target, source, data);
    }
    inline void Cleanup(const std::shared_ptr<Context>& owner, bool restore)
    {
        if (!owner) return;
        if (Modal() && NativeOwned(owner))
        {
            // A covering native modal may be opened by a foreign getter. Keep
            // its underlying controls and logical owner intact; rebuild after
            // focus returns rather than treating suspension as commit failure.
            if (owner->retired) owner->returnPending = true;
            else RWGTAMenu::Model::Changed() = true;
            return;
        }
        Weak current, window;
        current.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        auto own = current.Get() && current.Get() == owner->anchor.Get();
        auto gap = owner->constructing && !current.Get();
        // Issue the waiting parent script before retiring the usable owner.
        // A constructing gap is ours only while the exact native window remains
        // current and no foreign anchor/modal has replaced it.
        if (restore && (own || gap) && owner->window.Get() && window.Get() == owner->window.Get() && !Modal())
        {
            Parameters parameters;
            if (!Spawn(owner->pause ? "create_options_menu" : "create_setup_options_menu", parameters, Checksum("wfp_thps4_return")))
            {
                LogOnce("stock Options asynchronous return rejected");
                owner->returnPending = true;
                return; // Retain context/Back and retry after the next outer task.
            }
            focusEntry = true;
            if (gap && !Modal())
            {
                Weak partial, alias;
                partial.Find(privateAnchor);
                alias.Find(Checksum("current_menu_anchor"));
                if (!alias.Get() && partial.Get() && partial.Get() == owner->anchor.Get()) Destroy(privateAnchor);
            }
        }
        Retire(owner);
        if (owner->anchor.Get() && !Modal())
        {
            Weak privateRoot;
            privateRoot.Find(privateAnchor);
            if (privateRoot.Get() == owner->anchor.Get()) Destroy(privateAnchor);
        }
        if (context == owner) context.reset();
    }
    inline void Open(uint32_t serial)
    {
        if (context || !host || host->serial != serial || !host->anchor.Get() || !host->menu.Get() || !host->window.Get() || Modal()) return;
        Weak current, window, anchor;
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (current.Get() != host->menu.Get() || anchor.Get() != host->anchor.Get() || window.Get() != host->window.Get()
            || Read<uint8_t>(host->menu.Get(), 332) == 0 || Read<uint32_t>(host->menu.Get(), 328) != entryId
            || !ScriptAvailable(host->pause ? "create_options_menu" : "create_setup_options_menu")) return;
        auto sourceMenu = host->menu.Get();
        auto sourceAnchor = host->anchor.Get();
        auto sourceWindow = host->window.Get();
        auto owner = std::make_shared<Context>();
        owner->serial = Serial();
        owner->pause = host->pause;
        owner->language = Language();
        owner->model.OpenPlugins(module, owner->language, 4096);
        if (!owner->model.IsOpen()) return; // Discovery may logically cancel.
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (!host || host->serial != serial || host->menu.Get() != sourceMenu || host->anchor.Get() != sourceAnchor || host->window.Get() != sourceWindow || current.Get() != sourceMenu || anchor.Get() != sourceAnchor || window.Get() != sourceWindow || Modal()
            || Read<uint8_t>(sourceMenu, 332) == 0 || Read<uint32_t>(sourceMenu, 328) != entryId)
        {
            owner->model.Close();
            return;
        }
        owner->anchor.Find(Read<uint32_t>(sourceAnchor, 20));
        owner->menu.Find(Read<uint32_t>(sourceMenu, 20));
        owner->window.Find(Checksum("root_window"));
        context = owner;
        if (!Build(owner))
        {
            LogOnce("private page commit rejected");
            Cleanup(owner, true);
            return;
        }
    }
    inline void Act(const Pending& action)
    {
        if (action.action == Action::Open) { Open(action.serial); return; }
        auto owner = context;
        if (!owner || owner->serial != action.serial) return;
        if (action.action == Action::Back)
        {
            if (owner->retired) { Cleanup(owner, true); return; }
            if (!Current(owner) || Modal() || Read<uint8_t>(owner->menu.Get(), 332) == 0) return;
            if (owner->details)
            {
                owner->details = false;
                owner->selected = owner->detailSelection;
                if (!Build(owner)) Cleanup(owner, true);
            }
            else if (owner->page != SIZE_MAX)
            {
                owner->model.PageShown(owner->page);
                owner->page = SIZE_MAX;
                owner->rowStart = 0;
                owner->selected = 0;
                if (!Build(owner)) Cleanup(owner, true);
            }
            else Cleanup(owner, true);
            return;
        }
        if (!Current(owner) || Modal() || action.index >= owner->items.size()) return;
        auto item = owner->items[action.index];
        if (!item.native || !item.native->Get() || Read<uint32_t>(item.native->Get(), 20) != item.id
            || Read<void*>(item.native->Get(), 64) != owner->menu.Get()) return;
        if (action.action == Action::Unfocus) { Color(item.id, false, item.enabled); return; }
        if (!Active(owner, action.index)) return;
        owner->selected = action.index;
        if (action.action == Action::Focus)
        {
            Color(item.id, true, item.enabled);
            Parameters description;
            description.Name("id", descriptionId);
            float scale = 0.8f;
            auto fitted = FitCaption(item.description, scale);
            Extent extent;
            if (!Measure(fitted, extent)) return;
            description.Text("text", EscapeNativeText(fitted, true));
            description.XY("scale", scale, scale);
            Properties(description);
            return;
        }
        if (!item.enabled) return;
        auto accept = action.action == Action::Accept;
        if (item.kind != Kind::Row && !accept) return;
        if (item.kind == Kind::Back)
        {
            Pending back{ Action::Back, action.serial, 0 };
            Act(back);
            return;
        }
        if (item.kind == Kind::Link)
        {
            owner->page = item.index;
            owner->rowStart = 0;
            owner->selected = 0;
            owner->model.PageShown(item.index);
        }
        else if (item.kind == Kind::Previous)
        {
            if (owner->details) owner->detailStart -= std::min(owner->detailStart, size_t(5));
            else if (owner->page == SIZE_MAX) owner->rootStart = owner->rootStart >= 5 ? owner->rootStart - 5 : 0;
            else owner->rowStart -= std::min(owner->rowStart, size_t(3));
            owner->selected = 0;
        }
        else if (item.kind == Kind::Next)
        {
            if (owner->details) owner->detailStart += item.index;
            else if (owner->page == SIZE_MAX) owner->rootStart += item.index;
            else owner->rowStart += item.index;
            owner->selected = 0;
        }
        else if (item.kind == Kind::Details)
        {
            auto rows = Foreign([&] { return owner->model.VisibleRows(owner->page); });
            if (!Active(owner, action.index) || item.index >= rows.size()) return;
            const auto row = *rows[item.index];
            auto text = owner->model.Label(row) + ": ";
            if (!Current(owner)) return;
            text += owner->model.Help(row);
            if (!Current(owner)) return;
            owner->detailLines = HelpLines(text);
            if (owner->detailLines.empty()) return;
            owner->details = true;
            owner->detailStart = 0;
            owner->detailSelection = action.index;
            owner->selected = 0;
        }
        else if (item.kind == Kind::Text)
            return;
        else
        {
            auto rows = Foreign([&] { return owner->model.VisibleRows(owner->page); });
            if (!Active(owner, action.index) || item.index >= rows.size()
                || !item.native || item.native->Get() != owner->items[action.index].native->Get()) return;
            auto row = *rows[item.index];
            if (!Foreign([&] { return owner->model.Enabled(row, owner->pause); }) || !Active(owner, action.index)
                || !item.native || item.native->Get() != owner->items[action.index].native->Get()) return;
            if (!accept && row.kind != RWGTAMenu::RowKind::Setting && row.kind != RWGTAMenu::RowKind::Preset) return;
            Foreign([&] { return owner->model.Change(row, accept ? 0 : action.action == Action::Left ? -1 : 1, owner->pause); });
            if (!Current(owner)) return;
        }
        if (!Build(owner)) Cleanup(owner, true);
    }
    // The title Options panel is sized for the stock rows; make its background, frame and middle part one row taller
    // for the added Plugins row (the values the script gives the console versions for their extra rows)
    inline void GrowOptionsPanel()
    {
        static bool done = false;
        if (done || !NativeSymbols.get())
            return;
        auto* symbol = Symbol(Checksum("create_setup_options_menu"));
        if (Read<uint8_t>(symbol, 1) != 7 || !Read<uint8_t*>(symbol, 12))
            return;
        done = true;
        auto pair = [](float x, float y)
        {
            std::vector<uint8_t> tokens(9, 0x1F);
            std::memcpy(&tokens[1], &x, sizeof(x));
            std::memcpy(&tokens[5], &y, sizeof(y));
            return tokens;
        };
        auto count = [](const char* name, int value)
        {
            std::vector<uint8_t> tokens{ 0x16, 0, 0, 0, 0, 0x07, 0x17, 0, 0, 0, 0 };
            const auto id = Checksum(name);
            std::memcpy(&tokens[1], &id, sizeof(id));
            std::memcpy(&tokens[7], &value, sizeof(value));
            return tokens;
        };
        const std::pair<std::vector<uint8_t>, std::vector<uint8_t>> changes[] =
        {
            { count("middle_repeat", 14), count("middle_repeat", 16) },
            { pair(0.8f, 1.06f), pair(0.8f, 1.185f) },
            { pair(1.27f, 1.1f), pair(1.27f, 1.19f) },
        };
        auto* script = Read<uint8_t*>(symbol, 12);
        auto* end = script + 2048;
        std::array<uint8_t*, std::size(changes)> sites{};
        for (size_t i = 0; i < sites.size(); ++i)
        {
            sites[i] = std::search(script, end, changes[i].first.begin(), changes[i].first.end());
            if (sites[i] == end)
            {
                LogOnce("Options panel size not found");
                return;
            }
        }
        for (size_t i = 0; i < sites.size(); ++i)
            std::copy(changes[i].second.begin(), changes[i].second.end(), sites[i]);
    }
    inline int __cdecl Task(void* task)
    {
        if (!nativeThread) nativeThread = GetCurrentThreadId();
        if (nativeThread != GetCurrentThreadId()) return taskHook.call<int>(task);
        int result;
        { DepthScope depth(taskDepth); result = taskHook.call<int>(task); }
        if (!ready || taskDepth || nativeDepth || draining) return result;
        GrowOptionsPanel();
        DrainScope drain;
        for (size_t n = 0; !pending.empty() && n < 32; ++n)
        {
            auto action = pending.front();
            pending.pop_front();
            Act(action);
        }
        auto owner = context;
        if (owner && owner->returnPending && !Modal()) Cleanup(owner, true);
        else if (owner && !Current(owner) && (!Modal() || !Owned(owner))) Cleanup(owner, false);
        else if (owner && !Modal() && RWGTAMenu::Model::Changed() && !Build(owner)) Cleanup(owner, true);
        if (UpdateFrame) UpdateFrame();
        return result;
    }
    inline void Init()
    {
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            LogOnce("native bridge module pin rejected");
            return;
        }
        // The native controls retain event-vtable pointers. Pin before any
        // publication; dynamic host DLL unload is unsupported.
        textTable.original = Game().textTable;
        containerTable.original = Game().containerTable;
        menuTable.original = Game().menuTable;
        if (!textTable.Prepare() || !containerTable.Prepare() || !menuTable.Prepare()) { LogOnce("native vtable contracts rejected"); return; }
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                restartBaseline.emplace(std::make_pair(module, setting.id), FixSettings().Get(setting.id));
        }
        destructorHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().elementDestroyed), ElementDestroyed);
        sourceHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().source), Source);
        brokerHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().broker), Broker);
        taskHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().task), Task);
        if (!destructorHook || !sourceHook || !brokerHook || !taskHook)
        {
            taskHook.reset(); brokerHook.reset(); sourceHook.reset(); destructorHook.reset();
            LogOnce("native hook transaction rejected");
            return;
        }
        ready = true;
    }
}

namespace THPS4Updates
{
    inline bool Current()
    {
        const auto owner = THPS4Menu::context;
        return THPS4Menu::Current(owner) && !THPS4Menu::Modal() && !THPS4Menu::foreignDepth &&
            !owner->constructing && !owner->rebuilding && !owner->details;
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        if (!THPS4Menu::ready)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TonyHawksProSkater4.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = Current;
        config.format = [](const std::string&, bool hub)
        {
            const auto owner = THPS4Menu::context;
            return owner ? asi_settings::text::Widen(owner->model.Text(hub ? "update.availablehub" : "update.availablesite")) : std::wstring{};
        };
        config.show = [](const std::wstring& text)
        {
            if (!Current())
                return;
            const auto owner = THPS4Menu::context;
            owner->detailLines = THPS4Menu::HelpLines(asi_settings::text::Narrow(text));
            if (owner->detailLines.empty())
                return;
            owner->detailSelection = owner->selected;
            owner->details = true;
            owner->detailStart = 0;
            owner->selected = 0;
            if (!THPS4Menu::Build(owner))
                THPS4Menu::Cleanup(owner, true);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        THPS4Menu::UpdateFrame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
}
