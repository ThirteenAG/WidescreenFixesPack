module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "RuntimeBridge.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/RainDroplets.hpp"
#include <filesystem>
#include <stdexcept>

export module NativeMenu;

import ComVars;

export namespace SaboteurMenu
{
    struct Value
    {
        uint32_t type{ 0 };
        uint32_t padding{ 0 };
        union
        {
            double number;
            const char* string;
            bool boolean;
        } data{};

        explicit Value(const char* text) : type{ 4 } { data.string = text; }
        explicit Value(double number) : type{ 3 } { data.number = number; }
        Value() = default;
    };
    static_assert(sizeof(Value) == 16);

    inline RWGTAMenu::Model Model;
    inline SafetyHookInline FileHook, CallbackHook;
    inline void* (__stdcall* OpenFile)(const char*, int, int) = nullptr;
    inline HMODULE Module = nullptr;
    inline GameRef<uintptr_t>* NativeLanguage = nullptr;
    inline std::filesystem::path Directory;
    inline std::map<std::string, std::string> Startup;
    inline std::string ShownLanguage;
    // The shown list (the plugins list, a plugin's page or a category) scrolls through the page's 4 rows (the
    // Settings pages' row height, the help under them), Back last
    inline constexpr size_t Slots{ 4 };
    inline size_t Page{ RWGTAMenu::Model::RootPage };
    inline bool Details{ false };
    inline RWGTAMenu::Row HelpRow;
    inline std::string Notice;
    inline void* NotificationMovie = nullptr;
    inline void* ActiveMovie = nullptr;
    inline std::atomic<bool> MenuOpen{ false }, PendingHelp{ false };
    inline bool HelpHeld = false;
    // The thread the menu movie calls from, the wheel's notches from the window, the last call of a menu movie
    inline std::atomic<DWORD> MovieThread{ 0 };
    inline MenuList::WheelNotches Notches;
    inline std::atomic<ULONGLONG> LastMenuCall{ 0 };
    // The gamma screen at startup (gamma.gfx) comes before the menu movies: it counts as a menu until one of them calls
    inline std::atomic<bool> GammaShown{ false };
    inline std::map<std::pair<HMODULE, std::string>, std::string> OtherStartup;

    inline void Remember(const asi_settings::LoadedPlugin& plugin)
    {
        if (plugin.module == Module)
            return;
        for (const auto& setting : plugin.file.description.settings)
            if (setting.HasValue() && setting.apply == "restart")
                OtherStartup.try_emplace({ plugin.module, setting.id }, OtherPlugins::Get(plugin, setting));
    }

    inline std::atomic<int> Wheel{ 0 }, PageKeys{ 0 }, EdgeKeys{ 0 };

    inline void Poll(bool blocked)
    {
        const bool down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        const auto window = SaboteurRuntime::GetWindow();
        const bool focused = !blocked && MenuOpen.load() && window && GetForegroundWindow() == window;
        if (focused && down && !HelpHeld)
            PendingHelp.store(true);
        HelpHeld = down;
        // Page Up/Down, Home/End: the menu movie has no use for them
        static bool held[4]{};
        constexpr int keys[]{ VK_PRIOR, VK_NEXT, VK_HOME, VK_END };
        for (int i = 0; i < 4; ++i)
        {
            const bool pressed = focused && (GetAsyncKeyState(keys[i]) & 0x8000) != 0;
            if (pressed && !held[i])
            {
                if (i < 2)
                    PageKeys += i ? 1 : -1;
                else
                    EdgeKeys = i == 3 ? 1 : -1;
            }
            held[i] = pressed;
        }
    }

    struct Item
    {
        const RWGTAMenu::Row* row{};     // null: Back
        std::string label, value;
    };
    inline std::vector<Item> Items;

    inline std::string Language()
    {
        const auto language = FixSettings().String("MAIN.TextLanguage");
        if (language != "auto")
            return language;
        const auto manager = NativeLanguage ? NativeLanguage->get() : 0;
        switch (manager ? *reinterpret_cast<const int*>(manager) : 0)
        {
            case 1: return "de";
            case 2: return "fr";
            case 3: return "es";
            case 4: return "it";
            default: return "en";
        }
    }

