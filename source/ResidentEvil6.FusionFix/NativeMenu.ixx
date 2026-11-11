module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <unordered_map>

export module NativeMenu;

import ComVars;
import HUD;
import InputDrain;

export namespace RE6Menu
{
    template<class T> T Read(const void* object, size_t offset)
    {
        return *reinterpret_cast<const T*>(static_cast<const uint8_t*>(object) + offset);
    }

    template<class T> void Write(void* object, size_t offset, T value)
    {
        injector::WriteMemory<T>(static_cast<uint8_t*>(object) + offset, value, true);
    }

    inline uint8_t* initialize = nullptr;
    inline uint8_t* execute = nullptr;
    inline uint8_t* destroy = nullptr;
    inline int (__stdcall* change)(int*, int, int, int) = nullptr;
    inline uint8_t* labels = nullptr;
    inline uint8_t* lookupMessage = nullptr;
    inline uint8_t* renderLabel = nullptr;
    inline uint8_t* hover = nullptr;
    inline void (__thiscall* setCount)(void*, int) = nullptr;
    inline void (__thiscall* setLabel)(void*, int, int, int, int, int) = nullptr;
    inline void (__thiscall* setSelection)(void*, int, int) = nullptr;
    inline unsigned (__thiscall* drawText)(void*, void*, const char*, int, int, const void*, int, int16_t) = nullptr;
    inline bool (__stdcall* accept)(int, char) = nullptr;
    inline bool (__stdcall* back)(int) = nullptr;
    inline bool (__stdcall* help)(int) = nullptr;
    inline unsigned (__stdcall* navigate)(unsigned, int, int, int, int, char) = nullptr;

    inline bool Bind()
    {
        auto optionInitPattern = hook::pattern("A1 ? ? ? ? 56 8B F1 8B 88 0C 01 00 00 89 4E 48 C7 46 4C 09 00 00 00");
        auto optionExecPattern = hook::pattern("53 56 8B F1 8B 86 88 01 00 00 33 DB 3B C3 74 ? 8B 40 10 83 E0 07 83 F8 02");
        auto optionLabelsPattern = hook::pattern("56 57 8B F9 33 F6 39 77 4C 7E ? 53 8D 9F 60 01 00 00 8B 0B BA 01 00 00 00");
        auto setCountPattern = hook::pattern("8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ? C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ?");
        auto setLabelPattern = hook::pattern("8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ? C2 14 00 CC CC CC CC CC CC");
        auto setSelectionPattern = hook::pattern("8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ? C2 08 00 CC CC CC CC CC CC CC CC CC CC CC CC CC CC 8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ? C3");
        auto drawTextPattern = hook::pattern("A1 ? ? ? ? 80 B8 9C 04 01 00 00 56 8B F1 0F 85 ? ? ? ? 83 78 68 00 75 ? 80 78 6C 00 0F 85 ? ? ? ? 57 8B 7C 24 1C 8B 47 18 83 F8 04 0F 83 ? ? ? ?");
        auto hoverPattern = hook::pattern("8B 89 50 45 00 00 85 C9 74 ? E9 ? ? ? ? 83 C8 FF C3 CC CC CC CC CC CC CC CC CC CC CC CC CC");
        auto renderLabelPattern = hook::pattern("55 8B EC 83 E4 F0 83 EC 24 8B 45 08 53 56 8B F1 8D 04 40 8B 84 C6 64 02 00 00 F3 0F 10 04 C5 ? ? ? ?");
        auto messagePattern = hook::pattern("0F B6 44 24 04 81 EC 14 01 00 00 55 57 8D 7C 81 54 33 ED 39 2F 75 ? 5F");
        auto acceptPattern = hook::pattern("8B 0D ? ? ? ? 80 B9 50 02 01 00 00 8B 44 24 04 53 56 74 ? 33 F6 EB ?");
        auto helpPattern = hook::pattern("8B 0D ? ? ? ? 80 B9 50 02 01 00 00 8B 44 24 04 74 ? 33 C0 C1 E8 0F 24 01 C2 04 00 8B 0D ? ? ? ?");
        auto backPattern = hook::pattern("8B 0D ? ? ? ? 80 B9 50 02 01 00 00 8B 44 24 04 56 74 ? 8B 0D ? ? ? ?");
        auto navigatePattern = hook::pattern("53 56 57 8B F9 8B 0D ? ? ? ? E8 ? ? ? ? 8B 5C 24 14 8B F0 85 F6");
        auto changePattern = hook::pattern("8B 44 24 10 8B 54 24 0C 50 8B 44 24 0C 52 8B 54 24 0C 50 52 6A FF E8 ? ? ? ?");
        auto destroyPattern = hook::pattern("56 8B F1 8B 86 88 01 00 00 C7 06 ? ? ? ? 85 C0 74 ? 81 60 10 FF DF FF FF");
        auto managerRef = hook::pattern("8B 0D ? ? ? ? 6A 00 6A 0C E8 ? ? ? ? 8B 4E 4C 51");
        auto textRef = hook::pattern("8B 0D ? ? ? ? 52 50 56 57 53 E8 ? ? ? ? 8B 4D 00");
        destroy = destroyPattern.get_first<uint8_t>();
        change = reinterpret_cast<decltype(change)>(changePattern.get_first());
        initialize = optionInitPattern.get_first<uint8_t>();
        execute = optionExecPattern.get_first<uint8_t>();
        labels = optionLabelsPattern.get_first<uint8_t>();
        lookupMessage = messagePattern.get_first<uint8_t>();
        renderLabel = renderLabelPattern.get_first<uint8_t>();
        hover = hoverPattern.get_first<uint8_t>();
        pMenuManager.SetAddress(*managerRef.get_first<void**>(2));
        pTextManager.SetAddress(*textRef.get_first<void**>(2));
        setCount = reinterpret_cast<decltype(setCount)>(setCountPattern.get_first());
        setLabel = reinterpret_cast<decltype(setLabel)>(setLabelPattern.get_first());
        setSelection = reinterpret_cast<decltype(setSelection)>(setSelectionPattern.get_first());
        drawText = reinterpret_cast<decltype(drawText)>(drawTextPattern.get_first());
        accept = reinterpret_cast<decltype(accept)>(acceptPattern.get_first());
        back = reinterpret_cast<decltype(back)>(backPattern.get_first());
        help = reinterpret_cast<decltype(help)>(helpPattern.get_first());
        navigate = reinterpret_cast<decltype(navigate)>(navigatePattern.get_first());
        return true;
    }
}

