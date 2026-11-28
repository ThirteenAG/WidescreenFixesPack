module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include <Xinput.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/PadInput.hpp"

export module NativeMenu;
import Imports;

// The Plugins page in the game's own Settings menu. Settings gets one more row, Plugins; it turns the Settings list
// into the plugins list, a plugin's page or a category, drawn by the game's own menu window: its font, colours,
// selection arrow and transitions. Values sit in a second column like the game's radio buttons (every choice listed,
// the current one bright) and the key line and the help under the list use the game's small font.
export namespace JCMenu
{
    struct GameString
    {
        uint32_t proxy;
        union
        {
            char buffer[16];
            char* pointer;
        };
        uint32_t size;
        uint32_t capacity;

        const char* c_str() const { return capacity < 16 ? buffer : pointer; }
    };
    static_assert(sizeof(GameString) == 28);

    // The game's own functions
    struct Api
    {
        void(__fastcall* tidy)(GameString*, void*, bool) = nullptr;
        GameString* (__fastcall* assign)(GameString*, void*, const char*, size_t) = nullptr;
        void(__fastcall* clear)(uintptr_t, void*) = nullptr;
        void(__fastcall* split)(uintptr_t, void*, GameString*, uintptr_t, GameString*) = nullptr;
        void* (__cdecl* alloc)(size_t) = nullptr;
        uintptr_t(__fastcall* construct)(void*, void*) = nullptr;
        float* (__fastcall* measure)(uintptr_t, void*, float*) = nullptr;
        void(__fastcall* flag)(uintptr_t, void*, int) = nullptr;
        uintptr_t textTable = 0;
    } inline api;

    // A game string for a call, freed by the game's own string code
    struct Scoped
    {
        GameString value{};
        explicit Scoped(const std::string& text)
        {
            api.tidy(&value, nullptr, false);
            api.assign(&value, nullptr, text.c_str(), text.size());
        }
        ~Scoped() { api.tidy(&value, nullptr, true); }
    };

    // Nodes of the game's GUI tree: position +20/+24, flags +44 (2 shown, 4 changed, 8 enabled), parent +60, children +68..+72
    inline float& X(uintptr_t node) { return *reinterpret_cast<float*>(node + 20); }
    inline float& Y(uintptr_t node) { return *reinterpret_cast<float*>(node + 24); }
    inline uint8_t& Flags(uintptr_t node) { return *reinterpret_cast<uint8_t*>(node + 44); }
    inline uintptr_t Parent(uintptr_t node) { return *reinterpret_cast<uintptr_t*>(node + 60); }
    inline std::vector<uintptr_t> Children(uintptr_t node)
    {
        auto first = *reinterpret_cast<uintptr_t**>(node + 68);
        auto last = *reinterpret_cast<uintptr_t**>(node + 72);
        return first ? std::vector<uintptr_t>(first, last) : std::vector<uintptr_t>{};
    }
    inline void Move(uintptr_t node, float x, float y)
    {
        if (X(node) != x || Y(node) != y)
        {
            X(node) = x;
            Y(node) = y;
            Flags(node) |= 4;
        }
    }
    inline void Show(uintptr_t node, bool shown)
    {
        if (((Flags(node) & 2) != 0) != shown)
            Flags(node) = (Flags(node) & ~2) | (shown ? 2 : 0) | 4;
    }
    inline void Enable(uintptr_t node, bool enabled)
    {
        if (((Flags(node) & 8) != 0) != enabled)
            Flags(node) = (Flags(node) & ~8) | (enabled ? 8 : 0) | 4;
    }
    inline void AddChild(uintptr_t parent, uintptr_t child)
    {
        auto table = *reinterpret_cast<void(__fastcall***)(uintptr_t, void*, uintptr_t)>(parent);
        table[3](parent, nullptr, child);
    }

    // Text objects: font +92, set font/colour/text through their table
    using SetText_t = int(__fastcall*)(uintptr_t, void*, GameString*, int, int);
    inline SetText_t NativeSetText = nullptr;
    inline bool settingOwnText = false;
    inline std::unordered_map<uintptr_t, std::string> gameTexts;
    inline bool IsText(uintptr_t node) { return *reinterpret_cast<uintptr_t*>(node) == api.textTable; }

    inline void SetText(uintptr_t text, const std::string& value)
    {
        Scoped game(value);
        settingOwnText = true;
        NativeSetText(text, nullptr, &game.value, 0, 1);
        settingOwnText = false;
    }
    inline void SetFont(uintptr_t text, uintptr_t font)
    {
        (*reinterpret_cast<void(__fastcall***)(uintptr_t, void*, uintptr_t)>(text))[22](text, nullptr, font);
    }
    inline void SetColour(uintptr_t text, const uint8_t* rgba)
    {
        (*reinterpret_cast<void(__fastcall***)(uintptr_t, void*, const uint8_t*)>(text))[23](text, nullptr, rgba);
    }
    inline float Width(uintptr_t text)
    {
        float size[2]{};
        api.measure(text, nullptr, size);
        return size[0];
    }

