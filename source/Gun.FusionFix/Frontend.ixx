module;

#include <stdafx.h>
#include <d3d9.h>
#include "FixSettings.hpp"
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include "Shared/RWGTA/FixMenu.hpp"
#include <fstream>

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
        size_t page{ SIZE_MAX };
        RWGTAMenu::Row row{};
        bool back{}, next{}, previous{}, details{};
    };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        SafetyHookInline updateHook, launchHook;
        WeakSlot anchor{};
        std::deque<Pending> pending;
        std::vector<Item> items;
        uint32_t generation{ 1 };
        unsigned depth{};
        size_t page{ SIZE_MAX }, slice{};
        bool busy{}, open{}, display{}, pause{}, enabled{}, modelOpen{}, details{}, notice{};
        size_t helpSlice{};
        std::string language, help;
    };
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
    inline void Release()
    {
        auto& state = Get();
        if (state.anchor.element) unlink(state.anchor.element, &state.anchor);
        state.anchor = {};
        state.open = state.display = false;
        state.details = state.modelOpen = state.notice = false;
        state.help.clear();
        state.items.clear();
        state.pending.clear();
        ++state.generation;
    }
    inline void Model(bool force = false)
    {
        auto& state = Get();
        auto language = AutoLanguage();
        if (!force && state.modelOpen && state.language == language) return;
        state.language = language;
        state.model.OpenPlugins(state.module, language, 32, joined);
        state.modelOpen = true;
    }
    inline void Layout()
    {
        if (auto* menu = Lookup("pause_right_menu"))
            reinterpret_cast<char(__thiscall*)(void*)>(Native().layout)(menu);
    }
    inline uint32_t RowId(size_t index)
    {
        return Checksum(("wfp_gun_row_" + std::to_string(index)).c_str());
    }
    inline bool Row(uint32_t id, const std::string& label, const std::string& value, size_t index, bool privatePage)
    {
        Parameters parameters;
        parameters.Name("id", id);
        parameters.Name("parent", "pause_right_menu");
        parameters.Text("text", NativeText(label));
        parameters.Name("pad_choose_script", "nullscript");
        parameters.Integer("index", static_cast<uint32_t>(index));
        if (!Lookup(id) && !Run("make_right_menu_item", parameters)) return false;
        if (!Lookup(id)) return false;
        // Keep the native focus container and its text child; give complete labels and values two lines.
        uint32_t child = id;
        for (char c : std::string("_text"))
        {
            child ^= static_cast<uint8_t>(c);
            for (int bit = 0; bit < 8; ++bit) child = (child >> 1) ^ ((child & 1) ? 0xEDB88320 : 0);
        }
        Parameters text;
        text.Text("text", NativeText(label + (value.empty() ? "" : "\n" + value)));
        text.XY("dims", 256.0f, 48.0f);
        text.XY("pos", 128.0f, 0.0f);
        text.Number("scale", 0.85f);
        ArrayView justify(13, { Checksum("left"), Checksum("center") });
        text.List("just", &justify.value);
        text.List("internal_just", &justify.value);
        if (!Properties(child, text)) return false;
        Parameters container;
        container.XY("dims", 256.0f, 48.0f);
        container.List("just", &justify.value);
        container.List("internal_just", &justify.value);
        Properties(id, container);
        if (privatePage) Get().items.push_back({ id });
        return true;
    }
    inline void RefreshJoined()
    {
        auto& state = Get();
        const auto& rows = state.model.JoinedRows();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto id = Checksum(("wfp_gun_display_" + std::to_string(i)).c_str());
            Row(id, state.model.Label(rows[i]), state.model.Value(rows[i]), i + 8, false);
        }
        Layout();
    }
    inline std::vector<std::string> HelpParts()
    {
        std::vector<std::string> parts;
        auto text = Get().help;
        if (text.empty()) text = Get().model.Text("menu.presethelp");
        while (!text.empty())
        {
            size_t count = std::min<size_t>(180, text.size());
            if (count < text.size())
            {
                auto space = text.rfind(' ', count);
                if (space != std::string::npos && space > 80) count = space;
                else while (count && (static_cast<unsigned char>(text[count]) & 0xC0) == 0x80) --count;
            }
            parts.push_back(text.substr(0, count));
            text.erase(0, count);
        }
        return parts;
    }
    inline bool BuildHelp()
    {
        auto& state = Get();
        state.busy = true;
        state.items.clear();
        Parameters menu;
        ArrayView empty(10, {});
        menu.List("menu_struct", &empty.value);
        menu.Name("pad_back_script", "nullscript");
        menu.XY("pos", 288.0f, 96.0f);
        menu.Text("subtitle", NativeText(state.model.Text(state.notice ? "menu.plugins" : "menu.details")));
        if (!Run("make_right_menu", menu)) { state.busy = false; return false; }
        auto parts = HelpParts();
        if (parts.empty()) parts.emplace_back();
        state.helpSlice = std::min(state.helpSlice, parts.size() - 1);
        Parameters body;
        body.Name("type", "TextBlockElement");
        body.Name("id", "wfp_gun_help");
        body.Name("parent", "pause_right_menu_anchor");
        body.Name("font", "small");
        body.Text("text", NativeText(parts[state.helpSlice]));
        body.XY("dims", 256.0f, 112.0f);
        body.XY("pos", 0.0f, 32.0f);
        body.Number("scale", 0.8f);
        body.Integer("z_priority", 205);
        ArrayView justify(13, { Checksum("left"), Checksum("top") });
        ArrayView color(1, { 40, 32, 24, 255 });
        body.List("just", &justify.value);
        body.List("internal_just", &justify.value);
        body.List("rgba", &color.value);
        if (!Create(Checksum("wfp_gun_help"), Checksum("TextBlockElement"), body))
        {
            state.busy = false;
            return false;
        }
        Parameters navigation;
        navigation.XY("pos", 0.0f, 156.0f);
        navigation.Number("regular_space_amount", 40.0f);
        Properties(Checksum("pause_right_menu"), navigation);
        if (state.helpSlice)
            if (Row(RowId(state.items.size()), state.model.Text("menu.previous"), {}, state.items.size(), true)) state.items.back().previous = true;
        if (state.helpSlice + 1 < parts.size())
            if (Row(RowId(state.items.size()), state.model.Text("menu.next"), {}, state.items.size(), true)) state.items.back().next = true;
        if (Row(RowId(state.items.size()), state.model.Text("menu.back"), {}, state.items.size(), true)) state.items.back().back = true;
        Layout();
        state.busy = false;
        return Lookup("wfp_gun_help") && !state.items.empty();
    }
    inline bool Build()
    {
        auto& state = Get();
        if (!Current()) return false;
        state.busy = true;
        state.items.clear();
        Parameters menu;
        ArrayView empty(10, {});
        menu.List("menu_struct", &empty.value);
        menu.Name("pad_back_script", "nullscript");
        menu.XY("pos", 288.0f, 96.0f);
        menu.Text("subtitle", NativeText(state.page == SIZE_MAX ? state.model.Text("menu.plugins") : state.model.Pages()[state.page].title));
        if (!Run("make_right_menu", menu)) { state.busy = false; return false; }
        // The rows start below the page title.
        Parameters spacing;
        spacing.XY("pos", 0.0f, 40.0f);
        spacing.Number("regular_space_amount", 44.0f);
        Properties(Checksum("pause_right_menu"), spacing);
        const auto& pages = state.model.Pages();
        auto visible = state.page == SIZE_MAX ? std::vector<const RWGTAMenu::Row*>{} : state.model.VisibleRows(state.page);
        auto count = state.page == SIZE_MAX ? pages.size() : visible.size();
        auto start = state.slice * 2;
        for (size_t i = start; i < std::min(count, start + 2); ++i)
        {
            auto id = RowId(state.items.size());
            if (state.page == SIZE_MAX)
            {
                if (!Row(id, pages[i].title, {}, state.items.size(), true)) { state.busy = false; return false; }
                state.items.back().page = i;
            }
            else
            {
                const auto& row = *visible[i];
                if (!Row(id, state.model.Label(row), state.model.Value(row), state.items.size(), true)) { state.busy = false; return false; }
                state.items.back().row = row;
            }
        }
        if (start)
        {
            if (Row(RowId(state.items.size()), state.model.Text("menu.previous"), {}, state.items.size(), true)) state.items.back().previous = true;
        }
        if (start + 2 < count)
        {
            if (Row(RowId(state.items.size()), state.model.Text("menu.next"), {}, state.items.size(), true)) state.items.back().next = true;
        }
        if (state.page != SIZE_MAX)
        {
            if (Row(RowId(state.items.size()), state.model.Text("menu.details"),
                    state.model.RestartRequired() ? state.model.Text("menu.restartshort") : std::string{}, state.items.size(), true))
                state.items.back().details = true;
        }
        if (Row(RowId(state.items.size()), state.model.Text("menu.back"), {}, state.items.size(), true)) state.items.back().back = true;
        Layout();
        state.busy = false;
        return !state.items.empty();
    }
    inline void Close()
    {
        Get().open = false;
        Get().details = false;
        Get().items.clear();
        Parameters parameters;
        Run("return_to_left_menu", parameters);
    }
    inline void Dispatch(const Pending& input)
    {
        auto& state = Get();
        if (input.generation != state.generation || input.root != state.anchor.element || !Current()) return;
        if (!state.open)
        {
            if (input.selected == Entry && input.command == Command::Choose)
            {
                state.open = true;
                state.display = false;
                state.page = SIZE_MAX;
                state.slice = 0;
                if (!Build()) Close();
                return;
            }
            for (size_t i = 0; i < state.model.JoinedRows().size(); ++i)
            {
                if (input.selected != Checksum(("wfp_gun_display_" + std::to_string(i)).c_str())) continue;
                const auto& row = state.model.JoinedRows()[i];
                state.model.Change(row, input.command == Command::Left ? -1 : input.command == Command::Right ? 1 : 0, state.pause);
                RefreshJoined();
                return;
            }
            return;
        }
        auto item = std::find_if(state.items.begin(), state.items.end(), [&](const auto& row) { return row.id == input.selected; });
        if (state.details)
        {
            if (input.command == Command::Back || (item != state.items.end() && item->back))
            {
                state.details = false;
                if (state.notice)
                {
                    state.notice = false;
                    state.help.clear();
                }
                if (!Build()) Close();
            }
            else if (item != state.items.end())
            {
                if (item->next) ++state.helpSlice;
                else if (item->previous && state.helpSlice) --state.helpSlice;
                if (!BuildHelp()) Close();
            }
            return;
        }
        if (input.command == Command::Back || (item != state.items.end() && item->back))
        {
            if (state.page == SIZE_MAX) { Close(); return; }
            state.page = SIZE_MAX;
            state.slice = 0;
        }
        else if (item != state.items.end())
        {
            auto selected = *item;
            if (selected.next) ++state.slice;
            else if (selected.previous && state.slice) --state.slice;
            else if (selected.details)
            {
                state.details = true;
                state.helpSlice = 0;
                if (!BuildHelp()) Close();
                return;
            }
            else if (selected.page != SIZE_MAX)
            {
                state.page = selected.page;
                state.slice = 0;
                state.model.PageShown(state.page);
            }
            else if (state.model.Enabled(selected.row, state.pause))
            {
                state.model.Change(selected.row, input.command == Command::Left ? -1 : input.command == Command::Right ? 1 : 0, state.pause);
                for (const auto& row : state.items)
                {
                    if (!row.back && !row.next && !row.previous && !row.details && row.page == SIZE_MAX)
                        Row(row.id, state.model.Label(row.row), state.model.Value(row.row), 0, false);
                    else if (row.details)
                        Row(row.id, state.model.Text("menu.details"), state.model.RestartRequired() ? state.model.Text("menu.restartshort") : std::string{}, 0, false);
                }
                RWGTAMenu::Model::Changed() = false;
                return;
            }
        }
        if (!Build()) Close();
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
            auto selected = Selected(state.open || state.display ? "pause_right_menu" : "pause_left_menu");
            bool own = state.open || selected == Entry;
            for (size_t i = 0; !own && i < state.model.JoinedRows().size(); ++i)
                own = selected == Checksum(("wfp_gun_display_" + std::to_string(i)).c_str());
            if (own && (state.open || command != Command::Back))
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
        if (state.open && !state.details)
        {
            auto selected = Selected("pause_right_menu");
            for (const auto& item : state.items)
                if (item.id == selected && item.page == SIZE_MAX && !item.back && !item.next && !item.previous && !item.details)
                    state.help = state.model.Label(item.row) + "\n" + state.model.Help(item.row);
        }
        if (!state.open)
        {
            state.display = Lookup("display_hideHUD") != nullptr;
            if (state.display && !Lookup(Checksum("wfp_gun_display_0")))
            {
                state.busy = true;
                RefreshJoined();
                state.busy = false;
            }
        }
        if (RWGTAMenu::Model::Changed().exchange(false))
        {
            Model();
            if (state.open) { if (!(state.details ? BuildHelp() : Build())) Close(); }
            else if (state.display) RefreshJoined();
        }
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
    }
    inline void Init()
    {
        Resolve();
        auto& state = Get();
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&Init), &state.module);
        state.launchHook = safetyhook::create_inline(reinterpret_cast<void*>(Native().launch), Launch);
        state.updateHook = safetyhook::create_inline(reinterpret_cast<void*>(Native().update), Update);
        state.enabled = state.launchHook && state.updateHook;
        if (!state.enabled)
        {
            state.launchHook.reset();
            state.updateHook.reset();
            Log("Native menu hooks unavailable");
        }
    }
} // namespace GunMenu
