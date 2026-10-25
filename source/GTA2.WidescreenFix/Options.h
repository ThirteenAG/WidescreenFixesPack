#pragma once
#include <functional>
#include "MenuText.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

// In-game Options page. GTA2's Options button started the external GTA2 Manager;
// it now opens a frontend page with the native setup sections and one Plugins
// row. Plugins lists the fix first, then other loaded plugins with settings;
// settings already on the setup pages stay there. The page reuses
// the frontend's own page format (frontend2.cpp: 17 pages of 3018 bytes, up to
// ten 130-byte items each) on the unused page 16, and is rebuilt for each section.
// Enter, Left and Right change the selected setting, Esc goes back; changes are
// written to the cfg and applied immediately where the game allows it.
namespace GTA2Options
{
    constexpr uint16_t Page = 16;
    constexpr int PageBase = 290, PageSize = 3018, ItemSize = 130, MaxItems = 10;
    constexpr int CurrentPage = 286, Flags = 51640; // left, right, up, down, enter, esc, delete
    constexpr uint16_t NoAction = 0x10C, ManagerAction = 0x101;

    struct Entry
    {
        std::wstring label;
        std::function<std::wstring()> value;   // empty: plain button
        std::function<void(int)> change;       // -1 left, +1 right, 0 enter
        uint16_t action = NoAction;
        std::function<std::wstring()> help;
    };

    inline SafetyHookInline BuildHook;
    inline int (__fastcall* SetPage)(void*, int, uint16_t) = nullptr;
    inline std::vector<std::vector<Entry>> Sections;
    inline std::vector<std::wstring> Titles;
    inline int Section = 0;
    inline char* Menu = nullptr;
    inline int* CaptureTarget = nullptr;          // key being rebound
    inline const char* CaptureSection = nullptr, * CaptureKey = nullptr;
    inline DWORD IgnoreUntil = 0;
    inline std::function<bool()> ApplyResolution;
    inline RWGTAMenu::Model Model;
    inline HMODULE HostModule = nullptr;
    inline size_t NativeSections = 0;
    inline std::vector<Entry> NativeRoot;
    inline std::vector<uint16_t> Remembered;
    inline int (__stdcall* TextWidth)(const wchar_t*, int){};
    inline uint16_t* FooterFont{}, * SelectedFont{};
    inline bool NativeRestartRequired{};
    inline std::pair<int, uint16_t> HelpSelection{ -1, 0 };
    inline ULONGLONG HelpStarted{};
    inline std::vector<size_t> ModelPages;

    inline char* PagePtr(char* menu) { return menu + PageBase + PageSize * Page; }

    // cfg --------------------------------------------------------------------
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
    inline void SaveFloat(const char* section, const char* key, float value)
    {
        char text[32]{}; sprintf_s(text, "%.2f", value); Save(section, key, text);
    }
    inline void SaveKey(const char* section, const char* key, int value)
    {
        SaveInt(section, key, value);
    }

    // labels -----------------------------------------------------------------
    inline std::wstring Upper(std::wstring text)
    {
        return GTA2MenuText::Native(std::move(text));
    }
    inline std::wstring KeyName(int vk)
    {
        switch (vk)
        {
        case 0: return L"NONE";
        case VK_LBUTTON: return L"MOUSE 1";
        case VK_RBUTTON: return L"MOUSE 2";
        case VK_MBUTTON: return L"MOUSE 3";
        case VK_XBUTTON1: return L"MOUSE 4";
        case VK_XBUTTON2: return L"MOUSE 5";
        }
        UINT scan = MapVirtualKeyW(vk, MAPVK_VK_TO_VSC);
        switch (vk)
        {
        case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
        case VK_END: case VK_HOME: case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
        case VK_RCONTROL: case VK_RMENU:
            scan |= 0x100; break;
        }
        wchar_t name[64]{};
        if (GetKeyNameTextW(LONG(scan << 16), name, 64) > 0) return Upper(name);
        wchar_t fallback[16]{}; swprintf_s(fallback, L"KEY %02X", vk);
        return fallback;
    }
    inline std::wstring OnOff(bool value) { return value ? L"ON" : L"OFF"; }
    inline std::wstring Number(float value)
    {
        wchar_t text[16]{}; swprintf_s(text, L"%.2f", value); return text;
    }