    inline void Invoke(void* movie, const char* name, std::initializer_list<Value> values)
    {
        Value result;
        const auto call = reinterpret_cast<bool(__thiscall*)(void*, const char*, Value*, const Value*, unsigned)>(
            (*static_cast<void***>(movie))[18]);
        call(movie, name, &result, values.begin(), static_cast<unsigned>(values.size()));
    }

    inline bool RestartPending()
    {
        for (const auto& [id, value] : Startup)
            if (FixSettings().Wanted(id) != value)
                return true;
        for (const auto& page : Model.Pages())
        {
            if (!page.plugin || page.plugin->module == Module)
                continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart")
                    continue;
                const auto original = OtherStartup.find({ page.plugin->module, setting.id });
                if (original != OtherStartup.end() && original->second != OtherPlugins::Get(*page.plugin, setting))
                    return true;
            }
        }
        return false;
    }

    inline void Open()
    {
        ShownLanguage = Language();
        Model.OpenMenu(Module, ShownLanguage, Slots);
        for (const auto& page : Model.Pages())
            if (page.plugin)
                Remember(*page.plugin);
        MenuOpen.store(true);
        Page = RWGTAMenu::Model::RootPage;
        Model.Window(Page).Reset();
        Notches.Clear();
        Wheel = PageKeys = EdgeKeys = 0;
        Details = false;
    }

    inline void Render(void* movie)
    {
        if (!Model.IsOpen())
            return;
        if (ShownLanguage != Language())
            Open();
        if (!Notice.empty())
        {
            Invoke(movie, "_global.FFBegin", { Value(Model.Text("menu.plugins").c_str()), Value(0.0), Value(0.0),
                Value(1.0), Value(Notice.c_str()), Value(""), Value(Model.Text("menu.back").c_str()) });
            return;
        }
        Items.clear();
        if (Page >= Model.Pages().size())
            Page = RWGTAMenu::Model::RootPage;
        for (auto row : Model.Shown(Page, true))
            Items.push_back(row ? Item{ row, Model.Label(*row), Model.Value(*row) } : Item{ nullptr, Model.Text("menu.back") });
        const auto& window = Model.Window(Page);
        const auto selected = (std::min)(window.Slot(), Items.size() - 1);
        if (Items[selected].row && !RWGTAMenu::Model::Opens(*Items[selected].row))
            HelpRow = *Items[selected].row;
        else if (!Details)
            HelpRow = {};
        const auto title = Page != RWGTAMenu::Model::RootPage ? Model.Pages()[Page].title : Model.Text("menu.plugins");
        const auto help = HelpRow.plugin ? Model.Help(HelpRow) : std::string{};
        const auto details = "F1: " + Model.Text("menu.details");
        Invoke(movie, "_global.FFLabel", { Value(Model.Text("menu.plugins").c_str()) });
        Invoke(movie, "_global.FFBegin", { Value(title.c_str()), Value(static_cast<double>(Items.size())),
            Value(static_cast<double>(selected)), Value(Details ? 1.0 : 0.0), Value(help.c_str()),
            Value(RestartPending() ? Model.Text("menu.restartwarning").c_str() : ""), Value(help.empty() ? "" : details.c_str()) });
        for (size_t i = 0; i < Items.size(); ++i)
            Invoke(movie, "_global.FFRow", { Value(static_cast<double>(i)), Value(Items[i].label.c_str()), Value(Items[i].value.c_str()) });
        Invoke(movie, "_global.FFMarks", { Value(!Details && window.Above() ? 1.0 : 0.0), Value(!Details && window.Below() ? 1.0 : 0.0) });
    }

    inline void Back(void* movie)
    {
        if (!Notice.empty())
            Notice.clear();
        else if (Details)
            Details = false;
        else if (const auto parent = Model.Back(Page); parent != SIZE_MAX)
        {
            // The list it came from, on the row it came from
            Page = parent;
            HelpRow = {};
        }
        else
        {
            Model.Close();
            Invoke(movie, "_global.FFClose", {});
            ActiveMovie = nullptr;
            MenuOpen.store(false);
            PendingHelp.store(false);
        }
    }

    inline void Input(void* movie, const char* command)
    {
        if (!Model.IsOpen() || movie != ActiveMovie)
            return;
        const std::string_view input{ command ? command : "" };
        if (input == "back")
            Back(movie);
        else if (input == "help" && HelpRow.plugin)
            Details = !Details;
        else if (!Details && !Items.empty())
        {
            auto& window = Model.Window(Page);
            if (input == "up" || input == "down")
                window.Move(input == "up" ? -1 : 1);
            else if (input.starts_with("select:"))
                window.Point(strtoul(command + 7, nullptr, 10));
            else if (input.starts_with("scroll:"))
                window.Scroll(input == "scroll:-1" ? -1 : 1);
            else if (input == "accept" || input == "left" || input == "right")
            {
                const auto slot = window.Slot();
                if (slot >= Items.size())
                    return;
                const auto item = Items[slot];
                const int direction = input == "left" ? -1 : input == "right" ? 1 : 0;
                if (!item.row)
                {
                    if (!direction)
                        Back(movie);
                }
                else if (RWGTAMenu::Model::Opens(*item.row))
                {
                    if (!direction)
                        if (const auto target = Model.Enter(*item.row); target != SIZE_MAX)
                            Page = target;
                }
                else
                    Model.Change(*item.row, direction, false);
            }
        }
        Render(movie);
    }

    // The wheel (one row per notch) and Page Up/Down, Home/End: the window moves
    inline void Scroll(void* movie)
    {
        if (!Model.IsOpen() || movie != ActiveMovie || Details || !Notice.empty())
        {
            Wheel = PageKeys = EdgeKeys = 0;
            return;
        }
        auto& window = Model.Window(Page);
        bool moved = false;
        const int wheel = Wheel.exchange(0);
        for (int i = 0; i < std::abs(wheel); ++i)
            moved |= window.Scroll(wheel > 0 ? -1 : 1);
        const int pages = PageKeys.exchange(0);
        for (int i = 0; i < std::abs(pages); ++i)
            moved |= window.Page(pages > 0 ? 1 : -1);
        if (const int edge = EdgeKeys.exchange(0))
            moved |= window.Edge(edge);
        if (moved)
            Render(movie);
    }

    inline void Message(UINT message, WPARAM wParam)
    {
        if (!MenuOpen.load())
            return;
        if (message == WM_MOUSEWHEEL)
            Wheel += Notches.Take(GET_WHEEL_DELTA_WPARAM(wParam));
    }

    inline int __fastcall Callback(void* self, void* edx, void* movie, const char* command, const char* argument)
    {
        if (command && command[0] == 'F' && command[1] == 'F')
        {
            MovieThread = GetCurrentThreadId();
            LastMenuCall = GetTickCount64();
            GammaShown = false;
        }
        if (command)
        {
            const std::string_view name{ command };
            if (name == "FFLabel")
            {
                const auto label = RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins");
                Invoke(movie, "_global.FFLabel", { Value(label.c_str()) });
                return 0;
            }
            if (name == "FFOpen")
            {
                ActiveMovie = movie;
                Open();
                Render(movie);
                return 0;
            }
            if (name == "FFInput")
            {
                Input(movie, argument);
                return 0;
            }
            if (name == "FFRefresh")
            {
                if (movie != ActiveMovie || !Model.IsOpen())
                    return 0;
                if (PendingHelp.exchange(false))
                    Input(movie, "help");
                NotificationMovie = movie;
                RWGTA::UpdateCheck::Tick();
                NotificationMovie = nullptr;
                if (Model.IsOpen() && (RWGTAMenu::Model::Changed().exchange(false) || ShownLanguage != Language()))
                    Render(movie);
                Scroll(movie);
                return 0;
            }
            // A menu movie is shown (LastMenuCall above)
            if (name == "FFMenu")
                return 0;
            if (name == "FFDismiss")
            {
                if (movie == ActiveMovie)
                {
                    Model.Close();
                    ActiveMovie = nullptr;
                    MenuOpen.store(false);
                    PendingHelp.store(false);
                    Notice.clear();
                }
                return 0;
            }
        }
        return CallbackHook.unsafe_fastcall<int>(self, edx, movie, command, argument);
    }

    inline void* __fastcall File(void* self, void* edx, const char* name, int flags, int mode)
    {
        if (name)
        {
            auto stem = std::filesystem::path(name).stem().string();
            if (!_stricmp(stem.c_str(), "gamma"))
                GammaShown = true;
            if (!_stricmp(stem.c_str(), "StartScreen") || !_stricmp(stem.c_str(), "PauseMenu") || !_stricmp(stem.c_str(), "FusionFixInterface"))
            {
                const auto path = Directory / (stem + ".gfx");
                if (std::filesystem::is_regular_file(path))
                    return OpenFile(path.string().c_str(), flags, mode);
                OutputDebugStringA("The Saboteur: native menu movie missing.\n");
            }
        }
        return FileHook.unsafe_fastcall<void*>(self, edx, name, flags, mode);
    }

    inline void PrepareInterface()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&PrepareInterface), &Module);
        wchar_t path[32768]{};
        GetModuleFileNameW(Module, path, static_cast<DWORD>(std::size(path)));
        Directory = std::filesystem::path(path).replace_extension();
        auto pattern = hook::pattern("51 53 56 57 33 DB 53 89 5C 24 10 8B 0D ? ? ? ? 6A 14 E8");
        OpenFile = reinterpret_cast<decltype(OpenFile)>(pattern.get_first());
        pattern = hook::pattern("8B 44 24 08 56 6A 01 50 8D 4C 24 14 E8 ? ? ? ? 8B 00 3D 46 06 4D 41 74");
        CallbackHook = safetyhook::create_inline(pattern.get_first(), Callback);
        pattern = hook::pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 08 53 55 57 33 DB 8D 44 24 0C 89 5C 24 0C");
        FileHook = safetyhook::create_inline(pattern.get_first(), File);
    }

    inline void Initialize()
    {
        for (const auto& plugin : asi_settings::FindLoadedPlugins(Module, "en"))
            Remember(plugin);
        auto language = hook::pattern("A1 ? ? ? ? 8B 00 3B C3 75 0E 6A 01 68 ? ? ? ? 68");
        NativeLanguage = BindGameRef(nativeLanguage, *language.get_first<uintptr_t*>(1));
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.apply == "restart")
                Startup.emplace(setting.id, FixSettings().Wanted(setting.id));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "TheSaboteur.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.playable = []() { return Model.IsOpen() && NotificationMovie && Notice.empty(); };
        config.format = [](const std::string&, bool hub)
        {
            return asi_settings::text::Widen(RWGTAMenu::PluginText(FixSettings(), Language(),
                hub ? "update.availablehub" : "update.availablesite"));
        };
        config.show = [](const std::wstring& text)
        {
            Notice = asi_settings::text::Narrow(text);
            Render(NotificationMovie);
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        WFP::onShutdownEvent() += []()
        {
            static std::atomic<bool> exited{ false };
            if (!exited.exchange(true))
                RWGTA::UpdateCheck::OnExit();
        };
    }
}

export namespace SaboteurMenu
{
    void InstallEarly()
    {
        PrepareInterface();
    }

    void Install()
    {
        Initialize();
    }

    void Frame(bool blocked)
    {
        Poll(blocked);
        // The menu movies call in while the start screen or pause menu is shown: no drops over them
        RainDroplets::MenuOpen(GammaShown.load() || GetTickCount64() - LastMenuCall.load() < 1000);
        if (ActiveMovie && GetCurrentThreadId() == MovieThread.load())
            Scroll(ActiveMovie);
    }

    void WindowMessage(UINT message, WPARAM wParam)
    {
        Message(message, wParam);
    }
}
