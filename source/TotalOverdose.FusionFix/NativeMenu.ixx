module;

#include "stdafx.h"
#include "FixSettings.hpp"
#include <Xinput.h>
#include <atomic>
#include <string>
#include <vector>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"
#include "Shared/RainDroplets.hpp"

export module NativeMenu;

// The Plugins page: one more row, Plugins, in the game's Options list. It opens the game's own Settings page and turns
// its ten rows into the plugins list, a plugin's page or a category: the game's font, colours, selection bar, value
// arrows, scroll arrows, sounds and help line. The menu scripts are compiled into the exe; their state is one object.
export namespace TODMenu
{
    // The game's functions
    struct Api
    {
        char(__fastcall* setIndex)(uintptr_t, void*, int) = nullptr;
        int(__fastcall* setText)(uintptr_t, void*, const char*) = nullptr;
        void* (__cdecl* alloc)(int, size_t) = nullptr;
        void(__cdecl* free)(void*) = nullptr;
        int(__fastcall* dirty)(uintptr_t, void*) = nullptr;
        void(__fastcall* colour)(uintptr_t, void*, const float*) = nullptr;
        float* (__fastcall* getPos)(uintptr_t, void*, float*) = nullptr;
        void(__fastcall* setPos)(uintptr_t, void*, const float*) = nullptr;
        int(__fastcall* width)(uintptr_t, void*) = nullptr;
        int(__fastcall* sound)(uintptr_t, void*, int, const void*) = nullptr;
        uintptr_t* current = nullptr;       // the script object a compiled script runs on
        uintptr_t* game = nullptr;          // the game's main script object
        uintptr_t* entityList = nullptr;    // list(Entity)
        uintptr_t* optionList = nullptr;    // list(s_menuoption)
    } inline api;

    template <typename T = uintptr_t>
    inline T& At(uintptr_t base, int offset) { return *reinterpret_cast<T*>(base + offset); }

    // Script lists: element count, element k (the elements of these lists are pointers)
    inline int Count(uintptr_t list, uintptr_t type) { return list ? At<int>(list, 8) / At<int>(type, 36) : 0; }
    inline uintptr_t Element(uintptr_t list, uintptr_t type, int k)
    {
        return At(At(At(list, 4), 4) + 4 * k * At<int>(type, 36), 0);
    }

    // The menu object's fields
    namespace Field
    {
        constexpr int Bar = 112, ArrowLeft = 144, ArrowLeftBack = 148, ArrowRight = 152, ArrowRightBack = 156;
        constexpr int MoreUp = 160, MoreUpBack = 164, MoreDown = 168, MoreDownBack = 172;
        constexpr int Names = 176, ValueTexts = 188, ValueStrings = 192;
        constexpr int RowCount = 196, Slot = 204, First = 208, Scrolls = 216;
        constexpr int Options = 376, OptionSelected = 380, OptionCount = 584;
        constexpr int Screen = 592;     // 1: the Options list
        constexpr int Event = 384, LastEvent = 388, RepeatDelay = 392, EventTime = 396;
        constexpr int OptionsPage = 644, SettingsPage = 652, MouseClick = 904, MouseAction = 908;
    }

    // Events of the menu input: 0 up, 1 down, 2 left, 3 right, 4 accept, 5 none, 6 back, 7 restore defaults, 8 mouse
    enum Event { Up = 0, Down = 1, Left = 2, Right = 3, Accept = 4, NoEvent = 5, Back = 6, Restore = 7, Mouse = 8, ScrollDown = 100, ScrollUp = 101 };
    // Click actions of the menu's controls
    enum Click { ClickScrollDown = 1, ClickScrollUp = 2, ClickRight = 3, ClickLeft = 4, ClickBack = 5, ClickAccept = 6 };

    inline uintptr_t Menu() { return At(*api.current, 24); }
    inline uintptr_t SoundObject() { return At(At(At(At(*api.game, 24), 596), 24), 60); }
    inline int Time() { return At<int>(At(At(At(*api.game, 24), 628), 24), 8); }

    inline void Play(int id)
    {
        struct { int id; float volume; } args{ id, 1.0f };
        api.sound(SoundObject(), nullptr, -19, &args);
    }