    // Menu windows: items "$"-joined +88 split into +116, selection +68, row count +72, row spacing +76, row height +80,
    // item x +84, highlight target +224, colours normal +252 selected +256, font +260, item texts +504, fade +544, state +674
    inline int& Selection(uintptr_t window) { return *reinterpret_cast<int*>(window + 68); }
    inline uintptr_t Node(uintptr_t window) { return *reinterpret_cast<uintptr_t*>(window + 32); }
    inline std::vector<uintptr_t> ItemTexts(uintptr_t window)
    {
        auto first = *reinterpret_cast<uintptr_t**>(window + 504);
        auto last = *reinterpret_cast<uintptr_t**>(window + 508);
        return first ? std::vector<uintptr_t>(first, last) : std::vector<uintptr_t>{};
    }
    inline std::string Items(uintptr_t window) { return reinterpret_cast<GameString*>(window + 88)->c_str(); }
    inline void SetItems(uintptr_t window, const std::string& items)
    {
        if (Items(window) == items)
            return;
        api.assign(reinterpret_cast<GameString*>(window + 88), nullptr, items.c_str(), items.size());
        api.clear(window + 116, nullptr);
        Scoped separator("$");
        api.split(window, nullptr, reinterpret_cast<GameString*>(window + 88), window + 116, &separator.value);
    }
    inline void Highlight(uintptr_t window, int slot, bool animate)
    {
        Selection(window) = slot;
        const float y = float(slot) * *reinterpret_cast<float*>(window + 76) * *reinterpret_cast<float*>(window + 80);
        *reinterpret_cast<float*>(window + 224) = y;
        if (animate)
            *reinterpret_cast<uint8_t*>(window + 674) |= 2;
        else
            *reinterpret_cast<float*>(window + 228) = y;
    }

