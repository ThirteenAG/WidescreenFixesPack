module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <cstdint>
#include <cstring>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <iomanip>
#include <sstream>
#include "Shared/RWGTA/FixMenu.hpp"
#include <functional>
#include <map>
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import ComVars;

export
{
namespace THAWMenu
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
} // namespace THAWMenu

namespace THAWMenu
{
    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        if (!base)
            return value;
        std::memcpy(&value, static_cast<const uint8_t*>(base) + offset, sizeof(value));
        return value;
    }
    inline void* find_pattern(const std::string& bytes)
    {
        return hook::pattern(bytes).get_first();
    }
    inline std::string Bytes(const void* data, size_t count)
    {
        std::ostringstream result;
        for (size_t i = 0; i < count; ++i)
            result << std::hex << std::setfill('0') << std::setw(2)
                   << static_cast<unsigned>(static_cast<const uint8_t*>(data)[i]) << ' ';
        return result.str();
    }
    // Script function names and their table rows are data, past the code the default pattern scan covers:
    // the whole image is searched.
    inline void* find_data(const std::string& bytes)
    {
        const auto image = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        const auto header = reinterpret_cast<const IMAGE_NT_HEADERS*>(image + reinterpret_cast<const IMAGE_DOS_HEADER*>(image)->e_lfanew);
        return hook::range_pattern(image, image + header->OptionalHeader.SizeOfImage, bytes).get_first();
    }
    inline void* Native(const char* name)
    {
        auto text = find_data(Bytes(name, std::strlen(name) + 1));
        auto row = find_data(Bytes(&text, sizeof(text)));
        return Read<void*>(row, sizeof(void*));
    }
    struct Bindings
    {
        void*(__cdecl* symbol)(uint32_t){};
        char(__cdecl* run)(uint32_t, const Structure*, void*, char){};
        void*(__cdecl* spawn)(uint32_t, const Structure*, char, uint32_t, char, char, int, const Structure*, int,
                              int){};
        WeakSlot*(__thiscall* lookup)(void*, WeakSlot*, const Structure*, uint32_t, int){};
        void(__thiscall* unlink)(void*, WeakSlot*){};
        bool(__cdecl* props)(const Structure*, void*){};
        bool(__cdecl* destroy)(const Structure*, void*){};
        bool(__cdecl* fire)(const Structure*, void*){};
        void* call{};
        void* update{};
        void* launch{};
    };
    inline Bindings api;
    inline void* Branch(void* address, size_t offset)
    {
        return reinterpret_cast<void*>(injector::GetBranchDestination(static_cast<uint8_t*>(address) + offset).as_int());
    }
    inline void Bind()
    {
        auto run = find_pattern("55 8B 6C 24 08 56 55 E8 ? ? ? ? 8B F0 83 C4 04 85 F6");
        api.run = reinterpret_cast<decltype(api.run)>(run);
        api.symbol = reinterpret_cast<decltype(api.symbol)>(Branch(run, 7));
        NativeSymbols.SetAddress(Read<void**>(reinterpret_cast<uint8_t*>(api.symbol), 9));
        api.spawn = reinterpret_cast<decltype(api.spawn)>(
            find_pattern("53 8B 5C 24 08 56 57 53 E8 ? ? ? ? 8B F8 83 C4 04 85 FF"));
        api.unlink = reinterpret_cast<decltype(api.unlink)>(
            find_pattern("8B 44 24 04 8B 50 04 56 33 F6 3B D6 74 08 8B 48 08 89 4A 08"));
        api.props = reinterpret_cast<decltype(api.props)>(Native("SetScreenElementProps"));
        api.destroy = reinterpret_cast<decltype(api.destroy)>(Native("DestroyScreenElement"));
        api.fire = reinterpret_cast<decltype(api.fire)>(Native("FireEvent"));
        NativeLanguage.SetAddress(Read<int*>(Native("English"), 2));
        auto lookup = find_pattern(
            "8B 0D ? ? ? ? 56 8B 74 24 20 6A 01 68 AF 98 C6 40 56 8D 44 24 10 50 E8 ? ? ? ? 8B 4C 24 04 8B 11 56 C7 44 "
            "24 1C 00 00 00 00 FF 52 04");
        NativeManager.SetAddress(Read<void**>(lookup, 2));
        api.lookup = reinterpret_cast<decltype(api.lookup)>(Branch(lookup, 24));
        auto call = find_pattern("85 C9 6A 00 57 75 03 8B 4E 14 51 50 8B CE 53 E8 ? ? ? ? 5F 5E 5D 32 C0 5B C3");
        api.call = Branch(call, 15);
        api.update = find_pattern("56 57 8B 3D ? ? ? ? 85 FF C6 05 ? ? ? ? 01 C6 05 ? ? ? ? 00 8B F7 0F 84 ? ? ? ?");
        auto controller = find_pattern("68 74 EE B2 36 68 74 EE B2 36 51 8B 4C 24 2C E8");
        api.launch = Branch(controller, 15);
    }
    inline void* Lookup(uint32_t id)
    {
        auto manager = Read<void*>(NativeManager.get_ptr());
        if (!manager)
            return nullptr;
        Parameters parameters;
        parameters.Name("id", id);
        WeakSlot slot;
        api.lookup(manager, &slot, &parameters.value, Checksum("id"), 0);
        auto value = slot.element;
        if (value)
            api.unlink(value, &slot);
        return value;
    }
    inline void* Lookup(const char* name)
    {
        return Lookup(Checksum(name));
    }
    inline void Fire(const char* event, uint32_t target, Structure* data = nullptr)
    {
        Parameters parameters;
        parameters.Name("type", event);
        parameters.Name("target", target);
        parameters.Name("source", target);
        if (data)
            parameters.Nested("data", data);
        api.fire(&parameters.value, nullptr);
    }
    inline bool Script(const char* name)
    {
        if (!Read<void*>(NativeSymbols.get_ptr()))
            return false;
        auto symbol = api.symbol(Checksum(name));
        return Read<uint8_t>(symbol, 2) == 7;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!Read<void*>(NativeSymbols.get_ptr()))
            return false;
        auto symbol = api.symbol(Checksum(name));
        auto type = Read<uint8_t>(symbol, 2);
        if (type != 7 && type != 8)
            return false;
        api.run(Checksum(name), &parameters.value, nullptr, 0);
        return true;
    }
    inline bool Spawn(const char* name, const Parameters& parameters)
    {
        return Script(name) && api.spawn(Checksum(name), &parameters.value, 0, 0, 0, 0, 0, nullptr, 0, 0);
    }
    inline bool Integer(const Structure* parameters, const char* name, int& value)
    {
        if (!parameters)
            return false;
        auto component = Read<Component*>(parameters, 4);
        for (size_t i = 0; component && i < 256; ++i)
        {
            if (Read<uint32_t>(component, 4) == Checksum(name))
            {
                value = Read<uint32_t>(component, 8);
                return true;
            }
            component = Read<Component*>(component, 12);
        }
        return false;
    }
    inline bool Properties(const Parameters& parameters)
    {
        int id{};
        // The native name getter uses the same component representation as integer.
        for (auto component = parameters.value.head; component; component = component->next)
            if (component->name == Checksum("id"))
                id = component->value;
        return Lookup(id) && api.props(&parameters.value, nullptr);
    }
} // namespace THAWMenu

