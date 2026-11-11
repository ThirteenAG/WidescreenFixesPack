module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include <array>
#include <cstring>
#include <cmath>
#include <set>


export module NativeMenu;

import ComVars;

// 7001 MEC screens, controls and font service. Native input owns selection/repeat/hit testing;
// API callbacks run only after native screen input returns. Unsupported executables fail closed.
export namespace DPLMenu
{
    inline std::function<bool()> blocksInput, padHelp;
    inline std::function<std::wstring()> helpPrefix;
    using Bytes = std::vector<uint8_t>;
    template<class T> T& At(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset); }
    template<class F> F Virtual(void* p, size_t offset) { return reinterpret_cast<F>(At<uintptr_t>(At<void*>(p, 0), offset)); }
    // Native class tables and functions, found by pattern at Init
    inline uintptr_t textClass, bitmapClass, rowClass, rootClass, guiClass, fontClass, nativeFontClass, languageClass;
    inline uintptr_t setStyle, setText, renderText, buttonAction;
    inline HMODULE module;
    inline uint32_t processDepth = 0;
    inline SafetyHookInline loadHook, processHook, releaseHook, pushHook, textHook, switchHook, activateHook, deactivateHook;
    struct Host;
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;

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
        std::unique_ptr<Text> heading, description, restart;
        std::vector<std::unique_ptr<Text>> helpText;
        std::vector<std::wstring> helpLines;
        size_t helpOffset = 0, helpSelection = 0;
        bool help = false;
        std::vector<Bytes> decorations;
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
        OutputDebugStringA((std::string("DPL native menu: ") + message + "\n").c_str());
    }
    inline std::wstring Wide(const std::string& value)
    {
        int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
        std::wstring result(count, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), count);
        if (result.size() > 4096) { result.resize(4096); if (result.back() >= 0xD800 && result.back() <= 0xDBFF) result.pop_back(); }
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
        if (!LanguageService.is_initialized() || !LanguageService.get() || At<uintptr_t>(LanguageService.get(), 0) != languageClass) return "en";
        int index = Virtual<int(__thiscall*)(void*)>(LanguageService.get(), 84)(LanguageService.get());
        return index >= 0 && index < static_cast<int>(std::size(tags)) ? tags[index] : "en";
    }
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;
    inline void RememberRestartValues(const asi_settings::LoadedPlugin& plugin)
    {
        for (const auto& setting : plugin.file.description.settings)
            if (setting.HasValue() && setting.apply == "restart")
                restartValues.try_emplace({ reinterpret_cast<uintptr_t>(plugin.module), setting.id }, OtherPlugins::Get(plugin, setting));
    }
    inline bool RestartRequired(const RWGTAMenu::Model& model)
    {
        for (const auto& page : model.Pages())
        {
            if (!page.plugin) continue;
            for (const auto& setting : page.plugin->file.description.settings)
            {
                if (!setting.HasValue() || setting.apply != "restart") continue;
                auto initial = restartValues.find({ reinterpret_cast<uintptr_t>(page.plugin->module), setting.id });
                if (initial != restartValues.end() && initial->second != OtherPlugins::Get(*page.plugin, setting)) return true;
            }
        }
        return false;
    }
    inline bool Current(const std::shared_ptr<Host>& host)
    {
        auto found = hosts.find(host->root);
        return !host->retired && found != hosts.end() && found->second == host;
    }
    inline bool Active(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open || !GuiService.is_initialized() || !GuiService.get() || At<int>(GuiService.get(), 736) != host->menuId) return false;
        auto selected = At<uint32_t>(host->root, 856);
        return selected < At<uint32_t>(host->root, 272) && At<void*>(host->root, 16 + selected * 4) == host->page.data();
    }
    inline std::array<uint32_t, 14> Style(Text& text)
    {
        std::array<uint32_t, 14> style{};
        for (size_t i = 0; i < 4; ++i) style[i] = At<uint32_t>(text.bytes.data(), 20 + i * 4);
        style[5] = Virtual<uint32_t(__thiscall*)(void*)>(GuiService.get(), 52)(GuiService.get());
        style[6] = At<uint32_t>(text.bytes.data(), 4); style[7] = At<uint32_t>(text.bytes.data(), 8);
        At<float>(style.data(), 32) = 65536.0f;
        style[9] = At<uint32_t>(text.bytes.data(), 72); style[10] = At<uint32_t>(text.bytes.data(), 76);
        style[11] = At<uint32_t>(text.bytes.data(), 80);
        style[12] = 0; // Literal text with independently fitted columns.
        return style;
    }
    inline void* SetStyle(std::array<uint32_t, 14>& style)
    {
        return reinterpret_cast<void*(__thiscall*)(void*, uint32_t*)>(setStyle)(FontService.get(), style.data());
    }
    inline float Measure(std::array<uint32_t, 14>& style, const std::wstring& value, float maximumHeight = INFINITY, float* height = nullptr)
    {
        auto native = SetStyle(style);
        auto literal = Format(value);
        reinterpret_cast<void(__thiscall*)(void*, const wchar_t*, bool)>(setText)(native, literal.c_str(), false);
        float width = At<float>(native, 128), h = At<float>(native, 132);
        At<uint32_t>(native, 16936) = At<uint32_t>(native, 8740) = 0;
        if (height) *height = h;
        return std::isfinite(width) && std::isfinite(h) && h > 0 && h <= maximumHeight ? width : INFINITY;
    }
    inline int __fastcall DrawText(void* self, void*, int render, int bank)
    {
        auto found = texts.find(self);
        if (found == texts.end()) return textHook.thiscall<int>(self, render, bank);
        auto host = found->second.first.lock();
        if (!host || !Current(host) || !GuiService.is_initialized() || !GuiService.get() || !FontService.is_initialized() || !FontService.get() ||
            At<uintptr_t>(GuiService.get(), 0) != guiClass || At<uintptr_t>(FontService.get(), 0) != fontClass) return 0;
        auto nativeFont = At<void*>(FontService.get(), 12);
        if (!nativeFont || At<uintptr_t>(nativeFont, 0) != nativeFontClass || !At<uint8_t>(nativeFont, 12) ||
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
        reinterpret_cast<void(__thiscall*)(void*, const wchar_t*, bool)>(setText)(native, text.formatted.c_str(), false);
        int result = reinterpret_cast<int(__thiscall*)(void*, int)>(renderText)(native, render);
        At<uint32_t>(native, 16936) = At<uint32_t>(native, 8740) = 0;
        return result;
    }
    inline void __fastcall TextState(void*, void*, int) {}
    inline void Populate(const std::shared_ptr<Host>& host);
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack);
    inline void Help(const std::shared_ptr<Host>& host, const std::string* content = nullptr, const std::string* title = nullptr);
    inline void EndHelp(const std::shared_ptr<Host>& host);
    inline void ScrollHelp(const std::shared_ptr<Host>& host, int amount);
    inline std::string pendingNotice;
    inline int __fastcall Input(void* self, void*, int action, uint32_t* state, char orientation)
    {
        auto found = buttons.find(self);
        if (found == buttons.end()) return 0;
        auto host = found->second.first.lock(); auto button = found->second.second;
        if (!host || !Current(host)) return 0;
        state[5] = 0; state[6] = static_cast<uint32_t>(-1);
        if (host->help)
        {
            if (action >= 5 && action <= 9) host->pending = action;
            return 0;
        }
        if (button->entry)
        {
            if (action == 9) { host->pending = 9; host->pendingRow = button->row; }
            else reinterpret_cast<int(__thiscall*)(void*, int, uint32_t*, char)>(buttonAction)(self, action, state, orientation);
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
                if (!native || At<uintptr_t>(native, 0) != textClass || index + (entry ? 1 : 2) > 8) return nullptr;
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
        for (auto& text : button.text)
        {
            if (!button.entry && !text->fullValue)
            {
                const float width = value.empty() ? 0.84f : 0.45f;
                if (text->width != width) { text->width = width; text->fitted = false; }
            }
            text->value = Wide(text->fullValue ? value : label);
        }
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
        At<void*>(host->page.data(), 8 + host->items.size() * 4) = host->restart->bytes.data();
        for (size_t i = 0; i < host->decorations.size(); ++i)
            At<void*>(host->page.data(), (host->items.size() + 3 + i) * 4) = host->decorations[i].data();
        At<uint32_t>(host->page.data(), 256) = static_cast<uint32_t>(host->items.size() + 3 + host->decorations.size());
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
    inline void ScrollHelp(const std::shared_ptr<Host>& host, int amount)
    {
        if (!Active(host) || !host->help) return;
        const int last = std::max(0, static_cast<int>(host->helpLines.size()) - 6);
        host->helpOffset = static_cast<size_t>(std::clamp(static_cast<int>(host->helpOffset) + amount, 0, last));
        for (size_t i = 0; i < host->helpText.size(); ++i)
            host->helpText[i]->value = host->helpOffset + i < host->helpLines.size() ? host->helpLines[host->helpOffset + i] : L"";
    }
    inline void Help(const std::shared_ptr<Host>& host, const std::string* content, const std::string* title)
    {
        if (!Active(host) || host->help || host->helpText.size() != 6) return;
        const auto selected = At<uint32_t>(host->page.data(), 368);
        if (selected >= host->items.size()) return;
        std::string value = content ? *content : host->model.Help(host->items[selected].row);
        if (value.empty()) return;
        auto style = Style(*host->helpText.front());
        std::wstring line;
        host->helpLines.clear();
        for (auto c : Wide(value))
        {
            if (c == L'\r') continue;
            if (c == L'\n') { host->helpLines.push_back(line); line.clear(); continue; }
            line += c;
            if (Measure(style, line, INFINITY) <= 0.84f) continue;
            auto split = line.find_last_of(L' ');
            if (split == std::wstring::npos || split == 0) split = line.size() - 1;
            host->helpLines.push_back(line.substr(0, split));
            line.erase(0, split + (line[split] == L' ' ? 1 : 0));
        }
        if (!line.empty()) host->helpLines.push_back(line);
        host->help = true; host->helpOffset = 0; host->helpSelection = selected;
        host->heading->value = Wide(title ? *title : host->items[selected].label);
        host->description->value = Wide(host->model.Text("menu.help.controls"));
        SetText(*host->buttons[6], host->model.Text("menu.back"), "");
        At<uint32_t>(host->buttons[6]->bytes.data(), 68) = 0;
        At<void*>(host->page.data(), 0) = host->buttons[6]->bytes.data();
        for (size_t i = 0; i < host->helpText.size(); ++i)
            At<void*>(host->page.data(), 4 + i * 4) = host->helpText[i]->bytes.data();
        At<void*>(host->page.data(), 28) = host->heading->bytes.data();
        At<void*>(host->page.data(), 32) = host->description->bytes.data();
        At<void*>(host->page.data(), 36) = host->restart->bytes.data();
        for (size_t i = 0; i < host->decorations.size(); ++i)
            At<void*>(host->page.data(), (10 + i) * 4) = host->decorations[i].data();
        At<uint32_t>(host->page.data(), 256) = static_cast<uint32_t>(10 + host->decorations.size());
        At<uint32_t>(host->page.data(), 368) = 0;
        ScrollHelp(host, 0);
    }
    inline void EndHelp(const std::shared_ptr<Host>& host)
    {
        if (!Active(host) || !host->help) return;
        host->help = false;
        At<uint32_t>(host->page.data(), 368) = static_cast<uint32_t>(host->helpSelection);
        Populate(host);
    }
    inline bool NoticeReady()
    {
        for (auto& [root, host] : hosts) if (Active(host) && !host->help) return true;
        return false;
    }
    inline void ShowNotice(const std::wstring& value)
    {
        pendingNotice = asi_settings::text::Narrow(value);
    }
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack)
    {
        if (nativeBack && Active(host)) Virtual<int(__thiscall*)(void*, bool)>(host->root, 80)(host->root, true);
        host->help = false; host->open = false; host->model.Close(); host->items.clear(); host->pending = 0;
    }
    inline void Action(const std::shared_ptr<Host>& host)
    {
        int action = std::exchange(host->pending, 0);
        if (!action || !Current(host) || host->busy) return;
        if (host->help)
        {
            if (action == 9) EndHelp(host);
            else ScrollHelp(host, action == 5 ? -1 : action == 6 ? 1 : action == 7 ? -6 : 6);
            return;
        }
        struct BusyScope { Host& host; explicit BusyScope(Host& value) : host(value) { host.busy = true; } ~BusyScope() { host.busy = false; } } busy(*host);
        if (!host->open)
        {
            auto selected = At<uint32_t>(host->root, 856);
            if (selected >= host->originalScreens || At<void*>(host->root, 16 + selected * 4) != host->parent) return;
            auto menuId = At<int>(GuiService.get(), 736);
            host->model.OpenPlugins(module, Language(), 4096);
        for (const auto& page : host->model.Pages()) if (page.plugin) RememberRestartValues(*page.plugin);
            if (Current(host) && host->model.IsOpen() && At<int>(GuiService.get(), 736) == menuId &&
                At<uint32_t>(host->root, 856) == selected && At<void*>(host->root, 16 + selected * 4) == host->parent)
            {
                host->menuId = menuId;
                host->open = true; host->modelPage = host->rootOffset = host->rowStart = 0;
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
        if (!GuiService.is_initialized() || !GuiService.get() || !FontService.is_initialized() || !FontService.get() || At<uintptr_t>(root, 0) != rootClass ||
            At<uintptr_t>(GuiService.get(), 0) != guiClass || At<uintptr_t>(FontService.get(), 0) != fontClass) return reject("service/class identity rejected");
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
            if (row && At<uintptr_t>(row, 0) == rowClass && At<float>(row, 8) < 0.8f) candidates.emplace_back(At<float>(row, 8), i);
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
        host->restart = CloneText(host, textTemplate, 0.075f, 0.74f, 0.84f);
        host->restart->height = 0.045f;
        for (size_t i = 0; i < 6; ++i)
        {
            auto text = CloneText(host, textTemplate, 0.075f, 0.24f + static_cast<float>(i) * 0.055f, 0.84f, true);
            text->height = 0.05f;
            host->helpText.push_back(std::move(text));
        }
        if (host->pause)
        {
            auto* panel = At<void*>(host->parent, 0);
            if (!panel || At<uintptr_t>(panel, 0) != bitmapClass)
                return reject("native pause background contract rejected");
            Bytes bitmap(80);
            std::memcpy(bitmap.data(), panel, bitmap.size());
            At<void*>(bitmap.data(), 64) = nullptr;
            At<float>(bitmap.data(), 4) = 0.025f;
            At<float>(bitmap.data(), 8) = 0.09f;
            At<float>(bitmap.data(), 12) = 0.95f;
            At<float>(bitmap.data(), 16) = 0.73f;
            for (auto offset : {20u, 24u, 28u}) At<float>(bitmap.data(), offset) = 0;
            At<float>(bitmap.data(), 32) = 1;
            for (int i = 0; i < 3; ++i) host->decorations.push_back(bitmap);
        }
        // Only inert bitmap decoration is cloned; vanilla heading/action text is not duplicated.
        for (size_t i = 0; i < host->originalRows; ++i)
        {
            auto row = At<void*>(host->parent, i * 4);
            if (row && At<uintptr_t>(row, 0) == bitmapClass)
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
        Prepare(root);
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.expired()) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.expired()) i = buttons.erase(i); else ++i; }
        return result;
    }
    inline bool BackPressed(const uint32_t* state)
    {
        if (!state || !state[0]) return false;
        auto* actions = reinterpret_cast<uint8_t*>(state[0]);
        auto* current = At<float*>(actions, 40);
        return current && !At<uint8_t>(actions, 44) && !At<uint8_t>(current, 4) && *current != 0.0f;
    }
    inline char __fastcall Process(void* self, void*, uint32_t* state)
    {
        auto found = hosts.find(self); auto host = found == hosts.end() ? nullptr : found->second;
        bool active = host && Active(host);
        const bool back = active && BackPressed(state);
        char result;
        { struct Scope { Scope() { ++processDepth; } ~Scope() { --processDepth; } } depth; result = processHook.thiscall<char>(self, state); }
        if (!host || !Current(host) || processDepth || host->busy) return result;
        if (active && host->help && back)
        {
            At<uint8_t>(host->root, 876) &= ~0x10u;
            state[5] &= ~8u;
            EndHelp(host);
            return result;
        }
        if (active && Active(host) && back && host->modelPage)
        {
            // Native private exit has no animation or command. Cancel its queued parent pop for
            // inner Back, then repopulate the same page as the Advanced Options root.
            At<uint8_t>(host->root, 876) &= ~0x10u;
            host->model.PageShown(host->modelPage - 1); host->modelPage = host->rowStart = 0;
            host->pending = 0;
            state[5] &= ~8u;
            if (Active(host)) Populate(host);
            return result;
        }
        else if (active && !Active(host)) Close(host, false);
        Action(host);
        if (Active(host) && !host->help && RWGTAMenu::Model::Changed()) Populate(host);
        if (Active(host))
        {
            host->restart->value = RestartRequired(host->model) ? Wide(host->model.Text("menu.restartwarning")) : L"";
            static bool helpDown = false;
            DWORD foregroundProcess = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
            const bool down = foregroundProcess == GetCurrentProcessId() &&
                ((GetAsyncKeyState(VK_F1) & 0x8000) || (padHelp && padHelp()));
            if (down && !helpDown && !(blocksInput && blocksInput()))
            {
                if (host->help) EndHelp(host); else Help(host);
            }
            helpDown = down;
            if (host->help) return result;
            if (!pendingNotice.empty())
            {
                const auto title = host->model.Text("plugin.name");
                Help(host, &pendingNotice, &title);
                if (host->help) pendingNotice.clear();
                return result;
            }
            auto selected = At<uint32_t>(host->page.data(), 368);
            if (selected < host->items.size())
            {
                const auto row = host->items[selected].row;
                auto description = host->model.Help(row);
                host->description->value = description.empty() ? L"" : (helpPrefix ? helpPrefix() : L"F1: ") + Wide(description);
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
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&Init), &module);
        // Class tables are read from the constructors that store them
        auto operand = [](const char* signature, ptrdiff_t offset) { return *hook::get_pattern<uintptr_t>(signature, offset); };
        auto code = [](const char* signature) { return reinterpret_cast<uintptr_t>(hook::get_pattern(signature)); };
        textClass = operand("C7 06 ? ? ? ? E8 ? ? ? ? 8B C6 5E C3 8B C1 C7 00 ? ? ? ? C3 56 8B F1 8D 4E 44 C7 06 ? ? ? ?", 2);
        bitmapClass = operand("C7 00 ? ? ? ? C3 56 8B F1 8D 4E 44 C7 06 ? ? ? ? E8 ? ? ? ? 8B C6 5E C3 8B C1 C7 00 ? ? ? ?", 2);
        rowClass = operand("C7 06 ? ? ? ? 83 A6 D8 00 00 00 00", 2);
        rootClass = operand("C7 06 ? ? ? ? C7 46 04 ? ? ? ? C7 46 0C ? ? ? ? E8 ? ? ? ?", 2);
        guiClass = operand("C7 06 ? ? ? ? C7 46 04 ? ? ? ? E8 ? ? ? ? 6A 51", 2);
        fontClass = operand("C7 06 ? ? ? ? C7 46 04 ? ? ? ? E8 ? ? ? ? 8D 7E 0C", 2);
        nativeFontClass = operand("C7 03 ? ? ? ? C7 43 04 ? ? ? ? E8 ? ? ? ? 68 ? ? ? ?", 2);
        languageClass = operand("C7 06 ? ? ? ? 33 C0 8D BE CC 02 00 00", 2);
        GuiService.SetAddress(operand("8B 1D ? ? ? ? 8B 03 57 57 8B CE", 2));
        FontService.SetAddress(operand("8B 35 ? ? ? ? F3 0F 11 44 24 28", 2));
        LanguageService.SetAddress(operand("8B 0D ? ? ? ? 8B 01 52 FF 50 68", 2));
        setStyle = code("53 55 8B 6C 24 0C 8B 45 10 69 C0 48 01 00 00");
        setText = code("55 8B EC 83 EC 3C 0F 57 C0 53");
        renderText = code("53 56 8B F1 8B 46 68 33 DB 48");
        buttonAction = code("53 8B 5C 24 0C 83 4B 18 FF 55");
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            Log("native menu lifetime pin refused");
            return;
        }
        for (const auto& plugin : asi_settings::FindLoadedPlugins(module, "en")) RememberRestartValues(plugin);
        releaseHook = safetyhook::create_inline(hook::get_pattern("53 56 57 8B F1 33 FF F6 86 6C 03 00 00 01"), &Release);
        textHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 E4 F8 83 EC 54 A1 ? ? ? ? 53 56 33 F6 8B D9 57 8D 4C 24 28 89 44 24 0C"), &DrawText);
        processHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 51 51 53 8B D9 F6 83 6C 03 00 00 02"), &Process);
        switchHook = safetyhook::create_inline(hook::get_pattern("53 56 8B F1 8B 86 58 03 00 00"), &Switch);
        pushHook = safetyhook::create_inline(hook::get_pattern("53 56 8B F1 33 DB 39 9E 10 01 00 00"), &Push);
        activateHook = safetyhook::create_inline(hook::get_pattern("56 57 8B F1 8D 44 24 0C 50 8D 4E 08"), &Activate);
        deactivateHook = safetyhook::create_inline(hook::get_pattern("53 56 57 8B F9 8D 44 24 10 50"), &Deactivate);
        loadHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 EC 3C 53 56 8B F1 57"), &Load);
    }
}
