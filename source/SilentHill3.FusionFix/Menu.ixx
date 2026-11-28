module;

#include <stdafx.h>
#include "FixPad.hpp"
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/SilentHill/Display.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/RainDroplets.hpp"

export module Menu;

import ComVars;

// The fourth Options page lists plugins with settings, with this fix first. Each plugin opens its page: Preset, its
// settings or a row per category opening that category, and Restore Defaults. Every list scrolls row by row. Native
// text, colours and navigation stay the same; values use the same runtime interface as FusionFixHub.
//
// The native Options screen keeps one selection for its 28 rows across its three pages; the page boundaries are
// hard-coded all over it. The new page therefore is a "virtual" page: while it shows, the native selection stays
// parked on the first OPTIONS row and the fix runs the frame instead of the native frame function.

namespace
{


    void(__cdecl* TextReset)() = nullptr;
    void(__cdecl* ReadOptInput)() = nullptr;
    void(__cdecl* SetTextBg)(int, int, int) = nullptr;
    void(__cdecl* TextLayerFront)() = nullptr;
    void(__cdecl* TextLayerBack)() = nullptr;
    void(__cdecl* OptBackground)() = nullptr;
    void* DrawOptTextFn = nullptr;      // usercall eax = colour, edx = row, ecx = kind; stack: message id
    void(__cdecl* SetColor)(int, int, int, int) = nullptr;
    const uint16_t* (__cdecl* Encode)(const char*) = nullptr;
    void(__cdecl* CentreOn)() = nullptr;
    void(__cdecl* CentreOff)() = nullptr;
    void(__cdecl* SetGlyphSize)(int, int) = nullptr;
    void(__cdecl* ClearGlyphSize)() = nullptr;
    int(__cdecl* MsgWidth)(int) = nullptr;
    void(__cdecl* ClearRects)() = nullptr;
    void(__cdecl* AddRect)(int, int, int, int, int) = nullptr;
    int(__cdecl* HitTest)() = nullptr;
    int(__cdecl* MouseMoved)() = nullptr;
    int(__cdecl* MouseClicked)() = nullptr;
    int(__cdecl* MouseHeld)() = nullptr;
    void(__cdecl* SliderRects)(int, int) = nullptr;
    void(__cdecl* PlaySE)(int, float, float) = nullptr;
    void(__cdecl* QueueDraw)(void*, void*, int) = nullptr;
    void(__cdecl* SliderDraw)(void*) = nullptr;

    SafetyHookInline FrameHook, PageNextHook, PagePrevHook, ReadInputHook;
    SafetyHookMid DrawOptTextHook, OptInitHook;

    using RWGTAMenu::RowKind;

    // Our page: the plugins list (View = RootPage), a plugin's page or a category
    bool PageActive = false;
    bool HelpActive = false, HelpRequested = false, HelpForNotice = false;
    int HelpScroll = 0;
    struct Notice
    {
        std::string version, page;
        bool hub;
    };
    std::optional<Notice> UpdateNotice;
    bool OptionsSeen = false;
    RWGTAMenu::Model Model;
    size_t View = RWGTAMenu::Model::RootPage;
    std::string DisplayLanguage;
    // Wheel notches and Home/End from the window and the key state, taken by the next frame
    std::atomic<int> WheelDelta{ 0 };
    MenuList::WheelNotches Wheel;
    bool HomePressed = false, EndPressed = false;
    std::map<std::pair<uintptr_t, std::string>, std::string> RestartValues;     // restart settings as the game started

    constexpr int VisibleRows = 10; // rows above the help line, Back or Exit among them
    constexpr const char* ScrollUp = "^";
    constexpr const char* ScrollDown = "v";
    // The Options frame ran since the last Present: a menu of the game is on the screen
    bool OptionsFrame = false;
    constexpr int SoundMove = 1, SoundAccept = 3, SoundChange = 5, SoundSlider = 7;

    struct MenuPad
    {
        static constexpr unsigned Help = 1 << 9;
        static constexpr unsigned Directions = (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);
        ActivePad controller;
        DWORD slot = XUSER_MAX_COUNT;
        unsigned held = 0, pressed = 0;
        ULONGLONG repeat = 0;

