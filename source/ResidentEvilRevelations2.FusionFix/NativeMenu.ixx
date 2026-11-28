module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RainDroplets.hpp"
#include "Shared/MenuList.hpp"
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
    enum class View { Root, List, Details };
    enum class Kind { None, Plugin, Open, Setting, Details, Back, Text };
    struct Item { Kind kind{}; const RWGTAMenu::Row* row{}; };

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
    inline size_t currentPage{}, detailsFirst{}, listShown{};
    inline int detailsReturnSelection{};
    inline const RWGTAMenu::Row* lastRow{};
    inline std::vector<std::string> detailLines;
    inline std::string updateNotice;
    inline bool renderedNotice{};

    inline bool UpdateVisible() { return !updateNotice.empty() && FixSettings().Get("MAIN.CheckForUpdates") == "1"; }
    // Lines of a list the Options list shows at once: its eight rows less Back, Details and the restart warning on a
    // page of settings, less the update notice on the plugins list. The list scrolls through them.
    constexpr size_t ListSlots = 5;

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
        const auto language = FixSettings().Get("MAIN.TextLanguage");
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

    // help: the line the game shows in its footer for the selected row (none keeps the footer empty)
    inline void Row(size_t slot, const std::string& label, Kind kind, const RWGTAMenu::Row* row = nullptr, const std::string& help = {})
    {
        rows[slot] = originalRows[0];
        rows[slot].action = ActionBase + static_cast<int>(slot);
        rows[slot].file = CustomFile;
        rows[slot].text = TextId(label);
        rows[slot].value = 0;
        rows[slot].helpFile = CustomFile;
        rows[slot].helpText = TextId(help.empty() ? " " : help);
        rows[slot].disabledFile = -1;
        rows[slot].disabledText = -1;
        items[slot] = { kind, row };
    }

    // The row added to the game's Options list, in the menu language
    inline void PluginsRow()
    {
        auto texts = FixSettings().GetTexts();
        texts.Select(Language());
        Row(7, texts.Resolve("@menu.plugins"), Kind::Plugin, nullptr, texts.Resolve("@menu.pluginshelp"));
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
        PluginsRow();
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

    inline MenuList::Window& List()
    {
        return model.Window(currentPage);
    }

    // The page: the list's window, then what acts on the whole page; selection -1 keeps the list's selected row
    inline void Populate(void* object, int selection = -1)
    {
        items = {};
        size_t count = 0;
        std::string title = model.Text("menu.plugins");
        listShown = 0;
        if (view == View::List && currentPage < model.Pages().size())
        {
            title = model.Pages()[currentPage].title;
            for (auto row : model.Shown(currentPage, false))
            {
                auto label = model.Label(*row), value = model.Value(*row);
                if (!value.empty()) label += ": " + value;
                const bool opens = RWGTAMenu::Model::Opens(*row);
                const auto help = opens ? std::vector<std::string>{} : Wrap(model.Help(*row), 80);
                Row(count++, label, opens ? Kind::Open : Kind::Setting, row,
                    row->kind == RWGTAMenu::RowKind::Plugin ? model.Text("menu.pluginshelp") :
                    help.empty() ? std::string{} : help.size() > 1 ? help.front() + "..." : help.front());
            }
            listShown = count;
            if (selection < 0) selection = static_cast<int>(List().Slot());
            const auto* selected = static_cast<size_t>(selection) < count ? items[selection].row : lastRow;
            if (selected && !RWGTAMenu::Model::Opens(*selected) && !model.Help(*selected).empty())
            {
                lastRow = selected;
                Row(count++, model.Text("menu.details"), Kind::Details, selected, Wrap(model.Help(*selected), 80).front());
            }
            if (currentPage == RWGTAMenu::Model::RootPage)
            {
                renderedNotice = UpdateVisible();
                if (renderedNotice)
                {
                    const auto lines = Wrap(updateNotice);
                    for (size_t i = 0; i < (std::min)(size_t(2), lines.size()); ++i) Row(count++, lines[i], Kind::Text);
                }
            }
            if (RestartRequired()) Row(count++, model.Text("menu.restartwarning"), Kind::Text);
            Row(count++, model.Text("menu.back"), Kind::Back);
        }
        else if (view == View::Details)
        {
            title = lastRow ? model.Label(*lastRow) : model.Text("menu.details");
            detailsFirst = (std::min)(detailsFirst, detailLines.size());
            const auto end = (std::min)(detailLines.size(), detailsFirst + 7);
            for (size_t i = detailsFirst; i < end; ++i) Row(count++, detailLines[i], Kind::Text);
            Row(count++, model.Text("menu.back"), Kind::Back);
        }
        if (!count) Row(count++, model.Text("menu.back"), Kind::Back);
        Publish(object, count, title, (std::max)(selection, 0));
    }

    // A list's window follows the game's own selection: a step off the last or first shown row scrolls the list by
    // one, a wrap from Back to the top shows the list's top, and the row the selection lands on is the selected row
    inline bool Follow(void* object, int previous, int selection)
    {
        if (view != View::List || previous == selection) return false;
        auto& window = List();
        const int shown = static_cast<int>(listShown);
        int count = 0;
        while (count < static_cast<int>(Capacity) && items[count].kind != Kind::None) ++count;
        if (previous == shown - 1 && selection == shown && window.Below())
            window.Move(1);
        else if (previous == 0 && selection == count - 1 && window.Above())
            window.Move(-1);
        else if (previous == count - 1 && selection == 0 && window.Above())
            window.Edge(-1);
        else if (selection < shown)
        {
            window.Point(static_cast<size_t>(selection));
            return false;
        }
        else return false;
        Populate(object);
        return true;
    }

    inline void Back(void* object)
    {
        if (view == View::Details) { view = View::List; Populate(object, detailsReturnSelection); return; }
        if (view == View::List)
        {
            const auto parent = model.Back(currentPage);
            if (parent != SIZE_MAX)
            {
                currentPage = parent;
                Populate(object);
                return;
            }
        }
        Root(object);
    }

    inline void Activate(void* object, const Item& item)
    {
        switch (item.kind)
        {
        case Kind::Plugin:
            model.OpenMenu(module, Language(), ListSlots);
            RememberRestartValues();
            currentPage = RWGTAMenu::Model::RootPage;
            List().Reset();
            lastRow = nullptr;
            view = View::List;
            Populate(object);
            break;
        case Kind::Open:
        {
            const auto page = model.Enter(*item.row);
            if (page == SIZE_MAX) break;
            currentPage = page;
            lastRow = nullptr;
            Populate(object);
            break;
        }
        case Kind::Setting:
            lastRow = item.row; model.Change(*item.row, 0, false);
            if (currentObject == object && model.IsOpen()) Populate(object, *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 0x2c0));
            break;
        case Kind::Details:
            lastRow = item.row;
            detailsReturnSelection = *reinterpret_cast<int*>(static_cast<uint8_t*>(object) + 0x2c0);
            detailLines = Wrap(model.Help(*lastRow)); detailsFirst = 0; view = View::Details; Populate(object, 0);
            break;
        case Kind::Back: Back(object); break;
        default: break;
        }
    }

    inline void __fastcall Options(void* object, void*)
    {
        auto bytes = static_cast<uint8_t*>(object);
        // The Options menu is on the screen from its opening (0) to its closing (5)
        RainDroplets::MenuOpen(*reinterpret_cast<int*>(bytes + 0x260) != 5);
        if (currentObject != object)
        {
            currentObject = object; view = View::Root; model.Close();
        }
        const auto state = *reinterpret_cast<int*>(bytes + 0x260);
        const auto selection = *reinterpret_cast<int*>(bytes + 0x2c0);
        const auto action = *reinterpret_cast<int*>(bytes + 0x2c4);
        if (view == View::List && currentPage == RWGTAMenu::Model::RootPage && state == 2)
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
        if (view == View::List && state == 2 && selection >= 0 && selection < static_cast<int>(Capacity))
        {
            const auto item = items[selection];
            auto table = *reinterpret_cast<uintptr_t**>(object);
            const auto repeated = reinterpret_cast<uint32_t(__thiscall*)(void*)>(table[0xe4 / 4])(object);
            const int direction = repeated & 0x20020 ? -1 : repeated & 0x80080 ? 1 : 0;
            if (direction && item.kind == Kind::Setting)
            {
                model.Change(*item.row, direction, false);
                if (currentObject != object || !model.IsOpen()) return;
                Populate(object, selection);
            }
        }
        optionsHook.unsafe_thiscall(object);
        if (state == 2 && view == View::List && currentObject == object)
        {
            const auto now = *reinterpret_cast<int*>(bytes + 0x2c0);
            if (Follow(object, selection, now)) return;
            // The Details row follows the selected setting
            if (now != selection && now >= 0 && now < static_cast<int>(listShown) && items[now].row != lastRow &&
                items[now].kind == Kind::Setting)
            {
                Populate(object, now);
                return;
            }
        }
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
        RainDroplets::MenuOpen(false);
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
        PluginsRow();
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
        updates.playable = [] { return currentObject && view == View::List && currentPage == RWGTAMenu::Model::RootPage; };
        updates.show = [](const std::wstring& notice) { updateNotice = asi_settings::text::Narrow(notice); };
        RWGTA::UpdateCheck::Init(std::move(updates));
    }
}
