module;

#include <stdafx.h>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <Windows.h>
#include <cmath>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <functional>
#include <map>
#include <memory>
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;
import Window;

export
{
namespace THUG2Menu
{
    // Installed THUG2: packed component types, CStruct head at +4, and pair
    // coordinates after a four-byte native header. These are borrowed inputs
    // only; owning native Clear/destructor functions must never receive them.
    struct Component
    {
        uint8_t encodedType = 0, flags = 0;
        uint16_t reserved = 0;
        uint32_t name = 0, value = 0;
        Component* next = nullptr;
    };
    struct Structure
    {
        uint8_t reserved = 0, flags = 0;
        uint16_t padding = 0;
        Component* head = nullptr;
    };
    struct Array
    {
        uint8_t reserved = 0, type = 0;
        uint16_t padding = 0;
        uint32_t count = 0, data = 0;
    };
    struct Pair { uint32_t header = 0; float x = 0, y = 0; };
    struct WeakSlot { void* element = nullptr; WeakSlot* previous = nullptr; WeakSlot* next = nullptr; };
    static_assert(sizeof(Component) == 16 && sizeof(Structure) == 8);
    static_assert(sizeof(Array) == 12 && sizeof(Pair) == 12 && sizeof(WeakSlot) == 12);

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
            pairs.push_back({ 0, x, y });
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
            components.push_back({ static_cast<uint8_t>(type << 1), 0, 0, name, data, nullptr });
            auto* item = &components.back();
            if (tail) tail->next = item;
            else value.head = item;
            tail = item;
        }
    };

    // SetText4AF980 collapses one escape layer; font4CEE20 consumes another.
    // Fitting uses two slashes per literal glyph; the setter needs four.
    inline std::string EscapeNativeText(const std::string& text, bool setter)
    {
        std::string result;
        result.reserve(text.size());
        for (char c : text)
        {
            if (c == '\\') result.append(setter ? 3 : 1, '\\');
            result += c;
        }
        return result;
    }
}

namespace THUG2Menu
{
    template<class T> T Read(const void* value, size_t offset = 0)
    {
        T result{};
        std::memcpy(&result, static_cast<const uint8_t*>(value) + offset, sizeof(result));
        return result;
    }

    struct Addresses
    {
        uintptr_t symbol, run, spawn, findWeak, setProps, unlinkWeak, destroy, fire,
            getFont, measure, elementDestroyed, scriptCall, task;
        uintptr_t menuTable, fontTable;
    };
    inline const Addresses& Game()
    {
        static const Addresses game = []
        {
            auto at = [](const char* bytes) { return reinterpret_cast<uintptr_t>(hook::pattern(bytes).get_first()); };
            auto operand = [](const char* bytes, ptrdiff_t offset) { return *hook::pattern(bytes).get_first<uintptr_t>(offset); };
            Addresses a{};
            a.symbol = at("8B 4C 24 04 8B C1 56 8B 35 ? ? ? ? 25");
            a.run = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 55 8B 6C 24 18 56 55");
            a.spawn = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 55 8B 6C 24 18 56 57");
            a.findWeak = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 10 8B 44 24 24 8B");
            a.setProps = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 0C 8B 0D ? ? ? ? 56 8B 74 24 20 6A 01 68 AF 98 C6 40 56 8D 44 24 10 50 E8 ? ? ? ? 8B 4C 24 04 85");
            a.unlinkWeak = at("8B 44 24 04 8B 50 04 56 33 F6 3B D6");
            a.destroy = at("51 53 8B 1D ? ? ? ? 57 8B 7C 24");
            a.fire = at("83 EC 20 53 55 56 8B 74 24 30 57 8B");
            a.getFont = at("8B 0D ? ? ? ? 8B 15 ? ? ? ? B8 01 00 00 00 D3 E0 8B 4C 24 04 48 23 C1 8D 04 40 8D 04 82 85 C0 74 12 39 08 75 07 8B 50 04 85 D2 75 0A 8B 40 08 85 C0 75 EE 33 C0 C3 8B 40 04 85");
            a.measure = at("8B 01 FF 60 20 CC CC CC CC CC CC CC");
            a.elementDestroyed = at("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 10 53 56 8B F1");
            a.scriptCall = at("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 55 8B E9");
            a.task = at("8B 0D ? ? ? ? 8B 81 1C 01 00 00 50 E8 ? ? ? ? 8B 4C");
            NativeLanguage.SetAddress(operand("A1 ? ? ? ? 83 C0 FD 83 F8 03 8D", 1));
            NativeSymbols.SetAddress(operand("89 3D ? ? ? ? B9 00 10 00 00 33", 2));
            a.menuTable = operand("C7 06 ? ? ? ? 89 8E 78 01 00 00", 2);
            a.fontTable = operand("C7 06 ? ? ? ? C7 46 08 00 00 00 00 8B", 2);
            return a;
        }();
        return game;
    }
}