        static unsigned MappedButtons(const XINPUT_GAMEPAD& pad)
        {
            // The native binding table stores a DirectInput button code followed by its action.
            struct Binding { uint8_t button, action; };
            struct Bindings { const Binding* data; int count, capacity; };
            constexpr WORD buttons[] = { XINPUT_GAMEPAD_Y, XINPUT_GAMEPAD_A, XINPUT_GAMEPAD_B, XINPUT_GAMEPAD_X,
                XINPUT_GAMEPAD_RIGHT_THUMB, 0, XINPUT_GAMEPAD_LEFT_SHOULDER, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                XINPUT_GAMEPAD_BACK, XINPUT_GAMEPAD_START, XINPUT_GAMEPAD_LEFT_THUMB, XINPUT_GAMEPAD_X };
            if (!pPads)
                return 0;
            const auto& bindings = *reinterpret_cast<const Bindings*>(pPads.get() + 0x10);
            if (!bindings.data)
                return 0;
            unsigned result = 0;
            for (int i = 0; i < bindings.count; ++i)
            {
                const auto& binding = bindings.data[i];
                const int index = binding.button - 0x30;
                if (index < 0 || index >= static_cast<int>(std::size(buttons)))
                    continue;
                const bool down = (pad.wButtons & buttons[index]) ||
                    (index == 1 && pad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) ||
                    (index == 5 && pad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
                if (!down)
                    continue;
                switch (binding.action)
                {
                    case 16: result |= 1 << 1; break; // Enter
                    case 17: result |= 1 << 2; break; // Cancel
                    case 3: result |= 1 << 7; break;  // Step right
                    case 2: result |= 1 << 8; break;  // Step left
                    case 4: result |= Help; break;    // Map
                }
            }
            return result;
        }

        static unsigned Actions(const XINPUT_GAMEPAD& pad)
        {
            unsigned actions = MappedButtons(pad);
            if ((pad.wButtons & XINPUT_GAMEPAD_DPAD_UP) || pad.sThumbLY > 16384) actions |= 1 << 3;
            if ((pad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) || pad.sThumbLY < -16384) actions |= 1 << 4;
            if ((pad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) || pad.sThumbLX > 16384) actions |= 1 << 5;
            if ((pad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) || pad.sThumbLX < -16384) actions |= 1 << 6;
            return actions;
        }

        void Poll()
        {
            const auto window = SHDisplay::Get().window;
            if (!window || GetForegroundWindow() != window)
            {
                *this = {};
                return;
            }
            const auto sample = controller.Read(Actions);
            if (slot != sample.slot)
                repeat = 0;
            slot = sample.slot;
            held = Actions(sample.current);
            pressed = held & ~Actions(sample.previous);
            const auto now = GetTickCount64();
            if (pressed & Directions)
                repeat = now + 350;
            else if ((held & Directions) && repeat && now >= repeat)
            {
                pressed |= held & Directions;
                repeat = now + 80;
            }
        }

        int Action(int native) const
        {
            for (int action = 1; action <= 8; ++action)
            {
                if (pressed & (1 << action))
                    return action;
            }
            return native > 0 && native <= 8 && (held & (1 << native)) ? 0 : native;
        }
    } Pad;

    // The screen row of slot 1..VisibleRows
    int RowY(int slot)
    {
        return 25 * slot + 71;
    }

    void Sound(int id)
    {
        if (PlaySE)
            PlaySE(id, 1.0f, 0.0f);
    }

    // The native row text: a native message at the row's label, value, help or centred position
    void DrawOptText(int colour, int row, int kind, int message)
    {
        auto fn = DrawOptTextFn;
        __asm
        {
            push message
            mov eax, colour
            mov edx, row
            mov ecx, kind
            call fn
            add esp, 4
        }
    }

    // The language of the texts: MAIN.TextLanguage, or the one picked in the OPTIONS page (which may not be
    // applied yet)
    std::string LanguageTag()
    {
        auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto")
            return chosen;
        static const char* tags[] = { "en", "en", "fr", "de", "it", "es", "en", "en" };
        int language = aOptionsPendingValues.is_initialized() ? (*aOptionsPendingValues)[2] : 1;
        return tags[std::clamp(language, 0, 7)];
    }

    std::string FormatNotice(const Notice& notice, const std::string& language)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(language);
        auto line = texts.Resolve(notice.hub ? "@update.availablehub" : "@update.availablesite");
        auto replace = [&line](std::string_view token, std::string_view value)
        {
            const auto index = line.find(token);
            if (index != std::string::npos)
                line.replace(index, token.size(), value);
        };
        replace("{name}", texts.Resolve(FixSettings().GetDescription().name));
        replace("{version}", notice.version);
        replace("{page}", notice.page);
        return line;
    }

