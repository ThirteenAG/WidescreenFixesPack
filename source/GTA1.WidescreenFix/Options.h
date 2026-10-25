#pragma once
#include <functional>
#include "MenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

// In-game options. The native Options screen (sound, music, text speed and so
// on) gets a Plugins item below its own. It opens an extra frontend state drawn
// with the game's own font and highlight: the list of plugins, the fix first,
// then the other plugins with settings. The fix's page has its preset, sections
// that list every setting of this fix and those of the original setup program
// (language and the native keyboard controls), and Restore defaults. Up and Down select, Enter, Left
// and Right change a setting, Esc goes back; changes are saved at once and
// applied immediately where the game allows it. During gameplay Esc (or the
// controller's Back button) pauses the game and opens the same pages over the
// dimmed paused frame, with Resume and Quit Game; Quit Game hands over to the
// native quit prompt that Esc used to open.
namespace GTA1Options
{
    constexpr int State = 21;                      // the native frontend has states 0 to 20
    constexpr int Up = 1, Down = 2, Left = 4, Right = 8, Enter = 0x10, Escape = 0x20, ResumeAction = 0x40, Button = 0x200;

    struct Entry
    {
        std::string label;
        std::function<std::string()> value;      // empty: plain button
        std::function<void(int)> change;         // -1 left, +1 right, 0 enter
        bool restart = false;                    // read only when the game starts
        std::string id;
        std::function<std::string()> help;
        const RWGTAMenu::Row* modelRow{};
    };

    // native frontend
    inline int* MenuState = nullptr;
    constexpr int OptionsState = 9, NativeItems = 5;
    inline int* Selection = nullptr;               // selected menu item, from 1
    inline void (__cdecl* Sound)(int) = nullptr;
    inline int (__cdecl* Background)(int, int) = nullptr;
    inline int (__cdecl* Header)(int, const char*) = nullptr;
    inline int (__cdecl* Row)(int, int, const char*) = nullptr;
    inline int (__cdecl* Text)(int, int, int, const char*) = nullptr;
    inline int (__cdecl* TextWidth)(int, const char*) = nullptr;
    inline int (__cdecl* Present)() = nullptr;
    inline int* SmallFont = nullptr;
    inline int* HeaderFont = nullptr, * MarkerFont = nullptr;
    inline int* RawControls = nullptr, * Controls = nullptr;
    inline int (__cdecl* LoadControls)(int*) = nullptr;
    inline SafetyHookInline OptionsHook;
    inline bool Installed = false;

    // menu state
    inline std::vector<std::vector<Entry>> Sections;
    inline std::vector<std::string> Titles;
    inline std::vector<int> Parents;
    inline int Section = 0, Selected = 0, Scroll = 0;
    inline std::vector<int> Remembered;            // selection of each section
    inline int* CaptureTarget = nullptr;           // modern binding being changed
    inline const char* CaptureSection = nullptr, * CaptureKey = nullptr;
    inline int NativeCapture = -1;                 // native control being changed
    inline DWORD IgnoreUntil = 0;
    inline bool DrawExtra = false;                 // native options frame: add our item
    inline std::function<bool(int, int)> ApplyResolution;
    inline std::pair<int, int> Applying{};         // resolution to apply before the next frame
    inline int* ResX = nullptr, * ResY = nullptr;
    inline RWGTAMenu::Model Model;
    inline HMODULE HostModule = nullptr;
    inline size_t NativeSections = 0;
    inline std::vector<size_t> ModelPages;
    inline std::vector<Entry> NativeRoot;
    inline bool NativeRestartRequired{};
    inline std::pair<int, int> HelpSelection{ -1, -1 };
    inline ULONGLONG HelpStarted{};
    inline bool Overlay = false;                   // shown over paused gameplay

    inline int PauseSection = 0;
    inline int PluginsSection = 0;                 // the list of plugins
    inline bool PadResumeTaken = false;
    inline std::string LanguageTag();
    inline void ResumeMenu();
    inline void ResetPause();

