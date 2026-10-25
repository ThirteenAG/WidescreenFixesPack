#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <memory>

namespace SufferingMenu
{
    using Object = uintptr_t;
    template<class T> T& Field(Object object, size_t offset)
    {
        return *reinterpret_cast<T*>(object + offset);
    }
    template<class T> void SetField(Object object, size_t offset, T value)
    {
        Field<T>(object, offset) = value;
    }
    inline void* find_pattern(HMODULE module, const char* bytes)
    {
        auto pattern = hook::module_pattern(module, bytes);
        return pattern.size() == 1 ? pattern.get_first() : nullptr;
    }
    struct Bindings
    {
        void*(__cdecl* allocate)(int, size_t){};
        Object(__thiscall* button)(Object, int){};
        Object(__thiscall* text)(Object, int){};
        int(__thiscall* setText)(Object, const wchar_t*){};
        int(__thiscall* addChild)(Object, Object){};
        int(__thiscall* select)(Object, int){};
    };
    inline Bindings api;
    inline HMODULE host{};
    inline const uint32_t* languageCode{};
    inline std::string Language()
    {
        switch (languageCode ? *languageCode : 0)
        {
            case 0x5247: return "de";
            case 0x5053: return "es";
            case 0x5246: return "fr";
            case 0x5449: return "it";
            default: return "en";
        }
    }
    inline SafetyHookInline optionsHook, videoHook, defaultsHook;
    enum class Kind
    {
        Plugin,
        Setting,
        Back,
        Previous,
        Next
    };
    struct Item
    {
        Kind kind{};
        size_t index{};
        RWGTAMenu::Row row{};
        std::string label;
    };
    struct Page;
    struct Listener
    {
        void** vtable{};
        Page* page{};
    };
    struct Page
    {
        Object root{}, entry{}, help{}, title{};
        uintptr_t* originalTable{};
        std::array<uintptr_t, 29> table{};
        Listener listener;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<Object> controls, privateChildren;
        std::vector<std::pair<Object, int>> stockStates;
        Object stockChildren{};
        int stockCapacity{}, stockCount{}, stockSelection{};
        int page{ -1 }, command{ -1 }, direction{};
        size_t start{}, selected{};
        bool open{}, video{};
        std::string shownHelp;
        std::vector<std::pair<Object, RWGTAMenu::Row>> joined;
    };
    inline std::map<Object, std::unique_ptr<Page>> pages;
    inline std::wstring FontText(std::string text, bool label = true)
    {
        if (label && Language() == "en")
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
        int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        std::wstring wide(length, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), wide.data(), length);
        return wide;
    }
    inline std::string Help(const RWGTAMenu::Model& model, const RWGTAMenu::Row& row)
    {
        std::string text;
        if (row.plugin && row.plugin->module == host && row.setting && row.setting->id.starts_with("MAIN."))
            text = OtherPlugins::Text(*row.plugin, "@menu." + row.setting->id.substr(5) + ".help");
        else
            text = model.Help(row);
        if (text.size() > 85)
        {
            auto end = text.rfind(' ', 82);
            text.resize(end == std::string::npos ? 82 : end);
            text += "...";
        }
        return text;
    }
    inline std::string Text(const char* key)
    {
        return RWGTAMenu::PluginText(FixSettings(), Language(), key);
    }
    inline void SetText(Object control, const std::string& text, bool label = true)
    {
        api.setText(control, FontText(text, label).c_str());
    }
    inline void Geometry(Object object, float x, float y, float width, float height)
    {
        SetField<int>(object, 48, 0);
        SetField<int>(object, 52, 0);
        SetField<float>(object, 56, x);
        SetField<float>(object, 60, y);
        SetField<float>(object, 64, width);
        SetField<float>(object, 68, height);
    }
    inline Object Create(Page& page, Object source, bool button)
    {
        auto memory = reinterpret_cast<Object>(api.allocate(0, button ? 0xC4 : 0xC0));
        if (!memory)
            return 0;
        auto object = (button ? api.button : api.text)(memory, Field<int>(source, 12));
        SetField<Object>(object, 96, reinterpret_cast<Object>(&page.listener));
        SetField<Object>(object, 100, page.root);
        SetField<int>(object, 36, 4);
        return object;
    }
    inline void Restore(Page& page)
    {
        if (!page.open)
            return;
        SetField<Object>(page.root, 80, page.stockChildren);
        SetField<int>(page.root, 84, page.stockCapacity);
        SetField<int>(page.root, 88, page.stockCount);
        for (auto& [control, state] : page.stockStates)
            SetField<int>(control, 36, state);
        page.stockStates.clear();
        SetField<int>(page.root, 8, -1);
        page.open = false;
        api.select(page.root, page.stockSelection);
        page.model.Close();
    }
    inline void Build(Page& page)
    {
        page.items.clear();
        if (page.page < 0)
        {
            auto& plugins = page.model.Pages();
            for (size_t i = page.start; i < plugins.size() && i < page.start + 5; ++i)
                page.items.push_back({ Kind::Plugin, i, {}, plugins[i].plugin && plugins[i].plugin->module == host ? "The Suffering" : plugins[i].title });
            if (page.start)
                page.items.push_back({ Kind::Previous, 0, {}, Text("menu.previous") });
            if (page.start + 5 < plugins.size())
                page.items.push_back({ Kind::Next, 0, {}, Text("menu.next") });
        }
        else
        {
            for (auto row : page.model.VisibleRows(page.page))
                page.items.push_back({ Kind::Setting, 0, *row, page.model.Label(*row) });
        }
        page.items.push_back({ Kind::Back, 0, {}, Text("menu.back") });
        const auto count = std::min(page.items.size(), page.controls.size());
        for (size_t i = 0; i < page.controls.size(); ++i)
        {
            SetField<int>(page.controls[i], 36, i < count ? 4 : 1);
            if (i >= count)
                continue;
            auto& item = page.items[i];
            auto label = item.label;
            if (item.kind == Kind::Setting)
            {
                auto value = page.model.Value(item.row);
                if (!value.empty())
                    label += ": " + value;
            }
            SetText(page.controls[i], label);
        }
        auto title = page.page < 0 ? Text("menu.plugins") : page.model.Pages()[page.page].title;
        if (page.page >= 0 && page.model.Pages()[page.page].plugin->module == host)
            title = "The Suffering";
        SetText(page.title, title);
        page.selected = std::min(page.selected, count - 1);
        SetField<int>(page.root, 8, -1);
        api.select(page.root, static_cast<int>(page.selected));
    }
    inline void Open(Page& page)
    {
        page.model.OpenPlugins(host, Language(), 5, { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
        page.stockChildren = Field<Object>(page.root, 80);
        page.stockCapacity = Field<int>(page.root, 84);
        page.stockCount = Field<int>(page.root, 88);
        page.stockSelection = Field<int>(page.root, 8);
        auto stock = reinterpret_cast<Object*>(page.stockChildren);
        for (int i = 0; i < page.stockCount; ++i)
        {
            page.stockStates.emplace_back(stock[i], Field<int>(stock[i], 36));
            SetField<int>(stock[i], 36, 1);
        }
        SetField<Object>(page.root, 80, reinterpret_cast<Object>(page.privateChildren.data()));
        SetField<int>(page.root, 84, static_cast<int>(page.privateChildren.size()));
        SetField<int>(page.root, 88, static_cast<int>(page.privateChildren.size()));
        page.open = true;
        page.page = -1;
        page.start = page.selected = 0;
        Build(page);
    }
    inline void Act(Page& page, int index, int direction)
    {
        if (!page.open)
        {
            if (!page.video)
                Open(page);
            else if (index >= 0 && static_cast<size_t>(index) < page.joined.size())
            {
                page.model.Change(page.joined[index].second, direction, true);
                auto& row = page.joined[index];
                SetText(row.first, page.model.Label(row.second) + ": " + page.model.Value(row.second));
            }
            return;
        }
        if (index < 0 || static_cast<size_t>(index) >= page.items.size())
            return;
        page.selected = index;
        auto item = page.items[index];
        switch (item.kind)
        {
            case Kind::Plugin:
                page.page = static_cast<int>(item.index);
                page.model.PageShown(item.index);
                page.selected = 0;
                break;
            case Kind::Setting:
                page.model.Change(item.row, direction, true);
                break;
            case Kind::Back:
                if (page.page < 0)
                {
                    Restore(page);
                    return;
                }
                page.page = -1;
                page.selected = 0;
                break;
            case Kind::Previous:
                page.start -= std::min<size_t>(5, page.start);
                page.selected = 0;
                break;
            case Kind::Next:
                page.start += 5;
                page.selected = 0;
                break;
        }
        Build(page);
    }
    inline int __fastcall Notify(Listener* listener, void*, Object control)
    {
        auto& page = *listener->page;
        if (page.open)
        {
            auto found = std::find(page.controls.begin(), page.controls.end(), control);
            if (found != page.controls.end())
                page.command = static_cast<int>(found - page.controls.begin());
        }
        else if (control == page.entry)
            page.command = 0;
        else
        {
            for (size_t i = 0; i < page.joined.size(); ++i)
                if (page.joined[i].first == control)
                    page.command = static_cast<int>(i);
        }
        page.direction = 0;
        return 0;
    }
    inline void* listenerTable[]{ nullptr, reinterpret_cast<void*>(Notify) };
    inline Object __fastcall Destroy(Object self, void*, int flags)
    {
        auto found = pages.find(self);
        if (found == pages.end())
            return 0;
        auto page = std::move(found->second);
        pages.erase(found);
        Restore(*page);
        SetField<uintptr_t*>(self, 0, page->originalTable);
        for (auto control : page->privateChildren)
            reinterpret_cast<Object(__thiscall*)(Object, int)>(Field<uintptr_t*>(control, 0)[0])(control, 1);
        return reinterpret_cast<Object(__thiscall*)(Object, int)>(page->originalTable[0])(self, flags);
    }
    inline int __fastcall Input(Object self, void*, int message)
    {
        auto& page = *pages.at(self);
        int index = Field<int>(self, 8);
        auto children = Field<Object*>(self, 80);
        Object selected = index >= 0 && index < Field<int>(self, 88) ? children[index] : 0;
        if (page.open)
        {
            if (message == 1021)
            {
                page.command = static_cast<int>(page.items.size() - 1);
                page.direction = 0;
                return 1;
            }
            if ((message == 1006 || message == 1007) && index >= 0 && static_cast<size_t>(index) < page.items.size()
                && page.items[index].kind == Kind::Setting)
            {
                page.command = index;
                page.direction = message == 1006 ? -1 : 1;
                return 1;
            }
        }
        else if (page.video)
        {
            for (size_t i = 0; i < page.joined.size(); ++i)
            {
                if (selected == page.joined[i].first && (message == 1020 || message == 1006 || message == 1007))
                {
                    page.command = static_cast<int>(i);
                    page.direction = message == 1006 ? -1 : message == 1007 ? 1 : 0;
                    return 1;
                }
            }
        }
        return reinterpret_cast<int(__thiscall*)(Object, int)>(page.originalTable[5])(self, message);
    }
    inline int __fastcall Update(Object self, void*)
    {
        auto& page = *pages.at(self);
        int result = reinterpret_cast<int(__thiscall*)(Object)>(page.originalTable[23])(self);
        if (page.command >= 0)
        {
            int command = std::exchange(page.command, -1);
            Act(page, command, page.direction);
        }
        if (page.video)
        {
            auto index = Field<int>(self, 8);
            auto children = Field<Object*>(self, 80);
            for (auto& [control, row] : page.joined)
                if (Field<int>(control, 36) == 5)
                    for (int i = 0; i < Field<int>(self, 88); ++i)
                        if (children[i] == control && index != i)
                        {
                            index = i;
                            SetField<int>(self, 8, i);
                        }
            auto selected = index >= 0 && index < Field<int>(self, 88) ? children[index] : 0;
            std::string help;
            for (auto& [control, row] : page.joined)
            {
                auto label = FontText(page.model.Label(row) + ": " + page.model.Value(row));
                auto text = Field<const wchar_t*>(control, 108);
                if (!text || label != text)
                    api.setText(control, label.c_str());
                if (selected == control)
                    help = Help(page.model, row);
            }
            if (!help.empty())
                for (size_t offset = 192; offset <= 216; offset += 4)
                    if (auto control = Field<Object>(self, offset))
                        SetField<int>(control, 36, 1);
            SetField<int>(page.help, 36, help.empty() ? 1 : 3);
            if (help != page.shownHelp)
            {
                SetText(page.help, help, false);
                page.shownHelp = std::move(help);
            }
        }
        if (page.open)
        {
            if (RWGTAMenu::Model::Changed().exchange(false))
                Build(page);
            int selected = Field<int>(self, 8);
            std::string help;
            if (selected >= 0 && static_cast<size_t>(selected) < page.items.size())
            {
                auto& item = page.items[selected];
                if (item.kind == Kind::Setting)
                    help = Help(page.model, item.row);
            }
            if (page.model.RestartRequired())
                help = Text("menu.restartwarning");
            if (help.size() > 100)
            {
                auto end = help.rfind(' ', 97);
                help.resize(end == std::string::npos ? 97 : end);
                help += "...";
            }
            if (help != page.shownHelp)
            {
                SetText(page.help, help, false);
                page.shownHelp = std::move(help);
            }
        }
        return result;
    }
    inline void Attach(Object root, bool video)
    {
        if (pages.count(root))
            return;
        auto page = std::make_unique<Page>();
        page->root = root;
        page->video = video;
        page->listener = { listenerTable, page.get() };
        auto source = Field<Object>(root, 120);
        if (!source)
            return;
        if (video)
        {
            page->model.OpenPlugins(host, Language(), 5, { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
            auto rows = page->model.JoinedRows();
            float x = Field<float>(root, 56) + Field<float>(root, 64) * 0.12f;
            float width = Field<float>(root, 64) * 0.76f;
            float height = Field<float>(source, 68);
            float top = Field<float>(source, 60);
            auto children = Field<Object*>(root, 80);
            for (int i = 0; i < Field<int>(root, 88); ++i)
            {
                float y = Field<float>(children[i], 60);
                if (y >= top && y < Field<float>(root, 68) * 0.72f)
                    SetField<float>(children[i], 60, top + (y - top) * 0.8f);
            }
            float y = top + height * (8.0f / 9.0f) * 0.8f * 6.0f;
            for (auto& row : rows)
            {
                auto control = Create(*page, source, true);
                if (!control)
                    continue;
                Geometry(control, x, y, width, height);
                SetText(control, page->model.Label(row) + ": " + page->model.Value(row));
                api.addChild(root, control);
                children = Field<Object*>(root, 80);
                auto accept = Field<Object>(root, 220);
                for (int i = 0; i < Field<int>(root, 88) - 1; ++i)
                    if (children[i] == accept)
                    {
                        for (int j = Field<int>(root, 88) - 1; j > i; --j)
                            children[j] = children[j - 1];
                        children[i] = control;
                        break;
                    }
                page->joined.emplace_back(control, row);
                y += height * (8.0f / 9.0f) * 0.8f;
            }
            auto sourceHelp = Field<Object>(root, 216);
            page->help = Create(*page, sourceHelp ? sourceHelp : source, false);
            if (!page->help)
                return;
            Geometry(page->help, Field<float>(root, 56) + Field<float>(root, 64) * 0.1f,
                Field<float>(root, 60) + Field<float>(root, 68) * 0.85f,
                Field<float>(root, 64) * 0.8f, Field<float>(root, 68) * 0.18f);
            SetField<int>(page->help, 36, 1);
            api.addChild(root, page->help);
        }
        else
        {
            page->entry = Create(*page, source, true);
            if (!page->entry)
                return;
            auto children = Field<Object*>(root, 80);
            auto back = Field<Object>(root, 148);
            auto previous = Field<Object>(root, 144);
            int backIndex = -1;
            for (int i = 0; i < Field<int>(root, 88); ++i)
                if (children[i] == back)
                    backIndex = i;
            Geometry(page->entry, Field<float>(source, 56),
                (Field<float>(previous, 60) + Field<float>(back, 60)) * 0.5f,
                Field<float>(source, 64), Field<float>(source, 68));
            SetText(page->entry, Text("menu.plugins"));
            api.addChild(root, page->entry);
            if (backIndex >= 0)
            {
                children = Field<Object*>(root, 80);
                for (int i = Field<int>(root, 88) - 1; i > backIndex; --i)
                    children[i] = children[i - 1];
                children[backIndex] = page->entry;
            }
            for (int i = 0; i < 8; ++i)
            {
                auto control = Create(*page, source, true);
                if (!control)
                    return;
                Geometry(control, Field<float>(root, 56) + Field<float>(root, 64) * 0.12f,
                    Field<float>(root, 60) + Field<float>(root, 68) * (0.18f + i * 0.075f),
                    Field<float>(root, 64) * 0.76f, Field<float>(source, 68));
                page->controls.push_back(control);
                page->privateChildren.push_back(control);
            }
            page->help = Create(*page, source, false);
            page->title = Create(*page, source, false);
            if (!page->help || !page->title)
                return;
            SetText(page->help, "", false);
            Geometry(page->help, Field<float>(root, 56) + Field<float>(root, 64) * 0.08f,
                Field<float>(root, 60) + Field<float>(root, 68) * 0.84f,
                Field<float>(root, 64) * 0.84f, Field<float>(root, 68) * 0.12f);
            Geometry(page->title, Field<float>(root, 56) + Field<float>(root, 64) * 0.08f,
                Field<float>(root, 60) + Field<float>(root, 68) * 0.06f,
                Field<float>(root, 64) * 0.84f, Field<float>(source, 68));
            page->privateChildren.push_back(page->help);
            page->privateChildren.push_back(page->title);
        }
        page->originalTable = Field<uintptr_t*>(root, 0);
        std::copy_n(page->originalTable, page->table.size(), page->table.begin());
        page->table[0] = reinterpret_cast<uintptr_t>(Destroy);
        page->table[5] = reinterpret_cast<uintptr_t>(Input);
        page->table[23] = reinterpret_cast<uintptr_t>(Update);
        SetField<uintptr_t*>(root, 0, page->table.data());
        pages.emplace(root, std::move(page));
    }
    inline Object __fastcall Options(Object self, void*, int object)
    {
        auto result = optionsHook.thiscall<Object>(self, object);
        Attach(self, false);
        return result;
    }
    inline Object __fastcall Video(Object self, void*, int object)
    {
        auto result = videoHook.thiscall<Object>(self, object);
        Attach(self, true);
        return result;
    }
    inline int __fastcall Defaults(Object self, void*)
    {
        RWGTAMenu::RestoreDefaults(FixSettings(), { "MAIN.FOVFactor", "MAIN.CutsceneBorders" });
        return defaultsHook.thiscall<int>(self);
    }
    inline void Init()
    {
        auto module = GetModuleHandleW(L"suffering.rfl");
        if (!module)
            return;
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<const wchar_t*>(Init), &host);
        auto language = find_pattern(GetModuleHandleW(nullptr), "8D 44 24 10 50 6A 23 55 89 15 ? ? ? ? FF D6 8A 54 24 0C 33 C9 80 FA 30");
        if (language)
            languageCode = *reinterpret_cast<const uint32_t**>(static_cast<uint8_t*>(language) + 10);
        api.allocate = reinterpret_cast<decltype(api.allocate)>(find_pattern(module, "81 EC 80 00 00 00 53 8B 1D ? ? ? ? 55 56 57 8B BC 24 98 00 00 00 33 ED"));
        api.button = reinterpret_cast<decltype(api.button)>(find_pattern(module, "8B 44 24 04 56 50 8B F1 E8 ? ? ? ? C7 06 ? ? ? ? C7 86 C0 00 00 00 00 00 00 00 8B C6"));
        api.text = reinterpret_cast<decltype(api.text)>(find_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 8B F1 50 89 74 24 0C"));
        api.setText = reinterpret_cast<decltype(api.setText)>(find_pattern(module, "53 55 8B 6C 24 0C 56 57 55 8B D9 FF 15 ? ? ? ? 8B F0 8B 43 6C 83 C4 04"));
        api.addChild = reinterpret_cast<decltype(api.addChild)>(find_pattern(module, "56 57 8B 7C 24 0C 8D 71 50 89 4F 64 8B 46 08 8B 4E 04 40 3B C8"));
        api.select = reinterpret_cast<decltype(api.select)>(find_pattern(module, "53 56 8B F1 8B 46 58 57 8B 7C 24 10 48 3B F8 7E 02 8B F8 85 FF 7D 02 33 FF"));
        auto options = find_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 57 8B F1 50 89 74 24 10 E8 ? ? ? ? C7 46 6C ? ? ? ? 8B 4E 0C 33 DB 3B CB");
        auto video = find_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 8B 44 24 14 53 56 57 8B F1 50 89 74 24 10 E8 ? ? ? ? C7 46 6C ? ? ? ? 33 DB C7 06");
        auto defaults = find_pattern(module, "B8 01 00 00 00 C7 81 24 01 00 00 20 03 00 00 C7 81 28 01 00 00 58 02 00 00");
        if (!api.allocate || !api.button || !api.text || !api.setText || !api.addChild || !api.select || !options || !video || !defaults)
        {
            OutputDebugStringA("The Suffering: native menu unavailable for this executable.\n");
            return;
        }
        HMODULE pinned{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<const wchar_t*>(module), &pinned))
            return;
        optionsHook = safetyhook::create_inline(options, Options);
        videoHook = safetyhook::create_inline(video, Video);
        defaultsHook = safetyhook::create_inline(defaults, Defaults);
    }
}
