module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <cmath>
#include <cwctype>
#include <functional>
#include <set>
#include <sstream>
#include <array>
#include <deque>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <cstring>
#include <Windows.h>
#include <algorithm>

export module NativeMenu;

export import MenuSettings;
export import MenuLayout;
export import MenuText;
export import Controls;
export import Display;
import ComVars;
import GUI;

// The Plugins page is built from the game's own menu page, list and row objects,
// so it uses the same allocation, input and rendering as the native menus.
export namespace CTMenu
{
    inline SafetyHookInline titleBuildHook, pauseBuildHook, acceptHook, cancelHook, keyHook, displayHook, popHook, destructorHook, stackDestructorHook, pushHook;
    inline const wchar_t* (__cdecl* getLanguage)() = nullptr;
    inline void* (__stdcall* makePage)(int) = nullptr;
    inline void* (__fastcall* makeItem)(void*, void*, int) = nullptr;
    inline int (__fastcall* bindStyle)(void*, void*, void*, void*) = nullptr;
    inline int (__fastcall* append)(void*, void*, const wchar_t*, int) = nullptr;
    inline int (__fastcall* clear)(void*, void*) = nullptr;
    inline bool (__fastcall* select)(void*, void*, int16_t, char, char) = nullptr;
    inline unsigned (__fastcall* addChild)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* getStack)(void*, void*) = nullptr;
    inline int (__cdecl* resetFocus)(void*) = nullptr;
    inline char (__fastcall* moveMouse)(void*, void*, const uint32_t*) = nullptr;
    inline HMODULE module = nullptr;
    inline bool busy = false, ready = false;
    struct BusyScope
    {
        bool previous = busy;
        BusyScope() { busy = true; }
        ~BusyScope() { busy = previous; }
    };

    template<class T> T Read(void* object, size_t offset)
    { return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset); }
    template<class T> void Set(void* object, size_t offset, T value)
    { *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) = value; }
    inline void LogOnce(const std::string& reason)
    {
        static std::set<std::string> written;
        if (!written.insert(reason).second) return;
        OutputDebugStringA(("SCCT native menu: " + reason + "\n").c_str());
    }
    inline std::wstring Wide(const std::string& text)
    {
        auto n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        std::wstring out(n, L'\0');
        if (n) MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), out.data(), n);
        for (auto& c : out) if (c < L' ') c = L' ';
        if (out.size() > 512) { out.resize(512); if (out.back() >= 0xD800 && out.back() <= 0xDBFF) out.pop_back(); }
        return out;
    }
    inline std::string Language()
    {
        auto language = FixSettings().String("MAIN.TextLanguage");
        if (!language.empty() && language != "auto") return language;
        auto native = getLanguage ? getLanguage() : nullptr;
        if (!native) return "en";
        if (!_wcsicmp(native, L"fre") || !_wcsicmp(native, L"fra")) return "fr";
        if (!_wcsicmp(native, L"deu")) return "de";
        if (!_wcsicmp(native, L"ita")) return "it";
        if (!_wcsicmp(native, L"spa") || !_wcsicmp(native, L"esp")) return "es";
        if (!_wcsicmp(native, L"rus")) return "ru";
        return "en"; // Unreal's native "int" resources are English.
    }
    inline int Count(void* list)
    {
        if (!list) return 0;
        auto begin = Read<uintptr_t>(list, 108), end = Read<uintptr_t>(list, 112);
        if (!begin) return 0;
        return end >= begin && end - begin <= 60 * 256 && (end - begin) % 60 == 0 ? int((end - begin) / 60) : -1;
    }
    struct Host
    {
        void* controller = nullptr, * manager = nullptr, * parent = nullptr, * list = nullptr;
        int entry = -1;
        bool pause = false;
        uint64_t serial = 0;
        void* entryPage = nullptr;
        uint64_t entrySerial = 0;
    };
    inline std::map<void*, Host> hosts;
    inline uint64_t nextHostSerial = 0;
    struct ScreenState
    {
        Host host;
        void* page = nullptr, * item = nullptr, * list = nullptr, * stack = nullptr;
        size_t descriptorPage = SIZE_MAX, rootStart = 0, rowStart = 0, selected = 0;
        std::shared_ptr<CTLayout::Templates> templates;
        bool details = false, notice = false;
        size_t helpIndex = 0, detailsReturn = 0, detailsPage = 0;
        std::vector<std::wstring> detailsLines;
    };
    inline std::optional<ScreenState> screen;
    struct RetiredPage { void* page; void* stack; std::shared_ptr<CTLayout::Templates> templates; };
    inline std::vector<RetiredPage> retired;
    inline RWGTAMenu::Model model;
    inline constexpr size_t RootLinks = 3, PageRows = 5;

    inline bool InStack(void* stack, void* page)
    {
        if (!stack || !page) return false;
        auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        if (!begin || end < begin || end - begin > 8 * 64 || (end - begin) % 8) return false;
        for (auto at = begin; at != end; at += 8) if (*reinterpret_cast<void**>(at) == page) return true;
        return false;
    }
    inline void* MenuStack()
    {
        auto menuTop = [](void* stack)
        {
            if (!stack) return false;
            auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
            if (!begin || end <= begin || end - begin > 8 * 64 || (end - begin) % 8) return false;
            auto top = *reinterpret_cast<void**>(end - 8);
            return top && ((screen && screen->page == top) || CMenusManager::IsPointerMenuPage(Read<uint32_t>(top, 20)));
        };
        if (screen && menuTop(screen->stack)) return screen->stack;
        for (const auto& [list, host] : hosts)
        {
            auto stack = getStack(host.manager, nullptr);
            if (menuTop(stack)) return stack;
        }
        return nullptr;
    }
    inline bool WindowsMenuMouse(LPARAM position, bool move)
    {
        if (!ready || busy || !moveMouse || Screen.Width <= 0 || Screen.Height <= 0 || GetForegroundWindow() != hGameWindow) return false;
        auto stack = MenuStack();
        if (!stack) return false;
        if (move)
        {
            // Use the same logical bounds as GUI's native mouse conversion.
            // CT's native Windows path may capture relative motion; Magma's
            // verified absolute event supplies the actual menu hover/picking.
            auto delta = int16_t(((480.0f * Screen.fAspectRatio) - 640.0f) / 2.0f);
            auto x = int16_t(std::clamp(float(int16_t(LOWORD(position))) / Screen.Width, 0.0f, 1.0f) * (640 + 2 * delta) - delta);
            auto y = int16_t(std::clamp(float(int16_t(HIWORD(position))) / Screen.Height, 0.0f, 1.0f) * 480);
            uint32_t event[] = { 1, uint16_t(x) | (uint32_t(uint16_t(y)) << 16) };
            BusyScope scope;
            moveMouse(stack, nullptr, event);
        }
        return true;
    }
    inline void DestroyPage(void* page)
    {
        if (!page) return;
        auto fn = Read<void**>(page, 0)[0];
        reinterpret_cast<void* (__fastcall*)(void*, void*, char)>(fn)(page, nullptr, 1);
    }
    inline void Retire()
    {
        if (!screen) return;
        // KeyRelease resets these two indices after Accept returns. Its reset
        // helpers animate borrowed prototypes only when an index changes.
        // Set both to -1 before any package teardown can free those prototypes;
        // the post-Accept resets then touch only our still-live list fields.
        Set<int16_t>(screen->list, 170, -1);
        Set<int16_t>(screen->list, 172, -1);
        // Native hover/leave dispatchers check this active bit after invoking
        // handlers. Retirement during a handler must not animate freed source
        // prototypes when those dispatchers continue with our retained list.
        Set<uint8_t>(screen->list, 180, uint8_t(Read<uint8_t>(screen->list, 180) & ~2u));
        retired.push_back({ screen->page, screen->stack, screen->templates }); screen.reset(); model.Close();
    }
    inline void Frame(void* updatingPage)
    {
        // A click/key handler may continue using its list after Accept returns.
        // Free only on the next native page update, after input has unwound.
        if (!busy)
        {
            BusyScope scope;
            std::erase_if(retired, [updatingPage](const RetiredPage& old)
            {
                if (old.page == updatingPage || InStack(old.stack, old.page)) return false;
                DestroyPage(old.page); return true;
            });
        }
    }
    inline void Close()
    {
        if (!screen) return;
        auto manager = screen->host.manager, page = screen->page;
        BusyScope scope;
        displayHook.thiscall<void>(manager, page, char(0), char(1), char(0));
        if (screen && screen->page == page) Retire();
    }
    inline size_t Links()
    { return std::min(RootLinks, model.Pages().size() - std::min(screen->rootStart, model.Pages().size())); }
    inline void* TextElement(void* page, unsigned depth = 0)
    {
        if (!page || depth > 4) return nullptr;
        auto begin = Read<uintptr_t>(page, 8), end = Read<uintptr_t>(page, 12);
        if (!begin || end < begin || end - begin > 4 * 64 || (end - begin) % 4) return nullptr;
        for (auto at = begin; at < end; at += 4)
        {
            auto item = *reinterpret_cast<void**>(at);
            if (!item) continue;
            auto element = Read<void*>(item, 8);
            if (!element) continue;
            auto type = reinterpret_cast<int (__fastcall*)(void*, void*)>(Read<void**>(element, 0)[3])(element, nullptr);
            if (type == 8 && Read<void*>(element, 40)) return element;
            if (type == 2 || type == 3 || type == 4)
                if (auto found = TextElement(element, depth + 1)) return found;
        }
        return nullptr;
    }
    using CTText::Metrics;
    using CTText::Fit;
    inline Metrics PrototypeMetrics(void* prototype)
    {
        Metrics result;
        auto element = TextElement(prototype);
        if (!element) { LogOnce("native row text/font metrics unavailable"); return result; }
        result.font = Read<void*>(element, 40);
        auto point = Read<void*>(element, 4);
        auto vt = Read<void**>(result.font, 0);
        if (!vt[3] || !vt[4] || !vt[5] || !vt[6]) { result.font = nullptr; return result; }
        auto height = reinterpret_cast<int (__fastcall*)(void*, void*)>(vt[6])(result.font, nullptr);
        if (!point || height <= 0) { result.font = nullptr; return result; }
        auto scalable = reinterpret_cast<char (__fastcall*)(void*, void*)>(vt[5])(result.font, nullptr);
        if (scalable && height > 0) result.scale = Read<uint16_t>(point, 22) / double(height);
        result.spacing = result.scale * height * Read<int16_t>(point, 18) * 0.001;
        // Bound even a free-width prototype to the private 490-unit text span.
        // A fixed-width native text rectangle supplies a tighter limit.
        auto width = Read<int16_t>(point, 10) - Read<int16_t>(point, 8);
        if (!(Read<uint8_t>(element, 88) & 0x40) && width > 0) result.width = std::min(result.width, double(width));
        return result;
    }
    inline Metrics RowMetrics(void* list)
    {
        return PrototypeMetrics(Read<void*>(list, 44));
    }
    inline bool SettingsTextFits(void* body, double width)
    {
        auto metrics = PrototypeMetrics(Read<void*>(body, 28));
        const auto length = Read<uint32_t>(body, 60), capacity = Read<uint32_t>(body, 64);
        if (!metrics.font || length > 512 || capacity < length || capacity > 16384)
            return false;
        auto text = capacity < 8 ? reinterpret_cast<const wchar_t*>(static_cast<uint8_t*>(body) + 44) : Read<const wchar_t*>(body, 44);
        return text && metrics.Measure(std::wstring(text, length)) <= width;
    }
    inline std::wstring SettingsCaption(void* body, double width)
    {
        auto metrics = PrototypeMetrics(Read<void*>(body, 28));
        auto text = Wide(RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"));
        if (!metrics.font || text.empty())
            return {};
        text = CTText::Style(std::move(text), metrics, Wide(Language()).c_str());
        return metrics.Measure(text) <= width ? text : std::wstring{};
    }
    inline std::wstring Styled(const std::string& text, const Metrics& metrics)
    {
        return CTText::Style(Wide(text), metrics, Wide(Language()).c_str());
    }
    inline void RowWide(const std::wstring& wide, bool disabled)
    {
        auto index = append(screen->list, nullptr, wide.c_str(), 0);
        if (index >= 0 && index < Count(screen->list))
            Set<uint8_t>(Read<void*>(screen->list, 108), size_t(index) * 60 + 53, disabled ? 1 : 0);
    }
    inline void Row(const std::string& text, bool disabled)
    {
        auto metrics = RowMetrics(screen->list);
        RowWide(Fit(Styled(text, metrics), metrics, metrics.width), disabled);
    }
    inline bool SettingRow(const RWGTAMenu::Row& row, bool enabled, void* pageToken)
    {
        if (!screen || screen->page != pageToken) return false;
        auto labelText = model.Label(row);
        auto valueText = enabled ? model.Value(row) : model.Text("menu.unavailable");
        // A plugin API may synchronously rebuild the native source package.
        // Reacquire the font only after its page identity has been checked.
        if (!screen || screen->page != pageToken) return false;
        auto metrics = RowMetrics(screen->list);
        auto label = Styled(labelText, metrics);
        RowWide(CTText::Setting(label, Styled(valueText, metrics), metrics), !enabled);
        return true;
    }
    inline void Populate();
inline constexpr size_t DetailLines = 5;
inline size_t DetailPages()
{
    return screen ? std::max(size_t(1), (screen->detailsLines.size() + DetailLines - 1) / DetailLines) : 1;
}
inline std::wstring DetailsWide(const std::string& text)
{
    if (text.size() > 65536) return {};
    const int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
    if (length <= 0) return {};
    std::wstring wide(length, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), length);
    for (auto& c : wide) if (c < L' ' && c != L'\n') c = L' ';
    return wide;
}
inline std::vector<std::wstring> WrapDetails(const std::wstring& text, double width,
    const std::function<double(const std::wstring&)>& measure)
{
    if (!std::isfinite(width) || width <= 0) return {};
    std::vector<std::wstring> lines;
    std::wstring line;
    auto flush = [&]() { if (!line.empty()) { lines.push_back(std::move(line)); line.clear(); } };
    for (size_t at = 0; at < text.size();)
    {
        if (text[at] == L'\n') { flush(); ++at; continue; }
        if (iswspace(text[at])) { ++at; continue; }
        auto end = at;
        while (end < text.size() && !iswspace(text[end])) ++end;
        auto word = text.substr(at, end - at); at = end;
        if (!line.empty() && measure(line + L" " + word) > width) flush();
        while (!word.empty() && measure(word) > width)
        {
            size_t take = 0;
            for (size_t i = 0; i < word.size();)
            {
                size_t next = i + 1;
                if (word[i] >= 0xD800 && word[i] <= 0xDBFF && next < word.size() && word[next] >= 0xDC00 && word[next] <= 0xDFFF) ++next;
                if (measure(word.substr(0, next)) > width) break;
                take = next; i = next;
            }
            if (!take) return {};
            lines.push_back(word.substr(0, take)); word.erase(0, take);
        }
        if (!word.empty()) { if (!line.empty()) line += L' '; line += word; }
    }
    flush();
    return lines;
}
inline void PopulateDetails()
{
    if (!screen || !screen->details) return;
    const auto pages = DetailPages();
    screen->detailsPage = std::min(screen->detailsPage, pages - 1);
    clear(screen->list, nullptr);
    Row((screen->notice ? RWGTAMenu::PluginText(FixSettings(), Language(), "update.title") : model.Text("menu.details")) + " (" + std::to_string(screen->detailsPage + 1) + "/" + std::to_string(pages) + ")", true);
    for (size_t i = 0; i < DetailLines; ++i)
    {
        const auto at = screen->detailsPage * DetailLines + i;
        RowWide(at < screen->detailsLines.size() ? screen->detailsLines[at] : L"", true);
    }
    if (pages > 1) Row(model.Text("menu.next"), false);
    Row(model.Text("menu.back"), false);
    select(screen->list, nullptr, int16_t(Count(screen->list) - 1), 1, 1);
}
inline void EndDetails()
{
    if (!screen || !screen->details) return;
    if (screen->notice) { Close(); return; }
    screen->details = false; screen->detailsLines.clear();
    screen->selected = screen->detailsReturn;
    Populate();
}
inline void StepDetails(int direction)
{
    if (!screen || !screen->details) return;
    const auto pages = DetailPages();
    screen->detailsPage = direction < 0 ? (screen->detailsPage + pages - 1) % pages : (screen->detailsPage + 1) % pages;
    PopulateDetails();
}
inline void BeginDetails()
{
    if (!screen || screen->descriptorPage == SIZE_MAX) return;
    const auto token = screen->page;
    const auto snapshot = model;
    const auto rows = snapshot.VisibleRows(screen->descriptorPage);
    const auto at = screen->rowStart + screen->helpIndex;
    if (at >= rows.size()) return;
    auto title = snapshot.Label(*rows[at]);
    if (!screen || screen->page != token) return;
    auto value = snapshot.Value(*rows[at]);
    if (!screen || screen->page != token) return;
    auto help = snapshot.Help(*rows[at]);
    if (!screen || screen->page != token || help.empty()) return;
    if (!value.empty()) title += ": " + value;
    const auto metrics = RowMetrics(screen->list);
    const auto locale = Wide(Language());
    if (!metrics.font || !std::isfinite(metrics.width) || metrics.width <= 0) return;
    auto text = CTText::Style(DetailsWide(title), metrics, locale.c_str()) + L"\n" +
        CTText::Style(DetailsWide(help), metrics, locale.c_str());
    auto lines = WrapDetails(text, metrics.width, [&](const auto& part)
    {
        const auto width = metrics.Measure(part);
        return std::isfinite(width) && (part.empty() || width > 0) ? width : INFINITY;
    });
    if (lines.empty()) return;
    screen->detailsReturn = screen->selected; screen->detailsPage = 0;
    screen->detailsLines = std::move(lines); screen->details = true;
    PopulateDetails();
}
    inline bool WindowsMenuHelp(bool pressed)
    {
        if (!ready || busy || !screen || screen->descriptorPage == SIZE_MAX || GetForegroundWindow() != hGameWindow) return false;
        const auto stack = MenuStack();
        if (!stack || stack != screen->stack) return false;
        const auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        if (!begin || end < begin + 8 || end - begin > 8 * 64 || (end - begin) % 8 ||
            Read<void*>(reinterpret_cast<void*>(end - 8), 0) != screen->page) return false;
        if (pressed)
        {
            BusyScope scope;
            if (screen->details) EndDetails();
            else BeginDetails();
        }
        return true;
    }
    inline std::string PageTitle(size_t index)
    {
        const auto& page = model.Pages()[index];
        return page.plugin && page.plugin->module == module ?
            RWGTAMenu::PluginText(FixSettings(), Language(), "plugin.shortname") : page.title;
    }
    inline void Populate()
    {
        if (!screen) return;
        if (screen->details) { PopulateDetails(); return; }
        auto pageToken = screen->page;
        auto root = screen->descriptorPage == SIZE_MAX;
        auto rows = root ? std::vector<const RWGTAMenu::Row*>{} : model.VisibleRows(screen->descriptorPage);
        if (!screen || screen->page != pageToken) return;
        auto links = root ? Links() : 0;
        clear(screen->list, nullptr);
        Row(root ? model.Text("menu.plugins") : PageTitle(screen->descriptorPage), true);
        if (root)
        {
            for (size_t i = 0; i < links; ++i) Row(PageTitle(screen->rootStart + i), false);
            if (screen->rootStart) Row(model.Text("menu.previous"), false);
            if (screen->rootStart + links < model.Pages().size()) Row(model.Text("menu.next"), false);
        }
        else
        {
            screen->rowStart = rows.empty() ? 0 : std::min(screen->rowStart, ((rows.size() - 1) / PageRows) * PageRows);
            auto count = std::min(PageRows, rows.size() - screen->rowStart);
            for (size_t i = 0; i < count; ++i)
            {
                auto row = rows[screen->rowStart + i];
                auto enabled = model.Enabled(*row, screen->host.pause);
                if (!screen || screen->page != pageToken) return;
                if (!SettingRow(*row, enabled, pageToken)) return;
            }
            if (screen->rowStart) Row(model.Text("menu.previous"), false);
            if (screen->rowStart + count < rows.size()) Row(model.Text("menu.next"), false);
            if (count)
            {
                if (screen->selected < count) screen->helpIndex = screen->selected;
                screen->helpIndex = std::min(screen->helpIndex, count - 1);
                Row(model.Text("menu.details"), false);
            }
            if (model.RestartRequired()) Row(model.Text("menu.restartwarning"), true);
            else if (count) Row(model.Help(*rows[screen->rowStart + screen->helpIndex]), true);
            else Row("", true);
        }
        Row(model.Text("menu.back"), false);
        auto count = Count(screen->list) - 1;
        screen->selected = std::min(screen->selected, size_t(std::max(0, count - 1)));
        auto rowData = Read<void*>(screen->list, 108);
        auto chosen = int(screen->selected + 1);
        while (chosen < Count(screen->list) - 1 && Read<uint8_t>(rowData, size_t(chosen) * 60 + 53)) ++chosen;
        screen->selected = size_t(chosen - 1);
        select(screen->list, nullptr, int16_t(chosen), 1, 1);
    }
    inline void Action(int direction)
    {
        if (!screen) return;
        auto pageToken = screen->page;
        auto selected = Read<int16_t>(screen->list, 168);
        if (selected < 1 || selected >= Count(screen->list)) return;
        if (screen->details)
        {
            if (direction) StepDetails(direction);
            else if (DetailPages() > 1 && selected == int(DetailLines + 1)) StepDetails(1);
            else EndDetails();
            return;
        }
        screen->selected = size_t(selected - 1);
        if (screen->descriptorPage != SIZE_MAX)
        {
            auto rows = model.VisibleRows(screen->descriptorPage);
            if (!screen || screen->page != pageToken) return;
            auto count = std::min(PageRows, rows.size() - std::min(screen->rowStart, rows.size()));
            if (screen->selected < count)
            {
                model.Change(*rows[screen->rowStart + screen->selected], direction, screen->host.pause);
                if (!screen || screen->page != pageToken) return;
            }
            else if (!direction)
            {
                auto at = screen->selected - count;
                if (screen->rowStart && !at) screen->rowStart -= PageRows;
                else
                {
                    if (screen->rowStart) --at;
                    const bool next = screen->rowStart + count < rows.size();
                    if (next && !at) screen->rowStart += PageRows;
                    else
                    {
                        if (next) --at;
                        if (count && !at) { BeginDetails(); return; }
                        screen->descriptorPage = SIZE_MAX; screen->rowStart = 0;
                    }
                }
                screen->selected = 0;
            }
        }
        else if (!direction)
        {
            auto links = Links(), at = screen->selected;
            if (at < links) { screen->descriptorPage = screen->rootStart + at; screen->rowStart = 0; screen->selected = 0; model.PageShown(screen->descriptorPage); }
            else
            {
                at -= links;
                if (screen->rootStart && !at) { screen->rootStart -= RootLinks; screen->selected = 0; }
                else
                {
                    if (screen->rootStart) --at;
                    if (screen->rootStart + links < model.Pages().size() && !at) { screen->rootStart += RootLinks; screen->selected = 0; }
                    else { Close(); return; }
                }
            }
        }
        Populate();
    }
    inline bool ListContract(void* list, bool ownsBindings = false)
    {
        if (!list || Count(list) < 0 || !Read<void*>(list, 4) || !Read<void*>(list, 44) || !Read<void*>(list, 40)) return false;
        // Three style blocks own their font strings but borrow package prototypes.
        // The native destructor frees +52/+72/+92, never +44/+64/+84.
        // Loaded resource lists discard those binding strings in 10D66E20;
        // constructor-created private lists keep their independently owned ones.
        // StyleBind explicitly accepts a null definition with an empty string.
        for (auto offset : { 36u, 56u, 76u })
            if ((ownsBindings && !Read<void*>(list, offset + 16)) ||
                (Read<void*>(list, offset + 8) && !Read<void*>(list, offset + 4))) return false;
        return true;
    }
    inline void Open(Host host)
    {
        if (screen) Close();
        auto source = hosts.find(host.list);
        if (source == hosts.end() || source->second.serial != host.serial ||
            (host.entryPage && !CTSettings::Lookup(host.entryPage, host.entrySerial)))
        { LogOnce("native source host changed while closing previous page"); return; }
        if (!ListContract(host.list)) { LogOnce("native list/template contract unavailable"); return; }
        auto templates = std::make_shared<CTLayout::Templates>();
        templates->pages[0] = CTLayout::Clone(Read<void*>(host.list, 44), 500);
        if (!templates->pages[0]) { LogOnce("native wider row template contract unavailable"); return; }
        auto page = makePage(0), item = makeItem(nullptr, nullptr, 10);
        if (!page || !item)
        {
            if (page) DestroyPage(page);
            if (item) DestroyPage(item);
            LogOnce("native page/item allocation failed"); return;
        }
        auto list = Read<void*>(item, 8);
        if (!list) { DestroyPage(item); DestroyPage(page); LogOnce("native list allocation failed"); return; }
        if (!ListContract(list, true) && !Read<void*>(list, 44))
        {
            // A new list has no borrowed prototypes until the styles are bound.
            for (auto offset : { 36u, 56u, 76u })
            {
                if (!Read<void*>(host.list, offset + 8)) continue;
                Set<void*>(list, offset + 4, Read<void*>(host.list, offset + 4));
                bindStyle(static_cast<uint8_t*>(list) + offset, nullptr,
                    Read<void*>(host.list, offset + 12), offset == 36 ? templates->pages[0] : Read<void*>(host.list, offset + 8));
            }
        }
        auto sourcePoint = Read<void*>(host.list, 4), targetPoint = Read<void*>(list, 4);
        Set<uint32_t>(targetPoint, 4, Read<uint32_t>(sourcePoint, 4));
        Set<uint32_t>(targetPoint, 8, uint16_t(70) | (uint32_t(110) << 16));
        Set<int8_t>(list, 164, Read<int8_t>(host.list, 164));
        Set<uint8_t>(list, 166, 12);
        Set<uint32_t>(list, 176, Read<uint32_t>(host.list, 176));
        Set<uint8_t>(list, 180, uint8_t(Read<uint8_t>(host.list, 180) | 0x42));
        Set<uint8_t>(list, 184, Read<uint8_t>(host.list, 184));
        Set<uint32_t>(item, 4, crc32(0, "WFP.AdvancedOptions.List", 24));
        Set<uint32_t>(page, 20, crc32(0, "WFP.AdvancedOptions", 19));
        addChild(page, nullptr, item); // Native page now owns wrapper/list.
        auto stack = getStack(host.manager, nullptr);
        if (!ListContract(list, true) || !stack || !RowMetrics(list).font)
        { DestroyPage(page); LogOnce("native private page style/stack contract failed"); return; }
        const auto controlsJoined = CTControls::JoinedFor(host.manager);
        const auto displayJoined = CTDisplay::JoinedFor(host.manager);
        auto joined = controlsJoined ? CTControls::Settings : std::set<std::string>{};
        if (displayJoined) joined.insert(CTDisplay::Settings.begin(), CTDisplay::Settings.end());
        model.OpenPlugins(module, Language(), 4096, joined);
        auto currentHost = hosts.find(host.list);
        if (currentHost == hosts.end() || currentHost->second.serial != host.serial ||
            controlsJoined != CTControls::JoinedFor(host.manager) ||
            displayJoined != CTDisplay::JoinedFor(host.manager) ||
            (host.entryPage && !CTSettings::Lookup(host.entryPage, host.entrySerial)))
        {
            // Plugin discovery/Subscribe/Get may synchronously unload/rebuild
            // the native source package before our page enters its stack.
            model.Close(); DestroyPage(page); LogOnce("native source host changed during model discovery"); return;
        }
        screen.emplace(); screen->host = host; screen->page = page; screen->item = item; screen->list = list; screen->stack = stack; screen->templates = std::move(templates);
        auto current = [&]
        {
            auto binding = hosts.find(host.list);
            return screen && screen->page == page && binding != hosts.end() && binding->second.serial == host.serial &&
                (!host.entryPage || CTSettings::Lookup(host.entryPage, host.entrySerial));
        };
        auto abandon = [&]
        {
            if (!screen || screen->page != page) return;
            if (InStack(screen->stack, page)) popHook.thiscall<void>(screen->stack, char(0), char(0));
            if (screen && screen->page == page) Retire();
        };
        BusyScope scope; Populate();
        if (!current()) { abandon(); return; }
        resetFocus(page);
        if (!current()) { abandon(); return; }
        displayHook.thiscall<void>(host.manager, page, char(1), char(1), char(0));
        if (!current()) { abandon(); return; }
        if (!InStack(stack, page)) { abandon(); LogOnce("native stack rejected private page"); return; }
    }
    inline void Entry(void* controller, bool pause)
    {
        if (!ready) return;
        auto list = Read<void*>(controller, pause ? 80 : 68), parent = Read<void*>(controller, 56);
        auto count = Count(list);
        if (!parent || !ListContract(list) || count != (pause ? 6 : 7))
        {
            LogOnce(pause ? "pause menu layout unsupported" : "title menu layout unsupported");
            return;
        }
        Host host{ controller, Read<void*>(controller, 20), parent, list, -1, pause };
        host.serial = ++nextHostSerial;
        hosts[list] = host;
    }

    inline int __fastcall TitleBuild(void* object, void*)
    { auto result = titleBuildHook.thiscall<int>(object); Entry(object, false); return result; }
    inline int __fastcall PauseBuild(void* object, void*)
    { auto result = pauseBuildHook.thiscall<int>(object); Entry(object, true); return result; }
    inline bool SettingsCurrent(void* page, void* manager)
    {
        auto stack = getStack(manager, nullptr);
        if (!stack) return false;
        const auto begin = Read<uintptr_t>(stack, 4), end = Read<uintptr_t>(stack, 8);
        return begin && end > begin && end - begin <= 8 * 64 && (end - begin) % 8 == 0 &&
            *reinterpret_cast<void**>(end - 8) == page;
    }
    inline void OpenSettings(void* controller, void* page)
    {
        auto manager = Read<void*>(controller, 20);
        if (!SettingsCurrent(page, manager)) return;
        std::optional<Host> source;
        for (const auto& [list, host] : hosts)
        {
            if (host.manager != manager) continue;
            if (source) { LogOnce("Settings source template is ambiguous"); return; }
            source = host;
        }
        if (!source) { LogOnce("Settings source template unavailable"); return; }
        auto entry = CTSettings::bindings.find(page);
        if (entry == CTSettings::bindings.end() || !CTSettings::Live(entry->second)) return;
        source->entryPage = page;
        source->entrySerial = entry->second.serial;
        Open(*source);
    }
    inline std::optional<Host> NoticeHost()
    {
        if (!ready || busy || screen || bPlayingVideo)
            return {};
        for (const auto& [list, host] : hosts)
            if (SettingsCurrent(host.parent, host.manager))
                return host;
        return {};
    }
    inline bool NoticeReady()
    {
        return NoticeHost().has_value();
    }
    inline void ShowNotice(const std::wstring& text)
    {
        const auto host = NoticeHost();
        if (!host)
            return;
        Open(*host);
        if (!screen)
            return;
        const auto metrics = RowMetrics(screen->list);
        const auto styled = CTText::Style(text, metrics, Wide(Language()).c_str());
        auto lines = WrapDetails(styled, metrics.width, [&](const auto& part)
        {
            return metrics.Measure(part);
        });
        if (lines.empty())
        {
            Close();
            return;
        }
        BusyScope scope;
        screen->details = true;
        screen->notice = true;
        screen->detailsLines = std::move(lines);
        screen->detailsPage = 0;
        PopulateDetails();
    }

    inline void RetireSettings(void* page)
    {
        CTControls::Forget(page);
        CTDisplay::Forget(page);
        if (!screen || screen->host.entryPage != page) return;
        auto privatePage = screen->page;
        BusyScope scope;
        if (InStack(screen->stack, privatePage))
            popHook.thiscall<void>(screen->stack, char(0), char(0));
        if (screen && screen->page == privatePage) Retire();
    }
    inline char __fastcall Accept(void* item, void*)
    {
        if (CTDisplay::Accept(item)) return 1;
        if (screen && item == screen->item) { BusyScope scope; Action(0); return 1; }
        for (const auto& [object, host] : hosts)
        {
            if (host.entry >= 0 && Read<void*>(item, 8) == host.list && Read<int16_t>(host.list, 168) == host.entry)
            { Open(host); return screen ? 1 : 0; }
        }
        return acceptHook.thiscall<char>(item);
    }
    inline char __fastcall Cancel(void* item, void*)
    {
        if (CTDisplay::Cancel(item)) return 1;
        if (screen && item == screen->item)
        {
            BusyScope scope;
            if (screen->details) EndDetails();
            else if (screen->descriptorPage == SIZE_MAX) Close();
            else { screen->descriptorPage = SIZE_MAX; screen->rowStart = 0; screen->selected = 0; Populate(); }
            return 1;
        }
        return cancelHook.thiscall<char>(item);
    }
    inline char __fastcall Key(void* list, void*, void* item, uint32_t* event)
    {
        if (CTControls::Key(list, item, event)) return 1;
        if (screen && list == screen->list && event && *event == VK_F1)
        { BusyScope scope; if (screen->details) EndDetails(); else BeginDetails(); return 1; }
        if (screen && list == screen->list && event && (*event == VK_LEFT || *event == VK_RIGHT))
        { BusyScope scope; Action(*event == VK_LEFT ? -1 : 1); return 1; }
        return keyHook.thiscall<char>(list, item, event);
    }
    inline char __fastcall Display(void* manager, void*, void* page, char show, char modal, char priority)
    {
        CTDisplay::Leaving(manager, page);
        // busy protects input lifetime, not the native navigation invariant:
        // foreign callbacks must dismiss ours even while Accept/hover unwinds.
        if (screen && manager == screen->host.manager && page != screen->page) Close();
        return displayHook.thiscall<char>(manager, page, show, modal, priority);
    }
    inline void __fastcall Pop(void* stack, void*, char all, char priority)
    {
        popHook.thiscall<void>(stack, all, priority);
        if (screen && stack == screen->stack && !InStack(stack, screen->page)) Retire();
    }
    inline void __fastcall Push(void* stack, void*, void* page, char modal, char priority)
    {
        // Cover direct native stack users as well as DisplayMenu. Our page is
        // always topmost until native navigation dismisses it.
        if (screen && stack == screen->stack && page != screen->page) Close();
        pushHook.thiscall<void>(stack, page, modal, priority);
    }
    inline int __fastcall PageDestroyed(void* page, void*)
    {
        CTSettings::Forget(page);
        RetireSettings(page);
        // Unlink before the source package frees the prototypes borrowed by our
        // list. The independently owned stack is destroyed before its package
        // pages (UserContext dtor); StackDestroyed drains our pages at that gate.
        std::erase_if(hosts, [page](const auto& pair) { return pair.second.parent == page; });
        if (screen && screen->host.parent == page)
        {
            auto privatePage = screen->page;
            BusyScope scope;
            if (InStack(screen->stack, screen->page))
                popHook.thiscall<void>(screen->stack, char(0), char(0));
            if (screen && screen->page == privatePage) Retire();
        }
        return destructorHook.thiscall<int>(page);
    }
    inline int __fastcall StackDestroyed(void* stack, void*)
    {
        CTSettings::ForgetStack(stack);
        auto inputUnwinding = busy;
        BusyScope scope;
        if (screen && screen->stack == stack) Retire();
        std::erase_if(retired, [stack, inputUnwinding](RetiredPage& old)
        {
            if (old.stack != stack) return false;
            // An invoked sibling action may tear down menus from Accept.
            // Keep its list alive until input unwinds, but forget the stack
            // that the native destructor is about to release.
            if (inputUnwinding) { old.stack = nullptr; return false; }
            DestroyPage(old.page); return true;
        });
        return stackDestructorHook.thiscall<int>(stack);
    }
    inline void Tick(void* page)
    {
        Frame(page);
        if (!busy && screen && page == screen->page && !screen->details)
        {
            auto selected = Read<int16_t>(screen->list, 168);
            auto moved = selected > 0 && screen->selected != size_t(selected - 1);
            if (moved) screen->selected = size_t(selected - 1);
            if (moved || RWGTAMenu::Model::Changed().exchange(false)) { BusyScope scope; Populate(); }
        }
        if (!busy) { CTControls::Tick(page); CTDisplay::Tick(page); }
    }
    inline void Init();
}