namespace THUG2Menu
{
    inline void* Symbol(uint32_t name)
    {
        if (!NativeSymbols.get()) return nullptr;
        return reinterpret_cast<void* (__cdecl*)(uint32_t)>(Game().symbol)(name);
    }
    inline bool ScriptAvailable(const char* name)
    {
        auto* symbol = Symbol(Checksum(name));
        return symbol && Read<uint8_t>(symbol, 1) == 7 && Read<uintptr_t>(symbol, 12) != 0;
    }
    inline void Run(const char* name, const Parameters& parameters)
    {
        // The dispatcher returns its cleanup result. Callers check the controls it creates.
        reinterpret_cast<bool (__cdecl*)(uint32_t, const Structure*, void*, char, const char*)>(Game().run)(Checksum(name), &parameters.value, nullptr, 0, name);
    }
    // A native script function (symbol type 8), called as a script would call it
    inline bool Function(const char* name, const Parameters& parameters)
    {
        auto* symbol = Symbol(Checksum(name));
        if (!symbol) return false;
        const auto function = Read<uintptr_t>(symbol, 12);
        if (Read<uint8_t>(symbol, 1) != 8 || !function) return false;
        return reinterpret_cast<bool (__cdecl*)(const Structure*, void*)>(function)(&parameters.value, nullptr);
    }
    inline bool Spawn(const char* name, const Parameters& parameters, uint32_t id)
    {
        // Parent menus may wait for native animation/gameframes. Queue them in
        // the engine's VM; the synchronous dispatcher must never run these.
        return reinterpret_cast<void* (__cdecl*)(uint32_t, const Structure*, uint32_t, const Structure*, uint32_t, uint32_t, char, char, char, char)>(Game().spawn)(Checksum(name), &parameters.value, 0, nullptr, 0, id, 0, 0, 0, 0) != nullptr;
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
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(Game().findWeak)(&slot, id, 0);
        }
        void* Get() const { return slot.element; }
    };

    inline bool SetProperties(const Parameters& parameters)
    {
        return reinterpret_cast<bool (__cdecl*)(const Structure*)>(Game().setProps)(&parameters.value);
    }

    inline bool Destroy(uint32_t id)
    {
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        Parameters parameters;
        parameters.Name("id", id);
        return reinterpret_cast<bool (__cdecl*)(const Structure*, void*)>(Game().destroy)(&parameters.value, nullptr);
    }
    inline bool Fire(const char* event, uint32_t target, Structure* data = nullptr)
    {
        Weak object;
        object.Find(target);
        if (!object.Get()) return false;
        Parameters parameters;
        parameters.Name("type", event);
        parameters.Name("target", target);
        parameters.Name("source", target);
        if (data) parameters.Nested("data", data);
        return reinterpret_cast<bool (__cdecl*)(const Structure*, void*)>(Game().fire)(&parameters.value, nullptr);
    }

    struct Extent { float width = 0, height = 0; };
    inline bool Measure(const std::string& literal, Extent& extent, uint32_t fontId = Checksum("small"))
    {
        if (literal.size() > 4096) return false;
        auto* font = reinterpret_cast<void* (__cdecl*)(uint32_t)>(Game().getFont)(fontId);
        if (!font || Read<uintptr_t>(font) != Game().fontTable) return false;
        auto* platform = Read<void*>(font, 8);
        if (!platform) return false;
        auto glyphs = Read<uintptr_t>(platform, 8);
        if (!glyphs) return false;
        // Literal text is escaped below, so the native metric never interprets icon/control commands
        auto escaped = EscapeNativeText(literal, false);
        reinterpret_cast<void* (__thiscall*)(void*, const char*, float*, float*)>(Game().measure)(font, escaped.c_str(), &extent.width, &extent.height);
        return std::isfinite(extent.width) && std::isfinite(extent.height) && extent.width >= 0 && extent.height > 0;
    }
}

