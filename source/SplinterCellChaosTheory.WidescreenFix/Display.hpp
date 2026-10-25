#pragma once
#include "ControlsRows.hpp"
#include "MenuText.hpp"
#include "FixSettings.hpp"
#include "../Shared/RWGTA/FixMenu.hpp"

namespace CTDisplay
{
using CTSettings::Read;
using CTSettings::Set;
inline constexpr uint32_t PageID = 0xC5C95E66;
inline constexpr uint32_t LabelID = 0xC05DD411, ButtonID = 0xC05DD412, ListID = 0xC05DD413, FooterID = 0xC05DD414;
inline const std::set<std::string> Settings{ "MAIN.WindowMode" };
inline HMODULE module = nullptr;
inline std::string (*language)() = nullptr;
inline std::wstring (*wide)(const std::string&) = nullptr;
inline CTText::Metrics (*metrics)(void*) = nullptr;
inline int(__fastcall* append)(void*, void*, const wchar_t*, int) = nullptr;
inline bool(__fastcall* select)(void*, void*, int16_t, char, char) = nullptr;
inline SafetyHookInline defaultsHook;
inline bool ready = false;
inline uint64_t nextSerial = 0;
struct Host
{
    void *page = nullptr, *controller = nullptr, *manager = nullptr;
    void *label = nullptr, *button = nullptr, *list = nullptr, *footer = nullptr;
    uint64_t serial = 0, settingsSerial = 0;
    bool published = false, opened = false, reset = false, hiddenByStock = false;
    std::string tag;
    std::wstring labelText, valueText, help;
    CTText::Metrics footerMetrics;
    RWGTAMenu::Model model;
};
inline std::map<void*, std::shared_ptr<Host>> hosts;
struct Action
{
    void* page;
    uint64_t serial;
    int direction;
};
inline std::deque<Action> pending;
inline bool Known(const Host& host)
{
    auto binding = CTSettings::Lookup(host.page, host.settingsSerial);
    return binding && binding->controller == host.controller && binding->manager == host.manager;
}
inline bool Live(const Host& host)
{
    if (!Known(host) || !host.published)
        return false;
    const std::array<void*, 4> items{ host.label, host.button, host.list, host.footer };
    const std::array<uint32_t, 4> ids{ LabelID, ButtonID, ListID, FooterID };
    for (size_t i = 0; i < items.size(); ++i)
        if (!CTSettings::Child(host.page, items[i]) || Read<uint32_t>(items[i], 4) != ids[i])
            return false;
    return CTSettings::Kind(host.button) == 2 && CTSettings::Kind(host.list) == 10;
}
inline std::shared_ptr<Host> Lookup(void* page, uint64_t serial)
{
    auto found = hosts.find(page);
    return found != hosts.end() && found->second->serial == serial && Known(*found->second) ? found->second : nullptr;
}
inline const RWGTAMenu::Row* Row(const Host& host)
{
    for (const auto& row : host.model.JoinedRows())
        if (row.setting && row.setting->id == "MAIN.WindowMode")
            return &row;
    return nullptr;
}
inline std::vector<std::string> Choices(const RWGTAMenu::Row& row)
{
    std::vector<std::string> result;
    if (!row.setting || !row.plugin || row.setting->choices.size() != 3)
        return result;
    for (const auto& choice : row.setting->choices)
        result.push_back(OtherPlugins::Text(*row.plugin, choice.label));
    return result;
}
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
inline void Queue(const std::shared_ptr<Host>& host, int direction)
{
    if (Live(*host) && CTSettings::current(host->page, host->manager) && pending.size() < 16)
        pending.push_back({ host->page, host->serial, direction });
}
struct Callback
{
    void** vtable = nullptr;
    uint32_t references = 1;
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
    static char __fastcall Key(Callback* self, void*, void* item, uint32_t* event)
    {
        auto host = Lookup(self->page, self->serial);
        if (!host || !Live(*host) || item != host->button || !event)
            return 0;
        if (*event == VK_LEFT || *event == VK_RIGHT)
        {
            Queue(host, *event == VK_LEFT ? -1 : 1);
            return 1;
        }
        if (*event == VK_UP || *event == VK_DOWN)
        {
            Queue(host, *event == VK_UP ? -3 : 3);
            return 1;
        }
        return 0;
    }
};
inline std::array<void*, 14> callbackVtable{ reinterpret_cast<void*>(&Callback::Destroy), reinterpret_cast<void*>(&Callback::Type),
                                             reinterpret_cast<void*>(&Callback::Notify),  reinterpret_cast<void*>(&Callback::Notify),
                                             reinterpret_cast<void*>(&Callback::Notify),  reinterpret_cast<void*>(&Callback::Notify),
                                             reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Key),
                                             reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Event),
                                             reinterpret_cast<void*>(&Callback::Event),   reinterpret_cast<void*>(&Callback::Event),
                                             reinterpret_cast<void*>(&Callback::Notify),  reinterpret_cast<void*>(&Callback::Notify) };
