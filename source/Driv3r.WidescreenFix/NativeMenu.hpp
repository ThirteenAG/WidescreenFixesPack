#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <cstring>
#include <cmath>
#include <set>

// Update 2 MEC screens, controls and font service. Native input owns selection/repeat/hit testing;
// API callbacks run only after native screen input returns. Unsupported executables fail closed.
namespace Driv3rMenu
{
    using Bytes = std::vector<uint8_t>;
    template<class T> T& At(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset); }
    template<class F> F Virtual(void* p, size_t offset) { return reinterpret_cast<F>(At<uintptr_t>(At<void*>(p, 0), offset)); }
    inline uintptr_t base;
    inline HMODULE module;
    inline bool ready;
    inline uint32_t processDepth = 0;
    inline SafetyHookInline loadHook, processHook, destroyHook, releaseHook, textHook, switchHook, activateHook, deactivateHook;
    inline void** gui, **font, **language;
    inline std::unordered_map<void*, std::shared_ptr<struct Host>> hosts;

    struct Text
    {
        std::array<uint8_t, 120> bytes{};
        std::wstring value;
        float width = 0.4f;
        float height = 0.044f;
        bool fullValue = false;
        bool fitted = false;
        std::wstring cachedValue, formatted;
        std::array<uint32_t, 13> cachedStyle{};
        void* cachedFont = nullptr;
        float fittedScale = 1, fittedVerticalScale = 1;
    };
    struct Button
    {
        std::array<uint8_t, 452> bytes{};
        std::vector<Bytes> artwork;
        std::vector<std::unique_ptr<Text>> text;
        std::array<uintptr_t, 12> vtable{};
        size_t row = 0;
        bool entry = false;
    };
    struct Item { std::string label, value; RWGTAMenu::Row row; int link = -1; bool enabled = true; };
    struct Host
    {
        void* root = nullptr;
        void* parent = nullptr;
        uint32_t originalScreens = 0, originalRows = 0;
        uint32_t first = 0, last = 0;
        std::array<uint8_t, 8> firstUp{}, lastDown{};
        std::array<uint8_t, 312> page{};
        std::unique_ptr<Button> entry;
        std::vector<std::unique_ptr<Button>> buttons;
        std::unique_ptr<Text> heading, description;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        size_t modelPage = 0, rootOffset = 0, rowStart = 0;
        bool pause = false, retired = false, open = false, busy = false;
        int pending = 0;
        int menuId = 0;
        size_t pendingRow = 0;
    };
    inline std::unordered_map<void*, std::pair<std::weak_ptr<Host>, Text*>> texts;
    inline std::unordered_map<void*, std::pair<std::weak_ptr<Host>, Button*>> buttons;

    inline void Log(const char* message)
    {
        static std::set<std::string> logged;
        if (!logged.insert(message).second)
        {
            return;
        }
        OutputDebugStringA((std::string("Driv3r native menu: ") + message + "\n").c_str());
    }
    inline std::wstring Wide(const std::string& value)
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        std::wstring result(count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
        if (result.size() > 256) { result.resize(256); if (result.back() >= 0xD800 && result.back() <= 0xDBFF) result.pop_back(); }
        return result;
    }
    inline std::wstring Format(const std::wstring& value)
    {
        std::wstring result;
        for (auto c : value) { if (c == L'%') result += L'%'; result += c < 32 || c == 8232 || c == 8233 ? L' ' : c; }
        return result;
    }
    inline std::string Language()
    {
        auto chosen = FixSettings().String("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        static const char* tags[] = { "en", "fr", "de", "es", "it", "ja", "zh-CN", "zh-TW", "ko", "pt" };
        if (!language || !*language || At<uintptr_t>(*language, 0) != base + 0x38BE50) return "en";
        int index = Virtual<int(__thiscall*)(void*)>(*language, 80)(*language);
        return index >= 0 && index < static_cast<int>(std::size(tags)) ? tags[index] : "en";
    }
    inline bool Current(const std::shared_ptr<Host>& host)
    {
        auto found = hosts.find(host->root);
        return !host->retired && found != hosts.end() && found->second == host;
    }
    inline bool Active(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open || !gui || !*gui || At<int>(*gui, 33572) != host->menuId) return false;
        auto selected = At<uint32_t>(host->root, 16664);
        return selected < At<uint32_t>(host->root, 16660) && At<void*>(host->root, 16460 + selected * 4) == host->page.data();
    }
    inline std::array<uint32_t, 13> Style(Text& text)
    {
        std::array<uint32_t, 13> style{};
        style[0] = At<uint32_t>(text.bytes.data(), 20); style[1] = At<uint32_t>(text.bytes.data(), 24);
        style[2] = At<uint32_t>(text.bytes.data(), 28); style[3] = At<uint32_t>(text.bytes.data(), 32);
        style[4] = 0; style[5] = Virtual<uint32_t(__thiscall*)(void*)>(*gui, 32)(*gui);
        style[6] = At<uint32_t>(text.bytes.data(), 4); style[7] = At<uint32_t>(text.bytes.data(), 8);
        // Native SetPosition converts width to a wrapping bound. Measure an unwrapped line and
        // fit it explicitly; stock widths would hide overflow by returning a multi-line max width.
        At<float>(style.data(), 32) = 65536.0f; style[9] = At<uint32_t>(text.bytes.data(), 48);
        style[10] = At<uint32_t>(text.bytes.data(), 52); style[11] = At<uint32_t>(text.bytes.data(), 44);
        return style;
    }
    inline float Measure(std::array<uint32_t, 13>& style, const std::wstring& value, float maximumHeight = 0.044f, float* height = nullptr)
    {
        float extent[2]{};
        auto formatted = Format(value);
        Virtual<void*(__cdecl*)(void*, float*, uint32_t*, const wchar_t*, ...)>(*font, 36)(*font, extent, style.data(), formatted.c_str());
        if (height) *height = extent[1];
        return std::isfinite(extent[0]) && std::isfinite(extent[1]) && extent[1] > 0 && extent[1] <= maximumHeight ? extent[0] : INFINITY;
    }
    inline int __fastcall DrawText(void* self, void*, int render, int bank)
    {
        auto found = texts.find(self);
        if (!ready || found == texts.end()) return textHook.thiscall<int>(self, render, bank);
        auto host = found->second.first.lock();
        if (!host || !Current(host) || !gui || !*gui || !font || !*font ||
            At<uintptr_t>(*gui, 0) != base + 0x381EC8 || At<uintptr_t>(*font, 0) != base + 0x38BD80) return 0;
        auto& text = *found->second.second;
        auto style = Style(text);
        auto nativeFont = At<void*>(*font, 12); // Native style selects font bank zero.
        if (!nativeFont || At<uintptr_t>(nativeFont, 0) != base + 0x38BC28 || !At<uint8_t>(nativeFont, 12) ||
            !At<void*>(nativeFont, 24) || !At<void*>(nativeFont, 28))
        { Log("native font resources not ready"); return 0; }
        // Resource load/release receives the second interface (+4), not this primary font.
        // Its loaded flag +8 and glyph pointer +20 therefore belong at +12 and +24 here.
        auto glyphData = At<void*>(nativeFont, 24);
        if (text.fitted && text.cachedValue == text.value && text.cachedStyle == style && text.cachedFont == glyphData)
        {
            At<float>(style.data(), 36) = text.fittedScale;
            At<float>(style.data(), 40) = text.fittedVerticalScale;
            return Virtual<int(__cdecl*)(void*, int, uint32_t*, const wchar_t*, ...)>(*font, 24)(*font, render, style.data(), text.formatted.c_str());
        }
        auto originalStyle = style;
        auto value = text.value;
        if (value.empty()) return 0;
        if (!std::isfinite(text.width) || text.width <= 0 || !std::isfinite(At<float>(style.data(), 36)) ||
            !std::isfinite(At<float>(style.data(), 40)) || At<float>(style.data(), 36) <= 0 || At<float>(style.data(), 40) <= 0)
        { Log("native font style contract rejected"); return 0; }
        float lineHeight = 0;
        float measured = Measure(style, value, INFINITY, &lineHeight);
        if (!std::isfinite(measured) || measured <= 0) { Log("native font extent contract rejected"); return 0; }
        if (lineHeight > text.height)
        {
            At<float>(style.data(), 40) *= text.height / lineHeight;
            measured = Measure(style, value, text.height + 0.00001f);
            if (!std::isfinite(measured) || measured <= 0) { Log("native line height fitting rejected"); return 0; }
        }
        if (text.fullValue && measured > text.width && measured > 0)
        {
            for (int attempt = 0; attempt < 4 && measured > text.width; ++attempt)
            {
                At<float>(style.data(), 36) *= text.width / measured;
                measured = Measure(style, value, text.height + 0.00001f);
                if (!std::isfinite(measured) || measured <= 0) { Log("native value extent contract rejected"); return 0; }
            }
            if (measured > text.width + 0.00001f) { Log("native value fitting rejected"); return 0; }
        }
        else if (measured > text.width)
        {
            if (Measure(style, L"...", text.height + 0.00001f) > text.width) { Log("native ellipsis extent rejected"); return 0; }
            while (!value.empty() && Measure(style, value + L"...", text.height + 0.00001f) > text.width)
            {
                value.pop_back();
                if (!value.empty() && value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back();
            }
            value += L"...";
        }
        auto formatted = Format(value);
        text.cachedValue = text.value; text.cachedStyle = originalStyle; text.cachedFont = glyphData;
        text.formatted = formatted; text.fittedScale = At<float>(style.data(), 36); text.fittedVerticalScale = At<float>(style.data(), 40); text.fitted = true;
        return Virtual<int(__cdecl*)(void*, int, uint32_t*, const wchar_t*, ...)>(*font, 24)(*font, render, style.data(), formatted.c_str());
    }
    inline void Populate(const std::shared_ptr<Host>& host);
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack);
    inline int __fastcall Input(void* self, void*, int action, uint32_t* state)
    {
        auto found = buttons.find(self);
        if (found == buttons.end()) return 0;
        auto host = found->second.first.lock();
        auto button = found->second.second;
        if (!host || !Current(host)) return 0;
        state[5] = 0; state[6] = static_cast<uint32_t>(-1);
        if (button->entry)
        {
            if (action == 6) { host->pending = 6; host->pendingRow = button->row; }
            else reinterpret_cast<int(__thiscall*)(void*, int, uint32_t*)>(base + 0x670E0)(self, action, state);
            return 0;
        }
        if (action == 2 || action == 3)
        {
            auto count = static_cast<int>(host->items.size());
            if (count) { state[5] = 1; state[6] = static_cast<uint32_t>((static_cast<int>(button->row) + count + (action == 2 ? -1 : 1)) % count); }
        }
        else if (action >= 4 && action <= 6) { host->pending = action; host->pendingRow = button->row; }
        return 0;
    }
    inline std::unique_ptr<Text> CloneText(const std::shared_ptr<Host>& host, void* source, float x, float y, float width, bool full = false)
    {
        auto text = std::make_unique<Text>();
        memcpy(text->bytes.data(), source, 120);
        At<float>(text->bytes.data(), 4) = x; At<float>(text->bytes.data(), 8) = y;
        At<uint32_t>(text->bytes.data(), 44) = 0; // Native left alignment, independent of the unwrapped width.
        text->width = width; text->fullValue = full;
        texts[text->bytes.data()] = { host, text.get() };
        return text;
    }
    inline std::unique_ptr<Button> CloneButton(const std::shared_ptr<Host>& host, void* source, float x, float y, size_t row, bool entry)
    {
        auto button = std::make_unique<Button>();
        memcpy(button->bytes.data(), source, 452); button->row = row; button->entry = entry;
        memcpy(button->vtable.data(), At<void*>(source, 0), button->vtable.size() * 4);
        button->vtable[3] = reinterpret_cast<uintptr_t>(&Input);
        At<void*>(button->bytes.data(), 0) = button->vtable.data();
        float dx = x - At<float>(source, 4), dy = y - At<float>(source, 8);
        At<float>(button->bytes.data(), 4) = x; At<float>(button->bytes.data(), 8) = y;
        At<float>(button->bytes.data(), 12) = entry ? 0.45f : 0.84f;
        At<uint32_t>(button->bytes.data(), 36) = 1;
        At<uint8_t>(button->bytes.data(), 448) = 0;
        for (size_t offset = 192; offset <= 416; offset += 32)
            strcpy_s(reinterpret_cast<char*>(button->bytes.data() + offset), 32, "<NoAction>");
        for (int state = 0; state < 4; ++state)
        {
            auto count = At<uint32_t>(source, 56 + state * 20);
            if (count > 4) return nullptr;
            size_t index = 0;
            for (size_t child = 0; child < count; ++child)
            {
                if (index >= 4) return nullptr;
                auto native = At<void*>(source, 40 + state * 20 + child * 4);
                auto vt = At<uintptr_t>(native, 0);
                if (vt == base + 0x381D90)
                {
                    auto text = CloneText(host, native, At<float>(native, 4) + dx, At<float>(native, 8) + dy, entry ? 0.44f : 0.45f);
                    At<void*>(button->bytes.data(), 40 + state * 20 + index++ * 4) = text->bytes.data();
                    button->text.push_back(std::move(text));
                    if (!entry && index < 4)
                    {
                        auto value = CloneText(host, native, x + 0.5f, At<float>(native, 8) + dy, 0.32f, true);
                        At<void*>(button->bytes.data(), 40 + state * 20 + index++ * 4) = value->bytes.data();
                        button->text.push_back(std::move(value));
                    }
                }
                else if (vt == base + 0x381DC0)
                {
                    Bytes bitmap(56); memcpy(bitmap.data(), native, 56);
                    At<float>(bitmap.data(), 4) += dx; At<float>(bitmap.data(), 8) += dy;
                    At<void*>(button->bytes.data(), 40 + state * 20 + index++ * 4) = bitmap.data();
                    button->artwork.push_back(std::move(bitmap));
                }
                else return nullptr;
                if (index > 4) return nullptr;
            }
            At<uint32_t>(button->bytes.data(), 56 + state * 20) = static_cast<uint32_t>(index);
        }
        if (!entry)
        {
            // Stock state 2 hides some unavailable rows entirely. Our disabled settings remain
            // visible, using dimmed copies of the same native unselected text controls.
            uint32_t count = 0;
            for (uint32_t i = 0; i < At<uint32_t>(button->bytes.data(), 76); ++i)
            {
                auto child = At<void*>(button->bytes.data(), 60 + i * 4);
                auto found = texts.find(child);
                if (found == texts.end()) continue;
                auto original = found->second.second;
                auto dim = CloneText(host, child, At<float>(child, 4), At<float>(child, 8), original->width, original->fullValue);
                for (auto offset : { 20u, 24u, 28u }) At<float>(dim->bytes.data(), offset) *= 0.5f;
                At<void*>(button->bytes.data(), 80 + count++ * 4) = dim->bytes.data();
                button->text.push_back(std::move(dim));
            }
            At<uint32_t>(button->bytes.data(), 96) = count;
        }
        buttons[button->bytes.data()] = { host, button.get() };
        return button;
    }
    inline void SetText(Button& button, const std::string& label, const std::string& value)
    {
        for (auto& text : button.text) text->value = Wide(text->fullValue ? value : label);
    }
    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Active(host)) return;
        std::vector<Item> items;
        auto& model = host->model;
        std::string title = model.Text("menu.plugins");
        auto pages = model.Pages();
        if (!host->modelPage)
        {
            if (host->rootOffset >= pages.size()) host->rootOffset = 0;
            for (size_t i = host->rootOffset; i < pages.size() && items.size() < 4; ++i) items.push_back({ pages[i].title, "", {}, static_cast<int>(i + 1) });
            if (host->rootOffset) items.push_back({ model.Text("menu.previous"), "", {}, -3 });
            if (host->rootOffset + 4 < pages.size()) items.push_back({ model.Text("menu.next"), "", {}, -2 });
        }
        else if (host->modelPage <= pages.size())
        {
            size_t page = host->modelPage - 1;
            title = pages[page].title;
            auto rows = model.VisibleRows(page);
            if (!Active(host)) return;
            if (host->rowStart >= rows.size()) host->rowStart = 0;
            for (size_t i = host->rowStart; i < rows.size() && i < host->rowStart + 4; ++i)
            {
                auto row = *rows[i];
                bool enabled = model.Enabled(row, host->pause);
                if (!Active(host)) return;
                auto value = model.Value(row);
                if (!Active(host)) return;
                auto label = model.Label(row);
                if (!Active(host)) return;
                items.push_back({ std::move(label), std::move(value), row, -1, enabled });
            }
            if (host->rowStart) items.push_back({ model.Text("menu.previous"), "", {}, -5 });
            if (host->rowStart + 4 < rows.size()) items.push_back({ model.Text("menu.next"), "", {}, -6 });
        }
        if (!Active(host)) return;
        items.push_back({ model.Text("menu.back"), "", {}, -4 });
        host->items = std::move(items);
        host->heading->value = Wide(title + (model.RestartRequired() ? " - " + model.Text("menu.restartwarning") : ""));
        host->description->value.clear();
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            auto& item = host->items[i];
            auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            SetText(*host->buttons[i], item.label, value);
            At<void*>(host->page.data(), 100 + i * 4) = host->buttons[i]->bytes.data();
        }
        At<void*>(host->page.data(), 100 + host->items.size() * 4) = host->heading->bytes.data();
        At<void*>(host->page.data(), 104 + host->items.size() * 4) = host->description->bytes.data();
        At<uint32_t>(host->page.data(), 300) = static_cast<uint32_t>(host->items.size() + 2);
        if (At<uint32_t>(host->page.data(), 304) >= host->items.size()) At<uint32_t>(host->page.data(), 304) = 0;
        for (size_t i = 0; i < host->buttons.size(); ++i)
            At<uint32_t>(host->buttons[i]->bytes.data(), 36) = i < host->items.size() && !host->items[i].enabled ? 2 : 1;
        auto selected = At<uint32_t>(host->page.data(), 304);
        if (!host->items[selected].enabled)
            for (size_t i = 0; i < host->items.size(); ++i) if (host->items[i].enabled) { selected = static_cast<uint32_t>(i); break; }
        At<uint32_t>(host->page.data(), 304) = selected;
        At<uint32_t>(host->buttons[selected]->bytes.data(), 36) = 0;
        RWGTAMenu::Model::Changed() = false;
    }
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack)
    {
        if (nativeBack && Active(host)) Virtual<int(__thiscall*)(void*)>(host->root, 48)(host->root);
        host->open = false; host->model.Close(); host->items.clear(); host->pending = 0;
    }
    inline void Action(const std::shared_ptr<Host>& host)
    {
        int action = std::exchange(host->pending, 0);
        if (!action || !Current(host) || host->busy) return;
        struct BusyScope { Host& host; explicit BusyScope(Host& value) : host(value) { host.busy = true; } ~BusyScope() { host.busy = false; } } busy(*host);
        if (!host->open)
        {
            auto selected = At<uint32_t>(host->root, 16664);
            if (selected >= host->originalScreens || At<void*>(host->root, 16460 + selected * 4) != host->parent) return;
            auto menuId = At<int>(*gui, 33572);
            host->model.OpenPlugins(module, Language(), 4096);
            if (Current(host) && host->model.IsOpen() && At<int>(*gui, 33572) == menuId &&
                At<uint32_t>(host->root, 16664) == selected && At<void*>(host->root, 16460 + selected * 4) == host->parent)
            {
                host->menuId = menuId;
                host->open = true; host->modelPage = host->rootOffset = host->rowStart = 0;
                // The native named-page switch pushes the native parent onto its own Back stack.
                Virtual<char(__thiscall*)(void*, const char*)>(host->root, 40)(host->root, "WFPMenu");
                if (Active(host)) { Populate(host); }
                else Close(host, false);
            }
            else Close(host, false);
        }
        else if (Active(host) && host->pendingRow < host->items.size())
        {
            auto item = host->items[host->pendingRow];
            if (item.link == -1)
            {
                host->model.Change(item.row, action == 4 ? -1 : action == 5 ? 1 : 0, host->pause);
            }
            else if (action == 6)
            {
                if (item.link == -4) { if (host->modelPage) { host->model.PageShown(host->modelPage - 1); host->modelPage = host->rowStart = 0; } else Close(host, true); }
                else if (item.link == -3) host->rootOffset = host->rootOffset >= 4 ? host->rootOffset - 4 : 0;
                else if (item.link == -2) host->rootOffset += 4;
                else if (item.link == -5) host->rowStart = host->rowStart >= 4 ? host->rowStart - 4 : 0;
                else if (item.link == -6) host->rowStart += 4;
                else if (item.link > 0) { host->rowStart = 0; host->modelPage = static_cast<size_t>(item.link); host->model.PageShown(host->modelPage - 1); }
            }
            if (Active(host)) Populate(host);
        }
    }
    inline void Retire(const std::shared_ptr<Host>& host)
    {
        // Release precedes asynchronous native resource destruction: remove every borrowed UI reference now.
        auto parentIndex = uint32_t(0);
        for (uint32_t i = 0; i < host->originalScreens; ++i)
            if (At<void*>(host->root, 16460 + i * 4) == host->parent) parentIndex = i;
        if (At<uint32_t>(host->root, 16664) == host->originalScreens) At<uint32_t>(host->root, 16664) = parentIndex;
        auto head = static_cast<uint8_t*>(host->root) + 17072;
        auto node = At<void*>(host->root, 17072);
        for (uint32_t i = 0, count = At<uint32_t>(host->root, 17068); node != head && i < count; ++i)
        {
            if (At<uint32_t>(node, 8) == host->originalScreens) At<uint32_t>(node, 8) = parentIndex;
            node = At<void*>(node, 0);
        }
        At<void*>(host->root, 16460 + host->originalScreens * 4) = nullptr;
        At<uint32_t>(host->root, 16660) = host->originalScreens;
        At<uint32_t>(host->parent, 300) = host->originalRows;
        if (At<uint32_t>(host->parent, 304) >= host->originalRows)
        {
            At<uint32_t>(host->parent, 304) = host->first;
            At<uint32_t>(At<void*>(host->parent, 100 + host->first * 4), 36) = 0;
        }
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, 100 + host->first * 4)) + 128, host->firstUp.data(), 8);
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, 100 + host->last * 4)) + 136, host->lastDown.data(), 8);
        At<void*>(host->parent, 100 + host->originalRows * 4) = nullptr;
        host->retired = true; host->open = false; host->model.Close();
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.lock() == host) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.lock() == host) i = buttons.erase(i); else ++i; }
        hosts.erase(host->root);
    }
    inline bool Prepare(void* root)
    {
        auto refuse = [](const char* reason) { Log(reason); return false; };
        if (!gui || !*gui) return refuse("native MEC preparation: GUI service not ready");
        if (!font || !*font) return refuse("native MEC preparation: font service not ready");
        if (At<uintptr_t>(root, 0) != base + 0x381E48) return refuse("native MEC preparation: root class differs");
        if (At<uintptr_t>(*gui, 0) != base + 0x381EC8) return refuse("native MEC preparation: GUI class differs");
        if (At<uintptr_t>(*font, 0) != base + 0x38BD80) return refuse("native MEC preparation: font class differs");
        auto count = At<uint32_t>(root, 16660);
        if (count != 36 && count != 14)
        {
            Log(("native MEC preparation: unsupported screen count " + std::to_string(count)).c_str());
            return false;
        }
        auto host = std::make_shared<Host>(); host->root = root; host->pause = count == 14; host->originalScreens = count;
        for (size_t i = 0; i < count; ++i)
        {
            auto screen = At<void*>(root, 16460 + i * 4);
            if (!strncmp(static_cast<char*>(screen), host->pause ? "Main" : "Options", 7)) { host->parent = screen; break; }
        }
        if (!host->parent) return refuse("native MEC preparation: Options/Main page missing");
        host->originalRows = At<uint32_t>(host->parent, 300);
        if (host->originalRows != (host->pause ? 18u : 11u))
        {
            Log(("native MEC preparation: unsupported parent row count " + std::to_string(host->originalRows)).c_str());
            return false;
        }
        host->first = host->pause ? 4 : 1; host->last = host->pause ? 6 : 2;
        auto first = At<void*>(host->parent, 100 + host->first * 4), last = At<void*>(host->parent, 100 + host->last * 4);
        if (At<uintptr_t>(first, 0) != base + 0x381D20 || At<uintptr_t>(last, 0) != base + 0x381D20) return refuse("native MEC preparation: navigation button class differs");
        auto templateButton = At<void*>(host->parent, host->pause ? 100 : 104);
        if (At<uintptr_t>(templateButton, 0) != base + 0x381D20) return refuse("native MEC preparation: template button class differs");
        for (auto offset : { 4u, 8u, 12u, 16u }) if (!std::isfinite(At<float>(templateButton, offset))) return refuse("native MEC preparation: template geometry is non-finite");
        if (At<float>(templateButton, 12) <= 0 || At<float>(templateButton, 16) <= 0 || At<float>(templateButton, 16) >= 0.045f) return refuse("native MEC preparation: template geometry is outside native row bounds");
        host->entry = CloneButton(host, templateButton, host->pause ? 0.075f : 0.3875f, host->pause ? 0.583333f : 0.716667f, host->originalRows, true);
        if (!host->entry || host->entry->text.empty()) return false;
        auto languageTag = Language(); FixSettings().GetTexts().Select(languageTag);
        SetText(*host->entry, FixSettings().GetTexts().Resolve("@menu.plugins"), "");
        memcpy(host->page.data(), host->parent, 84);
        strcpy_s(reinterpret_cast<char*>(host->page.data()), 8, "WFPMenu");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 17), 32, "<NoAction>");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 49), 32, "<NoAction>");
        At<uint8_t>(host->page.data(), 16) = At<uint8_t>(host->page.data(), 81) = 0;
        At<uint32_t>(host->page.data(), 12) = At<uint32_t>(host->page.data(), 304) = 0;
        for (size_t i = 0; i < 7; ++i)
        {
            auto button = CloneButton(host, templateButton, 0.075f, 0.22f + static_cast<float>(i) * 0.045f, i, false);
            if (!button || button->text.empty()) return false;
            host->buttons.push_back(std::move(button));
        }
        auto textTemplate = host->entry->text.front()->bytes.data();
        host->heading = CloneText(host, textTemplate, 0.075f, 0.12f, 0.84f);
        host->description = CloneText(host, textTemplate, 0.075f, 0.64f, 0.84f);
        host->heading->height = host->description->height = 0.06f;
        // Native input dereferences selected before validating count. Keep a valid placeholder even
        // during native enter callbacks, before model discovery/first population completes.
        At<void*>(host->page.data(), 100) = host->buttons[0]->bytes.data();
        At<uint32_t>(host->page.data(), 300) = 1;
        memcpy(host->firstUp.data(), static_cast<uint8_t*>(first) + 128, 8);
        memcpy(host->lastDown.data(), static_cast<uint8_t*>(last) + 136, 8);
        // Insert one entry into the existing up/down cycle, retaining all vanilla rows and actions.
        At<uint8_t>(first, 128) = 0; At<uint32_t>(first, 132) = host->originalRows;
        At<uint8_t>(last, 136) = 0; At<uint32_t>(last, 140) = host->originalRows;
        At<uint8_t>(host->entry->bytes.data(), 128) = At<uint8_t>(host->entry->bytes.data(), 136) = 0;
        At<uint32_t>(host->entry->bytes.data(), 132) = host->last; At<uint32_t>(host->entry->bytes.data(), 140) = host->first;
        At<void*>(host->parent, 100 + host->originalRows * 4) = host->entry->bytes.data();
        At<uint32_t>(host->parent, 300) = host->originalRows + 1;
        At<void*>(root, 16460 + count * 4) = host->page.data(); At<uint32_t>(root, 16660) = count + 1;
        hosts[root] = host;
        return true;
    }
    inline int __fastcall Load(void* self, void*, int mode, void* assets, int index)
    {
        int result = loadHook.thiscall<int>(self, mode, assets, index);
        if (ready && !Prepare(static_cast<uint8_t*>(self) - 4)) Log("native MEC host contract unsupported");
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.expired()) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.expired()) i = buttons.erase(i); else ++i; }
        return result;
    }
    inline char __fastcall Process(void* self, void*, uint32_t* state)
    {
        auto found = hosts.find(self);
        auto host = found == hosts.end() ? nullptr : found->second;
        bool active = host && Active(host);
        char result;
        {
            struct InputScope { InputScope() { ++processDepth; } ~InputScope() { --processDepth; } } nativeInput;
            result = processHook.thiscall<char>(self, state);
        }
        if (!host || !Current(host)) return result;
        if (processDepth || host->busy) return result;
        if (active && !Active(host))
        {
            auto selected = At<uint32_t>(host->root, 16664);
            bool parent = selected < host->originalScreens && At<void*>(host->root, 16460 + selected * 4) == host->parent;
            if ((state[5] & 8) && parent && host->modelPage && At<int>(*gui, 33572) == host->menuId)
            {
                // Native Back already popped exactly one page. Inner settings return to our root;
                // pushing again preserves the native parent/selection for the final Back.
                host->model.PageShown(host->modelPage - 1);
                host->modelPage = host->rowStart = 0;
                Virtual<char(__thiscall*)(void*, const char*)>(host->root, 40)(host->root, "WFPMenu");
                if (Active(host)) Populate(host); else Close(host, false);
            }
            else Close(host, false);
        }
        Action(host);
        if (Active(host) && RWGTAMenu::Model::Changed()) Populate(host);
        if (Active(host))
        {
            auto selected = At<uint32_t>(host->page.data(), 304);
            if (selected < host->items.size())
            {
                const auto row = host->items[selected].row;
                host->description->value = Wide(host->model.Help(row));
            }
        }
        return result;
    }
    inline int __fastcall Destroy(void* self, void*)
    {
        auto found = hosts.find(self);
        auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep) Retire(keep);
        return destroyHook.thiscall<int>(self);
    }
    inline int __fastcall Release(void* self, void*)
    {
        // Resource interface is the second base (+4); its virtual release runs before arrays disappear.
        auto found = hosts.find(static_cast<uint8_t*>(self) - 4);
        auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep) Retire(keep);
        return releaseHook.thiscall<int>(self);
    }
    inline char __fastcall Switch(void* self, void*, const char* name)
    {
        auto found = hosts.find(self);
        auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep && Active(keep) && (!name || _strnicmp(name, "WFPMenu", 7))) Close(keep, true);
        return switchHook.thiscall<char>(self, name);
    }
    inline int __fastcall Activate(void* self, void*, int id)
    {
        std::vector<std::shared_ptr<Host>> keep;
        for (const auto& [root, host] : hosts) if (Active(host) && host->menuId != id) keep.push_back(host);
        for (const auto& host : keep) if (Active(host)) Close(host, true);
        return activateHook.thiscall<int>(self, id);
    }
    inline int __fastcall Deactivate(void* self, void*, int id)
    {
        std::vector<std::shared_ptr<Host>> keep;
        for (const auto& [root, host] : hosts) if (Active(host) && host->menuId == id) keep.push_back(host);
        for (const auto& host : keep) if (Active(host)) Close(host, true);
        return deactivateHook.thiscall<int>(self, id);
    }
    inline void Init()
    {
        base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Init), &module);
        const auto dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 4096) { Log("native image header guard failed"); return; }
        const auto pe = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || pe->OptionalHeader.SizeOfImage < 0x4AC3B8)
        { Log("native image range guard failed"); return; }
        auto guard = [](uintptr_t rva, std::initializer_list<uint8_t> bytes) { return !memcmp(reinterpret_cast<void*>(base + rva), bytes.begin(), bytes.size()); };
        if (!guard(0x67CA0, { 0x6A,0xFF,0x68,0x47,0x17,0x6D,0x00,0x64,0xA1 }) ||
            !guard(0x67A00, { 0x53,0x55,0x56,0x8B,0x74,0x24,0x10,0x57,0x8B,0xF9,0x8D,0x5F,0x10 }) ||
            !guard(0x675C0, { 0x6A,0xFF,0x68,0x8F,0x3B,0x6D,0x00,0x64,0xA1 }) ||
            !guard(0x68270, { 0x8B,0x89,0xB4,0x42,0x00,0x00,0xA1,0x54,0xC2,0x8A,0x00 }) ||
            !guard(0x66C20, { 0x83,0xEC,0x58,0x53,0x8B,0xD9,0xC7,0x44,0x24,0x08,0x00,0x00,0x80,0x3F }) ||
            !guard(0x176150, { 0x55,0x8B,0xEC,0x83,0xE4,0xF8,0x83,0xEC,0x14,0x8B,0x4D,0x08,0x57 }) ||
            !guard(0x176080, { 0x8B,0x4C,0x24,0x04,0x57,0x8B,0x7C,0x24,0x10 })) { Log("native menu executable guard failed"); return; }
        if (!guard(0x670E0, { 0x8B,0x44,0x24,0x04,0x53,0x8B,0x5C,0x24,0x0C,0x55,0x83,0xC0,0xFE }) ||
            !guard(0xD59E0, { 0x8B,0x41,0x20,0xC3 }) || !guard(0xD76B0, { 0x33,0xC0,0xC3 })) { Log("native control/language guard failed"); return; }
        if (!guard(0x67880, { 0x51,0x53,0x55,0x8B,0xA9,0x14,0x41,0x00,0x00,0x56,0x33,0xF6 }) ||
            !guard(0x68D60, { 0x83,0xEC,0x08,0x53,0x56,0x57,0x8B,0xF9,0x8D,0x5F,0x08,0x8D,0x4C,0x24,0x18,0x8D,0x44,0x24,0x0C,0x8B,0xD3,0xE8,0x16,0x06 }) ||
            !guard(0x68DF0, { 0x83,0xEC,0x08,0x53,0x56,0x57,0x8B,0xF9,0x8D,0x5F,0x08,0x8D,0x4C,0x24,0x18,0x8D,0x44,0x24,0x0C,0x8B,0xD3,0xE8,0x86,0x05 })) { Log("native lifecycle guard failed"); return; }
        const uintptr_t buttonMethods[] = { 0x68160,0x67020,0x66A00,0x670E0,0x671C0,0x1F070,0x67200,0x67210,0x67220,0x67230,0x67260,0x669F0 };
        const uintptr_t textMethods[] = { 0x680F0,0x66C20,0x66BE0,0x1154F0,0x1154F0,0x18EA80,0x18EA80,0x18EA80,0x1E0830,0x669C0,0x120E80,0xD76B0 };
        for (size_t i = 0; i < std::size(buttonMethods); ++i)
            if (At<uintptr_t>(reinterpret_cast<void*>(base + 0x381D20), i * 4) != base + buttonMethods[i] ||
                At<uintptr_t>(reinterpret_cast<void*>(base + 0x381D90), i * 4) != base + textMethods[i]) { Log("native control vtable guard failed"); return; }
        gui = reinterpret_cast<void**>(base + 0x4AC3B0); font = reinterpret_cast<void**>(base + 0x4AC254); language = reinterpret_cast<void**>(base + 0x4AC248);
        destroyHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x675C0), &Destroy);
        releaseHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x68270), &Release);
        textHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x66C20), &DrawText);
        processHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x67A00), &Process);
        switchHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x67880), &Switch);
        activateHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x68D60), &Activate);
        deactivateHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x68DF0), &Deactivate);
        loadHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0x67CA0), &Load);
        ready = destroyHook && releaseHook && textHook && processHook && loadHook && switchHook && activateHook && deactivateHook;
        if (!ready) { loadHook.reset(); deactivateHook.reset(); activateHook.reset(); switchHook.reset(); processHook.reset(); textHook.reset(); releaseHook.reset(); destroyHook.reset(); Log("native menu hook transaction failed"); }
    }
}
