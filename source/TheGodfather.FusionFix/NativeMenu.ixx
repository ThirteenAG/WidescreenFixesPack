module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <map>
#include <stdexcept>

export module NativeMenu;

import NativeMenuABI;
import NativeMenuView;
import ResolutionPicker;

import Input;
import ComVars;

export namespace GodfatherMenu
{
    enum class Kind
    {
        Plugin,
        Setting,
        Back,
        Previous,
        Next,
        Details,
        Text
    };
    struct Item
    {
        Kind kind{};
        size_t index{};
        RWGTAMenu::Row row{};
        std::string label, help;
        bool enabled{ true };
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
        View view;
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
    State state;
    std::map<std::string, std::string> restartValues;
    std::map<std::pair<HMODULE, std::string>, std::string> pluginRestartValues;
    std::function<void()> tick;

    template <class T> T Read(const void* base, size_t offset = 0)
    {
        // Screens and values the game hasn't created yet are null
        return base ? *reinterpret_cast<const T*>(reinterpret_cast<uintptr_t>(base) + offset) : T{};
    }
    Value* OptionsRoot()
    {
        auto* screens = Read<void*>(GodfatherMenuScreens.is_initialized() ? GodfatherMenuScreens.get_ptr() : nullptr);
        auto* options = Read<void*>(screens, 8);
        if (Read<int>(options, 32 + layoutShift) != 1)
            return nullptr;
        auto* path = Read<void*>(options, 100 + layoutShift);
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
    Value* VideoRoot()
    {
        auto options = OptionsRoot();
        auto gamma = options ? Get(options, "gamma") : nullptr;
        return gamma ? Get(gamma, "_parent") : nullptr;
    }

    void RememberRestartValues()
    {
        for (const auto& page : state.model.Pages())
        {
            const auto plugin = page.plugin;
            if (!plugin || plugin->module == state.module)
                continue;
            for (const auto& setting : plugin->file.description.settings)
                if (setting.HasValue() && setting.apply == "restart")
                {
                    const auto key = std::pair{ plugin->module, setting.id };
                    if (!pluginRestartValues.contains(key))
                        pluginRestartValues.emplace(key, OtherPlugins::Get(*plugin, setting));
                }
        }
    }
    bool NeedsRestart()
    {
        for (const auto& [id, value] : restartValues)
            if (FixSettings().Wanted(id) != value)
                return true;
        for (const auto& page : state.model.Pages())
        {
            const auto plugin = page.plugin;
            if (!plugin || plugin->module == state.module)
                continue;
            for (const auto& setting : plugin->file.description.settings)
                if (const auto initial = pluginRestartValues.find({ plugin->module, setting.id });
                    initial != pluginRestartValues.end() && OtherPlugins::Get(*plugin, setting) != initial->second)
                    return true;
        }
        return false;
    }
    // Failures only, to the debugger
    void Log(const char* message)
    {
        OutputDebugStringA(("TheGodfather.FusionFix menu: " + std::string(message) + "\n").c_str());
    }
    Value* Owner(Value* widget)
    {
        if (state.root && widget && Get(widget, "_parent") != state.root)
            widget = Get(widget, "_parent");
        return state.root && widget && Get(widget, "_parent") == state.root ? widget : nullptr;
    }
    bool Owned(Value* widget)
    {
        return widget && Owner(widget) == widget;
    }
    void Handler(Value* widget, const char* event, Value*(__cdecl* callback)(Value*, int))
    {
        Set(widget, event, api.function(reinterpret_cast<void*>(callback)));
    }
    void FitButton(Value* widget, float width);
    void Frame(Value* widget, const char* frame)
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
    Value* HelpField()
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
    void ShowHelp(const std::string& text)
    {
        auto help = HelpField();
        Set(help, "_x", state.open ? 70.0f : 45.0f);
        Set(help, "_y", state.open ? 445.0f : 315.0f);
        Set(help, "_width", state.open ? 650.0f : 350.0f);
        Set(help, "_height", state.open ? 90.0f : 65.0f);
        Set(help, "_visible", text.empty() ? 0.0f : 1.0f);
        Set(help, "text", text);
    }
    Value* TextField(const std::string& name, float depth, float x, float y, float width, float height)
    {
        auto field = Get(state.root, name.c_str());
        if (!field)
        {
            auto label = api.text(name.c_str());
            Execute(state.root, "createTextField", { label, api.number(depth), api.number(x), api.number(y),
                api.number(width), api.number(height) });
            if (label)
                api.releaseRoot(label);
            field = Get(state.root, name.c_str());
            Set(field, "selectable", 0.0f);
            Set(field, "wordWrap", 1.0f);
            Set(field, "multiline", 1.0f);
            auto format = Call(Get(Get(state.root, "test"), "labelContent"), "getTextFormat");
            if (format)
            {
                Set(format, "align", std::string("left"));
                Execute(field, "setNewTextFormat", { format });
                Execute(field, "setTextFormat", { format });
                Reference(format, false);
            }
        }
        Set(field, "_x", x);
        Set(field, "_y", y);
        Set(field, "_width", width);
        Set(field, "_height", height);
        return field;
    }
    void ShowRestart()
    {
        auto field = TextField("wfpRestart", 29000.0f, 70.0f, 540.0f, 650.0f, 35.0f);
        Set(field, "text", state.model.Text("menu.restartshort"));
        Set(field, "_visible", state.open && NeedsRestart() ? 1.0f : 0.0f);
    }
    Value* __cdecl Choose(Value* receiver, int)
    {
        if (auto widget = Owner(receiver); !state.busy && widget)
        {
            state.command = static_cast<int>(Number(widget, "wfpIndex"));
            state.direction = static_cast<int>(Number(widget, "wfpDirection"));
        }
        return api.undefined();
    }
    Value* __cdecl Help(Value* receiver, int)
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
    Value* __cdecl Blur(Value* receiver, int)
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
    Value* Clone(Value* source, const std::string& name, float depth)
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
    void Label(Value* widget, const std::string& label, const std::string& help)
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
    void FitButton(Value* widget, float width)
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
    void Focus(Value* widget, bool selected)
    {
        Frame(widget, selected ? "rollover" : "normal");
    }
    void HideRows()
    {
        for (int i = 0; i < 9; ++i)
        {
            auto name = "wfpRow" + std::to_string(i);
            if (auto row = Get(state.root, name.c_str()))
                Set(row, "_visible", 0.0f);
            if (auto text = Get(state.root, ("wfpText" + std::to_string(i)).c_str()))
                Set(text, "_visible", 0.0f);
            for (auto prefix : { "wfpLeft", "wfpRight" })
            {
                name = prefix + std::to_string(i);
                if (auto row = Get(state.root, name.c_str()))
                    Set(row, "_visible", 0.0f);
            }
        }
    }
    Value* AtDepth(Value* root, int depth)
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
    void DecorationVisible(Value* widget, bool visible)
    {
        if (Read<Value*>(widget, 68) != state.root)
            return;
        auto flags = Read<uint32_t>(widget, 88);
        *reinterpret_cast<uint32_t*>(reinterpret_cast<uintptr_t>(widget) + 88) = (flags & ~0x100000u) | (visible ? 0x100000u : 0u);
    }
    void Stock(bool visible)
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
                           "ok", "okButton", "gcbDefaultsButton", "staticBackground", "wfpPlugins" })
        {
            auto widget = Get(state.root, name);
            if (!widget)
                continue;
            state.hidden.emplace_back(widget, Number(widget, "_visible"));
            Reference(widget, true);
            Set(widget, "_visible", 0.0f);
        }
    }
    void Close();
    void Build()
    {
        state.items.clear();
        HideRows();
        state.view.selected = state.selected;
        if (!state.view.Build(state.model, false, 8))
        {
            Close();
            return;
        }
        state.selected = state.view.selected;
        for (const auto& shown : state.view.items)
        {
            auto kind = Kind::Text;
            switch (shown.kind)
            {
                case View::Kind::Page: kind = Kind::Plugin; break;
                case View::Kind::Row: kind = Kind::Setting; break;
                case View::Kind::Previous: kind = Kind::Previous; break;
                case View::Kind::Next: kind = Kind::Next; break;
                case View::Kind::Details: kind = Kind::Details; break;
                case View::Kind::Back: kind = Kind::Back; break;
                default: break;
            }
            auto help = shown.kind == View::Kind::Row ? state.model.Help(shown.row) : std::string{};
            state.items.push_back({ kind, shown.index, shown.row, asi_settings::text::Narrow(shown.text),
                std::move(help), shown.enabled });
        }
        const auto count = std::min(size_t(8), state.items.size());
        state.selected = count ? std::min(state.selected, count - 1) : 0;
        for (size_t i = 0; i < count; ++i)
        {
            const auto index = i;
            if (state.items[i].kind == Kind::Text)
            {
                auto text = TextField("wfpText" + std::to_string(i), 29100.0f + static_cast<float>(i),
                    70.0f, 110.0f + 36.0f * static_cast<float>(i), 650.0f, 35.0f);
                Set(text, "text", state.items[i].label);
                Set(text, "_visible", 1.0f);
                continue;
            }
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
            Set(widget, "buttonEnabled", state.items[index].enabled ? 1.0f : 0.0f);
            Set(widget, "isEnabled", state.items[index].enabled ? 1.0f : 0.0f);
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
        ShowHelp(count ? state.items[state.selected].help : std::string{});
        ShowRestart();
        auto title = state.view.page < state.model.Pages().size() ? state.model.Pages()[state.view.page].title
            : state.model.Text("menu.plugins");
        if (state.view.count > 1)
            title += " (" + std::to_string(state.view.physical + 1) + "/" + std::to_string(state.view.count) + ")";
        auto heading = TextField("wfpTitle", 29200.0f, 70.0f, 70.0f, 650.0f, 35.0f);
        Set(heading, "text", title);
        Set(heading, "_visible", 1.0f);
        RWGTAMenu::Model::Changed() = false;
    }
    void Close()
    {
        HideRows();
        Stock(true);
        state.items.clear();
        state.open = false;
        state.view = {};
        ShowHelp({});
        ShowRestart();
        Set(Get(state.root, "wfpTitle"), "_visible", 0.0f);
    }
    void Act()
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
        if (!state.open || command < 0 || static_cast<size_t>(command) >= state.items.size())
            return;
        state.view.selected = static_cast<size_t>(command);
        if (!state.view.Activate(state.model, false, state.direction))
        {
            Close();
            return;
        }
        state.selected = state.view.selected;
        Build();
    }
    void Joined()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
            language = state.autoLanguage;
        if (language != state.language)
        {
            state.language = language;
            state.items.clear();
            state.view = {};
            state.selected = 0;
            state.model.OpenPlugins(state.module, state.language, 32, {}, true);
            RememberRestartValues();
        }
        Label(Get(state.root, "wfpPlugins"), state.model.Text("menu.plugins"), {});
    }
    void Input()
    {
        DWORD foreground{};
        GetWindowThreadProcessId(GetForegroundWindow(), &foreground);
        if (GodfatherInput::Blocked() || foreground != GetCurrentProcessId() || !state.root || !Number(state.root, "_visible"))
        {
            state.inputMask = 0;
            return;
        }
        uint32_t mask{};
        const int keys[]{ VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT, VK_RETURN, VK_ESCAPE, VK_F6, VK_F1 };
        for (size_t i = 0; i < std::size(keys); ++i)
            if (GetAsyncKeyState(keys[i]) < 0)
                mask |= 1u << i;
        mask |= GodfatherInput::MenuMask();
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
        {
            state.view.selected = state.selected;
            state.view.Move(-1);
            state.selected = state.view.selected;
        }
        if (pressed & 2)
        {
            state.view.selected = state.selected;
            state.view.Move(1);
            state.selected = state.view.selected;
        }
        if (pressed & ((1u << 6) | (1u << 7)))
        {
            for (size_t i = 0; i < state.items.size(); ++i)
                if (state.items[i].kind == Kind::Details)
                    state.command = static_cast<int>(i);
            state.direction = 0;
            return;
        }
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
    void Install(Value* root)
    {
        if (state.root == root)
            return;
        if (state.root)
        {
            Stock(true);
            state.settleAt = 0;
            state.items.clear();
            state.view = {};
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
        state.model.OpenPlugins(state.module, state.language, 32, {}, true);
        RememberRestartValues();
        auto source = Get(root, "test");
        auto widget = Clone(source, "wfpPlugins", 25002.0f);
        if (widget && source)
        {
            Set(widget, "_x", Number(source, "_x"));
            Set(widget, "_y", Number(source, "_y") + 32.0f);
            Set(widget, "wfpIndex", -2.0f);
            Label(widget, state.model.Text("menu.plugins"), {});
        }
        else
            Log("Native plugin entry creation failed");
        Joined();
        GodfatherResolutions::Populate(root);
        ShowHelp({});
    }
    Value* __cdecl Gamma(Value* receiver, int count)
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
    int __fastcall Actions(void* interpreter, void*, void* code, Value* receiver, int scope, int flags)
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
                Stock(true);
                state.settleAt = 0;
                state.items.clear();
                state.view = {};
                state.model.Close();
                Reference(state.root, false);
                state.root = nullptr;
                state.open = false;
                state.command = -1;
            }
            else
            {
                GodfatherResolutions::Populate(video);
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
        if (tick)
            tick();
        state.busy = false;
        return result;
    }
    void Init()
    {
        GodfatherInput::Init();
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.emplace(setting.id, FixSettings().Wanted(setting.id));
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                reinterpret_cast<LPCWSTR>(&Init), &state.module) ||
            !Bind())
            throw std::runtime_error("Native menu bindings unavailable");
        auto gamma = GammaEntry();
        auto actions = find_pattern("8B 44 24 0C 83 EC 1C 83 F8 FF 53 55 56 57 8B 7C 24 34 8B F1");
        state.gammaHook = safetyhook::create_inline(gamma, Gamma);
        state.actionsHook = safetyhook::create_inline(actions, Actions);
        if (!state.gammaHook || !state.actionsHook)
        {
            state.gammaHook.reset();
            state.actionsHook.reset();
            throw std::runtime_error("Native menu hooks unavailable");
        }
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
                           reinterpret_cast<LPCWSTR>(&Init), &state.module);
    }
} // namespace GodfatherMenu