namespace RE6MenuGeometry
{
    inline int HitRow(float x, float y, float width, float height, int count)
    {
        if (width <= 0 || height <= 0 || count <= 0 || count > 13 || !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(width) || !std::isfinite(height)) return -1;
        const float scale = (std::min)(width / 1280.0f, height / 720.0f);
        x = (x - (width - 1280.0f * scale) / 2) / scale;
        y = (y - (height - 720.0f * scale) / 2) / scale;
        if (x < 207 || x >= 1176 || y < 163 || y >= 163 + count * 40) return -1;
        const int row = static_cast<int>((y - 163) / 40);
        return row < count && y < 195 + row * 40 ? row : -1;
    }
}

export namespace RE6Menu
{
    constexpr int MessageBase = 0x6F000000;
    inline HMODULE module = nullptr;
    inline std::set<std::string> joinedIds;
    inline std::function<int(void*)> joinedHover;
    inline SafetyHookInline initializeHook, executeHook, labelsHook, messageHook, destroyHook, renderLabelHook, hoverHook;
    inline std::array<std::string, 13> messages;
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;
    inline std::atomic<uint32_t> languageGeneration{ 0 };
    inline std::string updateNotice, updateVersion;
    inline bool updateHub = false;

    inline void RememberRestartValues(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                    if (setting.HasValue() && setting.apply == "restart")
                        restartValues.try_emplace({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id },
                            OtherPlugins::Get(*page.plugin, setting));
    }