namespace THUG2Menu
{
    inline SafetyHookInline taskHook, callHook, destructorHook;
    inline char(__thiscall* launch)(void*, uint32_t, uint32_t, uint32_t, Structure*, int, float) = nullptr;
    inline HMODULE module = nullptr;
    inline bool ready = false, registered = false, draining = false;
    inline unsigned taskDepth = 0;
    inline DWORD nativeThread = 0;
    inline uint32_t nextSerial = 0;
    constexpr uint32_t privateAnchor = Checksum("wfp_thug2_anchor");
    constexpr uint32_t privateMenu = Checksum("wfp_thug2_vmenu");
    constexpr uint32_t entryId = Checksum("wfp_thug2_entry");
    inline std::string notice;                  // the update notice, shown on the Plugins list
    inline std::function<void()> updateFrame;

    inline uint32_t Serial()
    {
        nextSerial = (nextSerial + 1) & 0x7FFFFFFF;
        if (!nextSerial) ++nextSerial;
        return nextSerial;
    }
    inline std::string Language()
    {
        const auto chosen = FixSettings().Get("MAIN.TextLanguage");
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
            default: return "en";
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
        const auto page = NativeLanguage.get() == 9 ? 1251u : 1252u;
        const DWORD flags = page == 1251u ? 0 : WC_NO_BEST_FIT_CHARS;
        if (page == 1251u)
        {
            // The Cyrillic font can display the plain Latin forms of localized text.
            for (size_t i = wide.find(L'\u00DF'); i != std::wstring::npos; i = wide.find(L'\u00DF', i + 2))
                wide.replace(i, 1, L"ss");
        }
        auto bytes = WideCharToMultiByte(page, flags, wide.c_str(), -1, nullptr, 0, "?", nullptr);
        if (bytes <= 0 || bytes > 4096) return "?";
        std::string result(size_t(bytes), '\0');
        if (!WideCharToMultiByte(page, flags, wide.c_str(), -1, result.data(), bytes, "?", nullptr)) return "?";
        result.pop_back();
        return result;
    }
    inline std::string Fit(const std::string& label, const std::string& value, float width)
    {
        auto left = NativeText(label);
        auto suffix = value.empty() ? std::string() : ": " + NativeText(value);
        Extent measured;
        auto full = left + suffix;
        if (!Measure(full, measured)) return {};
        if (measured.width <= width) return full;
        // Keep the entire value. The native text node may subsequently scale
        // horizontally if the value alone is wider than the available column.
        while (!left.empty())
        {
            left.pop_back();
            full = left + "..." + suffix;
            if (!Measure(full, measured)) return {};
            if (measured.width <= width) return full;
        }
        return suffix.empty() ? "..." : suffix.substr(2);
    }

    struct Host
    {
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false;
    };
    enum class Kind { Link, Previous, Next, Row, Info, Back };
    struct Item
    {
        Kind kind;
        size_t index = 0;
        bool enabled = true;
        std::string label, value, description;
        uint32_t id = 0;
        const RWGTAMenu::Row* displayedRow = nullptr;
    };
    struct Context
    {
        RWGTAMenu::Model model;
        Weak anchor, menu, window;
        uint32_t serial = 0;
        bool pause = false, retired = false, rebuilding = false, constructing = false, returnPending = false;
        size_t page = SIZE_MAX, rootStart = 0, selected = 0;
        uint32_t focused = 0;
        std::string language;
        std::vector<Item> items;
        std::vector<std::string> helpLines;     // the selected row's help, one line at a time
        size_t helpLine = 0;
        ULONGLONG helpTime = 0;                 // when the next help line is due
    };
    inline std::unique_ptr<Host> host;
    inline std::shared_ptr<Context> context;
    inline Weak returnWindow;
    inline bool returnToEntry = false, returnPause = false;
    enum class Action { Open, Accept, Left, Right, Back, Focus };
    struct Pending { Action action; uint32_t serial; size_t index; };
    inline std::deque<Pending> pending;