    // UTF-8 to the game's text: Windows-1252, "\h" for the proportional font, no backslash (an escape)
    std::string ToGame(const std::string& utf8)
    {
        int length = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), int(utf8.size()), wide.data(), length);
        int size = WideCharToMultiByte(1252, 0, wide.data(), length, nullptr, 0, nullptr, nullptr);
        std::string text(size, '\0');
        WideCharToMultiByte(1252, 0, wide.data(), length, text.data(), size, nullptr, nullptr);
        std::replace(text.begin(), text.end(), '\\', '/');
        return "\\h" + text;
    }

    int Width(const std::string& text)
    {
        int widths[8] = {};
        MeasureLines(widths, Encode(text.c_str()));
        return widths[0];
    }

    std::string FitLine(std::string text, int width)
    {
        for (auto& c : text)
        {
            if (c == '\n' || c == '\r')
                c = ' ';
        }
        if (Width(text) <= width)
            return text;
        while (text.size() > 2 && Width(text + "...") > width)
            text.pop_back();
        return text + "...";
    }

    // The default glyph size of the menu text, measured once
    int DefaultGlyph()
    {
        static int size = 0;
        if (!size)
        {
            const std::string sample = "\\hMMMMMMMM";
            const int normal = Width(sample);
            SetGlyphSize(32, 32);
            const int large = Width(sample);
            ClearGlyphSize();
            size = large > 0 ? std::max(8, int(std::lround(32.0 * normal / large))) : 24;
        }
        return size;
    }

    // Text wider than width is drawn condensed to fit, like the game's long titles; the caller clears the glyph size
    void Condense(const std::string& text, int width)
    {
        const int measured = Width(text);
        if (measured > width)
        {
            const int size = DefaultGlyph();
            SetGlyphSize(std::max(8, size * width / measured), size);
        }
    }

    void SetColour(int index)
    {
        auto c = aTextColours->data() + index * 4;
        SetColor(c[0], c[1], c[2], c[3]);
    }

    // Text in a native colour; the selected (5) and title (4) colours get the raised look with a drop shadow
    void DrawText(int colour, const std::string& text, int x, int y, bool centred = false)
    {
        if (colour == 0)
            return;
        if (centred)
            CentreOn();
        if (colour == 4 || colour == 5)
        {
            SetColour(6);
            DrawGlyphs(Encode(text.c_str()), x + 2, y + 2);
            SetColour(colour);
            DrawGlyphs(Encode(text.c_str()), x - 2, y - 2);
        }
        else
        {
            SetColour(colour);
            DrawGlyphs(Encode(text.c_str()), x, y);
        }
        if (centred)
            CentreOff();
    }

    // Rows above or below the window: small marks in the normal text colour right of the list
    void ScrollMarks(bool above, bool below, int shown)
    {
        SetGlyphSize(16, 16);
        if (above)
            DrawText(1, ToGame(ScrollUp), 478, RowY(1) + 4, true);
        if (below)
            DrawText(1, ToGame(ScrollDown), 478, RowY(shown) + 4, true);
        ClearGlyphSize();
    }

    // UTF-8 text split into lines of the game's text no wider than width
    std::vector<std::string> Wrap(const std::string& utf8, int width)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t start = 0;
        while (start <= utf8.size())
        {
            size_t end = utf8.find(' ', start);
            if (end == std::string::npos)
                end = utf8.size();
            auto word = utf8.substr(start, end - start);
            auto candidate = line.empty() ? word : line + " " + word;
            if (!line.empty() && Width(ToGame(candidate)) > width)
            {
                lines.push_back(ToGame(line));
                line = word;
            }
            else
                line = candidate;
            start = end + 1;
        }
        if (!line.empty())
            lines.push_back(ToGame(line));
        return lines;
    }

    // The volume slider of the OPTIONS page, for numbers with a few steps: the game draws it later in the frame from
    // the row's pending value, so that value is swapped in while it draws
    struct SliderData
    {
        uint8_t row;
        uint8_t colour;
        uint8_t value;
    } Sliders[VisibleRows + 1];

    void __cdecl DrawSlider(void* data)
    {
        auto slider = static_cast<SliderData*>(data);
        int8_t saved = (*aOptionsPendingValues)[slider->row];
        (*aOptionsPendingValues)[slider->row] = slider->value;
        SliderDraw(data);
        (*aOptionsPendingValues)[slider->row] = saved;
    }

    constexpr int SliderTicks = 16;

    // The number of positions of a number shown as a slider, 0 for other rows
    int SliderPositions(const RWGTAMenu::Row& row)
    {
        if (row.kind != RowKind::Setting)
            return 0;
        auto& s = *row.setting;
        bool number = s.type == asi_settings::Type::Int || s.type == asi_settings::Type::Float;
        if (!number || !s.min || !s.max || !s.step || *s.step <= 0 || !s.special.empty() || !s.suggestions.empty())
            return 0;
        int positions = int(std::lround((*s.max - *s.min) / *s.step)) + 1;
        return positions >= 2 && positions <= SliderTicks ? positions : 0;
    }

    // The slider's tick of the value in effect, 0..15
    int SliderTick(const RWGTAMenu::Row& row)
    {
        auto& s = *row.setting;
        int positions = SliderPositions(row);
        double value = std::atof(OtherPlugins::Get(*row.plugin, s).c_str());
        int position = std::clamp(int(std::lround((value - *s.min) / *s.step)), 0, positions - 1);
        return int(std::lround(double(position) * (SliderTicks - 1) / (positions - 1)));
    }

    // Sets a slider's value from a tick; true when it changed
    bool SetSliderTick(const RWGTAMenu::Row& row, int tick)
    {
        auto& s = *row.setting;
        int positions = SliderPositions(row);
        int position = int(std::lround(double(tick) * (positions - 1) / (SliderTicks - 1)));
        auto value = asi_settings::text::FormatNumber(*s.min + position * *s.step);
        if (!OtherPlugins::Enabled(*row.plugin, s, false) || value == OtherPlugins::Get(*row.plugin, s))
            return false;
        return OtherPlugins::Set(*row.plugin, s, value) == ASI_SETTINGS_OK;
    }

    // The row's name: settings by their label (a restart is mentioned in the help line), the rest from the model
    std::string Label(const RWGTAMenu::Row& row)
    {
        if (row.kind == RowKind::Setting || row.kind == RowKind::Action)
            return OtherPlugins::Text(*row.plugin, row.setting->label);
        return Model.Label(row);
    }

    // The help line of a row (UTF-8)
    std::string Help(const RWGTAMenu::Row& row)
    {
        switch (row.kind)
        {
        case RowKind::Preset:
        case RowKind::ApplyPreset:
            return Model.Text("menu.presethelp");
        case RowKind::SettingsFile:
            return Model.Text("menu.settingsfilehelp");
        case RowKind::RestoreDefaults:
            return Model.Help(row);
        default:
            break;
        }
        auto help = OtherPlugins::Text(*row.plugin, row.setting->description);
        if (row.kind == RowKind::Setting && row.setting->apply == "restart")
            help += (help.empty() ? "" : ". ") + Model.Text("menu.restarthelp");
        return help;
    }

    // The menu in the language picked in the OPTIONS page; true when the rows were built again
    bool RefreshRows()
    {
        const auto language = LanguageTag();
        if (language != DisplayLanguage && Model.SelectLanguage(language))
        {
            DisplayLanguage = language;
            return true;
        }
        return false;
    }

    bool HasHelp(const RWGTAMenu::Row* row)
    {
        return row && !RWGTAMenu::Model::Opens(*row);
    }

    // Rows without a value (an entry that opens or acts, Back, Exit) are centred like the game's own such rows
    bool Centred(const RWGTAMenu::Row* row)
    {
        return !HasHelp(row) || (!SliderPositions(*row) && Model.Value(*row).empty());
    }

    // The column between labels and values: the game's (labels end at 244, values start 24 further), moved right when
    // a label shown needs the room and the values leave it; a page with a volume slider keeps the game's column
    int LabelColumn = 244;

    void PlaceColumn(const std::vector<const RWGTAMenu::Row*>& rows)
    {
        int labels = 0, values = 0;
        bool slider = false;
        for (auto row : rows)
        {
            if (Centred(row))
                continue;
            labels = std::max(labels, Width(ToGame(Label(*row))));
            if (SliderPositions(*row))
                slider = true;
            else
                values = std::max(values, Width(ToGame(Model.Value(*row))));
        }
        LabelColumn = 244;
        if (!slider && labels > 228)
            LabelColumn = std::clamp(16 + labels, 244, 496 - 24 - values);
    }

    // A centred entry: a plugin or category, or Exit (plugins list) and Back (null row)
    std::string EntryText(const RWGTAMenu::Row* row)
    {
        if (row)
            return Model.Label(*row);
        return Model.Text(View == RWGTAMenu::Model::RootPage ? "menu.exit" : "menu.back");
    }

    void RememberRestartValues(const asi_settings::LoadedPlugin& plugin)
    {
        for (auto& setting : plugin.file.description.settings)
        {
            if (setting.HasValue() && setting.apply == "restart")
                RestartValues.try_emplace({ reinterpret_cast<uintptr_t>(plugin.module), setting.id }, OtherPlugins::Get(plugin, setting));
        }
    }

    // A restart setting differs from the value the game started with
    bool RestartRequired()
    {
        for (auto& page : Model.Pages())
        {
            if (!page.plugin)
                continue;
            for (auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart")
                    continue;
                auto initial = RestartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                if (initial == RestartValues.end())
                    continue;
                const auto value = OtherPlugins::Get(*page.plugin, setting);
                if (initial->second == value)
                    continue;
                if (page.plugin->file.description.id == "SilentHill3.FusionFix")
                {
                    const auto& display = SHDisplay::Get();
                    if ((setting.id == "MAIN.ResX" && value == std::to_string(display.backBufferWidth)) ||
                        (setting.id == "MAIN.ResY" && value == std::to_string(display.backBufferHeight)))
                        continue;
                }
                return true;
            }
        }
        return false;
    }

    void OpenList()
    {
        HMODULE self = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
        DisplayLanguage = LanguageTag();
        Model.OpenMenu(self, DisplayLanguage, VisibleRows);
        for (auto& page : Model.Pages())
        {
            if (page.plugin)
                RememberRestartValues(*page.plugin);
        }
        View = RWGTAMenu::Model::RootPage;
        Model.Window(View).Reset();
        Wheel.Clear();
        WheelDelta = 0;
        PageActive = true;
        Pad = {};
        Sound(SoundMove);
    }

    void LeavePage(int nativeRow)
    {
        PageActive = HelpActive = false;
        Model.Close();
        nOptionsSelection = nOptionsSelectionNext = int8_t(nativeRow);
    }

    // The native title arrows, shared by all pages
    int TitleArrowWidth()
    {
        return std::max({ MsgWidth(45), MsgWidth(46), MsgWidth(136) });
    }

    int __cdecl Frame()
    {
        OptionsSeen = true;
        OptionsFrame = true;
        RainDroplets::MenuOpen(true);
        if (UpdateNotice && FixSettings().Get("MAIN.CheckForUpdates") == "0")
            UpdateNotice.reset();
        if (!PageActive || nOptionsMode != 0)
        {
            if (PageActive)
            {
                PageActive = false;
                Model.Close();
            }
            int result = FrameHook.ccall<int>();
            if (UpdateNotice && nOptionsMode == 0)
            {
                TextLayerFront();
                SetGlyphSize(16, 16);
                const auto lines = Wrap(FormatNotice(*UpdateNotice, LanguageTag()), 480);
                int y = 431 - 18 * (static_cast<int>(lines.size()) - 1);
                for (const auto& line : lines)
                {
                    DrawText(1, line, 256, y, true);
                    y += 18;
                }
                ClearGlyphSize();
                TextLayerBack();
            }
            return result;
        }

        TextReset();
        ReadOptInput();
        nOptionsModeNext = nOptionsMode.get();
        nOptionsSelectionNext = nOptionsSelection.get();
        SetTextBg((*aTextBackground)[0], (*aTextBackground)[1], (*aTextBackground)[2]);
        TextLayerFront();

        const bool rowsChanged = RefreshRows();
        auto& window = Model.Window(View);
        // The rows in the window, slot by slot; a null row is Exit (the plugins list) or Back
        auto shownRows = Model.Shown(View, true);
        PlaceColumn(shownRows);
        int shown = int(shownRows.size());
        const auto at = [&](int slot) { return slot >= 1 && slot <= shown ? shownRows[slot - 1] : nullptr; };
        const RWGTAMenu::Row* selected = at(int(window.Slot()) + 1);
        Pad.Poll();
        int action = Pad.Action(nOptionsAction);
        if (rowsChanged && (action == 1 || action == 5 || action == 6))
            action = 0;
        bool leave = false;
        bool helpPressed = HelpRequested || (Pad.pressed & MenuPad::Help);
        HelpRequested = false;
        if (helpPressed && !HelpActive && (HasHelp(selected) || UpdateNotice))
        {
            HelpActive = true;
            HelpForNotice = !HasHelp(selected);
            HelpScroll = 0;
            Sound(SoundAccept);
        }
        if (HelpActive && (HelpForNotice ? !UpdateNotice : !HasHelp(selected)))
            HelpActive = false;
        if (HelpActive)
        {
            auto text = HelpForNotice ? FormatNotice(*UpdateNotice, DisplayLanguage) : Label(*selected) + ": " + Help(*selected);
            auto lines = Wrap(text, 470);
            int maximum = std::max(0, int(lines.size()) - 12);
            if (action == 3) --HelpScroll;
            if (action == 4) ++HelpScroll;
            HelpScroll = std::clamp(HelpScroll, 0, maximum);
            if (action == 1 || action == 2 || helpPressed)
            {
                // The opening help press belongs only to this view.
                if (action == 1 || action == 2)
                    HelpActive = false;
            }
            OptBackground();
            DrawText(4, ToGame(Model.Text("menu.help")), 256, 56, true);
            SetGlyphSize(20, 20);
            for (int i = 0; i < 12 && i + HelpScroll < int(lines.size()); ++i)
                DrawText(1, lines[i + HelpScroll], 256, 105 + i * 21, true);
            ClearGlyphSize();
            DrawText(5, ToGame(Model.Text("menu.back")), 256, 369, true);
            SetGlyphSize(18, 18);
            if (RestartRequired())
                DrawText(1, FitLine(ToGame(Model.Text("menu.restartwarning")), 480), 256, 425, true);
            ClearGlyphSize();
            ClearRects();
            AddRect(196, 365, 316, 395, 1);
            if (HitTest() == 1 && MouseClicked())
                HelpActive = false;
            TextLayerBack();
            return 0;
        }

        // Mouse: rows, the title arrows and the slider
        ClearRects();
        int arrowWidth = TitleArrowWidth();
        AddRect(241 - arrowWidth / 2 - 30, 54, 241 - arrowWidth / 2, 82, 31);
        AddRect(arrowWidth / 2 + 271, 54, arrowWidth / 2 + 301, 82, 30);
        bool sliderSelected = selected && SliderPositions(*selected);
        if (sliderSelected)
            SliderRects(8, RowY(int(window.Slot()) + 1) - 248);
        else
        {
            for (int slot = 1; slot <= shown; ++slot)
            {
                const auto row = at(slot);
                int y = RowY(slot);
                if (!Centred(row))
                {
                    const auto label = ToGame(Label(*row));
                    int labelWidth = std::min(Width(label), LabelColumn - 16);
                    AddRect(LabelColumn - labelWidth, y, LabelColumn + 224, y + 25, slot);
                }
                else
                {
                    int w = Width(ToGame(row ? Label(*row) : EntryText(row)));
                    AddRect(256 - w / 2, y, 256 + w / 2, y + 25, slot);
                }
            }
        }
        int hit = rowsChanged ? 0 : HitTest();
        const bool click = MouseClicked() != 0;
        if (click && hit == 0 && action == 1)
            action = 0;
        if ((hit == 30 || hit == 31) && click)
            action = hit == 30 ? 7 : 8;
        else if (sliderSelected && hit > 0 && hit <= SliderTicks && MouseHeld())
        {
            if (!RefreshRows() && SetSliderTick(*selected, hit - 1))
                Sound(SoundSlider);
        }
        else if (!sliderSelected && hit > 0 && hit <= shown && (MouseMoved() || click))
        {
            if (window.Point(size_t(hit - 1)))
                Sound(SoundMove);
            selected = at(int(window.Slot()) + 1);
            if (click)
                action = 1;
        }
        // The wheel moves the list a row per notch; Home and End go to its ends
        bool moved = false;
        for (int notches = Wheel.Take(WheelDelta.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
            moved |= window.Scroll(notches > 0 ? -1 : 1);
        if (HomePressed)
            moved |= window.Edge(-1);
        if (EndPressed)
            moved |= window.Edge(1);
        HomePressed = EndPressed = false;
        if (moved)
        {
            Sound(SoundMove);
            selected = at(int(window.Slot()) + 1);
        }

        if ((action == 1 || action == 5 || action == 6) && RefreshRows())
            action = 0;
        switch (action)
        {
        case 2:     // Cancel: back to the list the page was opened from; from the plugins list out of Options
        {
            const auto parent = Model.Back(View);
            if (parent == SIZE_MAX)
                leave = true;
            else
            {
                View = parent;
                Sound(SoundMove);
            }
            break;
        }
        case 3:
            window.Move(-1);
            Sound(SoundMove);
            break;
        case 4:
            window.Move(1);
            Sound(SoundMove);
            break;
        case 7:     // next page: OPTIONS
            Sound(SoundMove);
            LeavePage(1);
            break;
        case 8:     // previous page: Configuration
            Sound(SoundMove);
            LeavePage(22);
            break;
        case 1:
        case 5:
        case 6:
            if (!selected)
            {
                if (action == 1)
                {
                    Sound(SoundAccept);
                    const auto parent = Model.Back(View);
                    if (parent == SIZE_MAX)
                        leave = true;
                    else
                        View = parent;
                }
            }
            else if (RWGTAMenu::Model::Opens(*selected))
            {
                const auto page = action == 1 ? Model.Enter(*selected) : SIZE_MAX;
                if (page != SIZE_MAX)
                {
                    Sound(SoundAccept);
                    View = page;
                }
            }
            else if (sliderSelected)
            {
                if (action != 1 && Model.Change(*selected, action == 5 ? 1 : -1, false))
                    Sound(SoundSlider);
            }
            else if (Model.Change(*selected, action == 1 ? 0 : action == 5 ? 1 : -1, false))
                Sound(action == 1 ? SoundAccept : SoundChange);
            break;
        }

        if (PageActive)
        {
            RefreshRows();
            auto& view = Model.Window(View);
            shownRows = Model.Shown(View, true);
            PlaceColumn(shownRows);
            shown = int(shownRows.size());
            selected = at(int(view.Slot()) + 1);

            // Background and the native title arrows; the native title itself is hidden by the DrawOptText hook
            OptBackground();
            // The title between the arrows, narrower when it's longer than the game's titles
            auto title = ToGame(View == RWGTAMenu::Model::RootPage ? Model.Text("menu.title") : Model.Pages()[View].title);
            SetGlyphSize(30, 30);
            int titleWidth = Width(title), room = arrowWidth + 20;
            if (titleWidth > room)
                SetGlyphSize(std::max(16, 30 * room / titleWidth), 30);
            DrawText(4, title, 256, 56, true);
            ClearGlyphSize();

            for (int slot = 1; slot <= shown; ++slot)
            {
                const auto row = at(slot);
                int colour = slot == int(view.Slot()) + 1 ? 5 : 1;
                int y = RowY(slot);
                if (Centred(row))
                {
                    // Plugins, categories, actions, Back and Exit are centred like the game's own page entries
                    const auto text = ToGame(row ? Label(*row) : EntryText(row));
                    Condense(text, 460);
                    DrawText(colour, text, 256, y, true);
                    ClearGlyphSize();
                    continue;
                }
                auto& entry = *row;
                const auto label = ToGame(Label(entry));
                // Labels never show raised except when selected, like the native rows; long ones are condensed
                Condense(label, LabelColumn - 16);
                DrawText(colour, label, LabelColumn - Width(label), y);
                ClearGlyphSize();
                if (SliderPositions(entry))
                {
                    auto& slider = Sliders[slot];
                    slider.row = uint8_t(slot);
                    slider.colour = uint8_t(colour);
                    slider.value = uint8_t(SliderTick(entry));
                    QueueDraw(reinterpret_cast<void*>(&DrawSlider), &slider, 1);
                }
                else
                {
                    const auto value = ToGame(Model.Value(entry));
                    Condense(value, 496 - LabelColumn - 24);
                    DrawText(colour, value, LabelColumn + 24, y);
                    ClearGlyphSize();
                }
            }
            // More rows above or below the window
            ScrollMarks(view.Above(), view.Below(), shown);

            // The help of the selected row where the game shows its help line, in up to three lines of the help page's size; the
            // complete text opens with F1, whose hint sits below with the restart warning
            std::vector<std::string> help;
            SetGlyphSize(20, 20);
            if (HasHelp(selected))
                help = Wrap(Help(*selected), 480);
            else if (!selected && View == RWGTAMenu::Model::RootPage)
                help = Wrap(Model.Text("menu.exithelp"), 480);
            if (help.size() > 3)
            {
                help.resize(3);
                help[2] = FitLine(help[2] + " ...", 480);
            }
            for (size_t i = 0; i < help.size(); ++i)
                DrawText(1, help[i], 256, 354 + int(i) * 20, true);
            SetGlyphSize(16, 16);
            DrawText(1, ToGame(Model.Text("menu.helphint")), 256, 414, true);
            if (RestartRequired())
                DrawText(1, FitLine(ToGame(Model.Text("menu.restartwarning")), 480), 256, 432, true);
            ClearGlyphSize();
        }

        TextLayerBack();

        if (leave)
        {
            // Native exit: the confirm dialog for native changes (it closes at once when there are none)
            LeavePage(10);
            nOptionsMode = nOptionsModeNext = 3;
        }
        return 0;
    }

    template<class T>
    void Resolve(T& target, const char* signature, int offset = 0)
    {
        target = reinterpret_cast<T>(hook::get_pattern(signature, offset));
    }

    template<class T>
    void ResolveCall(T& target, const char* signature, int offset)
    {
        target = reinterpret_cast<T>(injector::GetBranchDestination(hook::get_pattern(signature, offset)).as_int());
    }
}

class Menu
{
public:
    Menu()
    {
        WFP::onInitEvent() += []()
        {
            HMODULE self = nullptr;
            GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&OpenList), &self);
            for (auto& plugin : asi_settings::FindLoadedPlugins(self, "en"))
                RememberRestartValues(plugin);

            auto frame = hook::pattern("53 56 57 E8 ? ? ? ? E8 ? ? ? ? A0 ? ? ? ? 8A 0D ? ? ? ?");
            nOptionsMode.SetAddress(*frame.get_first<int8_t*>(0x0E));
            nOptionsSelection.SetAddress(*frame.get_first<int8_t*>(0x14));
            nOptionsSelectionNext.SetAddress(*frame.get_first<int8_t*>(0x14) + 1);
            nOptionsModeNext.SetAddress(*frame.get_first<int8_t*>(0x0E) + 1);
            nOptionsAction.SetAddress(*frame.get_first<int8_t*>(0x0E) + 2);
            aOptionsPendingValues.SetAddress(*frame.get_first<int8_t*>(0x14) + 0x27);
            TextReset = reinterpret_cast<decltype(TextReset)>(injector::GetBranchDestination(frame.get_first(3)).as_int());
            ReadOptInput = reinterpret_cast<decltype(ReadOptInput)>(injector::GetBranchDestination(frame.get_first(8)).as_int());

            auto frameStart = frame.get_first<uint8_t>();
            aTextBackground.SetAddress(*reinterpret_cast<uint8_t**>(frameStart + 0x36));     // mov cl, [text background red]
            SetTextBg = reinterpret_cast<decltype(SetTextBg)>(injector::GetBranchDestination(frameStart + 0x3D).as_int());
            TextLayerFront = reinterpret_cast<decltype(TextLayerFront)>(injector::GetBranchDestination(frameStart + 0x45).as_int());

            Resolve(TextLayerBack, "B8 04 00 00 00 66 39 05 ? ? ? ? 7D");
            Resolve(OptBackground, "A0 ? ? ? ? 83 EC 14 3C 01 0F 84");
            Resolve(DrawOptTextFn, "51 53 56 8B D8 85 DB 57 8B F1 8B FA");
            Resolve(SetColor, "0F B6 4C 24 04 0F B6 54 24 10 33 C0");
            Resolve(Encode, "83 EC 0C 8B 44 24 10 53 8A 18 55 56");
            // DrawGlyphs and MeasureLines are bound in Init, before the button icons hook them
            Resolve(CentreOn, "81 0D ? ? ? ? 00 01 00 00 C3 90");
            Resolve(CentreOff, "81 25 ? ? ? ? FF FE FF FF C3 90 90 90 90 90 81");
            Resolve(SetGlyphSize, "A1 ? ? ? ? 66 8B 4C 24 08 0D 00");
            Resolve(ClearGlyphSize, "81 25 ? ? ? ? FF F7 FF FF C3 90");
            Resolve(MsgWidth, "83 EC 20 57 33 C0 B9 08 00 00 00 8D");
            Resolve(ClearRects, "A1 ? ? ? ? 85 C0 74 1A 56 8D 9B");
            Resolve(AddRect, "E8 ? ? ? ? 6A 18 E8 ? ? ? ? 8B");
            Resolve(HitTest, "E8 ? ? ? ? 85 C0 75 01 C3 56 E8");
            Resolve(MouseMoved, "8B 0D ? ? ? ? 33 C0 83 F9 03 0F");
            ResolveCall(SliderRects, "6A FE 6A 08 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 85 C0 74 2D E8", 4);
            ResolveCall(MouseHeld, "6A FE 6A 08 E8 ? ? ? ? 83 C4 08 E8 ? ? ? ? 85 C0 74 2D E8", 0x0C);
            auto slider = hook::pattern("6A 01 68 ? ? ? ? 68 ? ? ? ? C6 05 ? ? ? ? 07 88 1D ? ? ? ? E8");
            SliderDraw = *slider.get_first<decltype(SliderDraw)>(8);
            QueueDraw = reinterpret_cast<decltype(QueueDraw)>(injector::GetBranchDestination(slider.get_first(0x19)).as_int());
            auto colours = hook::pattern("33 C9 8A 0C 85 ? ? ? ? 33 D2 8A");
            aTextColours.SetAddress(*colours.get_first<uint8_t*>(5) - 3);
            MouseClicked = reinterpret_cast<decltype(MouseClicked)>(injector::GetBranchDestination(frameStart + 0xEE).as_int());

            auto pageNext = hook::pattern("53 8A 1D ? ? ? ? 0F BE C3 33 C9 83 F8 0A 7E 07 83 F8 15 0F 9F C1 41 83 E9 00 74 24");
            auto pagePrev = hook::pattern("53 8A 1D ? ? ? ? 0F BE C3 33 C9 83 F8 0A 7E 07 83 F8 15 0F 9F C1 41 83 E9 00 74 2B");
            PlaySE = reinterpret_cast<decltype(PlaySE)>(injector::GetBranchDestination(pageNext.get_first(0x68)).as_int());
            auto optInitEnd = hook::pattern("C6 05 ? ? ? ? 00 C7 05 ? ? ? ? 00 00 00 00 C6 05 ? ? ? ? 00 5B C3");

            FrameHook = safetyhook::create_inline(frame.get_first(), &Frame);
            SHDisplay::OnWindowMessage().push_back([](HWND, UINT message, WPARAM key, LPARAM flags, LRESULT& result)
            {
                if (PageActive && message == WM_MOUSEWHEEL)
                    WheelDelta += GET_WHEEL_DELTA_WPARAM(key);
                if (PageActive && message == WM_KEYDOWN && key == VK_F1)
                {
                    if (!(flags & (1L << 30)))
                        HelpRequested = true;
                    result = 0;
                    return true;
                }
                return false;
            });
            RWGTA::UpdateCheck::Config updates;
            updates.pluginId = "SilentHill3.FusionFix";
            updates.fileVersion = rsc_FileVersion;
            updates.settings = &FixSettings();
            updates.playable = []() { return OptionsSeen && nOptionsMode == 0; };
            updates.show = [](const std::wstring&) {};
            updates.format = [](const std::string& version, bool hub)
            {
                UpdateNotice = Notice{ version, RWGTA::UpdateCheck::Page(), hub };
                return asi_settings::text::Widen(FormatNotice(*UpdateNotice, LanguageTag()));
            };
            RWGTA::UpdateCheck::Init(std::move(updates));
            SHDisplay::OnPresent().push_back([]()
            {
                RWGTA::UpdateCheck::Tick();
                // The droplets stay off the Options screen while it shows
                if (!OptionsFrame)
                    RainDroplets::MenuOpen(false);
                OptionsFrame = false;
            });

            // Page Up / Page Down switch pages too (the game only uses its step keys and the shoulder buttons)
            ReadInputHook = safetyhook::create_inline(ReadOptInput, static_cast<void(__cdecl*)()>([]()
            {
                ReadInputHook.ccall<void>();
                static bool previousHeld = false, nextHeld = false, helpHeld = false;
                DWORD process = 0;
                GetWindowThreadProcessId(GetForegroundWindow(), &process);
                bool focused = process == GetCurrentProcessId();
                bool previous = focused && (GetAsyncKeyState(VK_PRIOR) & 0x8000);
                bool next = focused && (GetAsyncKeyState(VK_NEXT) & 0x8000);
                bool help = focused && (GetAsyncKeyState(VK_F1) & 0x8000);
                static bool homeHeld = false, endHeld = false;
                const bool home = focused && (GetAsyncKeyState(VK_HOME) & 0x8000);
                const bool end = focused && (GetAsyncKeyState(VK_END) & 0x8000);
                HomePressed |= PageActive && home && !homeHeld;
                EndPressed |= PageActive && end && !endHeld;
                homeHeld = home;
                endHeld = end;
                if (PageActive && help && !helpHeld)
                    HelpRequested = true;
                if (previous && !previousHeld)
                    nOptionsAction = 8;
                else if (next && !nextHeld)
                    nOptionsAction = 7;
                previousHeld = previous;
                nextHeld = next;
                helpHeld = help;
            }));

            // From Configuration the next page is ours, and from OPTIONS the previous one
            PageNextHook = safetyhook::create_inline(pageNext.get_first(), static_cast<int(__cdecl*)()>([]() -> int
            {
                if (nOptionsSelection > 21 && !PageActive)
                {
                    OpenList();
                    return 0;
                }
                return PageNextHook.ccall<int>();
            }));
            PagePrevHook = safetyhook::create_inline(pagePrev.get_first(), static_cast<int(__cdecl*)()>([]() -> int
            {
                if (nOptionsSelection <= 10 && !PageActive)
                {
                    OpenList();
                    return 0;
                }
                return PagePrevHook.ccall<int>();
            }));

            // The native page title isn't drawn under ours
            DrawOptTextHook = safetyhook::create_mid(DrawOptTextFn, [](SafetyHookContext& ctx)
            {
                if (PageActive && ctx.ecx == 1)
                    ctx.eax = 0;
            });

            // Options always opens on the native first page
            OptInitHook = safetyhook::create_mid(optInitEnd.get_first(0x11), [](SafetyHookContext&)
            {
                PageActive = HelpActive = false;
                Model.Close();
            });
        };
    }
} Menu;
