#pragma once
#include "../Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <cstring>
#include <cmath>
#include <set>

// 7001 MEC screens, controls and font service. Native input owns selection/repeat/hit testing;
// API callbacks run only after native screen input returns. Unsupported executables fail closed.
namespace DPLMenu
{
    using Bytes = std::vector<uint8_t>;
    template<class T> T& At(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset); }
    template<class F> F Virtual(void* p, size_t offset) { return reinterpret_cast<F>(At<uintptr_t>(At<void*>(p, 0), offset)); }
    inline uintptr_t base;
    inline HMODULE module;
    inline bool ready;
    inline uint32_t processDepth = 0;
    inline SafetyHookInline loadHook, processHook, releaseHook, pushHook, textHook, switchHook, activateHook, deactivateHook;
    inline void** gui, **font, **language;
    inline std::unordered_map<void*, std::shared_ptr<struct Host>> hosts;

    struct Text
    {
        std::array<uint8_t, 172> bytes{};
        std::wstring value;
        float width = 0.4f;
        float height = 0.044f;
        bool fullValue = false;
        bool centered = false;
        bool fitted = false;
        std::wstring cachedValue, formatted;
        std::array<uint32_t, 14> cachedStyle{};
        void* cachedFont = nullptr;
        std::array<float, 3> cachedGeometry{};
        float fittedScale = 1, fittedVerticalScale = 1;
        std::array<uintptr_t, 31> vtable{};
    };
    struct Button
    {
        std::array<uint8_t, 736> bytes{};
        std::vector<Bytes> artwork;
        std::vector<std::unique_ptr<Text>> text;
        std::array<uintptr_t, 32> vtable{};
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
        std::array<uint8_t, 396> page{};
        std::unique_ptr<Button> entry;
        std::vector<std::unique_ptr<Button>> buttons;
        std::unique_ptr<Text> heading, description;
        std::vector<Bytes> decorations;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        size_t modelPage = 0, rootOffset = 0;
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
        OutputDebugStringA((std::string("DPL native menu: ") + message + "\n").c_str());
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
        // The raw native glyph layout route treats plugin strings literally, including % and #.
        auto result = value;
        for (auto& c : result) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        return result;
    }
    inline std::string Language()
    {
        const auto& selected = FixSettings().String("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        static const char* tags[] = { "en", "fr", "de", "es", "it", "ja", "zh-CN", "zh-TW", "ko", "pt" };
        if (!language || !*language || At<uintptr_t>(*language, 0) != base + 0x251A58) return "en";
        int index = Virtual<int(__thiscall*)(void*)>(*language, 84)(*language);
        return index >= 0 && index < static_cast<int>(std::size(tags)) ? tags[index] : "en";
    }
    inline bool Current(const std::shared_ptr<Host>& host)
    {
        auto found = hosts.find(host->root);
        return !host->retired && found != hosts.end() && found->second == host;
    }
    inline bool Active(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open || !gui || !*gui || At<int>(*gui, 736) != host->menuId) return false;
        auto selected = At<uint32_t>(host->root, 856);
        return selected < At<uint32_t>(host->root, 272) && At<void*>(host->root, 16 + selected * 4) == host->page.data();
    }
    inline std::array<uint32_t, 14> Style(Text& text)
    {
        std::array<uint32_t, 14> style{};
        for (size_t i = 0; i < 4; ++i) style[i] = At<uint32_t>(text.bytes.data(), 20 + i * 4);
        style[5] = Virtual<uint32_t(__thiscall*)(void*)>(*gui, 52)(*gui);
        style[6] = At<uint32_t>(text.bytes.data(), 4); style[7] = At<uint32_t>(text.bytes.data(), 8);
        At<float>(style.data(), 32) = 65536.0f;
        style[9] = At<uint32_t>(text.bytes.data(), 72); style[10] = At<uint32_t>(text.bytes.data(), 76);
        style[11] = At<uint32_t>(text.bytes.data(), 80);
        style[12] = 0; // Literal text with independently fitted columns.
        return style;
    }
    inline void* SetStyle(std::array<uint32_t, 14>& style)
    {
        return reinterpret_cast<void*(__thiscall*)(void*, uint32_t*)>(base + 0x16DB6B)(*font, style.data());
    }
    inline float Measure(std::array<uint32_t, 14>& style, const std::wstring& value, float maximumHeight = INFINITY, float* height = nullptr)
    {
        auto native = SetStyle(style);
        auto literal = Format(value);
        reinterpret_cast<void(__thiscall*)(void*, const wchar_t*, bool)>(base + 0x16E85F)(native, literal.c_str(), false);
        float width = At<float>(native, 128), h = At<float>(native, 132);
        At<uint32_t>(native, 16936) = At<uint32_t>(native, 8740) = 0;
        if (height) *height = h;
        return std::isfinite(width) && std::isfinite(h) && h > 0 && h <= maximumHeight ? width : INFINITY;
    }
    inline int __fastcall DrawText(void* self, void*, int render, int bank)
    {
        auto found = texts.find(self);
        if (!ready || found == texts.end()) return textHook.thiscall<int>(self, render, bank);
        auto host = found->second.first.lock();
        if (!host || !Current(host) || !gui || !*gui || !font || !*font ||
            At<uintptr_t>(*gui, 0) != base + 0x24ACC0 || At<uintptr_t>(*font, 0) != base + 0x2508C0) return 0;
        auto nativeFont = At<void*>(*font, 12);
        if (!nativeFont || At<uintptr_t>(nativeFont, 0) != base + 0x2509B8 || !At<uint8_t>(nativeFont, 12) ||
            !At<void*>(nativeFont, 24) || !At<void*>(nativeFont, 28)) return 0;
        for (auto offset : {8408u,8412u,8416u})
            if (!std::isfinite(At<float>(nativeFont, offset)) || At<float>(nativeFont, offset) <= 0) return 0;
        const std::array<float, 3> geometry = {At<float>(nativeFont, 8408), At<float>(nativeFont, 8412), At<float>(nativeFont, 8416)};
        auto& text = *found->second.second;
        auto style = Style(text), original = style;
        auto glyphData = At<void*>(nativeFont, 24); // RegisterData receives the second base (+4).
        if (text.value.empty()) return 0;
        if (!std::isfinite(text.width) || text.width <= 0 || !std::isfinite(At<float>(style.data(), 36)) ||
            !std::isfinite(At<float>(style.data(), 40)) || At<float>(style.data(), 36) <= 0 || At<float>(style.data(), 40) <= 0) return 0;
        if (!text.fitted || text.cachedValue != text.value || text.cachedStyle != style || text.cachedFont != glyphData || text.cachedGeometry != geometry)
        {
            auto value = text.value;
            float height = 0, measured = Measure(style, value, INFINITY, &height);
            if (!std::isfinite(measured) || measured <= 0) { Log("native font extent rejected"); return 0; }
            if (height > text.height) At<float>(style.data(), 40) *= text.height / height;
            if (text.fullValue && measured > text.width) At<float>(style.data(), 36) *= text.width / measured;
            else if (!text.fullValue && measured > text.width)
            {
                if (Measure(style, L"...", text.height * 1.001f) > text.width) return 0;
                do { value.pop_back(); if (!value.empty() && value.back() >= 0xD800 && value.back() <= 0xDBFF) value.pop_back(); }
                while (!value.empty() && Measure(style, value + L"...", text.height * 1.001f) > text.width);
                value += L"...";
            }
            text.cachedValue = text.value; text.cachedStyle = original; text.cachedFont = glyphData;
            text.cachedGeometry = geometry;
            text.fittedScale = At<float>(style.data(), 36); text.fittedVerticalScale = At<float>(style.data(), 40);
            text.formatted = Format(value); text.fitted = true;
        }
        At<float>(style.data(), 36) = text.fittedScale; At<float>(style.data(), 40) = text.fittedVerticalScale;
        if (text.centered)
        {
            const auto measured = Measure(style, text.formatted);
            if (!std::isfinite(measured) || measured > text.width) return 0;
            At<float>(style.data(), 24) += (text.width - measured) * 0.5f;
        }
        auto native = SetStyle(style);
        reinterpret_cast<void(__thiscall*)(void*, const wchar_t*, bool)>(base + 0x16E85F)(native, text.formatted.c_str(), false);
        int result = reinterpret_cast<int(__thiscall*)(void*, int)>(base + 0x16E54C)(native, render);
        At<uint32_t>(native, 16936) = At<uint32_t>(native, 8740) = 0;
        return result;
    }
    inline void __fastcall TextState(void*, void*, int) {}
    inline void Populate(const std::shared_ptr<Host>& host);
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack);
    inline int __fastcall Input(void* self, void*, int action, uint32_t* state, char orientation)
    {
        auto found = buttons.find(self);
        if (found == buttons.end()) return 0;
        auto host = found->second.first.lock(); auto button = found->second.second;
        if (!host || !Current(host)) return 0;
        state[5] = 0; state[6] = static_cast<uint32_t>(-1);
        if (button->entry)
        {
            if (action == 9) { host->pending = 9; host->pendingRow = button->row; }
            else reinterpret_cast<int(__thiscall*)(void*, int, uint32_t*, char)>(base + 0xAEE50)(self, action, state, orientation);
            return 0;
        }
        if (action == 5 || action == 6)
        {
            auto count = static_cast<int>(host->items.size());
            if (count) { state[5] = 1; state[6] = static_cast<uint32_t>((static_cast<int>(button->row) + count + (action == 5 ? -1 : 1)) % count); }
        }
        else if (action >= 7 && action <= 9) { host->pending = action; host->pendingRow = button->row; }
        return 0;
    }
    inline std::unique_ptr<Text> CloneText(const std::shared_ptr<Host>& host, void* source, float x, float y, float width, bool full = false)
    {
        auto text = std::make_unique<Text>(); memcpy(text->bytes.data(), source, text->bytes.size());
        memcpy(text->vtable.data(), At<void*>(source, 0), sizeof(text->vtable));
        text->vtable[24] = reinterpret_cast<uintptr_t>(&TextState);
        At<void*>(text->bytes.data(), 0) = text->vtable.data(); At<void*>(text->bytes.data(), 64) = nullptr;
        At<void*>(text->bytes.data(), 164) = nullptr;
        At<float>(text->bytes.data(), 4) = At<float>(text->bytes.data(), 152) = x;
        At<float>(text->bytes.data(), 8) = At<float>(text->bytes.data(), 156) = y;
        At<uint8_t>(text->bytes.data(), 71) = 0;
        text->width = width; text->fullValue = full;
        texts[text->bytes.data()] = {host, text.get()}; return text;
    }
    inline std::unique_ptr<Button> CloneButton(const std::shared_ptr<Host>& host, void* source, float x, float y, size_t row, bool entry)
    {
        auto button = std::make_unique<Button>(); memcpy(button->bytes.data(), source, button->bytes.size());
        button->row = row; button->entry = entry;
        memcpy(button->vtable.data(), At<void*>(source, 0), sizeof(button->vtable));
        button->vtable[3] = reinterpret_cast<uintptr_t>(&Input);
        At<void*>(button->bytes.data(), 0) = button->vtable.data();
        At<void*>(button->bytes.data(), 64) = nullptr;
        At<float>(button->bytes.data(), 4) = At<float>(button->bytes.data(), 48) = x;
        At<float>(button->bytes.data(), 8) = At<float>(button->bytes.data(), 52) = y;
        At<float>(button->bytes.data(), 12) = entry ? At<float>(source, 12) : 0.84f;
        At<uint32_t>(button->bytes.data(), 68) = 1;
        At<uint32_t>(button->bytes.data(), 708) = 0; At<uint8_t>(button->bytes.data(), 732) = 0;
        for (size_t offset = 320; offset <= 672; offset += 32) strcpy_s(reinterpret_cast<char*>(button->bytes.data() + offset), 32, "<NoAction>");
        for (int state = 0; state < 4; ++state)
        {
            auto count = At<uint32_t>(source, 104 + state * 36);
            if (count > 8) return nullptr;
            size_t index = 0;
            for (size_t child = 0; child < count; ++child)
            {
                auto native = At<void*>(source, 72 + state * 36 + child * 4);
                if (!native || At<uintptr_t>(native, 0) != base + 0x24AFA8 || index + (entry ? 1 : 2) > 8) return nullptr;
                float relativeX = At<float>(native, 152), relativeY = At<float>(native, 156);
                auto text = CloneText(host, native, relativeX, relativeY, entry ? At<float>(source, 12) - relativeX : 0.45f);
                text->centered = entry;
                At<void*>(button->bytes.data(), 72 + state * 36 + index++ * 4) = text->bytes.data(); button->text.push_back(std::move(text));
                if (!entry)
                {
                    auto value = CloneText(host, native, 0.5f, relativeY, 0.32f, true);
                    At<void*>(button->bytes.data(), 72 + state * 36 + index++ * 4) = value->bytes.data(); button->text.push_back(std::move(value));
                }
            }
            At<uint32_t>(button->bytes.data(), 104 + state * 36) = static_cast<uint32_t>(index);
        }
        if (!entry)
        {
            uint32_t count = 0;
            for (uint32_t i = 0; i < At<uint32_t>(button->bytes.data(), 140); ++i)
            {
                auto child = At<void*>(button->bytes.data(), 108 + i * 4);
                auto found = texts.find(child); if (found == texts.end() || count >= 8) return nullptr;
                auto sourceText = found->second.second;
                auto dim = CloneText(host, child, At<float>(child, 152), At<float>(child, 156), sourceText->width, sourceText->fullValue);
                for (auto offset : {20u,24u,28u}) At<float>(dim->bytes.data(), offset) *= 0.5f;
                At<void*>(button->bytes.data(), 144 + count++ * 4) = dim->bytes.data(); button->text.push_back(std::move(dim));
            }
            At<uint32_t>(button->bytes.data(), 176) = count;
        }
        buttons[button->bytes.data()] = {host, button.get()}; return button;
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
        std::string title = model.Text("menu.title");
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
            for (const auto* original : rows)
            {
                auto row = *original;
                bool enabled = model.Enabled(row, host->pause);
                if (!Active(host)) return;
                auto value = model.Value(row);
                if (!Active(host)) return;
                auto label = model.Label(row);
                if (!Active(host)) return;
                items.push_back({ std::move(label), std::move(value), row, -1, enabled });
                if (items.size() == 6) break;
            }
        }
        if (!Active(host)) return;
        items.push_back({ model.Text("menu.back"), "", {}, -4 });
        host->items = std::move(items);
        host->heading->value = Wide(title);
        host->description->value.clear();
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            auto& item = host->items[i];
            auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            SetText(*host->buttons[i], item.label, value);
            At<void*>(host->page.data(), i * 4) = host->buttons[i]->bytes.data();
        }
        At<void*>(host->page.data(), host->items.size() * 4) = host->heading->bytes.data();
        At<void*>(host->page.data(), 4 + host->items.size() * 4) = host->description->bytes.data();
        for (size_t i = 0; i < host->decorations.size(); ++i)
            At<void*>(host->page.data(), (host->items.size() + 2 + i) * 4) = host->decorations[i].data();
        At<uint32_t>(host->page.data(), 256) = static_cast<uint32_t>(host->items.size() + 2 + host->decorations.size());
        if (At<uint32_t>(host->page.data(), 368) >= host->items.size()) At<uint32_t>(host->page.data(), 368) = 0;
        for (size_t i = 0; i < host->buttons.size(); ++i)
            At<uint32_t>(host->buttons[i]->bytes.data(), 68) = i < host->items.size() && !host->items[i].enabled ? 2 : 1;
        auto selected = At<uint32_t>(host->page.data(), 368);
        if (!host->items[selected].enabled)
            for (size_t i = 0; i < host->items.size(); ++i) if (host->items[i].enabled) { selected = static_cast<uint32_t>(i); break; }
        At<uint32_t>(host->page.data(), 368) = selected;
        At<uint32_t>(host->buttons[selected]->bytes.data(), 68) = 0;
        RWGTAMenu::Model::Changed() = false;
    }
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack)
    {
        if (nativeBack && Active(host)) Virtual<int(__thiscall*)(void*, bool)>(host->root, 80)(host->root, true);
        host->open = false; host->model.Close(); host->items.clear(); host->pending = 0;
    }
    inline void Action(const std::shared_ptr<Host>& host)
    {
        int action = std::exchange(host->pending, 0);
        if (!action || !Current(host) || host->busy) return;
        struct BusyScope { Host& host; explicit BusyScope(Host& value) : host(value) { host.busy = true; } ~BusyScope() { host.busy = false; } } busy(*host);
        if (!host->open)
        {
            auto selected = At<uint32_t>(host->root, 856);
            if (selected >= host->originalScreens || At<void*>(host->root, 16 + selected * 4) != host->parent) return;
            auto menuId = At<int>(*gui, 736);
            host->model.OpenPlugins(module, Language(), 6);
            if (Current(host) && host->model.IsOpen() && At<int>(*gui, 736) == menuId &&
                At<uint32_t>(host->root, 856) == selected && At<void*>(host->root, 16 + selected * 4) == host->parent)
            {
                host->menuId = menuId;
                host->open = true; host->modelPage = host->rootOffset = 0;
                // The native named-page switch pushes the native parent onto its own Back stack.
                Virtual<char(__thiscall*)(void*, const char*, bool)>(host->root, 72)(host->root, "WFPMenu", true);
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
                host->model.Change(item.row, action == 7 ? -1 : action == 8 ? 1 : 0, host->pause);
            }
            else if (action == 9)
            {
                if (item.link == -4) { if (host->modelPage) { host->model.PageShown(host->modelPage - 1); host->modelPage = 0; } else Close(host, true); }
                else if (item.link == -3) host->rootOffset = host->rootOffset >= 4 ? host->rootOffset - 4 : 0;
                else if (item.link == -2) host->rootOffset += 4;
                else if (item.link > 0) { host->modelPage = static_cast<size_t>(item.link); host->model.PageShown(host->modelPage - 1); }
            }
            if (Active(host)) Populate(host);
        }
    }
    inline void Retire(const std::shared_ptr<Host>& host)
    {
        auto parentIndex = uint32_t(0);
        for (uint32_t i = 0; i < host->originalScreens; ++i)
            if (At<void*>(host->root, 16 + i * 4) == host->parent) parentIndex = i;
        if (At<uint32_t>(host->root, 856) == host->originalScreens) At<uint32_t>(host->root, 856) = parentIndex;
        if (At<uint32_t>(host->root, 860) == host->originalScreens) At<uint32_t>(host->root, 860) = parentIndex;
        auto head = static_cast<uint8_t*>(host->root) + 672;
        auto node = At<void*>(host->root, 672);
        for (uint32_t i = 0, count = At<uint32_t>(host->root, 668); node != head && i < count; ++i)
        {
            if (At<uint32_t>(node, 8) == host->originalScreens) At<uint32_t>(node, 8) = parentIndex;
            node = At<void*>(node, 0);
        }
        At<void*>(host->root, 16 + host->originalScreens * 4) = nullptr;
        At<uint32_t>(host->root, 272) = host->originalScreens;
        At<uint32_t>(host->parent, 256) = host->originalRows;
        if (At<uint32_t>(host->parent, 368) >= host->originalRows) At<uint32_t>(host->parent, 368) = host->first;
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, host->first * 4)) + 224, host->firstUp.data(), 8);
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, host->last * 4)) + 232, host->lastDown.data(), 8);
        At<void*>(host->parent, host->originalRows * 4) = nullptr;
        host->retired = true; host->open = false; host->model.Close();
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.lock() == host) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.lock() == host) i = buttons.erase(i); else ++i; }
        hosts.erase(host->root);
    }
    inline bool Prepare(void* root)
    {
        auto count = At<uint32_t>(root, 272);
        if (count != 12 && count != 52 && count != 54) return true; // Other native resources are untouched.
        auto reject = [&](const char* reason) { Log((std::string("MEC ") + std::to_string(count) + ": " + reason).c_str()); return false; };
        if (!gui || !*gui || !font || !*font || At<uintptr_t>(root, 0) != base + 0x24B350 ||
            At<uintptr_t>(*gui, 0) != base + 0x24ACC0 || At<uintptr_t>(*font, 0) != base + 0x2508C0) return reject("service/class identity rejected");
        auto host = std::make_shared<Host>(); host->root = root; host->pause = count != 12; host->originalScreens = count;
        for (size_t i = 0; i < count; ++i)
        {
            auto screen = At<void*>(root, 16 + i * 4);
            if (screen && !strncmp(static_cast<char*>(screen) + 260, host->pause ? "Setting" : "Main", 7)) { host->parent = screen; break; }
        }
        if (!host->parent) return reject("Options.Main/Pause.Setting not found");
        host->originalRows = At<uint32_t>(host->parent, 256);
        if (host->originalRows != (host->pause ? 17u : 9u)) return reject("parent row count rejected");
        std::vector<std::pair<float, uint32_t>> candidates;
        for (uint32_t i = 0; i < host->originalRows; ++i)
        {
            auto row = At<void*>(host->parent, i * 4);
            if (row && At<uintptr_t>(row, 0) == base + 0x24B1A8 && At<float>(row, 8) < 0.8f) candidates.emplace_back(At<float>(row, 8), i);
        }
        if (candidates.size() != (host->pause ? 5u : 4u)) return reject("native button category count rejected");
        std::sort(candidates.begin(), candidates.end()); host->first = candidates.front().second; host->last = candidates.back().second;
        auto first = At<void*>(host->parent, host->first * 4), last = At<void*>(host->parent, host->last * 4);
        for (size_t i = 0; i < candidates.size(); ++i)
        {
            auto row = At<void*>(host->parent, candidates[i].second * 4);
            if (At<uint8_t>(row, 224) || At<uint8_t>(row, 232) ||
                At<uint32_t>(row, 228) != candidates[(i + candidates.size() - 1) % candidates.size()].second ||
                At<uint32_t>(row, 236) != candidates[(i + 1) % candidates.size()].second) return reject("native up/down cycle rejected");
        }
        auto templateButton = first;
        for (auto offset : {4u,8u,12u,16u}) if (!std::isfinite(At<float>(templateButton, offset))) return reject("nonfinite native geometry");
        if (At<float>(templateButton, 12) <= 0 || At<float>(templateButton, 16) <= 0 || At<float>(templateButton, 16) > 0.051f) return reject("native geometry rejected");
        host->entry = CloneButton(host, templateButton, At<float>(first, 4), At<float>(last, 8) + 0.0666667f, host->originalRows, true);
        if (!host->entry || host->entry->text.empty()) return reject("entry child/style contract rejected");
        FixSettings().GetTexts().Select(Language()); SetText(*host->entry, FixSettings().GetTexts().Resolve("@menu.title"), "");
        memcpy(host->page.data() + 260, static_cast<uint8_t*>(host->parent) + 260, 136);
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 260), 8, "WFPMenu");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 268), 32, "<NoAction>");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 300), 32, "<NoAction>");
        At<uint32_t>(host->page.data(), 336) = At<uint32_t>(host->page.data(), 368) = 0;
        for (auto offset : {340u,341u,342u,360u,361u,365u,392u,393u}) At<uint8_t>(host->page.data(), offset) = 0;
        At<uint32_t>(host->page.data(), 356) = At<uint32_t>(host->page.data(), 372) = 0;
        for (size_t i = 0; i < 7; ++i)
        {
            auto button = CloneButton(host, templateButton, 0.075f, 0.22f + static_cast<float>(i) * 0.058f, i, false);
            if (!button || button->text.empty()) return reject("private child/style contract rejected");
            host->buttons.push_back(std::move(button));
        }
        auto textTemplate = host->entry->text.front()->bytes.data();
        host->heading = CloneText(host, textTemplate, 0.075f, 0.12f, 0.84f);
        host->description = CloneText(host, textTemplate, 0.075f, 0.68f, 0.84f);
        host->heading->height = host->description->height = 0.06f;
        // Only inert bitmap decoration is cloned; vanilla heading/action text is not duplicated.
        for (size_t i = 0; i < host->originalRows; ++i)
        {
            auto row = At<void*>(host->parent, i * 4);
            if (row && At<uintptr_t>(row, 0) == base + 0x24B0A8)
            {
                Bytes copy(80); memcpy(copy.data(), row, copy.size()); At<void*>(copy.data(), 64) = nullptr;
                host->decorations.push_back(std::move(copy));
            }
        }
        At<void*>(host->page.data(), 0) = host->buttons[0]->bytes.data(); At<uint32_t>(host->page.data(), 256) = 1;
        memcpy(host->firstUp.data(), static_cast<uint8_t*>(first) + 224, 8);
        memcpy(host->lastDown.data(), static_cast<uint8_t*>(last) + 232, 8);
        At<uint8_t>(first, 224) = 0; At<uint32_t>(first, 228) = host->originalRows;
        At<uint8_t>(last, 232) = 0; At<uint32_t>(last, 236) = host->originalRows;
        At<uint8_t>(host->entry->bytes.data(), 224) = At<uint8_t>(host->entry->bytes.data(), 232) = 0;
        At<uint32_t>(host->entry->bytes.data(), 228) = host->last; At<uint32_t>(host->entry->bytes.data(), 236) = host->first;
        At<void*>(host->parent, host->originalRows * 4) = host->entry->bytes.data(); At<uint32_t>(host->parent, 256) = host->originalRows + 1;
        At<void*>(root, 16 + count * 4) = host->page.data(); At<uint32_t>(root, 272) = count + 1;
        hosts[root] = host; return true;
    }
    inline int __fastcall Load(void* self, void*, int mode, void* assets, int index)
    {
        auto root = static_cast<uint8_t*>(self) - 4;
        auto old = hosts.find(root);
        if (old != hosts.end()) { auto keep = old->second; Retire(keep); }
        int result = loadHook.thiscall<int>(self, mode, assets, index);
        if (ready) Prepare(root);
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.expired()) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.expired()) i = buttons.erase(i); else ++i; }
        return result;
    }
    inline char __fastcall Process(void* self, void*, uint32_t* state)
    {
        auto found = hosts.find(self); auto host = found == hosts.end() ? nullptr : found->second;
        bool active = host && Active(host); char result;
        { struct Scope { Scope() { ++processDepth; } ~Scope() { --processDepth; } } depth; result = processHook.thiscall<char>(self, state); }
        if (!host || !Current(host) || processDepth || host->busy) return result;
        if (active && Active(host) && (state[5] & 8) && host->modelPage)
        {
            // Native private exit has no animation or command. Cancel its queued parent pop for
            // inner Back, then repopulate the same page as the Advanced Options root.
            At<uint8_t>(host->root, 876) &= ~0x10u;
            host->model.PageShown(host->modelPage - 1); host->modelPage = 0;
            if (Active(host)) Populate(host);
        }
        else if (active && !Active(host)) Close(host, false);
        Action(host);
        if (Active(host) && RWGTAMenu::Model::Changed()) Populate(host);
        if (Active(host))
        {
            auto selected = At<uint32_t>(host->page.data(), 368);
            if (selected < host->items.size())
            {
                const auto row = host->items[selected].row;
                host->description->value = row.setting && row.plugin ? Wide(row.plugin->file.texts.Resolve(row.setting->description)) : L"";
            }
        }
        return result;
    }
    inline void __fastcall Release(void* self, void*)
    {
        auto found = hosts.find(self); auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep) Retire(keep); releaseHook.thiscall<void>(self);
    }
    inline char __fastcall Switch(void* self, void*, const char* name)
    {
        auto found = hosts.find(self); auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep && Active(keep) && (!name || _strnicmp(name, "WFPMenu", 7))) Close(keep, true);
        return switchHook.thiscall<char>(self, name);
    }
    inline char __fastcall Push(void* self, void*, const char* name, bool immediate)
    {
        auto found = hosts.find(self); auto keep = found == hosts.end() ? nullptr : found->second;
        if (keep && Active(keep) && (!name || _strnicmp(name, "WFPMenu", 7))) Close(keep, true);
        return pushHook.thiscall<char>(self, name, immediate);
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
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0 || dos->e_lfanew > 4096) { Log("native image header rejected"); return; }
        const auto pe = reinterpret_cast<IMAGE_NT_HEADERS32*>(base + dos->e_lfanew);
        // Bound the highest static read (pMenuManager at RVA30C658, four bytes). The unused
        // resource/relocation tail differs between otherwise matching supported executables.
        if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_I386 || pe->OptionalHeader.SizeOfImage < 0x30C65C) { Log("native image range rejected"); return; }
        auto guard = [](uintptr_t rva, std::initializer_list<uint8_t> bytes) { return !memcmp(reinterpret_cast<void*>(base + rva), bytes.begin(), bytes.size()); };
        if (
            !guard(0xb43b2, { 0x55,0x8b,0xec,0x83,0xec,0x3c,0x53,0x56,0x8b,0xf1,0x57,0x8d,0x8e,0xb8,0x2,0x0,0x0,0xe8,0xe4,0x1 }) ||
            !guard(0xb3b54, { 0x55,0x8b,0xec,0x51,0x51,0x53,0x8b,0xd9,0xf6,0x83,0x6c,0x3,0x0,0x0,0x2,0x8b,0x83,0x58,0x3,0x0 }) ||
            !guard(0xb3d2b, { 0x53,0x56,0x57,0x8b,0xf1,0x33,0xff,0xf6,0x86,0x6c,0x3,0x0,0x0,0x1,0xf,0x84,0x5,0x1,0x0,0x0 }) ||
            !guard(0xb092f, { 0x55,0x8b,0xec,0x83,0xe4,0xf8,0x83,0xec,0x54,0xa1,0x40,0xc5,0x70,0x0,0x53,0x56,0x33,0xf6,0x8b,0xd9 }) ||
            !guard(0xb3090, { 0x53,0x56,0x8b,0xf1,0x8b,0x86,0x58,0x3,0x0,0x0,0x8b,0x44,0x86,0x10,0x85,0xc0,0x57,0x74,0x33,0x33 }) ||
            !guard(0xb3cdb, { 0x53,0x56,0x8b,0xf1,0x33,0xdb,0x39,0x9e,0x10,0x1,0x0,0x0,0x57,0x76,0x29,0x8d,0x7e,0x10,0x8b,0x7 }) ||
            !guard(0xad1e7, { 0x56,0x57,0x8b,0xf1,0x8d,0x44,0x24,0xc,0x50,0x8d,0x4e,0x8,0xe8,0x66,0x7,0x0,0x0,0x8b,0x8e,0xd8 }) ||
            !guard(0xad24c, { 0x53,0x56,0x57,0x8b,0xf9,0x8d,0x44,0x24,0x10,0x50,0x8d,0x4f,0x8,0xe8,0x0,0x7,0x0,0x0,0x8b,0xf0 }) ||
            !guard(0xaee50, { 0x53,0x8b,0x5c,0x24,0xc,0x83,0x4b,0x18,0xff,0x55,0x56,0x57,0x6a,0x2,0x33,0xc0,0x21,0x43,0x14,0x33 }) ||
            !guard(0x16db6b, { 0x53,0x55,0x8b,0x6c,0x24,0xc,0x8b,0x45,0x10,0x69,0xc0,0x48,0x1,0x0,0x0,0x8b,0x5c,0x8,0xc,0x8b }) ||
            !guard(0x16e85f, { 0x55,0x8b,0xec,0x83,0xec,0x3c,0xf,0x57,0xc0,0x53,0x56,0x8b,0xf1,0x57,0x33,0xff,0x89,0xbe,0x28,0x42 }) ||
            !guard(0x16e54c, { 0x53,0x56,0x8b,0xf1,0x8b,0x46,0x68,0x33,0xdb,0x48,0x57,0xf,0x84,0xa3,0x0,0x0,0x0,0x48,0x74,0x46 }) ||
            !guard(0x17b641, { 0x8b,0x41,0x1c,0xc3,0x8a,0x44,0x24,0x4,0x88,0x81,0x95,0x2,0x0,0x0,0xc2,0x4,0x0,0x8a,0x81,0x95 })) { Log("native menu executable rejected"); return; }
        const uintptr_t buttonMethods[] = { 0xb470d,0xaed01,0xb0c2f,0xaee50,0xae39e,0xae3d9,0xaf01f,0xae3de,0xae3e6,0xae3f3,0xae41a,0xb0e88,0xaf055,0xbc8e3,0xaedf3,0xae336,0xae16b,0xae494,0xae4a9,0xae49d,0xae489,0xae468,0xae5a7,0xae4c3,0xbc8e3,0xaf0f3,0xaf21d,0x8e610,0xaf261,0xaefa5 };
        const uintptr_t textMethods[] = { 0xb46bf,0xb092f,0xaf2f4,0x18adbd,0x1ec664,0xae142,0xae142,0xae142,0xbc8e3,0x139446,0xae143,0x68384,0x1ec664,0xae85f,0xae142,0xae6b2,0xae16b,0xa93bd,0xbc8e3,0xa93bd,0xa93bd,0xae142,0xae142,0xae148,0xb082d,0xae142,0xbc8e3,0x8e610,0xa93bd,0xae142 };
        if (At<uintptr_t>(reinterpret_cast<void*>(base + 0x24B1A8), 120) != base + 0xAEFE2 ||
            At<uintptr_t>(reinterpret_cast<void*>(base + 0x24B1A8), 124) != base + 0xAF136 ||
            At<uintptr_t>(reinterpret_cast<void*>(base + 0x24AFA8), 120) != base + 0xAE142)
        {
            Log("native control update slot rejected");
            return;
        }
        for (size_t i = 0; i < std::size(buttonMethods); ++i)
            if (At<uintptr_t>(reinterpret_cast<void*>(base + 0x24B1A8), i * 4) != base + buttonMethods[i] ||
                At<uintptr_t>(reinterpret_cast<void*>(base + 0x24AFA8), i * 4) != base + textMethods[i]) { Log("native control vtable rejected"); return; }
        gui = reinterpret_cast<void**>(base + 0x30C658); font = reinterpret_cast<void**>(base + 0x30C540); language = reinterpret_cast<void**>(base + 0x30C538);
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            Log("native menu lifetime pin refused");
            return;
        }
        releaseHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB3D2B), &Release);
        textHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB092F), &DrawText);
        processHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB3B54), &Process);
        switchHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB3090), &Switch);
        pushHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB3CDB), &Push);
        activateHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xAD1E7), &Activate);
        deactivateHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xAD24C), &Deactivate);
        loadHook = safetyhook::create_inline(reinterpret_cast<void*>(base + 0xB43B2), &Load);
        ready = releaseHook && textHook && processHook && switchHook && pushHook && activateHook && deactivateHook && loadHook;
        if (!ready) { loadHook.reset(); deactivateHook.reset(); activateHook.reset(); pushHook.reset(); switchHook.reset(); processHook.reset(); textHook.reset(); releaseHook.reset(); Log("native menu hook transaction rejected"); }
    }
}
