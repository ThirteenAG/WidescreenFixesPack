#pragma once
#include "ControlsRows.hpp"
#include "MenuText.hpp"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace CTControls
{
    using CTSettings::Read;
    using CTSettings::Set;
    inline constexpr std::array<const char*, 3> IDs{ "MAIN.RawInputMouse", "MAIN.MouseSensitivity", "MAIN.RawInputMouseRawData" };
    inline const std::set<std::string> Settings{ IDs.begin(), IDs.end() };
    inline constexpr uint32_t PageID = 0x8F89F2D1;
    inline constexpr uint32_t FooterID = 0x37D0AEBB;
    inline constexpr std::array<uint32_t, 5> ListIDs{ 0x41823774, 0x502255d5, 0x8bd8a458, 0x12d1f5e2, 0x65d6c574 };
    inline HMODULE module = nullptr;
    inline std::string (*language)() = nullptr;
    inline std::wstring (*wide)(const std::string&) = nullptr;
    inline CTText::Metrics (*metrics)(void*) = nullptr;
    inline void (*log)(const std::string&) = nullptr;
    inline bool(__fastcall* select)(void*, void*, int16_t, char, char) = nullptr;
    inline SafetyHookInline rebuildHook, defaultsHook;
    inline bool ready = false;
    inline uint64_t nextSerial = 0;

    struct Host
    {
        void *controller = nullptr, *page = nullptr, *manager = nullptr, *footer = nullptr;
        std::array<void*, 5> lists{};
        std::array<void*, 3> widgets{};
        std::array<void*, 5> rowData{};
        std::array<void*, 3> keyData{};
        uint64_t serial = 0;
        int first = 0;
        bool published = false, attached = false, reset = false, restart = false;
        std::array<std::wstring, 3> values{}, names{};
        std::wstring help;
        std::string tag;
        RWGTAMenu::Model model;
    };
    inline std::map<void*, std::shared_ptr<Host>> hosts;
    struct Action
    {
        void* page;
        uint64_t serial;
        size_t row;
        int direction;
    };
    inline std::deque<Action> pending;

    inline bool Known(const Host& host)
    {
        return host.controller && host.page && host.manager && Read<void*>(host.controller, 56) == host.page &&
               Read<void*>(host.controller, 20) == host.manager && Read<uint32_t>(host.page, 20) == PageID;
    }
    inline bool SourceCurrent(const Host& host)
    {
        if (!Known(host))
            return false;
        for (size_t i = 0; i < 5; ++i)
            if (Read<void*>(host.controller, 124 + i * 4) != host.lists[i])
                return false;
        for (size_t i = 0; i < 3; ++i)
            if (Read<void*>(host.controller, 144 + i * 4) != host.widgets[i])
                return false;
        return true;
    }
    inline bool Live(const Host& host)
    {
        if (!Known(host) || !host.published)
            return false;
        for (size_t i = 0; i < 5; ++i)
            if (Read<void*>(host.controller, 124 + i * 4) != host.lists[i] || Read<void*>(host.lists[i], 108) != host.rowData[i])
                return false;
        for (size_t i = 0; i < 3; ++i)
            if (!CTSettings::Child(host.page, host.widgets[i]) || Read<uint32_t>(host.widgets[i], 4) != ListIDs[i + 2] ||
                Read<void*>(host.widgets[i], 8) != host.lists[i + 2] || CTControlRows::Count(host.lists[i + 2]) != host.first + 6 ||
                Read<int>(host.controller, 76 + i * 12) != host.first + 6 || Read<void*>(host.controller, 72 + i * 12) != host.keyData[i])
                return false;
        return host.footer && CTSettings::Child(host.page, host.footer) && Read<uint32_t>(host.footer, 4) == FooterID;
    }
    inline std::shared_ptr<Host> Lookup(void* page, uint64_t serial)
    {
        auto found = hosts.find(page);
        return found != hosts.end() && found->second->serial == serial && Known(*found->second) ? found->second : nullptr;
    }
    inline const RWGTAMenu::Row* Row(const Host& host, size_t index)
    {
        if (index >= IDs.size())
            return nullptr;
        for (const auto& row : host.model.JoinedRows())
            if (row.setting && row.setting->id == IDs[index])
                return &row;
        return nullptr;
    }
    inline int Selected(const Host& host, void* item)
    {
        if (!Live(host) || std::find(host.widgets.begin(), host.widgets.end(), item) == host.widgets.end())
            return -1;
        const auto selected = int(Read<int16_t>(Read<void*>(item, 8), 168)) - host.first;
        return selected >= 0 && selected < 6 && selected % 2 == 1 ? selected / 2 : -1;
    }
    inline void Queue(const Host& host, size_t row, int direction)
    {
        if (pending.size() < 16 && Live(host) && CTSettings::current(host.page, host.manager))
            pending.push_back({ host.page, host.serial, row, direction });
    }
    struct Callback
    {
        void** vtable = nullptr;
        uint32_t references = 3;
        uint8_t enabled = 1, padding[3]{};
        void* page = nullptr;
        uint64_t serial = 0;
        static void* __fastcall Destroy(Callback* self, void*, char flags)
        {
            if (flags & 1)
                delete self;
            return self;
        }
        static int __fastcall Type(Callback*, void*)
        {
            return 0;
        }
        static char __fastcall Notify(Callback*, void*, void*)
        {
            return 0;
        }
        static char __fastcall Event(Callback*, void*, void*, void*)
        {
            return 0;
        }
        static char __fastcall Accept(Callback* self, void*, void* item)
        {
            auto host = Lookup(self->page, self->serial);
            auto row = host ? Selected(*host, item) : -1;
            if (row < 0)
                return 0;
            Queue(*host, size_t(row), 0);
            return 1;
        }
    };
    inline std::array<void*, 14> callbackVtable{ reinterpret_cast<void*>(&Callback::Destroy), reinterpret_cast<void*>(&Callback::Type),
                                                 reinterpret_cast<void*>(&Callback::Notify),  reinterpret_cast<void*>(&Callback::Notify),
                                                 reinterpret_cast<void*>(&Callback::Accept),  reinterpret_cast<void*>(&Callback::Notify),
                                                 reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Event),
                                                 reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Event),
                                                 reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Event),
                                                 reinterpret_cast<void*>(&Callback::Notify),  reinterpret_cast<void*>(&Callback::Notify) };
    inline void Forget(void* page)
    {
        hosts.erase(page);
        std::erase_if(pending, [page](const Action& action) { return action.page == page; });
    }
    inline void ControllerRetired(void* controller)
    {
        std::vector<void*> pages;
        for (const auto& [page, host] : hosts)
            if (host->controller == controller)
                pages.push_back(page);
        for (auto page : pages)
            Forget(page);
    }
    inline void Rebuilt(void* controller)
    {
        auto page = Read<void*>(controller, 56);
        if (!page || Read<uint32_t>(page, 20) != PageID)
            return;
        auto found = hosts.find(page);
        if (found != hosts.end() && found->second->controller == controller)
        {
            found->second->published = false;
            found->second->values = {};
            return;
        }
        auto host = std::make_shared<Host>();
        host->controller = controller;
        host->page = page;
        host->manager = Read<void*>(controller, 20);
        host->serial = ++nextSerial;
        hosts[page] = std::move(host);
    }
    inline int __fastcall Rebuild(void* controller, void*)
    {
        auto result = rebuildHook.thiscall<int>(controller);
        Rebuilt(controller);
        return result;
    }
    inline int __fastcall Defaults(void* controller, void*)
    {
        auto page = Read<void*>(controller, 56);
        auto found = hosts.find(page);
        auto host = found != hosts.end() ? found->second : nullptr;
        auto joined = host && Live(*host);
        auto result = defaultsHook.thiscall<int>(controller);
        if (joined && Lookup(page, host->serial))
            host->reset = true;
        return result;
    }
    inline void* Footer(void* list)
    {
        auto prototype = Read<void*>(list, 44);
        uintptr_t begin = 0;
        size_t count = 0;
        if (!prototype || !CTLayout::Vector(prototype, 8, 32, begin, count))
            return nullptr;
        for (size_t i = 0; i < count; ++i)
        {
            auto child = reinterpret_cast<void**>(begin)[i];
            if (CTSettings::Kind(child) != 8)
                continue;
            auto sourcePoint = Read<void*>(Read<void*>(child, 8), 4);
            if (!sourcePoint)
                return nullptr;
            const auto width = std::max(100, int(Read<int16_t>(sourcePoint, 10)) + 5);
            auto item = CTLayout::CloneItem(child, width, width, false);
            if (!item)
                return nullptr;
            auto body = Read<void*>(item, 8), point = Read<void*>(body, 4);
            Set<uint32_t>(item, 4, FooterID);
            Set<int16_t>(point, 8, 70);
            Set<int16_t>(point, 10, 570);
            Set<int16_t>(point, 12, 458);
            Set<int16_t>(point, 14, 478);
            Set<uint32_t>(body, 84, Read<uint32_t>(body, 84) & ~7u);
            Set<uint8_t>(item, 24, uint8_t(Read<uint8_t>(item, 24) & ~1u));
            return item;
        }
        return nullptr;
    }
    inline std::wstring Text(const std::string& text, CTText::Metrics measurement, const std::string& tag, bool shorten)
    {
        // Key-binding labels and hints use mixed case, unlike the category tabs.
        auto result = CTText::Style(wide(text), measurement, wide(tag).c_str(), false);
        return shorten                                            ? CTText::Fit(std::move(result), measurement, measurement.width)
               : measurement.Measure(result) <= measurement.width ? result
                                                                  : std::wstring{};
    }
    inline std::string Label(const Host& host, const RWGTAMenu::Row& row, size_t index)
    {
        return index == 1 ? host.model.Text("menu.rawsensitivity.label") : host.model.Label(row);
    }
    inline bool Join(const std::shared_ptr<Host>& host)
    {
        if (!Known(*host) || !CTSettings::current(host->page, host->manager))
            return false;
        if (host->published)
            return Live(*host);
        for (size_t i = 0; i < 5; ++i)
            host->lists[i] = Read<void*>(host->controller, 124 + i * 4);
        for (size_t i = 0; i < 3; ++i)
            host->widgets[i] = Read<void*>(host->controller, 144 + i * 4);
        for (size_t i = 0; i < 5; ++i)
        {
            auto item = CTSettings::findChild(host->page, nullptr, ListIDs[i]);
            if (CTSettings::Kind(item) != 10 || Read<void*>(item, 8) != host->lists[i])
                return false;
            if (i < 2)
                continue;
            if (item != host->widgets[i - 2])
                return false;
            uintptr_t callbacks = 0;
            size_t count = 0;
            auto primary = Read<void*>(host->controller, 4);
            if (!primary || !CTLayout::Vector(item, 48, 128, callbacks, count) || !count ||
                std::find(reinterpret_cast<void**>(callbacks), reinterpret_cast<void**>(callbacks) + count, primary) ==
                    reinterpret_cast<void**>(callbacks) + count)
                return false;
        }
        const auto first = CTControlRows::Count(host->lists[2]);
        if (first < 2 || first > 128)
            return false;
        int headers = 0;
        auto source = Read<uint8_t*>(host->lists[2], 108);
        for (int i = 0; i < first; ++i)
            headers += source[i * 60 + 53] != 0;
        for (size_t i = 0; i < 3; ++i)
            if (!CTSettings::Child(host->page, host->widgets[i]) || Read<void*>(host->widgets[i], 8) != host->lists[i + 2] ||
                CTControlRows::Count(host->lists[i + 2]) != first || Read<int>(host->controller, 76 + i * 12) != first)
                return false;
        if (headers * 2 != first + 5 || CTControlRows::Count(host->lists[0]) != first || CTControlRows::Count(host->lists[1]) != first)
            return false;
        auto tag = language();
        if (host->tag != tag || host->model.JoinedRows().empty())
        {
            host->model.OpenPlugins(module, tag, 4096, Settings);
            if (!Lookup(host->page, host->serial) || !SourceCurrent(*host))
                return false;
            host->tag = tag;
        }
        std::array<std::wstring, 3> names{}, shown{};
        for (size_t i = 0; i < 3; ++i)
        {
            auto row = Row(*host, i);
            if (!row)
                return false;
            auto label = Label(*host, *row, i), value = host->model.Value(*row);
            if (!Lookup(host->page, host->serial) || !SourceCurrent(*host))
                return false;
            const auto labels = metrics(host->lists[1]), values = metrics(host->lists[2]);
            if (!labels.font || !values.font)
                return false;
            names[i] = Text(label, labels, tag, true);
            shown[i] = Text(value, values, tag, false);
            if (names[i].empty() || shown[i].empty())
                return false;
        }
        void* footer = host->footer;
        std::unique_ptr<void, decltype(&CTSettings::Destroy)> fresh(nullptr, CTSettings::Destroy);
        if (!footer)
        {
            if (CTSettings::findChild(host->page, nullptr, FooterID))
                return false;
            fresh.reset(Footer(host->lists[1]));
            footer = fresh.get();
            if (!footer)
                return false;
        }
        std::array<CTControlRows::Rows, 5> lists;
        std::array<CTControlRows::Integers, 3> arrays;
        std::array<CTControlRows::Pointers, 3> callbacks;
        CTControlRows::Pointers children;
        auto callback = std::unique_ptr<Callback>(host->attached ? nullptr : new (std::nothrow) Callback);
        if (!host->attached && !callback)
            return false;
        if (callback)
        {
            callback->vtable = callbackVtable.data();
            callback->page = host->page;
            callback->serial = host->serial;
        }
        for (size_t i = 0; i < 5; ++i)
            if (!lists[i].Prepare(host->lists[i], 6))
                return false;
        for (size_t i = 0; i < 3; ++i)
        {
            if (!arrays[i].Prepare(static_cast<uint8_t*>(host->controller) + 72 + i * 12, first))
                return false;
            if (callback && !callbacks[i].Prepare(host->widgets[i], 48, callback.get()))
                return false;
            if (!lists[0].Text(int(i * 2), L"", false) || !lists[0].Text(int(i * 2 + 1), L"", false) ||
                !lists[1].Text(int(i * 2), names[i], false) || !lists[1].Text(int(i * 2 + 1), L"", false) ||
                !lists[2].Text(int(i * 2), L"", true) || !lists[2].Text(int(i * 2 + 1), shown[i], false) ||
                !lists[3].Text(int(i * 2), L"", true) || !lists[3].Text(int(i * 2 + 1), L"", true) ||
                !lists[4].Text(int(i * 2), L"", true) || !lists[4].Text(int(i * 2 + 1), L"", true))
                return false;
        }
        if (fresh && !children.Prepare(host->page, 8, footer))
            return false;
        if (!SourceCurrent(*host) || !CTSettings::current(host->page, host->manager))
            return false;
        for (auto& list : lists)
            if (!list.Current())
                return false;
        for (auto& array : arrays)
            if (!array.Current())
                return false;
        if (callback)
            for (auto& cb : callbacks)
                if (!cb.Current())
                    return false;
        if (fresh && !children.Current())
            return false;
        for (auto& list : lists)
            list.Publish();
        for (auto& array : arrays)
            array.Publish();
        if (callback)
        {
            for (auto& cb : callbacks)
                cb.Publish();
            callback.release();
            host->attached = true;
        }
        if (fresh)
        {
            children.Publish();
            host->footer = fresh.release();
        }
        host->first = first;
        host->values = std::move(shown);
        host->names = std::move(names);
        for (size_t i = 0; i < 5; ++i)
            host->rowData[i] = Read<void*>(host->lists[i], 108);
        for (size_t i = 0; i < 3; ++i)
            host->keyData[i] = Read<void*>(host->controller, 72 + i * 12);
        host->published = true;
        if (log)
            log("native Controls raw-input rows joined above mouse sensitivity");
        return true;
    }
    inline bool Key(void* list, void* item, const uint32_t* event)
    {
        if (!event)
            return false;
        for (const auto& [page, host] : hosts)
        {
            auto row = Selected(*host, item);
            if (row < 0 || Read<void*>(item, 8) != list || !CTSettings::current(page, host->manager))
                continue;
            if (*event == VK_DELETE || *event == VK_BACK)
                return true;
            if (*event == VK_TAB)
            {
                Queue(*host, 3, GetKeyState(VK_SHIFT) & 0x8000 ? -2 : 2);
                return true;
            }
            if (*event == VK_LEFT || *event == VK_RIGHT || *event == VK_SPACE)
            {
                Queue(*host, size_t(row), *event == VK_LEFT ? -1 : *event == VK_RIGHT ? 1 : 0);
                return true;
            }
        }
        return false;
    }
    inline void Refresh(const std::shared_ptr<Host>& host)
    {
        if (!Live(*host))
            return;
        auto tag = language();
        if (tag != host->tag)
        {
            host->model.OpenPlugins(module, tag, 4096, Settings);
            if (!Lookup(host->page, host->serial) || !Live(*host))
                return;
            host->tag = tag;
            host->names = {};
            host->values = {};
        }
        for (size_t i = 0; i < 3; ++i)
        {
            auto row = Row(*host, i);
            if (!row)
                continue;
            auto label = Label(*host, *row, i), value = host->model.Value(*row);
            if (!Lookup(host->page, host->serial) || !Live(*host))
                return;
            auto valueMetrics = metrics(host->lists[2]), labelMetrics = metrics(host->lists[1]);
            auto name = Text(label, labelMetrics, host->tag, true);
            auto text = Text(value, valueMetrics, host->tag, false);
            if (!name.empty() && name != host->names[i])
            {
                auto target = Read<uint8_t*>(host->lists[1], 108) + (host->first + i * 2) * 60;
                if (CTControlRows::Assign(target, name))
                    host->names[i] = std::move(name);
            }
            if (!text.empty() && text != host->values[i])
            {
                auto target = Read<uint8_t*>(host->lists[2], 108) + (host->first + i * 2 + 1) * 60;
                if (CTControlRows::Assign(target, text))
                    host->values[i] = std::move(text);
            }
        }
        auto selected = Selected(*host, Read<void*>(host->page, 36));
        auto body = Read<void*>(host->footer, 8);
        std::wstring help;
        if (selected >= 0)
        {
            auto row = Row(*host, size_t(selected));
            if (!row)
                return;
            auto footerMetrics = metrics(host->lists[1]);
            footerMetrics.width = 490;
            help = Text(host->restart || host->model.RestartRequired() ? host->model.Text("menu.restartwarning") : host->model.Help(*row),
                        footerMetrics, host->tag, true);
        }
        if (!Lookup(host->page, host->serial) || !Live(*host))
            return;
        if (help != host->help && CTControlRows::Assign(static_cast<uint8_t*>(body) + 56, help))
            host->help = std::move(help);
        Set<uint8_t>(host->footer, 24, uint8_t((Read<uint8_t>(host->footer, 24) & ~1u) | (!host->help.empty() ? 1u : 0u)));
    }
    inline void Tick(void* page)
    {
        auto found = hosts.find(page);
        if (found == hosts.end())
        {
            pending.clear();
            return;
        }
        auto host = found->second;
        if (!Known(*host) || !CTSettings::current(page, host->manager))
        {
            pending.clear();
            return;
        }
        if (host->reset)
        {
            host->reset = false;
            host->restart |= RWGTAMenu::RestoreDefaults(FixSettings(), Settings);
            if (!Lookup(page, host->serial) || !Known(*host))
                return;
        }
        if (!Join(host))
        {
            pending.clear();
            return;
        }
        auto actions = std::move(pending);
        pending.clear();
        for (const auto& action : actions)
        {
            if (action.page != page || action.serial != host->serial || !Live(*host) || !CTSettings::current(page, host->manager))
                continue;
            if (action.row == 3)
            {
                if (action.direction < 0)
                {
                    select(host->lists[2], nullptr, int16_t(host->first - 1), 1, 1);
                    CTSettings::setFocus(page, nullptr, host->widgets[0]);
                }
                else if (auto slider = CTSettings::findChild(page, nullptr, 0xe450494c))
                    CTSettings::setFocus(page, nullptr, slider);
                continue;
            }
            auto row = Row(*host, action.row);
            if (!row)
                continue;
            host->model.Change(*row, action.direction, false);
            if (!Lookup(page, host->serial) || !Live(*host))
                return;
        }
        Refresh(host);
    }
    inline bool Joined(void* page)
    {
        auto found = hosts.find(page);
        return found != hosts.end() && Live(*found->second);
    }
    inline bool JoinedFor(void* manager)
    {
        for (const auto& [page, host] : hosts)
            if (host->manager == manager && Live(*host))
                return true;
        return false;
    }
    inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
    {
        if (ready)
            return true;
        auto rebuild = hook::pattern(game, "56 8B F1 8B 4E 7C E8 35 BF 1C 00 8B 8E 80 00 00 00 E8 2A BF 1C 00 8B 8E 84 00 00 00 E8 1F BF "
                                           "1C 00 8B 8E 88 00 00 00 E8 14 BF 1C 00");
        auto defaults = hook::pattern(
            game,
            "83 EC 10 53 55 56 8B E9 57 89 6C 24 1C 33 F6 E8 4C 2D 17 00 8B F8 39 35 ? ? ? ? 75 17 68 ? ? ? ? E8 68 4B ED FF 83 C4 04 A3");
        if (rebuild.size() != 1 || defaults.size() != 1 || !module || !language || !wide || !metrics || !select)
            return false;
        rebuildHook = safetyhook::create_inline(rebuild.get_first(), Rebuild);
        if (!rebuildHook)
            return false;
        defaultsHook = safetyhook::create_inline(defaults.get_first(), Defaults);
        if (!defaultsHook)
        {
            rebuildHook.reset();
            return false;
        }
        ready = true;
        return true;
    }
} // namespace CTControls
