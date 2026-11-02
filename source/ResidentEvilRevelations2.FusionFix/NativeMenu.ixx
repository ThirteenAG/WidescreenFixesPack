module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include <array>
#include <map>
#include <mutex>

export module NativeMenu;

// Native Options list host. The game owns the row widgets and their navigation; this module supplies row text and
// handles the synthetic actions after the game's normal selection transition.
export namespace Rev2Menu
{
    constexpr int CustomFile = 0x6f45;
    constexpr int ActionBase = 1000;
    constexpr size_t Capacity = 8;
    struct Definition { int action, file, text, value, helpFile, helpText, disabledFile, disabledText; uintptr_t extra; };
    static_assert(sizeof(Definition) == 36);
    enum class View { Root, Plugins, Settings, Details };
    enum class Kind { None, Plugin, Setting, Previous, Next, Details, Back, Text };
    struct Item { Kind kind{}; size_t index{}; RWGTAMenu::Row row{}; };

    inline SafetyHookInline optionsHook, messageHook, destroyHook;
    inline void (__thiscall* build)(void*, int, int){};
    inline void (__thiscall* refresh)(void*, int, int){};
    inline void* (__thiscall* findWidget)(void*, int, int){};
    inline void (__thiscall* showWidget)(void*, int){};
    inline void (__thiscall* animateWidget)(void*, int){};
    inline uint32_t* typeTable{};
    inline std::array<uint32_t, 5> originalType{};
    inline std::array<Definition, Capacity> originalRows{}, rows{};
    inline std::array<Item, Capacity> items{};
    inline std::map<std::string, int> textIds;
    inline std::map<int, std::string> texts;
    inline std::map<std::pair<HMODULE, std::string>, std::string> restartValues;
    inline std::mutex textMutex;
    inline RWGTAMenu::Model model;
    inline HMODULE module{};
    inline View view = View::Root;
    inline void* currentObject{};
    inline size_t pluginFirst{}, currentPage{}, first{}, detailsFirst{};
    inline int detailsReturnSelection{};
    inline RWGTAMenu::Row lastRow{};
    inline bool hasLastRow{};
    inline std::vector<std::string> detailLines;
    inline std::string updateNotice;
    inline bool renderedNotice{};

    inline bool UpdateVisible() { return !updateNotice.empty() && FixSettings().Bool("MAIN.CheckForUpdates"); }
    inline size_t PluginPageSize() { return UpdateVisible() ? 2 : 5; }

    inline int TextId(const std::string& value)
    {
        std::lock_guard guard(textMutex);
        if (const auto found = textIds.find(value); found != textIds.end()) return found->second;
        const int id = static_cast<int>(texts.size()) + 1;
        texts.emplace(id, value);
        textIds.emplace(value, id);
        return id;
    }

    inline const char* __fastcall Message(void* object, void*, int file, int id)
    {
        if (file == CustomFile)
        {
            std::lock_guard guard(textMutex);
            const auto found = texts.find(id);
            if (found != texts.end()) return found->second.c_str();
        }
        return messageHook.unsafe_thiscall<const char*>(object, file, id);
    }

    inline std::string Language()
    {
        const auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto") return language;
        switch (PRIMARYLANGID(GetUserDefaultUILanguage()))
        {
        case LANG_FRENCH: return "fr";
        case LANG_GERMAN: return "de";
        case LANG_ITALIAN: return "it";
        case LANG_SPANISH: return "es";
        default: return "en";
        }
    }

