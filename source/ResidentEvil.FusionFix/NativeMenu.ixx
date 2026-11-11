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

export namespace REMenu
{
    inline HMODULE module = nullptr;
    inline std::function<bool()> blocksInput;
    inline SafetyHookInline initHook, execHook, inputHook, destroyHook, releaseHook;
    inline SafetyHookMid firstCountHook, secondCountHook;
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;

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
            REText::Set(node, value);
        }

        void Fit(const std::string& text, float x, float y, int height, float width, uint32_t color)
        {
            const std::array<float, 4> style{ x, y, static_cast<float>(height), width };
            if (fitted && text == value && geometry == style && tint == color) return;
            REText::Place(node, x, y, height, color);
            REText::Set(node, text);
            const auto measured = Read<float>(node, 216);
            if (std::isfinite(measured) && measured > width)
            {
                const auto smaller = std::max(12, static_cast<int>(std::floor(height * width / measured)));
                REText::Place(node, x, y + (height - smaller) * 0.5f, smaller, color);
                REText::Set(node, text);
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
        std::array<void*, 9> actors{};
        std::array<uint32_t, 9> actorFlags{};
        std::array<float, 9> rowY{};
        std::array<float, 2> entryPosition{};
        void* guideRoot = nullptr;
        std::vector<std::pair<void*, bool>> guideVisibility;
        std::string entryHelp, language;
        std::array<Text, 8> labels, values;
        std::array<Text, 9> helpLines;
        Text entry, heading, footer, restart;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<std::string> help;
        size_t page = 0, rowStart = 0, selection = 0, helpStart = 0;
        uint32_t entryIndex = 0, entryActor = 0;
        bool open = false, inHelp = false, busy = false, hidden = false;
    };
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    inline std::weak_ptr<Host> noticeHost;

    inline bool Current(const std::shared_ptr<Host>& host)
    {
        return host && hosts.count(host->gui) && hosts.at(host->gui) == host &&
            Read<void*>(host->gui, 244) == host->root;
    }

    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto")
            return language;
        // The language chosen in the game's options (Japanese, English, French, Spanish, German, Italian, ...)
        const auto system = gameLanguage.get();
        switch (system ? Read<uint32_t>(system, 0x28) : 1)
        {
            case 2: return "fr";
            case 3: return "es";
            case 4: return "de";
            case 5: return "it";
            default: return "en";
        }
    }

    // The Plugins row and its help follow the game's language, also when it changes while the menu exists
    inline void EntryText(const std::shared_ptr<Host>& host)
    {
        auto language = Language();
        if (language == host->language) return;
        host->language = std::move(language);
        host->model.OpenPlugins(module, host->language, 4096);
        RememberRestartValues(host->model);
        host->entry.Set(host->model.Text("menu.plugins"));
        if (host->entry.node) Write(host->entry.node, 160, 640.0f - Read<float>(host->entry.node, 216) * 0.5f);
        host->entryHelp = host->model.Text("menu.pluginshelp");
        host->footer.fitted = false;
    }

    inline void Guide(const std::shared_ptr<Host>& host, bool hidden)
    {
        auto manager = sceneManager.get();
        auto guide = manager ? Read<void*>(manager, 208) : nullptr;
        auto root = guide ? Read<void*>(guide, 244) : nullptr;
        if (root != host->guideRoot)
        {
            host->guideVisibility.clear();
            host->guideRoot = root;
        }
        if (!root) return;
        if (hidden)
        {
            if (host->guideVisibility.empty())
                for (auto child = Read<void*>(root, 96); child; child = Read<void*>(child, 100))
                    host->guideVisibility.emplace_back(child, (Read<uint32_t>(child, 84) & 1) != 0);
            for (const auto& [node, visible] : host->guideVisibility) REText::Show(node, false);
        }
        else
        {
            for (const auto& [node, visible] : host->guideVisibility) REText::Show(node, visible);
            host->guideVisibility.clear();
        }
    }

    inline void ShowPage(const std::shared_ptr<Host>& host)
    {
        if (!Current(host)) return;
        // Closing the menu clears the update flag of every child; reopening restores it only on the native items
        const auto activate = [](const Text& text)
        {
            const auto flags = Read<uint32_t>(text.node, 84);
            if (!(flags & 8u)) Write(text.node, 84, flags | 8u);
        };
        for (const auto* text : { &host->entry, &host->heading, &host->footer, &host->restart }) activate(*text);
        for (const auto& text : host->labels) activate(text);
        for (const auto& text : host->values) activate(text);
        for (const auto& text : host->helpLines) activate(text);
        if (host->open && !host->hidden)
        {
            for (size_t i = 0; i < host->actors.size(); ++i)
                host->actorFlags[i] = Read<uint32_t>(host->actors[i], 84);
            host->hidden = true;
        }
        if (host->open)
        {
            for (auto actor : host->actors) REText::Show(actor, false);
        }
        else if (host->hidden)
        {
            for (size_t i = 0; i < host->actors.size(); ++i)
                REText::Show(host->actors[i], (host->actorFlags[i] & 1) != 0);
            host->hidden = false;
        }
        if (!host->open)
        {
            auto actor = host->actors[host->entryActor];
            // Visibility and mouse activity are separate native flags. Keep the hit region while hiding the spare label.
            Write(actor, 84, (Read<uint32_t>(actor, 84) | 8u) & ~1u);
            for (size_t i = 0; i <= host->entryIndex; ++i)
            {
                const auto logical = Read<uint32_t>(host->gui, 804 + i * 4);
                if (logical >= 9) continue;
                if (Read<float>(host->actors[logical], 164) != host->rowY[i])
                {
                    Write(host->actors[logical], 164, host->rowY[i]);
                    Write(host->actors[logical], 84, Read<uint32_t>(host->actors[logical], 84) | 0x10000);
                }
            }
        }
        if (!host->open && !host->busy) EntryText(host);
        const bool entrySelected = !host->open && Read<uint32_t>(host->gui, 844) == host->entryIndex;
        Guide(host, host->open || entrySelected);
        if (!host->open) host->footer.Fit(entrySelected ? host->entryHelp : "", 280, 550, 20, 760, 0xFFFFFFFF);
        REText::Show(host->entry.node, !host->open);
        REText::Show(host->heading.node, host->open);
        REText::Show(host->footer.node, host->open || entrySelected);
        REText::Show(host->restart.node, host->open && RestartRequired(host->model));
        for (size_t i = 0; i < 8; ++i)
        {
            const bool shown = host->open && !host->inHelp && i < host->items.size();
            REText::Show(host->labels[i].node, shown);
            REText::Show(host->values[i].node, shown);
        }
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            REText::Show(host->helpLines[i].node, host->open && host->inHelp && host->helpStart + i < host->help.size());
    }

    inline Text MakeText(const std::shared_ptr<Host>& host, float x, float y, int size, uint32_t priority)
    {
        Text text{ REText::Append(host->gui, host->root, priority), {} };
        REText::Place(text.node, x, y, size);
        REText::Show(text.node, false);
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
            if (host->rowStart >= pages.size()) host->rowStart = 0;
            for (size_t i = host->rowStart; i < pages.size() && i < host->rowStart + 5; ++i)
                items.push_back({ pages[i].title, "", {}, static_cast<int>(i + 1) });
            if (host->rowStart) items.push_back({ model.Text("menu.previous"), "", {}, -2 });
            if (host->rowStart + 5 < pages.size()) items.push_back({ model.Text("menu.next"), "", {}, -3 });
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
                auto enabled = model.Enabled(row, Read<uint32_t>(host->gui, 764) == 0);
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
            const auto color = i == host->selection ? 0xFF88D8FFu : item.enabled ? 0xFFFFFFFFu : 0xFF888888u;
            host->labels[i].Fit(item.label, 280, 250 + 30.0f * i, 22, 500, color);
            host->values[i].Fit(item.enabled ? item.value : model.Text("menu.unavailable"),
                820, 250 + 30.0f * i, 22, 220, color);
        }
        const auto& selected = host->items[host->selection];
        const auto icons = FixSettings().Int("MAIN.GamepadIcons");
        const auto helpKey = icons == 2 ? model.Text("menu.pad.triangle") : icons == 3 ? "X" : "Y";
        host->footer.Set(selected.link == -1 && !model.Help(selected.row).empty() ? "F1 / " + helpKey + ": " + model.Text("menu.help") : "");
        host->restart.Fit(RestartRequired(model) ? model.Text("menu.restartwarning") : "", 280, 590, 20, 760, 0xFFFFFFFF);
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

    inline void Change(const std::shared_ptr<Host>& host, int direction)
    {
        if (!Current(host) || host->selection >= host->items.size()) return;
        const auto item = host->items[host->selection];
        if (item.link == -1)
            host->model.Change(item.row, direction, Read<uint32_t>(host->gui, 764) == 0);
        else if (!direction)
        {
            if (item.link == -4) { Back(host); return; }
            if (item.link == -2) host->rowStart = host->rowStart >= 5 ? host->rowStart - 5 : 0;
            else if (item.link == -3) host->rowStart += 5;
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

    inline void Help(const std::shared_ptr<Host>& host, const std::string* message = nullptr, const std::string* title = nullptr)
    {
        if (!message && (host->selection >= host->items.size() || host->items[host->selection].link != -1)) return;
        auto content = message ? *message : host->model.Help(host->items[host->selection].row);
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
            REText::Set(host->helpLines[0].node, joined);
            if (!line.empty() && Read<float>(host->helpLines[0].node, 216) > 760)
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
        host->heading.Fit(title ? *title : host->items[host->selection].label, 280, 205, 24, 760, 0xFFFFFFFF);
        for (size_t i = 0; i < host->helpLines.size(); ++i)
        {
            // Measuring used this node directly, so invalidate its previous cached value.
            host->helpLines[i].value.clear();
            host->helpLines[i].Set(i < host->help.size() ? host->help[i] : "");
        }
        host->footer.Set(host->model.Text("menu.back"));
        ShowPage(host);
    }

    inline bool NoticeReady()
    {
        const auto host = noticeHost.lock();
        return Current(host) && !host->inHelp && !host->busy && (!blocksInput || !blocksInput());
    }

    inline void ShowNotice(const std::wstring& message)
    {
        const auto host = noticeHost.lock();
        if (!Current(host)) return;
        if (!host->open)
        {
            host->open = true;
            host->page = host->rowStart = host->selection = 0;
            Populate(host);
        }
        const auto content = asi_settings::text::Narrow(message);
        const auto title = host->model.Text("menu.plugins");
        Help(host, &content, &title);
    }

    inline bool PrepareHost(void* gui)
    {
        if (hosts.count(gui)) return Current(hosts.at(gui));
        auto root = Read<void*>(gui, 244);
        auto nodes = Read<void**>(gui, 248);
        auto manager = inputManager.get();
        if (!root || !nodes || !manager || !Read<void*>(manager, 52)) return false;
        auto host = std::make_shared<Host>();
        host->gui = gui;
        host->root = root;
        const bool extended = Read<uint32_t>(gui, 764) != 0;
        host->entryActor = extended ? 6 : 4;
        host->entryIndex = extended ? 8 : 6;
        for (size_t i = 0; i < 9; ++i)
        {
            if (Read<uint32_t>(gui, 768 + i * 4) != i + 2 || !nodes[i + 2]) return false;
            host->actors[i] = nodes[i + 2];
            host->actorFlags[i] = Read<uint32_t>(nodes[i + 2], 84);
        }
        auto previous = nodes[Read<uint32_t>(gui, 804 + (host->entryIndex - 1) * 4) + 2];
        auto first = nodes[Read<uint32_t>(gui, 804) + 2];
        const float top = Read<float>(first, 164);
        const float bottom = Read<float>(previous, 164);
        if (!std::isfinite(top) || !std::isfinite(bottom) || bottom <= top) return false;
        for (size_t i = 0; i <= host->entryIndex; ++i)
            host->rowY[i] = top + (bottom - top) * i / host->entryIndex;
        host->entryPosition = { Read<float>(previous, 160), bottom };
        auto actor = host->actors[host->entryActor];
        host->entry = MakeText(host, 0, bottom + 6, 24, 1000);
        host->heading = MakeText(host, 280, 205, 24, 1000);
        host->footer = MakeText(host, 280, 550, 20, 1000);
        host->restart = MakeText(host, 280, 590, 20, 1000);
        for (size_t i = 0; i < 8; ++i)
        {
            host->labels[i] = MakeText(host, 280, 250 + 30.0f * i, 22, 1000);
            host->values[i] = MakeText(host, 820, 250 + 30.0f * i, 22, 1000);
        }
        for (size_t i = 0; i < host->helpLines.size(); ++i)
            host->helpLines[i] = MakeText(host, 280, 250 + 26.0f * i, 20, 1000);
        if (!host->entry.node || !host->heading.node || !host->footer.node || !host->restart.node ||
            std::any_of(host->labels.begin(), host->labels.end(), [](const auto& t) { return !t.node; }) ||
            std::any_of(host->values.begin(), host->values.end(), [](const auto& t) { return !t.node; }) ||
            std::any_of(host->helpLines.begin(), host->helpLines.end(), [](const auto& t) { return !t.node; })) return false;
        Write(actor, 160, host->entryPosition[0]);
        Write(actor, 164, host->entryPosition[1]);
        Write(actor, 84, Read<uint32_t>(actor, 84) | 0x10009);
        Write(gui, 804 + host->entryIndex * 4, host->entryActor);
        EntryText(host);
        hosts.emplace(gui, host);
        ShowPage(host);
        return true;
    }

    inline int __fastcall Initialize(void* gui, void*)
    {
        if (hosts.count(gui) && Current(hosts.at(gui))) Guide(hosts.at(gui), false);
        hosts.erase(gui);
        return initHook.thiscall<int>(gui);
    }

    inline void* __fastcall Destroy(void* gui, void*, uint32_t flags)
    {
        if (hosts.count(gui) && Current(hosts.at(gui))) Guide(hosts.at(gui), false);
        hosts.erase(gui);
        return destroyHook.thiscall<void*>(gui, flags);
    }

    inline int __fastcall Release(void* gui, void*)
    {
        for (const auto& [owner, host] : hosts)
        {
            if (host->guideRoot != Read<void*>(gui, 244)) continue;
            host->guideRoot = nullptr;
            host->guideVisibility.clear();
        }
        if (hosts.count(gui) && Current(hosts.at(gui))) Guide(hosts.at(gui), false);
        hosts.erase(gui);
        return releaseHook.thiscall<int>(gui);
    }

    inline int __fastcall Exec(void* gui, void*)
    {
        if (hosts.count(gui) && !Current(hosts.at(gui))) hosts.erase(gui);
        const auto result = execHook.thiscall<int>(gui);
        if (Read<uint32_t>(gui, 840) == 3)
        {
            if (PrepareHost(gui))
            {
                const auto host = hosts.at(gui);
                noticeHost = host;
                RWGTA::UpdateCheck::Tick();
                noticeHost.reset();
                ShowPage(host);
            }
        }
        return result;
    }

    inline int __fastcall Input(void* gui, void*)
    {
        auto found = hosts.find(gui);
        if (found == hosts.end() || !Current(found->second)) return inputHook.thiscall<int>(gui);
        const auto host = found->second;
        if (host->busy || (blocksInput && blocksInput())) return 0;
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        if (foregroundProcess != GetCurrentProcessId()) return 0;
        auto manager = inputManager.get();
        auto controller = pad.get();
        if (!manager || !controller) return inputHook.thiscall<int>(gui);
        prepare(manager, 0);
        const auto buttons = pressed(manager, 0, false);
        const auto repeat = repeated(manager, 0);
        const bool click = !host->open && mouseSelection(gui);
        bool accept = (buttons & Read<uint32_t>(controller, 2924)) != 0 || click;
        auto keys = keyboard.get();
        bool escape = false;
        if (keys && Read<uint32_t>(gui, 764) == 0)
        {
            const auto key = Read<uint32_t>(keys, 2676);
            if (key < 256) escape = (Read<uint32_t>(keys, 2752 + (key >> 5) * 4) & (1u << (key & 31))) != 0;
        }
        const auto profile = Read<uint32_t>(controller, 3316);
        const auto profileOffset = 2976 + (profile < 5 ? profile * 68 : 0);
        const bool secondaryBack = !Read<uint8_t>(manager, 1057) &&
            (buttons & Read<uint32_t>(controller, profileOffset + 48)) != 0;
        const bool back = (buttons & Read<uint32_t>(controller, 2928)) != 0 || secondaryBack || escape;
        if (!host->open)
        {
            if (Read<uint32_t>(gui, 844) != host->entryIndex || !accept) return inputHook.thiscall<int>(gui);
            host->busy = true;
            host->language.clear();
            EntryText(host);
            RememberRestartValues(host->model);
            host->busy = false;
            if (!Current(host)) return 0;
            host->open = true;
            host->page = host->rowStart = host->selection = 0;
            Populate(host);
            return 0;
        }
        const bool help = (buttons & 0x1000u) != 0 ||
            (keys && (Read<uint32_t>(keys, 2752 + (VK_F1 >> 5) * 4) & (1u << (VK_F1 & 31))) != 0);
        POINT point{};
        bool mouseClick = false, mouseMoved = false;
        const bool haveMouse = MousePoint(gui, point, mouseClick, mouseMoved);
        const bool mouseBack = host->inHelp && haveMouse && mouseClick &&
            point.x >= 280 && point.x < 1040 && point.y >= 550 && point.y < 580;
        if (back || mouseBack || (host->inHelp && help)) { Back(host); return 0; }
        if (host->inHelp)
        {
            if ((repeat & 0x40040) && host->helpStart + host->helpLines.size() < host->help.size()) ++host->helpStart;
            if ((repeat & 0x10010) && host->helpStart) --host->helpStart;
            for (size_t i = 0; i < host->helpLines.size(); ++i)
                host->helpLines[i].Set(host->helpStart + i < host->help.size() ? host->help[host->helpStart + i] : "");
            ShowPage(host);
            return 0;
        }
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            const int top = 250 + 30 * static_cast<int>(i);
            const bool inside = haveMouse && point.x >= 280 && point.x < 1040 &&
                point.y >= top && point.y < top + 30;
            const bool clicked = inside && mouseClick;
            const bool moved = inside && mouseMoved;
            if (!clicked && !moved) continue;
            host->selection = i;
            accept |= clicked;
            break;
        }
        if (repeat & 0x40040) host->selection = (host->selection + 1) % host->items.size();
        if (repeat & 0x10010) host->selection = (host->selection + host->items.size() - 1) % host->items.size();
        if (repeat & 0x80080) Change(host, -1);
        if (repeat & 0x20020) Change(host, 1);
        if (accept) Change(host, 0);
        if (help) Help(host);
        if (!host->inHelp) Populate(host);
        return 0;
    }

    inline void Install(HMODULE ownModule)
    {
        module = ownModule;
        Bind();
        initHook = safetyhook::create_inline(initialize, Initialize);
        execHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[8]), Exec);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[0]), Destroy);
        releaseHook = safetyhook::create_inline(reinterpret_cast<void*>(menuVtable[36]), Release);
        inputHook = safetyhook::create_inline(processInput, Input);
        firstCountHook = safetyhook::create_mid(countInstruction + 7, [](SafetyHookContext& context)
        {
            auto gui = reinterpret_cast<void*>(originalCompiler ? context.esi : context.edi);
            if (!hosts.count(gui) || !Current(hosts.at(gui))) return;
            if (originalCompiler) ++context.ebx;
            else ++context.eax;
        });
        if (secondCountInstruction) // Current executable
            secondCountHook = safetyhook::create_mid(secondCountInstruction + 7, [](SafetyHookContext& context)
            {
                auto gui = reinterpret_cast<void*>(context.edi);
                if (hosts.count(gui) && Current(hosts.at(gui))) ++context.eax;
            });
    }
}
