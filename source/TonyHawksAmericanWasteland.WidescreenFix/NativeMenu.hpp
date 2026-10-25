#pragma once
#include "NativeMenuBindings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace THAWMenu
{
    constexpr auto Entry = Checksum("wfp_thaw_plugins");
    inline const std::set<std::string> joined{ "MAIN.FixHUD", "MAIN.HudAspectRatioConstraint" };
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
        bool registered{}, busy{}, open{}, pause{}, modelOpen{}, display{};
        std::string language;
    };
    inline State state;
    inline bool NeedsRestart()
    {
        const auto value = FixSettings().Wanted("MAIN.WindowMode");
        return state.model.RestartRequired() || value != std::to_string(FixSettings().Int("MAIN.WindowMode"));
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
        const auto codePage = Read<int>(api.language) == 9 ? 1251u : 1252u;
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
        auto* manager = Read<void*>(api.manager);
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
            switch (Read<int>(api.language))
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
        row.Flag("no_bg");
        if (item.page != SIZE_MAX || item.back)
            row.Flag("centered");
        row.Number("scale", 0.8f);
        auto helper = labelMenu ? "add_label_menu_item" : "theme_menu_add_item";
        if (!Run(helper, row) || !Lookup(item.id))
            return false;
        return true;
    }
    inline void Help(const std::string& text)
    {
        // The game's helper text factory owns the bottom information area.
        auto shortText = text.substr(0, 68);
        if (shortText.size() < text.size())
        {
            auto space = shortText.find_last_of(' ');
            if (space != std::string::npos)
                shortText.resize(space);
            shortText += "...";
        }
        Parameters line;
        line.Text("text", NativeText(shortText));
        ArrayView lines(10, { Parameters::Pointer(&line.value) });
        Parameters parameters;
        parameters.List("helper_text_elements", &lines.value);
        Run("create_helper_text", parameters);
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
            Help(NeedsRestart() ? state.model.Text("menu.restartwarning") : state.items[state.selected].help);
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
                Help(NeedsRestart() ? state.model.Text("menu.restartwarning") : state.model.Help(rows[input.index]));
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
            Help(item.help);
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
        if (!Bind())
        {
            Log("Native menu bindings unavailable");
            return;
        }
        state.callHook = safetyhook::create_inline(api.call, ScriptCall);
        state.updateHook = safetyhook::create_inline(api.update, Update);
        state.launchHook = safetyhook::create_inline(api.launch, Launch);
        if (!state.callHook || !state.updateHook || !state.launchHook)
        {
            state.callHook.reset();
            state.updateHook.reset();
            state.launchHook.reset();
            Log("Native menu hooks unavailable");
            return;
        }
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
    }
} // namespace THAWMenu
