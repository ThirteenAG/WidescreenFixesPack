#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include "NativeMenuContracts.hpp"
#include "NativeMenuABI.hpp"
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <limits>

namespace BlacklistMenu
{
    using Bridge = BlacklistMenuABI::Bridge;
    using Slot = BlacklistMenuABI::Slot;
    constexpr int32_t Advanced = 0x6000;
    constexpr int32_t RowCommand = 0x6100;
    constexpr size_t PageRows = 8;
    constexpr size_t RootPage = std::numeric_limits<size_t>::max();
    inline Bridge bridge;
    inline HMODULE self = nullptr;
    inline SafetyHookInline settingsHook, performHook, backHook, destroyHook, updateHook;
    inline bool ready = false, busy = false, draining = false;
    inline unsigned updateDepth = 0;
    inline void* currentManager = nullptr;
    inline asi_settings::DescribedFile hostFile;
    inline void Log(const std::string& message)
    {
        OutputDebugStringA(("Blacklist native menu: " + message + "\n").c_str());
    }

    struct PreserveBool
    {
        bool& target;
        bool previous;
        explicit PreserveBool(bool& value) : target(value), previous(value) { target = true; }
        ~PreserveBool() { target = previous; }
    };
    enum class Kind { Page, Previous, Next, Back, Model };
    struct Host;
    struct Binding
    {
        std::weak_ptr<Host> owner;
        Slot slot;
        size_t row = 0;
    };
    struct Item
    {
        int32_t command = 0;
        Kind kind = Kind::Back;
        size_t page = 0;
        RWGTAMenu::Row model;
        Binding binding;
        std::wstring label, value;
        bool enabled = true, cycler = false;
    };
    struct Host : std::enable_shared_from_this<Host>
    {
        void* scene = nullptr;
        void* tiered = nullptr;
        void* list = nullptr;
        void* manager = nullptr;
        int32_t advanced = Advanced;
        bool alive = true, open = false;
        uint64_t generation = 1;
        size_t page = RootPage, rootStart = 0, rowStart = 0;
        size_t renderedPage = RootPage, renderedStart = 0, renderedRowStart = 0;
        bool rendered = false;
        RWGTAMenu::Model model;
        std::array<Item, 14> items;
        size_t count = 0;
        Binding accept, selected;
    };
    inline std::map<void*, std::shared_ptr<Host>> hosts;
    // Native signal dispatchers touch Slot::emitting after our callback returns.
    // Keep invalidated C++ bindings until the complete manager update unwinds.
    inline std::vector<std::shared_ptr<Host>> retired;
    inline std::weak_ptr<Host> active;
    struct Pending
    {
        std::weak_ptr<Host> host;
        uint64_t generation;
        size_t page, rootStart, rowStart, row;
        int direction;
        bool open;
        uint32_t inputMask;
        int32_t gameMode;
    };
    inline std::deque<Pending> pending;
    inline bool Live(const Host& host)
    {
        auto it = hosts.find(host.scene);
        return host.alive && it != hosts.end() && it->second.get() == &host;
    }
    inline bool OwnsFocus(const Host& host, int32_t tier)
    {
        if (tier != 1 && tier != 12) return false;
        auto top = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
        if (!top) return false;
        if (top == host.scene || top == host.tiered) return true;
        auto lists = Bridge::Read<void**>(host.tiered, 612);
        if (!lists || Bridge::Read<int32_t>(host.tiered, 616) != 13) return false;
        auto list = lists[tier];
        if (!list) return false;
        if (top == list) return true;
        auto count = Bridge::Read<int32_t>(list, 644);
        if (count < 0 || count > 14) return false;
        for (int32_t i = 0; i < count; ++i)
        {
            auto widget = Bridge::Widget(list, i);
            // Native rows cache controls of multiple kinds. Focus can remain on
            // the previous kind's child while SetMaxVisibleItems replaces it.
            if (widget && (top == widget || top == bridge.Control(widget) ||
                Bridge::Read<void*>(top, 376) == widget)) return true;
        }
        return false;
    }
    struct Snapshot
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        bool open;
        int32_t tier;
        uint32_t inputMask;
        int32_t gameMode;
        bool requireFocus;
        bool rebuilding = false;
        void* previousFocus = nullptr;
        explicit Snapshot(Host& value) : host(value.shared_from_this()), generation(value.generation), open(value.open),
            tier(Bridge::Read<int32_t>(value.tiered, 592)), inputMask(Bridge::Read<uint32_t>(value.manager, 0x5A0)), gameMode(bridge.GameMode()),
            requireFocus(value.open) {}
        bool Owner() const { return Live(*host) && host->generation == generation && host->open == open; }
        const char* Failure() const
        {
            if (!Owner()) return "owner/generation";
            if (currentManager != host->manager) return "manager";
            if (Bridge::Read<int32_t>(host->tiered, 592) != tier) return "tier";
            if (Bridge::Read<uint32_t>(host->manager, 0x5A0) != inputMask) return "input mask";
            if (bridge.GameMode() != gameMode) return "game mode";
            if (tier == 12 && open && Bridge::Read<uint8_t>(host->list, 553) != 0) return "list transition";
            if (requireFocus && !OwnsFocus(*host, tier))
            {
                auto top = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
                if (!rebuilding || (top && top != previousFocus)) return "focus";
            }
            return nullptr;
        }
        bool Current() const { return Failure() == nullptr; }
    };
    inline std::wstring Wide(const std::string& value)
    {
        if (value.empty()) return {};
        auto length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0);
        if (!length) return L"?";
        std::wstring result(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), result.data(), length);
        return result;
    }
    inline std::string Language()
    {
        auto language = FixSettings().Get("MAIN.TextLanguage");
        if (language == "auto")
        {
            auto native = reinterpret_cast<const char*(__cdecl*)()>(bridge.currentLanguage)();
            language = native ? std::string(native, strnlen(native, 8)) : "en";
            std::transform(language.begin(), language.end(), language.begin(), [](unsigned char c) { return char(std::tolower(c)); });
            static const std::map<std::string, std::string> tags = {
                {"int", "en"}, {"eng", "en"}, {"fre", "fr"}, {"fra", "fr"},
                {"deu", "de"}, {"ger", "de"}, {"ita", "it"}, {"spa", "es"}, {"esp", "es"}
            };
            if (auto found = tags.find(language); found != tags.end()) language = found->second;
        }
        return language;
    }
    inline bool Queue(Host& host, size_t row, int direction, bool open = false)
    {
        if (!ready || busy || !Live(host) || pending.size() >= 64 || host.manager != currentManager ||
            Bridge::Read<int32_t>(host.tiered, 592) != (open ? 1 : 12) ||
            (!open && Bridge::Read<uint8_t>(host.list, 553) != 0) || !OwnsFocus(host, open ? 1 : 12)) return false;
        pending.push_back({ host.shared_from_this(), host.generation, host.page, host.rootStart, host.rowStart, row, direction, open,
            Bridge::Read<uint32_t>(host.manager, 0x5A0), bridge.GameMode() });
        return true;
    }
    inline void DisconnectRows(Host& host)
    {
        for (auto& item : host.items) bridge.Disconnect(item.binding.slot);
    }
    inline uint8_t __fastcall Changed(Binding* binding, void*, Slot*, int32_t value)
    {
        auto host = binding->owner.lock();
        if (host && Live(*host) && host->open && value != 0)
            Queue(*host, binding->row, value < 0 ? -1 : 1);
        return 1;
    }
    inline uint8_t __fastcall Accepted(Binding* binding, void*, Slot*, uint8_t back)
    {
        auto host = binding->owner.lock();
        if (!host || !Live(*host) || !host->open || busy) return 1;
        if (back) Queue(*host, host->count, 0);
        else
        {
            auto selected = Bridge::Read<int32_t>(host->list, 576);
            if (selected >= 0 && static_cast<size_t>(selected) < host->count)
                Queue(*host, static_cast<size_t>(selected), 0);
        }
        return 1;
    }
    inline uint8_t __fastcall Selected(Binding* binding, void*, Slot*, int32_t index)
    {
        auto host = binding->owner.lock();
        if (host && Live(*host) && host->open && Bridge::Read<int32_t>(host->tiered, 592) == 12 &&
            index >= 0 && static_cast<size_t>(index) < host->count)
        {
            Snapshot snapshot(*host);
            if (!snapshot.Current()) return 1;
            Bridge::Write<int32_t>(host->scene, 1052, index);
            if (!busy)
            {
                auto& item = host->items[index];
                auto hint = item.label;
                if (item.kind == Kind::Model)
                    hint = Wide(host->model.Help(item.model));
                if (!item.value.empty()) hint += L"  " + item.value;
                if (host->model.RestartRequired()) hint = Wide(host->model.Text("menu.restartwarning")) + L"  " + hint;
                if (snapshot.Current()) bridge.Caption(host->scene, 928, hint);
            }
        }
        return 1;
    }
    inline void SetBinding(Binding& binding, const std::shared_ptr<Host>& host, size_t row, void* callback)
    {
        binding.owner = host;
        binding.row = row;
        binding.slot.owner = &binding;
        binding.slot.callback = callback;
    }
    inline bool ItemRow(Host& host, Kind kind, size_t page, const RWGTAMenu::Row* model, const std::wstring& label)
    {
        Snapshot snapshot(host);
        if (host.count >= host.items.size()) return false;
        auto index = host.count++;
        auto& item = host.items[index];
        item.command = RowCommand + static_cast<int32_t>(index);
        item.kind = kind;
        item.page = page;
        if (model) item.model = *model;
        bool cycler = model && (model->kind == RWGTAMenu::RowKind::Setting || model->kind == RWGTAMenu::RowKind::Preset);
        bool enabled = true;
        std::string value;
        if (model)
        {
            enabled = host.model.Enabled(item.model, snapshot.gameMode != 11);
            if (!snapshot.Current()) return false;
            value = host.model.Value(item.model);
            if (!snapshot.Current()) return false;
        }
        auto display = label;
        if (!cycler && !value.empty()) display += L"  " + Wide(value);
        auto result = bridge.AddRow(host.list, display, &item.command, cycler ? 18 : 15);
        if (!snapshot.Current() || result <= 0)
        { Log("row creation refused at row " + std::to_string(index)); return false; }
        item.label = label;
        item.value = Wide(value);
        item.enabled = enabled;
        item.cycler = cycler;
        return true;
    }
    inline bool Configure(Host& host)
    {
        Snapshot snapshot(host);
        auto current = [&](size_t index, const char* stage)
        {
            auto failure = snapshot.Failure();
            if (!failure) return true;
            Log("row configure context lost at row " + std::to_string(index) + " after " + stage + ": " + failure);
            return false;
        };
        for (size_t index = 0; index < host.count; ++index)
        {
        auto& item = host.items[index];
        auto widget = Bridge::Widget(host.list, static_cast<int32_t>(index));
        auto record = bridge.Record(host.list, static_cast<int32_t>(index));
        if (!widget || !record || Bridge::Read<const int32_t*>(record, 64) != &item.command)
        { Log("row configure widget/record identity refused at row " + std::to_string(index)); return false; }
        if (item.kind != Kind::Model) continue;
        if (item.cycler)
        {
            auto control = bridge.Control(widget);
            if (!current(index, "control lookup")) return false;
            if (!control)
            { Log("row configure native cycler control unavailable at row " + std::to_string(index)); return false; }
            Bridge::Virtual<void>(control, 376, item.label.c_str());
            if (!current(index, "label")) return false;
            bridge.ChoiceClear(Bridge::At(control, 592));
            if (!current(index, "choice clear")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), L"-", -1);
            if (!current(index, "previous choice")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), item.value, 0);
            if (!current(index, "current choice")) return false;
            bridge.ChoiceAdd(Bridge::At(control, 592), L"+", 1);
            if (!current(index, "next choice")) return false;
            Bridge::Virtual<void>(control, 384, 1); // index of the current-value choice
            if (!current(index, "current value")) return false;
            SetBinding(item.binding, snapshot.host, index, reinterpret_cast<void*>(Changed));
            bridge.Connect(Bridge::At(control, 604), item.binding.slot, true);
            if (!current(index, "change callback binding")) return false;
        }
        // Native disabled styling and backing record follow ListBox::DisableItem.
        Bridge::Virtual<void>(widget, 352, item.enabled ? -1 : 3);
        if (!current(index, "enabled styling")) return false;
        Bridge::Write<int32_t>(record, 68, item.enabled ? -1 : 3);
        if (!item.enabled)
        {
            Bridge::Virtual<void>(widget, 304, 2);
            if (!current(index, "disabled styling")) return false;
        }
        }
        return true;
    }
    inline bool Populate(Host& host)
    {
        Item selectedItem;
        auto previousIndex = Bridge::Read<int32_t>(host.list, 576);
        bool preserveSelection = host.rendered && host.renderedPage == host.page && host.renderedStart == host.rootStart && host.renderedRowStart == host.rowStart &&
            previousIndex >= 0 && static_cast<size_t>(previousIndex) < host.count;
        if (preserveSelection) selectedItem = host.items[previousIndex];
        ++host.generation; // rows are rebound; old queued widget events expire
        Snapshot snapshot(host);
        PreserveBool guard(busy);
        DisconnectRows(host);
        if (!snapshot.Current()) return false;
        // Replacing the owned row pool temporarily retires its focused child.
        snapshot.previousFocus = reinterpret_cast<void*(__cdecl*)()>(bridge.topFocus)();
        snapshot.rebuilding = true;
        bridge.Clear(host.list);
        if (!snapshot.Current()) return false;
        host.count = 0;
        std::wstring title = Wide(host.model.Text("menu.plugins"));
        if (host.page == RootPage)
        {
            auto pages = host.model.Pages();
            auto end = std::min(pages.size(), host.rootStart + PageRows);
            for (size_t i = host.rootStart; i < end; ++i)
                if (!ItemRow(host, Kind::Page, i, nullptr, Wide(pages[i].title))) return false;
            if (host.rootStart && !ItemRow(host, Kind::Previous, 0, nullptr, Wide(host.model.Text("menu.previous")))) return false;
            if (end < pages.size() && !ItemRow(host, Kind::Next, 0, nullptr, Wide(host.model.Text("menu.next")))) return false;
        }
        else
        {
            auto rows = host.model.VisibleRows(host.page);
            if (!snapshot.Current()) return false;
            if (host.page >= host.model.Pages().size()) return false;
            title = Wide(host.model.Pages()[host.page].title);
            auto end = std::min(rows.size(), host.rowStart + PageRows);
            for (size_t i = host.rowStart; i < end; ++i)
            {
                if (!snapshot.Current() || !ItemRow(host, Kind::Model, 0, rows[i], Wide(host.model.Label(*rows[i])))) return false;
            }
            if (host.rowStart && !ItemRow(host, Kind::Previous, 0, nullptr, Wide(host.model.Text("menu.previous")))) return false;
            if (end < rows.size() && !ItemRow(host, Kind::Next, 0, nullptr, Wide(host.model.Text("menu.next")))) return false;
        }
        if (!ItemRow(host, Kind::Back, 0, nullptr, Wide(host.model.Text("menu.back")))) return false;
        // Native settings builders re-enable the list and allocate its row pool
        // through virtual SetMaxVisibleItems before touching cycler controls.
        Bridge::Write<uint8_t>(host.list, 696, 1);
        Bridge::Virtual<void>(host.list, 384, 14);
        if (auto failure = snapshot.Failure())
        { Log(std::string("row pool context lost before configure: ") + failure); return false; }
        if (!Configure(host) || !snapshot.Current()) return false;
        bridge.Caption(host.scene, 924, title);
        if (!snapshot.Current()) return false;
        bridge.Caption(host.scene, 928, Wide(host.model.Text(host.model.RestartRequired() ? "menu.restartwarning" : "menu.plugins")));
        if (!snapshot.Current()) return false;
        int32_t selectedIndex = 0;
        if (preserveSelection)
            for (size_t i = 0; i < host.count; ++i)
            {
                auto& item = host.items[i];
                if (item.kind == selectedItem.kind && item.page == selectedItem.page &&
                    (item.kind != Kind::Model || (item.model.kind == selectedItem.model.kind &&
                        item.model.plugin == selectedItem.model.plugin && item.model.setting == selectedItem.model.setting)))
                {
                    selectedIndex = static_cast<int32_t>(i);
                    break;
                }
            }
        // An inactive tier must not steal the stock root's focus before PushTier.
        reinterpret_cast<void(__thiscall*)(void*, int32_t, int32_t)>(bridge.selectRow)(host.list, selectedIndex, snapshot.tier == 12 ? 1 : 0);
        snapshot.rebuilding = false;
        if (!snapshot.Current()) return false;
        Bridge::Write<int32_t>(host.scene, 1052, Bridge::Read<int32_t>(host.list, 576));
        host.renderedPage = host.page;
        host.renderedStart = host.rootStart;
        host.renderedRowStart = host.rowStart;
        host.rendered = true;
        return true;
    }
    inline void Close(Host& host)
    {
        Snapshot snapshot(host);
        PreserveBool guard(busy);
        host.open = false;
        ++host.generation;
        snapshot.generation = host.generation;
        snapshot.open = false;
        host.model.Close();
        DisconnectRows(host);
        if (!snapshot.Current()) return;
        if (Bridge::Read<int32_t>(host.tiered, 592) == 12)
            bridge.PopTier(host.tiered);
        if (!snapshot.Owner()) return;
        if (Bridge::Read<int32_t>(host.tiered, 592) == 1)
        {
            auto lists = Bridge::Read<void**>(host.tiered, 612);
            if (lists && lists[1]) Bridge::Write<int32_t>(host.scene, 1052, Bridge::Read<int32_t>(lists[1], 576));
        }
        if (active.lock() == snapshot.host) active.reset();
    }
    struct OpenRollback
    {
        std::shared_ptr<Host> host;
        uint64_t generation;
        bool complete = false;
        ~OpenRollback()
        {
            if (!complete && Live(*host) && host->open && host->generation == generation) Close(*host);
        }
    };
    inline void Open(Host& host)
    {
        Snapshot snapshot(host);
        snapshot.requireFocus = true;
        PreserveBool guard(busy);
        auto old = active.lock();
        if (old && old.get() != &host) Close(*old);
        if (!snapshot.Current() || Bridge::Read<int32_t>(host.tiered, 592) != 1) return;
        host.model.OpenPlugins(self, Language(), 4096);
        if (!snapshot.Current())
        {
            if (snapshot.Owner())
            {
                if (host.open) Close(host);
                else host.model.Close();
            }
            return;
        }
        host.open = true;
        ++host.generation;
        snapshot.open = true;
        snapshot.generation = host.generation;
        OpenRollback rollback{ snapshot.host, host.generation };
        host.page = RootPage;
        host.rootStart = 0;
        host.rowStart = 0;
        host.rendered = false;
        active = snapshot.host;
        ++snapshot.generation; // Populate advances once before binding rows
        ++rollback.generation;
        if (!Populate(host) || !snapshot.Current()) return;
        // Native PushTier performs focus/blur, fade and the owned history.
        // It deliberately bypasses ScenePause's twelve-slot stock preparer.
        bridge.PushTier(host.tiered, 12);
        if (!snapshot.Owner() || Bridge::Read<int32_t>(host.tiered, 616) != 13 ||
            Bridge::Read<int32_t>(host.tiered, 592) != 12) return;
        snapshot.tier = 12;
        bridge.Caption(host.scene, 924, Wide(host.model.Text("menu.plugins")));
        rollback.complete = snapshot.Current();
    }
    inline void Activate(Host& host, size_t index, int direction)
    {
        Snapshot snapshot(host);
        if (!host.open || Bridge::Read<int32_t>(host.tiered, 592) != 12) return;
        if (index >= host.count || host.items[index].kind == Kind::Back)
        {
            if (direction != 0) return;
            if (host.page == RootPage) { Close(host); return; }
            host.model.PageShown(host.page);
            host.page = RootPage;
            host.rowStart = 0;
            ++host.generation;
        }
        else
        {
            auto item = host.items[index];
            if (direction != 0 && item.kind != Kind::Model) return;
            switch (item.kind)
            {
            case Kind::Page:
                host.page = item.page;
                host.rowStart = 0;
                host.model.PageShown(host.page);
                ++host.generation;
                break;
            case Kind::Previous:
                if (host.page == RootPage) host.rootStart = host.rootStart >= PageRows ? host.rootStart - PageRows : 0;
                else host.rowStart = host.rowStart >= PageRows ? host.rowStart - PageRows : 0;
                ++host.generation;
                break;
            case Kind::Next:
                if (host.page == RootPage) host.rootStart += PageRows;
                else host.rowStart += PageRows;
                ++host.generation;
                break;
            case Kind::Model:
                host.model.Change(item.model, direction, snapshot.gameMode != 11);
                if (!snapshot.Current()) return;
                break;
            default: break;
            }
        }
        snapshot.generation = host.generation; // only our navigation advances it
        if (snapshot.Current() && !Populate(host))
        {
            Log("row population refused; returning to stock Options");
            if (Live(host) && host.open) Close(host);
        }
    }
    inline void Drain()
    {
        if (draining) return;
        PreserveBool guard(draining);
        auto events = std::move(pending);
        pending.clear();
        for (auto& event : events)
        {
            auto host = event.host.lock();
            if (!host || !Live(*host) || host->generation != event.generation ||
                host->page != event.page || host->rootStart != event.rootStart || host->rowStart != event.rowStart || host->manager != currentManager ||
                Bridge::Read<uint32_t>(host->manager, 0x5A0) != event.inputMask || bridge.GameMode() != event.gameMode) continue;
            if (!OwnsFocus(*host, event.open ? 1 : 12)) continue;
            if (event.open) Open(*host);
            else Activate(*host, event.row, event.direction);
        }
        auto host = active.lock();
        if (host && Live(*host) && host->open && Bridge::Read<int32_t>(host->tiered, 592) == 12 &&
            Bridge::Read<uint8_t>(host->list, 553) == 0 && RWGTAMenu::Model::Changed().exchange(false))
        {
            if (OwnsFocus(*host, 12))
            {
                if (!Populate(*host))
                {
                    Log("changed row population refused; returning to stock Options");
                    if (Live(*host) && host->open) Close(*host);
                }
            }
            else RWGTAMenu::Model::Changed() = true;
        }
    }
    inline void Prepare(void* scene, int32_t tier)
    {
        if (!ready || tier != 1 || busy || !currentManager) return;
        PreserveBool guard(busy);
        auto it = hosts.find(scene);
        std::shared_ptr<Host> host;
        if (it != hosts.end()) host = it->second;
        else
        {
            auto tiered = Bridge::Read<void*>(scene, 960);
            if (!tiered || Bridge::Read<int32_t>(tiered, 616) != 12 ||
                !Bridge::Read<void*>(scene, 924) || !Bridge::Read<void*>(scene, 928))
            { Log("options owner/twelve tiers/caption contract unavailable"); return; }
            host = std::make_shared<Host>();
            host->scene = scene;
            host->tiered = tiered;
            host->manager = currentManager;
            hosts[scene] = host;
            Snapshot snapshot(*host);
            bridge.AddTier(tiered);
            if (!snapshot.Current() || Bridge::Read<int32_t>(tiered, 616) != 13)
            { Log("native appended tier creation failed"); return; }
            auto lists = Bridge::Read<void**>(tiered, 612);
            if (!lists || !lists[12]) return;
            host->list = lists[12];
            // Match the native ScenePause::Init configuration of its stock tiers.
            Bridge::Write<int32_t>(host->list, 620, 0);
            Bridge::Write<uint8_t>(host->list, 729, 0);
            Bridge::Write<int32_t>(host->list, 700, 14);
            SetBinding(host->accept, host, 0, reinterpret_cast<void*>(Accepted));
            SetBinding(host->selected, host, 0, reinterpret_cast<void*>(Selected));
            bridge.Connect(Bridge::At(host->list, 604), host->accept.slot, false);
            if (!snapshot.Current()) return;
            bridge.Connect(Bridge::At(host->list, 580), host->selected.slot, true);
            if (!snapshot.Current()) return;
        }
        if (!Live(*host) || !host->list) return;
        Snapshot snapshot(*host);
        auto lists = Bridge::Read<void**>(host->tiered, 612);
        if (!lists || Bridge::Read<int32_t>(host->tiered, 616) != 13) return;
        hostFile.texts.Select(Language());
        bridge.AddRow(lists[1], Wide(hostFile.texts.Resolve("@menu.plugins")), &host->advanced, 15);
    }
    inline uint8_t __fastcall Settings11(void* scene, void*, int32_t tier)
    {
        auto result = settingsHook.thiscall<uint8_t>(scene, tier);
        Prepare(scene, tier);
        return result;
    }
    inline int32_t __stdcall Settings9(void* scene, int32_t tier)
    {
        auto result = settingsHook.stdcall<int32_t>(scene, tier);
        Prepare(scene, tier);
        return result;
    }
    inline bool OwnedCommand(Host& host, int32_t command)
    {
        auto tier = Bridge::Read<int32_t>(host.tiered, 592);
        if (tier != 1 && tier != 12) return false;
        auto lists = Bridge::Read<void**>(host.tiered, 612);
        if (!lists) return false;
        auto list = lists[tier];
        auto index = Bridge::Read<int32_t>(list, 576);
        auto record = bridge.Record(list, index);
        if (!record) return false;
        auto pointer = Bridge::Read<const int32_t*>(record, 64);
        if (command == Advanced) return tier == 1 && pointer == &host.advanced;
        auto row = command - RowCommand;
        return tier == 12 && host.open && row >= 0 && static_cast<size_t>(row) < host.count && pointer == &host.items[row].command;
    }
    inline int32_t __fastcall Perform(void* scene, void*, int32_t command)
    {
        auto it = hosts.find(scene);
        auto host = it == hosts.end() ? nullptr : it->second;
        if (host && Live(*host) && OwnedCommand(*host, command))
        {
            Queue(*host, static_cast<size_t>(command - RowCommand), 0, command == Advanced);
            return 1;
        }
        return performHook.thiscall<int32_t>(scene, command);
    }
    inline int32_t __fastcall Back(void* scene, void*)
    {
        auto it = hosts.find(scene);
        auto host = it == hosts.end() ? nullptr : it->second;
        if (host && Live(*host) && host->open && Bridge::Read<int32_t>(host->tiered, 592) == 12 &&
            Bridge::Read<uint8_t>(scene, 2665) != 0 && Queue(*host, host->count, 0))
        {
            Bridge::Write<uint8_t>(scene, 2665, 0); // consume the same native Back request
            return 1;
        }
        return backHook.thiscall<int32_t>(scene);
    }
    inline void __fastcall Destroy(void* scene, void*)
    {
        auto it = hosts.find(scene);
        if (it != hosts.end())
        {
            auto host = it->second;
            host->alive = false;
            ++host->generation;
            host->model.Close();
            DisconnectRows(*host);
            bridge.Disconnect(host->accept.slot);
            bridge.Disconnect(host->selected.slot);
            retired.push_back(host);
            hosts.erase(it);
        }
        destroyHook.thiscall(scene);
    }
    inline uint8_t __fastcall Update(void* manager, void*, Slot* phase, float seconds)
    {
        currentManager = manager;
        ++updateDepth;
        auto result = updateHook.thiscall<uint8_t>(manager, phase, seconds);
        --updateDepth;
        if (!updateDepth && phase == Bridge::At(manager, 932))
        {
            Drain();
            retired.clear();
        }
        return result;
    }
    template<size_t N> inline bool Match(uint8_t* base, size_t imageSize, const BlacklistMenuContracts::Guard (&guards)[N], const char*& failed)
    {
        for (auto& guard : guards)
        {
            if (guard.rva > imageSize || guard.bytes.size() > imageSize - guard.rva) { failed = guard.name; return false; }
            for (size_t i = 0; i < guard.bytes.size(); ++i)
                if ((base[guard.rva + i] & guard.mask[i]) != (guard.bytes[i] & guard.mask[i])) { failed = guard.name; return false; }
        }
        return true;
    }
    inline void Init()
    {
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&Init), &self)) return;
        auto base = reinterpret_cast<uint8_t*>(GetModuleHandleW(nullptr));
        if (!base) return;
        auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 0x1000) return;
        auto nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) return;
        auto size = nt->OptionalHeader.SizeOfImage;
        const char* failed9 = "none";
        const char* failed11 = "none";
        bool dx9 = Match(base, size, BlacklistMenuContracts::DX9, failed9);
        if (!dx9 && !Match(base, size, BlacklistMenuContracts::DX11, failed11))
        { Log(std::string("unsupported native contracts DX9/") + failed9 + " DX11/" + failed11); return; }
        if (!asi_settings::ReadModule(self, hostFile)) return;
        auto address = [&](const char* name) -> void*
        {
            if (dx9)
            {
                for (auto& guard : BlacklistMenuContracts::DX9) if (!strcmp(guard.name, name)) return base + guard.rva;
            }
            else for (auto& guard : BlacklistMenuContracts::DX11) if (!strcmp(guard.name, name)) return base + guard.rva;
            return nullptr;
        };
        bridge.dx9 = dx9;
        bridge.addRow = address("AddRow"); bridge.addTier = address("AddTier"); bridge.clearList = address("ClearList");
        bridge.rehash = address("Rehash"); bridge.resizeArray = address("ResizeArray"); bridge.selectRow = address("SelectRow");
        bridge.cloneChoice = address("CloneChoice"); bridge.clearChoices = address("ClearChoices");
        bridge.connect = address("Register"); bridge.connectInt = address("RegisterInt"); bridge.disconnect = address("Disconnect");
        bridge.pushTier = address("PushTier"); bridge.popTier = address("PopTier"); bridge.historyPush = address("HistoryPush");
        bridge.setTitle = address("SetTitle"); bridge.currentLanguage = address("CurrentLanguage"); bridge.record = address("Record");
        bridge.gameMode = address("GameMode"); bridge.levelInfo = address("LevelInfo");
        bridge.topFocus = address("TopFocus");
        bridge.control = address("Control");
        if (dx9)
        {
            using namespace BlacklistMenuABI;
            clearMap9 = address("ClearMap"); hideRows9 = address("HideRows"); cloneChoice9 = bridge.cloneChoice;
            clearChoices9 = bridge.clearChoices; register9 = bridge.connect; registerInt9 = bridge.connectInt;
            popTier9 = bridge.popTier; switchTier9 = address("SwitchTier");
        }
        destroyHook = safetyhook::create_inline(address("Destroy"), Destroy);
        updateHook = safetyhook::create_inline(address("Update"), Update);
        backHook = safetyhook::create_inline(address("Back"), Back);
        performHook = safetyhook::create_inline(address("Perform"), Perform);
        settingsHook = safetyhook::create_inline(address("Settings"), dx9 ? reinterpret_cast<void*>(Settings9) : reinterpret_cast<void*>(Settings11));
        if (!destroyHook || !updateHook || !backHook || !performHook || !settingsHook)
        {
            settingsHook.reset(); performHook.reset(); backHook.reset(); updateHook.reset(); destroyHook.reset();
            return;
        }
        ready = true;
    }
}
