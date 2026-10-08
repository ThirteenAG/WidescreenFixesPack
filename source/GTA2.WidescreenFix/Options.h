#pragma once
#include <functional>

// In-game Options page. GTA2's Options button started the external GTA2 Manager;
// it now opens a frontend page with every setting of this fix. The page reuses
// the frontend's own page format (frontend2.cpp: 17 pages of 3018 bytes, up to
// ten 130-byte items each) on the unused page 16, and is rebuilt for each section.
// Enter, Left and Right change the selected setting, Esc goes back; changes are
// written to the ini and applied immediately where the game allows it.
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
    inline std::function<void()> ApplyResolution;

    inline char* PagePtr(char* menu) { return menu + PageBase + PageSize * Page; }

    // ini --------------------------------------------------------------------
    inline void Save(const char* section, const char* key, const std::string& value)
    {
        CIniReader ini("");
        ini.WriteString(section, key, value);
    }
    inline void SaveInt(const char* section, const char* key, int value) { Save(section, key, std::to_string(value)); }
    inline void SaveFloat(const char* section, const char* key, float value)
    {
        char text[32]{}; sprintf_s(text, "%.2f", value); Save(section, key, text);
    }
    inline void SaveKey(const char* section, const char* key, int value)
    {
        char text[32]{}; sprintf_s(text, "0x%02X", value); Save(section, key, text);
    }

    // labels -----------------------------------------------------------------
    inline std::wstring Upper(std::wstring text)
    {
        for (auto& c : text) c = wchar_t(towupper(c));
        return text;
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
        if (RegGetValueA(HKEY_CURRENT_USER, path.c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    inline void WriteDword(const char* key, const char* name, DWORD value)
    {
        std::string path = std::string(RegistryRoot) + key;
        RegSetKeyValueA(HKEY_CURRENT_USER, path.c_str(), name, REG_DWORD, &value, sizeof(value));
    }
    inline std::string ReadText(const char* key, const char* name, const char* fallback)
    {
        char value[64]{}; DWORD size = sizeof(value);
        std::string path = std::string(RegistryRoot) + key;
        if (RegGetValueA(HKEY_CURRENT_USER, path.c_str(), name, RRF_RT_REG_SZ, nullptr, value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    inline void WriteText(const char* key, const char* name, const std::string& value)
    {
        std::string path = std::string(RegistryRoot) + key;
        RegSetKeyValueA(HKEY_CURRENT_USER, path.c_str(), name, REG_SZ, value.c_str(), DWORD(value.size() + 1));
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
        DEVMODEW mode{ .dmSize = sizeof(mode) };
        auto [desktopX, desktopY] = GetDesktopRes();
        for (DWORD i = 0; EnumDisplaySettingsW(nullptr, i, &mode); ++i)
            if (mode.dmPelsWidth >= 640 && mode.dmPelsHeight >= 480 && int(mode.dmPelsWidth) <= desktopX && int(mode.dmPelsHeight) <= desktopY &&
                mode.dmPelsWidth <= mode.dmPelsHeight * 12 / 5)
                modes.emplace_back(int(mode.dmPelsWidth), int(mode.dmPelsHeight));
        // Common sizes fit any window up to the monitor; the monitor's own modes
        // are added, except unusually wide ones such as 1920x540.
        const std::pair<int, int> common[] = { {640, 480}, {800, 600}, {1024, 768}, {1152, 864}, {1280, 720}, {1280, 800},
            {1280, 960}, {1280, 1024}, {1366, 768}, {1440, 900}, {1440, 1080}, {1600, 900}, {1600, 1200}, {1680, 1050},
            {1920, 1080}, {1920, 1200}, {1920, 1440}, {2560, 1080}, {2560, 1440}, {2560, 1600}, {2880, 1620}, {3440, 1440},
            {3840, 1080}, {3840, 1600}, {3840, 2160}, {3840, 2400}, {5120, 1440}, {5120, 2160}, {5120, 2880}, {7680, 4320} };
        for (auto [w, h] : common)
            if (w <= desktopX && h <= desktopY) modes.emplace_back(w, h);
        if (ResX && ResY) modes.emplace_back(*ResX, *ResY);
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
                swprintf_s(text, Pending.first && Pending != std::pair{ *ResX, *ResY } ? L"%dX%d - ENTER" : L"%dX%d", x, y);
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
                    SaveInt("MAIN", "ResX", *ResX); SaveInt("MAIN", "ResY", *ResY);
                    if (ApplyResolution) ApplyResolution();
                    return;
                }
                auto at = std::find(modes.begin(), modes.end(), current);
                size_t index = at == modes.end() ? 0 : size_t(at - modes.begin());
                index = (index + modes.size() + (step < 0 ? -1 : 1)) % modes.size();
                Pending = modes[index];
            } };
    }

    // page -------------------------------------------------------------------
    inline void Build(char* menu)
    {
        Menu = menu;
        auto page = PagePtr(menu);
        auto& entries = Sections[Section];
        const int count = int(std::min<size_t>(entries.size(), MaxItems));
        const int top = 240 - (count * 22) / 2, x = 190; // right of the menu artwork
        *reinterpret_cast<uint16_t*>(page) = uint16_t(count);
        for (int k = 0; k < MaxItems; ++k)
        {
            auto item = page + 4 + ItemSize * k;
            auto marker = page + 2954 + 6 * k;
            if (k >= count) { item[1] = 0; marker[4] = 0; continue; }
            auto& entry = entries[k];
            std::wstring text = entry.label;
            if (entry.value) text += L"   " + entry.value();
            item[0] = 1; item[1] = 1;
            *reinterpret_cast<int16_t*>(item + 2) = int16_t(x);
            *reinterpret_cast<int16_t*>(item + 4) = int16_t(top + 22 * k);
            wcsncpy_s(reinterpret_cast<wchar_t*>(item + 6), 50, text.c_str(), _TRUNCATE);
            *reinterpret_cast<uint16_t*>(item + 128) = entry.action;
            *reinterpret_cast<int16_t*>(marker) = int16_t(x - 20);
            *reinterpret_cast<int16_t*>(marker + 2) = int16_t(top + 22 * k + 8);
            marker[4] = 1;
        }
        auto& selected = *reinterpret_cast<uint16_t*>(page + 3014);
        if (selected >= count) selected = 0;
        *reinterpret_cast<uint16_t*>(page + 3016) = 0;
    }

    // Called when the frontend builds its pages: route the Options button here.
    inline int __fastcall BuildPages(char* menu, int)
    {
        auto result = BuildHook.thiscall<int>(menu);
        auto options = menu + PageBase + 4 + ItemSize * 1;    // page 0, item 1: "options"
        if (*reinterpret_cast<uint16_t*>(options + 128) == ManagerAction)
            *reinterpret_cast<uint16_t*>(options + 128) = Page;
        std::memcpy(PagePtr(menu), menu + PageBase, PageSize); // start from the main page's layout
        Section = 0;
        *reinterpret_cast<uint16_t*>(PagePtr(menu) + 3014) = 0;
        Build(menu);
        return result;
    }

    // Before the frontend switches to a page.
    inline void Opening(char* menu, uint16_t page)
    {
        if (page != Page) return;
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

    // After the frontend decoded its input for this frame.
    inline void Process(char* menu)
    {
        Menu = menu;
        if (*reinterpret_cast<uint16_t*>(menu + CurrentPage) != Page) return;
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
            if (Section != 0) { Section = Section < int(Parents.size()) ? Parents[Section] : 0; *reinterpret_cast<uint16_t*>(page + 3014) = 0; Build(menu); }
            else if (SetPage) SetPage(menu, 0, 0);
            return;
        }
        if (selected >= entries.size()) return;
        auto& entry = entries[selected];
        if (entry.action != NoAction) return;                 // native button (GTA2 Manager)
        int step = flags[4] ? 0 : flags[0] ? -1 : flags[1] ? 1 : 2;
        if (step == 2) return;
        flags[0] = flags[1] = flags[4] = 0;
        if (step != 0 && !entry.value) return;               // buttons only react to Enter
        int before = Section;
        if (entry.change) entry.change(step);
        if (Section == -1)                                   // Back
        {
            if (before == 0) { Section = 0; if (SetPage) SetPage(menu, 0, 0); return; }
            Section = before < int(Parents.size()) ? Parents[before] : 0;
        }
        if (Section != before) *reinterpret_cast<uint16_t*>(page + 3014) = 0;
        Build(menu);
    }

    // setPage: the frontend's page switch (already hooked by the caller).
    inline void Install(void* setPage)
    {
        auto build = hook::pattern("53 55 56 8B F1 BD 03 00 00 00 BB 01 00 00 00 57 66 C7 86 20 01 00 00 10");
        if (build.size() != 1 || !setPage)
        {
            Log::Write("GTA2 options page signatures unavailable.");
            return;
        }
        SetPage = reinterpret_cast<decltype(SetPage)>(setPage);
        BuildHook = safetyhook::create_inline(build.get_first(), BuildPages);
        Log::Write("GTA2 in-game options page installed.");
    }
}
