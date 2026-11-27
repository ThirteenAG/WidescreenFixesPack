module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "NativeTextHooks.hpp"
#include "Shared/RWGTA/FixMenu.hpp"


export module NativeText;

import Hud;
import Renderer;

export namespace NeoText
{
    struct Template
    {
        std::array<uint8_t, 1204> Node{};
        uintptr_t Root = 0;
        uintptr_t Scene = 0;
        uintptr_t Edi = 0;
        uintptr_t Esi = 0;
        bool Ready = false;
    };

    struct TutorialText
    {
        const void* Node{};
        std::wstring Keyboard;
        std::wstring Pad;
    };

    inline SafetyHookMid TextHook, ActionHook;
    inline SafetyHookInline TutorialTextHook;
    inline thread_local std::array<TutorialText, 32> TutorialTexts;
    inline thread_local size_t NextTutorialText = 0;
    inline int (__thiscall* NativeBindingText)(void*, uint16_t) = nullptr;
    inline const int16_t* InputTagRows = nullptr;
    inline const int32_t* PadBindings = nullptr;
    inline uintptr_t NativeDraw = 0;
    inline thread_local bool Painting = false;
    inline thread_local Template Current;
    inline thread_local Template OptionText;
    inline thread_local Template PromptText;
    inline thread_local std::wstring Prompt;
    inline std::wstring (*UpdateNotice)() = nullptr;
    inline std::map<std::string, std::string> Startup;
    inline std::mutex RestartMutex;
    inline std::atomic<bool> RestartPending{ false };
    inline std::atomic<bool> CheckForUpdates{ false };
    inline std::mutex LanguageMutex;
    inline std::string Language = "en";
    inline bool LanguagePending = true;

    enum class Input : int16_t { Up = 1, Down, Left, Right, Select, Cancel };
    enum class Menu { Options, Plugins, Settings };
    enum class OptionsScreen { None, Title, Pause };
    constexpr size_t RowsPerPage = 7;
    constexpr uint32_t LanguagesLabel = 0x1E9A;
    constexpr uint32_t OptionsHelp = 0x2058;
    constexpr uint32_t SubtitlesLabel = 7860;
    constexpr uint32_t PauseOptionsHelp = 9307;
    inline RWGTAMenu::Model Plugins;
    inline std::recursive_mutex MenuMutex;
    inline uintptr_t OptionsRoot = 0;
    inline Menu Page = Menu::Options;
    inline OptionsScreen Source = OptionsScreen::None;
    inline bool PluginsSelected = false;
    inline size_t PluginIndex = 0, FirstPlugin = 0;
    inline int SettingSlot = 0;
    inline void ApplyLanguage();
    inline void DrawMenu(bool paused);

    inline bool IsOptions(const char* name)
    {
        return NeoHud::Contains(name, "gm_options_PC.lou");
    }

    inline void ShowPage(Menu page)
    {
        Page = page;
        NeoHud::OptionsSubpageVisible.store(Source == OptionsScreen::Title && page != Menu::Options, std::memory_order_relaxed);
    }

    inline std::string CurrentLanguage()
    {
        const std::lock_guard lock(LanguageMutex);
        return Language;
    }

    inline std::string MenuText(const char* key, const char* fallback)
    {
        ApplyLanguage();
        const auto text = FixSettings().GetTexts().Resolve(key);
        return text == key || text.empty() ? fallback : text;
    }

    inline int32_t* ScriptInteger(uintptr_t root, const char* key)
    {
        // Louie's script variables are a linked list of name/value pairs.
        auto variable = *reinterpret_cast<uint8_t**>(root + 388);
        while (variable)
        {
            const auto name = *reinterpret_cast<const char* const*>(variable);
            if (name && std::strcmp(name, key) == 0)
                return reinterpret_cast<int32_t*>(variable + 4);
            variable = *reinterpret_cast<uint8_t**>(variable + 16);
        }
        return nullptr;
    }

