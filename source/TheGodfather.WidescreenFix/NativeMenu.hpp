#pragma once
#include "NativeMenuABI.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <Xinput.h>

namespace GodfatherMenu
{
    enum class Kind
    {
        Plugin,
        Setting,
        Back,
        Previous,
        Next
    };
    struct Item
    {
        Kind kind{};
        size_t index{};
        RWGTAMenu::Row row{};
        std::string label, help;
    };
    struct State
    {
        HMODULE module{};
        RWGTAMenu::Model model;
        Value* root{};
        Value* pendingRoot{};
        std::vector<Item> items;
        std::vector<std::pair<Value*, float>> hidden;
        std::vector<std::pair<Value*, bool>> decorations;
        int page{ -1 };
        size_t start{};
        unsigned depth{};
        bool busy{}, open{}, refresh{};
        int command{ -1 };
        int direction{};
        std::string language, autoLanguage{ "en" };
        size_t selected{};
        uint32_t inputMask{};
        ULONGLONG settleAt{};
        SafetyHookInline gammaHook, actionsHook;
    };
    inline State state;

    template <class T> T Read(const void* base, size_t offset = 0)
    {
        T value{};
        if (!base)
            return value;
        auto address = reinterpret_cast<uintptr_t>(base) + offset;
        MEMORY_BASIC_INFORMATION memory{};
        if (VirtualQuery(reinterpret_cast<void*>(address), &memory, sizeof(memory)) && memory.State == MEM_COMMIT &&
            !(memory.Protect & (PAGE_NOACCESS | PAGE_GUARD)) &&
            address + sizeof(T) <= reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize)
            std::memcpy(&value, reinterpret_cast<void*>(address), sizeof(value));
        return value;
    }
    inline Value* OptionsRoot()
    {
        auto* screens = Read<void*>(api.screens);
        auto* options = Read<void*>(screens, 8);
        if (Read<int>(options, 32) != 1)
            return nullptr;
        auto* path = Read<void*>(options, 100);
        auto* text = Read<const char*>(path);
        std::string name;
        for (size_t i = 0; text && i < 512; ++i)
        {
            auto character = Read<char>(text, i);
            if (!character)
                break;
            name += character;
        }
        if (name.empty() || name.size() == 512)
            return nullptr;
        return Get(api.levelRoot(0), name.c_str());
    }
    inline Value* VideoRoot()
    {
        auto options = OptionsRoot();
        auto gamma = options ? Get(options, "gamma") : nullptr;
        return gamma ? Get(gamma, "_parent") : nullptr;
    }

