#pragma once
#include "NativeMenuBindings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <fstream>

namespace GunMenu
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
        bool busy{}, open{}, display{}, pause{}, enabled{}, modelOpen{}, details{};
        size_t helpSlice{};
        std::string language, help;
    };
    inline State& Get() { static auto* value = new State; return *value; }
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
        state.details = state.modelOpen = false;
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
            reinterpret_cast<char(__thiscall*)(void*)>(0x4F4D70)(menu);
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
        menu.Text("subtitle", NativeText(state.model.Text("menu.details")));
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
        Parameters spacing;
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
        try { Tick(); }
        catch (...)
        {
            state.enabled = false;
            Log("Native menu unavailable; original menu retained");
        }
    }
    inline void Init()
    {
        if (!SupportedImage()) { Log("Native menu unavailable for this executable"); return; }
        auto& state = Get();
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                          reinterpret_cast<LPCWSTR>(&Init), &state.module);
        state.launchHook = safetyhook::create_inline(reinterpret_cast<void*>(0x43E050), Launch);
        state.updateHook = safetyhook::create_inline(reinterpret_cast<void*>(0x41CE30), Update);
        state.enabled = state.launchHook && state.updateHook;
        if (!state.enabled)
        {
            state.launchHook.reset();
            state.updateHook.reset();
            Log("Native menu hooks unavailable");
        }
    }
} // namespace GunMenu
