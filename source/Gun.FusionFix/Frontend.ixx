module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RainDroplets.hpp"
#include "Shared/PadInput.hpp"

export module Frontend;

import ComVars;

export namespace GunMenu
{
    // Borrowed script parameters; native parsers copy retained values.
    struct Component
    {
        uint8_t flags = 0, encodedType = 0;
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
        uint8_t reserved = 0, flags = 0, type = 0, padding = 0;
        uint32_t count = 0, data = 0;
    };
    struct Pair
    {
        uint32_t header = 0;
        float x = 0, y = 0;
    };
    struct WeakSlot
    {
        void* element = nullptr;
        WeakSlot* previous = nullptr;
        WeakSlot* next = nullptr;
    };
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

        void Integer(const char* name, uint32_t data)
        {
            Add(1, Checksum(name), data);
        }
        void Number(const char* name, float data)
        {
            static_assert(sizeof(data) == sizeof(uint32_t));
            uint32_t bits;
            std::memcpy(&bits, &data, sizeof(bits));
            Add(2, Checksum(name), bits);
        }
        void Name(const char* name, uint32_t data)
        {
            Add(13, Checksum(name), data);
        }
        void Name(const char* name, const char* data)
        {
            Name(name, Checksum(data));
        }
        void Flag(const char* name)
        {
            Add(13, 0, Checksum(name));
        }
        void Flag(uint32_t name)
        {
            Add(13, 0, name);
        }
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
        void Nested(const char* name, Structure* structure)
        {
            Add(10, Checksum(name), Pointer(structure));
        }
        void List(const char* name, Array* array)
        {
            Add(12, Checksum(name), Pointer(array));
        }
        static uint32_t Pointer(const void* data)
        {
            return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(data));
        }

      private:
        std::deque<Component> components;
        std::deque<std::string> strings;
        std::deque<Pair> pairs;
        Component* tail = nullptr;
        void Add(uint8_t type, uint32_t name, uint32_t data)
        {
            components.push_back({ 0, static_cast<uint8_t>(type << 1), 0, name, data, nullptr });
            auto* item = &components.back();
            if (tail) tail->next = item;
            else value.head = item;
            tail = item;
        }
    };

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
} // namespace GunMenu

export namespace GunMenu
{
    // Native frontend entry points, found by signature in the retail and GOG executables.
    struct Addresses
    {
        uintptr_t symbol{}, run{}, lookup{}, unlink{}, props{}, destroy{}, factory{}, layout{}, launch{}, update{};
        uintptr_t manager{}, symbols{}, language{};
    };

    inline Addresses& Native()
    {
        static Addresses addresses;
        return addresses;
    }

    inline void Resolve()
    {
        static const bool resolved = []()
        {
            auto& a = Native();
            auto find = [](const char* signature)
            {
                return reinterpret_cast<uintptr_t>(hook::pattern(signature).get_first());
            };
            a.symbol = find("8B 4C 24 04 8B C1 56 8B 35 ? ? ? ? 25 FF 0F 00 00 8B 04 86 85 C0 74");
            a.run = find("53 8B 5C 24 08 55 56 57 53 E8 ? ? ? ? 8B 7C 24 20 8B 6C 24 1C 8B F0");
            a.lookup = find("83 EC 0C 8B 44 24 14 56 8B 35 ? ? ? ? 57 6A 01 8D 4C 24 20 51 8B 4E");
            a.unlink = find("8B 44 24 04 8B 50 04 56 33 F6 3B D6 74 08 8B 48 08 89 4A 08 EB 06 8B 50");
            a.props = find("83 EC 10 56 8B 35 ? ? ? ? 85 F6 74 4F 57 8B 7C 24 1C 8D 44 24 08 50");
            a.destroy = find("51 53 8B 1D ? ? ? ? 57 8B 7C 24 10 8D 44 24 08 50 68 AF 98 C6 40 57");
            a.factory = find("83 EC 18 53 55 8B 6C 24 30 56 57 33 FF 57 8D 44 24 1C 50 89 4C 24 1C 68");
            a.layout = find("83 EC 20 53 56 8B F1 D9 86 A0 01 00 00 57 D8 1D ? ? ? ? C7 44 24 10");
            a.launch = find("83 EC 2C 8B 44 24 3C 53 8B 5C 24 38 55 56 8B 74 24 44 57 8B F9 8B 57 20");
            a.update = find("53 56 8B 35 ? ? ? ? 33 DB 3B F3 C6 05 ? ? ? ? 01 88 1D");
            a.language = find("8B 0D ? ? ? ? 33 C0 83 F9 02 0F 94 C0 C3");
            a.manager = *reinterpret_cast<uintptr_t*>(a.props + 6);
            if (a.manager) MenuManager.SetAddress(a.manager);
            a.symbols = *reinterpret_cast<uintptr_t*>(a.symbol + 9);
            if (a.symbols) MenuSymbols.SetAddress(a.symbols);
            a.language = *reinterpret_cast<uintptr_t*>(a.language + 2);
            if (a.language) MenuLanguage.SetAddress(a.language);
            return true;
        }();
        (void)resolved;
    }
}