    // entries ----------------------------------------------------------------
    inline Entry Toggle(const wchar_t* label, const char* section, const char* key, bool& value, bool restart = false)
    {
        std::wstring text = label;
        if (restart) text += L" (RESTART)";
        return { text, [&value] { return OnOff(value); },
            [&value, section, key](int) { value = !value; SaveInt(section, key, value); } };
    }
    inline Entry Choice(const wchar_t* label, const char* section, const char* key, float& value, std::vector<float> choices)
    {
        return { label, [&value] { return Number(value); },
            [&value, section, key, choices](int step)
            {
                size_t index = 0;
                for (size_t i = 0; i < choices.size(); ++i)
                    if (std::fabs(choices[i] - value) < std::fabs(choices[index] - value)) index = i;
                index = (index + choices.size() + (step < 0 ? -1 : 1)) % choices.size();
                value = choices[index];
                SaveFloat(section, key, value);
            } };
    }
    inline Entry Key(const wchar_t* label, const char* section, const char* key, int& value)
    {
        return { label, [&value] { return CaptureTarget == &value ? std::wstring(L"PRESS A KEY") : KeyName(value); },
            [&value, section, key](int step)
            {
                if (step != 0) { value = 0; SaveKey(section, key, 0); return; } // left/right clear the binding
                CaptureTarget = &value; CaptureSection = section; CaptureKey = key;
            } };
    }
    inline std::vector<int> Parents;              // parent section of each section
    inline Entry Open(const wchar_t* label, int section)
    {
        return { label, nullptr, [section](int step) { if (step == 0) { Parents.resize(std::max<size_t>(Parents.size(), section + 1)); Parents[section] = Section; Section = section; } } };
    }
    inline Entry Back() { return { L"BACK", nullptr, [](int step) { if (step == 0) Section = -1; } }; }