    // Text boxes draw a wide string at +184 when it is set (freed by the game when the box gets a text index again)
    inline void SetWide(uintptr_t box, const std::wstring& text)
    {
        if (!box)
            return;
        auto& slot = At<wchar_t*>(box, 184);
        if (slot && text == slot)
            return;
        if (slot)
            api.free(slot);
        auto buffer = static_cast<wchar_t*>(api.alloc(0, (text.size() + 1) * sizeof(wchar_t)));
        std::memcpy(buffer, text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        slot = buffer;
        At<uint8_t>(box, 20) |= 8;
        if (auto node = At(box, 48))
            api.dirty(node, nullptr);
    }

    inline void SetString(uintptr_t box, const char* text)
    {
        auto string = At<int*>(box, 172);
        if (string && *string != 0)
            api.setText(box, nullptr, text);
    }

    // Colours as the game sets them (the game's own refreshes may set them again: Repaint forgets what was set)
    inline std::unordered_map<uintptr_t, uint32_t> colours;
    inline void Colour(uintptr_t box, float r, float g, float b)
    {
        const uint32_t key = uint32_t(r * 255.0f) | uint32_t(g * 255.0f) << 8 | uint32_t(b * 255.0f) << 16;
        if (auto it = colours.find(box); it != colours.end() && it->second == key)
            return;
        colours[box] = key;
        const float rgba[4]{ r, g, b, 0.0f };
        api.colour(box, nullptr, rgba);
    }
    inline void Repaint() { colours.clear(); }

    inline void Position(uintptr_t entity, float out[4])
    {
        out[0] = out[1] = out[2] = out[3] = 0.0f;
        api.getPos(entity, nullptr, out);
    }

    inline void Place(uintptr_t entity, float x, float y, float z)
    {
        float now[4];
        Position(entity, now);
        if (now[0] == x && now[1] == y)
            return;
        const float pos[4]{ x, y, z, 0.0f };
        api.setPos(entity, nullptr, pos);
    }

    // Entities are hidden by bit 0 of their flags (+40), set through their table like the scripts' show and hide
    inline void Visible(uintptr_t entity, bool shown)
    {
        if (!entity)
            return;
        const auto flags = At<uint32_t>(entity, 40);
        if (((flags & 1) == 0) == shown)
            return;
        auto table = *reinterpret_cast<void(__fastcall***)(uintptr_t, void*, uint32_t)>(entity);
        table[4](entity, nullptr, (flags & 0xFFE) | (shown ? 0 : 1));
    }

    inline HMODULE ThisModule()
    {
        HMODULE module = nullptr;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&ThisModule), &module);
        return module;
    }

    inline std::string Language()
    {
        std::string language = FixSettings().Get("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            wchar_t locale[LOCALE_NAME_MAX_LENGTH]{};
            language = GetUserDefaultLocaleName(locale, LOCALE_NAME_MAX_LENGTH) ? asi_settings::text::Narrow(locale) : "en";
        }
        return language;
    }

    // The game's fonts hold Latin-1
    inline std::wstring Game(const std::string& utf8)
    {
        auto wide = asi_settings::text::Widen(utf8);
        for (auto& c : wide)
        {
            if (c >= 0x100)
            {
                char mapped = '?';
                WideCharToMultiByte(1252, 0, &c, 1, &mapped, 1, "?", nullptr);
                c = static_cast<unsigned char>(mapped);
            }
        }
        return wide;
    }

    inline std::wstring Upper(std::wstring text)
    {
        if (!text.empty())
            CharUpperBuffW(text.data(), DWORD(text.size()));
        return text;
    }

    constexpr size_t Slots = 10;
    constexpr float LineHeight = 22.0f;     // the help font's lines, in the menus' 800x600 units
    constexpr float RowHeight = 25.0f;      // the Settings rows

    struct State
    {
        bool returning = false;
        bool pending = false, open = false, expanded = false, armed = false;
        std::vector<uintptr_t> prompts;     // the game's key line entries hidden while the page is open
        float helpAt[3]{}, helpY = 0.0f;    // where the game shows its help line, its text and whether it showed
        int helpIndex = -1;
        bool helpShown = false;
        std::wstring helpText;
        bool helpExpanded = false;
        size_t helpPage = SIZE_MAX, helpRows = 0;
        std::vector<std::wstring> helpLines;
        size_t page = RWGTAMenu::Model::RootPage;
        int slot = -1;
        uintptr_t menu = 0;     // the menu object the page is on
        uintptr_t header = 0;   // the screen's title object
        std::atomic<int> wheel{ 0 };
        MenuList::WheelNotches notches;
        MenuList::Repeat pageUp, pageDown, stickUp, stickDown, stickLeft, stickRight;
        MenuList::StickAxis stickX, stickY;
        bool held[4]{};
        HWND window = nullptr;
    } inline state;

    inline RWGTAMenu::Model model;

    inline bool Focused() { return state.window && GetForegroundWindow() == state.window; }
    inline bool Key(int vk) { return Focused() && (GetAsyncKeyState(vk) & 0x8000) != 0; }

    inline std::wstring Label(const RWGTAMenu::Row& row)
    {
        auto label = Upper(Game(model.Label(row)));
        if (row.kind == RWGTAMenu::RowKind::Setting && row.setting && row.setting->apply == "restart")
            label += L" *";
        return label;
    }

    inline std::wstring PageTitle()
    {
        if (state.page == RWGTAMenu::Model::RootPage || state.page >= model.Pages().size())
            return Upper(Game(model.Text("menu.plugins")));
        return Upper(Game(model.Pages()[state.page].title));
    }

    inline float Width(uintptr_t box, const std::wstring& text)
    {
        SetWide(box, text);
        return float(api.width(box, nullptr));
    }

    // The help in lines of the given width
    inline std::vector<std::wstring> Lines(uintptr_t box, const std::wstring& text, float width)
    {
        std::vector<std::wstring> lines;
        std::wstring line;
        size_t start = 0;
        while (start <= text.size())
        {
            auto next = text.find(L' ', start);
            auto word = text.substr(start, next == std::wstring::npos ? std::wstring::npos : next - start);
            auto candidate = line.empty() ? word : line + L" " + word;
            if (!line.empty() && Width(box, candidate) > width)
            {
                lines.push_back(line);
                line = word;
            }
            else
                line = candidate;
            if (next == std::wstring::npos)
                break;
            start = next + 1;
        }
        if (!line.empty())
            lines.push_back(line);
        return lines;
    }

    // Help that doesn't fit stops at its last whole sentence within the lines (F1 shows all of it)
    inline std::vector<std::wstring> Fit(uintptr_t box, const std::wstring& text, float width, size_t count)
    {
        auto lines = Lines(box, text, width);
        if (lines.size() <= count)
            return lines;
        for (size_t end = text.find_last_of(L".!?"); end != std::wstring::npos && end > 0; end = text.find_last_of(L".!?", end - 1))
        {
            auto part = Lines(box, text.substr(0, end + 1), width);
            if (part.size() <= count)
                return part;
        }
        lines.resize(count);
        return lines;
    }

    inline void Open()
    {
        model.OpenMenu(ThisModule(), Language(), Slots);
        state.page = RWGTAMenu::Model::RootPage;
        state.open = true;
        state.expanded = false;
        state.slot = -1;
        state.armed = false;
        state.helpY = 0.0f;
        state.helpText.clear();
        state.helpLines.clear();
        state.notches.Clear();
        state.wheel = 0;
    }

    inline void Close()
    {
        if (!state.open)
            return;
        state.open = false;
        model.Close();
        for (auto prompt : state.prompts)
            Visible(prompt, true);
        state.prompts.clear();
        // The game's help line as it was
        if (state.header && state.helpY)
        {
            const auto help = At(state.header, 0x9C);
            api.setIndex(help, nullptr, state.helpIndex);
            Place(help, state.helpAt[0], state.helpY, state.helpAt[2]);
            Visible(help, state.helpShown);
        }
    }

    // The game's page, filled with the model's rows
    inline void Draw(uintptr_t menu)
    {
        auto& pages = model.Pages();
        if (state.page >= pages.size())
            state.page = RWGTAMenu::Model::RootPage;
        auto& list = model.Window(state.page);
        list.SetSlots(Slots);
        auto shown = model.Shown(state.page, false);
        const auto entities = *api.entityList;
        const auto names = At(menu, Field::Names), values = At(menu, Field::ValueTexts), strings = At(menu, Field::ValueStrings);
        const int slots = std::min({ Count(names, entities), Count(values, entities), int(Slots) });
        const int selected = int(list.Slot());

        At<int>(menu, Field::RowCount) = int(shown.size());
        At<int>(menu, Field::First) = 0;
        At<int>(menu, Field::Scrolls) = 0;
        At<int>(menu, Field::Slot) = selected;
        state.slot = selected;
        // The selection bar behind the selected row, where the game puts it
        if (selected < slots)
        {
            float pos[4];
            Position(Element(names, entities, selected), pos);
            Place(At(menu, Field::Bar), 50.0f, pos[1], 0.0f);
        }

        // The help line
        float helpTop = FLT_MAX;
        const auto help = state.header ? At(state.header, 0x9C) : 0;
        std::wstring text;
        if (selected < int(shown.size()) && shown[selected])
            text = Game(model.Help(*shown[selected]));
        if (text.empty() && model.RestartRequired())
            text = Game(model.Text("menu.restartwarning"));
        if (help)
        {
            // The game's own line under the list about the selected setting; more lines grow upwards from it
            if (!state.helpY)
            {
                float pos[4];
                Position(help, pos);
                std::copy_n(pos, 3, state.helpAt);
                state.helpY = pos[1];
                state.helpIndex = int(At<uint32_t>(help, 216) << 5) >> 21;
                state.helpShown = !(At<uint32_t>(help, 40) & 1);
            }
            if (text != state.helpText || state.expanded != state.helpExpanded || state.page != state.helpPage || shown.size() != state.helpRows)
            {
                state.helpText = text;
                state.helpPage = state.page;
                state.helpRows = shown.size();
                state.helpExpanded = state.expanded;
                const float width = 700.0f - state.helpAt[0];     // to the box's right margin
                // As many lines as there is room for under the rows
                float last[4];
                Position(Element(names, entities, std::max(0, std::min(slots, int(shown.size())) - 1)), last);
                const float free = state.helpY - (last[1] + RowHeight);
                const size_t room = size_t(std::clamp(int(free / LineHeight) + 1, 1, 2));
                state.helpLines = text.empty() ? std::vector<std::wstring>{} : (state.expanded ? Lines(help, text, width) : Fit(help, text, width, room));
            }
            std::wstring joined;
            for (auto& line : state.helpLines)
                joined += (joined.empty() ? L"" : L"\n") + line;
            SetWide(help, joined);
            helpTop = state.helpY - LineHeight * float(std::max<size_t>(state.helpLines.size(), 1) - 1);
            if (selected < slots && !state.helpLines.empty())
            {
                // Not over the selected row
                float pos[4];
                Position(Element(names, entities, selected), pos);
                helpTop = std::max(helpTop, std::min(state.helpY, pos[1] + LineHeight * 0.7f));
            }
            Place(help, state.helpAt[0], helpTop, state.helpAt[2]);
            Visible(help, !joined.empty());
            if (state.helpLines.empty())
                helpTop = FLT_MAX;
        }

        // Values in the text column; the game's number column stays empty
        for (int i = 0; i < Count(strings, entities); ++i)
        {
            SetString(Element(strings, entities, i), "");
            Visible(Element(strings, entities, i), false);
        }
        uintptr_t selectedValue = 0;
        for (int i = 0; i < slots; ++i)
        {
            const auto name = Element(names, entities, i);
            const auto value = Element(values, entities, i);
            const RWGTAMenu::Row* row = i < int(shown.size()) ? shown[i] : nullptr;
            // Rows under the help lines give way while they are shown
            float pos[4];
            Position(name, pos);
            if (i != selected && pos[1] + RowHeight > helpTop)
                row = nullptr;
            SetWide(name, row ? Label(*row) : L"");
            std::wstring text;
            if (row && !RWGTAMenu::Model::Opens(*row))
                text = Upper(Game(model.Value(*row)));
            SetWide(value, text);
            Visible(value, true);
            const bool enabled = row && model.Enabled(*row, false);
            // The game's colours: selected red, others black, rows that can't change brown
            if (i == selected)
            {
                if (enabled)
                    Colour(name, 0.8f, 0.15f, 0.0f);
                else
                    Colour(name, 0.47f, 0.28f, 0.157f);
                Colour(value, 0.8f, 0.14f, 0.0f);
                if (!text.empty() && enabled)
                    selectedValue = value;
            }
            else
            {
                if (enabled || !row)
                    Colour(name, 0.0f, 0.0f, 0.0f);
                else
                    Colour(name, 0.47f, 0.3f, 0.157f);
                Colour(value, 0.0f, 0.0f, 0.0f);
            }
        }

        // The value arrows around the selected value, placed like the game places them
        if (selectedValue)
        {
            float pos[4];
            Position(selectedValue, pos);
            const float width = float(api.width(selectedValue, nullptr));
            float left[4], back[4];
            Position(At(menu, Field::ArrowLeft), left);
            Position(At(menu, Field::ArrowLeftBack), back);
            Place(At(menu, Field::ArrowLeft), left[0], pos[1] - 1.0f, left[2]);
            Place(At(menu, Field::ArrowLeftBack), back[0], pos[1] + 1.0f, back[2]);
            Place(At(menu, Field::ArrowRight), pos[0] + width + 6.0f, pos[1] - 1.0f, 1.0f);
            Place(At(menu, Field::ArrowRightBack), pos[0] + width + 8.0f, pos[1] + 1.0f, 1.0f);
        }
        for (auto field : { Field::ArrowLeft, Field::ArrowLeftBack, Field::ArrowRight, Field::ArrowRightBack })
            Visible(At(menu, field), selectedValue != 0);

        // The list's own scroll arrows; the lower one in the gap between the last row and the help line
        if (slots > 0)
        {
            float first[4], last[4], up[4], upBack[4], down[4], downBack[4];
            Position(Element(names, entities, 0), first);
            Position(Element(names, entities, slots - 1), last);
            Position(At(menu, Field::MoreUp), up);
            Position(At(menu, Field::MoreUpBack), upBack);
            Position(At(menu, Field::MoreDown), down);
            Position(At(menu, Field::MoreDownBack), downBack);
            Place(At(menu, Field::MoreDown), down[0], last[1] + (first[1] - up[1]) + RowHeight * 0.55f, down[2]);
            Place(At(menu, Field::MoreDownBack), downBack[0], last[1] + (first[1] - upBack[1]) + RowHeight * 0.55f, downBack[2]);
        }
        Visible(At(menu, Field::MoreUp), list.Above());
        Visible(At(menu, Field::MoreUpBack), list.Above());
        Visible(At(menu, Field::MoreDown), list.Below());
        Visible(At(menu, Field::MoreDownBack), list.Below());

        // Title
        if (state.header)
        {
            const auto title = PageTitle();
            SetWide(At(state.header, 0xB0), title);
            SetWide(At(state.header, 0xB4), title);
        }
        // The key line: Back only (Restore defaults is a row of the plugin pages)
        if (state.header)
        {
            for (int field : { 0x44, 0x48, 0x54, 0x60 })
            {
                const auto prompt = At(state.header, field);
                if (prompt && !(At<uint32_t>(prompt, 40) & 1))
                {
                    state.prompts.push_back(prompt);
                    Visible(prompt, false);
                }
            }
        }
    }

    // Keys the game's menus don't use: page keys, Home/End, F1, pad shoulders
    enum Extra { PageUp, PageDown, Home, End, Help, StickUp, StickDown, StickLeft, StickRight };
    inline std::vector<Extra> PollExtra()
    {
        std::vector<Extra> out;
        XINPUT_STATE pad{};
        DWORD slot = 0;
        const bool padOn = Focused() && PadInput::Read(pad, slot);
        const auto buttons = padOn ? pad.Gamepad.wButtons : 0;
        // The fix's controller layout sends the d-pad as arrow and page keys together: those are the arrows
        const bool pageUp = (Key(VK_PRIOR) && !Key(VK_UP)) || (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER);
        const bool pageDown = (Key(VK_NEXT) && !Key(VK_DOWN)) || (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
        const bool home = Key(VK_HOME), end = Key(VK_END), help = Key(VK_F1);
        const auto now = GetTickCount64();
        // Keys held when the page opened act only after they are let go
        if (!state.armed)
        {
            if (!pageUp && !pageDown && !home && !end && !help)
                state.armed = true;
            return out;
        }
        // The left stick moves like the d-pad, at the game's menu rate
        const int sx = padOn ? state.stickX.Direction(pad.Gamepad.sThumbLX) : 0;
        const int sy = padOn ? state.stickY.Direction(pad.Gamepad.sThumbLY) : 0;
        if (state.stickUp.Step(sy > 0, now)) out.push_back(StickUp);
        if (state.stickDown.Step(sy < 0, now)) out.push_back(StickDown);
        if (state.stickLeft.Step(sx < 0, now)) out.push_back(StickLeft);
        if (state.stickRight.Step(sx > 0, now)) out.push_back(StickRight);
        if (state.pageUp.Step(pageUp, now)) out.push_back(PageUp);
        if (state.pageDown.Step(pageDown, now)) out.push_back(PageDown);
        auto edge = [&](int i, bool down) { const bool pressed = down && !state.held[i]; state.held[i] = down; return pressed; };
        if (edge(0, home)) out.push_back(Home);
        if (edge(1, end)) out.push_back(End);
        if (edge(2, help)) out.push_back(Help);
        return out;
    }

    inline void Settle(size_t before, size_t firstBefore, size_t pageBefore)
    {
        if (state.page != pageBefore)
        {
            state.expanded = false;
            state.slot = -1;
        }
        else
        {
            auto& window = model.Window(state.page);
            model.ListRows(state.page, false);
            if (window.Selected() != before || window.First() != firstBefore)
                Play(90);
        }
    }

    // The page has the game's menu events; Back on the plugins list goes to the game (false)
    inline bool Handle(uintptr_t menu)
    {
        auto& list = model.Window(state.page);
        list.SetSlots(Slots);
        auto shown = model.Shown(state.page, false);
        auto current = [&]() -> const RWGTAMenu::Row*
        {
            auto rows = model.Shown(state.page, false);
            auto& window = model.Window(state.page);
            return window.Slot() < rows.size() ? rows[window.Slot()] : nullptr;
        };

        int event = At<int>(menu, Field::Event);
        const int last = At<int>(menu, Field::LastEvent);
        if (event == Mouse)
        {
            event = NoEvent;
            if (At<int>(menu, Field::MouseClick))
            {
                switch (At<int>(menu, Field::MouseAction))
                {
                    case ClickScrollDown: event = ScrollDown; break;
                    case ClickScrollUp: event = ScrollUp; break;
                    case ClickRight: event = Right; break;
                    case ClickLeft: event = Left; break;
                    case ClickBack: event = Back; break;
                    case ClickAccept: event = Accept; break;
                    default: break;
                }
                At<int>(menu, Field::MouseClick) = 0;
            }
        }
        // Held directions repeat at the game's own rate (the game sends them every frame they are held)
        bool act = event != NoEvent;
        const bool direction = event == Up || event == Down || event == Left || event == Right;
        if (act && direction && event == last && Time() - At<int>(menu, Field::EventTime) <= At<int>(menu, Field::RepeatDelay))
            act = false;
        if (act)
            At<int>(menu, Field::EventTime) = Time();
        At<int>(menu, Field::LastEvent) = At<int>(menu, Field::Event);

        const size_t before = list.Selected(), firstBefore = list.First();
        const size_t pageBefore = state.page;
        if (act)
        {
            switch (event)
            {
                case Up: list.Move(-1); break;
                case Down: list.Move(1); break;
                case ScrollDown: list.Scroll(1); break;
                case ScrollUp: list.Scroll(-1); break;
                case Left: case Right:
                    if (auto row = current(); row && !RWGTAMenu::Model::Opens(*row) && model.Enabled(*row, false))
                    {
                        if (model.Change(*row, event == Left ? -1 : 1, false))
                            Play(90);
                    }
                    break;
                case Accept:
                    if (auto row = current())
                    {
                        if (RWGTAMenu::Model::Opens(*row))
                        {
                            if (auto target = model.Enter(*row); target != SIZE_MAX)
                                state.page = target;
                        }
                        else if (model.Enabled(*row, false))
                            model.Change(*row, 0, false);
                        Play(95);
                    }
                    break;
                case Back:
                {
                    auto parent = model.Back(state.page);
                    if (parent == SIZE_MAX)
                        return false;
                    state.page = parent;
                    Play(95);
                    break;
                }
                default: break;
            }
        }
        Settle(before, firstBefore, pageBefore);
        return true;
    }

    // Every frame while the page is open: the mouse over a row, the keys the game's menus don't use, the wheel
    inline void Poll(uintptr_t menu)
    {
        auto& list = model.Window(state.page);
        list.SetSlots(Slots);
        auto shown = model.Shown(state.page, false);
        const size_t before = list.Selected(), firstBefore = list.First();
        const size_t pageBefore = state.page;
        // The game moves its slot when the pointer enters a row
        const int hovered = At<int>(menu, Field::Slot);
        if (state.slot >= 0 && hovered != state.slot)
        {
            Repaint();
            if (hovered >= 0 && hovered < int(shown.size()))
                list.Point(size_t(hovered));
        }
        for (auto extra : PollExtra())
        {
            auto& window = model.Window(state.page);
            switch (extra)
            {
                case StickUp: window.Move(-1); break;
                case StickDown: window.Move(1); break;
                case StickLeft: case StickRight:
                {
                    auto rows = model.Shown(state.page, false);
                    auto row = window.Slot() < rows.size() ? rows[window.Slot()] : nullptr;
                    if (row && !RWGTAMenu::Model::Opens(*row) && model.Enabled(*row, false) && model.Change(*row, extra == StickLeft ? -1 : 1, false))
                        Play(90);
                    break;
                }
                case PageUp: window.Page(-1); break;
                case PageDown: window.Page(1); break;
                case Home: window.Edge(-1); break;
                case End: window.Edge(1); break;
                case Help: state.expanded = !state.expanded; break;
                default: break;
            }
        }
        for (int n = state.notches.Take(state.wheel.exchange(0)); n; n += n > 0 ? -1 : 1)
            model.Window(state.page).Scroll(n > 0 ? -1 : 1);
        Settle(before, firstBefore, pageBefore);
    }

    // Native hooks
    inline SafetyHookInline optionsHook, acceptHook, namesHook, inputHook, titleHook;

    // The Options list: Settings, Sound, Controls, and Plugins after them
    inline int __cdecl BuildOptions(int context)
    {
        // The scripts point the current object elsewhere for the calls they make
        const auto menu = Menu();
        const int result = optionsHook.ccall<int>(context);
        const auto options = At(menu, Field::Options);
        const auto type = *api.optionList;
        if (At<int>(menu, Field::OptionsPage) && At<int>(menu, Field::OptionCount) == 3 && Count(options, type) > 3
            && At<int>(Element(options, type, 2), 12) == 9)
        {
            const auto previous = At(Element(options, type, 1), 4);
            const auto last = At(Element(options, type, 2), 4);
            const auto entry = Element(options, type, 3);
            const auto text = At(entry, 4);
            float a[4], b[4];
            Position(previous, a);
            Position(last, b);
            api.setIndex(text, nullptr, 0);
            SetWide(text, Upper(Game(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"))));
            Place(text, b[0], b[1] + (b[1] - a[1]), b[2]);
            Visible(text, true);
            At<int>(entry, 12) = 7;     // opens the Settings page, which then shows the plugins
            At<int>(menu, Field::OptionCount) = 4;
            // Back from the plugins lands on their row
            if (std::exchange(state.returning, false))
                At<int>(menu, Field::OptionSelected) = 3;
        }
        return result;
    }

    inline int __cdecl AcceptOption(int context)
    {
        const auto menu = Menu();
        if (At<int>(menu, Field::OptionsPage) && !At<int>(menu, Field::SettingsPage))
            state.pending = At<int>(menu, Field::Screen) == 1 && At<int>(menu, Field::OptionCount) == 4 && At<int>(menu, Field::OptionSelected) == 3;
        return acceptHook.ccall<int>(context);
    }

    inline int __cdecl BuildSettings(int context, int* page)
    {
        const auto menu = Menu();
        const int result = namesHook.ccall<int>(context, page);
        // The Settings page with its ten rows
        if (std::exchange(state.pending, false) && Count(At(menu, Field::Names), *api.entityList) >= int(Slots))
        {
            state.menu = menu;
            Open();
            Repaint();
            Draw(menu);
        }
        return result;
    }

    // The game's menu events
    inline int __cdecl MenuInput(int context)
    {
        const auto object = *api.current;
        const auto menu = At(object, 24);
        if (state.open && menu == state.menu)
        {
            if (!At<int>(menu, Field::SettingsPage))
                Close();
            else if (Handle(menu))
            {
                Draw(menu);
                return 0;
            }
            else
            {
                // Back on the plugins list: the game's own Back leaves the page
                Close();
                state.returning = true;
            }
        }
        return inputHook.ccall<int>(context);
    }

    inline int __cdecl Title(int context)
    {
        state.header = At(*api.current, 24);
        return titleHook.ccall<int>(context);
    }

    // From the game window's procedure
    inline void WindowMessage(HWND window, UINT message, WPARAM wParam)
    {
        state.window = window;
        if (message == WM_MOUSEWHEEL && state.open)
        {
            // The controller layout turns the shoulders into wheel notches; the page reads them as page keys
            XINPUT_STATE pad{};
            DWORD slot = 0;
            if (PadInput::Read(pad, slot) && (pad.Gamepad.wButtons & (XINPUT_GAMEPAD_LEFT_SHOULDER | XINPUT_GAMEPAD_RIGHT_SHOULDER)))
                return;
            state.wheel += GET_WHEEL_DELTA_WPARAM(wParam);
        }
    }

    // Once per frame
    inline void Frame()
    {
        if (state.open && state.menu)
        {
            if (!At<int>(state.menu, Field::SettingsPage))
                Close();
            else
            {
                Poll(state.menu);
                Draw(state.menu);
            }
        }
        // Rain Droplets keeps its drops off the menus: a menu screen shows its title
        const bool menu = state.header && !(At<uint32_t>(At(state.header, 0xB0), 40) & 1);
        RainDroplets::MenuOpen(menu);
    }

    inline void Install()
    {
        model.RememberRunningSettings(FixSettings());
        // The game's menus repeat a held direction about four times a second
        for (auto repeat : { &state.stickUp, &state.stickDown, &state.stickLeft, &state.stickRight })
        {
            repeat->delay = 250;
            repeat->interval = 250;
        }

        auto setIndex = hook::pattern("83 EC 0C 56 8B F1 C7 44 24 08 00 00 00 00");
        auto version = hook::pattern("E8 ? ? ? ? 8B 6C 24 68 8B 8E 94 00 00 00 55 E8 ? ? ? ? 55 E8 ? ? ? ? 83 C4 04 8D 4C 24 20");
        auto alloc = hook::pattern("A0 ? ? ? ? 84 C0 74 ? 33 C0 C3 8B 44 24 04 8B 0C 85 ? ? ? ? 8B 44 24 08 8B 11 6A 00 68 ? ? ? ? 50 FF 52 08 C3");
        auto dirty = hook::pattern("83 EC 50 56 8B F1 E8 ? ? ? ? 8B 06");
        auto setPos = hook::pattern("83 EC 30 53 8B D9 8B 43 3C 85 C0 0F 84");
        auto getPos = hook::pattern("8B 41 3C 85 C0 74 ? 83 C0 10 56 8B C8");
        auto width = hook::pattern("B8 10 10 00 00 E8 ? ? ? ? 56 8B F1 8B 86 A4 00 00 00");
        auto refresh = hook::pattern("81 EC A4 01 00 00 53 55 56 57 8B 3D ? ? ? ?");
        auto settingsDown = hook::pattern("83 EC 08 53 8B 5C 24 10 55 56 57 8B 3D ? ? ? ? 8B 77 18 8B 86 D8 00 00 00 85 C0 8B AE CC 00 00 00 0F 84");
        auto game = hook::pattern("8B 15 ? ? ? ? 8B 42 18 8D 4C 24 14 51 8B 88 54 02 00 00 8B 51 18 8B 4A 3C 6A ED");
        auto entities = hook::pattern("8B 0D ? ? ? ? 99 F7 79 24 85 C0 0F 8E ? ? ? ? 68 F5 00 00 00");
        auto options = hook::pattern("83 EC 3C 53 8B 5C 24 44 56 57 8B 3D ? ? ? ?");
        auto input = hook::pattern("83 EC 0C 53 55 56 57 8B 3D ? ? ? ? 8B 77 18 8B 86 A0 01 00 00");
        auto names = hook::pattern("81 EC 94 00 00 00 53 55 56 57 8B 3D ? ? ? ?");
        auto title = hook::pattern("83 EC 38 A1 ? ? ? ? 56 8B 70 18 8B 8E AC 00 00 00");
        auto accept = hook::pattern("83 EC 24 53 55 56 57 8B 3D ? ? ? ? 8B 77 18 6A 4E 8B CF E8 ? ? ? ? A1 ? ? ? ? 8B 48 24 0F AF 8E 7C 01 00 00");

        api.setIndex = reinterpret_cast<decltype(api.setIndex)>(setIndex.get_first());
        api.setText = reinterpret_cast<decltype(api.setText)>(injector::GetBranchDestination(version.get_first(16)).as_int());
        api.alloc = reinterpret_cast<decltype(api.alloc)>(alloc.get_first());
        api.free = reinterpret_cast<decltype(api.free)>(injector::GetBranchDestination(setIndex.get_first(0x19)).as_int());
        api.dirty = reinterpret_cast<decltype(api.dirty)>(dirty.get_first());
        api.setPos = reinterpret_cast<decltype(api.setPos)>(setPos.get_first());
        api.getPos = reinterpret_cast<decltype(api.getPos)>(getPos.get_first());
        api.width = reinterpret_cast<decltype(api.width)>(width.get_first());
        api.sound = reinterpret_cast<decltype(api.sound)>(injector::GetBranchDestination(settingsDown.get_first(0x14F)).as_int());
        api.colour = reinterpret_cast<decltype(api.colour)>(injector::GetBranchDestination(refresh.get_first(0x201)).as_int());
        api.current = *input.get_first<uintptr_t*>(9);
        api.game = *game.get_first<uintptr_t*>(2);
        api.entityList = *entities.get_first<uintptr_t*>(2);
        api.optionList = *accept.get_first<uintptr_t*>(0x1A);

        optionsHook = safetyhook::create_inline(options.get_first(), BuildOptions);
        acceptHook = safetyhook::create_inline(accept.get_first(), AcceptOption);
        namesHook = safetyhook::create_inline(names.get_first(), BuildSettings);
        inputHook = safetyhook::create_inline(input.get_first(), MenuInput);
        titleHook = safetyhook::create_inline(title.get_first(), Title);
        WFP::onGameProcessEvent() += Frame;
    }
}
