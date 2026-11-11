module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include "RuntimeBridge.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
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
    inline size_t Page{ SIZE_MAX }, Offset{ 0 }, Selected{ 0 };
    inline bool Details{ false };
    inline RWGTAMenu::Row HelpRow;
    inline std::string Notice;
    inline void* NotificationMovie = nullptr;
    inline void* ActiveMovie = nullptr;
    inline std::atomic<bool> MenuOpen{ false }, PendingHelp{ false };
    inline bool HelpHeld = false;
    inline std::map<std::pair<HMODULE, std::string>, std::string> OtherStartup;

    inline void Remember(const asi_settings::LoadedPlugin& plugin)
    {
        if (plugin.module == Module)
            return;
        for (const auto& setting : plugin.file.description.settings)
            if (setting.HasValue() && setting.apply == "restart")
                OtherStartup.try_emplace({ plugin.module, setting.id }, OtherPlugins::Get(plugin, setting));
    }

    inline void Poll(bool blocked)
    {
        const bool down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
        const auto window = SaboteurRuntime::GetWindow();
        if (!blocked && MenuOpen.load() && window && GetForegroundWindow() == window && down && !HelpHeld)
            PendingHelp.store(true);
        HelpHeld = down;
    }

    enum class Kind { Page, Setting, Previous, Next, Help, Back };
    struct Item
    {
        Kind kind;
        size_t index{ 0 };
        RWGTAMenu::Row row{};
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
        Model.OpenPlugins(Module, ShownLanguage, 4096, {}, true);
        for (const auto& page : Model.Pages())
            if (page.plugin)
                Remember(*page.plugin);
        MenuOpen.store(true);
        Page = SIZE_MAX;
        Offset = Selected = 0;
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
        if (Page == SIZE_MAX)
        {
            const auto count = Model.Pages().size();
            Offset = count ? (std::min)(Offset, (count - 1) / 5 * 5) : 0;
            for (size_t i = Offset; i < (std::min)(Offset + 5, count); ++i)
                Items.push_back({ Kind::Page, i, {}, Model.Pages()[i].title });
            if (Offset)
                Items.push_back({ Kind::Previous, 0, {}, Model.Text("menu.previous") });
            if (Offset + 5 < count)
                Items.push_back({ Kind::Next, 0, {}, Model.Text("menu.next") });
        }
        else if (Page < Model.Pages().size())
        {
            auto rows = Model.VisibleRows(Page);
            Offset = rows.empty() ? 0 : (std::min)(Offset, (rows.size() - 1) / 5 * 5);
            for (size_t i = Offset; i < (std::min)(Offset + 5, rows.size()); ++i)
                Items.push_back({ Kind::Setting, i, *rows[i], Model.Label(*rows[i]), Model.Value(*rows[i]) });
            if (Offset)
                Items.push_back({ Kind::Previous, 0, {}, Model.Text("menu.previous") });
            if (Offset + 5 < rows.size())
                Items.push_back({ Kind::Next, 0, {}, Model.Text("menu.next") });
        }
        Items.push_back({ Kind::Back, 0, {}, Model.Text("menu.back") });
        Selected = (std::min)(Selected, Items.size() - 1);
        if (Items[Selected].kind == Kind::Setting)
            HelpRow = Items[Selected].row;
        else if (!Details)
            HelpRow = {};
        const auto title = Page < Model.Pages().size() ? Model.Pages()[Page].title : Model.Text("menu.plugins");
        const auto help = HelpRow.plugin ? Model.Help(HelpRow) : std::string{};
        const auto details = "F1: " + Model.Text("menu.details");
        Invoke(movie, "_global.FFLabel", { Value(Model.Text("menu.plugins").c_str()) });
        Invoke(movie, "_global.FFBegin", { Value(title.c_str()), Value(static_cast<double>(Items.size())),
            Value(static_cast<double>(Selected)), Value(Details ? 1.0 : 0.0), Value(help.c_str()),
            Value(RestartPending() ? Model.Text("menu.restartwarning").c_str() : ""), Value(help.empty() ? "" : details.c_str()) });
        for (size_t i = 0; i < Items.size(); ++i)
            Invoke(movie, "_global.FFRow", { Value(static_cast<double>(i)), Value(Items[i].label.c_str()), Value(Items[i].value.c_str()) });
    }

    inline void Back(void* movie)
    {
        if (!Notice.empty())
            Notice.clear();
        else if (Details)
            Details = false;
        else if (Page != SIZE_MAX)
        {
            Model.PageShown(Page);
            Page = SIZE_MAX;
            Offset = Selected = 0;
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
            if (input == "up")
                Selected = (Selected + Items.size() - 1) % Items.size();
            else if (input == "down")
                Selected = (Selected + 1) % Items.size();
            else if (input.starts_with("select:"))
            {
                const auto index = strtoul(command + 7, nullptr, 10);
                if (index < Items.size())
                    Selected = index;
            }
            else if (input == "accept" || input == "left" || input == "right")
            {
                const auto item = Items[Selected];
                const int direction = input == "left" ? -1 : input == "right" ? 1 : 0;
                if (item.kind == Kind::Setting)
                    Model.Change(item.row, direction, false);
                else if (!direction)
                {
                    if (item.kind == Kind::Page)
                    {
                        Page = item.index;
                        Offset = Selected = 0;
                        Model.PageShown(Page);
                    }
                    else if (item.kind == Kind::Previous || item.kind == Kind::Next)
                    {
                        Offset = item.kind == Kind::Next ? Offset + 5 : Offset - 5;
                        Selected = 0;
                    }
                    else if (item.kind == Kind::Back)
                        Back(movie);
                }
            }
        }
        Render(movie);
    }

    inline int __fastcall Callback(void* self, void* edx, void* movie, const char* command, const char* argument)
    {
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
                return 0;
            }
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
    }
}
