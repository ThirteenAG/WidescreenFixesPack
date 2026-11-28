module;

#include "stdafx.h"
#include <d3d9.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RainDroplets.hpp"
#include "Shared/PadInput.hpp"
#include <array>
#include <cstring>
#include <cmath>
#include <set>


export module NativeMenu;

import ComVars;

// Update 2 MEC screens, controls and font service. Native input owns selection/repeat/hit testing;
// API callbacks run only after native screen input returns. Unsupported executables fail closed.
export namespace Driv3rMenu
{
    using Bytes = std::vector<uint8_t>;
    template<class T> T& At(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset); }
    template<class F> F Virtual(void* p, size_t offset) { return reinterpret_cast<F>(At<uintptr_t>(At<void*>(p, 0), offset)); }
    // Native class tables and functions, found by pattern at Init
    inline uintptr_t textClass, bitmapClass, buttonClass, rootClass, guiClass, fontClass, nativeFontClass, languageClass;
    inline uintptr_t buttonAction;
    inline void** menuSound;
    inline HMODULE module;
    // Rows the settings page shows at once; the list scrolls through them
    constexpr size_t Slots = 7;
    // The game's font has its menu arrows here
    constexpr wchar_t ArrowDown = 0x2480, ArrowUp = 0x2481, ArrowRight = 0x2482, ArrowLeft = 0x2483;
    inline uint32_t processDepth = 0;
    inline std::function<bool()> padHelp, blocksInput;
    inline std::function<std::wstring()> helpPrefix;
    inline SafetyHookInline loadHook, processHook, destroyHook, releaseHook, textHook, switchHook, deactivateHook;
    inline SafetyHookMid activateEntry;
    struct Host;
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;

    struct Text
    {
        std::array<uint8_t, 120> bytes{};
        std::wstring value;
        float width = 0.4f;
        float height = 0.044f;
        bool fullValue = false;
        bool fitted = false;
        bool centered = false;      // centred in its width, as the game's titles and values
        int role = 0;               // a row's label (0), value (1), left (2) or right arrow (3)
        float narrow = 0, wide = 0; // a label's width beside a value, and alone (a submenu row)
        float offset = 0;
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
    struct Item { std::string label, value; RWGTAMenu::Row row; bool enabled = true; };
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
        std::unique_ptr<Text> heading, description, description2, above, below;
        // The selected row's whole help text on two lines; a longer one moves on by a line after a while
        std::wstring describedText;
        std::vector<std::wstring> describedLines;
        size_t describedFirst = 0;
        ULONGLONG describedTick = 0;
        std::vector<std::unique_ptr<Text>> helpText;
        // Where the page's parts go, taken from the game's own settings page the page copies
        struct Layout
        {
            float rowX, rowTop, rowStep, rowHeight, leftX, valueX, valueWidth, rightX, arrowWidth, scrollX, textX, textWidth;
        } layout{};
        std::vector<Bytes> backdrop;        // the game page's frame and, in the pause menu, its menu column
        std::unique_ptr<Text> pluginsLabel; // that column's highlighted "Plugins"
        std::vector<void*> decor;
        std::array<float, 4> backIcon{};    // the game's Back icon (front end), clicked like the game's
        bool backIconShown = false, mouseDown = false;
        std::vector<std::wstring> helpLines;
        size_t helpOffset = 0, helpSelection = 0;
        bool help = false;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        size_t modelPage = RWGTAMenu::Model::RootPage;
        MenuList::WheelNotches wheel;
        MenuList::Repeat pageRepeat, arrowRepeat, holdRepeat;
        MenuList::StickAxis holdStick;
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
        if (result.size() > 4096) { result.resize(4096); if (result.back() >= 0xD800 && result.back() <= 0xDBFF) result.pop_back(); }
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
        auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        static const char* tags[] = { "en", "fr", "de", "es", "it", "ja", "zh-CN", "zh-TW", "ko", "pt" };
        if (!LanguageService.is_initialized() || !LanguageService.get() || At<uintptr_t>(LanguageService.get(), 0) != languageClass) return "en";
        int index = Virtual<int(__thiscall*)(void*)>(LanguageService.get(), 80)(LanguageService.get());
        return index >= 0 && index < static_cast<int>(std::size(tags)) ? tags[index] : "en";
    }
    inline bool Current(const std::shared_ptr<Host>& host)
    {
        auto found = hosts.find(host->root);
        return !host->retired && found != hosts.end() && found->second == host;
    }
    inline bool Active(const std::shared_ptr<Host>& host)
    {
        if (!Current(host) || !host->open || !GuiService.is_initialized() || !GuiService.get() || At<int>(GuiService.get(), 33572) != host->menuId) return false;
        auto selected = At<uint32_t>(host->root, 16664);
        return selected < At<uint32_t>(host->root, 16660) && At<void*>(host->root, 16460 + selected * 4) == host->page.data();
    }
    inline std::array<uint32_t, 13> Style(Text& text)
    {
        std::array<uint32_t, 13> style{};
        style[0] = At<uint32_t>(text.bytes.data(), 20); style[1] = At<uint32_t>(text.bytes.data(), 24);
        style[2] = At<uint32_t>(text.bytes.data(), 28); style[3] = At<uint32_t>(text.bytes.data(), 32);
        style[4] = 0; style[5] = Virtual<uint32_t(__thiscall*)(void*)>(GuiService.get(), 32)(GuiService.get());
        style[6] = At<uint32_t>(text.bytes.data(), 4); style[7] = At<uint32_t>(text.bytes.data(), 8);
        // Native SetPosition converts width to a wrapping bound. Measure an unwrapped line and
        // fit it explicitly; stock widths would hide overflow by returning a multi-line max width.
        At<float>(style.data(), 32) = 65536.0f; style[9] = At<uint32_t>(text.bytes.data(), 48);
        style[10] = At<uint32_t>(text.bytes.data(), 52); style[11] = At<uint32_t>(text.bytes.data(), 44);
        return style;
    }
    inline float Measure(std::array<uint32_t, 13>& style, const std::wstring& value, float maximumHeight = 0.044f, float* height = nullptr)
    {
        // The game's measure goes wrong across spaces (the font has no space glyph), so words are measured one by
        // one and each space counts as the font's space advance
        auto measure = [&](const std::wstring& text, float* lineHeight)
        {
            float extent[2]{};
            auto formatted = Format(text);
            Virtual<void*(__cdecl*)(void*, float*, uint32_t*, const wchar_t*, ...)>(FontService.get(), 36)(FontService.get(), extent, style.data(), formatted.c_str());
            if (lineHeight) *lineHeight = std::max(*lineHeight, extent[1]);
            return std::isfinite(extent[0]) && std::isfinite(extent[1]) ? extent[0] : INFINITY;
        };
        float lineHeight = 0, width = 0;
        size_t spaces = 0;
        for (size_t start = 0; start <= value.size();)
        {
            size_t end = value.find(L' ', start);
            if (end == std::wstring::npos) end = value.size();
            if (end > start) width += measure(value.substr(start, end - start), &lineHeight);
            if (end < value.size()) ++spaces;
            start = end + 1;
        }
        if (spaces) width += static_cast<float>(spaces) * measure(std::wstring(1, wchar_t(0x202F)), nullptr);
        if (height) *height = lineHeight;
        return std::isfinite(width) && lineHeight > 0 && lineHeight <= maximumHeight ? width : INFINITY;
    }
    inline int __fastcall DrawText(void* self, void*, int render, int bank)
    {
        auto found = texts.find(self);
        if (found == texts.end()) return textHook.thiscall<int>(self, render, bank);
        auto host = found->second.first.lock();
        if (!host || !Current(host) || !GuiService.is_initialized() || !GuiService.get() || !FontService.is_initialized() || !FontService.get() ||
            At<uintptr_t>(GuiService.get(), 0) != guiClass || At<uintptr_t>(FontService.get(), 0) != fontClass) return 0;
        auto& text = *found->second.second;
        auto style = Style(text);
        auto nativeFont = At<void*>(FontService.get(), 12); // Native style selects font bank zero.
        if (!nativeFont || At<uintptr_t>(nativeFont, 0) != nativeFontClass || !At<uint8_t>(nativeFont, 12) ||
            !At<void*>(nativeFont, 24) || !At<void*>(nativeFont, 28))
        { Log("native font resources not ready"); return 0; }
        // Resource load/release receives the second interface (+4), not this primary font.
        // Its loaded flag +8 and glyph pointer +20 therefore belong at +12 and +24 here.
        auto glyphData = At<void*>(nativeFont, 24);
        if (text.fitted && text.cachedValue == text.value && text.cachedStyle == style && text.cachedFont == glyphData)
        {
            At<float>(style.data(), 36) = text.fittedScale;
            At<float>(style.data(), 40) = text.fittedVerticalScale;
            At<float>(style.data(), 24) += text.offset;
            return Virtual<int(__cdecl*)(void*, int, uint32_t*, const wchar_t*, ...)>(FontService.get(), 24)(FontService.get(), render, style.data(), text.formatted.c_str());
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
        text.offset = 0;
        if (text.centered)
        {
            const float width = Measure(style, value, text.height + 0.00001f);
            if (std::isfinite(width) && width < text.width) text.offset = (text.width - width) * 0.5f;
        }
        text.cachedValue = text.value; text.cachedStyle = originalStyle; text.cachedFont = glyphData;
        text.formatted = formatted; text.fittedScale = At<float>(style.data(), 36); text.fittedVerticalScale = At<float>(style.data(), 40); text.fitted = true;
        At<float>(style.data(), 24) += text.offset;
        return Virtual<int(__cdecl*)(void*, int, uint32_t*, const wchar_t*, ...)>(FontService.get(), 24)(FontService.get(), render, style.data(), formatted.c_str());
    }
    inline void Populate(const std::shared_ptr<Host>& host);
    inline void Help(const std::shared_ptr<Host>& host, const std::string* content = nullptr, const std::string* title = nullptr);
    inline void EndHelp(const std::shared_ptr<Host>& host);
    inline void ScrollHelp(const std::shared_ptr<Host>& host, int amount);
    inline std::string pendingNotice;
    inline void Close(const std::shared_ptr<Host>& host, bool nativeBack);
    inline int __fastcall Input(void* self, void*, int action, uint32_t* state)
    {
        auto found = buttons.find(self);
        if (found == buttons.end()) return 0;
        auto host = found->second.first.lock();
        auto button = found->second.second;
        if (!host || !Current(host)) return 0;
        state[5] = 0; state[6] = static_cast<uint32_t>(-1);
        if (host->help)
        {
            if (action >= 2 && action <= 6) host->pending = action;
            return 0;
        }
        if (button->entry)
        {
            if (action == 6) { host->pending = 6; host->pendingRow = button->row; }
            else reinterpret_cast<int(__thiscall*)(void*, int, uint32_t*)>(buttonAction)(self, action, state);
            return 0;
        }
        if (action == 2 || action == 3)
        {
            // The list moves after native input returns; selecting the same slot keeps the native move sound
            if (!host->items.empty()) { state[5] = 1; state[6] = static_cast<uint32_t>(button->row); }
            host->pending = action; host->pendingRow = button->row;
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
        const auto& layout = host->layout;
        At<float>(button->bytes.data(), 4) = x; At<float>(button->bytes.data(), 8) = y;
        // A row reacts over its label; the arrows around its value are separate, as on the game's pages
        At<float>(button->bytes.data(), 12) = entry ? 0.45f : layout.leftX - layout.rowX;
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
                if (vt == textClass && entry)
                {
                    auto text = CloneText(host, native, At<float>(native, 4) + dx, At<float>(native, 8) + dy, 0.44f);
                    At<void*>(button->bytes.data(), 40 + state * 20 + index++ * 4) = text->bytes.data();
                    button->text.push_back(std::move(text));
                }
                else if (vt == textClass)
                {
                    // Label, value between the game's arrows, in the template's own colours per state
                    if (count != 1) return nullptr;
                    const float textY = At<float>(native, 8) + dy;
                    const std::array<std::tuple<float, float, int>, 4> parts{ {
                        { layout.rowX, layout.leftX - layout.rowX - layout.arrowWidth, 0 }, { layout.valueX, layout.valueWidth, 1 },
                        { layout.leftX, layout.arrowWidth, 2 }, { layout.rightX, layout.arrowWidth, 3 } } };
                    for (auto [partX, width, role] : parts)
                    {
                        auto text = CloneText(host, native, partX, textY, width, role == 1);
                        text->role = role; text->centered = role != 0;
                        if (role == 0) { text->narrow = width; text->wide = layout.rightX + layout.arrowWidth - layout.rowX; }
                        At<void*>(button->bytes.data(), 40 + state * 20 + index++ * 4) = text->bytes.data();
                        button->text.push_back(std::move(text));
                    }
                }
                else if (vt == bitmapClass)
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
                dim->role = original->role; dim->centered = original->centered;
                dim->narrow = original->narrow; dim->wide = original->wide;
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
        for (auto& text : button.text)
        {
            if (text->role >= 2) text->value = value.empty() ? L"" : std::wstring(1, text->role == 2 ? ArrowLeft : ArrowRight);
            else text->value = Wide(text->role == 1 ? value : label);
            if (text->role == 0 && text->wide > 0)
            {
                const float width = value.empty() ? text->wide : text->narrow;
                if (text->width != width) { text->width = width; text->fitted = false; }
            }
        }
    }
    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Active(host)) return;
        std::vector<Item> items;
        auto& model = host->model;
        if (host->modelPage >= model.Pages().size()) host->modelPage = RWGTAMenu::Model::RootPage;
        std::string title = model.Pages()[host->modelPage].title;
        // One continuous list per page (Back is the native back control, as on the game's own pages)
        auto rows = model.ListRows(host->modelPage, false);
        if (!Active(host)) return;
        auto& window = model.Window(host->modelPage);
        // Disabled rows aren't selectable, as in the game's own lists
        for (size_t tries = 0; tries < rows.size() && !RWGTAMenu::Model::Opens(*rows[window.Selected()]) &&
            !model.Enabled(*rows[window.Selected()], host->pause); ++tries)
            window.Move(1);
        if (!Active(host)) return;
        for (size_t i = window.First(); i < window.First() + window.Shown(); ++i)
        {
            auto row = *rows[i];
            const bool opens = RWGTAMenu::Model::Opens(row);
            bool enabled = opens || model.Enabled(row, host->pause);
            if (!Active(host)) return;
            auto value = opens ? std::string{} : model.Value(row);
            if (!Active(host)) return;
            auto label = model.Label(row);
            if (!Active(host)) return;
            items.push_back({ std::move(label), std::move(value), row, enabled });
        }
        if (!Active(host)) return;
        host->items = std::move(items);
        host->heading->value = Wide(title + (model.RestartRequired() ? " - " + model.Text("menu.restartwarning") : ""));
        host->description->value.clear();
        host->description2->value.clear();
        host->describedText.clear();
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            auto& item = host->items[i];
            auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            SetText(*host->buttons[i], item.label, value);
            At<void*>(host->page.data(), 100 + i * 4) = host->buttons[i]->bytes.data();
        }
        // Scroll arrows in the game's font, only while rows are hidden above or below
        host->above->value = window.Above() ? std::wstring(1, ArrowUp) : L"";
        host->below->value = window.Below() ? std::wstring(1, ArrowDown) : L"";
        size_t element = host->items.size();
        for (auto text : { host->heading.get(), host->description.get(), host->description2.get(), host->above.get(), host->below.get() })
            At<void*>(host->page.data(), 100 + element++ * 4) = text->bytes.data();
        for (auto decor : host->decor)
            At<void*>(host->page.data(), 100 + element++ * 4) = decor;
        At<uint32_t>(host->page.data(), 300) = static_cast<uint32_t>(element);
        for (size_t i = 0; i < host->buttons.size(); ++i)
            At<uint32_t>(host->buttons[i]->bytes.data(), 36) = i < host->items.size() && !host->items[i].enabled ? 2 : 1;
        const auto selected = static_cast<uint32_t>(std::min(window.Slot(), host->items.empty() ? 0 : host->items.size() - 1));
        At<uint32_t>(host->page.data(), 304) = selected;
        if (selected < host->items.size()) At<uint32_t>(host->buttons[selected]->bytes.data(), 36) = 0;
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
        const auto selected = At<uint32_t>(host->page.data(), 304);
        if (selected >= host->items.size()) return;
        std::string value = content ? *content : host->model.Help(host->items[selected].row);
        if (host->model.RestartRequired()) value = host->model.Text("menu.restartwarning") + " " + value;
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
        At<uint32_t>(host->buttons[6]->bytes.data(), 36) = 0;
        At<void*>(host->page.data(), 100) = host->buttons[6]->bytes.data();
        for (size_t i = 0; i < host->helpText.size(); ++i)
            At<void*>(host->page.data(), 104 + i * 4) = host->helpText[i]->bytes.data();
        At<void*>(host->page.data(), 128) = host->heading->bytes.data();
        At<void*>(host->page.data(), 132) = host->description->bytes.data();
        for (size_t i = 0; i < host->decor.size(); ++i)
            At<void*>(host->page.data(), 136 + i * 4) = host->decor[i];
        At<uint32_t>(host->page.data(), 300) = static_cast<uint32_t>(9 + host->decor.size());
        At<uint32_t>(host->page.data(), 304) = 0;
        ScrollHelp(host, 0);
    }
    inline void EndHelp(const std::shared_ptr<Host>& host)
    {
        if (!Active(host) || !host->help) return;
        host->help = false;
        At<uint32_t>(host->page.data(), 304) = static_cast<uint32_t>(host->helpSelection);
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
        if (nativeBack && Active(host)) Virtual<int(__thiscall*)(void*)>(host->root, 48)(host->root);
        host->help = false;
        host->open = false; host->model.Close(); host->items.clear(); host->pending = 0;
    }
    // Up/Down: the next selectable row, wrapping at the ends like the game's own lists
    inline void Move(const std::shared_ptr<Host>& host, int direction)
    {
        auto& model = host->model;
        auto rows = model.ListRows(host->modelPage, false);
        auto& window = model.Window(host->modelPage);
        for (size_t tries = 0; tries < rows.size() && Active(host); ++tries)
        {
            if (!window.Move(direction)) return;
            auto row = rows[window.Selected()];
            if (RWGTAMenu::Model::Opens(*row) || model.Enabled(*row, host->pause)) return;
        }
    }
    inline void PlayMoveSound()
    {
        if (*menuSound) Virtual<void(__thiscall*)(void*, int)>(*menuSound, 8)(*menuSound, 0);
    }
    // Page Up/Down, Home/End, the wheel and the scroll arrows; the native move sound when the list moved
    inline void Navigate(const std::shared_ptr<Host>& host, bool moved)
    {
        if (!moved || !Active(host)) return;
        PlayMoveSound();
        Populate(host);
    }
    inline void Action(const std::shared_ptr<Host>& host)
    {
        int action = std::exchange(host->pending, 0);
        if (!action || !Current(host) || host->busy) return;
        if (host->help)
        {
            if (action == 6) EndHelp(host);
            else ScrollHelp(host, action == 2 ? -1 : action == 3 ? 1 : action == 4 ? -6 : 6);
            return;
        }
        struct BusyScope { Host& host; explicit BusyScope(Host& value) : host(value) { host.busy = true; } ~BusyScope() { host.busy = false; } } busy(*host);
        if (!host->open)
        {
            auto selected = At<uint32_t>(host->root, 16664);
            if (selected >= host->originalScreens || At<void*>(host->root, 16460 + selected * 4) != host->parent) return;
            auto menuId = At<int>(GuiService.get(), 33572);
            host->model.OpenMenu(module, Language(), Slots);
            if (Current(host) && host->model.IsOpen() && At<int>(GuiService.get(), 33572) == menuId &&
                At<uint32_t>(host->root, 16664) == selected && At<void*>(host->root, 16460 + selected * 4) == host->parent)
            {
                host->menuId = menuId;
                host->open = true; host->modelPage = RWGTAMenu::Model::RootPage;
                host->wheel.Clear(); MouseRead::MenuWheel = 0;
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
            auto& window = host->model.Window(host->modelPage);
            window.Point(host->pendingRow);
            if (action == 2 || action == 3)
                Move(host, action == 2 ? -1 : 1);
            else if (RWGTAMenu::Model::Opens(item.row))
            {
                if (action == 6)
                {
                    auto page = host->model.Enter(item.row);
                    if (page != SIZE_MAX) host->modelPage = page;
                }
            }
            else
                host->model.Change(item.row, action == 4 ? -1 : action == 5 ? 1 : 0, host->pause);
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
            const uint32_t fallback = host->pause ? 4u : host->first;
            At<uint32_t>(host->parent, 304) = fallback;
            At<uint32_t>(At<void*>(host->parent, 100 + fallback * 4), 36) = 0;
        }
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, 100 + host->first * 4)) + 128, host->firstUp.data(), 8);
        memcpy(static_cast<uint8_t*>(At<void*>(host->parent, 100 + host->last * 4)) + 136, host->lastDown.data(), 8);
        At<void*>(host->parent, 100 + host->originalRows * 4) = nullptr;
        host->retired = true; host->open = false; host->model.Close();
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.lock() == host) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.lock() == host) i = buttons.erase(i); else ++i; }
        hosts.erase(host->root);
    }
    // The page copies the game's own settings page: Settings > Display in the front end, the pause menu's Settings
    // page (whose frame also shows the pause menu column, with Plugins highlighted here). Its frame bitmaps, title,
    // row and arrow styles and their places are taken from that page.
    inline bool Design(const std::shared_ptr<Host>& host, void* root, uint32_t count)
    {
        auto refuse = [](const char* reason) { Log(reason); return false; };
        void* source = nullptr;
        for (size_t i = 0; i < count && !source; ++i)
        {
            auto* screen = At<void*>(root, 16460 + i * 4);
            if (!strncmp(static_cast<char*>(screen), host->pause ? "Pause_O" : "Display", 8) &&
                At<uint32_t>(screen, 300) == (host->pause ? 39u : 16u))
                source = screen;
        }
        if (!source) return refuse("native settings page missing");
        void* title = nullptr;
        void* row = nullptr;
        void* left = nullptr;
        void* right = nullptr;
        void* backIcon = nullptr;
        std::vector<void*> textElements;
        auto singleText = [&](void* button)
        {
            return At<uint32_t>(button, 56) == 1 && At<uint32_t>(button, 76) == 1 &&
                At<uintptr_t>(At<void*>(button, 40), 0) == textClass && At<uintptr_t>(At<void*>(button, 60), 0) == textClass;
        };
        const uint32_t elements = At<uint32_t>(source, 300);
        for (uint32_t i = 0; i < elements; ++i)
        {
            auto* element = At<void*>(source, 100 + i * 4);
            const auto vt = At<uintptr_t>(element, 0);
            if (vt == bitmapClass)
            {
                Bytes bitmap(56);
                std::memcpy(bitmap.data(), element, bitmap.size());
                host->backdrop.push_back(std::move(bitmap));
            }
            else if (vt == textClass)
            {
                textElements.push_back(element);
                if (At<uint32_t>(element, 44) == 2 && (!title || At<float>(element, 48) > At<float>(title, 48))) title = element;
            }
            else if (vt == buttonClass && singleText(element))
            {
                const float x = At<float>(element, 4), width = At<float>(element, 12);
                if (width < 0.05f)
                {
                    if (!left || x < At<float>(left, 4)) left = element;
                    if (!right || x > At<float>(right, 4)) right = element;
                }
                else if (width >= 0.2f && (!row || x > At<float>(row, 4))) row = element;
            }
            else if (vt == buttonClass && At<uint32_t>(element, 56) == 1 && At<uintptr_t>(At<void*>(element, 40), 0) == bitmapClass &&
                (!backIcon || At<float>(element, 4) < At<float>(backIcon, 4)))
                backIcon = element;
        }
        if (!title || !row || !left || !right || left == right) return refuse("native settings page contract differs");
        auto& layout = host->layout;
        layout.rowX = At<float>(row, 4);
        layout.rowHeight = At<float>(row, 16);
        layout.leftX = At<float>(left, 4);
        layout.arrowWidth = At<float>(left, 12);
        layout.valueX = layout.leftX + layout.arrowWidth;
        layout.rightX = At<float>(right, 4);
        layout.valueWidth = layout.rightX - layout.valueX;
        layout.scrollX = layout.rightX + layout.arrowWidth + layout.arrowWidth * 0.5f;
        // Rows start at the page's first heading and keep its line spacing
        std::vector<float> lines;
        layout.rowTop = INFINITY; layout.textX = INFINITY;
        for (auto text : textElements)
        {
            if (text == title || At<float>(text, 4) < layout.rowX - 0.05f) continue;
            lines.push_back(At<float>(text, 8));
            if (At<uint32_t>(text, 44) == 0)
            {
                layout.rowTop = std::min(layout.rowTop, At<float>(text, 8));
                layout.textX = std::min(layout.textX, At<float>(text, 4));
            }
        }
        for (uint32_t i = 0; i < elements; ++i)
        {
            auto* element = At<void*>(source, 100 + i * 4);
            if (At<uintptr_t>(element, 0) == buttonClass && At<float>(element, 4) >= layout.rowX - 0.05f) lines.push_back(At<float>(element, 8));
        }
        std::sort(lines.begin(), lines.end());
        layout.rowStep = INFINITY;
        for (size_t i = 1; i < lines.size(); ++i)
            if (lines[i] - lines[i - 1] > 0.01f) layout.rowStep = std::min(layout.rowStep, lines[i] - lines[i - 1]);
        if (!std::isfinite(layout.rowTop) || !std::isfinite(layout.textX) || !std::isfinite(layout.rowStep) || layout.valueWidth <= 0)
            return refuse("native settings page geometry differs");
        layout.textWidth = layout.rightX + layout.arrowWidth - layout.textX;
        for (size_t i = 0; i < Slots; ++i)
        {
            auto button = CloneButton(host, row, layout.rowX, layout.rowTop + static_cast<float>(i) * layout.rowStep, i, false);
            if (!button || button->text.size() < 4) return refuse("native settings row contract differs");
            host->buttons.push_back(std::move(button));
        }
        auto rowText = At<void*>(row, 60);    // a row's unselected text
        host->heading = CloneText(host, title, At<float>(title, 4), At<float>(title, 8), At<float>(title, 12));
        host->heading->centered = true;
        host->heading->height = 0.06f;
        host->description = CloneText(host, rowText, layout.textX, layout.rowTop + (Slots + 0.5f) * layout.rowStep, layout.textWidth);
        host->description2 = CloneText(host, rowText, layout.textX, layout.rowTop + (Slots + 1.5f) * layout.rowStep, layout.textWidth);
        const float rowTextY = At<float>(rowText, 8) - At<float>(row, 8);
        host->above = CloneText(host, rowText, layout.scrollX, layout.rowTop + rowTextY, layout.arrowWidth);
        host->below = CloneText(host, rowText, layout.scrollX, layout.rowTop + (Slots - 1) * layout.rowStep + rowTextY, layout.arrowWidth);
        host->above->centered = host->below->centered = true;
        for (size_t i = 0; i < 6; ++i)
            host->helpText.push_back(CloneText(host, rowText, layout.textX, layout.rowTop + static_cast<float>(i) * layout.rowStep, layout.textWidth));
        if (backIcon)
        {
            // Its normal-state picture, clicked like the game's own Back icon
            auto* picture = At<void*>(backIcon, 60);
            Bytes bitmap(56);
            std::memcpy(bitmap.data(), At<uint32_t>(backIcon, 76) ? picture : At<void*>(backIcon, 40), bitmap.size());
            host->backIcon = { At<float>(backIcon, 4), At<float>(backIcon, 8), At<float>(backIcon, 12), At<float>(backIcon, 16) };
            host->backIconShown = true;
            host->backdrop.push_back(std::move(bitmap));
        }
        if (host->pause)
        {
            // The pause menu column the game's page shows beside it: Plugins highlighted instead of Settings
            void* highlighted = nullptr;
            void* plain = nullptr;
            float last = -INFINITY, previous = -INFINITY;
            for (auto text : textElements)
            {
                if (At<float>(text, 4) + At<float>(text, 12) > At<float>(title, 4) + 0.001f) continue;
                Bytes copy(120);
                std::memcpy(copy.data(), text, copy.size());
                if (At<uint32_t>(text, 44) == 0)
                {
                    const bool lit = At<float>(text, 28) < 0.5f;  // the yellow one
                    if (lit) highlighted = text; else plain = text;
                    const float y = At<float>(text, 8);
                    if (y > last) { previous = last; last = y; }
                    else if (y > previous) previous = y;
                }
                host->backdrop.push_back(std::move(copy));
            }
            if (!highlighted || !plain || !std::isfinite(previous)) return refuse("native pause column contract differs");
            for (auto& copy : host->backdrop)
            {
                if (copy.size() == 120 && At<float>(copy.data(), 8) == At<float>(highlighted, 8) && At<float>(copy.data(), 4) == At<float>(highlighted, 4))
                    for (size_t offset : { 20u, 24u, 28u, 32u }) At<float>(copy.data(), offset) = At<float>(plain, offset);
            }
            host->pluginsLabel = CloneText(host, highlighted, At<float>(highlighted, 4), last + (last - previous), At<float>(highlighted, 12));
            host->pluginsLabel->value = Wide(FixSettings().GetTexts().Resolve("@menu.plugins"));
        }
        for (auto& bytes : host->backdrop) host->decor.push_back(bytes.data());
        if (host->pluginsLabel) host->decor.push_back(host->pluginsLabel->bytes.data());
        if (host->buttons.size() + 4 + host->decor.size() > 50) return refuse("native page element capacity exceeded");
        return true;
    }
    inline bool Prepare(void* root)
    {
        auto refuse = [](const char* reason) { Log(reason); return false; };
        if (!GuiService.is_initialized() || !GuiService.get()) return refuse("native MEC preparation: GUI service not ready");
        if (!FontService.is_initialized() || !FontService.get()) return refuse("native MEC preparation: font service not ready");
        if (At<uintptr_t>(root, 0) != rootClass) return refuse("native MEC preparation: root class differs");
        if (At<uintptr_t>(GuiService.get(), 0) != guiClass) return refuse("native MEC preparation: GUI class differs");
        if (At<uintptr_t>(FontService.get(), 0) != fontClass) return refuse("native MEC preparation: font class differs");
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
        // The pause menu leaves a gap before its conditional mission rows.
        host->first = host->pause ? 13 : 1; host->last = 2;
        auto first = At<void*>(host->parent, 100 + host->first * 4), last = At<void*>(host->parent, 100 + host->last * 4);
        if (At<uintptr_t>(first, 0) != buttonClass || At<uintptr_t>(last, 0) != buttonClass) return refuse("native MEC preparation: navigation button class differs");
        auto templateButton = At<void*>(host->parent, host->pause ? 100 : 104);
        if (At<uintptr_t>(templateButton, 0) != buttonClass) return refuse("native MEC preparation: template button class differs");
        for (auto offset : { 4u, 8u, 12u, 16u }) if (!std::isfinite(At<float>(templateButton, offset))) return refuse("native MEC preparation: template geometry is non-finite");
        if (At<float>(templateButton, 12) <= 0 || At<float>(templateButton, 16) <= 0 || At<float>(templateButton, 16) >= 0.045f) return refuse("native MEC preparation: template geometry is outside native row bounds");
        float entryY = 0.716667f;
        if (host->pause)
        {
            const float previousY = At<float>(last, 8), nextY = At<float>(first, 8);
            if (!std::isfinite(previousY) || !std::isfinite(nextY) || nextY - previousY < 2.0f * At<float>(templateButton, 16))
                return refuse("native MEC preparation: pause entry gap unavailable");
            entryY = (previousY + nextY) * 0.5f;
        }
        host->entry = CloneButton(host, templateButton, host->pause ? At<float>(templateButton, 4) : 0.3875f, entryY, host->originalRows, true);
        if (!host->entry || host->entry->text.empty()) return false;
        auto languageTag = Language(); FixSettings().GetTexts().Select(languageTag);
        SetText(*host->entry, FixSettings().GetTexts().Resolve("@menu.plugins"), "");
        memcpy(host->page.data(), host->parent, 84);
        strcpy_s(reinterpret_cast<char*>(host->page.data()), 8, "WFPMenu");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 17), 32, "<NoAction>");
        strcpy_s(reinterpret_cast<char*>(host->page.data() + 49), 32, "<NoAction>");
        At<uint8_t>(host->page.data(), 16) = At<uint8_t>(host->page.data(), 81) = 0;
        At<uint32_t>(host->page.data(), 12) = At<uint32_t>(host->page.data(), 304) = 0;
        if (!Design(host, root, count)) return false;
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
        if (!Prepare(static_cast<uint8_t*>(self) - 4)) Log("native MEC host contract unsupported");
        for (auto i = texts.begin(); i != texts.end();) { if (i->second.first.expired()) i = texts.erase(i); else ++i; }
        for (auto i = buttons.begin(); i != buttons.end();) { if (i->second.first.expired()) i = buttons.erase(i); else ++i; }
        return result;
    }
    inline bool BackPressed(uint32_t* state)
    {
        if (!state || !state[0]) return false;
        auto* input = *reinterpret_cast<void**>(state[0]);
        if (!input) return false;
        return !Virtual<bool(__thiscall*)(void*, int)>(input, 8)(input, 7) &&
            Virtual<bool(__thiscall*)(void*, int)>(input, 4)(input, 7);
    }
    // Input the game's own lists don't have: Page Up/Down and the shoulders, Home/End, the wheel (one row per notch)
    // and the scroll arrows (click or hold). The native hover moves the selection only while the mouse moves.
    inline void Scrolling(const std::shared_ptr<Host>& host)
    {
        auto& window = host->model.Window(host->modelPage);
        const auto slot = At<uint32_t>(host->page.data(), 304);
        if (slot < host->items.size()) window.Point(slot);
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const bool input = foregroundProcess == GetCurrentProcessId() && !(blocksInput && blocksInput());
        auto key = [&](int code) { return input && (GetAsyncKeyState(code) & 0x8000) != 0; };
        const uint64_t now = GetTickCount64();
        bool moved = false;
        for (int notches = host->wheel.Take(std::exchange(MouseRead::MenuWheel, 0)); notches; notches += notches > 0 ? -1 : 1)
            moved |= window.Scroll(notches > 0 ? -1 : 1);
        const int page = key(VK_PRIOR) || (input && IsJoyButtonPressed(5)) ? -1 : key(VK_NEXT) || (input && IsJoyButtonPressed(6)) ? 1 : 0;
        if (host->pageRepeat.Step(page != 0, now) && page) moved |= window.Page(page);
        // A held Up/Down: the game's own repeat stops at the last shown row, the list goes on from there
        XINPUT_STATE pad{};
        DWORD padSlot = 0;
        const bool padRead = input && PadInput::Read(pad, padSlot);
        const WORD padButtons = padRead ? pad.Gamepad.wButtons : 0;
        const int stick = host->holdStick.Direction(padRead ? pad.Gamepad.sThumbLY : 0);
        const int held = key(VK_UP) || (padButtons & XINPUT_GAMEPAD_DPAD_UP) || stick > 0 ? -1 :
            key(VK_DOWN) || (padButtons & XINPUT_GAMEPAD_DPAD_DOWN) || stick < 0 ? 1 : 0;
        if (host->holdRepeat.Step(held != 0, now, false) && held && window.Shown() &&
            window.Slot() == (held > 0 ? window.Shown() - 1 : 0))
        {
            Move(host, held);
            moved = true;
        }
        static bool homeDown = false, endDown = false;
        const bool home = key(VK_HOME), end = key(VK_END);
        if (home && !homeDown) moved |= window.Edge(-1);
        if (end && !endDown) moved |= window.Edge(1);
        homeDown = home; endDown = end;
        // The mouse: the scroll arrows (held: repeated), the arrows around a value and the Back icon (clicked)
        const auto& layout = host->layout;
        const uintptr_t manager = InputManager;
        const bool down = input && manager && (*reinterpret_cast<uint8_t*>(manager + 0x224 + 12) & 0x80);
        const bool click = down && !host->mouseDown;
        host->mouseDown = down;
        const float x = At<float>(host->page.data(), 92), y = At<float>(host->page.data(), 96);
        // A row's band is the game's own button height; the gaps between rows hit nothing, as with the game's rows
        const float row = (y - layout.rowTop) / layout.rowStep;
        const size_t rowSlot = row >= 0 && y - layout.rowTop - std::floor(row) * layout.rowStep < layout.rowHeight ? static_cast<size_t>(row) : SIZE_MAX;
        int arrow = 0;
        if (down && x >= layout.rightX + layout.arrowWidth && x < layout.scrollX + layout.arrowWidth * 2)
        {
            const float top = layout.rowTop, bottom = layout.rowTop + Slots * layout.rowStep;
            if (window.Above() && y >= top - layout.rowStep * 0.5f && y < top + layout.rowStep) arrow = -1;
            else if (window.Below() && y >= bottom - layout.rowStep && y < bottom + layout.rowStep * 0.5f) arrow = 1;
        }
        if (host->arrowRepeat.Step(arrow != 0, now) && arrow) moved |= window.Scroll(arrow);
        Navigate(host, moved);
        if (!click || !Active(host)) return;
        if (rowSlot < host->items.size() && host->items[rowSlot].enabled && !host->items[rowSlot].value.empty())
        {
            // The arrows step the value; the value itself acts as accepting the row
            for (auto [areaX, areaWidth, action] : { std::tuple{ layout.leftX, layout.arrowWidth, 4 }, std::tuple{ layout.rightX, layout.arrowWidth, 5 },
                std::tuple{ layout.valueX, layout.valueWidth, 6 } })
            {
                if (x < areaX || x >= areaX + areaWidth) continue;
                host->pending = action; host->pendingRow = rowSlot;
                PlayMoveSound();
                Action(host);
                return;
            }
        }
        const auto& icon = host->backIcon;
        if (host->backIconShown && x >= icon[0] && x < icon[0] + icon[2] && y >= icon[1] && y < icon[1] + icon[3])
        {
            const size_t back = host->model.Back(host->modelPage);
            if (back == SIZE_MAX) Close(host, true);
            else
            {
                host->modelPage = back;
                PlayMoveSound();
                Populate(host);
            }
        }
    }
    inline char __fastcall Process(void* self, void*, uint32_t* state)
    {
        auto found = hosts.find(self);
        auto host = found == hosts.end() ? nullptr : found->second;
        bool active = host && Active(host);
        if (active && host->help && BackPressed(state))
        {
            EndHelp(host);
            state[5] &= ~8u;
            At<uint32_t>(self, 16) = 0;
            return 0;
        }
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
            const size_t back = host->model.Back(host->modelPage);
            if ((state[5] & 8) && parent && back != SIZE_MAX && At<int>(GuiService.get(), 33572) == host->menuId)
            {
                // Native Back already popped exactly one page. A category or plugin returns to its parent list,
                // on the row it was opened from; pushing again preserves the native parent for the final Back.
                host->modelPage = back;
                Virtual<char(__thiscall*)(void*, const char*)>(host->root, 40)(host->root, "WFPMenu");
                if (Active(host)) Populate(host); else Close(host, false);
            }
            else Close(host, false);
        }
        Action(host);
        if (Active(host) && !host->help) Scrolling(host);
        else MouseRead::MenuWheel = 0;
        if (Active(host) && !host->help && RWGTAMenu::Model::Changed()) Populate(host);
        if (Active(host))
        {
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
            auto selected = At<uint32_t>(host->page.data(), 304);
            if (selected < host->items.size())
            {
                const auto row = host->items[selected].row;
                auto description = host->model.Help(row);
                if (host->model.RestartRequired()) description = host->model.Text("menu.restartwarning") + " " + description;
                const auto text = description.empty() ? L"" : (helpPrefix ? helpPrefix() : L"F1: ") + Wide(description);
                const auto now = GetTickCount64();
                bool show = text != host->describedText;
                if (show)
                {
                    // Whole words in lines as wide as the help line
                    auto style = Style(*host->description);
                    host->describedLines.clear();
                    std::wstring line;
                    size_t position = 0;
                    while (position < text.size())
                    {
                        const auto end = std::min(text.find(L' ', position), text.size());
                        const auto word = text.substr(position, end - position);
                        position = end + 1;
                        if (word.empty()) continue;
                        const auto candidate = line.empty() ? word : line + L" " + word;
                        if (line.empty() || Measure(style, candidate, host->description->height + 0.00001f) <= host->description->width)
                        {
                            line = candidate;
                            continue;
                        }
                        host->describedLines.push_back(std::move(line));
                        line = word;
                    }
                    if (!line.empty()) host->describedLines.push_back(std::move(line));
                    host->describedText = text;
                    host->describedFirst = 0;
                }
                else if (host->describedLines.size() > 2 &&
                    now - host->describedTick >= (host->describedFirst == 0 || host->describedFirst + 2 >= host->describedLines.size() ? 4000u : 2500u))
                {
                    host->describedFirst = host->describedFirst + 2 >= host->describedLines.size() ? 0 : host->describedFirst + 1;
                    show = true;
                }
                if (show)
                {
                    const auto& lines = host->describedLines;
                    host->description->value = host->describedFirst < lines.size() ? lines[host->describedFirst] : L"";
                    host->description2->value = host->describedFirst + 1 < lines.size() ? lines[host->describedFirst + 1] : L"";
                    host->describedTick = now;
                }
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
    // At the entry of the native activation, which also reads its caller's frame: the game's stack stays as it is
    inline void Activate(SafetyHookContext& regs)
    {
        const int id = *reinterpret_cast<int*>(regs.esp + 4);
        std::vector<std::shared_ptr<Host>> keep;
        for (const auto& [root, host] : hosts) if (Active(host) && host->menuId != id) keep.push_back(host);
        for (const auto& host : keep) if (Active(host)) Close(host, true);
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
        textClass = operand("C7 00 ? ? ? ? C3 CC CC CC CC CC CC CC 53 8A 5C 24 08 F6 C3 02 56 8B F1 74 2E", 2);
        bitmapClass = operand("C7 00 ? ? ? ? C3 CC CC CC CC CC CC CC 53 8A 5C 24 08 F6 C3 02 56 8B F1 74 2B", 2);
        buttonClass = operand("C7 01 ? ? ? ? C7 01 ? ? ? ?", 2);
        rootClass = operand("C7 00 ? ? ? ? C7 40 04 ? ? ? ? C7 40 0C ? ? ? ? C7 40 20 ? ? ? ?", 2);
        guiClass = operand("C7 07 ? ? ? ? C7 47 04 ? ? ? ? 8D 88 A4 82 00 00", 2);
        fontClass = operand("C7 06 ? ? ? ? C7 46 04 ? ? ? ? 68 ? ? ? ?", 2);
        nativeFontClass = operand("C7 06 ? ? ? ? C7 46 04 ? ? ? ? 8D BE 84 00 00 00", 2);
        languageClass = operand("C7 00 ? ? ? ? C3 33 C0 C3 6A 44", 2);
        GuiService.SetAddress(operand("8B 35 ? ? ? ? E8 ? ? ? ? 8B F8", 2));
        FontService.SetAddress(operand("A1 ? ? ? ? 8B 08 52 8D 54 24 18", 1));
        LanguageService.SetAddress(operand("8B 0D ? ? ? ? 8B 45 28 8B 11 53", 2));
        buttonAction = reinterpret_cast<uintptr_t>(hook::get_pattern("8B 44 24 04 53 8B 5C 24 0C 55"));
        menuSound = *hook::get_pattern<void**>("8B 0D ? ? ? ? 8B 01 6A 01 FF 50 08 8B 7C 24", 2);
        destroyHook = safetyhook::create_inline(hook::get_pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 57"), &Destroy);
        releaseHook = safetyhook::create_inline(hook::get_pattern("8B 89 B4 42 00 00 A1 ? ? ? ?"), &Release);
        textHook = safetyhook::create_inline(hook::get_pattern("83 EC 58 53 8B D9 C7 44 24 08 00 00 80 3F"), &DrawText);
        processHook = safetyhook::create_inline(hook::get_pattern("53 55 56 8B 74 24 10 57 8B F9 8D 5F 10 8B CB"), &Process);
        switchHook = safetyhook::create_inline(hook::get_pattern("51 53 55 8B A9 14 41 00 00 56"), &Switch);
        activateEntry = safetyhook::create_mid(hook::get_pattern("83 EC 08 53 56 57 8B F9 8D 5F 08 8D 4C 24 18 8D 44 24 0C 8B D3 E8 ? ? ? ? 8B 74 24 0C 3B 74 24 10 75 06 8D B3 A4 82 00 00 8B 8F 1C 83 00 00"), Activate);
        deactivateHook = safetyhook::create_inline(hook::get_pattern("83 EC 08 53 56 57 8B F9 8D 5F 08 8D 4C 24 18 8D 44 24 0C 8B D3 E8 ? ? ? ? 8B 74 24 0C 3B 74 24 10 75 06 8D B3 A4 82 00 00 8B 44 24 18"), &Deactivate);
        // Fusion Rain Droplets keeps its drops off the game's menus while one is shown
        WFP::onGameProcessEvent() += []()
        {
            const auto gui = GuiService.is_initialized() ? GuiService.get() : nullptr;
            RainDroplets::MenuOpen(gui && At<uintptr_t>(gui, 0) == guiClass && At<int>(gui, 33572) != 0);
        };
        loadHook = safetyhook::create_inline(hook::get_pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 83 EC 24 8A 44 24 34"), &Load);
    }
}