    inline bool NeedsRestart()
    {
        const auto value = FixSettings().Wanted("MAIN.PreferWidescreenResolutions");
        return state.model.RestartRequired() || (value == "1" || asi_settings::text::IEquals(value, "true")) !=
                                                    FixSettings().Bool("MAIN.PreferWidescreenResolutions");
    }
    // Failures only, to the debugger
    inline void Log(const char* message)
    {
        OutputDebugStringA(("TheGodfather.WidescreenFix menu: " + std::string(message) + "\n").c_str());
    }
    inline Value* Owner(Value* widget)
    {
        if (state.root && widget && Get(widget, "_parent") != state.root)
            widget = Get(widget, "_parent");
        return state.root && widget && Get(widget, "_parent") == state.root ? widget : nullptr;
    }
    inline bool Owned(Value* widget)
    {
        return widget && Owner(widget) == widget;
    }
    inline void Handler(Value* widget, const char* event, Value*(__cdecl* callback)(Value*, int))
    {
        Set(widget, event, api.function(reinterpret_cast<void*>(callback)));
    }
    inline void FitButton(Value* widget, float width);
    inline void Frame(Value* widget, const char* frame)
    {
        if (!widget || !Get(widget, "overlay"))
            return;
        auto label = api.text(frame);
        Execute(widget, "gotoAndStop", { label });
        if (label)
            api.releaseRoot(label);
        const auto width = Number(widget, "wfpWidth");
        if (width > 0.0f)
            FitButton(widget, width);
    }
    inline Value* HelpField()
    {
        auto help = Get(state.root, "helpText");
        if (help && Number(help, "_width") > 0.0f)
            return help;
        help = Get(state.root, "wfpHelp");
        if (!help)
        {
            auto name = api.text("wfpHelp");
            Execute(state.root, "createTextField",
                    { name, api.number(28000.0f), api.number(45.0f), api.number(315.0f),
                      api.number(350.0f), api.number(65.0f) });
            if (name)
                api.releaseRoot(name);
            help = Get(state.root, "wfpHelp");
            Set(help, "wordWrap", 1.0f);
            Set(help, "multiline", 1.0f);
            Set(help, "selectable", 0.0f);
            auto format = Call(Get(Get(state.root, "test"), "labelContent"), "getTextFormat");
            Set(format, "align", std::string("left"));
            Execute(help, "setTextFormat", { format });
            Reference(format, false);
            Set(state.root, "helpText", help);
        }
        return help;
    }
    inline void ShowHelp(const std::string& text)
    {
        auto help = HelpField();
        Set(help, "_x", state.open ? 70.0f : 45.0f);
        Set(help, "_y", state.open ? 445.0f : 315.0f);
        Set(help, "_width", state.open ? 650.0f : 350.0f);
        Set(help, "_height", state.open ? 90.0f : 65.0f);
        Set(help, "_visible", text.empty() ? 0.0f : 1.0f);
        Set(help, "text", text);
    }
    inline Value* __cdecl Choose(Value* receiver, int)
    {
        if (auto widget = Owner(receiver); !state.busy && widget)
        {
            state.command = static_cast<int>(Number(widget, "wfpIndex"));
            state.direction = static_cast<int>(Number(widget, "wfpDirection"));
        }
        return api.undefined();
    }
    inline Value* __cdecl Help(Value* receiver, int)
    {
        if (auto widget = Owner(receiver))
        {
            Frame(widget, "rollover");
            const auto index = static_cast<int>(Number(widget, "wfpIndex"));
            if (state.open && index >= 0 && static_cast<size_t>(index) < state.items.size())
            {
                state.selected = index;
                for (size_t i = 0; i < state.items.size(); ++i)
                    if (i != state.selected)
                        Frame(Get(state.root, ("wfpRow" + std::to_string(i)).c_str()), "normal");
                ShowHelp(state.items[index].help);
            }
            else
            {
                auto help = HelpField();
                Set(help, "text", Get(widget, "help"));
                Set(help, "_visible", 1.0f);
            }
        }
        return api.undefined();
    }
    inline Value* __cdecl Blur(Value* receiver, int)
    {
        if (auto widget = Owner(receiver))
        {
            const auto selected = state.open && Number(widget, "wfpIndex") == static_cast<float>(state.selected);
            Frame(widget, selected ? "rollover" : "normal");
            if (!state.open)
                ShowHelp({});
        }
        return api.undefined();
    }
    inline Value* __cdecl Defaults(Value* widget, int)
    {
        if (Owned(widget))
        {
            Reference(Call(widget, Get(widget, "wfpDefaults")), false);
            RWGTAMenu::RestoreDefaults(FixSettings(), { "MAIN.PreferWidescreenResolutions" });
            state.refresh = true;
        }
        return api.undefined();
    }
    inline Value* __cdecl ModesChanged(Value* widget, int count)
    {
        if (!state.busy && Owned(widget))
        {
            const bool wanted = Number(widget, "checkBoxValue") != 0.0f;
            auto current = FixSettings().Wanted("MAIN.PreferWidescreenResolutions");
            if (wanted != (current == "1" || asi_settings::text::IEquals(current, "true")))
            {
                for (const auto& row : state.model.JoinedRows())
                    if (row.setting && row.setting->id == "MAIN.PreferWidescreenResolutions")
                        state.model.Change(row, 1, false);
                state.refresh = true;
            }
        }
        return api.undefined();
    }
    inline Value* Clone(Value* source, const std::string& name, float depth)
    {
        if (!source || !state.root || Get(state.root, name.c_str()))
            return nullptr;
        auto clipName = api.text(name.c_str());
        auto result = Call(source, "duplicateMovieClip", { clipName, api.number(depth) });
        if (clipName)
            api.releaseRoot(clipName);
        Reference(result, false);
        state.settleAt = GetTickCount64() + 100;
        return Get(state.root, name.c_str());
    }
    inline void Label(Value* widget, const std::string& label, const std::string& help)
    {
        Set(widget, "buttonEnabled", 1.0f);
        Set(widget, "isEnabled", 1.0f);
        auto count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, label.data(), static_cast<int>(label.size()),
                                         nullptr, 0);
        std::wstring wide(count > 0 ? count : 0, L'\0');
        if (count > 0)
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, label.data(), static_cast<int>(label.size()),
                                wide.data(), count);
        std::wstring locale(state.language.begin(), state.language.end());
        auto mapped = count > 0 ? LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), count, nullptr, 0,
                                                nullptr, nullptr, 0)
                                : 0;
        std::wstring upper(mapped > 0 ? mapped : 0, L'\0');
        if (mapped > 0)
            LCMapStringEx(locale.c_str(), LCMAP_UPPERCASE, wide.data(), count, upper.data(), mapped, nullptr, nullptr,
                          0);
        count = mapped > 0 ? WideCharToMultiByte(CP_UTF8, 0, upper.data(), mapped, nullptr, 0, nullptr, nullptr) : 0;
        std::string text = label;
        if (count > 0)
        {
            text.resize(count);
            WideCharToMultiByte(CP_UTF8, 0, upper.data(), mapped, text.data(), count, nullptr, nullptr);
        }
        Set(widget, "buttonText", text);
        Set(widget, "help", help);
        Set(Get(widget, "labelContent"), "text", text);
        Set(Get(widget, "labelText"), "text", text);
        Handler(widget, "OnButtonPressed", Choose);
        auto hit = Get(widget, "overlay");
        if (!hit)
            hit = Get(widget, "innerSquare");
        // Mouse events belong to the native hit clip inside the component.
        Handler(hit, "onRelease", Choose);
        Handler(hit, "onRollOver", Help);
        Handler(hit, "onRollOut", Blur);
        Set(hit, "_visible", 1.0f);
    }
    inline void FitButton(Value* widget, float width)
    {
        const auto original = Number(Get(state.root, "test"), "_width");
        if (!widget || original <= 0.0f)
            return;
        Set(widget, "wfpWidth", width);
        const auto factor = width / original;
        auto label = Get(widget, "labelContent");
        Set(label, "_xscale", 100.0f);
        Set(label, "_width", width);
        Set(widget, "_xscale", factor * 100.0f);
        Set(label, "_xscale", 100.0f / factor);
        Set(label, "_x", 7.0f / factor);
    }
    inline void Focus(Value* widget, bool selected)
    {
        Frame(widget, selected ? "rollover" : "normal");
    }
    inline void HideRows()
    {
        for (int i = 0; i < 9; ++i)
        {
            auto name = "wfpRow" + std::to_string(i);
            if (auto row = Get(state.root, name.c_str()))
                Set(row, "_visible", 0.0f);
            for (auto prefix : { "wfpLeft", "wfpRight" })
            {
                name = prefix + std::to_string(i);
                if (auto row = Get(state.root, name.c_str()))
                    Set(row, "_visible", 0.0f);
            }
        }
    }
    inline Value* AtDepth(Value* root, int depth)
    {
        auto sprite = Read<void*>(root, 72);
        auto list = Read<void*>(sprite, 36);
        auto sentinel = Read<void*>(list);
        auto child = Read<Value*>(sentinel, 80);
        for (unsigned i = 0; child && i < 512; ++i)
        {
            const auto childDepth = static_cast<int32_t>(Read<uint32_t>(child, 84) << 15) >> 15;
            if (childDepth == depth && Read<Value*>(child, 68) == root)
                return child;
            auto next = Read<Value*>(child, 80);
            if (next == child)
                break;
            child = next;
        }
        return nullptr;
    }
    inline void DecorationVisible(Value* widget, bool visible)
    {
        if (Read<Value*>(widget, 68) != state.root)
            return;
        auto flags = Read<uint32_t>(widget, 88);
        *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(widget) + 88) = (flags & ~0x100000u) | (visible ? 0x100000u : 0u);
    }
    inline void Stock(bool visible)
    {
        if (visible)
        {
            for (auto [widget, value] : state.decorations)
                DecorationVisible(widget, value);
            state.decorations.clear();
            for (auto [widget, value] : state.hidden)
            {
                Set(widget, "_visible", value);
                Reference(widget, false);
            }
            state.hidden.clear();
            return;
        }
        // These unnamed movie clips are the stock warning and its asterisks.
        for (auto depth : { 63, 65, 67, 69, 71, 73, 74 })
        {
            if (auto widget = AtDepth(state.root, depth))
            {
                state.decorations.emplace_back(widget, (Read<uint32_t>(widget, 88) & 0x100000) != 0);
                DecorationVisible(widget, false);
            }
        }
        for (auto name : { "videoRes", "reflections",       "ani",        "antialiasing", "distortion", "textures",
                           "bloom",    "shadows",           "filmEffect", "bright",       "contrast",   "gamma",
                           "language", "subtitles",         "videoTab",   "controlsTab",  "test",       "cancel",
                           "ok", "okButton", "gcbDefaultsButton", "wfpPlugins", "wfpModes" })
        {
            auto widget = Get(state.root, name);
            if (!widget)
                continue;
            state.hidden.emplace_back(widget, Number(widget, "_visible"));
            Reference(widget, true);
            Set(widget, "_visible", 0.0f);
        }
    }
    inline void Build()
    {
        state.items.clear();
        HideRows();
        const auto& pages = state.model.Pages();
        if (state.page < 0)
        {
            for (size_t i = state.start; i < std::min(state.start + 5, pages.size()); ++i)
                state.items.push_back({ Kind::Plugin, i, {}, pages[i].title, {} });
            if (state.start)
                state.items.push_back({ Kind::Previous,
                                        state.start >= 5 ? state.start - 5 : 0,
                                        {},
                                        state.model.Text("menu.previous"),
                                        {} });
            if (state.start + 5 < pages.size())
                state.items.push_back({ Kind::Next, state.start + 5, {}, state.model.Text("menu.next"), {} });
        }
        else if (static_cast<size_t>(state.page) < pages.size())
        {
            for (auto source : state.model.VisibleRows(state.page))
            {
                auto row = *source;
                auto text = state.model.Label(row);
                auto value = state.model.Value(row);
                if (!value.empty())
                    text += ": " + value;
                state.items.push_back({ Kind::Setting, 0, row, std::move(text), state.model.Help(row) });
            }
        }
        state.items.push_back({ Kind::Back, 0, {}, state.model.Text("menu.back"), {} });
        const auto count = std::min(size_t(8), state.items.size());
        state.selected = count ? std::min(state.selected, count - 1) : 0;
        for (size_t i = 0; i < count; ++i)
        {
            const auto index = i;
            auto name = "wfpRow" + std::to_string(i);
            auto widget = Get(state.root, name.c_str());
            if (!widget)
                widget = Clone(Get(state.root, "test"), name, 24000.0f + static_cast<float>(i));
            if (!widget)
            {
                Log("Native option row creation failed");
                continue;
            }
            Set(widget, "_x", 70.0f);
            Set(widget, "_y", 110.0f + 36.0f * static_cast<float>(i));
            Set(widget, "_visible", 1.0f);
            Set(widget, "wfpIndex", static_cast<float>(index));
            Set(widget, "wfpDirection", 0.0f);
            Label(widget, state.items[index].label, state.items[index].help);
            FitButton(widget, 400.0f);
            Focus(widget, i == state.selected);
            const auto& item = state.items[index];
            if (item.kind == Kind::Setting &&
                (item.row.kind == RWGTAMenu::RowKind::Setting || item.row.kind == RWGTAMenu::RowKind::Preset))
            {
                for (int direction : { -1, 1 })
                {
                    auto arrowName = std::string(direction < 0 ? "wfpLeft" : "wfpRight") + std::to_string(i);
                    auto arrow = Get(state.root, arrowName.c_str());
                    if (!arrow)
                        arrow = Clone(Get(state.root, "test"), arrowName,
                                      26000.0f + static_cast<float>(2 * i + (direction > 0)));
                    if (!arrow)
                        continue;
                    Set(arrow, "_x", direction < 0 ? 470.0f : 520.0f);
                    Set(arrow, "_y", 110.0f + 36.0f * static_cast<float>(i));
                    Set(arrow, "_visible", 1.0f);
                    Set(arrow, "wfpIndex", static_cast<float>(index));
                    Set(arrow, "wfpDirection", static_cast<float>(direction));
                    Label(arrow, direction < 0 ? "<" : ">", item.help);
                    FitButton(arrow, 35.0f);
                }
            }
        }
        ShowHelp(
            NeedsRestart() ? state.model.Text("menu.restartwarning")
            : count        ? state.items[state.selected].help
                           : std::string{});
        RWGTAMenu::Model::Changed() = false;
    }
    inline void Close()
    {
        HideRows();
        Stock(true);
        state.items.clear();
        state.open = false;
        state.page = -1;
        state.start = 0;
        ShowHelp({});
    }
    inline void Act()
    {
        const auto command = std::exchange(state.command, -1);
        if (command == -1)
            return;
        if (command == -2)
        {
            if (state.open)
                return;
            state.open = true;
            state.selected = 0;
            Stock(false);
            Build();
            return;
        }
        if (command == -4)
        {
            const auto& rows = state.model.JoinedRows();
            for (const auto& row : rows)
                state.model.Change(row, 1, false);
            state.refresh = true;
            return;
        }
        if (!state.open || command < 0 || static_cast<size_t>(command) >= state.items.size())
            return;
        const auto item = state.items[command];
        if (item.kind == Kind::Previous || item.kind == Kind::Next)
        {
            state.start = item.index;
            state.selected = 0;
            Build();
            return;
        }
        if (item.kind == Kind::Back)
        {
            if (state.page < 0)
            {
                Close();
                return;
            }
            state.page = -1;
            state.selected = 0;
        }
        else if (item.kind == Kind::Plugin)
        {
            state.page = static_cast<int>(item.index);
            state.selected = 0;
            state.model.PageShown(item.index);
        }
        else if (state.model.Enabled(item.row, false))
            state.model.Change(item.row, state.direction, false);
        state.start = 0;
        Build();
    }
    inline void Joined()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
            language = state.autoLanguage;
        if (language != state.language)
        {
            state.language = language;
            state.items.clear();
            state.page = -1;
            state.start = state.selected = 0;
            state.model.OpenPlugins(state.module, state.language, 6, { "MAIN.PreferWidescreenResolutions" });
        }
        Label(Get(state.root, "wfpPlugins"), state.model.Text("menu.plugins"), {});
        for (const auto& row : state.model.JoinedRows())
        {
            if (!row.setting)
                continue;
            auto widget = Get(state.root, "wfpModes");
            Label(widget, state.model.Label(row), state.model.Help(row));
            Set(widget, "_x", 350.0f);
            Set(widget, "_y", 199.0f);
            Set(Get(widget, "labelContent"), "_x", -305.0f);
            Set(Get(widget, "labelContent"), "_width", 295.0f);
            Handler(widget, "OnValueChanged", ModesChanged);
            auto value = FixSettings().Wanted("MAIN.PreferWidescreenResolutions");
            const auto checked = value == "1" || asi_settings::text::IEquals(value, "true") ? 1.0f : 0.0f;
            Set(widget, "checkBoxValue", checked);
            Set(widget, "isVisible", checked);
            Execute(widget, "SetChecked", { api.number(checked) });
            Set(Get(widget, "dot"), "_visible", checked);
            Set(Get(widget, "innerDot"), "_visible", 0.0f);
        }
    }
    inline void Input()
    {
        DWORD foreground{};
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (foreground != GetCurrentProcessId() || !state.root || !Number(state.root, "_visible"))
        {
            state.inputMask = 0;
            return;
        }
        uint32_t mask{};
        const int keys[]{ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN, VK_ESCAPE, VK_F6 };
        for (size_t i = 0; i < std::size(keys); ++i)
            if (GetAsyncKeyState(keys[i]) < 0)
                mask |= 1u << i;
        static auto getState = []()
        {
            auto module = LoadLibraryExW(L"xinput1_4.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            return module ? reinterpret_cast<DWORD(WINAPI*)(DWORD, XINPUT_STATE*)>(
                                GetProcAddress(module, "XInputGetState"))
                          : nullptr;
        }();
        if (getState)
        {
            for (DWORD i = 0; i < XUSER_MAX_COUNT; ++i)
            {
                XINPUT_STATE input{};
                if (getState(i, &input) != ERROR_SUCCESS)
                    continue;
                const WORD buttons[]{ XINPUT_GAMEPAD_DPAD_UP,    XINPUT_GAMEPAD_DPAD_DOWN, XINPUT_GAMEPAD_DPAD_LEFT,
                                      XINPUT_GAMEPAD_DPAD_RIGHT, XINPUT_GAMEPAD_A,         XINPUT_GAMEPAD_B,
                                      XINPUT_GAMEPAD_Y };
                for (size_t button = 0; button < std::size(buttons); ++button)
                    if (input.Gamepad.wButtons & buttons[button])
                        mask |= 1u << button;
                break;
            }
        }
        const auto pressed = mask & ~std::exchange(state.inputMask, mask);
        if (!state.open)
        {
            if (pressed & (1u << 6))
            {
                state.command = -2;
                state.direction = 0;
            }
            return;
        }
        if (state.items.empty())
            return;
        const auto previous = state.selected;
        state.selected = std::min(state.selected, state.items.size() - 1);
        if (pressed & 1)
            state.selected = (state.selected + state.items.size() - 1) % state.items.size();
        if (pressed & 2)
            state.selected = (state.selected + 1) % state.items.size();
        if (pressed & (1u << 5))
        {
            for (size_t i = 0; i < state.items.size(); ++i)
                if (state.items[i].kind == Kind::Back)
                    state.command = static_cast<int>(i);
            state.direction = 0;
        }
        else if (pressed & ((1u << 2) | (1u << 3) | (1u << 4)))
        {
            state.command = static_cast<int>(state.selected);
            state.direction = pressed & (1u << 2) ? -1 : pressed & (1u << 3) ? 1 : 0;
        }
        if (previous != state.selected)
        {
            for (size_t i = 0; i < state.items.size(); ++i)
            {
                auto name = "wfpRow" + std::to_string(i);
                Focus(Get(state.root, name.c_str()), i == state.selected);
            }
            ShowHelp(state.items[state.selected].help);
        }
    }
    inline void Install(Value* root)
    {
        if (state.root == root)
            return;
        if (state.root)
        {
            for (auto [widget, value] : state.hidden)
                Reference(widget, false);
            state.hidden.clear();
            state.decorations.clear();
            state.settleAt = 0;
            state.items.clear();
            state.open = false;
            state.command = -1;
            state.model.Close();
            Reference(state.root, false);
        }
        state.root = root;
        Reference(root, true);
        auto screen = Get(root, "_global.screen");
        if (auto language = Call(screen, "GetLanguageCode"))
        {
            if (api.isString(language))
            {
                auto string = static_cast<uint8_t*>(api.asString(language));
                auto text = string ? api.stringText(string + 8) : nullptr;
                if (text && std::strlen(text) >= 2)
                    state.autoLanguage.assign(text, 2);
            }
            Reference(language, false);
        }
        state.language = FixSettings().String("MAIN.TextLanguage");
        if (state.language.empty() || state.language == "auto")
            state.language = state.autoLanguage;
        state.model.OpenPlugins(state.module, state.language, 6, { "MAIN.PreferWidescreenResolutions" });
        struct Added
        {
            const char* name;
            const char* neighbor;
            int command;
            float gap;
        };
        for (auto entry : { Added{ "wfpPlugins", "test", -2, 32.0f }, Added{ "wfpModes", "videoRes", -4, 32.0f } })
        {
            auto source = Get(root, entry.command == -4 ? "subtitles" : "test");
            auto neighbor = Get(root, entry.neighbor);
            auto widget = Clone(source, entry.name, static_cast<float>(25000 - entry.command));
            if (!widget || !neighbor)
            {
                Log("Native option entry creation failed");
                continue;
            }
            Set(widget, "_x", Number(neighbor, "_x"));
            Set(widget, "_y", Number(neighbor, "_y") + entry.gap);
            Set(widget, "wfpIndex", static_cast<float>(entry.command));
            Label(widget, state.model.Text("menu.plugins"), {});
        }
        for (auto name : { "ani", "antialiasing" })
        {
            auto widget = Get(root, name);
            if (!Get(widget, "wfpOriginalY"))
                Set(widget, "wfpOriginalY", Number(widget, "_y"));
            Set(widget, "_y", Number(widget, "wfpOriginalY") + 36.0f);
        }
        for (auto depth : { 65, 67 })
        {
            if (auto widget = AtDepth(root, depth))
            {
                const auto key = "wfpDecoration" + std::to_string(depth) + "Y";
                if (!Get(root, key.c_str()))
                    Set(root, key.c_str(), Read<float>(widget, 32));
                api.transform(widget, 1, Number(root, key.c_str()) + 36.0f, 1);
            }
        }
        Joined();
        ShowHelp({});
        if (auto defaults = Get(root, "gcbDefaultsButton"))
        {
            Set(defaults, "wfpDefaults", Get(defaults, "OnButtonPressed"));
            Handler(defaults, "OnButtonPressed", Defaults);
        }
    }
    inline Value* __cdecl Gamma(Value* receiver, int count)
    {
        auto result = state.gammaHook.ccall<Value*>(receiver, count);
        if (!state.busy)
        {
            auto root = VideoRoot();
            if (!root)
            {
                static bool reported{};
                if (!reported)
                {
                    Log("Native video page has no gamma widget");
                    reported = true;
                }
            }
            if (root && root != state.root)
            {
                Reference(state.pendingRoot, false);
                state.pendingRoot = root;
                Reference(root, true);
            }
        }
        return result;
    }
    inline int __fastcall Actions(void* interpreter, void*, void* code, Value* receiver, int scope, int flags)
    {
        ++state.depth;
        const auto result = state.actionsHook.thiscall<int>(interpreter, code, receiver, scope, flags);
        --state.depth;
        if (state.depth || state.busy)
            return result;
        state.busy = true;
        auto video = VideoRoot();
        if (!state.pendingRoot && video && video != state.root)
        {
            state.pendingRoot = video;
            Reference(video, true);
        }
        if (auto root = std::exchange(state.pendingRoot, nullptr))
        {
            Install(root);
            Reference(root, false);
        }
        if (state.root)
        {
            if (video != state.root)
            {
                for (auto [widget, value] : state.hidden)
                    Reference(widget, false);
                state.hidden.clear();
                state.decorations.clear();
                state.settleAt = 0;
                state.items.clear();
                state.model.Close();
                Reference(state.root, false);
                state.root = nullptr;
                state.open = false;
                state.command = -1;
            }
            else
            {
                Input();
                Act();
                const auto settled = state.settleAt && GetTickCount64() >= state.settleAt;
                if (settled)
                    state.settleAt = 0;
                if (settled || state.refresh || RWGTAMenu::Model::Changed().exchange(false))
                {
                    state.refresh = false;
                    Joined();
                    if (state.open)
                        Build();
                }
            }
        }
        state.busy = false;
        return result;
    }
    inline void Init()
    {
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&Init), &state.module) ||
            !Bind())
        {
            Log("Native menu bindings unavailable");
            return;
        }
        auto gamma = find_pattern("A1 ? ? ? ? D9 05 ? ? ? ? 56 8B 70 08 85 F6 74 29 8B 86 A4 00 00 00");
        auto actions = find_pattern("8B 44 24 0C 83 EC 1C 83 F8 FF 53 55 56 57 8B 7C 24 34 8B F1");
        if (!gamma || !actions)
        {
            Log("Native menu entry points unavailable");
            return;
        }
        state.gammaHook = safetyhook::create_inline(gamma, Gamma);
        state.actionsHook = safetyhook::create_inline(actions, Actions);
        if (!state.gammaHook || !state.actionsHook)
        {
            state.gammaHook.reset();
            state.actionsHook.reset();
            Log("Native menu hook installation failed");
            return;
        }
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
    }
} // namespace GodfatherMenu