    inline bool RestartRequired(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                {
                    if (!setting.HasValue() || setting.apply != "restart") continue;
                    const auto initial = restartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                    if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting)) return true;
                }
        return false;
    }

    inline std::vector<std::string> Wrap(const std::string& text)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t characters = 0;
        for (size_t i = 0; i < text.size();)
        {
            const auto first = static_cast<uint8_t>(text[i]);
            const size_t length = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
            if (text[i] == '\n' || characters == 64)
            {
                const auto space = line.find_last_of(' ');
                if (text[i] != '\n' && space != std::string::npos)
                {
                    lines.push_back(line.substr(0, space));
                    line.erase(0, space + 1);
                    characters = 0;
                    for (const unsigned char character : line)
                        characters += (character & 0xC0) != 0x80;
                }
                else
                {
                    lines.push_back(std::move(line));
                    line.clear();
                    characters = 0;
                }
                if (text[i] == '\n') { ++i; continue; }
            }
            if (text[i] != '\r' && !(line.empty() && text[i] == ' '))
            {
                line.append(text, i, (std::min)(length, text.size() - i));
                ++characters;
            }
            i += (std::min)(length, text.size() - i);
        }
        if (!line.empty()) lines.push_back(std::move(line));
        return lines;
    }

    struct Item
    {
        std::string label;
        RWGTAMenu::Row row;
        int link = -1;
    };

    struct Host
    {
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<std::string> help;
        int count = 0;
        size_t page = 0, first = 0, selected = 0, helpFirst = 0;
        uint32_t language = 0;
        bool open = false, details = false, helpKey = false;
    };
    inline std::unordered_map<void*, Host> hosts;

    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (language.empty() || language == "auto")
        {
            switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
            {
            case LANG_GERMAN: return "de";
            case LANG_FRENCH: return "fr";
            case LANG_ITALIAN: return "it";
            case LANG_SPANISH: return "es";
            default: return "en";
            }
        }
        return language;
    }

    inline std::wstring FormatUpdate(const std::string& version, bool hub)
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(Language());
        auto line = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
        const auto replace = [&line](const char* key, const std::string& value)
        {
            const auto at = line.find(key);
            if (at != std::string::npos) line.replace(at, strlen(key), value);
        };
        replace("{name}", texts.Resolve(FixSettings().GetDescription().name));
        replace("{version}", version);
        return asi_settings::text::Widen(line);
    }

    inline const char* __fastcall Message(void* object, void*, uint8_t group, int id)
    {
        const int key = group == 19 && id >= MessageBase - 2816 &&
            id < MessageBase - 2816 + static_cast<int>(messages.size()) ? id + 2816 : id;
        if (key >= MessageBase && key < MessageBase + static_cast<int>(messages.size()))
            return messages[key - MessageBase].c_str();
        return messageHook.unsafe_thiscall<const char*>(object, group, id);
    }

    inline void Text(void* owner, const std::string& value, int x, int y, int size, uint32_t color = 0xFFFFFFFF)
    {
        if (value.empty() || !pTextManager.get()) return;
        std::array<uint32_t, 14> style{};
        style[0] = 16;
        style[1] = color;
        style[2] = style[3] = size;
        style[6] = 1;
        style[7] = 16 * (96 ^ 0xFFF) + 1;
        style[10] = 0xF;
        style[11] = 3;
        style[13] = 1;
        NativeMenuLayoutScope layout;
        drawText(pTextManager.get(), owner, value.c_str(), x, y, style.data(), Read<int>(owner, 104), 0);
    }

    inline int __fastcall RenderLabel(void* object, void*, int row, unsigned part, float* position, int transform, int color)
    {
        if (row >= 0 && row < 13)
        {
            const auto id = Read<int>(object, 612 + row * 24);
            if (id >= MessageBase && id < MessageBase + static_cast<int>(messages.size()))
            {
                Text(object, messages[id - MessageBase], 207, 187 + row * 40, 20,
                    Read<int>(object, 556) == row ? 0xFFFFFFFF : 0xFFAAAAAA);
                return 0;
            }
        }
        return renderLabelHook.unsafe_thiscall<int>(object, row, part, position, transform, color);
    }

    inline int __fastcall Hover(void* object, void*)
    {
        if (object == pMenuManager.get() && !RE6Input::Blocked())
        {
            const auto ui = Read<void*>(object, 0x4550);
            const auto window = RE6Input::window.load();
            POINT point{};
            RECT client{};
            if (ui && window && GetCursorPos(&point) && ScreenToClient(window, &point) && GetClientRect(window, &client))
            {
                for (const auto& [owner, host] : hosts)
                {
                    if (Read<uint8_t>(owner, 56) != 2) continue;
                    const int row = RE6MenuGeometry::HitRow(static_cast<float>(point.x), static_cast<float>(point.y),
                        static_cast<float>(client.right), static_cast<float>(client.bottom), Read<int>(ui, 552));
                    if (host.open) return host.details ? -1 : row;
                    if (row == host.count) return row;
                }
            }
        }
        if (joinedHover && !RE6Input::Blocked())
        {
            const int row = joinedHover(object);
            if (row != -2) return row;
        }
        return hoverHook.unsafe_thiscall<int>(object);
    }

    inline void Count(int count)
    {
        const auto ui = pMenuManager.get() ? Read<void*>(pMenuManager.get(), 0x4550) : nullptr;
        if (ui && Read<int>(ui, 552) != count) setCount(pMenuManager.get(), count);
    }

    inline void Populate(Host& host)
    {
        host.items.clear();
        if (!host.page)
        {
            const auto& pages = host.model.Pages();
            for (size_t i = host.first; i < pages.size() && i < host.first + 6; ++i)
                host.items.push_back({ pages[i].title, {}, static_cast<int>(i + 1) });
            if (host.first) host.items.push_back({ host.model.Text("menu.previous"), {}, -2 });
            if (host.first + 6 < pages.size()) host.items.push_back({ host.model.Text("menu.next"), {}, -3 });
            if (!updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates"))
                host.items.push_back({ host.model.Text("update.title"), {}, -5 });
        }
        else
        {
            const auto rows = host.model.VisibleRows(host.page - 1);
            if (host.first >= rows.size()) host.first = 0;
            for (size_t i = host.first; i < rows.size() && i < host.first + 6; ++i)
            {
                const auto row = *rows[i];
                auto label = host.model.Label(row);
                auto value = host.model.Value(row);
                if (!value.empty()) label += "  " + value;
                host.items.push_back({ std::move(label), row });
            }
            if (host.first) host.items.push_back({ host.model.Text("menu.previous"), {}, -2 });
            if (host.first + 6 < rows.size()) host.items.push_back({ host.model.Text("menu.next"), {}, -3 });
        }
        host.items.push_back({ host.model.Text("menu.back"), {}, -4 });
        host.selected = (std::min)(host.selected, host.items.size() - 1);
        for (size_t i = 0; i < host.items.size(); ++i)
        {
            messages[i] = host.items[i].label;
            setLabel(pMenuManager.get(), static_cast<int>(i), MessageBase + static_cast<int>(i), -1, -1, 0);
        }
        Count(static_cast<int>(host.items.size()));
        setSelection(pMenuManager.get(), static_cast<int>(host.selected), 0);
    }

    inline void Back(Host& host)
    {
        if (host.details) host.details = false;
        else if (host.page) host.page = host.first = host.selected = 0;
        else
        {
            host.open = false;
            host.model.Close();
            return;
        }
        Populate(host);
    }

    inline void Change(Host& host, int direction)
    {
        const auto item = host.items[host.selected];
        if (item.link == -1) host.model.Change(item.row, direction, false);
        else if (!direction)
        {
            if (item.link == -5)
            {
                host.help = Wrap(updateNotice);
                host.details = !host.help.empty();
                host.helpFirst = 0;
                return;
            }
            if (item.link == -4) { Back(host); return; }
            if (item.link == -2) host.first = host.first >= 6 ? host.first - 6 : 0;
            else if (item.link == -3) host.first += 6;
            else if (item.link > 0)
            {
                host.page = item.link;
                host.first = 0;
                host.model.PageShown(host.page - 1);
            }
            host.selected = 0;
        }
        Populate(host);
    }

    inline void __fastcall Labels(void* object, void*)
    {
        const auto found = hosts.find(object);
        if (found == hosts.end()) { labelsHook.unsafe_thiscall(object); return; }
        auto& host = found->second;
        if (host.open) { Populate(host); return; }
        if (Read<uint8_t>(object, 56) > 2) { labelsHook.unsafe_thiscall(object); return; }
        const auto count = Read<int>(object, 76);
        Write(object, 76, host.count);
        labelsHook.unsafe_thiscall(object);
        Write(object, 76, count);
        messages[12] = host.model.Text("menu.plugins");
        Count(host.count + 1);
        setLabel(pMenuManager.get(), host.count, MessageBase + 12, -1, -1, 0);
    }

    inline void __fastcall Initialize(void* object, void*)
    {
        hosts.erase(object);
        initializeHook.unsafe_thiscall(object);
        const auto count = Read<int>(object, 76);
        if (count < 1 || count > 9) return;
        auto& host = hosts[object];
        host.count = count;
        host.model.OpenPlugins(module, Language(), 6, joinedIds, true);
        host.language = languageGeneration.load();
        RememberRestartValues(host.model);
        Labels(object, nullptr);
    }

    inline int __fastcall Execute(void* object, void*)
    {
        RWGTA::UpdateCheck::Tick();
        const auto found = hosts.find(object);
        if (found == hosts.end() || !pMenuManager.get()) return executeHook.unsafe_thiscall<int>(object);
        auto& host = found->second;
        if (host.language != languageGeneration.load())
        {
            host.model.OpenPlugins(module, Language(), 6, joinedIds, true);
            RememberRestartValues(host.model);
            host.language = languageGeneration.load();
            host.details = false;
            if (!updateVersion.empty()) updateNotice = asi_settings::text::Narrow(FormatUpdate(updateVersion, updateHub));
            if (host.page > host.model.Pages().size()) host.page = host.first = host.selected = 0;
            if (host.open) Populate(host);
        }
        if (Read<uint8_t>(object, 56) != 2)
        {
            const bool reset = Read<uint8_t>(object, 56) == 11 && Read<int>(object, 388) == 0;
            const auto result = executeHook.unsafe_thiscall<int>(object);
            if (reset && Read<uint8_t>(object, 56) == 2)
                for (const auto& row : host.model.JoinedRows())
                    if (row.plugin && row.setting && row.setting->HasValue())
                    {
                        asi_settings::Slot slot;
                        slot.setting = row.setting;
                        asi_settings::SetDefault(slot);
                        OtherPlugins::Set(*row.plugin, *row.setting, asi_settings::FormatValue(slot));
                    }
            return result;
        }
        if (!host.open && Read<int>(object, 72) == host.count && accept(-1, 1))
        {
            host.model.OpenPlugins(module, Language(), 6, joinedIds, true);
            RememberRestartValues(host.model);
            host.open = true;
            host.page = host.first = host.selected = 0;
            Populate(host);
            return 0;
        }
        if (host.open)
        {
            DWORD foregroundProcess = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
            const bool helpKey = foregroundProcess == GetCurrentProcessId() && !RE6Input::Blocked() &&
                (((GetAsyncKeyState(VK_F1) & 0x8000) != 0 && !RE6Input::keyboardBlocked.load()) || help(-1));
            const bool detailsPressed = helpKey && !host.helpKey && !RE6Input::Blocked() && !RE6Input::keyboardBlocked.load();
            host.helpKey = helpKey;
            if (host.details)
            {
                if (back(-1) || accept(-1, 1) || detailsPressed)
                {
                    host.details = false;
                    Populate(host);
                    return 0;
                }
                const auto pages = (host.help.size() + 8) / 9;
                host.helpFirst = navigate(static_cast<unsigned>(host.helpFirst / 9), static_cast<int>(pages), -1, 0, 0, 0) * 9;
                Count(0);
                Text(object, host.items[host.selected].label, 207, 100, 24);
                for (size_t i = host.helpFirst; i < host.help.size() && i < host.helpFirst + 9; ++i)
                    Text(object, host.help[i], 104, 180 + static_cast<int>(i - host.helpFirst) * 40, 16);
                Text(object, host.model.Text("menu.back"), 104, 650, 16);
                return 0;
            }
            if (detailsPressed && (host.items[host.selected].link == -1 || host.items[host.selected].link == -5))
            {
                host.help = Wrap(host.items[host.selected].link == -5 ? updateNotice : host.model.Help(host.items[host.selected].row));
                if (!host.help.empty())
                {
                    host.details = true;
                    host.helpFirst = 0;
                    Count(0);
                    return 0;
                }
            }
            if (back(-1)) Back(host);
            else if (accept(-1, 1)) Change(host, 0);
            else
            {
                host.selected = navigate(static_cast<unsigned>(host.selected), static_cast<int>(host.items.size()), -1, 0, 0, 0);
                if (host.selected >= host.items.size()) host.selected = 0;
                int value = 1;
                change(&value, 3, -1, 0);
                if (value != 1) Change(host, value > 1 ? 1 : -1);
            }
            if (host.open)
            {
                Populate(host);
                const auto title = host.page ? host.model.Pages()[host.page - 1].title : host.model.Text("menu.plugins");
                Text(object, title, 207, 100, 24);
                const auto& item = host.items[host.selected];
                if (item.link == -1)
                {
                    const auto help = Wrap(host.model.Help(item.row));
                    if (!help.empty()) Text(object, help.front(), 104, 585, 16);
                    Text(object, host.model.Text("menu.helphint"), 104, 615, 16);
                }
                if (RestartRequired(host.model)) Text(object, host.model.Text("menu.restartwarning"), 104, 650, 16);
                return 0;
            }
            Write(object, 72, host.count);
            Labels(object, nullptr);
            return 0;
        }
        // The last native ID slot shares storage with reset confirmation. Borrow it only during active navigation.
        const size_t offset = 352 + host.count * sizeof(int);
        const auto saved = Read<int>(object, offset);
        Write(object, offset, 0);
        Write(object, 76, host.count + 1);
        const auto result = executeHook.unsafe_thiscall<int>(object);
        Write(object, 76, host.count);
        Write(object, offset, saved);
        if (!updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates"))
            Text(object, host.model.Text("update.short"), 104, 585, 16);
        return result;
    }

    inline void* __fastcall Destroy(void* object, void*, char flags)
    {
        hosts.erase(object);
        return destroyHook.unsafe_thiscall<void*>(object, flags);
    }

    inline void Install(HMODULE ownModule)
    {
        module = ownModule;
        FixSettings().OnChange("MAIN.TextLanguage", []() { ++languageGeneration; });
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.try_emplace({ reinterpret_cast<uintptr_t>(ownModule), setting.id }, FixSettings().Get(setting.id));
        Bind();
        initializeHook = safetyhook::create_inline(initialize, Initialize);
        executeHook = safetyhook::create_inline(execute, Execute);
        labelsHook = safetyhook::create_inline(labels, Labels);
        messageHook = safetyhook::create_inline(lookupMessage, Message);
        destroyHook = safetyhook::create_inline(destroy, Destroy);
        renderLabelHook = safetyhook::create_inline(renderLabel, RenderLabel);
        hoverHook = safetyhook::create_inline(hover, Hover);
        RWGTA::UpdateCheck::Config updates;
        updates.pluginId = "ResidentEvil6.FusionFix";
        updates.fileVersion = rsc_FileVersion;
        updates.settings = &FixSettings();
        updates.playable = []()
        {
            return std::any_of(hosts.begin(), hosts.end(), [](const auto& entry)
            {
                return Read<uint8_t>(entry.first, 56) == 2 && (Read<uint32_t>(entry.first, 16) & 7) == 2;
            });
        };
        updates.show = [](const std::wstring& notice) { updateNotice = asi_settings::text::Narrow(notice); };
        updates.format = [](const std::string& version, bool hub)
        {
            updateVersion = version;
            updateHub = hub;
            return FormatUpdate(version, hub);
        };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}

export namespace RE6CameraMenu
{
    struct Host
    {
        RWGTAMenu::Model model;
        std::vector<std::string> help;
        size_t helpFirst = 0;
        uint32_t language = 0;
        bool details = false, helpKey = false;
        void* hiddenUI = nullptr;
        void* hiddenParts = nullptr;
        std::array<uint8_t, 18> visibility{};
    };
    inline std::unordered_map<void*, Host> hosts;
    inline SafetyHookInline initializeHook, executeHook, labelsHook, footerHook, destroyHook, navigateHook, visibilityHook, categoriesHook;
    inline thread_local void* navigating = nullptr;
    constexpr int NativeRows = 11;

    inline void Open(Host& host)
    {
        host.model.OpenPlugins(RE6Menu::module, RE6Menu::Language(), 6, { "MAIN.FOVFactor" }, true);
        host.language = RE6Menu::languageGeneration.load();
    }

    inline const RWGTAMenu::Row* Row(const Host& host)
    {
        for (const auto& row : host.model.JoinedRows())
            if (row.setting && row.setting->id == "MAIN.FOVFactor") return &row;
        return nullptr;
    }

    inline void* CameraUI()
    {
        const auto manager = pMenuManager.get();
        const auto ui = manager ? RE6Menu::Read<void*>(manager, 0x4550) : nullptr;
        return ui && RE6Menu::Read<int>(ui, 544) == 14 ? ui : nullptr;
    }

    inline void HelpVisibility(Host& host, bool hide)
    {
        const auto ui = CameraUI();
        const auto parts = ui ? RE6Menu::Read<void*>(ui, 532) : nullptr;
        const auto count = ui ? RE6Menu::Read<unsigned>(ui, 504) : 0;
        if (hide && !host.hiddenUI && parts && count >= host.visibility.size())
        {
            host.hiddenUI = ui;
            host.hiddenParts = parts;
            for (unsigned i = 0; i < host.visibility.size(); ++i)
            {
                host.visibility[i] = RE6Menu::Read<uint8_t>(parts, 160 * i + 4);
                visibilityHook.unsafe_thiscall(ui, char{ 0 }, i);
            }
        }
        else if (!hide && host.hiddenUI)
        {
            if (ui == host.hiddenUI && parts == host.hiddenParts && count >= host.visibility.size())
                for (unsigned i = 0; i < host.visibility.size(); ++i)
                    visibilityHook.unsafe_thiscall(ui, static_cast<char>(host.visibility[i]), i);
            host.hiddenUI = host.hiddenParts = nullptr;
        }
    }

    inline int __fastcall Visibility(void* ui, void*, char visible, unsigned part)
    {
        for (auto& [object, host] : hosts)
            if (host.hiddenUI == ui && RE6Menu::Read<void*>(ui, 532) == host.hiddenParts && part < host.visibility.size())
            {
                host.visibility[part] = static_cast<uint8_t>(visible);
                visible = 0;
                break;
            }
        return visibilityHook.unsafe_thiscall<int>(ui, visible, part);
    }

    inline void __fastcall Categories(void* ui, void*)
    {
        if (ui == CameraUI())
            for (const auto& [object, host] : hosts)
            {
                if (host.details) return;
                const auto selected = RE6Menu::Read<int>(ui, 556);
                if (selected == NativeRows)
                {
                    RE6Menu::Write(ui, 556, NativeRows - 1);
                    categoriesHook.unsafe_thiscall(ui);
                    RE6Menu::Write(ui, 556, selected);
                    return;
                }
            }
        categoriesHook.unsafe_thiscall(ui);
    }

    inline void Labels(void* object)
    {
        labelsHook.unsafe_thiscall(object);
        const auto found = hosts.find(object);
        if (found == hosts.end()) return;
        auto& host = found->second;
        if (RE6Menu::Read<uint8_t>(object, 56) > 2) { RE6Menu::Count(NativeRows); return; }
        HelpVisibility(host, host.details);
        if (host.details) { RE6Menu::Count(0); return; }
        if (const auto row = Row(host))
        {
            RE6Menu::messages[11] = host.model.Label(*row) + "  " + host.model.Value(*row);
            RE6Menu::Count(NativeRows + 1);
            RE6Menu::setLabel(pMenuManager.get(), NativeRows, RE6Menu::MessageBase + 11, -1, -1, 0);
        }
    }

    inline void __fastcall NativeLabels(void* object, void*) { Labels(object); }

    inline int __fastcall Initialize(void* object, void*)
    {
        hosts.erase(object);
        const auto result = initializeHook.unsafe_thiscall<int>(object);
        if (RE6Menu::Read<int>(object, 76) != NativeRows) return result;
        auto& host = hosts[object];
        Open(host);
        if (!Row(host)) { hosts.erase(object); return result; }
        Labels(object);
        return result;
    }

    inline int __fastcall Footer(void* object, void*)
    {
        const auto found = hosts.find(object);
        if (found != hosts.end() && RE6Menu::Read<uint8_t>(object, 56) == 2 &&
            (found->second.details || RE6Menu::Read<int>(object, 72) == NativeRows))
        {
            const auto selected = RE6Menu::Read<int>(object, 72);
            RE6Menu::Write(object, 72, 0);
            const auto result = footerHook.unsafe_thiscall<int>(object);
            RE6Menu::Write(object, 72, selected);
            RE6Menu::Write(object, 208, 0);
            RE6Menu::Write(object, 212, 0);
            return result;
        }
        return footerHook.unsafe_thiscall<int>(object);
    }

    inline unsigned __stdcall Navigate(unsigned selected, int count, int player, int mask, int first, char mode)
    {
        if (navigating && hosts.contains(navigating) && count == NativeRows && RE6Menu::Read<uint8_t>(navigating, 56) == 2)
            ++count;
        return navigateHook.unsafe_stdcall<unsigned>(selected, count, player, mask, first, mode);
    }

    inline int Hover(void* manager)
    {
        if (manager != pMenuManager.get()) return -2;
        const auto ui = RE6Menu::Read<void*>(manager, 0x4550);
        if (!ui || RE6Menu::Read<int>(ui, 544) != 14) return -2;
        const auto window = RE6Input::window.load();
        POINT point{}; RECT client{};
        if (!window || !GetCursorPos(&point) || !ScreenToClient(window, &point) || !GetClientRect(window, &client)) return -2;
        for (const auto& [object, host] : hosts)
        {
            if (RE6Menu::Read<uint8_t>(object, 56) != 2) continue;
            if (host.details) return -1;
            const int row = RE6MenuGeometry::HitRow(static_cast<float>(point.x), static_cast<float>(point.y),
                static_cast<float>(client.right), static_cast<float>(client.bottom), NativeRows + 1);
            if (row == NativeRows) return row;
        }
        return -2;
    }

    inline void Draw(void* object, Host& host)
    {
        const auto row = Row(host);
        if (!row) return;
        if (host.details)
        {
            RE6Menu::Text(object, host.model.Label(*row), 207, 100, 24);
            for (size_t i = host.helpFirst; i < host.help.size() && i < host.helpFirst + 9; ++i)
                RE6Menu::Text(object, host.help[i], 104, 180 + static_cast<int>(i - host.helpFirst) * 40, 16);
            RE6Menu::Text(object, host.model.Text("menu.back"), 104, 650, 16);
        }
        else if (RE6Menu::Read<int>(object, 72) == NativeRows)
        {
            const auto help = RE6Menu::Wrap(host.model.Help(*row));
            if (!help.empty()) RE6Menu::Text(object, help.front(), 104, 650, 16);
            RE6Menu::Text(object, host.model.Text("menu.helphint"), 1000, 100, 16);
        }
        if (RE6Menu::RestartRequired(host.model)) RE6Menu::Text(object, host.model.Text("menu.restartwarning"), 104, 680, 16);
    }

    inline int __fastcall Execute(void* object, void*)
    {
        const auto found = hosts.find(object);
        if (found == hosts.end()) return executeHook.unsafe_thiscall<int>(object);
        auto& host = found->second;
        if (host.language != RE6Menu::languageGeneration.load()) { Open(host); host.details = false; }
        if (RE6Menu::Read<uint8_t>(object, 56) != 2) return executeHook.unsafe_thiscall<int>(object);
        const auto row = Row(host);
        if (!row) return executeHook.unsafe_thiscall<int>(object);
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const bool down = foregroundProcess == GetCurrentProcessId() && !RE6Input::Blocked() &&
            (((GetAsyncKeyState(VK_F1) & 0x8000) && !RE6Input::keyboardBlocked.load()) || RE6Menu::help(-1));
        const bool pressed = down && !host.helpKey;
        host.helpKey = down;
        if (host.details)
        {
            if (RE6Menu::back(-1) || RE6Menu::accept(-1, 0) || pressed) host.details = false;
            else
            {
                const auto pages = static_cast<int>((host.help.size() + 8) / 9);
                host.helpFirst = RE6Menu::navigate(static_cast<unsigned>(host.helpFirst / 9), pages, -1, 0, 0, 0) * 9;
            }
            Labels(object); Draw(object, host);
            return 0;
        }
        if (RE6Menu::Read<int>(object, 72) == NativeRows)
        {
            if (pressed)
            {
                host.help = RE6Menu::Wrap(host.model.Help(*row)); host.helpFirst = 0; host.details = !host.help.empty();
            }
            else if (RE6Menu::back(-1))
            {
                RE6Menu::Write(object, 72, 0);
                return executeHook.unsafe_thiscall<int>(object);
            }
            else
            {
                if (RE6Menu::accept(-1, 1)) host.model.Change(*row, 0, false);
                else
                {
                    int direction = 1;
                    RE6Menu::change(&direction, 3, -1, 0);
                    if (direction != 1) host.model.Change(*row, direction > 1 ? 1 : -1, false);
                }
                const auto selected = RE6Menu::navigate(NativeRows, NativeRows + 1, -1, 0, 0, 0);
                RE6Menu::Write(object, 72, static_cast<int>(selected));
                RE6Menu::setSelection(pMenuManager.get(), static_cast<int>(selected), 0);
            }
            Labels(object);
            if (!host.details) Footer(object, nullptr);
            Draw(object, host);
            return 0;
        }
        const auto previous = navigating;
        navigating = object;
        const auto result = executeHook.unsafe_thiscall<int>(object);
        navigating = previous;
        Draw(object, host);
        return result;
    }

    inline void* __fastcall Destroy(void* object, void*, char flags)
    {
        if (const auto found = hosts.find(object); found != hosts.end()) HelpVisibility(found->second, false);
        hosts.erase(object);
        return destroyHook.unsafe_thiscall<void*>(object, flags);
    }

    inline void Install()
    {
        auto initialize = hook::pattern("56 8B F1 C7 46 48 00 00 00 00 B9 0B 00 00 00 89 4E 4C A1 ? ? ? ? 8B 50 20");
        auto execute = hook::pattern("51 56 8B F1 8B 46 38 25 FF 00 00 00 57 83 F8 5C 0F 87 ? ? ? ? 0F B6 80 ? ? ? ? FF 24 85 ? ? ? ? 6A 00 E8 ? ? ? ? FE 46 38 E9 ? ? ? ? 6A 00 E8 ? ? ? ? 84 C0 0F 84 ? ? ? ? FE 46 38 E9 ? ? ? ? 6A FF E8 ? ? ? ? 84 C0 74 ? 8B CE E8 ? ? ? ? C6 46 41 01 C6 46 38 5A E9 ? ? ? ? 8B 4E 4C 8B 56 48 6A 00 6A 00 6A 00 6A FF 51 52 8B CE E8 ? ? ? ? 8B 4E 48 89 44 24 08 3B C8 74 ? 6A 01 89 46 48 8B 0D ? ? ? ? 50 E8 ? ? ? ? 8B CE E8 ? ? ? ? E9 ? ? ? ? 8B 84 8E 60 01 00 00 6A 00 89 44 24 0C 8B 94 8E 8C 01 00 00");
        auto labels = hook::pattern("83 EC 08 53 56 8B D9 33 F6 39 73 4C 0F 8E ? ? ? ? 55 57 EB ? 8D A4 24 00 00 00 00 8D 49 00 8B 84 B3 8C 01 00 00");
        auto footer = hook::pattern("53 55 56 8B D9 57 8D AB B8 01 00 00 BF ? ? ? ? BE BC 01 00 00 0F BF 07");
        auto navigate = hook::pattern("53 56 57 8B F9 8B 0D ? ? ? ? E8 ? ? ? ? 8B 5C 24 14 8B F0 85 F6");
        auto categories = hook::pattern("55 8B EC 83 E4 F0 83 EC 54 53 56 8B F1 83 BE 20 02 00 00 0E 57 0F 85");
        auto visibility = hook::pattern("8B 44 24 08 3B 81 F8 01 00 00 73 20 8A 54 24 04 8D 04 80 C1 E0 05 03 81 14 02 00 00");
        auto table = hook::pattern("C7 06 ? ? ? ? C7 86 B8 01 00 00 00 00 00 00");
        const auto vtable = *table.get_first<uintptr_t*>(2);
        const auto entries = reinterpret_cast<const uintptr_t*>(vtable);
        initializeHook = safetyhook::create_inline(initialize.get_first(), Initialize);
        executeHook = safetyhook::create_inline(execute.get_first(), Execute);
        labelsHook = safetyhook::create_inline(labels.get_first(), NativeLabels);
        footerHook = safetyhook::create_inline(footer.get_first(), Footer);
        navigateHook = safetyhook::create_inline(navigate.get_first(), Navigate);
        destroyHook = safetyhook::create_inline(entries[0], Destroy);
        visibilityHook = safetyhook::create_inline(visibility.get_first(), Visibility);
        categoriesHook = safetyhook::create_inline(categories.get_first(), Categories);
        RE6Menu::joinedIds.insert("MAIN.FOVFactor");
        RE6Menu::joinedHover = Hover;
    }
}
