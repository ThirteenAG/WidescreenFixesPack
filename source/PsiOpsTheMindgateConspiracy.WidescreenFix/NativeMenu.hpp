#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <memory>
#include <cstring>

namespace PsiOpsMenu
{
    using Node = uint32_t*;
    constexpr uint32_t ObjectTag = 0x4F424A20;
    constexpr uint32_t GroupTag = 0x47524F50;
    constexpr uint32_t TextTag = 0x54455854;
    struct GameString
    {
        std::array<uint32_t, 7> data{};
    };
    inline SafetyHookInline relocateHook, showHook, nativeShowHook, eventHook, drawTextHook, cleanupHook;
    inline const char*(__thiscall* rawString)(const GameString*){};
    inline void(__thiscall* destroyString)(GameString*){};
    inline void(__thiscall* createString)(GameString*, const char*){};
    inline void(__thiscall* setText)(void*, const GameString*, bool){};
    inline HMODULE host{};
    inline const char*(__cdecl* lookupString)(const char*){};
    inline void* find_pattern(const char* bytes)
    {
        auto pattern = hook::pattern(bytes);
        return pattern.size() == 1 ? pattern.get_first() : nullptr;
    }
    inline bool Readable(const void* pointer, size_t size)
    {
        MEMORY_BASIC_INFORMATION memory{};
        return pointer && VirtualQuery(pointer, &memory, sizeof(memory)) && memory.State == MEM_COMMIT
            && !(memory.Protect & (PAGE_NOACCESS | PAGE_GUARD))
            && reinterpret_cast<uintptr_t>(pointer) + size <= reinterpret_cast<uintptr_t>(memory.BaseAddress) + memory.RegionSize;
    }
    inline bool Group(Node node)
    {
        return Readable(node, 32) && (node[0] == GroupTag || node[0] == ObjectTag);
    }
    inline Node Pointer(uint32_t value)
    {
        return reinterpret_cast<Node>(value);
    }
    inline Node TextNode(Node node)
    {
        if (!Readable(node, 16))
            return nullptr;
        if (node[0] == TextTag)
            return node;
        if (!Group(node))
            return nullptr;
        auto list = Pointer(node[6]);
        if (!Readable(list, 4) || list[0] > 100 || !Readable(list, (list[0] + 1) * 4))
            return nullptr;
        for (uint32_t i = 0; i < list[0]; ++i)
            if (auto text = TextNode(Pointer(list[i + 1])))
                return text;
        return nullptr;
    }
    struct Link
    {
        uint32_t* field{};
        uint32_t original{};
        size_t row{};
    };
    struct Screen
    {
        Node resource{}, root{}, container{}, help{}, helpRoot{}, stockChildren{};
        uint32_t helpFlags{}, cancelTime{};
        std::vector<uint32_t> extra, children;
        std::vector<Node> rows, texts;
        std::vector<Link> links;
        std::vector<std::pair<uint32_t*, uint32_t>> navigation;
        std::array<std::vector<std::pair<Node, bool>>, 6> hidden;
        Node firstBoundary{}, lastBoundary{};
        std::array<std::string, 6> labels, drawn, stockLabels;
        std::string helpText, drawnHelp, stockHelp;
        std::string language{ "en" };
        RWGTAMenu::Model model;
        int page{ -1 }, context{};
        size_t start{}, selected{};
        bool open{}, cleaning{};
        size_t stockRows{ 5 };
        struct Item
        {
            enum class Kind
            {
                Plugin,
                Setting,
                Back,
                Previous,
                Next
            } kind{};
            size_t index{};
            RWGTAMenu::Row row{};
        };
        std::vector<Item> items;
    };
    inline std::map<Node, std::unique_ptr<Screen>> screens;
    struct JoinedScreen
    {
        Node resource{}, root{}, container{}, stockChildren{}, help{};
        std::array<std::vector<uint32_t>, 2> copies;
        std::array<Node, 2> rows{}, texts{}, values{};
        std::array<std::vector<uint32_t>, 2> valueCopies, valueChildren;
        std::array<std::string, 2> drawn, drawnValues;
        std::vector<uint32_t> children, helpCopy;
        RWGTAMenu::Model model;
        std::vector<RWGTAMenu::Row> settings;
        std::string helpText, drawnHelp;
        std::string language{ "en" };
        bool cleaning{};
    };
    inline std::map<Node, std::unique_ptr<JoinedScreen>> joinedScreens;
    inline JoinedScreen* joinedSender{};
    inline std::set<std::string> JoinedIds()
    {
        std::set<std::string> ids;
        for (auto& [resource, screen] : joinedScreens)
            for (auto& row : screen->settings)
                if (row.setting) ids.insert(row.setting->id);
        return ids;
    }
    inline constexpr const char* JoinedActions[]{ "WFP/Joined0", "WFP/Joined1" };
    inline Screen* current{};
    inline Screen* senderScreen{};
    inline constexpr const char* Actions[]{ "WFP/Row0", "WFP/Row1", "WFP/Row2", "WFP/Row3", "WFP/Row4", "WFP/Row5" };
    inline bool NeedsRestart()
    {
        for (auto id : { "MAIN.FixHUD", "MAIN.SkipIntro" })
        {
            auto wanted = FixSettings().Wanted(id);
            bool enabled = wanted == "1" || asi_settings::text::IEquals(wanted, "true");
            if (!wanted.empty() && enabled != FixSettings().Bool(id))
                return true;
        }
        return false;
    }
    inline std::string ShortHelp(std::string text)
    {
        if (text.size() > 40)
        {
            auto end = text.rfind(' ', 37);
            if (end == std::string::npos)
                end = 37;
            while (end && (static_cast<unsigned char>(text[end]) & 0xC0) == 0x80)
                --end;
            text.resize(end);
            text += "...";
        }
        return text;
    }
    inline std::string Help(const RWGTAMenu::Model& model, const RWGTAMenu::Row& row)
    {
        std::string text;
        if (row.plugin && row.plugin->module == host && row.setting && row.setting->id.starts_with("MAIN."))
            text = OtherPlugins::Text(*row.plugin, "@menu." + row.setting->id.substr(5) + ".help");
        else
            text = model.Help(row);
        return ShortHelp(std::move(text));
    }
    inline std::string Text(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), current ? current->language : "en", key);
    }
    inline std::string Convert(const std::string& text, UINT from, UINT to)
    {
        auto count = MultiByteToWideChar(from, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (count <= 0)
            return {};
        std::wstring wide(count, L'\0');
        MultiByteToWideChar(from, 0, text.data(), static_cast<int>(text.size()), wide.data(), count);
        count = WideCharToMultiByte(to, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
        std::string result(count, '\0');
        WideCharToMultiByte(to, 0, wide.data(), static_cast<int>(wide.size()), result.data(), count, nullptr, nullptr);
        return result;
    }
    inline std::string Language()
    {
        auto graphics = lookupString ? lookupString("Graphics") : nullptr;
        if (!graphics)
            return "en";
        auto label = Convert(graphics, 1252, CP_UTF8);
        if (label.starts_with("Graphismes")) return "fr";
        if (label.starts_with("Grafik")) return "de";
        if (label.starts_with("Grafica")) return "it";
        if (label.find("ficos") != std::string::npos) return "es";
        return "en";
    }
    inline std::string TitleCase(std::string text)
    {
        for (size_t start = 0; start < text.size();)
        {
            auto end = text.find(' ', start);
            if (end == std::string::npos) end = text.size();
            auto word = text.substr(start, end - start);
            bool minor = start && (word == "of" || word == "the" || word == "and" || word == "to");
            if (!minor && start < end && text[start] >= 'a' && text[start] <= 'z')
                text[start] -= 'a' - 'A';
            start = end + 1;
        }
        return text;
    }
    inline void SetFontText(Node definition, const std::string& value, std::string& previous, bool titleCase = false)
    {
        if (!definition || !definition[2] || previous == value)
            return;
        auto font = reinterpret_cast<uint8_t*>(definition[2]);
        if (!Readable(font, 136))
            return;
        auto text = Convert(titleCase ? TitleCase(value) : value, CP_UTF8, 1252);
        GameString string{};
        createString(&string, text.c_str());
        setText(font, &string, true);
        destroyString(&string);
        previous = value;
    }
    inline uint32_t* Navigation(Node node, int message)
    {
        if (!Group(node))
            return nullptr;
        auto list = Pointer(node[4]);
        if (!Readable(list, 4) || list[0] > 100 || !Readable(list, (list[0] + 1) * 4))
            return nullptr;
        for (uint32_t i = 0; i < list[0]; ++i)
        {
            auto event = Pointer(list[i + 1]);
            if (!Readable(event, 12) || event[0] != message || event[1] > 100 || !Readable(event, 12 + event[1] * 16))
                continue;
            for (uint32_t j = 0; j < event[1]; ++j)
            {
                auto command = event + 3 + j * 4;
                if (command[1] != 1004)
                    continue;
                auto parameters = Pointer(command[3]);
                if (!Readable(parameters, 8) || !parameters[0])
                    continue;
                auto target = Pointer(parameters[1]);
                if (Readable(target, 8) && target[0] == 5)
                    return target + 1;
            }
        }
        return nullptr;
    }
    inline void SetNavigation(Node node, int message, Node target)
    {
        if (auto field = Navigation(node, message))
            *field = reinterpret_cast<uint32_t>(target);
    }
    inline void HideLeaves(Node node, std::vector<std::pair<Node, bool>>& hidden)
    {
        if (!Readable(node, 16))
            return;
        if (Group(node))
        {
            auto list = Pointer(node[6]);
            if (Readable(list, 4) && list[0] <= 100 && Readable(list, (list[0] + 1) * 4))
                for (uint32_t i = 0; i < list[0]; ++i)
                    HideLeaves(Pointer(list[i + 1]), hidden);
        }
        else if (node[2] && (node[0] == TextTag || node[0] == 0x504F4C59))
        {
            auto object = reinterpret_cast<void*>(node[2]);
            auto table = *reinterpret_cast<uintptr_t**>(object);
            bool visible = reinterpret_cast<bool(__thiscall*)(void*)>(table[6])(object);
            hidden.emplace_back(node, visible);
            reinterpret_cast<void(__thiscall*)(void*, bool)>(table[5])(object, false);
        }
    }
    inline void ShowLeaves(std::vector<std::pair<Node, bool>>& hidden)
    {
        for (auto& [node, visible] : hidden)
            if (node[2])
            {
                auto object = reinterpret_cast<void*>(node[2]);
                auto table = *reinterpret_cast<uintptr_t**>(object);
                reinterpret_cast<void(__thiscall*)(void*, bool)>(table[5])(object, visible);
            }
        hidden.clear();
    }
    inline void Focus(Screen& screen, size_t index)
    {
        if (index < screen.rows.size() && screen.rows[index][2] && screen.context)
            eventHook.thiscall<int>(reinterpret_cast<void*>(screen.rows[index][2]), screen.context, 4000, 0);
    }
    inline void Restore(Screen& screen)
    {
        for (auto& link : screen.links)
            *link.field = link.row == screen.stockRows ? reinterpret_cast<uint32_t>("WFP/Plugins") : link.original;
        for (auto& [field, value] : screen.navigation)
            *field = value;
        for (auto& hidden : screen.hidden)
            ShowLeaves(hidden);
        screen.open = false;
        if (screen.helpRoot)
            screen.helpRoot[3] = (screen.helpRoot[3] & ~1u) | (screen.helpFlags & 1);
        screen.model.Close();
        for (size_t i = 0; i < screen.texts.size(); ++i)
        {
            screen.labels[i] = i == screen.stockRows ? Text("menu.plugins") : screen.stockLabels[i];
            screen.drawn[i].clear();
            SetFontText(screen.texts[i], screen.labels[i], screen.drawn[i], screen.language == "en");
        }
        screen.helpText.clear();
        SetFontText(screen.help, screen.stockHelp, screen.drawnHelp);
        if (current == &screen)
            current = nullptr;
        if (!screen.cleaning)
            Focus(screen, screen.stockRows);
    }
    inline void Build(Screen& screen)
    {
        screen.items.clear();
        auto pageLimit = screen.rows.size() - 3;
        for (auto& label : screen.labels)
            label.clear();
        if (screen.page < 0)
        {
            auto& pages = screen.model.Pages();
            for (size_t i = screen.start; i < pages.size() && i < screen.start + pageLimit; ++i)
            {
                screen.labels[screen.items.size()] = pages[i].plugin && pages[i].plugin->module == host ? "Psi-Ops" : pages[i].title;
                screen.items.push_back({ Screen::Item::Kind::Plugin, i });
            }
            if (screen.start)
            {
                screen.labels[screen.items.size()] = Text("menu.previous");
                screen.items.push_back({ Screen::Item::Kind::Previous });
            }
            if (screen.start + pageLimit < pages.size())
            {
                screen.labels[screen.items.size()] = Text("menu.next");
                screen.items.push_back({ Screen::Item::Kind::Next });
            }
        }
        else
        {
            for (auto row : screen.model.VisibleRows(screen.page))
            {
                auto label = screen.model.Label(*row);
                auto value = screen.model.Value(*row);
                if (!value.empty())
                    label += ": " + value;
                screen.labels[screen.items.size()] = std::move(label);
                screen.items.push_back({ Screen::Item::Kind::Setting, 0, *row });
            }
        }
        auto visibleRows = screen.items.size();
        screen.items.push_back({ Screen::Item::Kind::Back });
        for (size_t i = 0; i < screen.rows.size(); ++i)
        {
            if (i < visibleRows)
            {
                ShowLeaves(screen.hidden[i]);
                SetNavigation(screen.rows[i], 1006, screen.rows[i ? i - 1 : visibleRows - 1]);
                SetNavigation(screen.rows[i], 1007, screen.rows[i + 1 < visibleRows ? i + 1 : 0]);
            }
            else if (screen.hidden[i].empty())
                HideLeaves(screen.rows[i], screen.hidden[i]);
        }
        screen.helpText = (screen.model.RestartRequired() || NeedsRestart()) ? Text("menu.restartwarning") : Text("menu.plugins");
        if (screen.selected < screen.items.size() && screen.items[screen.selected].kind == Screen::Item::Kind::Setting)
            screen.helpText = Help(screen.model, screen.items[screen.selected].row);
        screen.helpText = ShortHelp(std::move(screen.helpText));
    }
    inline void Open(Screen& screen)
    {
        for (size_t i = 0; i < screen.texts.size(); ++i)
        {
            if (screen.texts[i][2])
                screen.stockLabels[i] = Convert(rawString(reinterpret_cast<GameString*>(screen.texts[i][2] + 28)), 1252, CP_UTF8);
        }
        screen.stockHelp.clear();
        if (screen.helpRoot)
        {
            screen.helpFlags = screen.helpRoot[3];
            screen.helpRoot[3] = screen.helpFlags | 1;
        }
        if (screen.help && screen.help[2])
            screen.stockHelp = Convert(rawString(reinterpret_cast<GameString*>(screen.help[2] + 28)), 1252, CP_UTF8);
        screen.language = Language();
        auto graphics = screen.stockLabels[2];
        if (graphics.starts_with("Graphismes"))
            screen.language = "fr";
        else if (graphics.starts_with("Grafik"))
            screen.language = "de";
        else if (graphics.starts_with("Grafica"))
            screen.language = "it";
        else if (graphics.find("ficos") != std::string::npos)
            screen.language = "es";
        current = &screen;
        screen.model.OpenPlugins(host, screen.language, screen.rows.size() - 1, JoinedIds());
        screen.page = -1;
        screen.start = screen.selected = 0;
        screen.open = true;
        current = &screen;
        for (auto& link : screen.links)
            *link.field = reinterpret_cast<uint32_t>(Actions[link.row]);
        Build(screen);
        Focus(screen, 0);
    }
    inline void Act(Screen& screen, size_t index, int direction)
    {
        if (index >= screen.items.size())
            return;
        screen.selected = index;
        auto item = screen.items[index];
        switch (item.kind)
        {
            case Screen::Item::Kind::Plugin:
                screen.page = static_cast<int>(item.index);
                screen.model.PageShown(item.index);
                screen.selected = 0;
                break;
            case Screen::Item::Kind::Setting:
                screen.model.Change(item.row, direction, true);
                break;
            case Screen::Item::Kind::Back:
                if (screen.page < 0)
                {
                    Restore(screen);
                    return;
                }
                screen.page = -1;
                screen.selected = 0;
                break;
            case Screen::Item::Kind::Previous:
                screen.start -= std::min(screen.rows.size() - 3, screen.start);
                break;
            case Screen::Item::Kind::Next:
                screen.start += screen.rows.size() - 3;
                break;
        }
        Build(screen);
        if (index != screen.selected)
            Focus(screen, screen.selected);
    }
    inline void Extend(Node resource)
    {
        if (screens.count(resource))
            return;
        auto strings = Pointer(resource[5]);
        if (!Readable(strings, 4) || strings[0] > 4096 || !Readable(strings, (strings[0] + 1) * 4))
            return;
        std::array<const char*, 5> paths{ "Shell/Keyboard", "Shell/GamePlay", "Shell/Graphics", "Shell/AudioOptions", "Shell/Credits" };
        std::array<uint32_t, 5> targets{};
        for (uint32_t i = 0; i < strings[0]; ++i)
        {
            auto text = reinterpret_cast<const char*>(strings[i + 1]);
            if (!Readable(text, 32))
                continue;
            for (size_t j = 0; j < paths.size(); ++j)
                if (std::strcmp(text, paths[j]) == 0)
                    targets[j] = strings[i + 1];
        }
        size_t stockRows = 5;
        if (std::ranges::any_of(targets, [](auto value) { return !value; }))
        {
            stockRows = 4;
            paths = { "HUD/UI/Options/InGameKeyboard", "HUD/UI/Options/InGameGamePlay", "HUD/UI/Options/InGameGraphic", "HUD/UI/Options/InGameAudioOptions", "" };
            targets.fill(0);
            for (uint32_t i = 0; i < strings[0]; ++i)
            {
                auto text = reinterpret_cast<const char*>(strings[i + 1]);
                for (size_t j = 0; j < stockRows; ++j)
                    if (Readable(text, std::strlen(paths[j]) + 1) && std::strcmp(text, paths[j]) == 0)
                        targets[j] = strings[i + 1];
            }
            for (size_t i = 0; i < stockRows; ++i)
                if (!targets[i])
                    return;
        }
        auto root = Pointer(resource[3]);
        if (!Group(root))
            return;
        auto top = Pointer(root[6]);
        if (!Readable(top, 8) || top[0] != 1)
            return;
        auto container = Pointer(top[1]);
        if (!Group(container))
            return;
        auto children = Pointer(container[6]);
        if (!Readable(children, 4) || children[0] != stockRows + 7 || !Readable(children, (children[0] + 1) * 4))
            return;
        auto screen = std::make_unique<Screen>();
        screen->stockRows = stockRows;
        screen->resource = resource;
        screen->root = root;
        screen->container = container;
        screen->stockChildren = children;
        screen->language = Language();
        for (size_t i = 0; i < stockRows; ++i)
        {
            auto row = Pointer(children[i + 3]);
            auto text = TextNode(row);
            if (!Group(row) || !text)
                return;
            screen->rows.push_back(row);
            screen->texts.push_back(text);
        }
        auto begin = reinterpret_cast<uintptr_t>(screen->rows.back());
        auto end = static_cast<uintptr_t>(children[stockRows + 3]);
        if (end <= begin || end - begin > 16384 || !Readable(reinterpret_cast<void*>(begin), end - begin))
            return;
        screen->extra.resize((end - begin) / 4);
        std::memcpy(screen->extra.data(), reinterpret_cast<void*>(begin), end - begin);
        auto delta = reinterpret_cast<uintptr_t>(screen->extra.data()) - begin;
        for (auto& value : screen->extra)
            if (value >= begin && value < end)
                value += static_cast<uint32_t>(delta);
        auto row = screen->extra.data();
        auto label = TextNode(row);
        if (!label)
            return;
        auto events = Pointer(row[4]);
        if (Readable(events, 4) && events[0] <= 100 && Readable(events, (events[0] + 1) * 4))
            for (uint32_t i = 0; i < events[0]; ++i)
            {
                auto event = Pointer(events[i + 1]);
                if (Readable(event, 12) && event[0] == 1010 && event[1] > 2)
                    event[1] = event[2] = 2;
            }
        screen->rows.push_back(row);
        screen->texts.push_back(label);
        auto first = Navigation(screen->rows.front(), 1006);
        auto last = Navigation(screen->rows[stockRows - 1], 1007);
        if (!first || !last)
            return;
        screen->firstBoundary = Pointer(*first);
        screen->lastBoundary = Pointer(*last);
        SetNavigation(screen->rows[stockRows - 1], 1007, row);
        SetNavigation(row, 1006, screen->rows[stockRows - 1]);
        for (auto item : screen->rows)
            for (auto message : { 1006, 1007 })
                if (auto field = Navigation(item, message))
                    screen->navigation.emplace_back(field, *field);
        // Keep six rows in the stock list's area.
        for (size_t i = 0; i < screen->rows.size(); ++i)
        {
            auto transform = Pointer(screen->rows[i][5]);
            if (Readable(transform, 80))
                *reinterpret_cast<float*>(transform + 7) = 160.0f + 29.0f * static_cast<float>(i);
        }
        screen->children.push_back(children[0] + 1);
        for (uint32_t i = 0; i < children[0]; ++i)
        {
            if (i == stockRows + 2)
                screen->children.push_back(reinterpret_cast<uint32_t>(row));
            screen->children.push_back(children[i + 1]);
        }
        for (size_t i = 0; i < screen->rows.size(); ++i)
        {
            auto first = reinterpret_cast<uintptr_t>(screen->rows[i]);
            auto limit = i == stockRows ? first + screen->extra.size() * 4 : static_cast<uintptr_t>(children[i + 4]);
            for (auto field = reinterpret_cast<uint32_t*>(first); reinterpret_cast<uintptr_t>(field) < limit; ++field)
                if (*field == targets[std::min(i, stockRows - 1)])
                    screen->links.push_back({ field, *field, i });
        }
        for (auto& link : screen->links)
            if (link.row == stockRows)
                *link.field = reinterpret_cast<uint32_t>("WFP/Plugins");
        auto helper = Pointer(children[stockRows + 4]);
        screen->help = TextNode(helper);
        screen->helpRoot = Group(helper) ? helper : nullptr;
        screen->labels[stockRows] = RWGTAMenu::PluginText(FixSettings(), screen->language, "menu.plugins");
        container[6] = reinterpret_cast<uint32_t>(screen->children.data());
        screens.emplace(resource, std::move(screen));
    }
    inline void ClearObjects(Node node)
    {
        if (!Readable(node, 16))
            return;
        node[2] = 0;
        if (!Group(node))
            return;
        auto list = Pointer(node[6]);
        if (!Readable(list, 4) || list[0] > 100 || !Readable(list, (list[0] + 1) * 4))
            return;
        for (uint32_t i = 0; i < list[0]; ++i)
            ClearObjects(Pointer(list[i + 1]));
    }
    inline void ExtendGraphics(Node resource)
    {
        if (joinedScreens.count(resource) || screens.empty())
            return;
        auto strings = Pointer(resource[5]);
        if (!Readable(strings, 4) || strings[0] > 4096 || !Readable(strings, (strings[0] + 1) * 4))
            return;
        if (strings[0] < 2 || screens.count(resource))
            return;
        auto name = reinterpret_cast<const char*>(strings[2]);
        if (!Readable(name, 32))
            return;
        bool pause = std::strcmp(name, "HUD/UI/Options/InGameGraphic") == 0
            || std::strcmp(name, "HUD/UI/Options/InGameGamePlay") == 0;
        bool hud = std::strcmp(name, "Shell/GamePlay") == 0 || std::strcmp(name, "HUD/UI/Options/InGameGamePlay") == 0;
        if (!pause && !hud && std::strcmp(name, "Shell/Graphics") != 0)
            return;
        Screen* source{};
        for (auto& [key, screen] : screens)
            if ((screen->stockRows == 4) == pause)
                source = screen.get();
        if (!source)
            return;
        auto root = Pointer(resource[3]);
        if (!Group(root))
            return;
        auto top = Pointer(root[6]);
        if (!Readable(top, 8) || top[0] != 1)
            return;
        auto container = Pointer(top[1]);
        if (!Group(container))
            return;
        auto children = Pointer(container[6]);
        if (!Readable(children, 4) || children[0] > 100 || !Readable(children, (children[0] + 1) * 4))
            return;
        auto screen = std::make_unique<JoinedScreen>();
        screen->resource = resource;
        screen->root = root;
        screen->container = container;
        screen->stockChildren = children;
        screen->language = Language();
        screen->model.OpenPlugins(host, screen->language, 3, { hud ? "MAIN.FixHUD" : "MAIN.FOVFactor" });
        screen->settings = screen->model.JoinedRows();
        if (screen->settings.size() != 1)
            return;
        screen->children.assign(children, children + children[0] + 1);
        screen->children[0] += static_cast<uint32_t>(screen->settings.size() + 1);
        for (size_t i = 0; i < screen->settings.size(); ++i)
        {
            auto& copy = screen->copies[i];
            copy = source->extra;
            auto begin = reinterpret_cast<uintptr_t>(source->extra.data());
            auto end = begin + source->extra.size() * 4;
            auto delta = reinterpret_cast<uintptr_t>(copy.data()) - begin;
            for (auto& value : copy)
            {
                if (value >= begin && value < end)
                    value += static_cast<uint32_t>(delta);
                else if (value == reinterpret_cast<uint32_t>("WFP/Plugins")
                    || value == reinterpret_cast<uint32_t>(Actions[source->stockRows]))
                    value = reinterpret_cast<uint32_t>(JoinedActions[i]);
            }
            for (auto& link : source->links)
                if (link.row == source->stockRows)
                {
                    auto offset = reinterpret_cast<uintptr_t>(link.field) - begin;
                    if (offset < copy.size() * 4)
                        copy[offset / 4] = reinterpret_cast<uint32_t>(JoinedActions[i]);
                }
            auto row = copy.data();
            ClearObjects(row);
            auto transform = Pointer(row[5]);
            if (!Readable(transform, 80))
                return;
            reinterpret_cast<float*>(transform)[6] = hud ? -125.0f : -131.0f;
            reinterpret_cast<float*>(transform)[7] = hud ? 191.0f : 305.0f;
            screen->rows[i] = row;
            screen->texts[i] = TextNode(row);
            auto rowChildren = Pointer(row[6]);
            if (!Readable(rowChildren, 8) || rowChildren[0] != 1)
                return;
            auto group = Pointer(rowChildren[1]);
            if (!Group(group) || !screen->texts[i])
                return;
            auto leaves = Pointer(group[6]);
            if (!Readable(leaves, 16) || leaves[0] != 3)
                return;
            screen->valueCopies[i].assign(screen->texts[i], screen->texts[i] + 13);
            auto value = screen->valueCopies[i].data();
            value[1] = value[2] = 0;
            value[3] = 1;
            reinterpret_cast<float*>(value)[6] = hud ? 366.0f : 376.0f;
            reinterpret_cast<float*>(value)[9] = 120.0f;
            screen->values[i] = value;
            screen->valueChildren[i].assign(leaves, leaves + 4);
            screen->valueChildren[i][0] = 4;
            screen->valueChildren[i].push_back(reinterpret_cast<uint32_t>(value));
            group[6] = reinterpret_cast<uint32_t>(screen->valueChildren[i].data());
            screen->children.push_back(reinterpret_cast<uint32_t>(row));
        }
        if (!source->help || !Readable(source->help, 52))
            return;
        screen->helpCopy.assign(source->help, source->help + 13);
        screen->help = screen->helpCopy.data();
        screen->help[1] = screen->help[2] = 0;
        screen->help[3] = 1;
        reinterpret_cast<float*>(screen->help)[6] = 0.0f;
        reinterpret_cast<float*>(screen->help)[7] = 421.0f;
        screen->children.push_back(reinterpret_cast<uint32_t>(screen->help));
        constexpr uint32_t previousIndex{ 9 };
        constexpr uint32_t nextIndex{ 11 };
        if (children[0] > nextIndex && Group(Pointer(children[previousIndex + 1])) && Group(Pointer(children[nextIndex + 1])))
        {
            auto previous = Pointer(children[previousIndex + 1]);
            auto next = Pointer(children[nextIndex + 1]);
            SetNavigation(screen->rows[0], 1006, previous);
            SetNavigation(screen->rows[0], 1007, next);
            SetNavigation(previous, 1007, screen->rows[0]);
            SetNavigation(next, 1006, screen->rows[0]);
        }
        if (hud && children[0] == 22)
            for (uint32_t i = 11; i <= 18; ++i)
            {
                auto node = Pointer(children[i + 1]);
                if (Group(node) && Readable(Pointer(node[5]), 80))
                {
                    auto y = reinterpret_cast<float*>(node[5]) + 7;
                    *y += 25.0f;
                }
            }
        container[6] = reinterpret_cast<uint32_t>(screen->children.data());
        joinedScreens.emplace(resource, std::move(screen));
    }
    inline bool Valid(Node resource, Node root, Node container, const std::vector<uint32_t>& children)
    {
        if (!Readable(resource, 24) || resource[0] != 0x53584258
            || resource[3] != reinterpret_cast<uint32_t>(root) || !Group(root) || !Group(container))
            return false;
        auto list = Pointer(container[6]);
        if (!Readable(list, 4) || list[0] > 256 || !Readable(list, (list[0] + 1) * 4))
            return false;
        return std::all_of(children.begin() + 1, children.end(), [list](auto child)
        {
            return std::find(list + 1, list + list[0] + 1, child) != list + list[0] + 1;
        });
    }
    inline void Prune()
    {
        for (auto it = screens.begin(); it != screens.end();)
        {
            auto& screen = *it->second;
            if (Valid(screen.resource, screen.root, screen.container, screen.children))
            {
                ++it;
                continue;
            }
            if (current == &screen) current = nullptr;
            if (senderScreen == &screen) senderScreen = nullptr;
            it = screens.erase(it);
        }
        for (auto it = joinedScreens.begin(); it != joinedScreens.end();)
        {
            auto& screen = *it->second;
            if (Valid(screen.resource, screen.root, screen.container, screen.children))
            {
                ++it;
                continue;
            }
            if (joinedSender == &screen) joinedSender = nullptr;
            it = joinedScreens.erase(it);
        }
    }
    inline int __cdecl Cleanup(void* object)
    {
        Prune();
        for (auto& [key, screen] : screens)
            if (screen->root[2] == reinterpret_cast<uint32_t>(object)
                || screen->container[2] == reinterpret_cast<uint32_t>(object))
            {
                screen->cleaning = true;
                if (screen->open) Restore(*screen);
                screen->cleaning = false;
                if (senderScreen == screen.get()) senderScreen = nullptr;
                for (auto& drawn : screen->drawn) drawn.clear();
                screen->drawnHelp.clear();
            }
        for (auto& [key, screen] : joinedScreens)
            if (screen->root[2] == reinterpret_cast<uint32_t>(object)
                || screen->container[2] == reinterpret_cast<uint32_t>(object))
            {
                if (joinedSender == screen.get()) joinedSender = nullptr;
                for (auto& drawn : screen->drawn) drawn.clear();
                for (auto& drawn : screen->drawnValues) drawn.clear();
                screen->drawnHelp.clear();
            }
        return cleanupHook.ccall<int>(object);
    }
    inline uint32_t __cdecl Relocate(int context, Node resource)
    {
        auto result = relocateHook.ccall<uint32_t>(context, resource);
        Prune();
        if (Readable(resource, 24) && resource[0] == 0x53584258)
        {
            Extend(resource);
            ExtendGraphics(resource);
        }
        return result;
    }
    inline bool HandlePath(const char* name)
    {
        if (name && std::strcmp(name, "WFP/Plugins") == 0)
        {
            if (senderScreen && senderScreen->root[2])
                Open(*senderScreen);
            return true;
        }
        if (name && std::strncmp(name, "WFP/Row", 7) == 0 && name[7] >= '0' && name[7] <= '5' && current)
        {
            Act(*current, name[7] - '0', 0);
            return true;
        }
        if (name && std::strncmp(name, "WFP/Joined", 10) == 0 && name[10] >= '0' && name[10] <= '1' && joinedSender)
        {
            auto index = static_cast<size_t>(name[10] - '0');
            if (index < joinedSender->settings.size())
                joinedSender->model.Change(joinedSender->settings[index], 0, true);
            return true;
        }
        return name && std::strncmp(name, "WFP/", 4) == 0;
    }
    inline int __fastcall NativeShow(void* self, void*, const char* name, int activate)
    {
        if (HandlePath(name))
            return 1;
        if (current)
            Restore(*current);
        return nativeShowHook.thiscall<int>(self, name, activate);
    }
    inline int __fastcall Show(void* self, void*, GameString path)
    {
        if (HandlePath(rawString(&path)))
        {
            destroyString(&path);
            return 1;
        }
        if (current)
            Restore(*current);
        return showHook.thiscall<int>(self, path);
    }
    inline int __fastcall Event(void* self, void*, int sender, int message, int parameter)
    {
        Prune();
        for (auto& [resource, screen] : screens)
        {
            bool repeat = message == 1011 && GetTickCount() - screen->cancelTime < 250;
            if (repeat && screen->root[2] == reinterpret_cast<uint32_t>(self))
                return 0;
            for (auto row : screen->rows)
                if (row[2] == reinterpret_cast<uint32_t>(self))
                {
                    if (repeat)
                        return 0;
                    if (sender)
                        screen->context = sender;
                    if (message == 1010 || message == 4002)
                        senderScreen = screen.get();
                }
        }
        for (auto& [resource, screen] : joinedScreens)
        {
            for (size_t i = 0; i < screen->settings.size(); ++i)
            {
                if (screen->rows[i][2] != reinterpret_cast<uint32_t>(self))
                    continue;
                if (message == 1011)
                    return 0;
                if (message == 4000 || message == 4001 || message == 4002 || message == 1010 || message == 1008 || message == 1009)
                {
                    joinedSender = screen.get();
                    screen->helpText = Help(screen->model, screen->settings[i]);
                }
                if (message == 1008 || message == 1009)
                {
                    screen->model.Change(screen->settings[i], message == 1008 ? -1 : 1, true);
                    return 0;
                }
            }
        }
        if (current && current->open)
        {
            if (current->root[2] == reinterpret_cast<uint32_t>(self) && message == 1011 && !current->items.empty())
            {
                current->cancelTime = GetTickCount();
                Act(*current, current->items.size() - 1, 0);
                return 0;
            }
            for (size_t i = 0; i < current->rows.size(); ++i)
            {
                if (current->rows[i][2] != reinterpret_cast<uint32_t>(self))
                    continue;
                if (message == 1011 || i + 1 >= current->items.size())
                    return 0;
                if (message == 4000 || message == 4001 || message == 4002 || message == 1010 || message == 1008 || message == 1009)
                    current->selected = i;
                if ((message == 1008 || message == 1009) && i < current->items.size()
                    && current->items[i].kind == Screen::Item::Kind::Setting)
                {
                    Act(*current, i, message == 1008 ? -1 : 1);
                    return 0;
                }
                Build(*current);
                break;
            }
        }
        return eventHook.thiscall<int>(self, sender, message, parameter);
    }
    inline int __fastcall DrawText(void* self, void*, int transform)
    {
        Prune();
        for (auto& [resource, screen] : screens)
        {
            if (screen->open && screen->helpRoot && !(screen->helpRoot[3] & 1))
                screen->helpRoot[3] = screen->helpRoot[3] | 1;
            for (size_t i = 0; i < screen->texts.size(); ++i)
                if (screen->texts[i][2] == reinterpret_cast<uint32_t>(self) && (screen->open || i == screen->stockRows))
                {
                    if (screen->open && RWGTAMenu::Model::Changed().exchange(false))
                        Build(*screen);
                    SetFontText(screen->texts[i], screen->labels[i], screen->drawn[i], screen->language == "en");
                }
            if (screen->open && screen->help && screen->help[2] == reinterpret_cast<uint32_t>(self))
                SetFontText(screen->help, screen->helpText, screen->drawnHelp);
        }
        for (auto& [resource, screen] : joinedScreens)
        {
            for (size_t i = 0; i < screen->settings.size(); ++i)
                if (screen->texts[i] && screen->texts[i][2] == reinterpret_cast<uint32_t>(self))
                {
                    auto& row = screen->settings[i];
                    auto label = screen->model.Label(row);
                    SetFontText(screen->texts[i], label, screen->drawn[i], screen->language == "en");
                }
            for (size_t i = 0; i < screen->settings.size(); ++i)
                if (screen->values[i] && screen->values[i][2] == reinterpret_cast<uint32_t>(self))
                    SetFontText(screen->values[i], screen->model.Value(screen->settings[i]), screen->drawnValues[i]);
            if (screen->help && screen->help[2] == reinterpret_cast<uint32_t>(self))
                SetFontText(screen->help, ShortHelp((screen->model.RestartRequired() || NeedsRestart()) ? RWGTAMenu::PluginText(FixSettings(), screen->language, "menu.restartwarning") : screen->helpText), screen->drawnHelp);
        }
        return drawTextHook.thiscall<int>(self, transform);
    }
    inline void Init()
    {
        lookupString = reinterpret_cast<decltype(lookupString)>(find_pattern(
            "55 8B EC 83 EC 10 C7 45 FC 00 00 00 00 8D 45 F8 50 B9 ? ? ? ? E8 ? ? ? ? EB 08 8D 4D F8"));
        setText = reinterpret_cast<decltype(setText)>(find_pattern(
            "55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 81 EC A4 00 00 00 A1 ? ? ? ? 89 45 F0 89 8D 60 FF FF FF 8D 4D D4 E8"));
        auto relocate = find_pattern("55 8B EC 83 EC 0C 83 7D 0C 00 75 05 E9 ? ? ? ? 8B 45 0C 8B 08 89 4D F4 81 7D F4 58 42 58 53");
        auto show = find_pattern("55 8B EC 6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 3C 89 4D D8 C7 45 FC 00 00 00 00");
        auto nativeShow = find_pattern("55 8B EC 83 EC 0C 89 4D F4 8B 45 F4 05 E4 00 00 00 50 E8 ? ? ? ? 83 C4 04 85 C0 75 16");
        auto cleanup = find_pattern("55 8B EC 83 EC 28 8B 4D 08 E8 ? ? ? ? 89 45 EC 8B 45 EC 8B 48 18 89 4D F4");
        auto event = find_pattern("55 8B EC 83 EC 28 89 4D D8 8B 4D 08 E8 ? ? ? ? 89 45 E8 8B 4D D8");
        auto draw = find_pattern("55 8B EC 83 EC 58 89 4D C8 8B 45 08 8B 08 83 E1 01");
        auto raw = find_pattern("8B 4D C8 83 C1 1C E8 ? ? ? ? 89 45 E0");
        if (raw)
            rawString = reinterpret_cast<decltype(rawString)>(injector::GetBranchDestination(static_cast<uint8_t*>(raw) + 6).as_int());
        auto create = find_pattern("8B 4D 0C 51 8B 8D 7C FF FF FF 83 C1 1C E8 ? ? ? ? C6 45 FC 01");
        if (create)
            createString = reinterpret_cast<decltype(createString)>(injector::GetBranchDestination(static_cast<uint8_t*>(create) + 13).as_int());
        destroyString = reinterpret_cast<decltype(destroyString)>(find_pattern("55 8B EC 51 89 4D FC 6A 00 6A 01 8B 4D FC E8 ? ? ? ? 8B E5 5D C3"));
        if (!relocate || !show || !nativeShow || !event || !draw || !cleanup || !rawString || !createString || !destroyString || !setText)
        {
            OutputDebugStringA("Psi-Ops: native menu unavailable for this executable.\n");
            return;
        }
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const wchar_t*>(Init), &host);
        cleanupHook = safetyhook::create_inline(cleanup, Cleanup);
        relocateHook = safetyhook::create_inline(relocate, Relocate);
        showHook = safetyhook::create_inline(show, Show);
        nativeShowHook = safetyhook::create_inline(nativeShow, NativeShow);
        eventHook = safetyhook::create_inline(event, Event);
        drawTextHook = safetyhook::create_inline(draw, DrawText);
    }
}