namespace THAWMenu
{
    constexpr auto Entry = Checksum("wfp_thaw_plugins");
    inline const std::set<std::string> joined{ "MAIN.HudAspectRatioConstraint" };
    enum class Command
    {
        Open,
        Choose,
        Left,
        Right,
        Back,
        Focus,
        Joined,
        JoinedLeft,
        JoinedFocus
    };
    struct Input
    {
        Command command{};
        uint32_t generation{};
        size_t index{};
        void* root{};
    };
    struct Item
    {
        size_t page{ SIZE_MAX };
        RWGTAMenu::Row row{};
        std::string label, value, help;
        uint32_t id{};
        bool back{};
    };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        SafetyHookInline callHook, updateHook, launchHook;
        std::deque<Input> pending;
        std::vector<Item> items;
        void* root{};
        WeakSlot anchor{};
        uint32_t generation{ 1 };
        uint32_t focus{};
        size_t page{ SIZE_MAX }, selected{};
        unsigned depth{};
        bool registered{}, busy{}, open{}, pause{}, modelOpen{}, display{}, ready{};
        std::string language;
        WeakSlot returnWindow{};
        bool returnToEntry{}, returnPause{};
        std::vector<std::string> helpLines;     // the help in the helper bar, one line at a time
        size_t helpLine{};
        ULONGLONG helpTime{};                   // when the next help line is due
    };
    State state;
    inline std::string notice;                  // the update notice, shown on the Plugins list
    inline std::function<void()> updateFrame;
    // Startup values of the restart settings: the restart line shows while one of them differs
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartBaseline;
    inline bool NeedsRestart()
    {
        bool pending = false;
        for (const auto& page : state.model.Pages())
        {
            for (const auto& row : page.rows)
            {
                if (!row.plugin || !row.setting || row.setting->apply != "restart" || !row.setting->HasValue())
                    continue;
                auto value = OtherPlugins::Get(*row.plugin, *row.setting);
                auto baseline = restartBaseline.emplace(std::make_pair(row.plugin->module, row.setting->id), value).first;
                pending |= !asi_settings::text::IEquals(baseline->second, value);
            }
        }
        return pending;
    }
    inline void Log(const char* message)
    {
        OutputDebugStringA("THAW native menu: ");
        OutputDebugStringA(message);
        OutputDebugStringA("\n");
    }
    inline std::string NativeText(const std::string& text, bool uppercase = false)
    {
        auto count =
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0 || count > 4096)
            return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), wide.data(),
                            count);
        if (uppercase)
        {
            std::wstring locale(state.language.begin(), state.language.end());
            auto size =
                LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), count, nullptr, 0, nullptr, nullptr, 0);
            if (size > 0)
            {
                std::wstring upper(size, L'\0');
                LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), count, upper.data(), size, nullptr, nullptr,
                              0);
                wide = std::move(upper);
                count = size;
            }
        }
        const auto codePage = Read<int>(NativeLanguage.get_ptr()) == 9 ? 1251u : 1252u;
        count = WideCharToMultiByte(codePage, WC_NO_BEST_FIT_CHARS, wide.data(), count, nullptr, 0, "?", nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(codePage, WC_NO_BEST_FIT_CHARS, wide.data(), static_cast<int>(wide.size()), result.data(),
                            count, "?", nullptr);
        return EscapeNativeText(result, true);
    }
    inline void ReleaseRoot()
    {
        if (state.anchor.element)
            api.unlink(state.anchor.element, &state.anchor);
        state.anchor = {};
        state.root = nullptr;
    }
    inline void AdoptRoot()
    {
        ReleaseRoot();
        auto* manager = Read<void*>(NativeManager.get_ptr());
        if (!manager)
            return;
        Parameters parameters;
        parameters.Name("id", "current_menu_anchor");
        api.lookup(manager, &state.anchor, &parameters.value, Checksum("id"), 0);
        state.root = state.anchor.element;
    }
    inline bool Current()
    {
        return state.root && state.anchor.element == state.root && Lookup("current_menu_anchor") == state.root &&
               Lookup("current_menu") && !Lookup("dialog_box_anchor") && !Lookup("keyboard_anchor");
    }
    inline bool Intercept(uint32_t type)
    {
        Command command;
        if (type == Checksum("pad_choose") || type == Checksum("pad_start"))
            command = Command::Choose;
        else if (type == Checksum("pad_left"))
            command = Command::Left;
        else if (type == Checksum("pad_right"))
            command = Command::Right;
        else if (type == Checksum("pad_back"))
            command = Command::Back;
        else
            return false;
        if (state.busy || state.pending.size() >= 32 || !Current())
            return false;
        auto* menu = Lookup("current_menu");
        if (Read<uint8_t>(menu, 396) != 1)
            return false;
        auto selected = Read<uint32_t>(menu, 392);
        if (!state.open)
        {
            if (state.display && command != Command::Back)
            {
                for (size_t i = 0; i < state.model.JoinedRows().size(); ++i)
                {
                    if (selected != Checksum(("wfp_thaw_joined_" + std::to_string(i)).c_str()))
                        continue;
                    state.pending.push_back({ command == Command::Left ? Command::JoinedLeft : Command::Joined,
                                              state.generation, i, state.root });
                    return true;
                }
            }
            if (command != Command::Choose || selected != Entry)
                return false;
            state.pending.push_back({ Command::Open, state.generation, 0, state.root });
            return true;
        }
        if (command == Command::Back)
        {
            state.pending.push_back({ command, state.generation, 0, state.root });
            return true;
        }
        for (size_t i = 0; i < state.items.size(); ++i)
        {
            const auto& item = state.items[i];
            if (item.id != selected)
                continue;
            if (command != Command::Choose &&
                (item.page != SIZE_MAX || item.back ||
                 (item.row.kind != RWGTAMenu::RowKind::Setting && item.row.kind != RWGTAMenu::RowKind::Preset)))
                return true;
            state.pending.push_back({ command, state.generation, i, state.root });
            return true;
        }
        return false;
    }
    inline uint8_t __fastcall Launch(void* manager, void*, uint32_t type, uint32_t target, uint32_t source,
                                     Structure* parameters, int flags, float radius)
    {
        if (Intercept(type))
            return 1;
        return state.launchHook.thiscall<uint8_t>(manager, type, target, source, parameters, flags, radius);
    }
    inline void Model()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            switch (Read<int>(NativeLanguage.get_ptr()))
            {
            case 3:
                language = "fr";
                break;
            case 4:
                language = "es";
                break;
            case 5:
                language = "de";
                break;
            case 6:
                language = "it";
                break;
            default:
                language = "en";
                break;
            }
        }
        if (state.modelOpen && state.language == language)
            return;
        state.model.Close();
        state.language = language;
        state.model.OpenPlugins(state.module, state.language, 128, joined);
        state.modelOpen = true;
    }
    inline bool Row(const Item& item, Command command)
    {
        Parameters row;
        row.Name("id", item.id);
        row.Name("text_id", item.id ^ 0x1234ABCD);
        row.Name("extra_text_id", item.id ^ 0x2345BCDE);
        const bool labelMenu = command == Command::Open && !state.open;
        row.Text("text", NativeText(item.label, labelMenu));
        row.Text("extra_text", NativeText(item.value));
        row.Name("pad_choose_script", "nullscript");
        row.Name("pad_left_script", "nullscript");
        row.Name("pad_right_script", "nullscript");
        // Rows next to the game's own display options look like them
        if (command != Command::Joined)
        {
            row.Flag("no_bg");
            row.Number("scale", 0.8f);
        }
        if (item.page != SIZE_MAX || item.back)
            row.Flag("centered");
        auto helper = labelMenu ? "add_label_menu_item" : "theme_menu_add_item";
        if (!Run(helper, row) || !Lookup(item.id))
            return false;
        return true;
    }
    // One line in the game's helper text bar (the bottom information area)
    inline void Line(const std::string& text)
    {
        Parameters line;
        line.Text("text", NativeText(text));
        ArrayView lines(10, { Parameters::Pointer(&line.value) });
        Parameters parameters;
        parameters.List("helper_text_elements", &lines.value);
        Run("create_helper_text", parameters);
    }
    // Lines that fit the helper text bar, broken at spaces
    inline std::vector<std::string> Lines(const std::string& text, size_t width = 64)
    {
        std::vector<std::string> lines;
        std::string line;
        for (size_t offset = 0; offset < text.size();)
        {
            auto end = text.find(' ', offset);
            if (end == std::string::npos)
                end = text.size();
            auto word = text.substr(offset, end - offset);
            offset = end + 1;
            if (word.empty())
                continue;
            if (!line.empty() && line.size() + 1 + word.size() > width)
            {
                lines.push_back(std::move(line));
                line = std::move(word);
            }
            else
                line = line.empty() ? std::move(word) : line + " " + word;
        }
        if (!line.empty())
            lines.push_back(std::move(line));
        return lines;
    }
    // The help runs line by line in the helper text bar, a line every three seconds, repeated; a pending restart
    // puts its line in front
    inline void Help(const std::string& text)
    {
        state.helpLines = Lines(text);
        if (NeedsRestart())
            state.helpLines.insert(state.helpLines.begin(), state.model.Text("menu.restartwarning"));
        state.helpLine = 0;
        state.helpTime = GetTickCount64() + 3000;
        Line(state.helpLines.empty() ? std::string() : state.helpLines.front());
    }
    inline void NextHelpLine()
    {
        if (state.helpLines.size() < 2 || GetTickCount64() < state.helpTime)
            return;
        state.helpLine = (state.helpLine + 1) % state.helpLines.size();
        state.helpTime = GetTickCount64() + 3000;
        Line(state.helpLines[state.helpLine]);
    }
    inline std::string ItemHelp(const Item& item)
    {
        return state.page == SIZE_MAX && !notice.empty() ? notice : item.help;
    }
    inline bool Build()
    {
        Model();
        state.busy = true;
        ++state.generation;
        state.items.clear();
        state.focus = 0;
        auto heading = state.model.Text("menu.plugins");
        if (state.page == SIZE_MAX)
        {
            for (size_t i = 0; i < state.model.Pages().size(); ++i)
                state.items.push_back({ i, {}, state.model.Pages()[i].title });
        }
        else
        {
            if (state.page >= state.model.Pages().size())
            {
                state.busy = false;
                return false;
            }
            heading = state.model.Pages()[state.page].title;
            for (auto source : state.model.VisibleRows(state.page))
            {
                auto row = *source;
                state.items.push_back(
                    { SIZE_MAX, row, state.model.Label(row), state.model.Value(row), state.model.Help(row) });
            }
        }
        state.items.push_back({ SIZE_MAX, {}, state.model.Text("menu.back"), {}, {}, 0, true });
        Parameters menu;
        const auto titleText = NativeText(heading, true);
        menu.Text("title", titleText);
        menu.Number("title_scale", std::min(2.0f, 44.0f / std::max(1.0f, static_cast<float>(titleText.size()))));
        menu.XY("dims", 400, 300);
        menu.Flag("scrolling");
        if (!Run("make_new_themed_sub_menu", menu))
        {
            state.busy = false;
            return false;
        }
        AdoptRoot();
        state.open = state.root && Lookup("current_menu");
        state.display = false;
        if (!state.open)
        {
            state.busy = false;
            return false;
        }
        for (size_t i = 0; i < state.items.size(); ++i)
        {
            state.items[i].id = Checksum(("wfp_thaw_row_" + std::to_string(i)).c_str());
            if (!Row(state.items[i], Command::Choose))
            {
                state.busy = false;
                return false;
            }
        }
        state.selected = std::min(state.selected, state.items.size() - 1);
        Parameters finish;
        auto result = Spawn("finish_themed_sub_menu", finish);
        state.busy = false;
        RWGTAMenu::Model::Changed() = false;
        return result;
    }
    inline bool Refresh()
    {
        if (state.page == SIZE_MAX)
            return true;
        const auto rows = state.model.VisibleRows(state.page);
        if (rows.size() + 1 != state.items.size())
            return Build();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto& item = state.items[i];
            const auto& row = *rows[i];
            if (item.row.kind != row.kind || item.row.plugin != row.plugin || item.row.setting != row.setting)
                return Build();
            item.label = state.model.Label(row);
            item.value = state.model.Value(row);
            item.help = state.model.Help(row);
            for (bool value : { false, true })
            {
                Parameters text;
                text.Name("id", item.id ^ (value ? 0x2345BCDE : 0x1234ABCD));
                text.Text("text", NativeText(value ? item.value : item.label));
                Properties(text);
            }
        }
        RWGTAMenu::Model::Changed() = false;
        if (state.selected < state.items.size())
            Help(ItemHelp(state.items[state.selected]));
        return true;
    }
    inline void RefreshJoined()
    {
        const auto& rows = state.model.JoinedRows();
        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto id = Checksum(("wfp_thaw_joined_" + std::to_string(i)).c_str());
            for (bool value : { false, true })
            {
                Parameters text;
                text.Name("id", id ^ (value ? 0x2345BCDE : 0x1234ABCD));
                text.Text("text", NativeText(value ? state.model.Value(rows[i]) : state.model.Label(rows[i])));
                Properties(text);
            }
        }
    }
    inline void Close()
    {
        state.busy = true;
        if (state.returnWindow.element)
            api.unlink(state.returnWindow.element, &state.returnWindow);
        state.returnWindow = {};
        if (auto* manager = Read<void*>(NativeManager.get_ptr()))
        {
            Parameters anchor;
            anchor.Name("id", "root_window");
            api.lookup(manager, &state.returnWindow, &anchor.value, Checksum("id"), 0);
        }
        state.returnToEntry = state.returnWindow.element && Read<void*>(state.root, 96) == state.returnWindow.element;
        if (!state.returnToEntry && state.returnWindow.element)
        {
            api.unlink(state.returnWindow.element, &state.returnWindow);
            state.returnWindow = {};
        }
        state.returnPause = state.pause;
        Parameters parameters;
        Spawn(state.pause ? "create_options_menu" : "create_setup_options_menu", parameters);
        state.open = false;
        state.display = false;
        ReleaseRoot();
        state.items.clear();
        state.focus = 0;
        state.page = SIZE_MAX;
        ++state.generation;
        state.busy = false;
    }
    inline void Dispatch(const Input& input)
    {
        if (input.generation != state.generation || input.root != state.root || !Current())
            return;
        if (input.command == Command::Open)
        {
            auto* menu = Lookup("current_menu");
            if (Read<uint8_t>(menu, 396) != 1 || Read<uint32_t>(menu, 392) != Entry)
                return;
            state.page = SIZE_MAX;
            state.selected = 0;
            if (!Build())
                Close();
            return;
        }
        if (input.command == Command::Joined || input.command == Command::JoinedLeft ||
            input.command == Command::JoinedFocus)
        {
            if (Read<uint32_t>(Lookup("current_menu"), 392) !=
                Checksum(("wfp_thaw_joined_" + std::to_string(input.index)).c_str()))
                return;
            const auto& rows = state.model.JoinedRows();
            if (input.index < rows.size())
            {
                if (input.command == Command::JoinedFocus)
                {
                    Help(state.model.Help(rows[input.index]));
                    return;
                }
                state.model.Change(rows[input.index], input.command == Command::JoinedLeft ? -1 : 1, state.pause);
                RefreshJoined();
                Help(state.model.Help(rows[input.index]));
            }
            return;
        }
        if (!state.open)
            return;
        if (input.command == Command::Back)
        {
            if (state.page == SIZE_MAX)
            {
                Close();
                return;
            }
            state.page = SIZE_MAX;
            state.selected = 0;
            if (!Build())
                Close();
            return;
        }
        if (input.index >= state.items.size())
            return;
        auto item = state.items[input.index];
        if (Read<uint32_t>(Lookup("current_menu"), 392) != item.id)
            return;
        state.selected = input.index;
        if (input.command == Command::Focus)
        {
            Help(ItemHelp(item));
            return;
        }
        if (item.back)
        {
            Dispatch({ Command::Back, state.generation, 0, state.root });
            return;
        }
        if (item.page != SIZE_MAX)
        {
            if (input.command != Command::Choose)
                return;
            state.page = item.page;
            state.selected = 0;
            state.model.PageShown(item.page);
        }
        else if (state.model.Enabled(item.row, state.pause))
        {
            state.model.Change(item.row,
                               input.command == Command::Left    ? -1
                               : input.command == Command::Right ? 1
                                                                 : 0,
                               state.pause);
            if (!Refresh())
                Close();
            return;
        }
        if (!Build())
            Close();
    }
    inline void AddEntries(void* script, uint32_t callee)
    {
        if (state.busy || state.open || !state.registered)
            return;
        const auto caller = Read<uint32_t>(script, 208);
        const bool display = caller == Checksum("create_display_options_menu");
        if (display && callee == Checksum("theme_menu_add_item"))
        {
            auto parameters = Read<Structure*>(script, 24);
            if (!parameters)
                parameters = Read<Structure*>(script, 20);
            int id{};
            if (!Integer(parameters, "id", id) || static_cast<uint32_t>(id) != Checksum("menu_done"))
                return;
        }
        else if (callee != Checksum("finish_themed_sub_menu") && callee != Checksum("roundbar_menu_finish") &&
                 callee != Checksum("label_menu_finish"))
            return;
        auto options = caller == Checksum("create_options_menu") || caller == Checksum("create_setup_options_menu");
        if (!options && !display)
            return;
        AdoptRoot();
        if (!state.root || !Lookup("current_menu"))
            return;
        if (display && Lookup(Checksum("wfp_thaw_joined_0")))
            return;
        if (options)
            state.pause = caller == Checksum("create_options_menu");
        state.display = display;
        state.focus = 0;
        Model();
        state.busy = true;
        ++state.generation;
        if (options && !Lookup(Entry))
        {
            Row({ SIZE_MAX, {}, state.model.Text("menu.plugins"), {}, {}, Entry }, Command::Open);
        }
        if (display)
        {
            const auto& rows = state.model.JoinedRows();
            for (size_t i = 0; i < rows.size(); ++i)
            {
                auto id = Checksum(("wfp_thaw_joined_" + std::to_string(i)).c_str());
                if (!Lookup(id))
                    Row({ SIZE_MAX, rows[i], state.model.Label(rows[i]), state.model.Value(rows[i]),
                          state.model.Help(rows[i]), id },
                        Command::Joined);
            }
        }
        state.busy = false;
    }
    inline uintptr_t __fastcall ScriptCall(void* script, void*, uint32_t name, void* code, Structure* parameters,
                                           void* object, char flag)
    {
        AddEntries(script, name);
        return state.callHook.thiscall<uintptr_t>(script, name, code, parameters, object, flag);
    }
    inline void __cdecl Update()
    {
        ++state.depth;
        if (!state.registered && Script("make_new_themed_sub_menu") && Script("nullscript"))
        {
            state.registered = true;
        }
        state.updateHook.ccall<void>();
        --state.depth;
        if (state.depth || state.busy)
            return;
        auto pending = std::move(state.pending);
        state.pending.clear();
        for (const auto& input : pending)
            Dispatch(input);
        if (state.returnToEntry)
        {
            auto* currentWindow = Lookup("root_window");
            if (!state.returnWindow.element || (currentWindow && currentWindow != state.returnWindow.element))
                state.returnToEntry = false;
            else if (!state.open && !state.display && state.pause == state.returnPause && Current() &&
                     Read<void*>(state.root, 96) == state.returnWindow.element && Lookup(Entry))
            {
                auto* menu = Lookup("current_menu");
                if (Read<uint8_t>(menu, 396) == 1)
                {
                    state.returnToEntry = false;
                    if (Read<uint32_t>(menu, 392) != Entry)
                    {
                        const auto id = Read<uint32_t>(menu, 24);
                        Fire("unfocus", id);
                        Parameters focus;
                        focus.Name("child_id", Entry);
                        Fire("focus", id, &focus.value);
                    }
                }
            }
            if (!state.returnToEntry && state.returnWindow.element)
            {
                api.unlink(state.returnWindow.element, &state.returnWindow);
                state.returnWindow = {};
            }
        }
        if ((state.open || state.display) && Current())
        {
            auto* menu = Lookup("current_menu");
            auto selected = Read<uint32_t>(menu, 392);
            if (Read<uint8_t>(menu, 396) == 1 && selected != state.focus)
            {
                state.focus = selected;
                if (state.open)
                {
                    for (size_t i = 0; i < state.items.size(); ++i)
                        if (state.items[i].id == selected)
                            Dispatch({ Command::Focus, state.generation, i, state.root });
                }
                else
                {
                    for (size_t i = 0; i < state.model.JoinedRows().size(); ++i)
                        if (selected == Checksum(("wfp_thaw_joined_" + std::to_string(i)).c_str()))
                            Dispatch({ Command::JoinedFocus, state.generation, i, state.root });
                }
            }
        }
        if ((state.open || state.display) && Current())
            NextHelpLine();
        if (updateFrame)
            updateFrame();
        if (state.open && !Current())
        {
            state.open = false;
            ReleaseRoot();
            state.items.clear();
            state.focus = 0;
            ++state.generation;
        }
        if ((state.open || state.display) && Current() && RWGTAMenu::Model::Changed().exchange(false))
        {
            const auto language = state.language;
            Model();
            if (state.open)
            {
                if (language != state.language)
                {
                    state.page = SIZE_MAX;
                    state.selected = 0;
                    if (!Build())
                        Close();
                }
                else if (!Refresh())
                    Close();
            }
            else
                RefreshJoined();
        }
    }
    inline void Init()
    {
        Bind();
        state.callHook = safetyhook::create_inline(api.call, ScriptCall);
        state.updateHook = safetyhook::create_inline(api.update, Update);
        state.launchHook = safetyhook::create_inline(api.launch, Launch);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                restartBaseline.emplace(std::make_pair(state.module, setting.id), FixSettings().Get(setting.id));
        }
        state.ready = true;
    }
    inline std::string NoticeText(const char* key)
    {
        Model();
        return RWGTAMenu::PluginText(FixSettings(), state.language, key);
    }
    // The Plugins list (not a plugin's page) is on screen
    inline bool ListShown()
    {
        return state.open && state.page == SIZE_MAX && !state.busy && Current();
    }
    inline void ShowNotice(const std::string& text)
    {
        notice = text;
        if (ListShown())
            Help(notice);
    }
} // namespace THAWMenu

// MAIN.CheckForUpdates: the notice runs in the help line of the Plugins list
namespace THAWUpdates
{
    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        if (!THAWMenu::state.ready)
            return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TonyHawksAmericanWasteland.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return THAWMenu::ListShown(); };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(THAWMenu::NoticeText(hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text) { THAWMenu::ShowNotice(asi_settings::text::Narrow(text)); };
        RWGTA::UpdateCheck::Init(std::move(config));
        THAWMenu::updateFrame = []() { RWGTA::UpdateCheck::Tick(); };
    }
}
}