    inline void RememberRestartValues()
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                    if (setting.HasValue() && setting.apply == "restart")
                        restartValues.try_emplace({page.plugin->module, setting.id}, OtherPlugins::Get(*page.plugin, setting));
    }

    inline bool RestartRequired()
    {
        for (const auto& page : model.Pages())
            if (page.plugin)
                for (const auto& setting : page.plugin->file.description.settings)
                {
                    if (!setting.HasValue() || setting.apply != "restart") continue;
                    const auto initial = restartValues.find({page.plugin->module, setting.id});
                    if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting))
                        return true;
                }
        return false;
    }

    inline std::vector<std::string> Wrap(const std::string& text, size_t columns = 45)
    {
        std::vector<std::string> lines;
        std::string line;
        size_t count = 0;
        for (size_t i = 0; i < text.size();)
        {
            const unsigned char c = static_cast<unsigned char>(text[i]);
            const size_t length = c < 0x80 ? 1 : c < 0xe0 ? 2 : c < 0xf0 ? 3 : 4;
            if (text[i] == '\n' || count >= columns)
            {
                const auto space = line.find_last_of(' ');
                if (text[i] != '\n' && space != std::string::npos)
                {
                    lines.push_back(line.substr(0, space));
                    line.erase(0, space + 1);
                    count = 0;
                    for (const unsigned char ch : line) count += (ch & 0xc0) != 0x80;
                }
                else { lines.push_back(line); line.clear(); count = 0; }
                if (text[i] == '\n') { ++i; continue; }
            }
            const size_t take = (std::min)(length, text.size() - i);
            line.append(text, i, take); i += take; ++count;
        }
        if (!line.empty()) lines.push_back(line);
        return lines;
    }

    inline void Row(size_t slot, const std::string& label, Kind kind, size_t index = 0, RWGTAMenu::Row row = {})
    {
        rows[slot] = originalRows[0];
        rows[slot].action = ActionBase + static_cast<int>(slot);
        rows[slot].file = CustomFile;
        rows[slot].text = TextId(label);
        rows[slot].value = 0;
        rows[slot].helpFile = -1;
        rows[slot].helpText = -1;
        rows[slot].disabledFile = -1;
        rows[slot].disabledText = -1;
        items[slot] = { kind, index, row };
    }

    inline void Publish(void* object, size_t count, const std::string& title, int selection = 0)
    {
        injector::WriteMemory<uint32_t>(typeTable, reinterpret_cast<uint32_t>(rows.data()), true);
        injector::WriteMemory<uint32_t>(typeTable + 1, static_cast<uint32_t>(count), true);
        injector::WriteMemory<uint32_t>(typeTable + 3, CustomFile, true);
        injector::WriteMemory<uint32_t>(typeTable + 4, TextId(title), true);
        auto bytes = static_cast<uint8_t*>(object);
        *reinterpret_cast<int*>(bytes + 0x2c0) = std::clamp(selection, 0, static_cast<int>(count) - 1);
        build(object, 14, 0); refresh(object, 14, 0);
        build(object, 14, 1); refresh(object, 14, 1);
        for (int i = 0; i < 10; ++i)
            if (auto widget = findWidget(object, 14 + i, 1))
            {
                showWidget(widget, i < static_cast<int>(count));
                if (i < static_cast<int>(count)) animateWidget(widget, i == selection ? 4 : 6);
            }
    }

    inline void ResetTable()
    {
        view = View::Root;
        model.Close();
        for (size_t i = 0; i < 7; ++i) rows[i] = originalRows[i];
        items = {};
        Row(7, "Plugins", Kind::Plugin);
        injector::WriteMemory<uint32_t>(typeTable, reinterpret_cast<uint32_t>(rows.data()), true);
        injector::WriteMemory<uint32_t>(typeTable + 1, 8, true);
        injector::WriteMemory<uint32_t>(typeTable + 3, originalType[3], true);
        injector::WriteMemory<uint32_t>(typeTable + 4, originalType[4], true);
    }

    inline void Root(void* object, int selection = 7)
    {
        ResetTable();
        auto bytes = static_cast<uint8_t*>(object);
        *reinterpret_cast<int*>(bytes + 0x2c0) = selection;
        build(object, 14, 0); refresh(object, 14, 0);
        build(object, 14, 1); refresh(object, 14, 1);
        for (int i = 0; i < 10; ++i)
            if (auto widget = findWidget(object, 14 + i, 1))
            {
                showWidget(widget, i < 8);
                if (i < 8) animateWidget(widget, i == selection ? 4 : 6);
            }
    }

    inline void Populate(void* object, int selection = 0)
    {
        items = {};
        size_t count = 0;
        std::string title = model.Text("menu.plugins");
        if (view == View::Plugins)
        {
            const auto& pages = model.Pages();
            pluginFirst = pages.empty() ? 0 : (std::min)(pluginFirst, ((pages.size() - 1) / PluginPageSize()) * PluginPageSize());
            const auto end = (std::min)(pages.size(), pluginFirst + PluginPageSize());
            for (size_t i = pluginFirst; i < end; ++i) Row(count++, pages[i].title, Kind::Plugin, i);
            if (pluginFirst) Row(count++, model.Text("menu.previous"), Kind::Previous);
            if (end < pages.size()) Row(count++, model.Text("menu.next"), Kind::Next);
            renderedNotice = UpdateVisible();
            if (renderedNotice)
            {
                const auto lines = Wrap(updateNotice);
                for (size_t i = 0; i < (std::min)(size_t(3), lines.size()); ++i) Row(count++, lines[i], Kind::Text);
            }
            Row(count++, model.Text("menu.back"), Kind::Back);
        }
        else if (view == View::Settings && currentPage < model.Pages().size())
        {
            title = model.Pages()[currentPage].title;
            const auto visible = model.VisibleRows(currentPage);
            first = (std::min)(first, visible.size());
            const auto end = (std::min)(visible.size(), first + 2);
            for (size_t i = first; i < end; ++i)
            {
                const auto row = *visible[i];
                auto label = model.Label(row), value = model.Value(row);
                if (!value.empty()) label += ": " + value;
                Row(count++, label, Kind::Setting, i, row);
                Row(count++, model.Text("menu.details"), Kind::Details, i, row);
            }
            if (first) Row(count++, model.Text("menu.previous"), Kind::Previous);
            if (end < visible.size()) Row(count++, model.Text("menu.next"), Kind::Next);
            if (RestartRequired()) Row(count++, model.Text("menu.restartwarning"), Kind::Text);
            Row(count++, model.Text("menu.back"), Kind::Back);
        }
        else if (view == View::Details)
        {
            title = hasLastRow ? model.Label(lastRow) : model.Text("menu.details");
            detailsFirst = (std::min)(detailsFirst, detailLines.size());
            const auto end = (std::min)(detailLines.size(), detailsFirst + 5);
            for (size_t i = detailsFirst; i < end; ++i) Row(count++, detailLines[i], Kind::Text);
            if (detailsFirst) Row(count++, model.Text("menu.previous"), Kind::Previous);
            if (end < detailLines.size()) Row(count++, model.Text("menu.next"), Kind::Next);
            Row(count++, model.Text("menu.back"), Kind::Back);
        }
        if (!count) Row(count++, model.Text("menu.back"), Kind::Back);
        Publish(object, count, title, selection);
    }

    inline void Back(void* object)
    {
        if (view == View::Details) { view = View::Settings; Populate(object, detailsReturnSelection); }
        else if (view == View::Settings) { view = View::Plugins; Populate(object); }
        else Root(object);
    }

    inline void Activate(void* object, const Item& item)
    {
        switch (item.kind)
        {
        case Kind::Plugin:
            if (view == View::Root)
            {
                model.OpenPlugins(module, Language(), 4, {}, true);
                RememberRestartValues();
                pluginFirst = 0; view = View::Plugins;
            }
            else if (item.index < model.Pages().size())
            {
                currentPage = item.index; first = 0; hasLastRow = false;
                model.PageShown(currentPage); view = View::Settings;
            }
            Populate(object); break;
        case Kind::Setting:
            lastRow = item.row; hasLastRow = true; model.Change(item.row, 0, false);
            if (currentObject == object && model.IsOpen()) Populate(object, *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 0x2c0));
            break;
        case Kind::Previous:
            if (view == View::Plugins) pluginFirst = pluginFirst >= PluginPageSize() ? pluginFirst - PluginPageSize() : 0;
            else if (view == View::Settings) first = first >= 2 ? first - 2 : 0;
            else if (view == View::Details) detailsFirst = detailsFirst >= 5 ? detailsFirst - 5 : 0;
            Populate(object); break;
        case Kind::Next:
            if (view == View::Plugins) pluginFirst += PluginPageSize();
            else if (view == View::Settings) first += 2;
            else if (view == View::Details) detailsFirst += 5;
            Populate(object); break;
        case Kind::Details:
            lastRow = item.row; hasLastRow = true;
            detailsReturnSelection = *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 0x2c0);
            detailLines = Wrap(model.Help(lastRow)); detailsFirst = 0; view = View::Details; Populate(object);
            break;
        case Kind::Back: Back(object); break;
        default: break;
        }
    }

    inline void __fastcall Options(void* object, void*)
    {
        auto bytes = static_cast<uint8_t*>(object);
        if (currentObject != object)
        {
            currentObject = object; view = View::Root; model.Close();
        }
        const auto state = *reinterpret_cast<int*>(bytes + 0x260);
        const auto selection = *reinterpret_cast<int*>(bytes + 0x2c0);
        const auto action = *reinterpret_cast<int*>(bytes + 0x2c4);
        if (view == View::Plugins && state == 2)
        {
            RWGTA::UpdateCheck::Tick();
            if (renderedNotice != UpdateVisible()) Populate(object, selection);
        }
        if (view != View::Root && state == 5)
        {
            // The game's default action performs its normal transition back to the active list.
            *reinterpret_cast<int*>(bytes + 0x260) = 4;
            *reinterpret_cast<int*>(bytes + 0x2c4) = ActionBase;
            optionsHook.unsafe_thiscall(object);
            Back(object);
            return;
        }
        if (view == View::Settings && state == 2 && selection >= 0 && selection < static_cast<int>(Capacity))
        {
            const auto item = items[selection];
            if (item.kind == Kind::Setting) { lastRow = item.row; hasLastRow = true; }
            auto table = *reinterpret_cast<uintptr_t**>(object);
            const auto repeated = reinterpret_cast<uint32_t(__thiscall*)(void*)>(table[0xe4 / 4])(object);
            const int direction = repeated & 0x20020 ? -1 : repeated & 0x80080 ? 1 : 0;
            if (direction && item.kind == Kind::Setting)
            {
                model.Change(item.row, direction, false);
                if (currentObject != object || !model.IsOpen()) return;
                Populate(object, selection);
            }
        }
        optionsHook.unsafe_thiscall(object);
        if (state == 4 && action >= ActionBase && action < ActionBase + static_cast<int>(Capacity))
            Activate(object, items[action - ActionBase]);
    }

    inline int __fastcall Destroy(void* object, void*)
    {
        if (currentObject == object)
        {
            currentObject = nullptr;
            ResetTable();
        }
        return destroyHook.unsafe_thiscall<int>(object);
    }

    inline void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    inline void Init()
    {
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &module);
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.try_emplace({module, setting.id}, FixSettings().Wanted(setting.id));
        auto index = hook::pattern("39 90 ? ? ? ? 0F 8E ? ? ? ? 55 33 ED 56 8D B7 00 04 00 00");
        typeTable = *index.get_first<uint32_t*>(2) - 1 + 14 * 5;
        for (size_t i = 0; i < 5; ++i) originalType[i] = typeTable[i];
        auto oldRows = reinterpret_cast<Definition*>(typeTable[0]);
        for (size_t i = 0; i < 7; ++i) originalRows[i] = oldRows[i];
        for (size_t i = 0; i < 7; ++i) rows[i] = originalRows[i];
        Row(7, "Plugins", Kind::Plugin);
        build = reinterpret_cast<decltype(build)>(hook::pattern("83 EC 10 53 8A 5C 24 1C 33 D2 57 8B F9 89 54 24 0C 84 DB").get_first());
        refresh = reinterpret_cast<decltype(refresh)>(hook::pattern("A1 ? ? ? ? 83 EC 10 05 E0 16 00 00 53 55 56 57 50 8B F9").get_first());
        auto visibility = hook::pattern("8D 46 0E 50 E8 ? ? ? ? 6A 00 8B C8 E8 ? ? ? ? 46 83 FE 0A 7C ? 6A 00 6A 22").get_first<uint8_t>();
        findWidget = reinterpret_cast<decltype(findWidget)>(injector::GetBranchDestination(visibility + 4).as_int());
        showWidget = reinterpret_cast<decltype(showWidget)>(injector::GetBranchDestination(visibility + 13).as_int());
        auto select = hook::pattern("FF 34 85 ? ? ? ? 8B CF E8 ? ? ? ? 85 C0 74 ? 6A 04 8B C8 E8 ? ? ? ? 8B 87 40 04 00 00").get_first<uint8_t>();
        animateWidget = reinterpret_cast<decltype(animateWidget)>(injector::GetBranchDestination(select + 22).as_int());
        auto messageCall = hook::pattern("E8 ? ? ? ? 8B D0 85 D2 75 13 8B 07 8B CF FF 50 60 56 FF 15 ? ? ? ? 5E 5F C2 0C 00 8B CA 53 8D 59 01 8D A4 24");
        messageHook = safetyhook::create_inline(injector::GetBranchDestination(messageCall.get_first()).as_int(), Message);
        optionsHook = safetyhook::create_inline(hook::pattern("83 EC 08 57 8B F9 E8 ? ? ? ? 8B 87 60 02 00 00 83 F8 05 0F 87").get_first(), Options);
        destroyHook = safetyhook::create_inline(hook::pattern("56 8B F1 C7 06 ? ? ? ? E8 ? ? ? ? 8B CE E8 ? ? ? ? 80 BE FA 03 00 00 00").get_first(), Destroy);
        injector::WriteMemory<uint32_t>(typeTable, reinterpret_cast<uint32_t>(rows.data()), true);
        injector::WriteMemory<uint32_t>(typeTable + 1, 8, true);
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config updates;
        updates.pluginId = "ResidentEvilRevelations2.FusionFix";
        updates.fileVersion = rsc_FileVersion;
        updates.settings = &FixSettings();
        updates.playable = [] { return currentObject && view == View::Plugins; };
        updates.show = [](const std::wstring& notice) { updateNotice = asi_settings::text::Narrow(notice); };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}
