module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <unordered_map>
#include <cmath>

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

    struct Item
    {
        std::string label, value;
        RWGTAMenu::Row row;
        int link = -1;
        bool enabled = true;
    };

    struct Text
    {
        void* node = nullptr;
        std::string value;
        std::array<float, 4> geometry{};
        uint32_t tint = 0;
        bool fitted = false;

        void Set(const std::string& text)
        {
            if (text == value) return;
            fitted = false;
            value = text;
            RE0Text::Set(node, value);
        }

        void Fit(const std::string& text, float x, float y, int height, float width, uint32_t color)
        {
            const std::array<float, 4> style{ x, y, static_cast<float>(height), width };
            if (fitted && text == value && geometry == style && tint == color) return;
            RE0Text::Place(node, x, y, height, color);
            RE0Text::Set(node, text);
            const auto measured = Read<float>(node, 216);
            if (std::isfinite(measured) && measured > width)
            {
                const auto smaller = std::max(12, static_cast<int>(std::floor(height * width / measured)));
                RE0Text::Place(node, x, y + (height - smaller) * 0.5f, smaller, color);
                RE0Text::Set(node, text);
            }
            value = text;
            geometry = style;
            tint = color;
            fitted = true;
        }
    };

    struct Host
    {
        void* gui = nullptr;
        void* root = nullptr;
        void* layer = nullptr;
        std::vector<std::pair<void*, uint32_t>> stockVisibility;
        std::array<Text, 8> labels, values;
        std::array<Text, 9> helpLines;
        Text entry, heading, footer, restart, notice;
        std::vector<std::pair<void*, uint32_t>> entryLayers;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<std::string> help;
        size_t page = 0, rowStart = 0, selection = 0, helpStart = 0;
        uint32_t entryIndex = 0;
        bool open = false, inHelp = false, busy = false, hidden = false;
        uint64_t nextRepeat = 0;
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
        auto language = FixSettings().String("MAIN.TextLanguage");
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
        const bool hasUpdate = !updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates");
        auto texts = FixSettings().GetTexts();
        texts.Select(Language());
        host->notice.Fit(hasUpdate ? texts.Resolve("@update.short") : "", 280, 610, 18, 760, 0xFFFFFFFF);
        RE0Text::Show(host->notice.node, hasUpdate && !host->open);
        RE0Text::Show(host->heading.node, host->open);
        if (!host->open)
            host->footer.Fit(entrySelected ? texts.Resolve("@menu.pluginshelp") : "", 280, 556, 20, 760, 0xFFFFFFFF);
        RE0Text::Show(host->footer.node, host->open || entrySelected);
        RE0Text::Show(host->restart.node, restart);
        for (size_t i = 0; i < host->labels.size(); ++i)
        {
            const bool shown = host->open && !host->inHelp && i < host->items.size();
            RE0Text::Show(host->labels[i].node, shown);
            RE0Text::Show(host->values[i].node, shown);
        }
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            RE0Text::Show(host->helpLines[i].node, host->open && host->inHelp && host->helpStart + i < host->help.size());
    }

    inline Text MakeText(const std::shared_ptr<Host>& host, float x, float y, int size, uint32_t priority)
    {
        Text text{ RE0Text::Append(host->gui, host->layer, priority), {} };
        RE0Text::Place(text.node, x, y, size);
        RE0Text::Show(text.node, false);
        return text;
    }

    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open) return;
        auto& model = host->model;
        const auto pages = model.Pages();
        std::vector<Item> items;
        std::string title = model.Text("menu.plugins");
        if (!host->page)
        {
            const auto pageRows = !updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates") ? 4u : 5u;
            if (host->rowStart >= pages.size()) host->rowStart = 0;
            for (size_t i = host->rowStart; i < pages.size() && i < host->rowStart + pageRows; ++i)
                items.push_back({ pages[i].title, "", {}, static_cast<int>(i + 1) });
            if (host->rowStart) items.push_back({ model.Text("menu.previous"), "", {}, -2 });
            if (host->rowStart + pageRows < pages.size()) items.push_back({ model.Text("menu.next"), "", {}, -3 });
            if (pageRows == 4) items.push_back({ model.Text("update.title"), "", {}, -5 });
        }
        else if (host->page <= pages.size())
        {
            const auto page = host->page - 1;
            title = pages[page].title;
            auto rows = model.VisibleRows(page);
            if (!Current(host)) return;
            if (host->rowStart >= rows.size()) host->rowStart = 0;
            for (size_t i = host->rowStart; i < rows.size() && i < host->rowStart + 5; ++i)
            {
                auto row = *rows[i];
                auto label = model.Label(row);
                auto value = model.Value(row);
                auto enabled = model.Enabled(row, InGame(host->gui));
                if (!Current(host)) return;
                items.push_back({ std::move(label), std::move(value), row, -1, enabled });
            }
            if (host->rowStart) items.push_back({ model.Text("menu.previous"), "", {}, -2 });
            if (host->rowStart + 5 < rows.size()) items.push_back({ model.Text("menu.next"), "", {}, -3 });
        }
        items.push_back({ model.Text("menu.back"), "", {}, -4 });
        host->items = std::move(items);
        host->selection = std::min(host->selection, host->items.size() - 1);
        host->heading.Fit(title, 280, 205, 24, 760, 0xFFFFFFFF);
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            const auto& item = host->items[i];
            const auto color = i == host->selection ? 0xFFFFFFFFu : item.enabled ? 0xFFB8B09Cu : 0xFF888888u;
            host->labels[i].Fit(item.label, 280, 250 + 30.0f * i, 22, 500, color);
            host->values[i].Fit(item.enabled ? item.value : model.Text("menu.unavailable"),
                820, 250 + 30.0f * i, 22, 220, color);
        }
        const auto& selected = host->items[host->selection];
        const bool hasHelp = selected.link == -5 || (selected.link == -1 && !model.Help(selected.row).empty());
        const bool restart = RestartRequired(model);
        if (!Current(host)) return;
        host->footer.Set(hasHelp ? model.Text("menu.helphint") : "");
        host->restart.Fit(restart ? model.Text("menu.restartwarning") : "", 280, 590, 20, 760, 0xFFFFFFFF);
        ShowPage(host);
    }

    inline void Close(const std::shared_ptr<Host>& host)
    {
        host->open = host->inHelp = false;
        host->model.Close();
        ShowPage(host);
    }

    inline void Back(const std::shared_ptr<Host>& host)
    {
        if (host->inHelp) { host->inHelp = false; Populate(host); }
        else if (host->page) { host->page = host->rowStart = host->selection = 0; Populate(host); }
        else Close(host);
    }

    inline void Help(const std::shared_ptr<Host>& host);

    inline void Change(const std::shared_ptr<Host>& host, int direction)
    {
        if (!Current(host) || host->selection >= host->items.size()) return;
        const auto item = host->items[host->selection];
        if (item.link == -1)
            host->model.Change(item.row, direction, InGame(host->gui));
        else if (!direction)
        {
            if (item.link == -4) { Back(host); return; }
            if (item.link == -5) { Help(host); return; }
            const auto pageRows = !host->page && !updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates") ? 4u : 5u;
            if (item.link == -2) host->rowStart = host->rowStart >= pageRows ? host->rowStart - pageRows : 0;
            else if (item.link == -3) host->rowStart += pageRows;
            else if (item.link > 0)
            {
                host->page = item.link;
                host->rowStart = 0;
                host->model.PageShown(host->page - 1);
            }
            host->selection = 0;
        }
        Populate(host);
    }

    inline void Help(const std::shared_ptr<Host>& host)
    {
        if (host->selection >= host->items.size()) return;
        const auto& item = host->items[host->selection];
        if (item.link != -1 && item.link != -5) return;
        auto content = item.link == -5 ? updateNotice : host->model.Help(item.row);
        if (content.empty() || !Current(host)) return;
        host->help.clear();
        std::string line;
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
        host->heading.Fit(host->items[host->selection].label, 280, 205, 24, 760, 0xFFFFFFFF);
        for (size_t i = 0; i < host->helpLines.size(); ++i)
        {
            // Measuring used this node directly, so invalidate its previous cached value.
            host->helpLines[i].value.clear();
            host->helpLines[i].Set(i < host->help.size() ? host->help[i] : "");
        }
        host->footer.Set(host->model.Text("menu.back"));
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
            if (click && x >= 280 && x <= 1040 && y >= 535 && y <= 580)
            {
                Back(host);
                return true;
            }
            return false;
        }
        if ((click || (moved && !navigation)) && x >= 280 && x <= 1040 &&
            y >= 245 && y < 245 + 30.0f * host->items.size())
        {
            const auto row = static_cast<size_t>((y - 245) / 30);
            if (row < host->items.size())
            {
                host->selection = row;
                if (click)
                {
                    const auto& item = host->items[row];
                    Change(host, item.link == -1 && x >= 820 ? (x < 930 ? -1 : 1) : 0);
                    return true;
                }
            }
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
        host->entry = MakeText(host, 0, 0, 40, 1000);
        host->heading = MakeText(host, 280, 205, 24, 1000);
        host->footer = MakeText(host, 280, 550, 20, 1000);
        host->restart = MakeText(host, 280, 590, 20, 1000);
        host->notice = MakeText(host, 280, 610, 18, 1000);
        for (size_t i = 0; i < host->labels.size(); ++i)
        {
            host->labels[i] = MakeText(host, 280, 250 + 30.0f * i, 22, 1000);
            host->values[i] = MakeText(host, 820, 250 + 30.0f * i, 22, 1000);
        }
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            host->helpLines[i] = MakeText(host, 280, 250 + 26.0f * i, 20, 1000);
        if (!host->entry.node || !host->heading.node || !host->footer.node || !host->restart.node || !host->notice.node ||
            std::any_of(host->labels.begin(), host->labels.end(), [](const auto& t) { return !t.node; }) ||
            std::any_of(host->values.begin(), host->values.end(), [](const auto& t) { return !t.node; }) ||
            std::any_of(host->helpLines.begin(), host->helpLines.end(), [](const auto& t) { return !t.node; }))
        {
            failedRoots[gui] = root;
            return false;
        }
        hosts.emplace(gui, host);
        host->model.OpenPlugins(module, Language(), 4096);
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
        auto buttons = pressed();
        auto repeat = held();
        remap(context, &repeat, &buttons);
        const auto now = GetTickCount64();
        if (buttons & 0xF0) host->nextRepeat = now + 350;
        else if (now < host->nextRepeat) repeat = 0;
        else if (repeat & 0xF0) host->nextRepeat = now + 80;
        const bool accept = (buttons & Read<uint32_t>(controller, 0xB6C)) != 0;
        const bool back = (buttons & Read<uint32_t>(controller, 0xB70)) != 0;
        if (back) { Back(host); return 0; }
        if (MouseActions(host, GetForegroundWindow(), buttons != 0)) return 0;
        if (!Current(host)) return 0;
        if (host->inHelp)
        {
            if ((repeat & 0x40) && host->helpStart + host->helpLines.size() < host->help.size()) ++host->helpStart;
            if ((repeat & 0x10) && host->helpStart) --host->helpStart;
            for (size_t i = 0; i < host->helpLines.size(); ++i)
                host->helpLines[i].Set(host->helpStart + i < host->help.size() ? host->help[host->helpStart + i] : "");
            ShowPage(host);
            return 0;
        }
        if (repeat & 0x40) host->selection = (host->selection + 1) % host->items.size();
        if (repeat & 0x10) host->selection = (host->selection + host->items.size() - 1) % host->items.size();
        if (repeat & 0x80) Change(host, -1);
        if (repeat & 0x20) Change(host, 1);
        if (accept) Change(host, 0);
        if ((buttons & 0x8000) || (Read<uint32_t>(keys, 0x1C0 + (VK_F1 >> 5) * 4) & (1u << (VK_F1 & 31)))) Help(host);
        if (!host->inHelp) Populate(host);
        return 0;
    }

    inline bool Install(HMODULE ownModule)
    {
        module = ownModule;
        if (!RE0Text::Bind() || !Bind()) return false;
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
            host->model.OpenPlugins(module, Language(), 4096);
            RememberRestartValues(host->model);
            host->busy = false;
            if (!Current(host)) return;
            host->open = true;
            host->page = host->rowStart = host->selection = 0;
            Populate(host);
        });
        const bool ready = initHook && execHook && destroyHook && releaseHook && inputHook && acceptHook;
        if (!ready)
        {
            initHook.reset(); execHook.reset(); destroyHook.reset(); releaseHook.reset(); inputHook.reset(); acceptHook.reset();
        }
        if (ready)
        {
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
        return ready;
    }
}