export namespace GunMenu
{
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        if (base)
            std::memcpy(&value, static_cast<const uint8_t*>(base) + offset, sizeof(value));
        return value;
    }

    inline void* symbol(uint32_t id)
    {
        return reinterpret_cast<void*(__cdecl*)(uint32_t)>(Native().symbol)(id);
    }
    inline char run(uint32_t id, const Structure* parameters, void* script, char flag)
    {
        return reinterpret_cast<char(__cdecl*)(uint32_t, const Structure*, void*, char)>(Native().run)(id, parameters, script, flag);
    }
    inline WeakSlot* lookup(WeakSlot* slot, uint32_t id, int flags)
    {
        return reinterpret_cast<WeakSlot*(__stdcall*)(WeakSlot*, uint32_t, int)>(Native().lookup)(slot, id, flags);
    }
    inline void unlink(void* element, WeakSlot* slot)
    {
        reinterpret_cast<void(__thiscall*)(void*, WeakSlot*)>(Native().unlink)(element, slot);
    }
    inline bool props(const Structure* parameters, void* script)
    {
        return reinterpret_cast<bool(__cdecl*)(const Structure*, void*)>(Native().props)(parameters, script);
    }
    inline bool destroy(const Structure* parameters, void* script)
    {
        return reinterpret_cast<bool(__cdecl*)(const Structure*, void*)>(Native().destroy)(parameters, script);
    }
    inline WeakSlot* factory(void* manager, WeakSlot* slot, uint32_t type, uint32_t id, const Structure* parameters)
    {
        return reinterpret_cast<WeakSlot*(__thiscall*)(void*, WeakSlot*, uint32_t, uint32_t, const Structure*)>(Native().factory)(manager, slot, type, id, parameters);
    }

    inline bool Create(uint32_t id, uint32_t type, const Parameters& parameters)
    {
        auto* manager = Read<void*>((MenuManager.is_initialized() ? MenuManager.get_ptr() : nullptr));
        if (!manager) return false;
        WeakSlot slot;
        factory(manager, &slot, type, id, &parameters.value);
        const bool result = slot.element != nullptr;
        if (slot.element) unlink(slot.element, &slot);
        return result;
    }

    inline void* Lookup(uint32_t id)
    {
        if (!Read<void*>((MenuManager.is_initialized() ? MenuManager.get_ptr() : nullptr))) return nullptr;
        WeakSlot slot;
        lookup(&slot, id, 0);
        auto* result = slot.element;
        if (result) unlink(result, &slot);
        return result;
    }
    inline void* Lookup(const char* id) { return Lookup(Checksum(id)); }
    inline bool Script(const char* name)
    {
        return Read<void*>((MenuSymbols.is_initialized() ? MenuSymbols.get_ptr() : nullptr)) && Read<uint8_t>(symbol(Checksum(name)), 2) == 7;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!Script(name)) return false;
        run(Checksum(name), &parameters.value, nullptr, 0);
        return true;
    }
    inline bool Properties(uint32_t id, Parameters& parameters)
    {
        if (!Lookup(id)) return false;
        parameters.Name("id", id);
        props(&parameters.value, nullptr);
        return true;
    }
    inline void Destroy(uint32_t id)
    {
        if (!Lookup(id)) return;
        Parameters parameters;
        parameters.Name("id", id);
        destroy(&parameters.value, nullptr);
    }
    inline std::string AutoLanguage()
    {
        // The native English script function compares this same frontend language value with 2.
        switch (Read<int>((MenuLanguage.is_initialized() ? MenuLanguage.get_ptr() : nullptr)))
        {
        case 3: return "fr";
        case 4: return "es";
        case 5: return "de";
        case 6: return "it";
        default: return "en";
        }
    }
} // namespace GunMenu