    // Settings of the original GTA2 Manager, stored in the game's registry keys.
    inline const char* RegistryRoot = "Software\\DMA Design Ltd\\GTA2\\";
    inline DWORD ReadDword(const char* key, const char* name, DWORD fallback)
    {
        DWORD value = 0, size = sizeof(value);
        std::string path = std::string(RegistryRoot) + key;
        if (GameRegistry::Override(path.c_str(), name, value)) return value;
        if (RegGetValueA(HKEY_CURRENT_USER, path.c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS &&
            RegGetValueA(HKEY_LOCAL_MACHINE, path.c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    inline void WriteDword(const char* key, const char* name, DWORD value)
    {
        std::string path = std::string(RegistryRoot) + key;
        Save((std::string("SETUP.") + key).c_str(), name, std::to_string(value));
    }
    inline std::string ReadText(const char* key, const char* name, const char* fallback)
    {
        char value[64]{}; DWORD size = sizeof(value);
        std::string path = std::string(RegistryRoot) + key;
        std::string configured;
        if (GameRegistry::StringOverride(path.c_str(), name, configured)) return configured;
        if (RegGetValueA(HKEY_CURRENT_USER, path.c_str(), name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS &&
            RegGetValueA(HKEY_LOCAL_MACHINE, path.c_str(), name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    inline void WriteText(const char* key, const char* name, const std::string& value)
    {
        std::string path = std::string(RegistryRoot) + key;
        Save((std::string("SETUP.") + key).c_str(), name, value);
    }
    inline Entry RegToggle(const wchar_t* label, const char* key, const char* name, DWORD fallback, const wchar_t* note = nullptr)
    {
        std::wstring text = label;
        if (note) text += note;
        return { text, [=] { return OnOff(ReadDword(key, name, fallback) != 0); },
            [=](int) { WriteDword(key, name, ReadDword(key, name, fallback) ? 0 : 1); } };
    }
    inline Entry RegRange(const wchar_t* label, const char* key, const char* name, int minimum, int maximum, int step, int fallback,
        std::function<void(int)> apply = nullptr)
    {
        return { label, [=] { return std::to_wstring(int(ReadDword(key, name, fallback))); },
            [=](int direction)
            {
                int value = int(ReadDword(key, name, fallback));
                value = direction == 0 ? (value + step > maximum ? minimum : value + step) : std::clamp(value + direction * step, minimum, maximum);
                WriteDword(key, name, DWORD(value));
                if (apply) apply(value);
            } };
    }
    // Native controls: DirectInput scan codes in registry Control, values 0 to 11.
    inline int DikToVk(int dik)
    {
        int vk = int(MapVirtualKeyW(UINT(dik & 0x7F) | (dik & 0x80 ? 0xE000 : 0), MAPVK_VSC_TO_VK_EX));
        switch (dik)
        {
        case 0xC8: return VK_UP; case 0xD0: return VK_DOWN; case 0xCB: return VK_LEFT; case 0xCD: return VK_RIGHT;
        case 0xC7: return VK_HOME; case 0xCF: return VK_END; case 0xC9: return VK_PRIOR; case 0xD1: return VK_NEXT;
        case 0xD2: return VK_INSERT; case 0xD3: return VK_DELETE; case 0x9D: return VK_RCONTROL; case 0xB8: return VK_RMENU;
        }
        return vk;
    }
    inline int VkToDik(int vk)
    {
        switch (vk)
        {
        case VK_UP: return 0xC8; case VK_DOWN: return 0xD0; case VK_LEFT: return 0xCB; case VK_RIGHT: return 0xCD;
        case VK_HOME: return 0xC7; case VK_END: return 0xCF; case VK_PRIOR: return 0xC9; case VK_NEXT: return 0xD1;
        case VK_INSERT: return 0xD2; case VK_DELETE: return 0xD3; case VK_RCONTROL: return 0x9D; case VK_RMENU: return 0xB8;
        case VK_CONTROL: return 0x1D; case VK_MENU: return 0x38; case VK_SHIFT: return 0x2A;
        }
        return int(MapVirtualKeyW(UINT(vk), MAPVK_VK_TO_VSC)) & 0xFF;
    }
    inline int NativeCapture = -1;                // index of the native control being rebound
    inline Entry NativeKey(const wchar_t* label, int index)
    {
        static const int defaults[] = { 0xC8, 0xD0, 0xCB, 0xCD, 0x1D, 0x1C, 0x39, 0x2C, 0x2D, 0x0F, 0x38, 0x36 };
        return { label, [=]
            {
                if (NativeCapture == index) return std::wstring(L"PRESS A KEY");
                return KeyName(DikToVk(int(ReadDword("Control", std::to_string(index).c_str(), defaults[index]))));
            },
            [=](int step) { if (step == 0) NativeCapture = index; } };
    }

    // resolution -------------------------------------------------------------
    inline int* ResX = nullptr, * ResY = nullptr;
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
        return { L"RESOLUTION", []
            {
                auto [x, y] = Pending.first ? Pending : std::pair{ *ResX, *ResY };
                wchar_t text[48]{};
                swprintf_s(text, Pending.first && Pending != std::pair{ *ResX, *ResY } ? L"%dX%d  ENTER" : L"%dX%d", x, y);
                return std::wstring(text);
            },
            [](int step)
            {
                auto modes = Modes();
                auto current = Pending.first ? Pending : std::pair{ *ResX, *ResY };
                if (step == 0)
                {
                    if (!Pending.first || Pending == std::pair{ *ResX, *ResY }) return;
                    *ResX = Pending.first; *ResY = Pending.second; Pending = {};
                    bool applied = ApplyResolution && ApplyResolution();
                    SaveInt("MAIN", "ResX", *ResX, applied); SaveInt("MAIN", "ResY", *ResY, applied);
                    return;
                }
                auto at = std::find(modes.begin(), modes.end(), current);
                size_t index = at == modes.end() ? 0 : size_t(at - modes.begin());
                index = (index + modes.size() + (step < 0 ? -1 : 1)) % modes.size();
                Pending = modes[index];
            } };
    }

    // page -------------------------------------------------------------------
    inline std::wstring FontText(const std::string& text)
    {
        int size = MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), nullptr, 0);
        std::wstring wide(size, L'\0');
        if (size) MultiByteToWideChar(CP_UTF8, 0, text.data(), int(text.size()), wide.data(), size);
        return Upper(std::move(wide));
    }
    inline std::string LanguageTag()
    {
        auto& chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        auto language = ReadText("Option", "Language", "e");
        if (language == "f") return "fr";
        if (language == "g") return "de";
        if (language == "i") return "it";
        if (language == "s") return "es";
        return "en";
    }
    inline void RefreshModelRows()
    {
        for (size_t i = 0; i < ModelPages.size(); ++i)
        {
            auto& entries = Sections[NativeSections + i];
            entries.clear();
            for (auto row : Model.VisibleRows(ModelPages[i]))
                entries.push_back({ FontText(Model.Label(*row)), [row] { return FontText(Model.Value(*row)); },
                    [row](int step) { Model.Change(*row, step, false); }, NoAction,
                    [row] { return FontText(Model.Help(*row)); } });
            entries.push_back(Back());
        }
    }
    inline const std::set<std::string> JoinedSettings
    {
        "MAIN.ResX", "MAIN.ResY", "MAIN.WindowMode",
        "MAIN.FillFrontendBackground", "MAIN.HudAspectRatioConstraint", "MAIN.SkipIntro",
        "MISC.SkipCredits", "MISC.NoSampManDelay", "MISC.QuicksaveKey",
        "MISC.ZoomIncreaseKey", "MISC.ZoomDecreaseKey", "MISC.RecoverPlayerKey",
        "CAMERA.RotateWithPlayer", "CAMERA.RotateInVehicles", "CAMERA.RotateOnFoot",
        "CAMERA.RotationSmoothing", "CAMERA.ToggleRotationKey", "CAMERA.SmoothFollow",
        "CAMERA.FollowTime", "CAMERA.HeightTime", "CAMERA.MaxLookAhead",
        "INPUT.ModernControls", "INPUT.Gamepad", "INPUT.StickThrottle",
        "INPUT.StickDeadzone", "INPUT.MouseAim", "INPUT.MoveUpKey",
        "INPUT.MoveDownKey", "INPUT.MoveLeftKey", "INPUT.MoveRightKey",
        "INPUT.FireKey", "INPUT.EnterVehicleKey", "INPUT.JumpKey",
        "INPUT.SpecialKey", "INPUT.PreviousWeaponKey", "INPUT.NextWeaponKey",
        "INPUT.PauseKey"
    };

    inline void OpenModel()
    {
        if (!HostModule)
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(&HostModule), &HostModule);
        Model.OpenPlugins(HostModule, LanguageTag(), MaxItems - 2, JoinedSettings);
        Sections.resize(NativeSections);
        Titles.resize(NativeSections);
        ModelPages.clear();
        auto root = NativeRoot;
        if (!root.empty()) root.pop_back();
        std::vector<Entry> links;
        for (size_t page = 0; page < Model.Pages().size(); ++page)
        {
            auto& described = Model.Pages()[page];
            int section = int(Sections.size());
            ModelPages.push_back(page); Sections.emplace_back();
            Titles.push_back(FontText(described.title));
            links.push_back({ FontText(described.title), nullptr, [page, section](int step)
                {
                    if (step != 0) return;
                    Parents.resize(Sections.size()); Parents[section] = Section;
                    Model.PageShown(page); Section = section;
                } });
        }
        Parents.assign(Sections.size(), 0);
        if (!links.empty())
        {
            // Native pages have ten rows. Reserve Previous/Next/Back so additional
            // loaded plugins never disappear behind the frontend's row bound.
            size_t count = (links.size() + 6) / 7;
            std::vector<int> pages;
            for (size_t i = 0; i < count; ++i) { pages.push_back(int(Sections.size())); Sections.emplace_back(); Titles.push_back(FontText(Model.Text("menu.plugins"))); }
            Parents.resize(Sections.size(), 0);
            for (size_t i = 0; i < count; ++i)
            {
                auto& entries = Sections[pages[i]];
                auto begin = std::min(links.size(), i * 7), end = std::min(links.size(), begin + 7);
                entries.assign(links.begin() + begin, links.begin() + end);
                auto turn = [](const wchar_t* label, int section)
                    { return Entry{ label, nullptr, [section](int step) { if (!step) Section = section; } }; };
                if (i) entries.push_back(turn(FontText(Model.Text("menu.previous")).c_str(), pages[i - 1]));
                if (i + 1 < count) entries.push_back(turn(FontText(Model.Text("menu.next")).c_str(), pages[i + 1]));
                entries.push_back(Back());
            }
            int first = pages[0];
            root.push_back({ FontText(Model.Text("menu.plugins")), nullptr, [first](int step)
                {
                    if (step != 0) return;
                    Parents.resize(Sections.size()); Parents[first] = Section; Section = first;
                } });
        }
        root.push_back(Back());
        Sections[0] = std::move(root);
        Remembered.assign(Sections.size(), 0);
        HelpSelection = { -1, 0 };
        RefreshModelRows();
    }
    inline void Build(char* menu)
    {
        Menu = menu;
        auto page = PagePtr(menu);
        auto& entries = Sections[Section];
        const int count = int(std::min<size_t>(entries.size(), MaxItems));
        const int top = std::max(180, 240 - (count * 22) / 2), x = 190; // right of the menu artwork
        *reinterpret_cast<uint16_t*>(page) = uint16_t(count);
        // Native static text is separate from the ten selectable items. Page 0
        // has none, so copying its layout alone leaves every section untitled.
        // DrawPage (457920) reads 15 110-byte records from page + 1304.
        *reinterpret_cast<uint16_t*>(page + 2) = 1;
        auto heading = page + 1304;
        std::memset(heading, 0, 110);
        heading[0] = 1; heading[1] = 1;
        *reinterpret_cast<int16_t*>(heading + 2) = int16_t(x);
        *reinterpret_cast<int16_t*>(heading + 4) = int16_t(top - 34);
        auto font = *reinterpret_cast<uint16_t*>(menu + 284);
        auto measure = [font](const std::wstring& text)
        {
            return TextWidth(text.c_str(), font);
        };
        auto rowMeasure = [font](const std::wstring& text)
        {
            return std::max(TextWidth(text.c_str(), font), TextWidth(text.c_str(), *SelectedFont));
        };
        auto title = GTA2MenuText::Fit(Titles[Section], 430, 49, measure);
        wcsncpy_s(reinterpret_cast<wchar_t*>(heading + 6), 50, title.c_str(), _TRUNCATE);
        *reinterpret_cast<uint16_t*>(heading + 106) = 0xFFFF; // current native font
        *reinterpret_cast<uint16_t*>(heading + 108) = 0xFFFF; // normal text path
        for (int k = 0; k < MaxItems; ++k)
        {
            auto item = page + 4 + ItemSize * k;
            auto marker = page + 2954 + 6 * k;
            if (k >= count) { item[1] = 0; marker[4] = 0; continue; }
            auto& entry = entries[k];
            auto text = GTA2MenuText::Join(entry.label, entry.value ? entry.value() : std::wstring{}, 430, 49, rowMeasure);
            item[0] = 1; item[1] = 1;
            *reinterpret_cast<int16_t*>(item + 2) = int16_t(x);
            *reinterpret_cast<int16_t*>(item + 4) = int16_t(top + 22 * k);
            wcsncpy_s(reinterpret_cast<wchar_t*>(item + 6), 50, text.c_str(), _TRUNCATE);
            *reinterpret_cast<uint16_t*>(item + 106) = 0xFFFF;
            *reinterpret_cast<uint16_t*>(item + 108) = 0xFFFF;
            *reinterpret_cast<uint16_t*>(item + 128) = entry.action;
            *reinterpret_cast<int16_t*>(marker) = int16_t(x - 20);
            *reinterpret_cast<int16_t*>(marker + 2) = int16_t(top + 22 * k + 8);
            marker[4] = 1;
        }
        auto& selected = *reinterpret_cast<uint16_t*>(page + 3014);
        if (selected >= count) selected = 0;
        *reinterpret_cast<uint16_t*>(page + 3016) = 0;
        auto staticText = [page](const std::wstring& text, int x, int y, uint16_t font)
        {
            auto& count = *reinterpret_cast<uint16_t*>(page + 2);
            if (text.empty() || count >= 15)
                return;
            auto record = page + 1304 + count++ * 110;
            std::memset(record, 0, 110);
            record[0] = record[1] = 1;
            *reinterpret_cast<int16_t*>(record + 2) = int16_t(x);
            *reinterpret_cast<int16_t*>(record + 4) = int16_t(y);
            wcsncpy_s(reinterpret_cast<wchar_t*>(record + 6), 50, text.c_str(), _TRUNCATE);
            *reinterpret_cast<uint16_t*>(record + 106) = font;
            *reinterpret_cast<uint16_t*>(record + 108) = 0xFFFF;
        };
        auto footerMeasure = [](const std::wstring& text)
        {
            return TextWidth(text.c_str(), *FooterFont);
        };
        auto selection = std::pair{ Section, selected };
        if (HelpSelection != selection)
        {
            HelpSelection = selection;
            HelpStarted = GetTickCount64();
        }
        if (selected < entries.size() && entries[selected].help)
        {
            auto lines = GTA2MenuText::Wrap(entries[selected].help(), 544, 49, footerMeasure);
            // Long help continues in the same native footer, without losing its end.
            auto pages = (lines.size() + 3) / 4;
            size_t part = pages ? size_t((GetTickCount64() - HelpStarted) / 12000) % pages : 0;
            for (size_t i = 0; i < 4 && part * 4 + i < lines.size(); ++i)
                staticText(lines[part * 4 + i], 48, 382 + int(i) * 16, *FooterFont);
            if (pages > 1)
                staticText(std::to_wstring(part + 1) + L"/" + std::to_wstring(pages), 594, 464, *FooterFont);
        }
        if (NativeRestartRequired || Model.RestartRequired())
        {
            auto lines = GTA2MenuText::Wrap(FontText(Model.Text("menu.restartwarning")), 544, 49, footerMeasure);
            for (size_t i = 0; i < std::min<size_t>(lines.size(), 2); ++i)
                staticText(lines[i], 48, 448 + int(i) * 16, *FooterFont);
        }
    }

    // Called when the frontend builds its pages: route the Options button here.
    inline int __fastcall BuildPages(char* menu, int)
    {
        auto result = BuildHook.thiscall<int>(menu);
        auto options = menu + PageBase + 4 + ItemSize * 1;    // page 0, item 1: "options"
        if (*reinterpret_cast<uint16_t*>(options + 128) == ManagerAction)
            *reinterpret_cast<uint16_t*>(options + 128) = Page;
        std::memcpy(PagePtr(menu), menu + PageBase, PageSize); // start from the main page's layout
        OpenModel();
        Section = 0;
        *reinterpret_cast<uint16_t*>(PagePtr(menu) + 3014) = 0;
        Build(menu);
        return result;
    }

    // Before the frontend switches to a page.
    inline void Opening(char* menu, uint16_t page)
    {
        if (page != Page) { Model.Close(); return; }
        OpenModel();
        Section = 0; CaptureTarget = nullptr; NativeCapture = -1; Pending = {};
        *reinterpret_cast<uint16_t*>(PagePtr(menu) + 3014) = 0;
        Build(menu);
    }

    // A key pressed while a binding is being changed. Returns true when consumed.
    inline bool KeyDown(int vk)
    {
        if (NativeCapture >= 0)
        {
            int dik = vk == VK_ESCAPE || vk < 8 ? 0 : VkToDik(vk); // mouse buttons cannot be native controls
            if (dik) WriteDword("Control", std::to_string(NativeCapture).c_str(), DWORD(dik));
            NativeCapture = -1;
            IgnoreUntil = GetTickCount() + 300;
            if (Menu) Build(Menu);
            return true;
        }
        if (!CaptureTarget) return false;
        if (vk != VK_ESCAPE)
        {
            *CaptureTarget = vk;
            SaveKey(CaptureSection, CaptureKey, vk);
        }
        CaptureTarget = nullptr;
        IgnoreUntil = GetTickCount() + 300;
        if (Menu) Build(Menu);
        return true;
    }

    inline void RememberSelection(char* page, int before)
    {
        if (Section == before)
            return;
        Remembered.resize(Sections.size());
        Remembered[before] = *reinterpret_cast<uint16_t*>(page + 3014);
        *reinterpret_cast<uint16_t*>(page + 3014) = Remembered[Section];
    }

    // After the frontend decoded its input for this frame.
    inline void Process(char* menu)
    {
        Menu = menu;
        if (*reinterpret_cast<uint16_t*>(menu + CurrentPage) != Page) return;
        if (RWGTAMenu::Model::Changed().exchange(false)) { RefreshModelRows(); Build(menu); }
        auto flags = reinterpret_cast<uint8_t*>(menu + Flags);
        if (CaptureTarget || NativeCapture >= 0 || int32_t(GetTickCount() - IgnoreUntil) < 0)
        {
            std::fill(flags, flags + 7, uint8_t(0));
            return;
        }
        auto page = PagePtr(menu);
        auto selected = *reinterpret_cast<uint16_t*>(page + 3014);
        auto& entries = Sections[Section];
        if (flags[5])                                        // Esc
        {
            flags[5] = 0;
            if (Section != 0)
            {
                int before = Section;
                Section = Section < int(Parents.size()) ? Parents[Section] : 0;
                RememberSelection(page, before);
                Build(menu);
            }
            else if (SetPage) SetPage(menu, 0, 0);
            return;
        }
        if (selected >= entries.size()) return;
        auto& entry = entries[selected];
        if (entry.action != NoAction) return;                 // native button (GTA2 Manager)
        int step = flags[4] ? 0 : flags[0] ? -1 : flags[1] ? 1 : 2;
        if (step == 2) { Build(menu); return; }
        flags[0] = flags[1] = flags[4] = 0;
        if (step != 0 && !entry.value) return;               // buttons only react to Enter
        int before = Section;
        if (entry.change) entry.change(step);
        if (Section == -1)                                   // Back
        {
            if (before == 0) { Section = 0; if (SetPage) SetPage(menu, 0, 0); return; }
            Section = before < int(Parents.size()) ? Parents[before] : 0;
        }
        RememberSelection(page, before);
        Build(menu);
    }

    // setPage: the frontend's page switch (already hooked by the caller).
    inline void Install(void* setPage)
    {
        auto build = hook::pattern("53 55 56 8B F1 BD 03 00 00 00 BB 01 00 00 00 57 66 C7 86 20 01 00 00 10");
        auto width = hook::pattern("8B 0D ? ? ? ? 53 55 56 8D 44 24 14 57 8B 7C 24 14 50 33 F6");
        auto footer = hook::pattern("66 A1 ? ? ? ? 50 51 8B CC 68 CC 01 00 00");
        auto selected = hook::pattern("66 8B 15 ? ? ? ? 8B 44 24 18 52 51 8B CC 50 E8 ? ? ? ? 51 8B CC 53");
        if (build.size() != 1 || width.size() != 1 || footer.size() != 1 || selected.size() != 1 || !setPage)
        {
            Log::Write("GTA2 options page signatures unavailable.");
            return;
        }
        TextWidth = reinterpret_cast<decltype(TextWidth)>(width.get_first());
        FooterFont = *footer.get_first<uint16_t*>(2);
        SelectedFont = *selected.get_first<uint16_t*>(3);
        SetPage = reinterpret_cast<decltype(SetPage)>(setPage);
        NativeSections = Sections.size(); NativeRoot = Sections[0];
        Titles = { L"ADVANCED OPTIONS", L"DISPLAY", L"GRAPHICS", L"SOUND", L"CAMERA", L"CONTROLS",
            L"MODERN KEYS", L"MORE MODERN KEYS", L"CLASSIC KEYS", L"MORE CLASSIC KEYS", L"GAME" };
        BuildHook = safetyhook::create_inline(build.get_first(), BuildPages);
        Log::Write("GTA2 in-game options page installed.");
    }
}
