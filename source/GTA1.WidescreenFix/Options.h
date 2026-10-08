#pragma once
#include <functional>

// In-game options. The native Options screen (sound, music, text speed and so
// on) gets a More Options item below its own. It opens an extra frontend state
// drawn with the game's own font and highlight, with sections that list every
// setting of this fix and those of the original setup program (language and
// the native keyboard controls). Up and Down select, Enter, Left
// and Right change a setting, Esc goes back; changes are saved at once and
// applied immediately where the game allows it. During gameplay Esc (or the
// controller's Back button) pauses the game and opens the same pages over the
// dimmed paused frame, with Resume and Quit Game; Quit Game hands over to the
// native quit prompt that Esc used to open.
namespace GTA1Options
{
    constexpr int State = 21;                      // the native frontend has states 0 to 20
    constexpr int Up = 1, Down = 2, Left = 4, Right = 8, Enter = 0x10, Escape = 0x20, Button = 0x200;

    struct Entry
    {
        std::string label;
        std::function<std::string()> value;      // empty: plain button
        std::function<void(int)> change;         // -1 left, +1 right, 0 enter
        bool restart = false;                    // read only when the game starts
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
    inline std::function<void(int, int)> ApplyResolution;
    inline std::pair<int, int> Applying{};         // resolution to apply before the next frame
    inline int* ResX = nullptr, * ResY = nullptr;