export namespace GunMenu
{
    constexpr auto Entry = Checksum("wfp_gun_plugins");
    // The rows the scrolling list shows at once, and the native right menus' row step
    constexpr size_t Slots = 6;
    constexpr float RowStep = 40.0f;
    constexpr float ListTop = 48.0f;
    inline const std::set<std::string> joined{ "MAIN.FOVFactor", "MAIN.WindowMode" };
    enum class Command { Choose, Left, Right, Back };
    struct Pending
    {
        Command command{};
        uint32_t generation{}, selected{};
        void* root{};
    };
    struct Item
    {
        uint32_t id{};
        const RWGTAMenu::Row* row{};
    };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        SafetyHookInline updateHook, launchHook;
        WeakSlot anchor{};
        std::deque<Pending> pending;
        std::vector<Item> items;
        uint32_t generation{ 1 }, focused{};
        unsigned depth{};
        size_t page{ SIZE_MAX };
        bool busy{}, display{}, pause{}, enabled{}, modelOpen{};
        std::string language, notice;
        MenuList::WheelNotches wheel;
        MenuList::Repeat shoulders[2];
    };
    // The wheel and Page Up/Page Down/Home/End from the window's messages, taken by the next menu update
    inline std::atomic<int> WheelInput{}, PageInput{}, EdgeInput{};
    inline void Message(UINT message, WPARAM wParam)
    {
        if (message == WM_MOUSEWHEEL)
            WheelInput += GET_WHEEL_DELTA_WPARAM(wParam);
        else if (message == WM_KEYDOWN && (wParam == VK_PRIOR || wParam == VK_NEXT))
            PageInput += wParam == VK_NEXT ? 1 : -1;
        else if (message == WM_KEYDOWN && (wParam == VK_HOME || wParam == VK_END))
            EdgeInput = wParam == VK_END ? 1 : -1;
    }
    inline State& Get() { static auto* value = new State; return *value; }
    inline void (*Frame)() = nullptr;
    inline void (*Scene)() = nullptr;
    inline void Log(const char* text)
    {
        OutputDebugStringA(("Gun native menu: " + std::string(text) + "\n").c_str());
    }
    inline std::string NativeText(const std::string& text)
    {
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0 || count > 8192) return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(), count);
        CharUpperBuffW(wide.data(), count);
        std::string result;
        for (wchar_t c : wide)
        {
            if (c == L'\n') result += '\n';
            else if (c >= 32 && c <= 255 && c != L'\\') result += static_cast<char>(c);
            else if (c == L'\\') result += "\\\\\\";
            else result += '?';
        }
        return result;
    }
    inline bool Open() { return Get().page != SIZE_MAX; }
    inline bool Current()
    {
        auto& state = Get();
        return state.anchor.element && Lookup("pause_left_menu_anchor") == state.anchor.element &&
               Lookup("options_display") && !Lookup("dialog_box_anchor") && !Lookup("keyboard_anchor");
    }
    inline uint32_t Selected(const char* menu)
    {
        auto* element = Lookup(menu);
        return Read<uint8_t>(element, 396) == 1 ? Read<uint32_t>(element, 392) : 0;
    }

    // A script command of the game (a C function of its script table), run outside a script
    inline bool Call(const char* name, const Parameters& parameters)
    {
        if (!Read<void*>((MenuSymbols.is_initialized() ? MenuSymbols.get_ptr() : nullptr))) return false;
        auto* entry = symbol(Checksum(name));
        auto* known = symbol(Checksum("SetScreenElementProps"));
        if (!entry || !known || Read<uint8_t>(entry, 2) != Read<uint8_t>(known, 2)) return false;
        auto function = Read<bool(__cdecl*)(const Structure*, void*)>(entry, 12);
        return function && function(&parameters.value, nullptr);
    }
    inline void Fire(const char* type, const char* target, uint32_t child = 0)
    {
        Parameters data;
        data.Name("child_id", child);
        Parameters parameters;
        parameters.Name("type", type);
        parameters.Name("target", target);
        if (child) parameters.Nested("data", &data.value);
        Call("FireEvent", parameters);
    }
    // A colour of the game's pause_menu_colors (right_selected, right_unselected...)
    inline std::vector<uint32_t> MenuColor(const char* name)
    {
        auto* entry = symbol(Checksum("pause_menu_colors"));
        if (!entry || Read<uint8_t>(entry, 2) != 10) return {};
        auto* colors = Read<const Structure*>(entry, 12);
        for (auto* item = colors ? colors->head : nullptr; item; item = item->next)
        {
            if (item->name != Checksum(name) || (item->encodedType >> 1) != 12) continue;
            auto* array = reinterpret_cast<const Array*>(item->value);
            if (!array || array->count != 4) return {};
            auto* values = reinterpret_cast<const uint32_t*>(array->data);
            return { values[0], values[1], values[2], values[3] };
        }
        return {};
    }
    // The game moves the selector arrows apart for the longer words of the other languages
    inline float ArrowOffset()
    {
        if (Read<int>((MenuLanguage.is_initialized() ? MenuLanguage.get_ptr() : nullptr)) == 2) return 0.0f;
        auto* entry = symbol(Checksum("menu_non_english_arrow_offset"));
        if (!entry || Read<uint8_t>(entry, 2) != 5) return 0.0f;
        auto* pair = Read<const float*>(entry, 12);
        return pair ? pair[0] : 0.0f;
    }

    inline uint32_t Suffix(uint32_t id, const char* suffix)
    {
        // The game's AppendSuffixToChecksum: the checksum continued over the suffix
        for (; *suffix; ++suffix)
        {
            id ^= static_cast<uint8_t>(*suffix);
            for (int bit = 0; bit < 8; ++bit) id = (id >> 1) ^ ((id & 1) ? 0xEDB88320 : 0);
        }
        return id;
    }
    inline uint32_t RowId(size_t index)
    {
        return Checksum(("wfp_gun_row_" + std::to_string(index)).c_str());
    }
    inline uint32_t JoinedId(size_t index)
    {
        return Checksum(("wfp_gun_display_" + std::to_string(index)).c_str());
    }
    inline void Release()
    {
        auto& state = Get();
        if (state.anchor.element) unlink(state.anchor.element, &state.anchor);
        state.anchor = {};
        state.page = SIZE_MAX;
        state.display = state.modelOpen = false;
        state.focused = 0;
        state.notice.clear();
        state.items.clear();
        state.pending.clear();
        ++state.generation;
    }
    inline void Model(bool force = false)
    {
        auto& state = Get();
        auto language = AutoLanguage();
        if (!force && state.modelOpen)
        {
            state.model.SelectLanguage(language);
            return;
        }
        state.language = language;
        state.model.OpenMenu(state.module, language, Slots, joined);
        state.modelOpen = true;
    }
    inline void Layout()
    {
        if (auto* menu = Lookup("pause_right_menu"))
            reinterpret_cast<char(__thiscall*)(void*)>(Native().layout)(menu);
    }

    // The value between the selector arrows: the game's font and size, narrowed for words longer than its own
    inline void ValueText(Parameters& parameters, const std::string& value)
    {
        const auto text = NativeText(value);
        parameters.Text("text", text);
        parameters.XY("scale", std::min(1.0f, 8.0f / static_cast<float>(std::max<size_t>(1, text.size()))), 0.7f);
    }
    // A row of the native right menu (make_right_menu_item). selector: the label at the left and the value in the
    // middle, the game's selector layout; arrows: the selector's arrows around the value.
    inline bool Row(uint32_t id, size_t index, const std::string& label, const std::string& value, bool selector, bool arrows)
    {
        Parameters parameters;
        parameters.Name("id", id);
        parameters.Name("parent", "pause_right_menu");
        parameters.Text("text", NativeText(label));
        parameters.Name("pad_choose_script", "nullscript");
        parameters.Integer("index", static_cast<uint32_t>(index));
        // A type the item script doesn't build keeps its label where its selectors have it
        if (selector) parameters.Name("type", "wfp_value");
        if (!Lookup(id) && !Run("make_right_menu_item", parameters)) return false;
        if (!Lookup(id)) return false;
        if (!selector)
        {
            // A plugin or category name on one line across the row, centred where the game centres its own
            Parameters text;
            text.XY("dims", 256.0f, 40.0f);
            // The block lays out its text again when the text is set
            text.Text("text", NativeText(label));
            Properties(Suffix(id, "_text"), text);
            return true;
        }
        if (Lookup(Suffix(id, "_value"))) return true;
        // The selector's elements as make_right_menu_selector makes them, under the names its highlight colours
        auto color = MenuColor("right_unselected");
        ArrayView rgba(1, color);
        ArrayView centre(13, { Checksum("center"), Checksum("center") });
        Parameters text;
        text.Name("parent", id);
        text.Name("local_id", "selector_text");
        text.Name("font", "dialog");
        text.List("just", &centre.value);
        if (color.size() == 4) text.List("rgba", &rgba.value);
        ValueText(text, value);
        text.Integer("z_priority", 202);
        text.XY("pos", 214.0f, 1.0f);
        if (!Create(Suffix(id, "_value"), Checksum("TextElement"), text)) return false;
        if (!arrows) return true;
        const float offset = ArrowOffset();
        for (int side = 0; side < 2; ++side)
        {
            ArrayView just(13, { Checksum(side ? "left" : "right"), Checksum("center") });
            Parameters arrow;
            arrow.Name("parent", id);
            arrow.Name("local_id", side ? "right_arrow" : "left_arrow");
            arrow.Name("texture", "arrow_LRUD");
            arrow.List("just", &just.value);
            if (color.size() == 4) arrow.List("rgba", &rgba.value);
            arrow.Integer("z_priority", 202);
            arrow.XY("pos", side ? 274.0f + offset : 164.0f - offset, 6.5f);
            arrow.XY("scale", 2.0f, 2.0f);
            arrow.Number("rot_angle", side ? -135.0f : 45.0f);
            arrow.Number("scale360", 0.5f);
            if (!Create(Suffix(id, side ? "_right" : "_left"), Checksum("SpriteElement"), arrow)) return false;
        }
        return true;
    }
    inline bool Selector(const RWGTAMenu::Row& row)
    {
        return !RWGTAMenu::Model::Opens(row);
    }
    inline bool Arrows(const RWGTAMenu::Row& row)
    {
        return row.kind == RWGTAMenu::RowKind::Setting || row.kind == RWGTAMenu::RowKind::Preset;
    }
    // The label and value of a row already shown
    inline void Refresh(uint32_t id, const std::string& label, const std::string& value)
    {
        if (Lookup(Suffix(id, "_text")))
        {
            Parameters text;
            text.Text("text", NativeText(label));
            Properties(Suffix(id, "_text"), text);
        }
        if (Lookup(Suffix(id, "_value")))
        {
            Parameters text;
            ValueText(text, value);
            Properties(Suffix(id, "_value"), text);
        }
    }
    inline void RefreshJoined()
    {
        auto& state = Get();
        const auto& rows = state.model.JoinedRows();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            if (Lookup(JoinedId(i))) Refresh(JoinedId(i), state.model.Label(rows[i]), state.model.Value(rows[i]));
            else Row(JoinedId(i), i + 8, state.model.Label(rows[i]), state.model.Value(rows[i]), true, Arrows(rows[i]));
        }
        Layout();
    }
    inline const Item* Focused()
    {
        auto& state = Get();
        for (const auto& item : state.items)
            if (item.id == state.focused) return &item;
        return nullptr;
    }
    // The selected row's description under the list; an update notice while it is shown
    inline void Help()
    {
        auto& state = Get();
        std::string text = state.notice;
        if (text.empty())
        {
            if (auto* item = Focused())
                text = state.model.Help(*item->row);
            if (state.model.RestartRequired())
                text += (text.empty() ? "" : "\n") + state.model.Text("menu.restartwarning");
        }
        Parameters body;
        body.Text("text", NativeText(text));
        if (Lookup("wfp_gun_help"))
        {
            Properties(Checksum("wfp_gun_help"), body);
            return;
        }
        body.Name("parent", "pause_right_menu_anchor");
        // In the font and size of the game's control hints at the bottom
        body.Name("font", "dialog");
        body.XY("dims", 256.0f / 0.7f, 80.0f / 0.7f);
        body.XY("pos", 0.0f, 20.0f + Slots * RowStep);
        body.Number("scale", 0.7f);
        body.Integer("z_priority", 205);
        ArrayView justify(13, { Checksum("left"), Checksum("top") });
        auto color = MenuColor("right_unselected");
        ArrayView rgba(1, color);
        body.List("just", &justify.value);
        body.List("internal_just", &justify.value);
        if (color.size() == 4) body.List("rgba", &rgba.value);
        Create(Checksum("wfp_gun_help"), Checksum("TextBlockElement"), body);
    }
    // The page as a native scrolling right menu with every row; the game scrolls it and moves the selection
    inline bool Build(size_t page)
    {
        auto& state = Get();
        if (!Current() || page >= state.model.Pages().size()) return false;
        state.busy = true;
        state.items.clear();
        state.page = page;
        state.focused = 0;
        const auto rows = state.model.ListRows(page, false);
        Parameters menu;
        ArrayView empty(10, {});
        menu.List("menu_struct", &empty.value);
        menu.Name("pad_back_script", "nullscript");
        menu.XY("pos", 288.0f, ListTop + RowStep * 0.5f);
        menu.XY("dims", 256.0f, Slots * RowStep);
        menu.Text("subtitle", NativeText(state.model.Pages()[page].title));
        menu.Flag("scrolling");
        menu.Flag("no_focus");
        bool built = Run("make_right_menu", menu);
        if (built)
        {
            // The rows have a height and are centred on their place: the list is half a row lower than the script
            // puts it, the title where it has it
            Parameters title;
            title.XY("pos", 0.0f, -RowStep * 0.5f);
            Properties(Checksum("pause_right_menu_subtitle"), title);
        }
        for (size_t i = 0; built && i < rows.size(); ++i)
        {
            const auto& row = *rows[i];
            const auto id = RowId(i);
            built = Row(id, i, state.model.Label(row), Selector(row) ? state.model.Value(row) : std::string{}, Selector(row), Arrows(row));
            state.items.push_back({ id, rows[i] });
            // The item script leaves its rows without a height (the game's scrolling menus list text rows); the
            // scrolling menu wraps by the height of its rows and divides by zero without one
            Parameters container;
            container.XY("dims", 256.0f, RowStep);
            if (built) Properties(id, container);
        }
        if (built && !state.items.empty())
        {
            Layout();
            // The row the page was left on (the one a page opened from), else its first
            const auto key = state.model.Window(page).SelectedKey();
            state.focused = state.items.front().id;
            for (const auto& item : state.items)
                if (reinterpret_cast<MenuList::Key>(item.row) == key) state.focused = item.id;
            Fire("unfocus", "pause_left_menu");
            Fire("focus", "pause_right_menu", state.focused);
            Help();
        }
        state.busy = false;
        return built && !state.items.empty();
    }
    inline void Close()
    {
        auto& state = Get();
        state.page = SIZE_MAX;
        state.focused = 0;
        state.notice.clear();
        state.items.clear();
        Parameters parameters;
        Run("return_to_left_menu", parameters);
    }
    inline void Dispatch(const Pending& input)
    {
        auto& state = Get();
        if (input.generation != state.generation || input.root != state.anchor.element || !Current()) return;
        const int direction = input.command == Command::Left ? -1 : input.command == Command::Right ? 1 : 0;
        if (!Open())
        {
            if (input.selected == Entry && input.command == Command::Choose)
            {
                state.display = false;
                state.model.Window(RWGTAMenu::Model::RootPage).Reset();
                if (!Build(RWGTAMenu::Model::RootPage)) Close();
                return;
            }
            for (size_t i = 0; i < state.model.JoinedRows().size(); ++i)
            {
                if (input.selected != JoinedId(i)) continue;
                state.model.Change(state.model.JoinedRows()[i], direction, state.pause);
                RefreshJoined();
                RWGTAMenu::Model::Changed() = false;
                return;
            }
            return;
        }
        if (input.command == Command::Back)
        {
            const auto parent = state.model.Back(state.page);
            state.notice.clear();
            if (parent == SIZE_MAX || !Build(parent)) Close();
            return;
        }
        auto item = std::find_if(state.items.begin(), state.items.end(), [&](const auto& row) { return row.id == input.selected; });
        if (item == state.items.end()) return;
        const auto row = *item->row;
        if (RWGTAMenu::Model::Opens(row))
        {
            if (input.command != Command::Choose) return;
            state.model.Window(state.page).SelectKey(reinterpret_cast<MenuList::Key>(item->row));
            const auto target = state.model.Enter(row);
            state.notice.clear();
            if (target != SIZE_MAX && !Build(target)) Close();
            return;
        }
        if (!state.model.Enabled(row, state.pause)) return;
        state.model.Change(row, direction, state.pause);
        for (const auto& shown : state.items)
            Refresh(shown.id, state.model.Label(*shown.row), Selector(*shown.row) ? state.model.Value(*shown.row) : std::string{});
        Help();
        RWGTAMenu::Model::Changed() = false;
    }
    inline char __fastcall Launch(void* manager, void*, uint32_t type, uint32_t target, uint32_t source, Structure* parameters, char flag, float radius)
    {
        auto& state = Get();
        Command command;
        bool input = true;
        if (type == Checksum("pad_choose") || type == Checksum("pad_start")) command = Command::Choose;
        else if (type == Checksum("pad_left")) command = Command::Left;
        else if (type == Checksum("pad_right")) command = Command::Right;
        else if (type == Checksum("pad_back")) command = Command::Back;
        else input = false;
        if (state.enabled && input && !state.busy && Current() && state.pending.size() < 32)
        {
            auto selected = Selected(Open() || state.display ? "pause_right_menu" : "pause_left_menu");
            bool own = Open() || selected == Entry;
            for (size_t i = 0; !own && i < state.model.JoinedRows().size(); ++i)
                own = selected == JoinedId(i);
            if (own && (Open() || command != Command::Back))
            {
                // Only the native active menu dispatch is consumed, once. Rebuild after the outer scheduler unwinds.
                state.pending.push_back({ command, state.generation, selected, state.anchor.element });
                return 1;
            }
        }
        return state.launchHook.thiscall<char>(manager, type, target, source, parameters, flag, radius);
    }
    inline void Tick()
    {
        auto& state = Get();
        if (!state.enabled || state.busy) return;
        if (state.modelOpen && !Current()) Release();
        if (!Current() && Lookup("options_display") && Lookup("pause_left_menu") && Script("make_right_menu"))
        {
            lookup(&state.anchor, Checksum("pause_left_menu_anchor"), 0);
            if (!Current()) { Release(); return; }
            ++state.generation;
            Model(true);
            state.pause = Lookup("options_cheats") != nullptr;
            if (!Lookup(Entry))
            {
                Parameters parameters;
                parameters.Name("id", Entry);
                parameters.Name("parent", "pause_left_menu");
                parameters.Text("text", NativeText(state.model.Text("menu.plugins")));
                parameters.Name("pad_choose_script", "nullscript");
                parameters.Number("hilite_scale", 1.1f);
                state.busy = true;
                Run("make_left_menu_item", parameters);
                state.busy = false;
            }
        }
        if (Frame) Frame();
        auto pending = std::move(state.pending);
        state.pending.clear();
        for (const auto& input : pending) Dispatch(input);
        if (!Current()) return;
        if (Open())
        {
            // The game moved the selection: the model's window follows it, and the help shows the new row's
            const auto selected = Selected("pause_right_menu");
            if (selected && selected != state.focused)
            {
                state.focused = selected;
                state.notice.clear();
                if (auto* item = Focused())
                    state.model.Window(state.page).SelectKey(reinterpret_cast<MenuList::Key>(item->row));
                Help();
            }
            // A wheel notch moves the selection a row (stopping at the ends), the page keys a window of rows; the
            // game's list follows the selection
            auto& window = state.model.Window(state.page);
            bool moved = false;
            for (int notches = state.wheel.Take(WheelInput.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
                moved |= window.Move(notches > 0 ? -1 : 1, false);
            if (const int page = PageInput.exchange(0)) moved |= window.Page(page);
            if (const int edge = EdgeInput.exchange(0)) moved |= window.Edge(edge);
            // LB/RB page like Page Up/Page Down
            XINPUT_STATE pad{};
            DWORD slot{};
            const bool read = PadInput::Read(pad, slot);
            const auto now = GetTickCount64();
            for (int i = 0; i < 2; ++i)
            {
                const WORD button = i ? XINPUT_GAMEPAD_RIGHT_SHOULDER : XINPUT_GAMEPAD_LEFT_SHOULDER;
                if (state.shoulders[i].Step(read && (pad.Gamepad.wButtons & button), now))
                    moved |= window.Page(i ? 1 : -1);
            }
            for (const auto& item : state.items)
            {
                if (!moved || reinterpret_cast<MenuList::Key>(item.row) != window.SelectedKey()) continue;
                Parameters sound;
                Run("generic_menu_up_or_down_sound", sound);
                // A focused menu keeps its child on a focus event; it takes the given child after an unfocus
                Fire("unfocus", "pause_right_menu");
                Fire("focus", "pause_right_menu", item.id);
            }
        }
        else
        {
            WheelInput = PageInput = EdgeInput = 0;
            state.wheel.Clear();
            state.display = Lookup("display_hideHUD") != nullptr;
            if (state.display && !Lookup(JoinedId(0)))
            {
                state.busy = true;
                RefreshJoined();
                state.busy = false;
            }
        }
        if (RWGTAMenu::Model::Changed().exchange(false))
        {
            Model();
            if (Open()) { if (!Build(state.page)) Close(); }
            else if (state.display) RefreshJoined();
        }
    }
    // The game's own flag for a shown menu: the root window's menu_state tag (entering, on, leaving; off in play)
    inline bool MenuShown()
    {
        auto* tags = Read<const Structure*>(Lookup("root_window"), 44);
        for (auto* item = tags ? tags->head : nullptr; item; item = item->next)
            if (item->name == Checksum("menu_state"))
                return item->value != Checksum("off");
        return false;
    }
    inline void __cdecl Update()
    {
        auto& state = Get();
        ++state.depth;
        state.updateHook.ccall<void>();
        --state.depth;
        if (state.depth) return;
        if (Scene) Scene();
        try { Tick(); }
        catch (...)
        {
            state.enabled = false;
            Log("Native menu unavailable; original menu retained");
        }
        // Drops stay off the game's menus: the pause menus set the root window's flag, the front end menus and
        // dialogs are found by their anchors
        static const uint32_t screens[] = { Checksum("main_menu_anchor"), Checksum("startup_main_menu_anchor"),
            Checksum("difficulty_menu_anchor"), Checksum("files_vmenu"), Checksum("pause_left_menu_anchor"),
            Checksum("dialog_box_anchor"), Checksum("fancy_message_anchor"), Checksum("keyboard_anchor") };
        const bool menu = MenuShown() ||
            std::any_of(std::begin(screens), std::end(screens), [](uint32_t id) { return Lookup(id) != nullptr; });
        RainDroplets::MenuOpen(menu);
    }
    inline void Init()
    {
        Resolve();
        auto& state = Get();
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&Init), &state.module);
        state.launchHook = safetyhook::create_inline(reinterpret_cast<void*>(Native().launch), Launch);
        state.updateHook = safetyhook::create_inline(reinterpret_cast<void*>(Native().update), Update);
        state.enabled = true;
    }
} // namespace GunMenu