// Native function signatures used by the Plugins page.
export namespace CTMenu
{
    inline void Init()
    {
        if (ready) return;
        struct Rollback
        {
            ~Rollback()
            {
                if (ready) return;
                CTDisplay::defaultsHook.reset(); CTDisplay::ready = false;
                CTControls::defaultsHook.reset(); CTControls::rebuildHook.reset(); CTControls::ready = false;
                CTSettings::initHook.reset(); CTSettings::destructorHook.reset(); CTSettings::tickHook.reset(); CTSettings::ready = false;
                titleBuildHook.reset(); pauseBuildHook.reset(); acceptHook.reset(); cancelHook.reset();
                keyHook.reset(); displayHook.reset(); popHook.reset(); destructorHook.reset(); stackDestructorHook.reset(); pushHook.reset();
            }
        } rollback;
        std::map<std::string, void*> found;
        auto MenuMouseMove = hook::pattern("8B 51 04 85 D2 56 8B 74 24 08 8B 46 04 89 41 16 74 0E 8B 49 08 8B C1 2B C2 C1 F8 03 85 C0 75 06 32 C0 5E C2 04 00 8B 49 F8 56 E8 41 E4 00 00 5E");
        moveMouse = reinterpret_cast<decltype(moveMouse)>(MenuMouseMove.get_first());
        auto NativeLanguageCaller = hook::pattern("53 55 57 8B BC 24 24 02 00 00 85 FF 75 07 E8 3F AD 00 00 8B F8 8B 9C 24 20 02 00 00 8B AC 24 1C");
        getLanguage = reinterpret_cast<decltype(getLanguage)>(injector::GetBranchDestination(NativeLanguageCaller.get_first(14)).as_int());
        auto StackPush = hook::pattern("83 EC 08 53 56 8B F1 8B 4E 04 85 C9 57 75 04 33 FF EB 20 8B 7E 08 2B F9 C1 FF 03 85 FF 7E 14 8D");
        found["StackPush"] = StackPush.get_first();
        auto UserContextDtor = hook::pattern("56 8B F1 57 8B 7E 14 85 FF C7 06 ? ? ? ? 74 10 8B CF E8 98 00 00 00 57 E8 32 03 00 00 83 C4");
        // The native context destroys its stack first, then its package pages.
        found["StackDtor"] = injector::GetBranchDestination(UserContextDtor.get_first(19)).as_int() ?
            reinterpret_cast<void*>(injector::GetBranchDestination(UserContextDtor.get_first(19)).as_int()) : nullptr;
        auto PageFactory = hook::pattern("8B 44 24 04 83 F8 04 77 7F FF 24 85 ? ? ? ? 6A 24 E8 B9 F7 FF FF 83 C4 04 85 C0 74 6A 8B C8");
        found["PageFactory"] = PageFactory.get_first();
        auto ItemFactory = hook::pattern("53 8B 5C 24 08 83 FB 0C 56 57 8B F9 0F 87 D1 00 00 00 0F B6 83 ? ? ? ? FF 24 85 ? ? ? ?");
        found["ItemFactory"] = ItemFactory.get_first();
        auto StyleBind = hook::pattern("83 EC 20 8B 44 24 24 53 56 8B F1 8B 4E 10 57 8B 7C 24 34 33 DB 85 C9 89 5C 24 0C 89 46 0C 89 7E");
        found["StyleBind"] = StyleBind.get_first();
        auto Append = hook::pattern("8B 44 24 08 83 EC 3C 55 8B 6C 24 44 56 57 50 83 EC 1C 8B FC C7 47 18 07 00 00 00 C7 47 14 00 00");
        found["Append"] = Append.get_first();
        auto Clear = hook::pattern("51 53 56 8B F1 8B 46 6C 57 8D 7E 68 33 DB 3B C3 74 1C 8B 4C 24 0C 8B 57 08 51 57 52 50 E8 AE DF");
        found["Clear"] = Clear.get_first();
        auto Select = hook::pattern("51 53 55 56 8B F1 8B 46 6C 57 66 8B BE A8 00 00 00 66 85 FF 0F 94 C3 85 C0 74 18 8B 4E 70 2B C8");
        found["Select"] = Select.get_first();
        auto AddChild = hook::pattern("83 C1 04 56 8B 71 04 85 F6 75 04 33 D2 EB 08 8B 51 08 2B D6 C1 FA 02 85 F6 74 23 8B 41 0C 2B C6 C1 F8 02 3B D0 73 17 8B 41 08 85 C0 74 06 8B 54 24 08 89 10 83 C0 04 89 41 08 5E C2 04 00 8B 41 08 8D 54 24 08 52 6A 01 50 E8 92 FB FF FF 5E C2 04 00 CC CC CC CC CC CC CC CC CC CC CC CC CC CC");
        found["AddChild"] = AddChild.get_first();
        auto GetStack = hook::pattern("8B 41 0C 50 E8 17 6C 00 00 8B C8 E8 F0 71 00 00 C3 CC CC CC CC CC CC CC CC CC CC CC CC CC CC CC");
        found["GetStack"] = GetStack.get_first();
        auto DisplayMenu = hook::pattern("51 53 8B 5C 24 0C 85 DB C7 44 24 04 00 00 00 00 75 07 32 C0 ? ? ? ? 00 8B 41 0C 56 50 E8 4D");
        found["DisplayMenu"] = DisplayMenu.get_first();
        auto StackPop = hook::pattern("83 EC 0C 53 55 56 57 8B F1 C6 44 24 13 00 8B FF 8B 46 04 85 C0 75 04 33 C9 EB 08 8B 4E 08 2B C8");
        found["StackPop"] = StackPop.get_first();
        auto ItemAccept = hook::pattern("F6 41 4C 02 74 05 E9 65 FC FF FF 32 C0 C3 CC CC F6 41 4C 02 74 05 E9 95 FC FF FF 32 C0 C3 CC CC");
        found["ItemAccept"] = ItemAccept.get_first();
        auto ItemCancel = hook::pattern("F6 41 4C 02 74 05 E9 95 FC FF FF 32 C0 C3 CC CC 8B 44 24 08 8B 54 24 04 50 51 8B 4C 24 10 51 52");
        found["ItemCancel"] = ItemCancel.get_first();
        auto KeyPress = hook::pattern("53 8B 5C 24 0C 56 57 8B 7C 24 10 8B F1 53 8B CF E8 8B 81 00 00 F6 86 B4 00 00 00 02 0F 84 36 01");
        found["KeyPress"] = KeyPress.get_first();
        auto PageDtor = hook::pattern("53 56 8B F1 8B 46 0C 57 8B 7E 08 33 DB 3B F8 C7 06 ? ? ? ? 74 16 8B 0F 3B CB 74 06 8B 01 6A");
        found["PageDtor"] = PageDtor.get_first();
        auto ResetFocus = hook::pattern("51 53 55 56 8B 74 24 14 32 DB 33 ED 57 89 6C 24 10 89 6E 24 8B 4E 08 85 C9 75 04 33 C0 EB 08 8B");
        found["ResetFocus"] = ResetFocus.get_first();
        auto PauseBuilder = hook::pattern("56 6A 01 68 ? ? ? ? 68 ? ? ? ? 8B F1 8B 46 38 68 ? ? ? ? 68 ? ? ? ? 50 E8 1E B3");
        found["PauseBuilder"] = PauseBuilder.get_first();
        auto Title72 = hook::pattern("83 EC 0C 53 55 56 57 6A 01 68 ? ? ? ? 68 ? ? ? ? 8B F1 8B 46 38 68 ? ? ? ? 68 ? ? ? ? 50 E8 08 96 01 00 6A 01 6A 00 6A 00 68 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 FE 38");
        found["Title72"] = Title72.get_first();
        auto ListCtor = hook::pattern("53 56 8B F1 8D 46 0C 89 46 04 C7 46 08 ? ? ? ? C7 06 ? ? ? ? C7 46 08 ? ? ? ? C7 00 ? ? ? ? 8B 0D ? ? ? ? 89 48 04 C7 00 ? ? ? ? 8B 15 ? ? ? ? 89 50 08 33 DB 8D 4E");
        found["ListCtor"] = ListCtor.get_first();
        auto ListDtor = hook::pattern("51 53 56 8B F1 8B 46 7C 33 DB 3B C3 57 74 09 50 E8 6B D5 DA FF 83 C4 04 8D 7E 68 89 5E 7C 89 9E 80 00 00 00 89 9E 84 00 00 00 8B 47 04 3B C3 74 1C 8B 4C 24 0C 8B 57 08 51 57 52 50 E8 6F D9 FF");
        found["ListDtor"] = ListDtor.get_first();
        auto StyleCtor = hook::pattern("56 8B F1 6A 00 6A 20 C7 06 ? ? ? ? C7 46 08 00 00 00 00 C7 46 0C 00 00 00 00 E8 F0 4D DB FF 83 C4 08 85 C0 74 17 8B C8 E8 72 D0 03 00 6A 00 8B CE 89 46 10 E8 16 C0 00 00 8B C6 5E C3 33 C0");
        found["StyleCtor"] = StyleCtor.get_first();
        auto NativeMemory = hook::pattern("8B 44 24 08 85 C0 75 05 B8 ? ? ? ? 8B 0D ? ? ? ? 8B 11 50 8B 44 24 08 50 FF 12 C3 CC CC 8B 0D ? ? ? ? 8B 54");
        CTLayout::allocate = reinterpret_cast<decltype(CTLayout::allocate)>(NativeMemory.get_first());
        CTLayout::release = reinterpret_cast<decltype(CTLayout::release)>(NativeMemory.get_first(32));
        makePage = reinterpret_cast<decltype(makePage)>(found["PageFactory"]);
        makeItem = reinterpret_cast<decltype(makeItem)>(found["ItemFactory"]);
        bindStyle = reinterpret_cast<decltype(bindStyle)>(found["StyleBind"]);
        append = reinterpret_cast<decltype(append)>(found["Append"]);
        clear = reinterpret_cast<decltype(clear)>(found["Clear"]);
        select = reinterpret_cast<decltype(select)>(found["Select"]);
        addChild = reinterpret_cast<decltype(addChild)>(found["AddChild"]);
        getStack = reinterpret_cast<decltype(getStack)>(found["GetStack"]);
        resetFocus = reinterpret_cast<decltype(resetFocus)>(found["ResetFocus"]);
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&module), &module) || !module)
        {
            throw std::runtime_error("Native host module unavailable");
        }
        titleBuildHook = safetyhook::create_inline(found["Title72"], TitleBuild);
        if (!titleBuildHook)
        {
            throw std::runtime_error("Native hook failed: Title72");
        }
        pauseBuildHook = safetyhook::create_inline(found["PauseBuilder"], PauseBuild);
        if (!pauseBuildHook)
        {
            throw std::runtime_error("Native hook failed: PauseBuilder");
        }
        acceptHook = safetyhook::create_inline(found["ItemAccept"], Accept);
        if (!acceptHook)
        {
            throw std::runtime_error("Native hook failed: ItemAccept");
        }
        cancelHook = safetyhook::create_inline(found["ItemCancel"], Cancel);
        if (!cancelHook)
        {
            throw std::runtime_error("Native hook failed: ItemCancel");
        }
        keyHook = safetyhook::create_inline(found["KeyPress"], Key);
        if (!keyHook)
        {
            throw std::runtime_error("Native hook failed: KeyPress");
        }
        displayHook = safetyhook::create_inline(found["DisplayMenu"], Display);
        if (!displayHook)
        {
            throw std::runtime_error("Native hook failed: DisplayMenu");
        }
        popHook = safetyhook::create_inline(found["StackPop"], Pop);
        if (!popHook)
        {
            throw std::runtime_error("Native hook failed: StackPop");
        }
        destructorHook = safetyhook::create_inline(found["PageDtor"], PageDestroyed);
        if (!destructorHook)
        {
            throw std::runtime_error("Native hook failed: PageDtor");
        }
        stackDestructorHook = safetyhook::create_inline(found["StackDtor"], StackDestroyed);
        if (!stackDestructorHook)
        {
            throw std::runtime_error("Native hook failed: StackDtor");
        }
        pushHook = safetyhook::create_inline(found["StackPush"], Push);
        if (!pushHook)
        {
            throw std::runtime_error("Native hook failed: StackPush");
        }
        CTSettings::makeItem = makeItem;
        CTSettings::bindStyle = bindStyle;
        CTSettings::addChild = addChild;
        CTSettings::getStack = getStack;
        CTSettings::caption = SettingsCaption;
        CTSettings::stockFits = SettingsTextFits;
        CTSettings::current = SettingsCurrent;
        CTSettings::open = OpenSettings;
        CTSettings::retire = RetireSettings;
        CTSettings::frame = Tick;
        CTSettings::log = LogOnce;
        if (!CTSettings::Init())
            throw std::runtime_error("Native Settings Plugins bridge unavailable");
        CTSettings::controllerRetired = [](void* controller) { CTControls::ControllerRetired(controller); CTDisplay::ControllerRetired(controller); };
        CTControls::module = module;
        CTControls::language = Language;
        CTControls::wide = Wide;
        CTControls::metrics = RowMetrics;
        CTControls::select = select;
        CTControls::log = LogOnce;
        if (!CTControls::Init())
            throw std::runtime_error("Native joined Controls bridge unavailable");
        CTDisplay::module = module;
        CTDisplay::language = Language;
        CTDisplay::wide = Wide;
        CTDisplay::metrics = PrototypeMetrics;
        CTDisplay::append = append;
        CTDisplay::select = select;
        if (!CTDisplay::Init())
            throw std::runtime_error("Native joined Display bridge unavailable");
        NativeMenuTick = nullptr;
        NativeWindowsMenuMouse = WindowsMenuMouse;
        NativeWindowsMenuHelp = WindowsMenuHelp;
        ready = true;
    }
}