    // ini and registry -------------------------------------------------------
    inline void Save(const char* section, const char* key, const std::string& value)
    {
        CIniReader ini("");
        ini.WriteString(section, key, value);
    }
    inline void SaveInt(const char* section, const char* key, int value) { Save(section, key, std::to_string(value)); }
    inline void SaveFloat(const char* section, const char* key, double value)
    {
        char text[32]{}; sprintf_s(text, "%.2f", value); Save(section, key, text);
    }
    inline void SaveKey(const char* section, const char* key, int value)
    {
        char text[32]{}; sprintf_s(text, "0x%02X", value); Save(section, key, text);
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
        if (RegGetValueA(HKEY_CURRENT_USER, RegistryPath(key).c_str(), name, RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) return fallback;
        return value;
    }
    // Settings of the original setup program. The game's per-user key is often
    // read-only for the user, so they are kept in the ini ([SETUP]) as well and
    // served to the game in place of the registry values.
    inline std::string SetupKey(const char* name)
    {
        std::string key;
        for (auto c = name; *c; ++c) if (*c != ' ') key += *c;
        return key;
    }
    inline void WriteDword(const char* key, const char* name, DWORD value)
    {
        RegSetKeyValueA(HKEY_CURRENT_USER, RegistryPath(key).c_str(), name, REG_DWORD, &value, sizeof(value));
        GameRegistry::SetOverride(RegistryPath(key).c_str(), name, value);
        SaveInt("SETUP", SetupKey(name).c_str(), int(value));
    }
    inline void LoadSetup()
    {
        CIniReader ini("");
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
        char name[64]{};
        if (GetKeyNameTextA(LONG(scan << 16), name, 64) > 0) return name;
        char fallback[16]{}; sprintf_s(fallback, "Key %02X", vk);
        return fallback;
    }
    // DirectInput scan codes, as the native controls store them.
    inline std::string ScanName(int dik)
    {
        if (dik <= 0) return "None";
        if (dik > 255) return "Joystick";
        char name[64]{};
        LONG scan = LONG(dik & 0x7F) | (dik & 0x80 ? 0x100 : 0);
        if (GetKeyNameTextA(scan << 16, name, 64) > 0) return name;
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
            [&value, section, key](int) { value = !value; SaveInt(section, key, value); }, restart };
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
            } };
    }
    inline Entry Range(const char* label, const char* section, const char* key, int& value, int minimum, int maximum, int step)
    {
        return { label, [&value] { return std::to_string(value); },
            [&value, section, key, minimum, maximum, step](int direction)
            {
                value = direction == 0 ? (value + step > maximum ? minimum : value + step) : std::clamp(value + direction * step, minimum, maximum);
                SaveInt(section, key, value);
            } };
    }
    inline Entry Key(const char* label, const char* section, const char* key, int& value)
    {
        return { label, [&value] { return CaptureTarget == &value ? std::string("Press a key") : KeyName(value); },
            [&value, section, key](int step)
            {
                if (step != 0) { value = 0; SaveKey(section, key, 0); return; } // left/right clear the binding
                CaptureTarget = &value; CaptureSection = section; CaptureKey = key;
            } };
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
    inline Entry Back() { return { "Back", nullptr, [](int step) { if (step == 0) Section = -1; } }; }

    // Native controls: registry "Control 0" to "Control 9", DirectInput scan codes.
    inline const int NativeDefaults[] = { 203, 205, 200, 208, 57, 28, 29, 45, 44, 15 };
    inline int NativeValue(int index)
    {
        return int(ReadDword("Controls", ("Control " + std::to_string(index)).c_str(), NativeDefaults[index]));
    }
    inline Entry NativeKey(const char* label, int index)
    {
        return { label, [index] { return NativeCapture == index ? std::string("Press a key") : ScanName(NativeValue(index)); },
            [index](int step) { if (step == 0) NativeCapture = index; } };
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
        DEVMODEA mode{};
        mode.dmSize = sizeof(mode);
        auto [desktopX, desktopY] = GetDesktopRes();
        for (DWORD i = 0; EnumDisplaySettingsA(nullptr, i, &mode); ++i)
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
            } };
    }

    // screen -----------------------------------------------------------------
    inline int Opened = 0;                         // section opened from the native screen
    inline bool Overlay = false;                   // shown over paused gameplay
    inline bool OverlayClosed = false;             // the paused frame must be restored
    inline void Begin(int section)
    {
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
        Pending = {};
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
        SaveInt("MAIN", "ResX", x); SaveInt("MAIN", "ResY", y);
        ApplyResolution(x, y);
    }

    // Menu input, as frontend direction and action bits.
    inline void Handle(int input)
    {
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
        // Rows use the native list layout: row i at 224 + (i - 1) * height.
        const int visible = std::max(1, (480 - height * 2 - 224) / height);
        Selected = std::clamp(Selected, 0, int(shown.size()) - 1);
        if (Selected < Scroll) Scroll = Selected;
        if (Selected >= Scroll + visible) Scroll = Selected - visible + 1;
        Scroll = std::clamp(Scroll, 0, std::max(0, int(shown.size()) - visible));
        Header(176, Titles[Section].c_str());
        for (int k = Scroll; k < std::min(int(shown.size()), Scroll + visible); ++k)
        {
            auto& entry = shown[k];
            auto text = entry.value ? entry.label + ": " + entry.value() : entry.label;   // as the native lists
            Row(k - Scroll + 1, Selected - Scroll + 1, text.c_str());
        }
        if (Scroll + visible < int(shown.size())) Text(font, 196, 224 + visible * height - height / 3, "...");
        if (shown[Selected].restart)
        {
            const char* note = "Applies after a restart";
            Text(font, 320 - TextWidth(font, note) / 2, 480 - height * 2, note);
        }
        // Bottom corners, like the native screens.
        bool capture = CaptureTarget || NativeCapture >= 0;
        const char* left = capture ? "Esc: Cancel" : "Esc: Back";
        const char* right = capture ? "Press a key" : "Enter: Change";
        Text(font, 0, 480 - height, left);
        Text(font, 640 - TextWidth(font, right), 480 - height, right);
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
    inline int PauseSection = 0;                   // first page of the gameplay menu
    inline std::atomic<int> OverlayInput = 0;      // direction and action bits from the window
    inline std::atomic<bool> OpenRequested = false;
    inline DWORD OpenRequestTime = 0;
    inline bool PausedByMenu = false;              // resume the game when the menu closes
    inline std::atomic<bool> QueuedEscape = false; // native Esc for the quit prompt
    inline std::atomic<bool> EscapeTaken = false;  // an Esc that opened the menu
    inline bool (__cdecl* ModeListOpen)() = nullptr;
    inline bool (__cdecl* QuitPromptOpen)() = nullptr;
    inline int* HeaderFont = nullptr, * MarkerFont = nullptr;
    inline const char* FontFiles[3]{};             // small text, header, highlight marker
    inline int OwnFonts[3]{};
    inline char (__cdecl* LoadFont)(int*, const char*, int, int) = nullptr;
    inline uint8_t** PixelBase = nullptr;          // where the frontend text routines draw
    inline int* PixelPitch = nullptr;              // in pixels
    inline bool OverlayStride = false;             // the glyph blitter draws into the overlay canvas
    inline void (__cdecl* Redraw)() = nullptr;     // requests a world redraw while paused
    inline std::vector<uint16_t> Canvas, Snapshot;
    inline int SnapshotWidth = 0, SnapshotHeight = 0;
    constexpr uint16_t Transparent = 0x0821;

    inline bool OverlayActive() { return Overlay; }
    inline bool Paused() { return GTA1Input::IsPaused() && GTA1Input::Available(); }
    inline bool TakeQueuedEscape() { return QueuedEscape.exchange(false); }
    inline bool TakeEscape() { return EscapeTaken.exchange(false); }

    // Pauses the game when needed; the menu opens once it is paused.
    inline void RequestOpen()
    {
        if (Overlay || OpenRequested || !GTA1Input::Available()) return;
        PausedByMenu = !GTA1Input::IsPaused();
        if (PausedByMenu) GTA1Presentation::PendingKey = 64;   // the native pause key
        OpenRequested = true; OpenRequestTime = GetTickCount();
    }
    inline void Resume()
    {
        if (PausedByMenu && GTA1Input::IsPaused()) GTA1Presentation::PendingKey = 64;
        PausedByMenu = false;
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
        static bool held[7]{};
        static DWORD repeat[4]{};
        Poll();
        auto [x, y] = Stick();
        bool down[] = { y < -0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_UP), y > 0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_DOWN),
            x < -0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_LEFT), x > 0.35f || InputDevices::Button(XINPUT_GAMEPAD_DPAD_RIGHT),
            InputDevices::Button(XINPUT_GAMEPAD_A), InputDevices::Button(XINPUT_GAMEPAD_B), InputDevices::Button(XINPUT_GAMEPAD_BACK) };
        constexpr int bits[] = { Up, Down, Left, Right, Enter, Escape, 0 };
        int input = 0;
        DWORD now = GetTickCount();
        for (int i = 0; i < 7; ++i)
        {
            bool pressed = down[i] && !held[i];
            if (i < 4)
            {
                if (pressed) repeat[i] = now + 400;
                else if (down[i] && int32_t(now - repeat[i]) >= 0) { pressed = true; repeat[i] = now + 120; }
            }
            if (pressed) input |= bits[i];
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

    // Before a gameplay frame is presented.
    inline void GameplayPresent(uint8_t* pixels, int width, int height, int pitch, int bits)
    {
        if (!Installed || !LoadFont || !PixelBase || !pixels || (bits != 15 && bits != 16) || pitch < width * 2) return;
        bool openPad = false;
        int pad = PadInput(openPad);
        if (openPad && !Overlay) RequestOpen();
        bool paused = Paused();
        if (OpenRequested && int32_t(GetTickCount() - OpenRequestTime) > 1500) { OpenRequested = false; PausedByMenu = false; }
        if (!paused) { if (Overlay) { Overlay = false; PausedByMenu = false; } OverlayInput = 0; Snapshot.clear(); return; }
        if (!Overlay && OpenRequested && LoadFonts())
        {
            OpenRequested = false; EscapeTaken = false;
            Begin(PauseSection); Overlay = true; OverlayInput = 0; Snapshot.clear();
            pad = 0;
        }
        // The paused world is not redrawn: keep a clean copy of the frame.
        if (Snapshot.empty() || SnapshotWidth != width || SnapshotHeight != height)
        {
            if (!Overlay) { Snapshot.clear(); }
            else
            {
                SnapshotWidth = width; SnapshotHeight = height;
                Snapshot.resize(size_t(width) * height);
                for (int y = 0; y < height; ++y) memcpy(Snapshot.data() + size_t(y) * width, pixels + size_t(y) * pitch, size_t(width) * 2);
            }
        }
        else
            for (int y = 0; y < height; ++y) memcpy(pixels + size_t(y) * pitch, Snapshot.data() + size_t(y) * width, size_t(width) * 2);
        if (Overlay)
        {
            Handle(OverlayInput.exchange(0) | pad);
            if (!Overlay) { OverlayClosed = false; Snapshot.clear(); Resume(); if (Redraw) Redraw(); return; }
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
        if (open && *MenuState == OptionsState) Open(0);
        return result;
    }
    // Before a frontend frame is presented.
    inline void BeforePresent()
    {
        if (!DrawExtra || !Row) return;
        Row(NativeItems + 1, *Selection, Titles[0].c_str());
    }

    // Settings that are only read when the game starts.
    inline bool SoftwareBuffer = true, FramePacing = true, Windowed = true;
    inline int Language = 0;

    inline void Build(int& resX, int& resY)
    {
        using namespace InputDevices;
        ResX = &resX; ResY = &resY;
        LoadSetup();
        CIniReader ini("");
        SoftwareBuffer = ini.ReadBoolean("MAIN", "SoftwareBuffer", true);
        Windowed = ini.ReadBoolean("MAIN", "WindowedMode", true);
        FramePacing = ini.ReadBoolean("MAIN", "FixFramePacing", true);
        Language = int(ReadDword(nullptr, "Language", 0));
        enum { Root, Display, Camera, Controls, Game, ModernKeys, MoreKeys, ClassicKeys, Pause, Count };
        Sections.assign(Count, {});
        Titles = { "MORE OPTIONS", "DISPLAY", "CAMERA", "CONTROLS", "GAME", "MODERN KEYS", "MORE MODERN KEYS", "CLASSIC KEYS", "PAUSED" };
        Sections[Root] = { Open("DISPLAY", Display), Open("CAMERA", Camera), Open("CONTROLS", Controls), Open("GAME", Game), Back() };
        PauseSection = Pause;
        Sections[Pause] = {
            { "RESUME", nullptr, [](int step) { if (step == 0) Close(); } },
            Open("DISPLAY", Display), Open("CAMERA", Camera), Open("CONTROLS", Controls), Open("GAME", Game),
            { "QUIT GAME", nullptr, [](int step) { if (step == 0) { Close(); QueuedEscape = true; } } } };
        Sections[Display] = {
            Resolution(),
            Range("HUD SAFE AREA", "MAIN", "HudSafeArea", GTA1Hud::SafeArea, 0, 32, 4),
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
                    SaveInt("MAIN", "WindowedMode", Windowed);
                    if (mode != 2 && GTA1Window::Fullscreen != (mode == 1))
                    {
                        if (GTA1Window::Enabled) GTA1Window::ToggleFullscreen();
                        else GTA1Window::Fullscreen = mode == 1;
                        SaveInt("MAIN", "FullscreenWindow", GTA1Window::Fullscreen);
                    }
                } },
            Toggle("SOFTWARE BUFFER", "MAIN", "SoftwareBuffer", SoftwareBuffer, true),
            Toggle("FRAME PACING", "MAIN", "FixFramePacing", FramePacing, true),
            Back() };
        Sections[Camera] = {
            Toggle("SMOOTH FOLLOW", "CAMERA", "SmoothFollow", GTA1Camera::Enabled),
            Choice("FOLLOW TIME", "CAMERA", "FollowTime", GTA1Camera::FollowSeconds, { 0.0, 0.1, 0.15, 0.2, 0.25, 0.3, 0.4, 0.5, 0.75, 1.0 }),
            Choice("HEIGHT TIME", "CAMERA", "HeightTime", GTA1Camera::HeightSeconds, { 0.0, 0.1, 0.2, 0.25, 0.35, 0.5, 0.75, 1.0, 1.5 }),
            Choice("ZOOM", "CAMERA", "Zoom", GTA1Camera::Zoom, { 0.75, 1.0, 1.25, 1.5, 1.75, 2.0, 2.5, 3.0 }),
            Toggle("ROTATE CAMERA", "CAMERA", "RotateWithPlayer", GTA1Rotation::Enabled),
            Toggle("ROTATE IN VEHICLES", "CAMERA", "RotateInVehicles", GTA1Rotation::RotateVehicle),
            Toggle("ROTATE ON FOOT", "CAMERA", "RotateOnFoot", GTA1Rotation::RotateFoot),
            Choice("ROTATION SMOOTHING", "CAMERA", "RotationSmoothing", GTA1Rotation::Smoothing, { 0.0, 0.1, 0.2, 0.25, 0.35, 0.5, 0.75, 1.0 }),
            Key("ROTATION KEY", "CAMERA", "ToggleRotationKey", GTA1Rotation::ToggleKey),
            Back() };
        Sections[Controls] = {
            Toggle("MODERN CONTROLS", "INPUT", "ModernControls", ModernControls),
            Toggle("GAMEPAD", "INPUT", "Gamepad", Gamepad),
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
                [](int step) { Language = (Language + (step < 0 ? 3 : 1)) % 4; WriteDword(nullptr, "Language", DWORD(Language)); }, true },
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
