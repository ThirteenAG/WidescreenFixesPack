module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/RainDroplets.hpp"
#include <unordered_map>
#include <cmath>
#include <span>

export module NativeMenu;

import EngineText;
import MenuBindings;

export namespace RE0Menu
{
    inline HMODULE module = nullptr;
    inline std::string updateNotice;
    inline SafetyHookInline initHook, execHook, inputHook, destroyHook, releaseHook;
    inline SafetyHookMid acceptHook;
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;

    inline void RememberStartupValues(HMODULE ownModule)
    {
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.try_emplace({ reinterpret_cast<uintptr_t>(ownModule), setting.id }, FixSettings().Get(setting.id));
    }

    inline void RememberRestartValues(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
        {
            if (!page.plugin) continue;
            for (const auto& setting : page.plugin->file.description.settings)
                if (setting.HasValue() && setting.apply == "restart")
                    restartValues.try_emplace({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id },
                        OtherPlugins::Get(*page.plugin, setting));
        }
    }

    inline bool RestartRequired(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
        {
            if (!page.plugin) continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart") continue;
                const auto initial = restartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting)) return true;
            }
        }
        return false;
    }

    // A line of the page: a row of the model, or Back (none)
    struct Item
    {
        std::string label, value;
        const RWGTAMenu::Row* row = nullptr;
        bool enabled = true;
    };

    // The look of the game's own settings pages (Video): names on the left, values centred on the right, eight lines
    enum class Align { Left, Centre, Right };
    namespace Look
    {
        // Text node colours are stored as A B G R
        constexpr uint32_t Rgb(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 255)
        {
            return (a << 24) | (b << 16) | (g << 8) | r;
        }
        constexpr float Top = 176.0f;
        constexpr float Pitch = 44.0f;
        constexpr float LabelLeft = 295.0f;
        constexpr float ValueCentre = 815.0f;
        constexpr float LeftArrow = 650.0f;
        constexpr float RightArrow = 978.0f;
        constexpr float UpArrow = 138.0f;
        constexpr float DownArrow = 509.0f;
        constexpr float Centre = 640.0f;
        constexpr float HelpLine = 586.0f;
        constexpr int Size = 34;
        constexpr int HelpSize = 24;
        constexpr uint32_t Selected = Rgb(224, 224, 224);
        constexpr uint32_t Normal = Rgb(118, 118, 118);
        constexpr uint32_t Dim = Rgb(60, 60, 60);
        constexpr uint32_t Arrow = Rgb(195, 195, 195);
        constexpr uint32_t Title = Rgb(170, 170, 170);
        constexpr uint32_t Help = Rgb(247, 247, 247);
    }

    // Lines of the page: the title, then the rows (Back among them) scrolling through the rest
    constexpr size_t Places = 8;
    constexpr size_t Slots = Places - 1;

    inline float Line(size_t place)
    {
        return Look::Top + Look::Pitch * place;
    }

    struct Text
    {
        void* node = nullptr;
        std::string value;
        std::array<float, 5> geometry{};
        uint32_t tint = 0;
        float width = 0;
        bool fitted = false;

        void Set(const std::string& text)
        {
            if (text == value) return;
            fitted = false;
            value = text;
            RE0Text::Set(node, value);
        }

        // x is the left edge or the centre of the text by the alignment; y is the centre of the line
        void Fit(const std::string& text, float x, float y, int height, float limit, uint32_t color, Align align = Align::Left)
        {
            const std::array<float, 5> style{ x, y, static_cast<float>(height), limit, static_cast<float>(align) };
            if (fitted && text == value && geometry == style && tint == color) return;
            RE0Text::Place(node, x, y - height * 0.5f, height, color);
            RE0Text::Set(node, text);
            auto size = height;
            auto measured = Read<float>(node, 216);
            if (std::isfinite(measured) && measured > limit)
            {
                size = std::max(12, static_cast<int>(std::floor(height * limit / measured)));
                RE0Text::Place(node, x, y - size * 0.5f, size, color);
                RE0Text::Set(node, text);
                measured = Read<float>(node, 216);
            }
            if (!std::isfinite(measured)) measured = 0;
            const float left = align == Align::Centre ? x - measured * 0.5f : align == Align::Right ? x - measured : x;
            RE0Text::Place(node, left, y - size * 0.5f, size, color);
            value = text;
            geometry = style;
            tint = color;
            width = measured;
            fitted = true;
        }
    };

    struct Host
    {
        void* gui = nullptr;
        void* root = nullptr;
        void* layer = nullptr;
        std::vector<std::pair<void*, uint32_t>> stockVisibility;
        std::array<Text, Slots> labels, values;
        std::array<Text, 9> helpLines;
        Text entry, heading, footer, restart, notice, left, right;
        std::vector<std::pair<void*, uint32_t>> entryLayers;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<std::string> help;
        size_t page = 0, helpStart = 0;
        uint32_t entryIndex = 0;
        bool open = false, inHelp = false, busy = false, hidden = false, noticeShown = false;
        MenuList::Repeat pageUp, pageDown;
        MenuList::WheelNotches wheel;
        bool homeHeld = false, endHeld = false;
        POINT mousePosition{};
        bool mouseSeen = false;
    };
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    inline std::unordered_map<void*, void*> failedRoots;

    inline bool Current(const std::shared_ptr<Host>& host)
    {
        return host && hosts.count(host->gui) && hosts.at(host->gui) == host &&
            Read<void*>(host->gui, 244) == host->root;
    }

    inline bool InGame(void* gui)
    {
        auto context = Read<void*>(gui, 0x260);
        return context && Read<uint8_t>(context, 0x4B) == 1;
    }

    inline std::string Language()
    {
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (!language.empty() && language != "auto")
            return language;
        // The game shows the language Steam runs it in
        static std::atomic<const char*> game{ nullptr };
        if (const auto known = game.load())
            return known;
        using Accessor = void* (__cdecl*)();
        using Name = const char* (__thiscall*)(void*);
        const auto steam = GetModuleHandleW(L"steam_api.dll");
        const auto accessor = steam ? reinterpret_cast<Accessor>(GetProcAddress(steam, "SteamApps")) : nullptr;
        const auto apps = accessor ? accessor() : nullptr;
        if (!apps)
            return "en";
        const auto current = reinterpret_cast<Name>((*reinterpret_cast<void***>(apps))[4])(apps);
        const std::string_view name = current ? current : "";
        const auto tag = name == "french" ? "fr" : name == "german" ? "de" : name == "italian" ? "it" :
            name == "spanish" || name == "latam" ? "es" : "en";
        game.store(tag);
        return tag;
    }

    inline void ShowPage(const std::shared_ptr<Host>& host)
    {
        if (!Current(host)) return;
        const bool restart = host->open && RestartRequired(host->model);
        if (!Current(host)) return;
        for (const auto& [node, flags] : host->entryLayers)
            Write(node, 84, (Read<uint32_t>(node, 84) & ~1u) | (flags & 1u));
        host->entryLayers.clear();
        if (host->open && !host->hidden)
        {
            host->stockVisibility.clear();
            for (size_t i = 0; i < 6; ++i)
            {
                auto node = Read<void*>(host->gui, 0x2C0 + i * 4);
                if (node) host->stockVisibility.emplace_back(node, Read<uint32_t>(node, 84));
            }
            // The Options resource's first instance draws the two inner rails from its texture atlas.
            auto nodes = Read<void**>(host->gui, 0xF8);
            auto border = nodes ? nodes[0] : nullptr;
            auto row = Read<void*>(host->gui, 0x2C0);
            if (border && row && Read<uintptr_t>(border, 0) == Read<uintptr_t>(row, 0) &&
                Read<void*>(border, 0x6C) == host->gui)
                host->stockVisibility.emplace_back(border, Read<uint32_t>(border, 84));
            auto scroll = Read<void*>(host->gui, 0x2B0);
            auto frame = scroll ? Read<void*>(scroll, 104) : nullptr;
            if (frame && frame != host->root && frame != host->layer)
                host->stockVisibility.emplace_back(frame, Read<uint32_t>(frame, 84));
            for (auto offset : { 0x2B4, 0x2B8, 0x2BC, 0x2E0 })
            {
                auto node = Read<void*>(host->gui, offset);
                if (node) host->stockVisibility.emplace_back(node, Read<uint32_t>(node, 84));
            }
            host->hidden = true;
        }
        if (host->hidden)
        {
            for (const auto& [node, flags] : host->stockVisibility)
                Write(node, 84, (Read<uint32_t>(node, 84) & ~1u) | (host->open ? 0u : flags & 1u));
            if (!host->open) host->hidden = false;
        }
        const auto first = Read<uint32_t>(host->gui, 0x298);
        const bool visible = !host->open && host->entryIndex >= first && host->entryIndex < first + 6;
        if (visible)
        {
            const auto slot = host->entryIndex - first;
            auto actor = Read<void*>(host->gui, 0x2C0 + slot * 4);
            if (actor)
            {
                host->entryLayers.emplace_back(actor, Read<uint32_t>(actor, 84));
                RE0Text::Show(actor, false);
            }
            float y = 0.0f;
            auto parent = actor;
            for (size_t depth = 0; parent && parent != host->layer && depth < 16; ++depth)
            {
                y += Read<float>(parent, 164);
                parent = Read<void*>(parent, 104);
            }
            if (parent == host->layer && std::isfinite(y))
            {
                const auto width = Read<float>(host->entry.node, 216);
                RE0Text::Place(host->entry.node, 640.0f - width * 0.5f, y - 4, 40,
                    Read<uint32_t>(host->gui, 0x29C) == slot ? 0xFFFFFFFF : 0xFFB8B09C);
            }
        }
        const bool entrySelected = visible && host->entryIndex == first + Read<uint32_t>(host->gui, 0x29C);
        if (entrySelected)
        {
            auto description = Read<void*>(host->gui, 0x2E0);
            if (description)
            {
                host->entryLayers.emplace_back(description, Read<uint32_t>(description, 84));
                RE0Text::Show(description, false);
            }
        }
        RE0Text::Show(host->entry.node, visible);
        const bool hasUpdate = !updateNotice.empty() && FixSettings().Get("MAIN.CheckForUpdates") == "1";
        auto texts = FixSettings().GetTexts();
        texts.Select(Language());
        host->notice.Fit(hasUpdate ? texts.Resolve("@update.short") : "", Look::Centre, 616, 18, 760, Look::Help, Align::Centre);
        RE0Text::Show(host->notice.node, hasUpdate && !host->open);
        RE0Text::Show(host->heading.node, host->open);
        if (!host->open)
            host->footer.Fit(entrySelected ? texts.Resolve("@menu.pluginshelp") : "", Look::Centre, Look::HelpLine,
                Look::HelpSize, 760, Look::Help, Align::Centre);
        RE0Text::Show(host->footer.node, host->open || entrySelected);
        RE0Text::Show(host->restart.node, restart);
        const bool list = host->open && !host->inHelp;
        for (size_t i = 0; i < host->labels.size(); ++i)
        {
            const bool shown = list && i < host->items.size();
            RE0Text::Show(host->labels[i].node, shown);
            RE0Text::Show(host->values[i].node, shown && !host->values[i].value.empty());
        }
        auto& window = host->model.Window(host->page);
        const auto slot = window.Slot();
        const bool arrows = list && slot < host->items.size() && host->items[slot].row && host->items[slot].enabled &&
            !RWGTAMenu::Model::Opens(*host->items[slot].row) && !host->items[slot].value.empty();
        RE0Text::Show(host->left.node, arrows);
        RE0Text::Show(host->right.node, arrows);
        // Rows above or below the window: the Options list's own scroll arrows
        for (auto offset : { 0x2B8, 0x2BC })
        {
            auto arrow = Read<void*>(host->gui, offset);
            auto other = Read<void*>(host->gui, offset == 0x2B8 ? 0x2BC : 0x2B8);
            if (!arrow || !other || !host->open) continue;
            const bool up = Read<float>(arrow, 164) < Read<float>(other, 164);
            RE0Text::Show(arrow, list && (up ? window.Above() : window.Below()));
        }
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            RE0Text::Show(host->helpLines[i].node, host->open && host->inHelp && host->helpStart + i < host->help.size());
    }

    inline Text MakeText(const std::shared_ptr<Host>& host, int size)
    {
        Text text{ RE0Text::Append(host->gui, host->layer, 1000), {} };
        RE0Text::Place(text.node, 0, 0, size);
        RE0Text::Show(text.node, false);
        return text;
    }

    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open) return;
        auto& model = host->model;
        model.SelectLanguage(Language());
        if (host->page >= model.Pages().size()) host->page = RWGTAMenu::Model::RootPage;
        std::vector<Item> items;
        const std::string title = model.Pages()[host->page].title;
        for (auto row : model.Shown(host->page, true))
        {
            if (!row)
            {
                items.push_back({ model.Text("menu.back"), "", nullptr });
                continue;
            }
            auto label = model.Label(*row);
            auto value = model.Value(*row);
            auto enabled = model.Enabled(*row, InGame(host->gui));
            if (!Current(host)) return;
            items.push_back({ std::move(label), std::move(value), row, enabled });
        }
        host->items = std::move(items);
        const size_t selection = model.Window(host->page).Slot();
        host->heading.Fit(title, Look::Centre, Line(0), Look::Size, 760, Look::Title, Align::Centre);
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            const auto& item = host->items[i];
            const auto color = !item.enabled ? Look::Dim : i == selection ? Look::Selected : Look::Normal;
            const auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            host->labels[i].Fit(item.label, Look::LabelLeft, Line(i + 1), Look::Size, value.empty() ? 690.0f : 330.0f, color);
            host->values[i].Fit(value, Look::ValueCentre, Line(i + 1), Look::Size, 280, color, Align::Centre);
        }
        if (selection < host->items.size())
        {
            host->left.Fit("\xC2\xAB", Look::LeftArrow, Line(selection + 1), 30, 40, Look::Arrow, Align::Centre);
            host->right.Fit("\xC2\xBB", Look::RightArrow, Line(selection + 1), 30, 40, Look::Arrow, Align::Centre);
        }
        const auto* selected = selection < host->items.size() ? host->items[selection].row : nullptr;
        const bool hasHelp = selected && !RWGTAMenu::Model::Opens(*selected) && !model.Help(*selected).empty();
        const bool restart = RestartRequired(model);
        if (!Current(host)) return;
        host->footer.Fit(hasHelp ? model.Text("menu.helphint") : "", Look::Centre, Look::HelpLine, Look::HelpSize, 760,
            Look::Help, Align::Centre);
        host->restart.Fit(restart ? model.Text("menu.restartwarning") : "", Look::Centre, Look::HelpLine + 30,
            Look::HelpSize, 760, Look::Help, Align::Centre);
        ShowPage(host);
    }

    inline void Close(const std::shared_ptr<Host>& host)
    {
        host->open = host->inHelp = false;
        host->model.Close();
        ShowPage(host);
    }

    // Back: the help page closes, a page returns to the list it was opened from (that row selected), the plugins list
    // closes the menu
    inline void Back(const std::shared_ptr<Host>& host)
    {
        if (host->inHelp) { host->inHelp = false; Populate(host); return; }
        const auto parent = host->model.Back(host->page);
        if (parent == SIZE_MAX) { Close(host); return; }
        host->page = parent;
        Populate(host);
    }

    inline const RWGTAMenu::Row* SelectedRow(const std::shared_ptr<Host>& host)
    {
        const auto slot = host->model.Window(host->page).Slot();
        return slot < host->items.size() ? host->items[slot].row : nullptr;
    }

    // The selected line: left (-1), right (+1) or accept (0)
    inline void Change(const std::shared_ptr<Host>& host, int direction)
    {
        if (!Current(host)) return;
        const auto slot = host->model.Window(host->page).Slot();
        if (slot >= host->items.size()) return;
        const auto item = host->items[slot];
        if (!item.row)
        {
            if (!direction) Back(host);
            return;
        }
        if (RWGTAMenu::Model::Opens(*item.row))
        {
            const auto page = direction ? SIZE_MAX : host->model.Enter(*item.row);
            if (page != SIZE_MAX) host->page = page;
        }
        else
            host->model.Change(*item.row, direction, InGame(host->gui));
        Populate(host);
    }

    // The lines of the help page from helpStart, centred like the game's help line
    inline void HelpLines(const std::shared_ptr<Host>& host)
    {
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            host->helpLines[i].Fit(host->helpStart + i < host->help.size() ? host->help[host->helpStart + i] : "",
                Look::Centre, Line(1) + 34.0f * i, Look::HelpSize, 760, Look::Help, Align::Centre);
    }

    inline void Help(const std::shared_ptr<Host>& host, const std::string* message = nullptr, const std::string* title = nullptr)
    {
        const auto* row = SelectedRow(host);
        if (!message && (!row || RWGTAMenu::Model::Opens(*row))) return;
        auto content = message ? *message : host->model.Help(*row);
        if (content.empty() || !Current(host)) return;
        host->help.clear();
        std::string line;
        RE0Text::Place(host->helpLines[0].node, 0, 0, Look::HelpSize);
        // Measure complete UTF-8 words with the same native font used to draw the help page.
        for (size_t first = 0; first < content.size();)
        {
            auto last = content.find_first_of(" \r\n", first);
            if (last == std::string::npos) last = content.size();
            auto word = content.substr(first, last - first);
            auto joined = line.empty() ? word : line + " " + word;
            RE0Text::Set(host->helpLines[0].node, joined);
            if (!line.empty() && Read<float>(host->helpLines[0].node, 216) > 720)
            {
                host->help.push_back(line);
                line = std::move(word);
            }
            else line = std::move(joined);
            if (last < content.size() && content[last] == '\n') { host->help.push_back(line); line.clear(); }
            first = last + 1;
        }
        if (!line.empty()) host->help.push_back(line);
        host->helpStart = 0;
        host->inHelp = true;
        host->heading.Fit(title ? *title : host->model.Label(*row), Look::Centre, Line(0), Look::Size, 760, Look::Title,
            Align::Centre);
        // Measuring used the first node directly, so its cached text is stale
        host->helpLines[0].fitted = false;
        host->helpLines[0].value.clear();
        HelpLines(host);
        host->footer.Fit(host->model.Text("menu.back"), Look::Centre, Look::HelpLine, Look::HelpSize, 760, Look::Help,
            Align::Centre);
        ShowPage(host);
    }

    inline bool MouseClick(const void* device)
    {
        return (Read<uint32_t>(device, 0x40) & 1) != 0;
    }

    inline bool MenuPoint(const void* reference, POINT point, float& x, float& y)
    {
        if (!reference) return false;
        const auto xx = Read<float>(reference, 16), xy = Read<float>(reference, 20);
        const auto yx = Read<float>(reference, 32), yy = Read<float>(reference, 36);
        const auto px = point.x - Read<float>(reference, 64), py = point.y - Read<float>(reference, 68);
        const auto determinant = xx * yy - xy * yx;
        if (!std::isfinite(determinant) || std::abs(determinant) < 0.000001f) return false;
        x = (px * yy - py * yx) / determinant + Read<float>(reference, 160);
        y = (py * xx - px * xy) / determinant + Read<float>(reference, 164);
        return std::isfinite(x) && std::isfinite(y);
    }

    // The mouse: the key guide's buttons, the line under it while it moves (the window's slot plus its scroll), a click
    // on a value's arrow changes it, a click on a scroll arrow scrolls
    inline bool MouseActions(const std::shared_ptr<Host>& host, HWND window, bool navigation)
    {
        auto device = mouse.get();
        RECT client{};
        if (!device || !GetClientRect(window, &client) || client.right <= 0 || client.bottom <= 0) return false;
        const POINT point{ Read<int32_t>(device, 0x20), Read<int32_t>(device, 0x24) };
        const bool moved = host->mouseSeen && (point.x != host->mousePosition.x || point.y != host->mousePosition.y);
        host->mouseSeen = true;
        host->mousePosition = point;
        const bool click = MouseClick(device);
        // The native mouse cache is already in scene coordinates; undo the text node's current layout transform.
        float x = 0.0f, y = 0.0f;
        if (!MenuPoint(host->heading.node, point, x, y)) return false;
        if (click && y >= 630 && y <= 680)
        {
            if (x >= 780 && x <= 980) Back(host);
            else if (x >= 580 && x <= 760 && !host->inHelp) Change(host, 0);
            else if (x >= 400 && x < 580 && !host->inHelp) Help(host);
            else return false;
            return true;
        }
        if (host->inHelp)
        {
            if (click && x >= 280 && x <= 1040 && std::abs(y - Look::HelpLine) < 20)
            {
                Back(host);
                return true;
            }
            return false;
        }
        auto& list = host->model.Window(host->page);
        if (click && std::abs(x - Look::Centre) < 30)
        {
            if (std::abs(y - Look::UpArrow) < 16) { list.Scroll(-1); return true; }
            if (std::abs(y - Look::DownArrow) < 16) { list.Scroll(1); return true; }
        }
        if (!(click || (moved && !navigation)) || x < 280 || x >= 1000) return false;
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            if (std::abs(y - Line(i + 1)) >= Look::Pitch * 0.5f) continue;
            list.Point(i);
            if (!click) return false;
            const bool arrows = host->items[i].row && !RWGTAMenu::Model::Opens(*host->items[i].row) &&
                !host->items[i].value.empty();
            Change(host, !arrows ? 0 : std::abs(x - Look::LeftArrow) < 30 ? -1 : std::abs(x - Look::RightArrow) < 30 ? 1 : 0);
            return true;
        }
        return false;
    }

    inline void RestoreEntry(const std::shared_ptr<Host>& host)
    {
        for (const auto& [node, flags] : host->entryLayers)
            Write(node, 84, (Read<uint32_t>(node, 84) & ~1u) | (flags & 1u));
        host->entryLayers.clear();
    }

    inline bool PrepareHost(void* gui)
    {
        if (hosts.count(gui)) return Current(hosts.at(gui));
        auto root = Read<void*>(gui, 244);
        if (failedRoots.count(gui) && failedRoots.at(gui) == root) return false;
        const auto count = Read<uint32_t>(gui, 0x294);
        if (!root || count < 2 || count >= 10 || Read<uint32_t>(gui, 0x26C + (count - 1) * 4) != 9)
            return false;
        auto host = std::make_shared<Host>();
        host->gui = gui;
        host->root = root;
        // The stock nodes hang below one layer whose transform scales the 1280-wide layout to the screen
        host->layer = Read<void*>(gui, 0x2E0);
        while (host->layer && Read<void*>(host->layer, 104) != root)
            host->layer = Read<void*>(host->layer, 104);
        if (!host->layer)
            host->layer = root;
        host->entryIndex = count - 1;
        host->entry = MakeText(host, 40);
        host->heading = MakeText(host, Look::Size);
        host->footer = MakeText(host, Look::HelpSize);
        host->restart = MakeText(host, Look::HelpSize);
        host->notice = MakeText(host, 18);
        for (size_t i = 0; i < host->labels.size(); ++i)
        {
            host->labels[i] = MakeText(host, Look::Size);
            host->values[i] = MakeText(host, Look::Size);
        }
        host->left = MakeText(host, 30);
        host->right = MakeText(host, 30);
        for (auto& line : host->helpLines) line = MakeText(host, Look::HelpSize);
        bool made = true;
        for (const auto* text : { &host->entry, &host->heading, &host->footer, &host->restart, &host->notice, &host->left,
            &host->right }) made = made && text->node;
        for (const auto& group : { std::span<const Text>(host->labels), std::span<const Text>(host->values),
            std::span<const Text>(host->helpLines) })
            made = made && std::none_of(group.begin(), group.end(), [](const Text& t) { return !t.node; });
        if (!made)
        {
            failedRoots[gui] = root;
            return false;
        }
        hosts.emplace(gui, host);
        host->model.OpenMenu(module, Language(), Slots);
        RememberRestartValues(host->model);
        if (!Current(host)) return false;
        host->entry.Set(host->model.Text("menu.plugins"));
        host->model.Close();
        // Reuse Back's atlas index so every native lookup remains within the original ten-item table.
        Write(gui, 0x26C + count * 4, uint32_t{ 9 });
        Write(gui, 0x294, count + 1);
        refreshLabels(gui);
        ShowPage(host);
        return true;
    }

    inline uintptr_t __fastcall Initialize(void* gui, void*)
    {
        auto found = hosts.find(gui);
        if (found != hosts.end())
        {
            if (Current(found->second)) RestoreEntry(found->second);
            hosts.erase(found);
        }
        return initHook.thiscall<uintptr_t>(gui);
    }

    inline void* __fastcall Destroy(void* gui, void*, uint32_t flags)
    {
        hosts.erase(gui);
        failedRoots.erase(gui);
        RainDroplets::MenuOpen(false);
        return destroyHook.thiscall<void*>(gui, flags);
    }

    inline uintptr_t __fastcall Release(void* gui, void*)
    {
        hosts.erase(gui);
        failedRoots.erase(gui);
        return releaseHook.thiscall<uintptr_t>(gui);
    }

    inline uintptr_t __fastcall Exec(void* gui, void*)
    {
        if (hosts.count(gui) && !Current(hosts.at(gui))) hosts.erase(gui);
        const auto result = execHook.thiscall<uintptr_t>(gui);
        // The Options menu is on the screen while it is active, from its opening to its closing
        RainDroplets::MenuOpen(Read<uint8_t>(gui, 0x254) != 0);
        RWGTA::UpdateCheck::Tick();
        if (Read<uint8_t>(gui, 0x254) && Read<uint32_t>(gui, 0x268) == 3 && PrepareHost(gui))
            ShowPage(hosts.at(gui));
        return result;
    }

    inline uintptr_t __fastcall Input(void* gui, void*)
    {
        const auto found = hosts.find(gui);
        if (found == hosts.end() || !Current(found->second) || !found->second->open)
        {
            return inputHook.thiscall<uintptr_t>(gui);
        }
        const auto host = found->second;
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        if (host->busy || foregroundProcess != GetCurrentProcessId()) return 0;
        auto controller = pad.get();
        auto keys = keyboard.get();
        auto context = Read<void*>(gui, 0x260);
        if (!controller || !keys || !context) return 0;
        // The presses of this frame, and the directions with the game's own menu repeat (keys, d-pad and stick), as the
        // Options list reads them
        auto buttons = held();
        auto repeat = pressed();
        remap(context, &buttons, &repeat);
        const auto now = GetTickCount64();
        const bool accept = (buttons & Read<uint32_t>(controller, 0xB6C)) != 0;
        const bool back = (buttons & Read<uint32_t>(controller, 0xB70)) != 0;
        if (back) { Back(host); return 0; }
        auto device = mouse.get();
        // The game's wheel delta of this frame. The game also turns the wheel into Up/Down; here it scrolls the list.
        const int wheelDelta = device ? Read<int32_t>(device, 0x34) / 2 : 0;
        if (wheelDelta) repeat &= ~0x50050u;
        if (MouseActions(host, GetForegroundWindow(), buttons != 0)) { if (!host->inHelp) Populate(host); return 0; }
        if (!Current(host)) return 0;
        if (host->inHelp)
        {
            if ((repeat & 0x40040) && host->helpStart + host->helpLines.size() < host->help.size()) ++host->helpStart;
            if ((repeat & 0x10010) && host->helpStart) --host->helpStart;
            HelpLines(host);
            ShowPage(host);
            return 0;
        }
        auto& window = host->model.Window(host->page);
        for (int notches = host->wheel.Take(wheelDelta); notches; notches += notches > 0 ? -1 : 1)
            window.Scroll(notches > 0 ? -1 : 1);
        if (repeat & 0x40040) window.Move(1);
        if (repeat & 0x10010) window.Move(-1);
        // Page Up/Page Down a window, Home/End the ends
        const auto down = [&](int key)
        {
            return (Read<uint32_t>(keys, 0x1C0 + (key >> 5) * 4) & (1u << (key & 31))) != 0;
        };
        if (host->pageUp.Step(down(VK_PRIOR), now)) window.Page(-1);
        if (host->pageDown.Step(down(VK_NEXT), now)) window.Page(1);
        if (down(VK_HOME) && !host->homeHeld) window.Edge(-1);
        if (down(VK_END) && !host->endHeld) window.Edge(1);
        host->homeHeld = down(VK_HOME);
        host->endHeld = down(VK_END);
        if (repeat & 0x80080) Change(host, -1);
        if (repeat & 0x20020) Change(host, 1);
        if (accept) Change(host, 0);
        if ((buttons & 0x8000) || down(VK_F1)) Help(host);
        if (!host->inHelp) Populate(host);
        return 0;
    }

    inline void Install(HMODULE ownModule)
    {
        module = ownModule;
        RE0Text::Bind();
        Bind();
        initHook = safetyhook::create_inline(initialize, Initialize);
        execHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[8]), Exec);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[0]), Destroy);
        releaseHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[36]), Release);
        inputHook = safetyhook::create_inline(processInput, Input);
        acceptHook = safetyhook::create_mid(dispatch, [](SafetyHookContext& regs)
        {
            auto gui = reinterpret_cast<void*>(regs.esi);
            auto found = hosts.find(gui);
            if (found == hosts.end() || !Current(found->second) || Read<uint8_t>(gui, 0x264)) return;
            const auto host = found->second;
            const auto selected = Read<uint32_t>(gui, 0x298) + Read<uint32_t>(gui, 0x29C);
            if (selected != host->entryIndex) return;
            regs.eip = reinterpret_cast<uintptr_t>(inputReturn);
            host->busy = true;
            host->model.OpenMenu(module, Language(), Slots);
            RememberRestartValues(host->model);
            host->busy = false;
            if (!Current(host)) return;
            host->open = true;
            host->page = RWGTAMenu::Model::RootPage;
            host->model.Window(host->page).Reset();
            host->wheel.Clear();
            Populate(host);
            // A newer version found by the update check is told once, on the first opening
            if (!host->noticeShown && !updateNotice.empty() && FixSettings().Get("MAIN.CheckForUpdates") == "1")
            {
                host->noticeShown = true;
                const auto title = host->model.Text("update.title");
                Help(host, &updateNotice, &title);
            }
        });
        RWGTA::UpdateCheck::Config updates;
        updates.pluginId = "ResidentEvil0.FusionFix";
        updates.fileVersion = rsc_FileVersion;
        updates.settings = &FixSettings();
        updates.playable = []()
        {
            return std::any_of(hosts.begin(), hosts.end(), [](const auto& entry)
            {
                return Current(entry.second) && Read<uint8_t>(entry.first, 0x254) && Read<uint32_t>(entry.first, 0x268) == 3;
            });
        };
        updates.show = [](const std::wstring& notice) { updateNotice = asi_settings::text::Narrow(notice); };
        updates.format = [](const std::string& version, bool hub)
        {
            auto texts = FixSettings().GetTexts();
            texts.Select(Language());
            auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            auto replace = [&line](const char* key, const std::string& value)
            {
                const auto at = line.find(key);
                if (at != std::string::npos) line.replace(at, strlen(key), value);
            };
            replace("{name}", texts.Resolve(FixSettings().GetDescription().name));
            replace("{version}", version);
            return asi_settings::text::Widen(line);
        };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}