    // A new text object as the window builds its rows, under the given parent
    inline uintptr_t NewText(uintptr_t window, uintptr_t parent, uintptr_t font)
    {
        auto memory = api.alloc(336);
        auto text = api.construct(memory, nullptr);
        *reinterpret_cast<float*>(text + 28) = *reinterpret_cast<float*>(text + 36) = 1.0f;
        *reinterpret_cast<float*>(text + 32) = *reinterpret_cast<float*>(text + 40) = 1.0f;
        SetFont(text, font);
        Enable(text, true);
        SetColour(text, reinterpret_cast<uint8_t*>(window + 252));
        api.flag(text, nullptr, 0);
        AddChild(parent, text);
        Show(text, false);
        return text;
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

    // The font's code page; game text keys stay as they are
    inline std::string Game(const std::string& utf8)
    {
        auto wide = asi_settings::text::Widen(utf8);
        std::string out;
        for (auto c : wide)
        {
            if (c < 0x100)
                out += char(c);
            else
            {
                char mapped = '?';
                BOOL lost = FALSE;
                WideCharToMultiByte(1252, 0, &c, 1, &mapped, 1, "?", &lost);
                out += mapped;
            }
        }
        // '$' separates rows and braces mark the game's key tokens
        for (auto& c : out)
        {
            if (c == '$')
                c = 'S';
            else if (c == '{' || c == '}')
                c = c == '{' ? '(' : ')';
        }
        return out;
    }

    inline std::string Upper(std::string text)
    {
        if (!text.empty())
            CharUpperBuffA(text.data(), DWORD(text.size()));
        return text;
    }

    struct Choice
    {
        std::string text;
        bool current = false;
    };

    // Input read while the page is open; the game's own input is held back meanwhile
    struct Input
    {
        MenuList::Repeat up, down, left, right, pageUp, pageDown;
        MenuList::StickAxis stickX, stickY;
        bool held[16]{};
        bool armed = false;
        std::atomic<int> wheel{ 0 };
        MenuList::WheelNotches notches;

        bool Edge(int index, bool down)
        {
            const bool pressed = down && !held[index];
            held[index] = down;
            return pressed && armed;
        }
    } inline input;

    inline std::atomic<bool> blocking{ false };
    inline HWND gameWindow = nullptr;

    struct State
    {
        uintptr_t window = 0;           // the game's Settings window the page lives in
        uintptr_t title = 0, hint = 0, restart = 0, above = 0, below = 0;
        std::vector<uintptr_t> values, help;
        std::string nativeItems;
        int pluginsRow = -1;
        bool open = false, pause = false, expanded = false;
        uint8_t textChange = 0;
        bool restoreTextChange = false;
        size_t page = RWGTAMenu::Model::RootPage;
        float titleY = 0, hintY = 0, rowsTop = 0, nativeGap = 0;
        int nativeRows = 0;
    } inline state;

    inline RWGTAMenu::Model model;
    constexpr size_t Slots = 9;

    inline bool IsSettings(uintptr_t window)
    {
        return std::strstr(reinterpret_cast<GameString*>(window + 132)->c_str(), "_s_control") != nullptr;
    }

    inline std::string Label(const RWGTAMenu::Row& row)
    {
        if (row.kind == RWGTAMenu::RowKind::RestoreDefaults)
            return "fe_menuitem_32";
        auto label = Game(model.Label(row));
        if (row.kind == RWGTAMenu::RowKind::Setting && row.setting && row.setting->apply == "restart")
            label += " *";
        return label;
    }

    // The values of a row as the game lists its radio buttons: all choices of a short list, else the current one
    inline std::vector<Choice> Choices(const RWGTAMenu::Row& row)
    {
        std::vector<Choice> list;
        auto value = model.Value(row);
        if (row.kind == RWGTAMenu::RowKind::Setting && row.setting && row.plugin)
        {
            auto& setting = *row.setting;
            if (setting.type == asi_settings::Type::Bool)
            {
                const bool on = value == model.Text("menu.on");
                return { { "fe_menuitem_79", !on }, { "fe_menuitem_82", on } };
            }
            if (!setting.choices.empty() && setting.choices.size() <= 4)
            {
                for (auto& entry : setting.choices)
                {
                    auto label = OtherPlugins::Text(*row.plugin, entry.label);
                    list.push_back({ Game(label), label == value });
                }
                if (std::any_of(list.begin(), list.end(), [](const Choice& c) { return c.current; }))
                    return list;
                list.clear();
            }
        }
        if (!value.empty())
            list.push_back({ Game(value), true });
        return list;
    }

    inline std::string PageTitle()
    {
        if (state.page == RWGTAMenu::Model::RootPage || state.page >= model.Pages().size())
            return Upper(Game(model.Text("menu.plugins")));
        return Upper(Game(model.Pages()[state.page].title));
    }

    // Help under the key line: whole lines that fit; a long text stops at its last whole sentence (F1 shows it all)
    inline std::vector<std::string> Wrap(uintptr_t measure, const std::string& text, float width, size_t lines)
    {
        std::vector<std::string> out;
        std::string line, word;
        auto fits = [&](const std::string& candidate)
        {
            SetText(measure, candidate);
            return Width(measure) <= width;
        };
        std::vector<std::string> words;
        for (size_t i = 0; i <= text.size(); ++i)
        {
            if (i == text.size() || text[i] == ' ')
            {
                if (!word.empty())
                    words.push_back(word);
                word.clear();
            }
            else
                word += text[i];
        }
        for (auto& w : words)
        {
            auto candidate = line.empty() ? w : line + " " + w;
            if (line.empty() || fits(candidate))
                line = candidate;
            else
            {
                out.push_back(line);
                line = w;
            }
        }
        if (!line.empty())
            out.push_back(line);
        if (out.size() <= lines)
            return out;
        // Too long: stop at the last sentence that ends within the lines shown
        std::string joined;
        for (size_t i = 0; i < lines; ++i)
            joined += (i ? " " : "") + out[i];
        auto end = joined.find_last_of(".!?");
        out.resize(lines);
        if (end != std::string::npos)
        {
            auto cut = Wrap(measure, joined.substr(0, end + 1), width, lines);
            return cut;
        }
        return out;
    }

    inline void Restore();

    inline void Close()
    {
        if (!state.open)
            return;
        state.open = false;
        Restore();
        model.Close();
        blocking = true;
    }

    inline void Restore()
    {
        auto window = state.window;
        if (!window)
            return;
        SetItems(window, state.nativeItems);
        state.restoreTextChange = true;
        Highlight(window, state.pluginsRow, false);
        for (auto text : { state.title, state.hint })
        {
            if (text && gameTexts.count(text))
                SetText(text, gameTexts[text]);
        }
        if (state.title)
            Y(state.title) = state.titleY;
        if (state.hint)
            Move(state.hint, X(state.hint), state.hintY);
        for (auto text : state.values)
            Show(text, false);
        for (auto text : state.help)
            Show(text, false);
        for (auto text : { state.restart, state.above, state.below })
            if (text)
                Show(text, false);
    }

    inline void Open(uintptr_t window)
    {
        state.window = window;
        state.nativeItems = Items(window);
        state.page = RWGTAMenu::Model::RootPage;
        state.expanded = false;
        // The screen around the window: its title above, the key line below
        auto node = Node(window);
        auto screen = Parent(node);
        state.title = state.hint = 0;
        for (auto child : Children(screen))
        {
            if (!IsText(child))
                continue;
            if (Y(child) < Y(node) && (!state.title || Y(child) > Y(state.title)))
                state.title = child;
            if (Y(child) > Y(node) && (!state.hint || Y(child) < Y(state.hint)))
                state.hint = child;
        }
        if (!state.title || !state.hint)
            return;
        state.titleY = Y(state.title);
        state.hintY = Y(state.hint);
        state.nativeRows = int(ItemTexts(window).size());
        state.rowsTop = Y(node);
        const float rowHeight = *reinterpret_cast<float*>(window + 76) * *reinterpret_cast<float*>(window + 80);
        state.nativeGap = state.hintY - Y(node) - rowHeight * float(state.nativeRows);
        if (state.help.empty())
        {
            const auto hintFont = state.hint + 92;
            for (int i = 0; i < 8; ++i)
                state.help.push_back(NewText(window, screen, hintFont));
            state.restart = NewText(window, screen, hintFont);
            state.above = NewText(window, screen, hintFont);
            state.below = NewText(window, screen, hintFont);
            for (auto text : state.help)
                SetColour(text, reinterpret_cast<uint8_t*>(window + 256));
        }
        // Rows change at once while the page is open, without the game's letter-by-letter text change
        state.textChange = (*reinterpret_cast<uint8_t*>(window + 674) >> 6) & 1;
        *reinterpret_cast<uint8_t*>(window + 674) &= ~0x40;
        for (auto text : ItemTexts(window))
            api.flag(text, nullptr, 0);
        model.OpenMenu(ThisModule(), Language(), Slots);
        state.open = true;
        input.armed = false;
    }

    inline bool Focused()
    {
        return gameWindow && GetForegroundWindow() == gameWindow;
    }

    inline bool Key(int vk)
    {
        return Focused() && (GetAsyncKeyState(vk) & 0x8000) != 0;
    }

    enum Action { None, Up, Down, Left, Right, Accept, Back, PageUp, PageDown, Home, End, Help, WheelUp, WheelDown };

    inline std::vector<Action> Poll()
    {
        std::vector<Action> actions;
        XINPUT_STATE pad{};
        DWORD slot = 0;
        const bool padOn = Focused() && PadInput::Read(pad, slot);
        const auto& g = pad.Gamepad;
        const int sx = padOn ? input.stickX.Direction(g.sThumbLX) : 0;
        const int sy = padOn ? input.stickY.Direction(g.sThumbLY) : 0;
        const auto now = GetTickCount64();
        const bool upDown = Key(VK_UP) || (padOn && (g.wButtons & XINPUT_GAMEPAD_DPAD_UP)) || sy > 0;
        const bool downDown = Key(VK_DOWN) || (padOn && (g.wButtons & XINPUT_GAMEPAD_DPAD_DOWN)) || sy < 0;
        const bool leftDown = Key(VK_LEFT) || (padOn && (g.wButtons & XINPUT_GAMEPAD_DPAD_LEFT)) || sx < 0;
        const bool rightDown = Key(VK_RIGHT) || (padOn && (g.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)) || sx > 0;
        const bool pageUpDown = Key(VK_PRIOR) || (padOn && (g.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER));
        const bool pageDownDown = Key(VK_NEXT) || (padOn && (g.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER));
        const bool accept = Key(VK_RETURN) || (padOn && (g.wButtons & XINPUT_GAMEPAD_A));
        const bool back = Key(VK_ESCAPE) || Key(VK_BACK) || (padOn && (g.wButtons & XINPUT_GAMEPAD_B));
        const bool home = Key(VK_HOME), end = Key(VK_END), help = Key(VK_F1);
        // Keys held when the page opened act only after they are let go
        if (!input.armed)
        {
            if (!upDown && !downDown && !leftDown && !rightDown && !accept && !back && !pageUpDown && !pageDownDown)
                input.armed = true;
            for (auto& h : input.held)
                h = true;
            input.wheel = 0;
            input.notches.Clear();
            return actions;
        }
        if (input.up.Step(upDown, now)) actions.push_back(Up);
        if (input.down.Step(downDown, now)) actions.push_back(Down);
        if (input.left.Step(leftDown, now)) actions.push_back(Left);
        if (input.right.Step(rightDown, now)) actions.push_back(Right);
        if (input.pageUp.Step(pageUpDown, now)) actions.push_back(PageUp);
        if (input.pageDown.Step(pageDownDown, now)) actions.push_back(PageDown);
        if (input.Edge(0, accept)) actions.push_back(Accept);
        if (input.Edge(1, back)) actions.push_back(Back);
        if (input.Edge(2, home)) actions.push_back(Home);
        if (input.Edge(3, end)) actions.push_back(End);
        if (input.Edge(4, help)) actions.push_back(Help);
        for (int n = input.notches.Take(input.wheel.exchange(0)); n; n += n > 0 ? -1 : 1)
            actions.push_back(n > 0 ? WheelUp : WheelDown);
        return actions;
    }

    // Before the window updates: input, then the rows it shows
    inline void Tick(uintptr_t window)
    {
        auto& pages = model.Pages();
        if (state.page >= pages.size())
            state.page = RWGTAMenu::Model::RootPage;
        auto& list = model.Window(state.page);
        list.SetSlots(Slots);
        model.ListRows(state.page, false);
        const size_t before = list.Selected(), firstBefore = list.First();
        for (auto action : Poll())
        {
            auto shown = model.Shown(state.page, false);
            const auto row = list.Slot() < shown.size() ? shown[list.Slot()] : nullptr;
            switch (action)
            {
                case Up: list.Move(-1); break;
                case Down: list.Move(1); break;
                case PageUp: list.Page(-1); break;
                case PageDown: list.Page(1); break;
                case Home: list.Edge(-1); break;
                case End: list.Edge(1); break;
                case WheelUp: list.Scroll(-1); break;
                case WheelDown: list.Scroll(1); break;
                case Help: state.expanded = !state.expanded; break;
                case Left: case Right:
                    if (row && !RWGTAMenu::Model::Opens(*row))
                        model.Change(*row, action == Left ? -1 : 1, state.pause);
                    break;
                case Accept:
                    if (!row)
                        break;
                    if (RWGTAMenu::Model::Opens(*row))
                    {
                        if (auto target = model.Enter(*row); target != SIZE_MAX)
                        {
                            state.page = target;
                            state.expanded = false;
                            Highlight(window, 0, false);
                        }
                    }
                    else
                        model.Change(*row, 0, state.pause);
                    break;
                case Back:
                {
                    auto parent = model.Back(state.page);
                    if (parent == SIZE_MAX)
                    {
                        Close();
                        return;
                    }
                    state.page = parent;
                    state.expanded = false;
                    Highlight(window, int(model.Window(parent).Slot()), false);
                    break;
                }
                default: break;
            }
            if (!state.open)
                return;
        }
        auto& now = model.Window(state.page);
        auto shown = model.Shown(state.page, false);
        std::string items;
        for (size_t i = 0; i < shown.size(); ++i)
            items += (i ? "$" : "") + (shown[i] ? Label(*shown[i]) : std::string{});
        SetItems(window, items);
        const int slot = int(now.Slot());
        if (Selection(window) != slot)
            Highlight(window, slot, now.First() == firstBefore && now.Selected() != before);
    }

    // After the window built its rows: the value column, the title, the key line and the help
    inline void Layout(uintptr_t window)
    {
        auto shown = model.Shown(state.page, false);
        auto texts = ItemTexts(window);
        auto node = Node(window);
        const float itemX = *reinterpret_cast<float*>(window + 84);
        const float rowHeight = *reinterpret_cast<float*>(window + 76) * *reinterpret_cast<float*>(window + 80);
        const uint8_t fade = uint8_t(*reinterpret_cast<float*>(window + 544) * 255.0f);
        float column = 0.0f;
        bool anyValue = false, anyRestart = false;
        for (size_t i = 0; i < shown.size() && i < texts.size(); ++i)
        {
            if (!shown[i])
                continue;
            column = std::max(column, Width(texts[i]));
            Enable(texts[i], model.Enabled(*shown[i], state.pause));
            anyRestart |= shown[i]->kind == RWGTAMenu::RowKind::Setting && shown[i]->setting && shown[i]->setting->apply == "restart";
        }
        // The value column starts where the game's own value columns do, or after the longest label
        // Like the game's General and Video pages: the column after the longest label, not left of theirs
        column = itemX + std::max(column + 0.05f, 0.245f);
        size_t used = 0;
        for (size_t i = 0; i < shown.size() && i < texts.size(); ++i)
        {
            if (!shown[i] || RWGTAMenu::Model::Opens(*shown[i]))
                continue;
            float x = column;
            for (auto& choice : Choices(*shown[i]))
            {
                if (used == state.values.size())
                    state.values.push_back(NewText(window, node, *reinterpret_cast<uintptr_t*>(window + 260)));
                auto text = state.values[used++];
                SetText(text, choice.text);
                uint8_t colour[4];
                std::memcpy(colour, reinterpret_cast<uint8_t*>(window + (choice.current ? 256 : 252)), 4);
                if (!model.Enabled(*shown[i], state.pause))
                    for (int c = 0; c < 3; ++c)
                        colour[c] /= 2;
                colour[3] = uint8_t(colour[3] * fade / 255);
                SetColour(text, colour);
                Move(text, x, Y(texts[i]));
                Show(text, true);
                x += Width(text) + 0.022f;
                anyValue = true;
            }
        }
        for (size_t i = used; i < state.values.size(); ++i)
            Show(state.values[i], false);

        // More rows above or below: the small font's marks in the gaps over the first and under the last row
        auto& scroll = model.Window(state.page);
        const float markX = X(node) + itemX;
        const float top = Y(node) + (texts.empty() ? 0.0f : Y(texts.front()));
        const float bottom = Y(node) + rowHeight * float(std::max<size_t>(shown.size(), 1));
        SetText(state.above, "^");
        SetText(state.below, "V");
        for (auto [text, visible, y] : { std::tuple{ state.above, scroll.Above(), top - rowHeight * 0.5f },
            std::tuple{ state.below, scroll.Below(), bottom + rowHeight * 0.05f } })
        {
            SetColour(text, reinterpret_cast<uint8_t*>(window + 252));
            Move(text, markX, y);
            Show(text, visible);
        }

        // Title, key line and help move with the rows like the game's own pages
        SetText(state.title, PageTitle());
        const float listBottom = Y(node) + rowHeight * float(std::max<size_t>(shown.size(), 1));
        // A list that scrolls keeps room for its lower mark
        const bool scrolls = scroll.Count() > scroll.Slots();
        const float hintY = listBottom + state.nativeGap + (scrolls ? rowHeight * 0.5f : 0.0f);
        SetText(state.hint, anyValue ? "{GUI_LEFT} {GUI_RIGHT} fe_menuitem_96  {GUI_CANCEL} fe_menuitem_14" : "{GUI_OK} fe_menuitem_96  {GUI_CANCEL} fe_menuitem_14");
        Move(state.hint, X(state.hint), hintY);
        const float lineHeight = rowHeight * 0.8f;
        float y = hintY + lineHeight * 1.5f;
        if (anyRestart || model.RestartRequired())
        {
            SetText(state.restart, "* fe_menuitem_93");
            SetColour(state.restart, reinterpret_cast<uint8_t*>(window + 256));
            Move(state.restart, X(state.hint), y);
            Show(state.restart, true);
            y += lineHeight * 1.5f;
        }
        else
            Show(state.restart, false);
        std::string help;
        auto& list = model.Window(state.page);
        if (list.Slot() < shown.size() && shown[list.Slot()])
            help = Upper(Game(model.Help(*shown[list.Slot()])));
        // Up to where the game's logo starts, like its own restart line
        const float width = 0.62f;
        auto lines = help.empty() ? std::vector<std::string>{} : Wrap(state.help.back(), help, width, state.expanded ? state.help.size() - 1 : 2);
        for (size_t i = 0; i < state.help.size(); ++i)
        {
            auto text = state.help[i];
            if (i < lines.size())
            {
                SetText(text, lines[i]);
                Move(text, X(state.hint), y + lineHeight * float(i));
                Show(text, true);
            }
            else
                Show(text, false);
        }
    }

    // Native hooks
    using Load_t = char(__fastcall*)(uintptr_t, void*, void*);
    using Update_t = void(__fastcall*)(uintptr_t, void*, int);
    using Destroy_t = void* (__fastcall*)(uintptr_t, void*, int);
    inline Load_t NativeLoad = nullptr;
    inline Update_t NativeUpdate = nullptr;
    inline Destroy_t NativeDestroy = nullptr;
    using Message_t = int(__fastcall*)(uintptr_t, void*, int, int);
    inline Message_t NativeMessage = nullptr;
    inline uintptr_t windowTable = 0;

    inline char __fastcall Load(uintptr_t window, void* edx, void* data)
    {
        auto result = NativeLoad(window, edx, data);
        if (IsSettings(window))
        {
            auto items = Items(window);
            SetItems(window, items + "$" + Game(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins")));
        }
        return result;
    }

    // The Settings screen's key line moves down by the added row
    inline std::unordered_map<uintptr_t, std::pair<uintptr_t, float>> keyLines;
    inline void PlaceKeyLine(uintptr_t window)
    {
        auto it = keyLines.find(window);
        if (it == keyLines.end())
        {
            auto node = Node(window);
            uintptr_t hint = 0;
            for (auto child : Children(Parent(node)))
            {
                if (IsText(child) && Y(child) > Y(node) && (!hint || Y(child) < Y(hint)))
                    hint = child;
            }
            if (!hint)
                return;
            it = keyLines.emplace(window, std::pair{ hint, Y(hint) }).first;
        }
        const float rowHeight = *reinterpret_cast<float*>(window + 76) * *reinterpret_cast<float*>(window + 80);
        Move(it->second.first, X(it->second.first), it->second.second + rowHeight);
    }

    // A menu window on screen: shown all the way up its tree and taking input
    inline std::atomic<DWORD> menuTick{ 0 };
    inline bool OnScreen(uintptr_t window)
    {
        if (!(*reinterpret_cast<uint8_t*>(window + 674) & 1))
            return false;
        int depth = 0;
        for (auto node = Node(window); node && depth < 32; node = Parent(node), ++depth)
            if (!(Flags(node) & 2))
                return false;
        return true;
    }
    inline bool KeyLineShown();
    inline bool MenuShown() { return state.open || GetTickCount() - menuTick.load() < 250 || KeyLineShown(); }

    // Every menu screen shows a key line of the game's menu keys ("{GUI_OK} ..."): one on screen means a menu is up
    inline std::unordered_set<uintptr_t> keyTexts;
    inline bool Visible(uintptr_t node)
    {
        int depth = 0;
        for (; node && depth < 32; node = Parent(node), ++depth)
            if (!(Flags(node) & 2))
                return false;
        return true;
    }
    inline bool KeyLineShown()
    {
        for (auto text : keyTexts)
            if (Visible(text))
                return true;
        return false;
    }

    using TextDestroy_t = void* (__fastcall*)(uintptr_t, void*, int);
    inline TextDestroy_t NativeTextDestroy = nullptr;
    inline void* __fastcall TextDestroy(uintptr_t text, void* edx, int flags)
    {
        keyTexts.erase(text);
        gameTexts.erase(text);
        return NativeTextDestroy(text, edx, flags);
    }

    inline void __fastcall Update(uintptr_t window, void* edx, int a2)
    {
        if (OnScreen(window))
            menuTick = GetTickCount();
        if (!state.open && IsSettings(window) && *reinterpret_cast<float*>(window + 80) > 0.0f)
            PlaceKeyLine(window);
        if (state.open && window == state.window)
        {
            Tick(window);
            if (state.open)
            {
                NativeUpdate(window, edx, a2);
                Layout(window);
                return;
            }
        }
        NativeUpdate(window, edx, a2);
        // The game's rows are back at once; its text change effect returns from the next change on
        if (window == state.window && state.restoreTextChange)
        {
            state.restoreTextChange = false;
            *reinterpret_cast<uint8_t*>(window + 674) |= state.textChange << 6;
            for (auto text : ItemTexts(window))
                api.flag(text, nullptr, state.textChange);
        }
    }

    inline void* __fastcall Destroy(uintptr_t window, void* edx, int flags)
    {
        keyLines.erase(window);
        if (window == state.window)
        {
            state = State{};
            if (model.IsOpen())
                model.Close();
            blocking = false;
        }
        return NativeDestroy(window, edx, flags);
    }

    // The window's events: accepting the Plugins row opens the page
    inline int __fastcall Message(uintptr_t handler, void* edx, int event, int data)
    {
        const auto window = handler - 12;
        if (*reinterpret_cast<uintptr_t*>(window) != windowTable)
            return NativeMessage(handler, edx, event, data);
        if (state.open && window == state.window)
            return 0;
        if (IsSettings(window) && Selection(window) == int(ItemTexts(window).size()) - 1)
        {
            auto set = *reinterpret_cast<uintptr_t*>(window + 576);
            auto first = set ? *reinterpret_cast<int**>(set + 4) : nullptr;
            auto last = set ? *reinterpret_cast<int**>(set + 8) : nullptr;
            if (first && std::find(first, last, event) != last && (*reinterpret_cast<uint8_t*>(window + 674) & 1))
            {
                state.pluginsRow = Selection(window);
                Open(window);
                return 0;
            }
        }
        return NativeMessage(handler, edx, event, data);
    }

    inline int __fastcall RecordText(uintptr_t text, void* edx, GameString* value, int a, int b)
    {
        if (!settingOwnText && value)
        {
            const char* s = value->c_str();
            if (std::strstr(s, "{GUI_") && std::strstr(s, "fe_menuitem"))
                keyTexts.insert(text);
            if (!std::strncmp(s, "fe_", 3) || s[0] == '{')
            {
                auto& stored = gameTexts[text];
                if (stored != s)
                    stored = s;
            }
        }
        return NativeSetText(text, edx, value, a, b);
    }

    // The game's DirectInput devices read nothing while the page has the input, and until its keys are let go
    inline SafetyHookInline createDeviceHook;
    inline std::vector<SafetyHookInline> stateHooks, dataHooks;
    inline std::vector<void*> hookedTables;
    inline HRESULT(WINAPI* NativeCreate)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN) = nullptr;

    inline bool Blocks()
    {
        if (state.open)
            return true;
        if (!blocking)
            return false;
        for (int vk : { VK_RETURN, VK_ESCAPE, VK_BACK, VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT })
            if (GetAsyncKeyState(vk) & 0x8000)
                return true;
        XINPUT_STATE pad{};
        DWORD slot = 0;
        if (PadInput::Read(pad, slot) && (pad.Gamepad.wButtons & (XINPUT_GAMEPAD_A | XINPUT_GAMEPAD_B | 0x000F)))
            return true;
        blocking = false;
        return false;
    }

    inline HRESULT WINAPI DeviceState(void* device, DWORD size, void* data)
    {
        HRESULT result = E_FAIL;
        for (size_t i = 0; i < hookedTables.size(); ++i)
            if (*reinterpret_cast<void***>(device) == hookedTables[i])
                result = stateHooks[i].stdcall<HRESULT>(device, size, data);
        if (SUCCEEDED(result) && data && Blocks())
            std::memset(data, 0, size);
        return result;
    }

    inline HRESULT WINAPI DeviceData(void* device, DWORD size, void* data, DWORD* count, DWORD flags)
    {
        HRESULT result = E_FAIL;
        for (size_t i = 0; i < hookedTables.size(); ++i)
            if (*reinterpret_cast<void***>(device) == hookedTables[i])
                result = dataHooks[i].stdcall<HRESULT>(device, size, data, count, flags);
        if (SUCCEEDED(result) && count && Blocks())
            *count = 0;
        return result;
    }

    inline HRESULT WINAPI CreateDevice(void* directInput, const GUID* guid, void** device, void* outer)
    {
        const auto result = createDeviceHook.stdcall<HRESULT>(directInput, guid, device, outer);
        if (SUCCEEDED(result) && device && *device)
        {
            auto table = *reinterpret_cast<void***>(*device);
            if (std::find(hookedTables.begin(), hookedTables.end(), table) == hookedTables.end())
            {
                hookedTables.push_back(table);
                stateHooks.push_back(safetyhook::create_inline(table[9], DeviceState));
                dataHooks.push_back(safetyhook::create_inline(table[10], DeviceData));
            }
        }
        return result;
    }

    inline HRESULT WINAPI Create(HINSTANCE instance, DWORD version, REFIID iid, LPVOID* output, LPUNKNOWN outer)
    {
        const auto result = NativeCreate(instance, version, iid, output, outer);
        if (SUCCEEDED(result) && output && *output && !createDeviceHook)
        {
            auto table = *reinterpret_cast<void***>(*output);
            createDeviceHook = safetyhook::create_inline(table[3], CreateDevice);
        }
        return result;
    }

    // From the game window's procedure
    inline void WindowMessage(HWND window, UINT message, WPARAM wParam)
    {
        gameWindow = window;
        if (message == WM_MOUSEWHEEL && state.open)
            input.wheel += GET_WHEEL_DELTA_WPARAM(wParam);
    }

    inline void Init()
    {
        model.RememberRunningSettings(FixSettings());

        auto construct = hook::pattern("C7 00 ? ? ? ? 8B 8D 04 FF FF FF C7 41 0C ? ? ? ? 8B 95 04 FF FF FF C7 42 34");
        auto table = *construct.get_first<uintptr_t*>(2);
        auto messages = *construct.get_first<uintptr_t*>(15);
        windowTable = uintptr_t(table);
        NativeDestroy = reinterpret_cast<Destroy_t>(table[0]);
        injector::WriteMemory(&table[0], &Destroy, true);
        NativeLoad = reinterpret_cast<Load_t>(table[6]);
        injector::WriteMemory(&table[6], &Load, true);
        NativeUpdate = reinterpret_cast<Update_t>(table[14]);
        injector::WriteMemory(&table[14], &Update, true);
        NativeMessage = reinterpret_cast<Message_t>(messages[1]);
        injector::WriteMemory(&messages[1], &Message, true);
        auto inputs = *construct.get_first<uintptr_t*>(28);
        if (inputs[2] == uintptr_t(NativeMessage))
            injector::WriteMemory(&inputs[2], &Message, true);


        auto text = hook::pattern("8D 4E 5C 89 5C 24 1C C7 06 ? ? ? ? E8");
        api.textTable = *text.get_first<uintptr_t>(9);
        auto textTable = reinterpret_cast<uintptr_t*>(api.textTable);
        NativeSetText = reinterpret_cast<SetText_t>(textTable[27]);
        NativeTextDestroy = reinterpret_cast<TextDestroy_t>(textTable[0]);
        injector::WriteMemory(&textTable[0], &TextDestroy, true);
        injector::WriteMemory(&textTable[27], &RecordText, true);

        auto split = hook::pattern("83 C1 74 E8 ? ? ? ? 6A 00 8D 4D 88 E8 ? ? ? ? 68 ? ? ? ? E8");
        api.clear = reinterpret_cast<decltype(api.clear)>(injector::GetBranchDestination(split.get_first(3)).as_int());
        api.tidy = reinterpret_cast<decltype(api.tidy)>(injector::GetBranchDestination(split.get_first(13)).as_int());
        api.assign = reinterpret_cast<decltype(api.assign)>(injector::GetBranchDestination(split.get_first(40)).as_int());
        api.split = reinterpret_cast<decltype(api.split)>(injector::GetBranchDestination(split.get_first(73)).as_int());

        auto newText = hook::pattern("68 50 01 00 00 E8 ? ? ? ? 83 C4 04 89 45 E8 C7 45 FC 00 00 00 00 83 7D E8 00 74 0D 8B 4D E8 E8 ? ? ? ? 89 45 A8 EB 07 C7 45 A8 00 00 00 00 8B 45 A8 89 45 EC C7 45 FC FF FF FF FF 8B 4D EC 89 4D F0 68 00 00 80 3F 68 00 00 80 3F 8B 4D F0 E8 ? ? ? ? 8B 55 AC 8B 82 04 01 00 00");
        api.alloc = reinterpret_cast<decltype(api.alloc)>(injector::GetBranchDestination(newText.get_first(5)).as_int());
        api.construct = reinterpret_cast<decltype(api.construct)>(injector::GetBranchDestination(newText.get_first(32)).as_int());
        api.flag = reinterpret_cast<decltype(api.flag)>(injector::GetBranchDestination(newText.get_first(0x9C)).as_int());

        auto measure = hook::pattern("8D 45 B8 50 8B 4D C0 E8 ? ? ? ? D9 45 F0");
        api.measure = reinterpret_cast<decltype(api.measure)>(injector::GetBranchDestination(measure.get_first(7)).as_int());

        NativeCreate = JCImports::Replace("DINPUT8.dll", "DirectInput8Create", &Create);
    }
}