    inline OptionsScreen ScreenFor(uintptr_t root, const char* name)
    {
        if (IsOptions(name))
            return OptionsScreen::Title;
        if (!NeoHud::Contains(name, "pause_screen_PC.lou") || *reinterpret_cast<const int32_t*>(root + 12) != 8)
            return OptionsScreen::None;
        const auto selection = ScriptInteger(root, "nSelectionState");
        // The pause script uses 2, 3, 4, 0 and 7 for its five PC options on frame 8.
        if (selection && (*selection == 0 || *selection == 7 || (*selection >= 2 && *selection <= 4)))
            return OptionsScreen::Pause;
        return OptionsScreen::None;
    }

    inline Input MenuInput(int16_t message, bool paused)
    {
        if (paused)
        {
            switch (message)
            {
                case 1: return Input::Up;
                case 2: return Input::Down;
                case 11: return Input::Left;
                case 12: return Input::Right;
                case 4: return Input::Select;
                case 5: return Input::Cancel;
                default: return {};
            }
        }
        return static_cast<Input>(message);
    }

    inline void MovePlugins(int direction)
    {
        const size_t count = Plugins.Pages().size() + 1;
        PluginIndex = direction > 0 ? (PluginIndex + 1) % count :
            (PluginIndex ? PluginIndex - 1 : count - 1);
        if (PluginIndex < FirstPlugin)
            FirstPlugin = PluginIndex;
        if (PluginIndex >= FirstPlugin + RowsPerPage)
            FirstPlugin = PluginIndex - RowsPerPage + 1;
    }

    inline void Navigate(Input input)
    {
        if (Page == Menu::Plugins)
        {
            if (input == Input::Up || input == Input::Down)
                MovePlugins(input == Input::Up ? -1 : 1);
            if (input == Input::Cancel || (input == Input::Select && PluginIndex == Plugins.Pages().size()))
                ShowPage(Menu::Options);
            else if (input == Input::Select)
            {
                ShowPage(Menu::Settings);
                SettingSlot = 0;
                while (Plugins.HiddenAbove(PluginIndex))
                    Plugins.Scroll(PluginIndex, -1);
            }
            return;
        }
        if (input == Input::Cancel)
        {
            ShowPage(Menu::Plugins);
            return;
        }

        auto rows = Plugins.WindowRows(PluginIndex);
        if (input == Input::Up || input == Input::Down)
            SettingSlot = Plugins.Move(PluginIndex, SettingSlot, input == Input::Up ? -1 : 1);
        else if (SettingSlot >= 0 && size_t(SettingSlot) < rows.size())
        {
            if (const auto row = rows[SettingSlot])
                Plugins.Change(*row, input == Input::Select ? 0 : input == Input::Left ? -1 : 1,
                    Source == OptionsScreen::Pause);
            else if (input == Input::Select)
                ShowPage(Menu::Plugins);
        }
    }

    inline bool HandleOptionAction(uintptr_t root, int16_t message)
    {
        if (message < 1 || message > 12 || !(*reinterpret_cast<const uint32_t*>(root + 612) & 1))
            return false;
        const auto name = *reinterpret_cast<const char**>(root + 632);
        const auto source = ScreenFor(root, name);
        if (source == OptionsScreen::None)
            return false;
        const bool paused = source == OptionsScreen::Pause;
        const auto input = MenuInput(message, paused);
        if (input < Input::Up || input > Input::Cancel)
            return false;

        const std::lock_guard lock(MenuMutex);
        struct RecentInput
        {
            Input message{};
            ULONGLONG at{};
        };
        static RecentInput previous;
        if (OptionsRoot != root || Source != source)
        {
            OptionsRoot = root;
            Source = source;
            PluginsSelected = false;
            ShowPage(Menu::Options);
            previous = {};
        }
        // Louie can send the same menu edge several times before the next input sample.
        const auto now = GetTickCount64();
        if (input == previous.message && now - previous.at < 80)
            return true;
        previous = { input, now };
        if (Page != Menu::Options)
        {
            Plugins.SelectLanguage(CurrentLanguage());
            Navigate(input);
            return true;
        }
        const auto selection = ScriptInteger(root, paused ? "nSelectionState" : "MenuSelection");
        if (!selection)
            return false;
        const int first = paused ? 2 : 1;
        const int last = paused ? 7 : 4;
        if (!PluginsSelected)
        {
            if ((input == Input::Down && *selection == last) || (input == Input::Up && *selection == first))
            {
                PluginsSelected = true;
                return true;
            }
            if (input == Input::Cancel)
                OptionsRoot = 0;
            return false;
        }
        if (input == Input::Select)
        {
            Plugins.OpenPlugins(NeoRenderer::ThisModule(), CurrentLanguage(), RowsPerPage, {}, true);
            PluginIndex = FirstPlugin = 0;
            ShowPage(Menu::Plugins);
            return true;
        }
        if (input == Input::Cancel)
        {
            PluginsSelected = false;
            OptionsRoot = 0;
            return false;
        }
        if (input == Input::Up || input == Input::Down)
        {
            PluginsSelected = false;
            // Let Louie perform the original wrap and update its animation and sound.
            *selection = input == Input::Up ? first : last;
            return false;
        }
        return true;
    }