inline bool Accept(void* item)
{
    for (const auto& [page, host] : hosts)
        if (Live(*host) && (item == host->button || (host->opened && item == host->list)))
        {
            Queue(host, item == host->button ? 0 : 2);
            return true;
        }
    return false;
}
inline bool Cancel(void* item)
{
    for (const auto& [page, host] : hosts)
        if (Live(*host) && host->opened && (item == host->button || item == host->list))
        {
            Queue(host, -2);
            return true;
        }
    return false;
}
inline void Close(const std::shared_ptr<Host>& host, bool focus = true)
{
    Set<uint8_t>(host->list, 24, uint8_t(Read<uint8_t>(host->list, 24) & ~1u));
    host->opened = false;
    if (focus)
        CTSettings::setFocus(host->page, nullptr, host->button);
}
inline void Leaving(void* manager, void* nextPage)
{
    for (const auto& [page, host] : hosts)
        if (host->manager == manager && page != nextPage && Live(*host))
        {
            if (host->opened)
                Close(host, false);
            std::erase_if(pending, [page](const Action& action) { return action.page == page; });
        }
}
inline bool Join(const std::shared_ptr<Host>& host)
{
    if (!Known(*host))
        return false;
    if (host->published)
        return Live(*host);
    // Resolve plugin values before borrowing or cloning native resources:
    // a provider's getter can synchronously retire the entire source page.
    auto row = Row(*host);
    if (!row)
        return false;
    auto name = host->model.Label(*row), value = host->model.Value(*row);
    if (!Lookup(host->page, host->serial))
        return false;
    auto choices = Choices(*row);
    if (choices.size() != 3)
        return false;
    auto source = CTSettings::findChild(host->page, nullptr, 0x1DCFA2CE);
    auto sourceLabel = CTSettings::findChild(host->page, nullptr, 0xF4FE71D0);
    auto sourceListItem = Read<void*>(host->controller, 92);
    if (CTSettings::Kind(source) != 2 || CTSettings::Kind(sourceLabel) != 8 || CTSettings::Kind(sourceListItem) != 10 ||
        !CTSettings::Child(host->page, sourceListItem))
        return false;
    auto sourceBody = Read<void*>(source, 8), sourceList = Read<void*>(sourceListItem, 8);
    auto labelBody = Read<void*>(sourceLabel, 8), labelPoint = Read<void*>(labelBody, 4);
    if (sourceList != Read<void*>(host->controller, 96) || !labelPoint || Read<int16_t>(labelPoint, 8) != 58 ||
        Read<int16_t>(labelPoint, 12) != 247 || !Read<void*>(labelBody, 40) || !Read<void*>(sourceBody, 28) || !Read<void*>(sourceList, 44))
        return false;
    for (auto id : { LabelID, ButtonID, ListID, FooterID })
        if (CTSettings::findChild(host->page, nullptr, id))
            return false;
    uintptr_t prototypeChildren = 0;
    size_t prototypeCount = 0;
    void* footerSource = nullptr;
    if (!CTLayout::Vector(Read<void*>(sourceBody, 28), 8, 64, prototypeChildren, prototypeCount))
        return false;
    for (size_t i = 0; i < prototypeCount; ++i)
    {
        auto child = reinterpret_cast<void**>(prototypeChildren)[i];
        if (CTSettings::Kind(child) == 8)
        {
            if (footerSource)
                return false;
            footerSource = child;
        }
    }
    if (!footerSource)
        return false;
    std::unique_ptr<void, decltype(&CTSettings::Destroy)> label(CTLayout::CloneItem(sourceLabel, 640, 640), CTSettings::Destroy);
    std::unique_ptr<void, decltype(&CTSettings::Destroy)> button(CTSettings::makeItem(nullptr, nullptr, 2), CTSettings::Destroy);
    std::unique_ptr<void, decltype(&CTSettings::Destroy)> list(CTSettings::makeItem(nullptr, nullptr, 10), CTSettings::Destroy);
    std::unique_ptr<void, decltype(&CTSettings::Destroy)> footer(CTLayout::CloneItem(footerSource, 640, 640, false), CTSettings::Destroy);
    if (!label || !button || !list || !footer)
        return false;
    auto target = Read<void*>(button.get(), 8), targetList = Read<void*>(list.get(), 8);
    if (!target || !targetList)
        return false;
    Set<void*>(target, 24, Read<void*>(sourceBody, 24));
    CTSettings::bindStyle(static_cast<uint8_t*>(target) + 20, nullptr, Read<void*>(sourceBody, 32), Read<void*>(sourceBody, 28));
    auto point = Read<void*>(target, 4);
    Set<uint32_t>(point, 4, Read<uint32_t>(Read<void*>(sourceBody, 4), 4));
    Set<uint32_t>(point, 8, 58u | (324u << 16));
    for (auto offset : { 36u, 56u, 76u })
    {
        if (!Read<void*>(sourceList, offset + 8))
            continue;
        Set<void*>(targetList, offset + 4, Read<void*>(sourceList, offset + 4));
        CTSettings::bindStyle(static_cast<uint8_t*>(targetList) + offset, nullptr, Read<void*>(sourceList, offset + 12),
                              Read<void*>(sourceList, offset + 8));
    }
    auto listPoint = Read<void*>(targetList, 4);
    Set<uint32_t>(listPoint, 4, Read<uint32_t>(Read<void*>(sourceList, 4), 4));
    Set<uint32_t>(listPoint, 8, 58u | (343u << 16));
    Set<int8_t>(targetList, 164, Read<int8_t>(sourceList, 164));
    Set<uint8_t>(targetList, 166, 3);
    Set<uint32_t>(targetList, 176, Read<uint32_t>(sourceList, 176));
    Set<uint8_t>(targetList, 180, Read<uint8_t>(sourceList, 180));
    Set<uint8_t>(targetList, 184, Read<uint8_t>(sourceList, 184));
    auto targetLabel = Read<void*>(label.get(), 8), targetLabelPoint = Read<void*>(targetLabel, 4);
    Set<int16_t>(targetLabelPoint, 12, 300);
    Set<int16_t>(targetLabelPoint, 14, 320);
    // Retain the native page's fade instead of freezing its first transparent
    // sample. Every independently cloned keyframe moves with the new row.
    if (auto frames = Read<void*>(label.get(), 12))
    {
        uintptr_t frameBegin = 0;
        size_t frameCount = 0;
        if (!CTLayout::Vector(frames, 4, 32, frameBegin, frameCount))
            return false;
        for (size_t i = 0; i < frameCount; ++i)
        {
            auto parameter = Read<void*>(reinterpret_cast<void**>(frameBegin)[i], 4);
            Set<int16_t>(parameter, 12, int16_t(Read<int16_t>(parameter, 12) + 53));
            Set<int16_t>(parameter, 14, int16_t(Read<int16_t>(parameter, 14) + 53));
        }
    }
    auto measurement = metrics(Read<void*>(target, 28));
    host->footerMetrics = measurement;
    host->footerMetrics.width = 522;
    auto footerPoint = Read<void*>(Read<void*>(footer.get(), 8), 4);
    Set<int16_t>(footerPoint, 8, 58);
    Set<int16_t>(footerPoint, 10, 580);
    Set<int16_t>(footerPoint, 12, 373);
    Set<int16_t>(footerPoint, 14, 393);
    Set<uint32_t>(footer.get(), 4, FooterID);
    auto styled = [&](const std::string& text) { return CTText::Style(wide(text), measurement, wide(host->tag).c_str()); };
    CTText::Metrics labelMetrics;
    labelMetrics.font = Read<void*>(labelBody, 40);
    auto labelVtable = Read<void**>(labelMetrics.font, 0);
    auto height = reinterpret_cast<int(__fastcall*)(void*, void*)>(labelVtable[6])(labelMetrics.font, nullptr);
    if (height <= 0)
        return false;
    if (reinterpret_cast<char(__fastcall*)(void*, void*)>(labelVtable[5])(labelMetrics.font, nullptr))
        labelMetrics.scale = Read<uint16_t>(labelPoint, 22) / double(height);
    labelMetrics.spacing = labelMetrics.scale * height * Read<int16_t>(labelPoint, 18) * 0.001;
    auto title = CTText::Style(wide(name), labelMetrics, wide(host->tag).c_str()), shown = styled(value);
    if (!measurement.font || title.empty() || shown.empty() || labelMetrics.Measure(title) > 291 ||
        measurement.Measure(shown) > measurement.width || !CTControlRows::Assign(static_cast<uint8_t*>(targetLabel) + 56, title))
        return false;
    CTSettings::setText(target, nullptr, shown.c_str());
    for (const auto& choice : choices)
    {
        auto text = styled(choice);
        if (text.empty() || measurement.Measure(text) > measurement.width || append(targetList, nullptr, text.c_str(), 0) < 0)
            return false;
    }
    if (CTControlRows::Count(targetList) != 3)
        return false;
    Set<uint32_t>(label.get(), 4, LabelID);
    Set<uint32_t>(button.get(), 4, ButtonID);
    Set<uint32_t>(list.get(), 4, ListID);
    Set<uint8_t>(list.get(), 24, uint8_t(Read<uint8_t>(list.get(), 24) & ~1u));
    auto callback = std::unique_ptr<Callback>(new (std::nothrow) Callback);
    if (!callback)
        return false;
    callback->vtable = callbackVtable.data();
    callback->page = host->page;
    callback->serial = host->serial;
    CTSettings::addCallback(button.get(), nullptr, callback.release());
    // Stage all four children in one owned vector. A refusal publishes none.
    uintptr_t begin = 0;
    size_t count = 0;
    if (!CTLayout::Vector(host->page, 8, 124, begin, count))
        return false;
    const auto capacity = Read<uintptr_t>(host->page, 16);
    if ((begin && capacity < begin + count * 4) || (!begin && capacity))
        return false;
    auto children = CTLayout::Memory((count + 4) * 4);
    if (!children)
        return false;
    if (count)
        std::memcpy(children, reinterpret_cast<void*>(begin), count * 4);
    auto entries = static_cast<void**>(children);
    entries[count] = label.get();
    entries[count + 1] = button.get();
    entries[count + 2] = list.get();
    entries[count + 3] = footer.get();
    Set<void*>(host->page, 8, children);
    Set<void*>(host->page, 12, entries + count + 4);
    Set<void*>(host->page, 16, entries + count + 4);
    if (begin)
        CTLayout::release(reinterpret_cast<void*>(begin));
    host->label = label.release();
    host->button = button.release();
    host->list = list.release();
    host->footer = footer.release();
    host->labelText = std::move(title);
    host->valueText = std::move(shown);
    host->published = true;
    return true;
}
inline void Tick(void* page)
{
    if (!ready)
        return;
    std::erase_if(pending, [page](const Action& action) { return action.page != page; });
    auto binding = CTSettings::bindings.find(page);
    if (binding == CTSettings::bindings.end() || !CTSettings::Live(binding->second) || Read<uint32_t>(page, 20) != PageID ||
        !CTSettings::current(page, binding->second.manager))
        return;
    auto found = hosts.find(page);
    if (found != hosts.end() && !Known(*found->second))
    {
        Forget(page);
        found = hosts.end();
    }
    if (found == hosts.end())
    {
        auto host = std::make_shared<Host>();
        host->page = page;
        host->controller = binding->second.controller;
        host->manager = binding->second.manager;
        host->settingsSerial = binding->second.serial;
        host->serial = ++nextSerial;
        host->tag = language();
        hosts.emplace(page, host);
        host->model.OpenPlugins(module, host->tag, 4096, Settings);
        if (!Lookup(page, host->serial))
            return;
        found = hosts.find(page);
    }
    auto host = found->second;
    if (!Join(host))
        return;
    auto focused = Read<void*>(page, 36);
    if (host->opened && focused != host->list && focused != host->button)
        Close(host, false);
    if (host->reset)
    {
        if (host->opened)
            Close(host);
        host->reset = false;
        RWGTAMenu::RestoreDefaults(FixSettings(), Settings);
        if (!Lookup(page, host->serial) || !Live(*host))
            return;
    }
    auto stockList = Read<void*>(host->controller, 92);
    const bool expanded = stockList && CTSettings::Child(page, stockList) && (Read<uint8_t>(stockList, 24) & 1);
    if (expanded != host->hiddenByStock)
    {
        for (auto item : { host->label, host->button })
            Set<uint8_t>(item, 24, uint8_t((Read<uint8_t>(item, 24) & ~1u) | (expanded ? 0u : 1u)));
        host->hiddenByStock = expanded;
    }
    // The stock Resolution dropdown overlaps the empty band used by this
    // row. Let it retain the entire native viewport and pointer hit area.
    if (expanded)
    {
        if (host->opened)
            Close(host, false);
        Set<uint8_t>(host->footer, 24, uint8_t(Read<uint8_t>(host->footer, 24) & ~1u));
        std::erase_if(pending, [page](const Action& action) { return action.page == page; });
        return;
    }
    auto actions = std::move(pending);
    pending.clear();
    for (const auto& action : actions)
    {
        if (action.page != page || action.serial != host->serial || !Live(*host))
            continue;
        auto row = Row(*host);
        if (!row)
            continue;
        auto enabled = host->model.Enabled(*row, false);
        if (!Lookup(page, host->serial) || !Live(*host))
            return;
        if (!enabled)
            continue;
        if (!action.direction)
        {
            if (host->opened)
            {
                Close(host);
                continue;
            }
            auto values = Choices(*row);
            auto value = host->model.Value(*row);
            if (!Lookup(page, host->serial) || !Live(*host))
                return;
            auto at = std::find(values.begin(), values.end(), value);
            if (at == values.end())
                continue;
            select(Read<void*>(host->list, 8), nullptr, int16_t(at - values.begin()), 1, 1);
            Set<uint8_t>(host->list, 24, uint8_t(Read<uint8_t>(host->list, 24) | 1u));
            host->opened = true;
            CTSettings::setFocus(page, nullptr, host->list);
        }
        else if (action.direction == -2)
            Close(host);
        else if (action.direction == 2)
        {
            auto selected = Read<int16_t>(Read<void*>(host->list, 8), 168);
            if (selected >= 0 && selected < 3)
            {
                auto values = Choices(*row);
                auto value = host->model.Value(*row);
                if (!Lookup(page, host->serial) || !Live(*host))
                    return;
                auto at = std::find(values.begin(), values.end(), value);
                if (values.size() == 3 && at != values.end())
                    for (int i = int(at - values.begin()); i != selected; i += i < selected ? 1 : -1)
                    {
                        host->model.Change(*row, i < selected ? 1 : -1, false);
                        if (!Lookup(page, host->serial) || !Live(*host))
                            return;
                    }
            }
            Close(host);
        }
        else if (std::abs(action.direction) == 3)
        {
            auto target = CTSettings::findChild(page, nullptr, action.direction < 0 ? 0x1DCFA2CE : 0xF4F4E402);
            if (target)
                CTSettings::setFocus(page, nullptr, target);
        }
        else
        {
            host->model.Change(*row, action.direction, false);
            if (!Lookup(page, host->serial) || !Live(*host))
                return;
        }
    }
    if (!host->opened && Read<void*>(page, 36) == host->list)
        CTSettings::setFocus(page, nullptr, host->button);
    auto row = Row(*host);
    if (!row)
        return;
    auto value = host->model.Value(*row);
    if (!Lookup(page, host->serial) || !Live(*host))
        return;
    auto measurement = metrics(Read<void*>(Read<void*>(host->button, 8), 28));
    auto shown = CTText::Style(wide(value), measurement, wide(host->tag).c_str());
    if (!shown.empty() && shown != host->valueText && measurement.Measure(shown) <= measurement.width)
    {
        CTSettings::setText(Read<void*>(host->button, 8), nullptr, shown.c_str());
        host->valueText = std::move(shown);
    }
    auto help = host->model.RestartRequired() ? host->model.Text("menu.restartwarning") : host->model.Help(*row);
    if (!Lookup(page, host->serial) || !Live(*host))
        return;
    auto text = CTText::Fit(CTText::Style(wide(help), host->footerMetrics, wide(host->tag).c_str()), host->footerMetrics, 522);
    if (text != host->help && CTControlRows::Assign(static_cast<uint8_t*>(Read<void*>(host->footer, 8)) + 56, text))
        host->help = std::move(text);
    Set<uint8_t>(host->footer, 24, uint8_t((Read<uint8_t>(host->footer, 24) & ~1u) | (!host->opened ? 1u : 0u)));
}
inline bool JoinedFor(void* manager)
{
    for (const auto& [page, host] : hosts)
        if (host->manager == manager && Live(*host))
            return true;
    return false;
}
inline int __fastcall Defaults(void* controller, void*)
{
    auto page = Read<void*>(controller, 56);
    auto found = hosts.find(page);
    auto host = found == hosts.end() ? nullptr : found->second;
    auto joined = host && Live(*host);
    auto result = defaultsHook.thiscall<int>(controller);
    if (joined && Lookup(page, host->serial))
        host->reset = true;
    return result;
}
inline bool Init(HMODULE game = GetModuleHandleW(nullptr))
{
    if (ready)
        return true;
    auto defaults =
        hook::pattern(game, "51 56 68 ? ? ? ? 8D 54 24 08 52 8B F1 8B 0D ? ? ? ? 8B 01 68 ? ? ? ? 68 ? ? ? ? FF 50 04 85 C0 74 0C");
    if (defaults.size() != 1 || !module || !language || !wide || !metrics || !append || !select)
        return false;
    defaultsHook = safetyhook::create_inline(defaults.get_first(), Defaults);
    ready = bool(defaultsHook);
    return ready;
}
} // namespace CTDisplay
