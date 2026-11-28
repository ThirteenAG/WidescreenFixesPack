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
#include <optional>
#include <set>


export module NativeMenu;

import ComVars;

// 7001 MEC screens, controls and font service. Native input owns selection/repeat/hit testing;
// API callbacks run only after native screen input returns. Unsupported executables fail closed.
export namespace DPLMenu
{
    inline std::function<bool()> blocksInput;
    inline std::function<bool(bool)> padHelp;
    inline std::function<bool(int)> pageButton;
    struct HelpControls
    {
        std::wstring toggle;
        std::wstring scroll;
    };
    inline HelpControls (*helpControls)() = [] { return HelpControls{ L"Y", {} }; };
    using Bytes = std::vector<uint8_t>;
    template<class T> T& At(void* p, size_t offset) { return *reinterpret_cast<T*>(static_cast<uint8_t*>(p) + offset); }
    template<class F> F Virtual(void* p, size_t offset) { return reinterpret_cast<F>(At<uintptr_t>(At<void*>(p, 0), offset)); }
    // Native class tables and functions, found by pattern at Init
    inline uintptr_t textClass, bitmapClass, rowClass, rootClass, guiClass, fontClass, nativeFontClass, languageClass;
    inline uintptr_t setStyle, setText, renderText, buttonAction;
    inline void** menuSound;
    inline HMODULE module;
    // Rows the settings page shows at once; the list scrolls through them
    constexpr size_t Slots = 7;
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
        float align = 0;            // 0 left, 0.5 centred, 1 right in its width (the game's own alignment)
        int role = 0;               // a row's label (0) or value (1)
        float narrow = 0, wide = 0; // a label's width beside a value, and alone (a submenu row, centred like the game's)
        bool fitted = false;
        std::optional<std::pair<float, float>> pagePosition;
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
        std::array<uint32_t, 4> children{};     // per state: the label alone, or with value and arrows
        size_t row = 0;
        bool entry = false;
    };
    struct Item { std::string label, value; RWGTAMenu::Row row; bool enabled = true; };
    struct RestartSetting
    {
        std::pair<uintptr_t, std::string> key;
        const AsiSettingsApi* api;
    };
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
        // Where the page's parts go, taken from the game's own settings page
        struct Layout
        {
            float rowX, rowTop, rowStep, rowHeight, labelWidth, leftX, valueX, valueWidth, rightX, arrowWidth, scrollX;
        } layout{};
        Bytes above, below;                     // the game's own list scroll arrows
        void* valueStyle = nullptr;             // the page's value text (colour and size of the game's values)
        std::array<std::array<Bytes, 2>, 2> arrows;  // the game's value arrows (left, right), selected and not
        std::array<float, 4> backButton{};      // the game's BACK button, clicked like the game's
        bool backShown = false, mouseDown = false;
        MenuList::WheelNotches wheel;
        MenuList::Repeat pageRepeat, arrowRepeat, stickRepeat;
        MenuList::StickAxis stick;
        RWGTAMenu::Model model;
        std::vector<Item> items;
        std::vector<RestartSetting> restartSettings;
        size_t modelPage = RWGTAMenu::Model::RootPage;
        bool pause = false, retired = false, open = false, busy = false;
        int pending = 0;
        int menuId = 0;
        size_t pendingRow = 0;
    };
    struct BusyScope
    {
        Host& host;
        explicit BusyScope(Host& value) : host(value) { host.busy = true; }
        ~BusyScope() { host.busy = false; }
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
        const auto selected = FixSettings().Get("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (selected == tag) return selected;
        static const char* tags[] = { "en", "fr", "de", "es", "it", "ja", "zh-CN", "zh-TW", "ko", "pt" };
        if (!LanguageService.is_initialized() || !LanguageService.get() || At<uintptr_t>(LanguageService.get(), 0) != languageClass) return "en";
        int index = Virtual<int(__thiscall*)(void*)>(LanguageService.get(), 84)(LanguageService.get());
        return index >= 0 && index < static_cast<int>(std::size(tags)) ? tags[index] : "en";
    }
    inline std::map<std::pair<uintptr_t, std::string>, std::string> restartValues;
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
    inline std::string ReadRestartValue(const RestartSetting& setting)
    {
        return OtherPlugins::Fetch([&](char* buffer, uint32_t size) { return setting.api->Get(setting.key.second.c_str(), buffer, size); });
    }
    inline void CollectRestartSettings(const asi_settings::LoadedPlugin& plugin, std::vector<RestartSetting>& settings)
    {
        for (const auto& setting : plugin.file.description.settings)
            if (setting.HasValue() && setting.apply == "restart")
                settings.push_back({ { reinterpret_cast<uintptr_t>(plugin.module), setting.id }, plugin.api });
    }
    inline void RememberRestartValues(const std::shared_ptr<Host>& host)
    {
        std::vector<RestartSetting> settings;
        for (const auto& page : host->model.Pages())
            if (page.plugin) CollectRestartSettings(*page.plugin, settings);
        for (const auto& setting : settings)
        {
            if (restartValues.contains(setting.key)) continue;
            auto value = ReadRestartValue(setting);
            if (!Current(host) || !host->model.IsOpen()) return;
            restartValues.try_emplace(setting.key, std::move(value));
        }
        host->restartSettings = std::move(settings);
    }
    inline bool RestartRequired(const std::shared_ptr<Host>& host)
    {
        for (const auto& entry : host->restartSettings)
        {
            const auto setting = entry;
            const auto value = ReadRestartValue(setting);
            if (!Active(host)) return false;
            auto initial = restartValues.find(setting.key);
            if (initial != restartValues.end() && initial->second != value) return true;
        }
        return false;
    }
    inline std::array<uint32_t, 14> Style(Text& text)
    {
        std::array<uint32_t, 14> style{};
        for (size_t i = 0; i < 4; ++i) style[i] = At<uint32_t>(text.bytes.data(), 20 + i * 4);
        style[5] = Virtual<uint32_t(__thiscall*)(void*)>(GuiService.get(), 52)(GuiService.get());
        style[6] = At<uint32_t>(text.bytes.data(), 4); style[7] = At<uint32_t>(text.bytes.data(), 8);
        if (text.pagePosition)
        {
            At<float>(style.data(), 24) = text.pagePosition->first;
            At<float>(style.data(), 28) = text.pagePosition->second;
        }
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
        if (text.centered || text.align > 0)
        {
            const auto measured = Measure(style, text.formatted);
            if (!std::isfinite(measured) || measured > text.width) return 0;
            At<float>(style.data(), 24) += (text.width - measured) * (text.centered ? 0.5f : text.align);
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
            // The list moves after native input returns
            if (!host->items.empty()) { state[5] = 1; state[6] = static_cast<uint32_t>(button->row); }
            host->pending = action; host->pendingRow = button->row;
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
    inline std::unique_ptr<Text> ClonePageText(const std::shared_ptr<Host>& host, void* source, float x, float y, float width, bool full = false)
    {
        auto text = CloneText(host, source, x, y, width, full);
        // Keep page text anchored when the native screen is reopened.
        text->pagePosition = std::pair{ x, y };
        return text;
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
        const auto& layout = host->layout;
        At<float>(button->bytes.data(), 12) = entry ? At<float>(source, 12) : layout.labelWidth;
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
                const uint8_t alignment = At<uint8_t>(native, 71);
                auto text = CloneText(host, native, relativeX, relativeY, entry ? At<float>(source, 12) - relativeX : layout.labelWidth - relativeX);
                text->centered = entry;
                if (!entry)
                {
                    text->align = alignment == 1 ? 1.0f : alignment == 2 ? 0.5f : 0.0f;
                    text->narrow = text->width;
                    text->wide = layout.rightX + layout.arrowWidth - layout.rowX - relativeX;
                }
                At<void*>(button->bytes.data(), 72 + state * 36 + index++ * 4) = text->bytes.data(); button->text.push_back(std::move(text));
                if (!entry)
                {
                    // The value between the game's arrows, centred like its own values
                    auto value = CloneText(host, native, layout.valueX - layout.rowX, relativeY, layout.valueWidth, true);
                    value->centered = true; value->role = 1;
                    if (auto* style = host->valueStyle)
                    {
                        for (size_t offset : { 20u, 24u, 28u, 32u, 72u, 76u }) At<float>(value->bytes.data(), offset) = At<float>(style, offset);
                        // Text of the smaller size sits on the row's text baseline
                        At<float>(value->bytes.data(), 156) += At<float>(native, 16) * (1.0f - At<float>(style, 76) / At<float>(native, 76)) * 0.5f;
                    }
                    At<void*>(button->bytes.data(), 72 + state * 36 + index++ * 4) = value->bytes.data(); button->text.push_back(std::move(value));
                    for (size_t arrow = 0; arrow < 2 && state < 2 && index < 8; ++arrow)
                    {
                        auto& picture = host->arrows[arrow][state];
                        if (picture.empty()) continue;
                        Bytes copy = picture;
                        const float x = arrow ? layout.rightX : layout.leftX;
                        // Children are placed relative to their row
                        At<float>(copy.data(), 72) = x - layout.rowX;
                        At<float>(copy.data(), 76) = 0;
                        At<void*>(copy.data(), 64) = nullptr;
                        At<void*>(button->bytes.data(), 72 + state * 36 + index++ * 4) = copy.data();
                        button->artwork.push_back(std::move(copy));
                    }
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
                if (count >= 8) return nullptr;
                if (At<uintptr_t>(child, 0) == bitmapClass)
                {
                    Bytes copy(80); memcpy(copy.data(), child, copy.size());
                    At<float>(copy.data(), 32) *= 0.5f;
                    At<void*>(button->bytes.data(), 144 + count++ * 4) = copy.data(); button->artwork.push_back(std::move(copy));
                    continue;
                }
                auto found = texts.find(child); if (found == texts.end()) return nullptr;
                auto sourceText = found->second.second;
                auto dim = CloneText(host, child, At<float>(child, 152), At<float>(child, 156), sourceText->width, sourceText->fullValue);
                dim->centered = sourceText->centered; dim->align = sourceText->align; dim->role = sourceText->role;
                dim->narrow = sourceText->narrow; dim->wide = sourceText->wide;
                for (auto offset : {20u,24u,28u}) At<float>(dim->bytes.data(), offset) *= 0.5f;
                At<void*>(button->bytes.data(), 144 + count++ * 4) = dim->bytes.data(); button->text.push_back(std::move(dim));
            }
            At<uint32_t>(button->bytes.data(), 176) = count;
        }
        for (int state = 0; state < 4; ++state) button->children[state] = At<uint32_t>(button->bytes.data(), 104 + state * 36);
        buttons[button->bytes.data()] = {host, button.get()}; return button;
    }
    inline void SetText(Button& button, const std::string& label, const std::string& value)
    {
        for (auto& text : button.text)
        {
            text->value = Wide(text->role == 1 ? value : label);
            if (button.entry || text->role != 0 || text->wide <= 0) continue;
            const bool alone = value.empty();
            const float width = alone ? text->wide : text->narrow;
            if (text->width != width || text->centered != alone) { text->width = width; text->centered = alone; text->fitted = false; }
        }
        // A row without a value (a submenu) shows its label alone, without the value's arrows
        if (!button.entry)
            for (int state = 0; state < 3; ++state)
                At<uint32_t>(button.bytes.data(), 104 + state * 36) = value.empty() ? std::min<uint32_t>(1, button.children[state]) : button.children[state];
    }
    inline void Populate(const std::shared_ptr<Host>& host)
    {
        if (!Active(host)) return;
        RWGTAMenu::Model::Changed().exchange(false);
        std::vector<Item> items;
        auto& model = host->model;
        if (host->modelPage >= model.Pages().size()) host->modelPage = RWGTAMenu::Model::RootPage;
        std::string title = model.Pages()[host->modelPage].title;
        // One continuous list per page (Back is the game's BACK button and key, as on its own pages)
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
        host->heading->value = Wide(title);
        host->description->fullValue = false;
        host->description->value.clear();
        for (size_t i = 0; i < host->items.size(); ++i)
        {
            auto& item = host->items[i];
            auto value = item.enabled ? item.value : model.Text("menu.unavailable");
            SetText(*host->buttons[i], item.label, value);
            At<void*>(host->page.data(), i * 4) = host->buttons[i]->bytes.data();
        }
        size_t element = host->items.size();
        for (auto text : { host->heading.get(), host->description.get(), host->restart.get() })
            At<void*>(host->page.data(), element++ * 4) = text->bytes.data();
        // The game's scroll arrows, only while rows are hidden above or below
        if (window.Above() && !host->above.empty()) At<void*>(host->page.data(), element++ * 4) = host->above.data();
        if (window.Below() && !host->below.empty()) At<void*>(host->page.data(), element++ * 4) = host->below.data();
        for (auto& decoration : host->decorations)
            At<void*>(host->page.data(), element++ * 4) = decoration.data();
        At<uint32_t>(host->page.data(), 256) = static_cast<uint32_t>(element);
        for (size_t i = 0; i < host->buttons.size(); ++i)
            At<uint32_t>(host->buttons[i]->bytes.data(), 68) = i < host->items.size() && !host->items[i].enabled ? 2 : 1;
        const auto selected = static_cast<uint32_t>(std::min(window.Slot(), host->items.empty() ? 0 : host->items.size() - 1));
        At<uint32_t>(host->page.data(), 368) = selected;
        if (selected < host->items.size()) At<uint32_t>(host->buttons[selected]->bytes.data(), 68) = 0;
    }
    inline void ScrollHelp(const std::shared_ptr<Host>& host, int amount)
    {
        if (!Active(host) || !host->help) return;
        const int last = std::max(0, static_cast<int>(host->helpLines.size()) - 6);
        host->helpOffset = static_cast<size_t>(std::clamp(static_cast<int>(host->helpOffset) + amount, 0, last));
        for (size_t i = 0; i < host->helpText.size(); ++i)
            host->helpText[i]->value = host->helpOffset + i < host->helpLines.size() ? host->helpLines[host->helpOffset + i] : L"";
    }
    inline std::wstring HelpFooter(RWGTAMenu::Model& model, const HelpControls& controls)
    {
        auto text = Wide(model.Text("menu.help.controls"));
        const auto scroll = controls.scroll.empty() ? Wide(model.Text("menu.help.padScroll")) : controls.scroll;
        if (const auto position = text.find(L"{toggle}"); position != std::wstring::npos)
            text.replace(position, std::wstring_view(L"{toggle}").size(), controls.toggle);
        if (const auto position = text.find(L"{scroll}"); position != std::wstring::npos)
            text.replace(position, std::wstring_view(L"{scroll}").size(), scroll);
        return text;
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
            if (Measure(style, line, INFINITY) <= host->layout.rightX + host->layout.arrowWidth - host->layout.rowX) continue;
            auto split = line.find_last_of(L' ');
            if (split == std::wstring::npos || split == 0) split = line.size() - 1;
            host->helpLines.push_back(line.substr(0, split));
            line.erase(0, split + (line[split] == L' ' ? 1 : 0));
        }
        if (!line.empty()) host->helpLines.push_back(line);
        host->help = true; host->helpOffset = 0; host->helpSelection = selected;
        host->heading->value = Wide(title ? *title : host->items[selected].label);
        host->description->fullValue = true;
        host->description->value = HelpFooter(host->model, helpControls());
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
        host->help = false; host->open = false; host->model.Close(); host->items.clear(); host->restartSettings.clear(); host->pending = 0;
    }
    inline void PlayMoveSound()
    {
        if (*menuSound) Virtual<void(__thiscall*)(void*, int, int)>(*menuSound, 8)(*menuSound, 0, 0);
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
        BusyScope busy(*host);
        if (!host->open)
        {
            auto selected = At<uint32_t>(host->root, 856);
            if (selected >= host->originalScreens || At<void*>(host->root, 16 + selected * 4) != host->parent) return;
            auto menuId = At<int>(GuiService.get(), 736);
            host->model.OpenMenu(module, Language(), Slots);
            if (Current(host) && host->model.IsOpen()) RememberRestartValues(host);
            if (Current(host) && host->model.IsOpen() && At<int>(GuiService.get(), 736) == menuId &&
                At<uint32_t>(host->root, 856) == selected && At<void*>(host->root, 16 + selected * 4) == host->parent)
            {
                host->menuId = menuId;
                host->open = true; host->modelPage = RWGTAMenu::Model::RootPage;
                host->wheel.Clear(); MenuMouse::Wheel = 0;
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
            host->model.Window(host->modelPage).Point(host->pendingRow);
            if (action == 5 || action == 6)
            {
                Move(host, action == 5 ? -1 : 1);
                PlayMoveSound();
            }
            else if (RWGTAMenu::Model::Opens(item.row))
            {
                if (action == 9)
                {
                    auto page = host->model.Enter(item.row);
                    if (page != SIZE_MAX) host->modelPage = page;
                }
            }
            else
                host->model.Change(item.row, action == 7 ? -1 : action == 8 ? 1 : 0, host->pause);
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
    // The page copies the game's own settings pages: the Video page's frame, title, row and arrow styles and their
    // places, the controls list's scroll arrows and the Options page's BACK button.
    inline bool Design(const std::shared_ptr<Host>& host, void* root, uint32_t count)
    {
        auto screen = [&](const char* name) -> void*
        {
            for (uint32_t i = 0; i < count; ++i)
            {
                auto* page = At<void*>(root, 16 + i * 4);
                if (page && !strncmp(static_cast<char*>(page) + 260, name, 8)) return page;
            }
            return nullptr;
        };
        auto children = [](void* button, int state) { return At<uint32_t>(button, 104 + state * 36); };
        auto child = [](void* button, int state, uint32_t index) { return At<void*>(button, 72 + state * 36 + index * 4); };
        auto is = [&](void* element, uintptr_t type) { return element && At<uintptr_t>(element, 0) == type; };
        // An arrow button's picture in a state (the pause menu's arrows also carry a text)
        auto picture = [&](void* button, int state) -> void*
        {
            for (uint32_t i = 0; i < children(button, state) && i < 8; ++i)
                if (is(child(button, state, i), bitmapClass)) return child(button, state, i);
            return nullptr;
        };
        // The Video page: "sDisplay" in the front end, "3_sDisplay" in the pause menu
        void* source = nullptr;
        for (uint32_t i = 0; i < count && !source; ++i)
        {
            auto* page = At<void*>(root, 16 + i * 4);
            char name[9]{};
            if (page) memcpy(name, static_cast<char*>(page) + 260, 8);
            if (page && strstr(name, "sDisp")) source = page;
        }
        if (!source) return false;
        void* title = nullptr;
        std::vector<void*> headings;
        std::map<float, std::vector<void*>> columns;
        std::vector<void*> arrows;
        for (uint32_t e = 0; e < At<uint32_t>(source, 256); ++e)
        {
            auto* element = At<void*>(source, e * 4);
            if (is(element, bitmapClass))
            {
                Bytes copy(80); memcpy(copy.data(), element, copy.size()); At<void*>(copy.data(), 64) = nullptr;
                host->decorations.push_back(std::move(copy));
            }
            else if (is(element, textClass))
            {
                // The page title: the largest text, the left one of a header pair ("Video" / "Game Paused")
                headings.push_back(element);
                if (!title || At<float>(element, 72) > At<float>(title, 72) ||
                    (At<float>(element, 72) == At<float>(title, 72) && At<float>(element, 4) < At<float>(title, 4))) title = element;
            }
            else if (is(element, rowClass) && At<float>(element, 12) < 0.06f && picture(element, 0)) arrows.push_back(element);
            else if (is(element, rowClass) && children(element, 0) == 1 && is(child(element, 0, 0), textClass) && At<float>(element, 12) >= 0.3f)
                columns[At<float>(element, 4)].push_back(element);
        }
        // The values' style: the page's smaller text that isn't a header
        for (auto text : headings)
            if (At<float>(text, 72) < At<float>(title, 72) && (!host->valueStyle || At<float>(text, 72) < At<float>(host->valueStyle, 72)))
                host->valueStyle = text;
        // The other header texts at the title's height stay as the game shows them
        for (auto text : headings)
        {
            if (text == title || std::abs(At<float>(text, 8) - At<float>(title, 8)) > 0.01f || At<float>(text, 72) != At<float>(title, 72)) continue;
            Bytes copy(172); memcpy(copy.data(), text, copy.size()); At<void*>(copy.data(), 64) = nullptr;
            host->decorations.push_back(std::move(copy));
        }
        // The rows: the column with the most rows
        std::vector<void*>* rows = nullptr;
        for (auto& [x, list] : columns) if (!rows || list.size() > rows->size()) rows = &list;
        if (!title || !rows || rows->size() < 2) return false;
        std::sort(rows->begin(), rows->end(), [](void* a, void* b) { return At<float>(a, 8) < At<float>(b, 8); });
        auto* row = rows->front();
        auto& layout = host->layout;
        layout.rowX = At<float>(row, 4);
        layout.rowHeight = At<float>(row, 16);
        layout.rowStep = INFINITY;
        for (size_t i = 1; i < rows->size(); ++i)
            if (At<float>((*rows)[i], 8) - At<float>((*rows)[i - 1], 8) > 0.01f)
                layout.rowStep = std::min(layout.rowStep, At<float>((*rows)[i], 8) - At<float>((*rows)[i - 1], 8));
        // The value arrows: the pair beside the rows, left and right of the value
        void* left = nullptr;
        void* right = nullptr;
        for (auto arrow : arrows)
        {
            if (At<float>(arrow, 4) < layout.rowX + At<float>(row, 12)) continue;
            if (!left || At<float>(arrow, 4) < At<float>(left, 4)) left = arrow;
            if (!right || At<float>(arrow, 4) > At<float>(right, 4)) right = arrow;
        }
        if (!left || !right || left == right || !std::isfinite(layout.rowStep)) return false;
        // The rows start where the page's own first row of settings does
        layout.rowTop = At<float>(row, 8);
        for (auto arrow : arrows) layout.rowTop = std::min(layout.rowTop, At<float>(arrow, 8));
        layout.arrowWidth = At<float>(left, 12);
        layout.leftX = At<float>(left, 4);
        layout.valueX = layout.leftX + layout.arrowWidth;
        layout.rightX = At<float>(right, 4);
        layout.valueWidth = layout.rightX - layout.valueX;
        layout.labelWidth = layout.leftX - layout.rowX - layout.arrowWidth * 0.5f;
        layout.scrollX = layout.rightX + layout.arrowWidth;
        for (auto [arrow, side] : { std::pair{ left, 0 }, std::pair{ right, 1 } })
            for (int state = 0; state < 2; ++state)
            {
                auto* image = picture(arrow, state) ? picture(arrow, state) : picture(arrow, 0);
                host->arrows[side][state].assign(static_cast<uint8_t*>(image), static_cast<uint8_t*>(image) + 80);
            }
        for (size_t i = 0; i < Slots; ++i)
        {
            auto button = CloneButton(host, row, layout.rowX, layout.rowTop + static_cast<float>(i) * layout.rowStep, i, false);
            if (!button || button->text.empty()) return false;
            host->buttons.push_back(std::move(button));
        }
        auto rowText = child(row, 1, 0);
        host->heading = ClonePageText(host, title, At<float>(title, 4), At<float>(title, 8), At<float>(title, 12));
        host->heading->height = 0.07f;
        const float textWidth = layout.rightX + layout.arrowWidth - layout.rowX;
        const float bottom = layout.rowTop + Slots * layout.rowStep;
        host->description = ClonePageText(host, rowText, layout.rowX, bottom, textWidth);
        host->restart = ClonePageText(host, rowText, layout.rowX, bottom + layout.rowStep * 0.75f, textWidth);
        host->description->height = host->restart->height = 0.045f;
        for (size_t i = 0; i < 6; ++i)
        {
            auto text = ClonePageText(host, rowText, layout.rowX, layout.rowTop + static_cast<float>(i) * layout.rowStep, textWidth, true);
            text->height = 0.05f;
            host->helpText.push_back(std::move(text));
        }
        // The controls list's own scroll arrows, beside the first and last rows
        if (auto* list = screen("kmMappi"))
        {
            std::vector<void*> marks;
            for (uint32_t e = 0; e < At<uint32_t>(list, 256); ++e)
            {
                auto* element = At<void*>(list, e * 4);
                if (is(element, rowClass) && At<float>(element, 12) < 0.04f && picture(element, 0)) marks.push_back(element);
            }
            if (marks.size() == 2)
            {
                if (At<float>(marks[0], 8) > At<float>(marks[1], 8)) std::swap(marks[0], marks[1]);
                for (auto [mark, target, y] : { std::tuple{ marks[0], &host->above, layout.rowTop }, std::tuple{ marks[1], &host->below, bottom - layout.rowStep } })
                {
                    auto* image = picture(mark, 0);
                    target->assign(static_cast<uint8_t*>(image), static_cast<uint8_t*>(image) + 80);
                    At<void*>(target->data(), 64) = nullptr;
                    const float x = layout.scrollX + (layout.arrowWidth - At<float>(image, 12)) * 0.5f;
                    const float top = y + (layout.rowHeight - At<float>(image, 16)) * 0.5f;
                    At<float>(target->data(), 4) = At<float>(target->data(), 48) = x;
                    At<float>(target->data(), 8) = At<float>(target->data(), 52) = top;
                }
            }
        }
        // The game's BACK button of the page it was opened from: its picture and text, clicked like the game's
        for (uint32_t e = 0; e < At<uint32_t>(host->parent, 256); ++e)
        {
            auto* element = At<void*>(host->parent, e * 4);
            if (!is(element, rowClass) || children(element, 1) != 2 || At<float>(element, 8) < 0.8f) continue;
            for (uint32_t i = 0; i < 2; ++i)
            {
                auto* part = child(element, 1, i);
                const size_t size = is(part, textClass) ? 172 : 80;
                Bytes copy(size); memcpy(copy.data(), part, size); At<void*>(copy.data(), 64) = nullptr;
                if (size == 172) { At<float>(copy.data(), 152) = At<float>(copy.data(), 156) = 0; }
                else { At<float>(copy.data(), 48) = At<float>(copy.data(), 4); At<float>(copy.data(), 52) = At<float>(copy.data(), 8); }
                host->decorations.push_back(std::move(copy));
            }
            host->backButton = { At<float>(element, 4), At<float>(element, 8), At<float>(element, 12), At<float>(element, 16) };
            host->backShown = true;
            break;
        }
        return Slots + 5 + host->decorations.size() <= 64;
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
        if (!Design(host, root, count)) return reject("native settings page contract rejected");
        At<void*>(host->page.data(), 0) = host->buttons[0]->bytes.data(); At<uint32_t>(host->page.data(), 256) = 1;
        memcpy(host->firstUp.data(), static_cast<uint8_t*>(first) + 224, 8);
        memcpy(host->lastDown.data(), static_cast<uint8_t*>(last) + 232, 8);
        At<uint8_t>(first, 224) = 0; At<uint32_t>(first, 228) = host->originalRows;
        At<uint8_t>(last, 232) = 0; At<uint32_t>(last, 236) = host->originalRows;
        At<uint8_t>(host->entry->bytes.data(), 224) = At<uint8_t>(host->entry->bytes.data(), 232) = 0;
        At<uint32_t>(host->entry->bytes.data(), 228) = host->last; At<uint32_t>(host->entry->bytes.data(), 236) = host->first;
        At<void*>(host->parent, host->originalRows * 4) = host->entry->bytes.data(); At<uint32_t>(host->parent, 256) = host->originalRows + 1;
        At<void*>(root, 16 + count * 4) = host->page.data(); At<uint32_t>(root, 272) = count + 1;
        hosts[root] = host;
        return true;
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
    // Back one level: a category or plugin returns to its list on the row it was opened from; the plugins list leaves
    inline void GoBack(const std::shared_ptr<Host>& host)
    {
        const size_t back = host->model.Back(host->modelPage);
        if (back == SIZE_MAX) { Close(host, true); return; }
        host->modelPage = back;
        host->pending = 0;
        if (Active(host)) Populate(host);
    }
    // Input the game's own lists don't have: Page Up/Down and the shoulders, Home/End, the wheel (one row per notch),
    // the scroll arrows (click or hold), the value arrows and the BACK button. The game's hover moves the selection
    // only while the mouse moves.
    inline void Scrolling(const std::shared_ptr<Host>& host)
    {
        auto& window = host->model.Window(host->modelPage);
        const auto slot = At<uint32_t>(host->page.data(), 368);
        if (slot < host->items.size()) window.Point(slot);
        DWORD foregroundProcess = 0;
        GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
        const bool input = foregroundProcess == GetCurrentProcessId() && !(blocksInput && blocksInput());
        auto key = [&](int code) { return input && (GetAsyncKeyState(code) & 0x8000) != 0; };
        const uint64_t now = GetTickCount64();
        bool moved = false;
        for (int notches = host->wheel.Take(std::exchange(MenuMouse::Wheel, 0)); notches; notches += notches > 0 ? -1 : 1)
            moved |= window.Scroll(notches > 0 ? -1 : 1);
        const int page = key(VK_PRIOR) || (input && pageButton && pageButton(-1)) ? -1 : key(VK_NEXT) || (input && pageButton && pageButton(1)) ? 1 : 0;
        if (host->pageRepeat.Step(page != 0, now) && page) moved |= window.Page(page);
        // The left stick moves like the d-pad, with its repeat
        XINPUT_STATE pad{};
        DWORD padSlot = 0;
        const int stick = host->stick.Direction(input && PadInput::Read(pad, padSlot) ? pad.Gamepad.sThumbLY : 0);
        if (host->stickRepeat.Step(stick != 0, now) && stick)
        {
            Move(host, stick > 0 ? -1 : 1);
            moved = true;
        }
        static bool homeDown = false, endDown = false;
        const bool home = key(VK_HOME), end = key(VK_END);
        if (home && !homeDown) moved |= window.Edge(-1);
        if (end && !endDown) moved |= window.Edge(1);
        homeDown = home; endDown = end;
        const auto& layout = host->layout;
        // A click is the game's own mouse event of this frame; holding it repeats the scroll arrows
        const bool click = input && At<uint8_t>(host->page.data(), 376) && At<uint32_t>(host->page.data(), 380) == 0;
        const bool down = click || (input && host->mouseDown && (GetAsyncKeyState(VK_LBUTTON) & 0x8000));
        host->mouseDown = down;
        const float x = At<float>(host->page.data(), 384), y = At<float>(host->page.data(), 388);
        int arrow = 0;
        if (down && x >= layout.scrollX && x < layout.scrollX + layout.arrowWidth)
        {
            const float top = layout.rowTop, bottom = layout.rowTop + Slots * layout.rowStep;
            if (window.Above() && y >= top && y < top + layout.rowStep) arrow = -1;
            else if (window.Below() && y >= bottom - layout.rowStep && y < bottom) arrow = 1;
        }
        if (host->arrowRepeat.Step(arrow != 0, now) && arrow) moved |= window.Scroll(arrow);
        if (moved)
        {
            PlayMoveSound();
            Populate(host);
        }
        if (!click || !Active(host)) return;
        // A row's band is the game's own button height; the gaps between rows hit nothing
        const float row = (y - layout.rowTop) / layout.rowStep;
        const size_t rowSlot = row >= 0 && y - layout.rowTop - std::floor(row) * layout.rowStep < layout.rowHeight ? static_cast<size_t>(row) : SIZE_MAX;
        if (rowSlot < host->items.size() && host->items[rowSlot].enabled && !host->items[rowSlot].value.empty())
        {
            // The arrows step the value; the value itself acts as accepting the row
            for (auto [areaX, areaWidth, action] : { std::tuple{ layout.leftX, layout.arrowWidth, 7 }, std::tuple{ layout.rightX, layout.arrowWidth, 8 },
                std::tuple{ layout.valueX, layout.valueWidth, 9 } })
            {
                if (x < areaX || x >= areaX + areaWidth) continue;
                // Taken by the next frame's native-input pass, as the game's own clicks
                host->pending = action; host->pendingRow = rowSlot;
                return;
            }
        }
        const auto& button = host->backButton;
        if (host->backShown && x >= button[0] && x < button[0] + button[2] && y >= button[1] && y < button[1] + button[3])
            GoBack(host);
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
        if (active && Active(host) && back && host->model.Parent(host->modelPage) != SIZE_MAX)
        {
            // Native private exit has no animation or command. Cancel its queued parent pop for
            // inner Back, then show the parent list on the row this page was opened from.
            At<uint8_t>(host->root, 876) &= ~0x10u;
            state[5] &= ~8u;
            BusyScope busy(*host);
            GoBack(host);
            return result;
        }
        else if (active && !Active(host)) Close(host, false);
        Action(host);
        BusyScope busy(*host);
        if (Active(host) && !host->help) Scrolling(host);
        else MenuMouse::Wheel = 0;
        if (Active(host) && !host->help && RWGTAMenu::Model::Changed()) Populate(host);
        if (Active(host))
        {
            const bool restart = RestartRequired(host);
            if (!Active(host)) return result;
            host->restart->value = restart ? Wide(host->model.Text("menu.restartwarning")) : L"";
            static bool helpDown = false;
            DWORD foregroundProcess = 0;
            GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
            const bool acceptHelp = foregroundProcess == GetCurrentProcessId() && !(blocksInput && blocksInput());
            const bool down = (GetAsyncKeyState(VK_F1) & 0x8000) != 0;
            const bool padPressed = padHelp && padHelp(acceptHelp);
            if (acceptHelp && ((down && !helpDown) || padPressed))
            {
                if (host->help) EndHelp(host); else Help(host);
            }
            helpDown = down;
            if (host->help)
            {
                host->description->value = HelpFooter(host->model, helpControls());
                return result;
            }
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
                host->description->value = description.empty() ? L"" : L"F1 / " + helpControls().toggle + L": " + Wide(description);
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
        menuSound = *hook::get_pattern<void**>("8B 0D ? ? ? ? 8B 01 6A 00 6A 00 FF 50 08 8B 0D", 2);
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            Log("native menu lifetime pin refused");
            return;
        }
        std::vector<RestartSetting> settings;
        for (const auto& plugin : asi_settings::FindLoadedPlugins(module, "en")) CollectRestartSettings(plugin, settings);
        for (const auto& setting : settings)
            if (!restartValues.contains(setting.key)) restartValues.emplace(setting.key, ReadRestartValue(setting));
        releaseHook = safetyhook::create_inline(hook::get_pattern("53 56 57 8B F1 33 FF F6 86 6C 03 00 00 01"), &Release);
        textHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 E4 F8 83 EC 54 A1 ? ? ? ? 53 56 33 F6 8B D9 57 8D 4C 24 28 89 44 24 0C"), &DrawText);
        processHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 51 51 53 8B D9 F6 83 6C 03 00 00 02"), &Process);
        switchHook = safetyhook::create_inline(hook::get_pattern("53 56 8B F1 8B 86 58 03 00 00"), &Switch);
        pushHook = safetyhook::create_inline(hook::get_pattern("53 56 8B F1 33 DB 39 9E 10 01 00 00"), &Push);
        activateHook = safetyhook::create_inline(hook::get_pattern("56 57 8B F1 8D 44 24 0C 50 8D 4E 08"), &Activate);
        deactivateHook = safetyhook::create_inline(hook::get_pattern("53 56 57 8B F9 8D 44 24 10 50"), &Deactivate);
        // Fusion Rain Droplets keeps its drops off the game's menus while one is shown
        WFP::onGameProcessEvent() += []()
        {
            const auto gui = GuiService.is_initialized() ? GuiService.get() : nullptr;
            RainDroplets::MenuOpen(gui && At<uintptr_t>(gui, 0) == guiClass && At<int>(gui, 736) != -1);
        };
        loadHook = safetyhook::create_inline(hook::get_pattern("55 8B EC 83 EC 3C 53 56 8B F1 57"), &Load);
    }
}