    inline void DispatchOptionAction(SafetyHookContext& regs)
    {
        // Louie has resolved the device action to a message for this root. -1 means there is no message to send.
        if (HandleOptionAction(regs.esi, static_cast<int16_t>(regs.eax)))
            regs.eax |= 0xFFFF;
    }

    inline void ReadLanguage()
    {
        const std::lock_guard lock(LanguageMutex);
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (language.empty() || _stricmp(language.c_str(), "auto") == 0)
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
                case LANG_FRENCH: language = "fr"; break;
                case LANG_GERMAN: language = "de"; break;
                case LANG_ITALIAN: language = "it"; break;
                case LANG_SPANISH: language = "es"; break;
                default: language = "en"; break;
            }
        }
        Language = language;
        LanguagePending = true;
    }

    inline void ApplyLanguage()
    {
        const std::lock_guard lock(LanguageMutex);
        if (LanguagePending)
        {
            FixSettings().GetTexts().Select(Language);
            LanguagePending = false;
        }
    }

    inline void RefreshRestartRequirement()
    {
        const std::lock_guard lock(RestartMutex);
        RestartPending.store(std::ranges::any_of(Startup, [](const auto& setting)
        {
            return FixSettings().Wanted(setting.first) != setting.second;
        }));
    }

    inline bool NeedsNotice()
    {
        return RestartPending.load() || CheckForUpdates.load();
    }

    inline bool IsNoticeRoot(uintptr_t root)
    {
        const auto name = *reinterpret_cast<const char**>(root + 632);
        return NeoHud::Contains(name, "start_screen") || NeoHud::Contains(name, "gm_") ||
            NeoHud::Contains(name, "pause_screen");
    }

    inline void SaveTemplate(Template& target, const SafetyHookContext& context, uintptr_t root, uintptr_t scene)
    {
        std::memcpy(target.Node.data(), reinterpret_cast<void*>(context.ecx), target.Node.size());
        target.Root = root;
        target.Scene = scene;
        target.Edi = context.edi;
        target.Esi = context.esi;
        target.Ready = true;
    }

    template <typename T>
    inline void SetNode(Template& target, size_t offset, T value)
    {
        std::memcpy(target.Node.data() + offset, &value, sizeof(value));
    }

    inline std::wstring_view TutorialButton(wchar_t tag)
    {
        // The game's four PC profiles use the same controller table for these tags. Xidi's
        // StandardGamepad mapper presents these DirectInput buttons from XInput.
        static constexpr std::array<std::wstring_view, 13> buttons{
            L"", L"(A)", L"(B)", L"(X)", L"(Y)", L"(LB)", L"(RB)", L"(LT)", L"(RT)",
            L"(Back)", L"(Start)", L"(LS)", L"(RS)"
        };
        if (tag < 1 || tag > 25)
            return {};
        const auto row = InputTagRows[tag];
        if (row < 0 || row >= 58)
            return {};
        const auto button = PadBindings[row];
        return button > 0 && button < static_cast<int>(buttons.size()) ? buttons[button] : std::wstring_view{};
    }

    inline int __fastcall RememberTutorialText(void* node, void*, const wchar_t** text, int* length)
    {
        const auto callNative = [&]() { return TutorialTextHook.thiscall<int>(node, text, length); };
        if (!*text || *length < 1 || *length >= 1024)
            return callNative();
        const std::wstring_view authored(*text, *length);
        if (std::ranges::none_of(authored, [](wchar_t tag) { return !TutorialButton(tag).empty(); }))
            return callNative();

        // During Set Text the original object still has the keyboard binding context used by the
        // lesson. Prepare its controller text here, then keep the game's keyboard text intact.
        alignas(void*) std::array<uint8_t, 1204> copy;
        std::memcpy(copy.data(), node, copy.size());
        constexpr size_t bufferOffset = 668;
        const auto buffer = reinterpret_cast<const wchar_t*>(copy.data() + bufferOffset);
        std::wstring pad;
        pad.reserve(authored.size());
        for (const auto letter : authored)
        {
            if (const auto button = TutorialButton(letter); !button.empty())
                pad.append(button);
            else if (letter > 0 && letter < L' ')
            {
                const auto size = NativeBindingText(copy.data(), letter);
                if (size > 0 && size_t(size) <= (copy.size() - bufferOffset) / sizeof(wchar_t))
                    pad.append(buffer, size);
                else
                    pad.push_back(letter);
            }
            else
                pad.push_back(letter);
        }
        const auto result = callNative();
        if (*length > 0 && *length <= 1024 && pad.size() <= 1024)
        {
            auto& entry = TutorialTexts[NextTutorialText++ % TutorialTexts.size()];
            std::wstring keyboard(*text, *length);
            if (keyboard.back() == L'\0')
                keyboard.pop_back();
            if (!pad.empty() && pad.back() == L'\0')
                pad.pop_back();
            entry = { node, std::move(keyboard), std::move(pad) };
        }
        return result;
    }

    inline std::wstring_view FindTutorialText(const void* node, std::wstring_view keyboard)
    {
        const TutorialText* matching = nullptr;
        for (const auto& entry : TutorialTexts)
        {
            if (entry.Keyboard != keyboard)
                continue;
            if (entry.Node == node)
                return entry.Pad;
            if (!matching)
                matching = &entry;
        }
        return matching ? std::wstring_view(matching->Pad) : std::wstring_view{};
    }

    inline void PadPrompt(SafetyHookContext& context, uintptr_t root, uintptr_t scene, bool hasBack)
    {
        const auto node = reinterpret_cast<const uint8_t*>(context.ecx);
        const auto count = *reinterpret_cast<const uint32_t*>(node + 68);
        const auto text = *reinterpret_cast<const wchar_t* const*>(node + 72);
        if (!text || !count || count > 1024)
            return;
        const auto original = std::wstring_view(text, count);
        if (hasBack && !original.contains(L'('))
            return;
        SaveTemplate(PromptText, context, root, scene);
        if (hasBack)
        {
            if (!original.contains(L"(Enter)") && !original.contains(L"(Esc)"))
                return;
            Prompt = original;
            const auto replace = [](std::wstring_view token, const wchar_t* button)
            {
                for (auto at = Prompt.find(token); at != std::wstring::npos; at = Prompt.find(token, at + 3))
                    Prompt.replace(at, token.size(), button);
            };
            replace(L"(Enter)", L"(B)");
            replace(L"(Esc)", L"(X)");
        }
        else
        {
            Prompt = FindTutorialText(node, original);
            if (Prompt.empty())
            {
                if (!original.contains(L"(Enter)"))
                    return;
                Prompt = original;
                for (auto at = Prompt.find(L"(Enter)"); at != std::wstring::npos; at = Prompt.find(L"(Enter)", at + 3))
                    Prompt.replace(at, 7, L"(B)");
            }
        }
        if (Prompt == original)
            return;
        SetNode(PromptText, 68, static_cast<uint32_t>(Prompt.size()));
        SetNode(PromptText, 72, Prompt.c_str());
        context.ecx = reinterpret_cast<uintptr_t>(PromptText.Node.data());
    }

    inline void Capture(SafetyHookContext& context)
    {
        const bool wantsNotice = NeedsNotice();
        const bool pad = NeoRenderer::PadPrompts.load();
        if (Painting)
            return;
        const auto root = *reinterpret_cast<uintptr_t*>(context.esp + 4);
        const auto scene = *reinterpret_cast<uintptr_t*>(context.esp + 8);
        if (!root || !scene)
            return;
        const auto name = *reinterpret_cast<const char**>(root + 632);
        const auto screen = ScreenFor(root, name);
        if (screen == OptionsScreen::None && !wantsNotice && !pad)
            return;
        const int width = *reinterpret_cast<int*>(root + 600), height = *reinterpret_cast<int*>(root + 604);
        if (width < 320 || width > 8192 || height < 240 || height > 8192)
            return;
        if (screen != OptionsScreen::None)
        {
            const bool paused = screen == OptionsScreen::Pause;
            const auto textId = *reinterpret_cast<const uint32_t*>(context.ecx + 80);
            const std::lock_guard lock(MenuMutex);
            if (textId == (paused ? SubtitlesLabel : LanguagesLabel))
            {
                SaveTemplate(OptionText, context, root, scene);
                DrawMenu(paused);
            }
            const float y = *reinterpret_cast<const float*>(context.ecx + 36);
            const bool help = paused ? textId == PauseOptionsHelp || y > 0.73f : textId == OptionsHelp;
            if (OptionsRoot == root && Page != Menu::Options && !help)
            {
                SaveTemplate(PromptText, context, root, scene);
                SetNode(PromptText, 68, uint32_t(0));
                context.ecx = reinterpret_cast<uintptr_t>(PromptText.Node.data());
                return;
            }
        }
        const bool menu = (wantsNotice || pad) && IsNoticeRoot(root);
        if (wantsNotice && menu)
            SaveTemplate(Current, context, root, scene);
        if (pad && (menu || NeoHud::Contains(name, "static_prompt.lou")))
        {
            PadPrompt(context, root, scene, menu);
        }
    }

    inline void DrawLine(const Template& source, const std::wstring& text, float x, float y, float width, float scale)
    {
        auto line = source;
        SetNode(line, 32, x);
        SetNode(line, 36, y);
        SetNode(line, 44, width);
        SetNode(line, 48, 0.06f);
        SetNode(line, 68, static_cast<uint32_t>(text.size()));
        SetNode(line, 72, text.c_str());
        SetNode(line, 620, 0.0f);
        SetNode(line, 624, 0.0f);
        SetNode(line, 628, scale);
        Painting = true;
        Invoke(NativeDraw, line.Node.data(), source.Root, source.Scene, source.Edi, source.Esi);
        Painting = false;
    }

    inline void DrawMenuText(const std::string& text, float x, float y, float width, float scale = 0.45f)
    {
        DrawLine(OptionText, asi_settings::text::Widen(text), x, y, width, scale);
    }

    inline void DrawMenu(bool paused)
    {
        const auto page = OptionsRoot == OptionText.Root ? Page : Menu::Options;
        const bool selected = OptionsRoot == OptionText.Root && PluginsSelected;
        if (page == Menu::Options)
        {
            const auto plugins = MenuText("@menu.plugins", "Plugins");
            const auto data = OptionText.Node.data();
            const auto x = *reinterpret_cast<const float*>(data + 32);
            const auto y = *reinterpret_cast<const float*>(data + 36);
            const auto scale = *reinterpret_cast<const float*>(data + 628);
            DrawMenuText(selected ? "> " + plugins : plugins, x, y + (paused ? 0.071f : 0.099f), 0.43f, scale);
        }
        else
        {
            Plugins.SelectLanguage(CurrentLanguage());
            const auto& pages = Plugins.Pages();
            const auto back = MenuText("@menu.back", "Back");
            const float firstRow = paused ? 0.25f : 0.335f;
            const float rowStep = paused ? 0.058f : 0.067f;
            DrawMenuText(page == Menu::Plugins ? MenuText("@menu.plugins", "Plugins") : pages[PluginIndex].title,
                0.19f, paused ? 0.15f : 0.225f, 0.67f, 0.63f);
            if (page == Menu::Plugins)
            {
                for (size_t i = FirstPlugin; i <= pages.size() && i < FirstPlugin + RowsPerPage; ++i)
                {
                    const auto& label = i == pages.size() ? back : pages[i].title;
                    DrawMenuText((PluginIndex == i ? "> " : "  ") + label,
                        0.19f, firstRow + (i - FirstPlugin) * rowStep, 0.67f);
                }
            }
            else
            {
                const auto rows = Plugins.WindowRows(PluginIndex);
                for (size_t i = 0; i < rows.size(); ++i)
                {
                    const auto row = rows[i];
                    const float y = firstRow + i * rowStep;
                    DrawMenuText((size_t(SettingSlot) == i ? "> " : "  ") + (row ? Plugins.Label(*row) : back),
                        0.19f, y, 0.47f);
                    if (row)
                        DrawMenuText(Plugins.Value(*row), 0.66f, y, 0.29f);
                }
            }
        }
        OptionText.Ready = false;
    }

    inline void BeforeRoot(void* root)
    {
        if (NeoHud::Root.options)
        {
            OptionText.Ready = false;
            const std::lock_guard lock(MenuMutex);
            NeoHud::Root.hideOptionFocus = OptionsRoot == reinterpret_cast<uintptr_t>(root) && PluginsSelected;
        }
        if (!Current.Ready || Current.Root != reinterpret_cast<uintptr_t>(root))
            return;
        if (!NeedsNotice() || !IsNoticeRoot(Current.Root))
        {
            Current.Ready = false;
            return;
        }

        ApplyLanguage();
        auto message = CheckForUpdates.load() && UpdateNotice ? UpdateNotice() : std::wstring{};
        if (message.empty() && RestartPending.load())
            message = asi_settings::text::Widen(FixSettings().GetTexts().Resolve("@menu.restartwarning"));
        if (message.empty())
        {
            Current.Ready = false;
            return;
        }
        size_t begin = 0;
        for (unsigned line = 0; begin < message.size() && line < 4; ++line)
        {
            size_t end = std::min(begin + 62, message.size());
            if (end < message.size())
            {
                const auto space = message.rfind(L' ', end);
                if (space != std::wstring::npos && space > begin)
                    end = space;
            }
            DrawLine(Current, message.substr(begin, end - begin), 0.06f, 0.04f + line * 0.045f, 0.88f, 0.35f);
            begin = end;
            while (begin < message.size() && message[begin] == L' ')
                ++begin;
        }
        Current.Ready = false;
    }

    inline void Install()
    {
        ReadLanguage();
        ApplyLanguage();
        FixSettings().OnChange("MAIN.TextLanguage", ReadLanguage);
        for (const auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.apply == "restart")
                Startup.emplace(setting.id, FixSettings().Wanted(setting.id));
        }
        for (const auto& setting : Startup)
            FixSettings().OnChange(setting.first, RefreshRestartRequirement);
        RefreshRestartRequirement();
        WatchNeoSetting("MAIN.CheckForUpdates", [](const std::string& value)
        {
            CheckForUpdates.store(value == "1");
        });
        auto text = hook::pattern("64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 81 EC 8C 01 00 00 53 55 8B E9 8B 45 44");
        NativeDraw = reinterpret_cast<uintptr_t>(text.get_first());
        TextHook = safetyhook::create_mid(text.get_first(), Capture);
        auto action = hook::pattern("8B 44 24 04 56 8B F1 66 8B 84 46 94 02 00 00 66 83 F8 FF 74 ? 0F BF C8 FF 35");
        Plugins.RememberRunningSettings(FixSettings());
        ActionHook = safetyhook::create_mid(action.get_first(15), DispatchOptionAction);
        auto formatter = hook::pattern("51 53 56 57 8B ? 0F B7 4C 24 14 8D 44 24 0C 50 51 33 ? 8B ? 89 ? 24 14 E8");
        const auto lookup = injector::GetBranchDestination(formatter.get_first(25), true).as_int();
        auto tags = hook::range_pattern(lookup, lookup + 0x40, "0F BF ? ? ? ? ? ? 83 ? FF");
        InputTagRows = *tags.get_first<const int16_t*>(4);
        auto bindings = hook::pattern("8B 98 ? ? ? ? 89 98 ? ? ? ? 8B 98 ? ? ? ? 89 98 ? ? ? ? 05 E8 00 00 00 4A 75 B0 8B 91");
        PadBindings = *bindings.get_first<const int32_t*>(8);
        NativeBindingText = reinterpret_cast<decltype(NativeBindingText)>(formatter.get_first());
        auto tutorialText = hook::pattern("83 EC 08 53 55 8B 6C 24 14 56 57 8B 7C 24 20 8B 17 8B F1 33 DB 85 D2");
        TutorialTextHook = safetyhook::create_inline(tutorialText.get_first(), RememberTutorialText);
        NeoHud::BeforeRoot = BeforeRoot;
    }
}