    // cfg and game registry reads -------------------------------------------------------
    inline void Save(const char* section, const char* key, const std::string& value, bool nativeApplied = false)
    {
        auto id = std::string(section) + "." + key;
        auto previous = FixSettings().Wanted(id);
        if (FixSettings().Set(id, value, true) != ASI_SETTINGS_OK || asi_settings::text::IEquals(previous, FixSettings().Wanted(id)))
            return;
        for (auto& setting : FixSettings().GetDescription().settings)
        {
            if (setting.id == id && setting.apply == "restart" && !nativeApplied)
                NativeRestartRequired = true;
        }
    }
    inline void SaveInt(const char* section, const char* key, int value, bool nativeApplied = false) { Save(section, key, std::to_string(value), nativeApplied); }
    inline void SaveFloat(const char* section, const char* key, double value)
    {
        char text[32]{}; sprintf_s(text, "%.2f", value); Save(section, key, text);
    }
    inline void SaveKey(const char* section, const char* key, int value)
    {
        SaveInt(section, key, value);
    }
    inline const char* RegistryRoot = "Software\\DMA Design\\Grand Theft Auto";
    inline std::string RegistryPath(const char* key)
    {
        return key ? std::string(RegistryRoot) + "\\" + key : std::string(RegistryRoot);
    }
    inline DWORD ReadDword(const char* key, const char* name, DWORD fallback)
    {
        DWORD value = 0, size = sizeof(value);
        if (GameRegistry::Override(RegistryPath(key).c_str(), name, value)) return value;
        if (RegGetValueA(HKEY_CURRENT_USER, RegistryPath(key).c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS &&
            RegGetValueA(HKEY_LOCAL_MACHINE, RegistryPath(key).c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    // Settings of the original setup program. The game's per-user key is often
    // read-only for the user, so they are kept in the cfg ([SETUP]) as well and
    // served to the game in place of the registry values.
    inline std::string SetupKey(const char* name)
    {
        std::string key;
        for (auto c = name; *c; ++c) if (*c != ' ') key += *c;
        return key;
    }
    inline void WriteDword(const char* key, const char* name, DWORD value)
    {
        GameRegistry::SetOverride(RegistryPath(key).c_str(), name, value);
        SaveInt("SETUP", SetupKey(name).c_str(), int(value));
    }
    inline void LoadSetup()
    {
        SettingsReader ini("");
        auto load = [&](const char* key, const char* name)
        {
            int value = ini.ReadInteger("SETUP", SetupKey(name).c_str(), -1);
            if (value >= 0) GameRegistry::SetOverride(RegistryPath(key).c_str(), name, DWORD(value));
        };
        load(nullptr, "Language");
        for (int i = 0; i < 10; ++i) load("Controls", ("Control " + std::to_string(i)).c_str());
    }

    // labels -----------------------------------------------------------------
    inline std::string Upper(std::string text)
    {
        for (auto& c : text) c = char(toupper(uint8_t(c)));
        return text;
    }
    inline std::string Utf8(const wchar_t* text)
    {
        int size = WideCharToMultiByte(CP_UTF8, 0, text, int(wcslen(text)), nullptr, 0, nullptr, nullptr);
        std::string out(size, '\0');
        if (size) WideCharToMultiByte(CP_UTF8, 0, text, int(wcslen(text)), out.data(), size, nullptr, nullptr);
        return out;
    }
    inline std::string KeyName(int vk)
    {
        switch (vk)
        {
        case 0: return "None";
        case VK_LBUTTON: return "Mouse 1";
        case VK_RBUTTON: return "Mouse 2";
        case VK_MBUTTON: return "Mouse 3";
        case VK_XBUTTON1: return "Mouse 4";
        case VK_XBUTTON2: return "Mouse 5";
        }
        UINT scan = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
        switch (vk)
        {
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
        case VK_END: case VK_HOME: case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
        case VK_RCONTROL: case VK_RMENU:
            scan |= 0x100; break;
        }
        wchar_t name[64]{};
        if (GetKeyNameTextW(LONG(scan << 16), name, 64) > 0) return Utf8(name);
        char fallback[16]{}; sprintf_s(fallback, "Key %02X", vk);
        return fallback;
    }
    // DirectInput scan codes, as the native controls store them.
    inline std::string ScanName(int dik)
    {
        if (dik <= 0) return "None";
        if (dik > 255) return "Joystick";
        wchar_t name[64]{};
        LONG scan = LONG(dik & 0x7F) | (dik & 0x80 ? 0x100 : 0);
        if (GetKeyNameTextW(scan << 16, name, 64) > 0) return Utf8(name);
        char fallback[16]{}; sprintf_s(fallback, "Key %02X", dik);
        return fallback;
    }
    inline std::string OnOff(bool value) { return value ? "On" : "Off"; }
    inline std::string Number(double value)
    {
        char text[16]{}; sprintf_s(text, "%.2f", value); return text;
    }

    // entries ----------------------------------------------------------------
    inline Entry Toggle(const char* label, const char* section, const char* key, bool& value, bool restart = false)
    {
        return { label, [&value] { return OnOff(value); },
            [&value, section, key](int) { value = !value; SaveInt(section, key, value); }, restart, std::string(section) + "." + key };
    }
    template <typename T>
    inline Entry Choice(const char* label, const char* section, const char* key, T& value, std::vector<double> choices)
    {
        return { label, [&value] { return Number(double(value)); },
            [&value, section, key, choices](int step)
            {
                size_t index = 0;
                for (size_t i = 0; i < choices.size(); ++i)
                    if (std::fabs(choices[i] - double(value)) < std::fabs(choices[index] - double(value))) index = i;
                index = (index + choices.size() + (step < 0 ? -1 : 1)) % choices.size();
                value = T(choices[index]);
                SaveFloat(section, key, double(value));
            }, false, std::string(section) + "." + key };
    }
    inline Entry Range(const char* label, const char* section, const char* key, int& value, int minimum, int maximum, int step)
    {
        return { label, [&value] { return std::to_string(value); },
            [&value, section, key, minimum, maximum, step](int direction)
            {
                value = direction == 0 ? (value + step > maximum ? minimum : value + step) : std::clamp(value + direction * step, minimum, maximum);
                SaveInt(section, key, value);
            }, false, std::string(section) + "." + key };
    }
    inline Entry Key(const char* label, const char* section, const char* key, int& value)
    {
        return { label, [&value] { return CaptureTarget == &value ? Model.Text("menu.presskeyhint") : KeyName(value); },
            [&value, section, key](int step)
            {
                if (step != 0) { value = 0; SaveKey(section, key, 0); return; } // left/right clear the binding
                CaptureTarget = &value; CaptureSection = section; CaptureKey = key;
            }, false, std::string(section) + "." + key };
    }
    inline Entry Open(const char* label, int section)
    {
        return { label, nullptr, [section](int step)
        {
            if (step != 0) return;
            Parents.resize(std::max<size_t>(Parents.size(), section + 1));
            Parents[section] = Section; Section = section;
        } };
    }
    inline Entry Back() { return { RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.back"), nullptr, [](int step) { if (step == 0) Section = -1; } }; }

    // Native controls: registry "Control 0" to "Control 9", DirectInput scan codes.
    inline const int NativeDefaults[] = { 203, 205, 200, 208, 57, 28, 29, 45, 44, 15 };
    inline int NativeValue(int index)
    {
        return int(ReadDword("Controls", ("Control " + std::to_string(index)).c_str(), NativeDefaults[index]));
    }
    inline Entry NativeKey(const char* label, int index)
    {
        return { label, [index] { return NativeCapture == index ? Model.Text("menu.presskeyhint") : ScanName(NativeValue(index)); },
            [index](int step) { if (step == 0) NativeCapture = index; }, false, "SETUP.Control" + std::to_string(index) };
    }
    inline int VkToDik(int vk)
    {
        switch (vk)
        {
        case VK_UP: return 0xC8; case VK_DOWN: return 0xD0; case VK_LEFT: return 0xCB; case VK_RIGHT: return 0xCD;
        case VK_HOME: return 0xC7; case VK_END: return 0xCF; case VK_PRIOR: return 0xC9; case VK_NEXT: return 0xD1;
        case VK_INSERT: return 0xD2; case VK_DELETE: return 0xD3; case VK_RCONTROL: return 0x9D; case VK_RMENU: return 0xB8;
        case VK_DIVIDE: return 0xB5; case VK_CONTROL: return 0x1D; case VK_MENU: return 0x38; case VK_SHIFT: return 0x2A;
        }
        return int(MapVirtualKeyA(UINT(vk), MAPVK_VK_TO_VSC)) & 0x7F;
    }
    inline void SetNative(int index, int dik)
    {
        WriteDword("Controls", ("Control " + std::to_string(index)).c_str(), DWORD(dik));
        if (RawControls && Controls && LoadControls)
        {
            RawControls[index] = dik;
            LoadControls(Controls);  // the game's own conversion into its live key table
        }
    }

    // resolution -------------------------------------------------------------
    inline std::vector<std::pair<int, int>> Modes()
    {
        std::vector<std::pair<int, int>> modes;
        for (auto [width, height, refresh] : GetResolutionsList(false))
        {
            if (width >= 640 && height >= 480 && width <= 32767 && height <= 32767)
                modes.emplace_back(width, height);
        }
        auto [desktopX, desktopY] = GetDesktopRes();
        if (desktopX >= 640 && desktopY >= 480 && desktopX <= 32767 && desktopY <= 32767)
            modes.emplace_back(desktopX, desktopY);
        if (ResX && ResY && *ResX >= 640 && *ResY >= 480 && *ResX <= 32767 && *ResY <= 32767)
            modes.emplace_back(*ResX, *ResY);
        if (modes.empty())
            modes.emplace_back(640, 480);
        std::sort(modes.begin(), modes.end());
        modes.erase(std::unique(modes.begin(), modes.end()), modes.end());
        return modes;
    }
    inline std::pair<int, int> Pending{};
    inline Entry Resolution()
    {
        return { "Resolution", []
            {
                auto current = std::pair{ *ResX, *ResY };
                auto [x, y] = Pending.first ? Pending : current;
                char text[48]{};
                sprintf_s(text, Pending.first && Pending != current ? "%dx%d - Enter" : "%dx%d", x, y);
                return std::string(text);
            },
            [](int step)
            {
                auto current = std::pair{ *ResX, *ResY };
                if (step == 0)
                {
                    if (!Pending.first || Pending == current) return;
                    Applying = Pending; Pending = {};
                    return;
                }
                auto modes = Modes();
                if (modes.empty()) return;
                auto selected = Pending.first ? Pending : current;
                auto at = std::find(modes.begin(), modes.end(), selected);
                size_t index = at == modes.end() ? 0 : size_t(at - modes.begin());
                index = (index + modes.size() + (step < 0 ? -1 : 1)) % modes.size();
                Pending = modes[index];
            }, false, "MAIN.ResX" };
    }

    // screen -----------------------------------------------------------------
    inline std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        static const char* tags[] = { "en", "fr", "de", "it" };
        auto language = int(ReadDword(nullptr, "Language", 0));
        return language >= 0 && language < int(std::size(tags)) ? tags[language] : "en";
    }
    // GTA1's native 486670 reader accepts UTF-8 sequences of up to three bytes.
    // Keep mapped Latin characters; normalize punctuation and replace missing glyphs.
    inline std::string FontText(const std::string& text)
    {
        auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (!size) return {};
        std::wstring wide(size, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), size);
        std::wstring supported;
        for (size_t i = 0; i < wide.size(); ++i)
        {
            auto c = wide[i];
            if (c >= 0xD800 && c <= 0xDFFF)
            {
                supported += L'?';
                if (c <= 0xDBFF && i + 1 < wide.size() && wide[i + 1] >= 0xDC00 && wide[i + 1] <= 0xDFFF) ++i;
            }
            else if (c == L'’' || c == L'‘')
                supported += L'\'';
            else if (c == L'“' || c == L'”')
                supported += L'"';
            else if (c == L'—' || c == L'–')
                supported += L'-';
            else if (c == L'Œ' || c == L'œ')
                supported += c == L'Œ' ? L"OE" : L"oe";
            else if (c < 32 || c == 0xA0)
                supported += L' ';
            else if (c < 127 || std::wstring_view(L"ÀÁÂÄÆÇÈÉÊËÌÍÎÏÒÓÔÖÙÚÛÜßàáâäæçèéêëìíîïòóôöùúûü").find(c) != std::wstring_view::npos)
                supported += c;
            else
                supported += L'?';
        }
        size = WideCharToMultiByte(CP_UTF8, 0, supported.data(), int(supported.size()), nullptr, 0, nullptr, nullptr);
        std::string out(size, '\0');
        if (size) WideCharToMultiByte(CP_UTF8, 0, supported.data(), int(supported.size()), out.data(), size, nullptr, nullptr);
        return out;
    }
    inline std::string Fit(const std::string& text, int font, int width)
    {
        return GTA1MenuText::Fit(FontText(text), width, [font](const std::string& value)
        {
            return TextWidth(font, value.c_str());
        });
    }
    inline void RefreshModelRows()
    {
        for (size_t i = 0; i < ModelPages.size(); ++i)
        {
            auto& entries = Sections[NativeSections + i];
            entries.clear();
            for (auto row : Model.VisibleRows(ModelPages[i]))
            {
                entries.push_back({ Model.Label(*row), [row] { return Model.Enabled(*row, Overlay) ? Model.Value(*row) : Model.Text("menu.unavailable"); },
                    [row](int step) { Model.Change(*row, step, Overlay); }, false, {},
                    [row] { return Model.Help(*row); }, row });
            }
            entries.push_back(Back());
        }
    }
    inline void OpenModel()
    {
        if (!HostModule)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&HostModule), &HostModule);
        Model.OpenPlugins(HostModule, LanguageTag(), 8);
        Sections.resize(NativeSections); Titles.resize(NativeSections);
        Sections[0] = NativeRoot;
        ModelPages.clear();
        // The fix's own page keeps its native sections (resolution confirmation, keyboard capture); the shared model
        // gives its preset and Restore defaults rows and every other plugin's pages.
        auto row = [](const RWGTAMenu::Row* row) -> Entry
        {
            return { Model.Label(*row), [row] { return Model.Value(*row); }, [row](int step) { Model.Change(*row, step, Overlay); }, false, {},
                [row] { return Model.Help(*row); }, row };
        };
        std::string fixName = Model.Text("plugin.name");
        std::vector<Entry> plugins = { Open("", 0) };
        for (size_t page = 0; page < Model.Pages().size(); ++page)
        {
            auto& described = Model.Pages()[page];
            if (described.plugin->module == HostModule)
            {
                fixName = OtherPlugins::Name(*described.plugin);
                for (auto& model : described.rows)
                {
                    if (model.kind == RWGTAMenu::RowKind::Preset) Sections[0].insert(Sections[0].begin(), row(&model));
                    else if (model.kind == RWGTAMenu::RowKind::RestoreDefaults) Sections[0].insert(Sections[0].end() - 1, row(&model));
                }
                continue;
            }
            int section = int(Sections.size());
            ModelPages.push_back(page);
            Sections.emplace_back(); Titles.push_back(described.title);
            plugins.push_back({ described.title, nullptr, [page, section](int step)
                {
                    if (step != 0) return;
                    Parents.resize(Sections.size()); Parents[section] = Section;
                    Model.PageShown(page); Section = section;
                } });
        }
        for (size_t section = 0; section < NativeSections; ++section)
        {
            for (auto& entry : Sections[section])
            {
                if (entry.id.empty())
                    continue;
                for (auto& setting : FixSettings().GetDescription().settings)
                {
                    if (setting.id != entry.id)
                        continue;
                    auto resolve = [](const std::string& token)
                    {
                        return token.starts_with("@") ? Model.Text(token.substr(1)) : token;
                    };
                    if (entry.id != "MAIN.ResX")
                        entry.label = resolve(setting.label);
                    else
                        entry.label = Model.Text("menu.resolution");
                    entry.restart = setting.apply == "restart" && entry.id != "MAIN.ResX" && entry.id != "MAIN.WindowMode";
                    entry.help = [description = setting.description, resolve]
                    {
                        return resolve(description);
                    };
                    break;
                }
                for (auto& page : Model.Pages())
                {
                    if (page.plugin->module != HostModule)
                        continue;
                    for (auto& row : page.rows)
                    {
                        if (!row.setting || row.setting->id != entry.id)
                            continue;
                        entry.modelRow = &row;
                        if (entry.id != "MAIN.ResX" && !entry.id.ends_with("Key"))
                            entry.value = [model = &row]
                            {
                                return Model.Value(*model);
                            };
                    }
                }
            }
        }
        const char* sectionKeys[] = { "", "page.display", "page.camera", "page.controls", "page.setup", "menu.modernkeys", "menu.morekeys", "menu.classickeys" };
        for (size_t i = 1; i < std::size(sectionKeys) && i < Titles.size(); ++i)
            Titles[i] = Model.Text(sectionKeys[i]);
        size_t rootSection{};
        for (auto& entry : Sections[0])
        {
            if (!entry.modelRow && !entry.value && rootSection < 4)
                entry.label = Titles[++rootSection];
        }
        for (size_t i = 0; i < 3 && 5 + i < Sections[3].size(); ++i)
            Sections[3][5 + i].label = Titles[5 + i];
        HelpSelection = { -1, -1 };
        plugins[0].label = fixName;
        plugins.push_back(Back());
        Sections[PluginsSection] = std::move(plugins);
        RefreshModelRows();
        Titles[0] = fixName;
        Titles[PluginsSection] = Model.Text("menu.plugins");
        if (Sections.size() > size_t(PauseSection) && Sections[PauseSection].size() == 3)
        {
            Titles[PauseSection] = Model.Text("menu.pause");
            Sections[PauseSection][0].label = Model.Text("menu.resume");
            Sections[PauseSection][1].label = Model.Text("menu.plugins");
        }
    }
    inline int Opened = 0;                         // section opened from the native screen
    inline bool OverlayClosed = false;             // the paused frame must be restored
    inline void Begin(int section)
    {
        OpenModel();
        Opened = Section = section; Selected = 0; Scroll = 0; Pending = {};
        CaptureTarget = nullptr; NativeCapture = -1;
        Remembered.assign(Sections.size(), 0);
        IgnoreUntil = GetTickCount() + 200;
    }
    inline void Open(int section)
    {
        Begin(section);
        *MenuState = State;
    }
    inline void Close()
    {
        Pending = {}; CaptureTarget = nullptr; NativeCapture = -1;
        Model.Close();
        if (Overlay) { Overlay = false; OverlayClosed = true; return; }
        *MenuState = OptionsState;
        *Selection = NativeItems + 1;
    }
    // The frontend's sounds are unloaded during gameplay.
    inline void Click(int sound) { if (Sound && !Overlay) Sound(sound); }

    inline void Change(int step)
    {
        auto& entries = Sections[Section];
        if (Selected >= int(entries.size())) return;
        auto& entry = entries[Selected];
        if (entry.modelRow && !Model.Enabled(*entry.modelRow, Overlay)) return;
        if (step != 0 && !entry.value) return;    // buttons only react to Enter
        int before = Section;
        if (entry.change) entry.change(step);
        if (Section == -1)                         // Back
        {
            if (before == Opened) { Section = Opened; Close(); return; }
            Section = before < int(Parents.size()) ? Parents[before] : 0;
        }
        if (Section != before)
        {
            Remembered[before] = Selected;
            Selected = Remembered[Section];
            Scroll = 0;
        }
    }

    inline bool Showing() { return Installed && MenuState && *MenuState == State && !Sections.empty(); }

    // Before the frontend draws: work that must not happen mid-frame.
    inline void BeforeFrame()
    {
        if (!Applying.first || !ApplyResolution) return;
        auto [x, y] = Applying; Applying = {};
        *ResX = x; *ResY = y;
        bool applied = ApplyResolution(x, y);
        SaveInt("MAIN", "ResX", x, applied); SaveInt("MAIN", "ResY", y, applied);
    }

    // Menu input, as frontend direction and action bits.
    inline void Handle(int input)
    {
        if (Overlay && (input & ResumeAction)) { ResumeMenu(); return; }
        if (RWGTAMenu::Model::Changed().exchange(false)) RefreshModelRows();
        Selected = std::clamp(Selected, 0, int(Sections[Section].size()) - 1);
        if (CaptureTarget || NativeCapture >= 0 || int32_t(GetTickCount() - IgnoreUntil) < 0) input = 0;
        const int count = int(Sections[Section].size());
        if (input & Escape)
        {
            Click(4);
            if (Section == Opened) { Close(); return; }
            Remembered[Section] = Selected;
            Section = Section < int(Parents.size()) ? Parents[Section] : 0;
            Selected = Remembered[Section]; Scroll = 0;
            input = 0;
        }
        if (input & Up) { Selected = (Selected + count - 1) % count; Click(7); }
        if (input & Down) { Selected = (Selected + 1) % count; Click(8); }
        if (input & (Enter | Button)) { Click(2); Change(0); }
        else if (input & Left) { Click(3); Change(-1); }
        else if (input & Right) { Click(3); Change(1); }
    }

    // The current page in the 640x480 frontend layout.
    inline void DrawPage()
    {
        auto& shown = Sections[Section];
        const int font = *SmallFont;
        const int height = std::max(8, int(reinterpret_cast<uint8_t*>(font)[1]));
        Selected = std::clamp(Selected, 0, int(shown.size()) - 1);
        auto selection = std::pair{ Section, Selected };
        if (HelpSelection != selection)
        {
            HelpSelection = selection;
            HelpStarted = GetTickCount64();
        }
        auto& selected = shown[Selected];
        std::string description;
        if (NativeRestartRequired || Model.RestartRequired() || selected.restart)
            description = Model.Text("menu.restartwarning") + " ";
        if (selected.help)
            description += selected.help();
        const int helpTop = 480 - height * 3 - 10;
        const int bottom = description.empty() ? 480 - height * 2 : helpTop - 10;
        const int visible = std::max(1, (bottom - 224) / height);
        if (Selected < Scroll) Scroll = Selected;
        if (Selected >= Scroll + visible) Scroll = Selected - visible + 1;
        Scroll = std::clamp(Scroll, 0, std::max(0, int(shown.size()) - visible));
        auto title = Fit(Titles[Section], HeaderFont ? *HeaderFont : font, 600);
        Header(176, title.c_str());
        auto measure = [font](const std::string& text)
        {
            return TextWidth(font, text.c_str());
        };
        for (int k = Scroll; k < std::min(int(shown.size()), Scroll + visible); ++k)
        {
            auto& entry = shown[k];
            std::string value;
            if (entry.value)
                value = entry.modelRow && !Model.Enabled(*entry.modelRow, Overlay) ? Model.Text("menu.unavailable") : entry.value();
            auto text = GTA1MenuText::Join(FontText(entry.label), FontText(value), 424, measure);
            Row(k - Scroll + 1, Selected - Scroll + 1, text.c_str());
        }
        auto lines = GTA1MenuText::Wrap(FontText(description), 600, measure);
        auto pages = (lines.size() + 1) / 2;
        size_t part = pages ? size_t((GetTickCount64() - HelpStarted) / 12000) % pages : 0;
        for (size_t i = 0; i < 2 && part * 2 + i < lines.size(); ++i)
            Text(font, 20, helpTop + int(i) * height, lines[part * 2 + i].c_str());
        bool capture = CaptureTarget || NativeCapture >= 0;
        auto left = FontText(Model.Text(capture ? "menu.cancelhint" : "menu.backhint"));
        auto right = FontText(Model.Text(capture ? "menu.presskeyhint" : "menu.changehint"));
        Text(font, 0, 480 - height, left.c_str());
        Text(font, 640 - TextWidth(font, right.c_str()), 480 - height, right.c_str());
        if (pages > 1 || int(shown.size()) > visible)
        {
            auto counter = pages > 1 ? std::to_string(part + 1) + "/" + std::to_string(pages) : std::to_string(Selected + 1) + "/" + std::to_string(shown.size());
            auto width = TextWidth(font, counter.c_str());
            auto begin = TextWidth(font, left.c_str()) + 10;
            auto end = 630 - TextWidth(font, right.c_str());
            if (end - begin >= width)
                Text(font, std::clamp(320 - width / 2, begin, end - width), 480 - height, counter.c_str());
        }
    }

    inline void Frame(int input)
    {
        Handle(input);
        if (*MenuState != State) return;
        Background(1, 0);
        DrawPage();
        Present();
    }

    // A window message while a binding is being changed. Returns true when consumed.
    inline bool KeyMessage(UINT message, WPARAM key, LPARAM flags)
    {
        if (!CaptureTarget && NativeCapture < 0) return false;
        int vk = 0;
        switch (message)
        {
        case WM_KEYDOWN: case WM_SYSKEYDOWN: vk = int(key); break;
        case WM_LBUTTONDOWN: vk = VK_LBUTTON; break;
        case WM_RBUTTONDOWN: vk = VK_RBUTTON; break;
        case WM_MBUTTONDOWN: vk = VK_MBUTTON; break;
        case WM_XBUTTONDOWN: vk = GET_XBUTTON_WPARAM(key) == XBUTTON1 ? VK_XBUTTON1 : VK_XBUTTON2; break;
        case WM_KEYUP: case WM_SYSKEYUP: case WM_CHAR: case WM_SYSCHAR: return true;
        default: return false;
        }
        if (NativeCapture >= 0)
        {
            bool keyboard = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            if (keyboard && vk != VK_ESCAPE)
            {
                // The scan code from the message when present (it tells keypad keys
                // from the arrow block), otherwise from the virtual key.
                int scan = int((flags >> 16) & 0x7F);
                scan = scan ? scan | ((flags & (1 << 24)) ? 0x80 : 0) : VkToDik(vk);
                if (scan) SetNative(NativeCapture, scan);
            }
            if (keyboard) NativeCapture = -1;      // mouse buttons cannot be native controls
            else return true;
        }
        else
        {
            if (vk != VK_ESCAPE)
            {
                *CaptureTarget = vk;
                SaveKey(CaptureSection, CaptureKey, vk);
            }
            CaptureTarget = nullptr;
        }
        IgnoreUntil = GetTickCount() + 300;
        return true;
    }

    // gameplay overlay ---------------------------------------------------------
    inline std::atomic<int> OverlayInput = 0;      // direction and action bits from the window
    inline std::atomic<bool> OpenRequested = false;
    inline DWORD OpenRequestTime = 0;
    inline bool PausedByMenu = false;              // resume the game when the menu closes
    inline std::atomic<bool> QueuedEscape = false; // native Esc for the quit prompt
    inline std::atomic<bool> EscapeTaken = false;  // an Esc that opened the menu
    inline bool (__cdecl* ModeListOpen)() = nullptr;
    inline bool (__cdecl* QuitPromptOpen)() = nullptr;
    inline const char* FontFiles[3]{};             // small text, header, highlight marker
    inline int OwnFonts[3]{};
    inline char (__cdecl* LoadFont)(int*, const char*, int, int) = nullptr;
    inline uint8_t** PixelBase = nullptr;          // where the frontend text routines draw
    inline int* PixelPitch = nullptr;              // in pixels
    inline bool OverlayStride = false;             // the glyph blitter draws into the overlay canvas
    inline void (__cdecl* Redraw)() = nullptr;     // requests a world redraw while paused
    inline std::vector<uint16_t> Canvas, Snapshot;
    inline int SnapshotWidth = 0, SnapshotHeight = 0, SnapshotBits = 0;
    constexpr uint16_t Transparent = 0x0821;

    inline bool OverlayActive() { return Overlay; }
    inline bool QuitPromptActive() { return QuitPromptOpen && QuitPromptOpen(); }
    inline bool Paused() { return GTA1Input::IsPaused() && GTA1Input::Available(); }
    inline bool TakeQueuedEscape() { return QueuedEscape.exchange(false); }
    inline bool TakeEscape() { return EscapeTaken.exchange(false); }
    inline bool SuppressNativePadPause()
    {
        if (!InputDevices::Button(XINPUT_GAMEPAD_START)) PadResumeTaken = false;
        return PadResumeTaken;
    }

    // Pauses the game when needed; the menu opens once it is paused.
    inline void RequestOpen()
    {
        if (Overlay || OpenRequested || !GTA1Input::Available() || QuitPromptActive() || (ModeListOpen && ModeListOpen())) return;
        PausedByMenu = !GTA1Input::IsPaused();
        if (PausedByMenu) GTA1Presentation::PendingKey = 64;   // the native pause key
        OpenRequested = true; OpenRequestTime = GetTickCount();
    }
    inline void Resume()
    {
        if (PausedByMenu && GTA1Input::IsPaused()) GTA1Presentation::PendingKey = 64;
        PausedByMenu = false;
    }

    // Explicit Resume/P/Start must also resume a game already paused with F6.
    // Escape/back still preserves the pause state that preceded this menu.
    inline void ResumeMenu()
    {
        PausedByMenu = true;
        Close();
    }
    inline void ResetPause()
    {
        Overlay = false; OverlayClosed = false; OpenRequested = false;
        PausedByMenu = false; OverlayInput = 0; QueuedEscape = false; EscapeTaken = false;
        CaptureTarget = nullptr; NativeCapture = -1; Pending = {}; Applying = {};
        Snapshot.clear();
        Model.Close();
    }

    // Window messages during gameplay. Returns true when consumed.
    inline bool OverlayMessage(UINT message, WPARAM key)
    {
        if (!Installed || InputDevices::Frontend || !LoadFont) return false;
        if (!Overlay)
        {
            // Esc opens the menu, unless a native prompt or list is waiting for it.
            if (message == WM_KEYDOWN && key == VK_ESCAPE && GTA1Input::Available() &&
                !(ModeListOpen && ModeListOpen()) && !(QuitPromptOpen && QuitPromptOpen()))
            {
                RequestOpen(); EscapeTaken = true;
                return true;
            }
            if ((message == WM_KEYUP || message == WM_CHAR) && key == VK_ESCAPE && OpenRequested) return true;
            return false;
        }
        switch (message)
        {
        case WM_KEYDOWN: case WM_SYSKEYDOWN:
            if ((InputDevices::ModernControls && key == InputDevices::PauseKey) || key == VK_F6)
            {
                OverlayInput |= ResumeAction; return true;
            }
            switch (key)
            {
            case VK_UP: case 'W': OverlayInput |= Up; break;
            case VK_DOWN: case 'S': OverlayInput |= Down; break;
            case VK_LEFT: case 'A': OverlayInput |= Left; break;
            case VK_RIGHT: case 'D': OverlayInput |= Right; break;
            case VK_RETURN: case VK_SPACE: OverlayInput |= Enter; break;
            case VK_ESCAPE: case VK_BACK: OverlayInput |= Escape; break;
            }
            return true;
        case WM_KEYUP: case WM_SYSKEYUP: case WM_CHAR: case WM_SYSCHAR:
            return true;
        }
        return false;
    }

    // Controller buttons, with repeat for held directions.
    inline int PadInput(bool& openPressed)
    {
        using namespace InputDevices;
        static bool held[8]{};
        static DWORD repeat[4]{};
        Poll();
        auto [x, y] = Stick();
        bool down[] = { y < -0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_UP), y > 0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_DOWN),
            x < -0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_LEFT), x > 0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_RIGHT),
            InputDevices::Button(XINPUT_GAMEPAD_A), InputDevices::Button(XINPUT_GAMEPAD_B), InputDevices::Button(XINPUT_GAMEPAD_BACK), InputDevices::Button(XINPUT_GAMEPAD_START) };
        constexpr int bits[] = { Up, Down, Left, Right, Enter, Escape, 0, ResumeAction };
        if (!down[7]) PadResumeTaken = false;
        int input = 0;
        DWORD now = GetTickCount();
        for (int i = 0; i < 8; ++i)
        {
            bool pressed = down[i] && !held[i];
            if (i < 4)
            {
                if (pressed) repeat[i] = now + 400;
                else if (down[i] && int32_t(now - repeat[i]) >= 0) { pressed = true; repeat[i] = now + 120; }
            }
            if (pressed && (i != 7 || Overlay)) input |= bits[i];
            if (pressed && i == 7 && Overlay) PadResumeTaken = true;
            if (i == 6) openPressed = pressed;
            held[i] = down[i];
        }
        return input;
    }

    inline bool LoadFonts()
    {
        if (OwnFonts[0] && OwnFonts[1] && OwnFonts[2]) return true;
        if (!LoadFont || !FontFiles[0] || !FontFiles[1] || !FontFiles[2]) return false;
        const int first[] = { 33, 33, 1 };
        for (int i = 0; i < 3; ++i) if (!OwnFonts[i]) LoadFont(&OwnFonts[i], FontFiles[i], first[i], 1);
        return OwnFonts[0] && OwnFonts[1] && OwnFonts[2];
    }

    // Draws with the frontend routines into a 640x480 canvas.
    template <typename F>
    inline void DrawCanvas(F&& draw)
    {
        Canvas.assign(640 * 480, Transparent);
        int fonts[] = { *SmallFont, *HeaderFont, *MarkerFont };
        auto base = *PixelBase; int pitch = *PixelPitch;
        *SmallFont = OwnFonts[0]; *HeaderFont = OwnFonts[1]; *MarkerFont = OwnFonts[2];
        *PixelBase = reinterpret_cast<uint8_t*>(Canvas.data()); *PixelPitch = 640;
        OverlayStride = true;
        draw();
        OverlayStride = false;
        *PixelBase = base; *PixelPitch = pitch;
        *SmallFont = fonts[0]; *HeaderFont = fonts[1]; *MarkerFont = fonts[2];
    }

    // Scales the canvas onto the frame like the frontend, dimming the frame around it.
    inline void Composite(uint8_t* pixels, int width, int height, int pitch, int bits, bool dim)
    {
        const uint16_t mask = bits == 15 ? 0x3DEF : 0x7BEF;
        double scale = std::min(width / 640.0, height / 480.0);
        int w = int(std::lround(640 * scale)), h = int(std::lround(480 * scale));
        int ox = (width - w) / 2, oy = (height - h) / 2;
        for (int y = 0; y < height; ++y)
        {
            auto line = reinterpret_cast<uint16_t*>(pixels + size_t(y) * pitch);
            bool inside = y >= oy && y < oy + h;
            auto source = inside ? Canvas.data() + size_t((y - oy) * 480 / h) * 640 : nullptr;
            for (int x = 0; x < width; ++x)
            {
                uint16_t value = Transparent;
                if (inside && x >= ox && x < ox + w) value = source[(x - ox) * 640 / w];
                if (value != Transparent) line[x] = value;
                else if (dim) line[x] = uint16_t((line[x] >> 1) & mask);
            }
        }
    }

    // Only an unpaused frame is a clean background: the native pause heading
    // and target score have already been drawn by GameplayPresent's caller.
    inline void CaptureSnapshot(const uint8_t* pixels, int width, int height, int pitch, int bits)
    {
        SnapshotWidth = width; SnapshotHeight = height; SnapshotBits = bits;
        Snapshot.resize(size_t(width) * height);
        for (int y = 0; y < height; ++y)
            memcpy(Snapshot.data() + size_t(y) * width, pixels + size_t(y) * pitch, size_t(width) * 2);
    }

    inline void RestoreSnapshot(uint8_t* pixels, int width, int height, int pitch, int bits)
    {
        if (SnapshotWidth == width && SnapshotHeight == height && SnapshotBits == bits)
        {
            for (int y = 0; y < height; ++y)
                memcpy(pixels + size_t(y) * pitch, Snapshot.data() + size_t(y) * width, size_t(width) * 2);
            return;
        }
        // A resolution change while paused must not replace the cache with a
        // native paused frame (or with our own menu). Scale its clean copy.
        for (int y = 0; y < height; ++y)
        {
            auto line = reinterpret_cast<uint16_t*>(pixels + size_t(y) * pitch);
            auto source = Snapshot.data() + size_t(y) * SnapshotHeight / height * SnapshotWidth;
            for (int x = 0; x < width; ++x)
            {
                uint16_t value = source[size_t(x) * SnapshotWidth / width];
                if (SnapshotBits != bits)
                {
                    if (bits == 16)
                        value = uint16_t(((value & 0x7C00) << 1) | ((value & 0x03E0) << 1) | ((value & 0x0200) >> 4) | (value & 0x001F));
                    else
                        value = uint16_t(((value & 0xF800) >> 1) | ((value & 0x07C0) >> 1) | (value & 0x001F));
                }
                line[x] = value;
            }
        }
    }

    // Before a gameplay frame is presented.
    inline void GameplayPresent(uint8_t* pixels, int width, int height, int pitch, int bits)
    {
        if (!Installed || !LoadFont || !PixelBase || !pixels || width <= 0 || height <= 0 ||
            (bits != 15 && bits != 16) || pitch <= 0 || size_t(pitch) < size_t(width) * 2 ||
            size_t(width) > Snapshot.max_size() / size_t(height)) return;
        bool openPad = false;
        int pad = PadInput(openPad);
        if (openPad && !Overlay) RequestOpen();
        bool paused = Paused();
        if (OpenRequested && int32_t(GetTickCount() - OpenRequestTime) > 1500) { OpenRequested = false; PausedByMenu = false; }
        if (!paused)
        {
            if (Overlay) ResetPause();
            OverlayInput = 0;
            if (GTA1Input::Available() && !QuitPromptActive() && !(ModeListOpen && ModeListOpen()))
                CaptureSnapshot(pixels, width, height, pitch, bits);
            else Snapshot.clear();
            return;
        }
        if (!Overlay && OpenRequested && Snapshot.empty())
        {
            // No clean world frame (for example, an initial paused state): keep
            // the stock Esc path rather than capture native pause text.
            OpenRequested = false; EscapeTaken = false;
            Resume(); QueuedEscape = true;
            return;
        }
        if (!Overlay && OpenRequested && LoadFonts())
        {
            OpenRequested = false; EscapeTaken = false;
            Begin(PauseSection); Overlay = true; OverlayInput = 0;
            pad = 0;
        }
        if (Overlay)
        {
            RestoreSnapshot(pixels, width, height, pitch, bits);
            Handle(OverlayInput.exchange(0) | pad);
            if (!Overlay) { OverlayClosed = false; Resume(); if (Redraw) Redraw(); return; }
            DrawCanvas([] { DrawPage(); });
            Composite(pixels, width, height, pitch, bits, true);
        }
    }

    // Each simulation tick, outside rendering.
    inline void Tick()
    {
        if (!InputDevices::Frontend && Applying.first) BeforeFrame();
    }

    // Native options screen: its five items, then one for each of our sections.
    inline int __cdecl NativeOptions(int input)
    {
        int& selected = *Selection;
        const int last = NativeItems + 1;
        if (input & Up)
        {
            if (selected == 1) { selected = last; Sound(7); input &= ~Up; }
            else if (selected > NativeItems) { --selected; Sound(7); input &= ~Up; }
        }
        if (input & Down)
        {
            if (selected >= NativeItems) { selected = selected >= last ? 1 : selected + 1; Sound(8); input &= ~Down; }
        }
        bool open = false;
        if (selected > NativeItems)
        {
            input &= ~(Left | Right);
            if (input & (Enter | Button)) { Sound(2); input &= ~(Enter | Button); open = true; }
        }
        DrawExtra = true;
        int result = OptionsHook.ccall<int>(input);
        DrawExtra = false;
        if (open && *MenuState == OptionsState) Open(PluginsSection);
        return result;
    }
    // Before a frontend frame is presented.
    inline void BeforePresent()
    {
        Snapshot.clear(); // never reuse a previous game scene after returning to the frontend
        if (!DrawExtra || !Row) return;
        auto label = Fit(RWGTAMenu::PluginText(FixSettings(), LanguageTag(), "menu.plugins"), *SmallFont, 424);
        Row(NativeItems + 1, *Selection, label.c_str());
    }

    // Settings that are only read when the game starts.
    inline bool SoftwareBuffer = true, FramePacing = true, Windowed = true;
    inline int Language = 0;

    inline void Build(int& resX, int& resY)
    {
        using namespace InputDevices;
        ResX = &resX; ResY = &resY;
        LoadSetup();
        SettingsReader ini("");
        SoftwareBuffer = ini.ReadBoolean("MAIN", "SoftwareBuffer", true);
        Windowed = ini.ReadInteger("MAIN", "WindowMode", 0) != 0;
        FramePacing = ini.ReadBoolean("MAIN", "FixFramePacing", true);
        Language = int(ReadDword(nullptr, "Language", 0));
        enum { Root, Display, Camera, Controls, Game, ModernKeys, MoreKeys, ClassicKeys, Pause, Plugins, Count };
        Sections.assign(Count, {});
        Titles = { "WIDESCREEN FIX", "DISPLAY", "CAMERA", "CONTROLS", "GAME", "MODERN KEYS", "MORE MODERN KEYS", "CLASSIC KEYS", "PAUSED", "PLUGINS" };
        Sections[Root] = { Open("DISPLAY", Display), Open("CAMERA", Camera), Open("CONTROLS", Controls), Open("GAME", Game), Back() };
        PauseSection = Pause;
        PluginsSection = Plugins;
        Sections[Plugins] = { Open("WIDESCREEN FIX", Root), Back() };
        Sections[Pause] = {
            { "RESUME", nullptr, [](int step) { if (step == 0) ResumeMenu(); } },
            Open("PLUGINS", Plugins),
            { "QUIT GAME", nullptr, [](int step) { if (step == 0) { Close(); QueuedEscape = true; } } } };
        Sections[Display] = {
            Resolution(),
            Range("HUD SAFE AREA", "MAIN", "HudSafeArea", GTA1Hud::SafeArea, 0, 32, 4),
            { "HUD ASPECT RATIO", [] { return Upper(FixSettings().String("MAIN.HudAspectRatioConstraint")); },
                [](int step)
                {
                    static const char* values[] = { "Auto", "4:3", "16:9", "21:9", "32:9" };
                    auto& current = FixSettings().String("MAIN.HudAspectRatioConstraint");
                    int index = 0;
                    for (int i = 0; i < int(std::size(values)); ++i) if (_stricmp(current.c_str(), values[i]) == 0) index = i;
                    index = (index + int(std::size(values)) + (step < 0 ? -1 : 1)) % int(std::size(values));
                    FixSettings().Set("MAIN.HudAspectRatioConstraint", values[index]);
                }, false, "MAIN.HudAspectRatioConstraint" },
            // Window: the resolution's size, centred. Borderless: covers the monitor, the
            // picture scaled to fit. Exclusive: the game's own fullscreen, changing the
            // display mode. Switching to or from exclusive needs a restart.
            { "DISPLAY MODE", []
                {
                    std::string text = !Windowed ? "Exclusive Fullscreen" : GTA1Window::Fullscreen ? "Borderless Fullscreen" : "Window";
                    if (Windowed != GTA1Window::Enabled) text += " - Restart";
                    return text;
                },
                [](int step)
                {
                    int mode = !Windowed ? 2 : GTA1Window::Fullscreen ? 1 : 0;
                    mode = (mode + (step < 0 ? 2 : 1)) % 3;
                    Windowed = mode != 2;
                    if (mode != 2 && GTA1Window::Fullscreen != (mode == 1))
                    {
                        if (GTA1Window::Enabled) GTA1Window::ToggleFullscreen();
                        else GTA1Window::Fullscreen = mode == 1;
                    }
                    SaveInt("MAIN", "WindowMode", mode == 2 ? 0 : mode == 1 ? 2 : 1, Windowed == GTA1Window::Enabled);
                }, false, "MAIN.WindowMode" },
            Toggle("SOFTWARE BUFFER", "MAIN", "SoftwareBuffer", SoftwareBuffer, true),
            Toggle("FRAME PACING", "MAIN", "FixFramePacing", FramePacing, true),
            Back() };
        Sections[Camera] = {
            Toggle("SMOOTH FOLLOW", "CAMERA", "SmoothFollow", GTA1Camera::Enabled),
            Choice("FOLLOW TIME", "CAMERA", "FollowTime", GTA1Camera::FollowSeconds, { 0.0, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.75, 1.0 }),
            Choice("HEIGHT TIME", "CAMERA", "HeightTime", GTA1Camera::HeightSeconds, { 0.0, 0.1, 0.2, 0.25, 0.35, 0.5, 0.75, 1.0, 1.5 }),
            Choice("FIELD OF VIEW", "MAIN", "FOVFactor", GTA1Camera::Zoom, { 0.5, 0.75, 1.0, 1.25, 1.5, 1.75, 2.0 }),
            Toggle("ROTATE CAMERA", "CAMERA", "RotateWithPlayer", GTA1Rotation::Enabled),
            Toggle("ROTATE IN VEHICLES", "CAMERA", "RotateInVehicles", GTA1Rotation::RotateVehicle),
            Toggle("ROTATE ON FOOT", "CAMERA", "RotateOnFoot", GTA1Rotation::RotateFoot),
            Choice("ROTATION SMOOTHING", "CAMERA", "RotationSmoothing", GTA1Rotation::Smoothing, { 0.0, 0.1, 0.2, 0.25, 0.35, 0.5, 0.75, 1.0 }),
            Key("ROTATION KEY", "CAMERA", "ToggleRotationKey", GTA1Rotation::ToggleKey),
            Back() };
        Sections[Controls] = {
            Toggle("MODERN CONTROLS", "INPUT", "ModernControls", ModernControls),
            Toggle("GAMEPAD", "INPUT", "Gamepad", Gamepad),
            Toggle("STICK THROTTLE", "INPUT", "StickThrottle", StickThrottle),
            Toggle("MOUSE AIM", "INPUT", "MouseAim", GTA1Input::MouseAim),
            Choice("STICK DEADZONE", "INPUT", "StickDeadzone", Deadzone, { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5 }),
            Open("MODERN KEYS", ModernKeys), Open("MORE MODERN KEYS", MoreKeys), Open("CLASSIC KEYS", ClassicKeys),
            Back() };
        Sections[ModernKeys] = {
            Key("MOVE UP", "INPUT", "MoveUpKey", MoveUpKey), Key("MOVE DOWN", "INPUT", "MoveDownKey", MoveDownKey),
            Key("MOVE LEFT", "INPUT", "MoveLeftKey", MoveLeftKey), Key("MOVE RIGHT", "INPUT", "MoveRightKey", MoveRightKey),
            Key("FIRE", "INPUT", "FireKey", FireKey), Key("ENTER VEHICLE", "INPUT", "EnterVehicleKey", EnterVehicleKey),
            Key("JUMP", "INPUT", "JumpKey", JumpKey), Key("SPECIAL", "INPUT", "SpecialKey", SpecialKey),
            Back() };
        Sections[MoreKeys] = {
            Key("PREVIOUS WEAPON", "INPUT", "PreviousWeaponKey", PreviousWeaponKey),
            Key("NEXT WEAPON", "INPUT", "NextWeaponKey", NextWeaponKey),
            Key("PAUSE", "INPUT", "PauseKey", PauseKey),
            Back() };
        // Registry index: left, right, up, down, fire, jump, enter, previous, next weapon, special.
        Sections[ClassicKeys] = {
            NativeKey("LEFT", 0), NativeKey("RIGHT", 1), NativeKey("UP", 2), NativeKey("DOWN", 3),
            NativeKey("FIRE", 4), NativeKey("JUMP", 5), NativeKey("ENTER VEHICLE", 6),
            NativeKey("PREVIOUS WEAPON", 7), NativeKey("NEXT WEAPON", 8), NativeKey("SPECIAL", 9),
            Back() };
        Sections[Game] = {
            { "LANGUAGE", []
                {
                    static const char* names[] = { "English", "French", "German", "Italian" };
                    return std::string(names[std::clamp(Language, 0, 3)]);
                },
                [](int step) { Language = (Language + (step < 0 ? 3 : 1)) % 4; WriteDword(nullptr, "Language", DWORD(Language)); }, true, "SETUP.Language" },
            Back() };
        // The native menus write words in title case.
        auto title = [](std::string text)
        {
            bool start = true;
            for (size_t i = 0; i < text.size(); ++i)
            {
                if (start && text.compare(i, 3, "HUD") == 0) { i += 2; start = false; continue; }
                if (!start) text[i] = char(tolower(uint8_t(text[i])));
                start = text[i] == ' ';
            }
            return text;
        };
        for (auto& section : Sections) for (auto& entry : section) entry.label = title(entry.label);
        for (auto& text : Titles) text = title(text);
        NativeSections = Sections.size();
        NativeRoot = Sections[Root];
    }

    inline void Install(int* menuState)
    {
        using GTA1Build::Find, GTA1Build::Off, GTA1Build::Kind;
        const auto kind = GTA1Build::Current;
        MenuState = menuState;
        // The native Options screen, its list rows, and the Error screen, which calls
        // every frontend drawing routine the options pages need.
        auto options = Find("53 8A 5C 24 08 56 57 33 FF F6 C3 01 74 24 6A 07 E8 ? ? ? ? A1",
            "53 56 57 33 F6 8B 7C 24 10 F7 C7 01 00 00 00 74 24 6A 07 E8 ? ? ? ? 83 C4 04 A1",
            "53 8A 5C 24 08 56 33 F6 F6 C3 01 74 24 6A 07 E8 ? ? ? ? A1");
        auto row = Find("8B 0D ? ? ? ? 68 ? ? ? ? 51 6A 05 E8", "8B 0D ? ? ? ? 68 ? ? ? ? 51 6A 05 E8",
            "A1 ? ? ? ? 83 C4 10 68 ? ? ? ? 50 6A 05 E8");
        auto error = Find("6A 01 E8 ? ? ? ? 68 ? ? ? ? 68 98 01 00 00 E8 ? ? ? ? 8B 0D ? ? ? ? 83 C4 10 68 ? ? ? ? 68 F0 00 00 00 68 ? ? ? ? 51 E8 ? ? ? ? 99 2B C2 BA 40 01 00 00 D1 F8 83 C4 08 2B D0 A1 ? ? ? ? 52 50 E8 ? ? ? ? 83 C4 10 E8",
            "6A 00 6A 01 E8 ? ? ? ? 83 C4 08 BE 40 01 00 00 68 ? ? ? ? 68 98 01 00 00 E8 ? ? ? ? 83 C4 08 A1 ? ? ? ? 68 ? ? ? ? 68 F0 00 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 99 83 C4 08 2B C2 D1 F8 2B F0 A1 ? ? ? ? 56 50 E9",
            "6A 01 E8 ? ? ? ? 83 C4 08 68 ? ? ? ? 68 98 01 00 00 E8 ? ? ? ? A1 ? ? ? ? 83 C4 08 68 ? ? ? ? 68 F0 00 00 00 68 ? ? ? ? 50 E8 ? ? ? ? 99 2B C2 8B 15 ? ? ? ? D1 F8 B9 40 01 00 00 83 C4 08 2B C8 51 52 E8 ? ? ? ? 83 C4 10 E9");
        // The native key conversion and the live key table it fills.
        auto loader = Find("8B 54 24 04 33 C9 56 8B 81 ? ? ? ? 3D FF 00 00 00 7E", "56 33 D2 8B 44 24 08 8B 8A ? ? ? ? 81 F9 FF 00 00 00 7E",
            "8B 54 24 04 56 33 C9 8B 81 ? ? ? ? 3D FF 00 00 00 7E");
        auto controls = Find("8B 04 B5 ? ? ? ? 3B F8 75 14", "8B 83 ? ? ? ? 3B F0 75 17", "8B 0C B5 ? ? ? ? 3B D9 75 10");
        if (!menuState || options.size() != 1 || row.size() != 1 || error.size() != 1)
        {
            char message[160]{};
            sprintf_s(message, "GTA1 options signatures: options=%zu row=%zu error=%zu", options.size(), row.size(), error.size());
            Log::Write(message);
            return;
        }
        auto target = [](auto& pattern, int offset) { return static_cast<void*>(GTA1Build::Target(pattern.get_first(offset))); };
        Sound = reinterpret_cast<decltype(Sound)>(target(options, Off(16, 19, 15)));
        Selection = *options.get_first<int*>(Off(22, 28, 21));
        Row = reinterpret_cast<decltype(Row)>(target(row, Off(14, 14, 16)));
        Background = reinterpret_cast<decltype(Background)>(target(error, Off(2, 4, 2)));
        Header = reinterpret_cast<decltype(Header)>(target(error, Off(17, 27, 20)));
        SmallFont = *error.get_first<int*>(Off(24, 36, 26));
        TextWidth = reinterpret_cast<decltype(TextWidth)>(target(error, Off(47, 56, 49)));
        if (kind == Kind::Retail)
        {
            // Retail jumps to a tail shared with other screens: Text, then Present.
            auto tail = static_cast<uint8_t*>(injector::GetBranchDestination(error.get_first(78)).get<void>());
            Text = reinterpret_cast<decltype(Text)>(GTA1Build::Target(tail));
            Present = reinterpret_cast<decltype(Present)>(GTA1Build::Target(tail + 8));
        }
        else
        {
            Text = reinterpret_cast<decltype(Text)>(target(error, Off(74, 0, 77)));
            Present = reinterpret_cast<decltype(Present)>(target(error, Off(82, 0, 85)));
        }
        if (loader.size() == 1 && controls.size() == 1)
        {
            LoadControls = reinterpret_cast<decltype(LoadControls)>(loader.get_first());
            RawControls = *loader.get_first<int*>(9);
            Controls = *controls.get_first<int*>(Off(3, 2, 3));
        }
        else Log::Write("GTA1 native controls cannot be changed while running.");
        // Gameplay overlay: own copies of the frontend fonts, the text routines' target.
        auto fonts = Find("6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 40 6A 01 6A 01 68 ? ? ? ? 68",
            "6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 01 68 ? ? ? ? 68",
            "6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 21 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 6A 01 6A 01 68 ? ? ? ? 68");
        auto redraw = Find("A1 ? ? ? ? 85 C0 75 09 83 3D ? ? ? ? 02 75 07 C6 05 ? ? ? ? 01 C3",
            "A1 ? ? ? ? 85 C0 75 0A A1 ? ? ? ? 83 F8 02 75 07 C6 05 ? ? ? ? 01 C3",
            "E8 ? ? ? ? 84 C0 75 09 E8 ? ? ? ? 84 C0 74 07 C6 05 ? ? ? ? 01 C3");
        // Where the text routine draws: its pixel pointer and pitch globals.
        auto text = reinterpret_cast<uint8_t*>(Text);
        const int pitchAt = Off(0x1B, 0x29, 0x13), baseAt = Off(0x21, 0x22, 0x1D);
        const bool textOk = kind == Kind::Classics ? text[0x19] == 0x8B && text[0x1A] == 0x3D && text[0x1F] == 0x8B && text[0x20] == 0x2D :
            kind == Kind::Retail ? text[0x20] == 0x8B && text[0x21] == 0x0D && text[0x26] == 0x0F && text[0x27] == 0xAF && text[0x28] == 0x35 :
            text[0x10] == 0x0F && text[0x11] == 0xAF && text[0x12] == 0x05 && text[0x1B] == 0x8B && text[0x1C] == 0x15;
        const int fontText = Off(43, 49, 49), fontCheck = Off(48, 54, 54), fontKey = Off(65, 71, 71), fontMarker = Off(70, 76, 76);
        if (fonts.size() == 1 && *fonts.get_first<int*>(fontCheck) == SmallFont && textOk)
        {
            FontFiles[0] = *fonts.get_first<const char*>(fontText);
            FontFiles[1] = *fonts.get_first<const char*>(5);
            FontFiles[2] = *fonts.get_first<const char*>(fontKey);
            HeaderFont = *fonts.get_first<int*>(10);
            MarkerFont = *fonts.get_first<int*>(fontMarker);
            PixelPitch = *reinterpret_cast<int**>(text + pitchAt);
            PixelBase = *reinterpret_cast<uint8_t***>(text + baseAt);
            LoadFont = reinterpret_cast<decltype(LoadFont)>(target(fonts, 14));
            if (redraw.size() == 1) Redraw = reinterpret_cast<decltype(Redraw)>(redraw.get_first());
            // The native Esc handler: the in-game mode list and the quit prompt keep their Esc.
            auto escape = Find("E8 ? ? ? ? 84 C0 75 78 E8 ? ? ? ? 84 C0 74 1B 6A 00 E8", "E8 ? ? ? ? 84 C0 75 7E E8 ? ? ? ? 84 C0 74 1B 6A 00 E8",
                "E8 ? ? ? ? 84 C0 75 7E E8 ? ? ? ? 84 C0 74 1B 6A 00 E8");
            if (escape.size() == 1)
            {
                ModeListOpen = reinterpret_cast<decltype(ModeListOpen)>(target(escape, 0));
                QuitPromptOpen = reinterpret_cast<decltype(QuitPromptOpen)>(target(escape, 9));
            }
            else LoadFont = nullptr;
        }
        else Log::Write("GTA1 options are unavailable during gameplay.");
        OptionsHook = safetyhook::create_inline(options.get_first(), NativeOptions);
        Installed = bool(OptionsHook);
        Log::Write(Installed ? "GTA1 in-game options installed." : "GTA1 in-game options hook FAILED.");
    }
}