    inline bool Modal()
    {
        // 7CF46F includes ordinary menus, so it is not a modal-only blocker.
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
        return window.Get() == owner->window.Get();
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
    // Controller events go to root_window and down the focus chain to the selected row. While our page or the
    // Plugins row is focused, choose/left/right/back are queued here and run after the frame (Task).
    inline bool Intercept(uint32_t type)
    {
        Action action;
        if (type == Checksum("pad_choose") || type == Checksum("pad_start")) action = Action::Accept;
        else if (type == Checksum("pad_left")) action = Action::Left;
        else if (type == Checksum("pad_right")) action = Action::Right;
        else if (type == Checksum("pad_back")) action = Action::Back;
        else return false;
        if (!ready || draining || nativeThread != GetCurrentThreadId() || pending.size() >= 32 || Modal()) return false;
        auto owner = context;
        if (!owner)
        {
            if (action != Action::Accept || !host || !host->menu.Get()) return false;
            Weak current, anchor;
            current.Find(Checksum("current_menu"));
            anchor.Find(Checksum("current_menu_anchor"));
            if (current.Get() != host->menu.Get() || anchor.Get() != host->anchor.Get()
                || Read<uint8_t>(host->menu.Get(), 384) == 0 || Read<uint32_t>(host->menu.Get(), 380) != entryId) return false;
            pending.push_back({ Action::Open, host->serial, 0 });
            return true;
        }
        if (!Current(owner)) return false;
        auto selected = Read<uint32_t>(owner->menu.Get(), 380);
        size_t index = 0;
        while (index < owner->items.size() && owner->items[index].id != selected) ++index;
        if (action == Action::Back || index < owner->items.size()) pending.push_back({ action, owner->serial, index });
        return true;
    }
    inline char __fastcall Launch(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* parameters, int flag, float time)
    {
        if (Intercept(type)) return 1;
        return launch(manager, type, target, source, parameters, flag, time);
    }
    inline void Color(uint32_t id, bool selected, bool enabled)
    {
        Weak target;
        target.Find(id);
        if (!target.Get()) return;
        // Native skutils do_scale_up/down palette, without their spawned blur
        // loop. These remain the game's own text-control colors and renderer.
        ArrayView rgba(1, enabled ? selected ? std::vector<uint32_t>{127,102,0,100} : std::vector<uint32_t>{88,105,112,128} : std::vector<uint32_t>{70,70,70,120});
        Parameters parameters;
        parameters.Name("id", id);
        parameters.List("rgba", &rgba.value);
        SetProperties(parameters);
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
        auto owner = context;
        if (owner && (object == owner->window.Get() || (!owner->rebuilding && (object == owner->anchor.Get() || object == owner->menu.Get())))) Retire(owner);
        destructorHook.thiscall<void>(object);
    }
    // Lines that fit the helper text bar, broken at spaces
    inline std::vector<std::string> Lines(const std::string& text, float width)
    {
        std::vector<std::string> lines;
        std::string line;
        Extent measured;
        for (size_t offset = 0; offset < text.size();)
        {
            auto end = text.find(' ', offset);
            if (end == std::string::npos) end = text.size();
            auto word = text.substr(offset, end - offset);
            offset = end + 1;
            if (word.empty()) continue;
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && (!Measure(NativeText(candidate), measured) || measured.width > width))
            {
                lines.push_back(std::move(line));
                line = std::move(word);
            }
            else line = std::move(candidate);
        }
        if (!line.empty()) lines.push_back(std::move(line));
        return lines;
    }
    inline bool Line(const std::shared_ptr<Context>& owner, const std::string& text)
    {
        if (!Current(owner)) return false;
        const auto native = Fit(text, "", 500);
        Parameters line;
        line.Text("text", EscapeNativeText(native, true));
        ArrayView lines(10, { Parameters::Pointer(&line.value) });
        Parameters parameters;
        parameters.List("helper_text_elements", &lines.value);
        Run("create_helper_text", parameters);
        return Current(owner);
    }
    // The help runs line by line in the helper text bar, a line every three seconds, repeated
    inline bool Help(const std::shared_ptr<Context>& owner, const std::string& text)
    {
        owner->helpLines = Lines(text, 500);
        owner->helpLine = 0;
        owner->helpTime = GetTickCount64() + 3000;
        return Line(owner, owner->helpLines.empty() ? std::string() : owner->helpLines.front());
    }
    inline void NextHelpLine(const std::shared_ptr<Context>& owner)
    {
        if (owner->helpLines.size() < 2 || GetTickCount64() < owner->helpTime) return;
        owner->helpLine = (owner->helpLine + 1) % owner->helpLines.size();
        owner->helpTime = GetTickCount64() + 3000;
        Line(owner, owner->helpLines[owner->helpLine]);
    }
    // Startup values of the restart settings: the restart line shows while one of them differs
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
                // The getter can close the native menu and release the model's row storage.
                const auto plugin = *row.plugin;
                const auto setting = *row.setting;
                auto value = OtherPlugins::Get(plugin, setting);
                if (!Current(owner))
                    return false;
                auto baseline = restartBaseline.emplace(std::make_pair(plugin.module, setting.id), value).first;
                pending |= !asi_settings::text::IEquals(baseline->second, value);
            }
        }
        return pending;
    }
    inline std::string ItemHelp(const std::shared_ptr<Context>& owner, const Item& item)
    {
        return owner->page == SIZE_MAX && !notice.empty() ? notice : item.description;
    }
    inline bool Build(const std::shared_ptr<Context>& owner)
    {
        if (!Current(owner))
            return false;
        RWGTAMenu::Model::Changed().exchange(false);
        std::vector<Item> items;
        std::string heading = owner->model.Text("menu.plugins");
        if (owner->page == SIZE_MAX)
        {
            auto count = std::min(size_t(5), owner->model.Pages().size() - std::min(owner->rootStart, owner->model.Pages().size()));
            for (size_t i = 0; i < count; ++i)
            {
                const auto& full = owner->model.Pages()[owner->rootStart + i].title;
                auto label = full;
                std::string number;
                const auto suffix = label.rfind(" (");
                if (suffix != std::string::npos && label.back() == ')')
                {
                    number = label.substr(suffix + 2, label.size() - suffix - 3);
                    label.resize(suffix);
                }
                items.push_back({ Kind::Link, owner->rootStart + i, true, std::move(label), std::move(number), full });
            }
            if (owner->rootStart) items.push_back({ Kind::Previous, 0, true, owner->model.Text("menu.previous") });
            if (owner->rootStart + count < owner->model.Pages().size()) items.push_back({ Kind::Next, count, true, owner->model.Text("menu.next") });
        }
        else
        {
            if (owner->page >= owner->model.Pages().size()) return false;
            heading = owner->model.Pages()[owner->page].title;
            auto rows = owner->model.VisibleRows(owner->page);
            if (!Current(owner))
                return false;
            for (size_t i = 0; i < rows.size(); ++i)
            {
                auto row = *rows[i];
                auto enabled = owner->model.Enabled(row, owner->pause);
                if (!Current(owner))
                    return false;
                auto label = owner->model.Label(row);
                auto value = enabled ? owner->model.Value(row) : owner->model.Text("menu.unavailable");
                if (!Current(owner))
                    return false;
                auto description = owner->model.Help(row);
                items.push_back({ Kind::Row, i, enabled, std::move(label), std::move(value), std::move(description) });
                items.back().displayedRow = rows[i];
            }
            if (RestartPending(owner))
                items.push_back({ Kind::Info, 0, false, owner->model.Text("menu.restartwarning") });
        }
        items.push_back({ Kind::Back, 0, true, owner->model.Text("menu.back") });
        if (!Current(owner) || items.size() > 8) return false;
        owner->serial = Serial(); // Row-pool generation rejects queued old-page input.
        owner->rebuilding = true;
        owner->constructing = true;
        auto oldAnchor = Read<uint32_t>(owner->anchor.Get(), 20);
        Destroy(oldAnchor);
        Weak replacement;
        replacement.Find(Checksum("current_menu_anchor"));
        if (context != owner || owner->retired || !owner->window.Get() || Modal() || replacement.Get())
        {
            owner->rebuilding = false;
            return false;
        }
        Parameters root;
        root.Name("menu_id", privateAnchor);
        root.Name("vmenu_id", privateMenu);
        const auto title = NativeText(heading);
        const auto escapedTitle = EscapeNativeText(title, true);
        root.Flag("no_helper_text");
        if (owner->pause)
        {
            root.XY("pos", 120, 108);
            root.XY("dims", 440, 252);
        }
        else
        {
            root.Text("title", escapedTitle);
            root.Number("title_scale", std::min(2.0f, 44.0f / std::max(1.0f, static_cast<float>(escapedTitle.size()))));
            root.XY("dims", 300, 252);
        }
        Run(owner->pause ? "make_new_roundbar_menu" : "make_new_themed_sub_menu", root);
        // The themed title script owns a parts container outside the menu itself.
        // Its current anchor alias is the lifetime/cleanup root for both styles.
        owner->anchor.Find(Checksum("current_menu_anchor"));
        owner->menu.Find(privateMenu);
        owner->rebuilding = false;
        if (!Current(owner) || Read<uint32_t>(owner->menu.Get(), 24) != Checksum("VMenu") || Read<uintptr_t>(owner->menu.Get()) != Game().menuTable)
            return false;
        if (owner->pause)
        {
            // Keep the right-aligned title inside the frame as settings rebuild the page.
            Extent extent;
            if (!Measure(title, extent, Checksum("testtitle")))
                return false;
            Parameters subtitle;
            subtitle.Text("title", escapedTitle);
            subtitle.Number("title_scale", std::min(1.9f, 560.0f / std::max(1.0f, extent.width)));
            Run("build_theme_sub_title", subtitle);
            if (!Current(owner))
                return false;
            Parameters position;
            position.Name("id", "title_text");
            position.XY("pos", 600.0f, 40.0f);
            position.Number("time", 0.0f);
            if (!Function("DoScreenElementMorph", position) || !Current(owner))
                return false;
        }
        owner->items = std::move(items);
        owner->selected = std::min(owner->selected, owner->items.size() - 1);
        for (size_t i = 0; i < owner->items.size(); ++i)
        {
            auto& item = owner->items[i];
            auto idText = "wfp_thug2_row" + std::to_string(i);
            item.id = Checksum(idText.c_str());
            // Measured in the font's own size; the rows draw it larger, so the text column is kept inside the bar
            const auto width = owner->pause ? 360.0f : 280.0f;
            auto fitted = NativeText(item.label);
            if (!item.value.empty())
                fitted += ": " + NativeText(item.value);
            Extent extent;
            if (!Measure(fitted, extent))
                return false;
            const auto rowScale = extent.width > width ? width / extent.width : 1.0f;
            // The stock row keeps its own focus visuals; input is taken in Intercept
            Parameters row;
            row.Name("id", item.id);
            row.Name("parent", privateMenu);
            row.Name("text_id", Checksum((idText + "_text").c_str()));
            row.Text("text", EscapeNativeText(fitted, true));
            row.Flag("no_choose_sound");
            if (!owner->pause)
            {
                row.Flag("centered");
                row.Flag("no_bg");
            }
            if (!item.enabled) row.Flag("not_focusable");
            Run(owner->pause ? "add_roundbar_menu_item" : "theme_menu_add_item", row);
            if (!Current(owner))
                return false;
            Weak created;
            created.Find(item.id);
            if (!created.Get() || Read<uint32_t>(created.Get(), 24) != Checksum("ContainerElement"))
                return false;
            Parameters scale;
            scale.Name("id", item.id);
            scale.XY("scale", rowScale, rowScale);
            if (!SetProperties(scale))
                return false;
        }
        if (owner->items[owner->selected].kind == Kind::Info) owner->selected = owner->items.size() - 1;
        if (!Help(owner, ItemHelp(owner, owner->items[owner->selected])))
            return false;
        Parameters finish;
        finish.Name("menu", privateAnchor);
        if (!Spawn(owner->pause ? "roundbar_menu_finish" : "finish_themed_sub_menu", finish, Checksum("wfp_thug2_finish")))
            return false;
        if (!Current(owner))
            return false;
        owner->focused = owner->items[owner->selected].id;
        Parameters focus;
        focus.Name("child_id", owner->items[owner->selected].id);
        if (!Fire("focus", privateMenu, &focus.value) || !Current(owner))
            return false;
        owner->constructing = false;
        return true;
    }

    inline bool Active(const std::shared_ptr<Context>& owner, size_t index)
    {
        return Current(owner) && index < owner->items.size()
            && Read<uint8_t>(owner->menu.Get(), 384) != 0
            && Read<uint32_t>(owner->menu.Get(), 380) == owner->items[index].id;
    }
    inline void Entry(void* script, uint32_t callee)
    {
        if (!ready || !script || nativeThread != GetCurrentThreadId() || context || !registered) return;
        auto caller = Read<uint32_t>(script, 200);
        auto pause = caller == Checksum("create_options_menu") && callee == Checksum("roundbar_menu_finish");
        // Title: before the last row (Blood), which closes the panel's frame
        auto title = caller == Checksum("create_gamesettings_menu") && callee == Checksum("theme_menu_add_flag_item");
        if (!pause && !title) return;
        Weak existing;
        existing.Find(entryId);
        if (existing.Get()) return;
        auto source = std::make_unique<Host>();
        source->menu.Find(Checksum("current_menu"));
        source->anchor.Find(Checksum("current_menu_anchor"));
        source->window.Find(Checksum("root_window"));
        if (!source->menu.Get() || !source->anchor.Get() || !source->window.Get()
            || Read<uint32_t>(source->menu.Get(), 24) != Checksum("VMenu")
            || Read<uintptr_t>(source->menu.Get()) != Game().menuTable || Modal())
            return;
        source->serial = Serial();
        source->pause = pause;
        Parameters row;
        row.Name("id", entryId);
        row.Text("text", EscapeNativeText(NativeText(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins")), true));
        // These are the installed roots' own row factories, before their native
        // finish/layout/focus call. Stock options and action handlers stay intact.
        Run(pause ? "add_roundbar_menu_item" : "theme_menu_add_item", row);
        existing.Find(entryId);
        if (!existing.Get() || !source->menu.Get() || !source->anchor.Get())
            return;
        host = std::move(source);
    }
    inline char __fastcall ScriptCall(void* script, void*, uint32_t name, void* code, Structure* parameters, void* object, char flag)
    {
        Entry(script, name);
        return callHook.thiscall<char>(script, name, code, parameters, object, flag);
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
            if (!Spawn(owner->pause ? "create_options_menu" : "create_gamesettings_menu", parameters, Checksum("wfp_thug2_return")))
            {
                owner->returnPending = true;
                return; // Retain context/Back and retry after the next outer task.
            }
            returnWindow.Find(Checksum("root_window"));
            returnPause = owner->pause;
            returnToEntry = returnWindow.Get() == owner->window.Get();
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
            || Read<uint8_t>(host->menu.Get(), 384) == 0 || Read<uint32_t>(host->menu.Get(), 380) != entryId
            || !ScriptAvailable("create_helper_text")
            || (host->pause && !ScriptAvailable("build_theme_sub_title"))) return;
        auto sourceMenu = host->menu.Get();
        auto sourceAnchor = host->anchor.Get();
        auto sourceWindow = host->window.Get();
        auto owner = std::make_shared<Context>();
        owner->serial = Serial();
        owner->pause = host->pause;
        owner->language = Language();
        owner->model.OpenPlugins(module, owner->language, 5);
        current.Find(Checksum("current_menu"));
        anchor.Find(Checksum("current_menu_anchor"));
        window.Find(Checksum("root_window"));
        if (!host || host->serial != serial || host->menu.Get() != sourceMenu || host->anchor.Get() != sourceAnchor || host->window.Get() != sourceWindow || current.Get() != sourceMenu || anchor.Get() != sourceAnchor || window.Get() != sourceWindow || Modal()
            || Read<uint8_t>(sourceMenu, 384) == 0 || Read<uint32_t>(sourceMenu, 380) != entryId)
        {
            owner->model.Close();
            return;
        }
        owner->anchor.Find(Read<uint32_t>(sourceAnchor, 20));
        owner->menu.Find(Read<uint32_t>(sourceMenu, 20));
        owner->window.Find(Checksum("root_window"));
        context = owner;
        if (!Build(owner)) Cleanup(owner, true);
    }
    inline void Act(const Pending& action)
    {
        if (action.action == Action::Open) { Open(action.serial); return; }
        auto owner = context;
        if (!owner || owner->serial != action.serial) return;
        if (action.action == Action::Back)
        {
            if (owner->retired) { Cleanup(owner, true); return; }
            if (!Current(owner) || Read<uint8_t>(owner->menu.Get(), 384) == 0) return;
            if (owner->page != SIZE_MAX)
            {
                owner->page = SIZE_MAX;
                owner->selected = 0;
                if (!Build(owner)) Cleanup(owner, true);
            }
            else Cleanup(owner, true);
            return;
        }
        if (!Active(owner, action.index)) return;
        auto item = owner->items[action.index];
        owner->selected = action.index;
        if (action.action == Action::Focus)
        {
            owner->focused = item.id;
            Help(owner, ItemHelp(owner, item));
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
            owner->selected = 0;
            owner->model.PageShown(item.index);
        }
        else if (item.kind == Kind::Previous)
        {
            owner->rootStart = owner->rootStart >= 5 ? owner->rootStart - 5 : 0;
            owner->selected = 0;
        }
        else if (item.kind == Kind::Next)
        {
            owner->rootStart += item.index;
            owner->selected = 0;
        }
        else
        {
            auto rows = owner->model.VisibleRows(owner->page);
            if (!Current(owner)) return;
            if (item.index >= rows.size() || rows[item.index] != item.displayedRow)
            {
                if (!Build(owner)) Cleanup(owner, true);
                return;
            }
            auto row = *rows[item.index];
            if (!owner->model.Enabled(row, owner->pause) || !Current(owner)) return;
            if (!accept && row.kind != RWGTAMenu::RowKind::Setting && row.kind != RWGTAMenu::RowKind::Preset) return;
            owner->model.Change(row, accept ? 0 : action.action == Action::Left ? -1 : 1, owner->pause);
            if (!Current(owner)) return;
        }
        if (!Build(owner)) Cleanup(owner, true);
    }
    inline int __cdecl Task(void* task)
    {
        if (!nativeThread) nativeThread = GetCurrentThreadId();
        if (nativeThread != GetCurrentThreadId()) return taskHook.call<int>(task);
        ++taskDepth;
        // The two menu styles share the same loaded script library. The stock
        // parent scripts are observed running in Entry before a page can open.
        if (ready && !registered && ScriptAvailable("make_new_themed_sub_menu") && ScriptAvailable("make_new_roundbar_menu")
            && ScriptAvailable("finish_themed_sub_menu") && ScriptAvailable("roundbar_menu_finish")
            && ScriptAvailable("theme_menu_add_item") && ScriptAvailable("add_roundbar_menu_item")) registered = true;
        auto result = taskHook.call<int>(task);
        --taskDepth;
        if (!ready || taskDepth || draining) return result;
        if (registered && !THUG2Display::StartupMoviesFinished.load(std::memory_order_relaxed))
        {
            Weak menu;
            menu.Find(Checksum("current_menu_anchor"));
            if (menu.Get())
                THUG2Display::StartupMoviesFinished.store(true, std::memory_order_relaxed);
        }
        draining = true;
        for (size_t n = 0; !pending.empty() && n < 32; ++n)
        {
            auto action = pending.front();
            pending.pop_front();
            Act(action);
        }
        if (returnToEntry && host && !context && !Modal())
        {
            Weak menu, anchor, window, entry;
            menu.Find(Checksum("current_menu"));
            anchor.Find(Checksum("current_menu_anchor"));
            window.Find(Checksum("root_window"));
            entry.Find(entryId);
            if (!returnWindow.Get() || window.Get() != returnWindow.Get())
            {
                returnToEntry = false;
                returnWindow.Reset();
            }
            else if (menu.Get() && host->pause == returnPause && host->menu.Get() == menu.Get() && host->anchor.Get() == anchor.Get()
                && host->window.Get() == window.Get() && entry.Get() && Read<uint8_t>(menu.Get(), 384))
            {
                returnToEntry = false;
                returnWindow.Reset();
                const auto id = Read<uint32_t>(menu.Get(), 20);
                if (Read<uint32_t>(menu.Get(), 380) != entryId)
                {
                    Fire("unfocus", id);
                    Parameters focus; focus.Name("child_id", entryId);
                    Fire("focus", id, &focus.value);
                }
            }
        }
        auto owner = context;
        if (owner && Current(owner) && !Modal())
        {
            // The selection moved (native up/down): show the row's description
            auto selected = Read<uint32_t>(owner->menu.Get(), 380);
            for (size_t i = 0; selected != owner->focused && i < owner->items.size(); ++i)
            {
                if (owner->items[i].id == selected) Act({ Action::Focus, owner->serial, i });
            }
        }
        if (owner && Current(owner) && !Modal())
            NextHelpLine(owner);
        if (updateFrame) updateFrame();
        if (owner && owner->returnPending && !Modal()) Cleanup(owner, true);
        else if (owner && !Current(owner) && (!Modal() || !Owned(owner))) Cleanup(owner, false);
        else if (owner && !Modal() && RWGTAMenu::Model::Changed() && !Build(owner)) Cleanup(owner, true);
        draining = false;
        return result;
    }
    inline std::string NoticeText(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language(), key);
    }
    // The Plugins list (not a plugin's page) is on screen
    inline bool ListShown()
    {
        auto owner = context;
        return owner && owner->page == SIZE_MAX && !owner->constructing && Current(owner);
    }
    inline void ShowNotice(const std::string& text)
    {
        notice = text;
        auto owner = context;
        if (ListShown()) Help(owner, notice);
    }
    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&Init), &module);
        // The game keeps calling into this module (hooks, the controller event call); unloading it is unsupported.
        auto pattern = hook::pattern("68 74 EE B2 36 68 74 EE B2 36 51 8B 4C 24 ? E8");
        destructorHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().elementDestroyed), ElementDestroyed);
        callHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().scriptCall), ScriptCall);
        taskHook = safetyhook::create_inline(reinterpret_cast<void*>(Game().task), Task);
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                restartBaseline.emplace(std::make_pair(module, setting.id), FixSettings().Get(setting.id));
        }
        launch = reinterpret_cast<decltype(launch)>(injector::GetBranchDestination(pattern.get_first(15)).as_int());
        injector::MakeCALL(pattern.get_first(15), Launch, true);
        ready = true;
    }
}

// MAIN.CheckForUpdates: the notice runs in the help line of the Plugins list
namespace THUG2Updates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        if (!THUG2Menu::ready)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TonyHawksUnderground2.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return THUG2Menu::ListShown(); };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(THUG2Menu::NoticeText(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text) { THUG2Menu::ShowNotice(asi_settings::text::Narrow(text)); };
        RWGTA::UpdateCheck::Init(std::move(config));
        THUG2Menu::updateFrame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
}
