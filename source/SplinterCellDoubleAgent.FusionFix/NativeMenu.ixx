module;

#include <stdafx.h>
#include <stdexcept>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/RainDroplets.hpp"
#include <atomic>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

export module NativeMenu;

export import NativeMenuContracts;
export import MenuText;
import ComVars;
import WidescreenHUD;

export namespace DAMenu
{
    inline HMODULE hostModule = nullptr;
    inline void* (__stdcall* createArea)(int) = nullptr;
    inline void* (__fastcall* createNode)(void*, void*, int) = nullptr;
    inline void* (__stdcall* createFrame)(int) = nullptr;
    inline void (__fastcall* addNode)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* addFrame)(void*, void*, void*) = nullptr;
    inline void (__cdecl* linkNeighbors)(void*) = nullptr;
    inline void (__fastcall* areaTime)(void*, void*, unsigned int, int) = nullptr;
    inline void* (__stdcall* createHandler)(int) = nullptr;
    inline void (__fastcall* addHandler)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* findNode)(void*, void*, uint32_t) = nullptr;
    inline void* (__fastcall* findPage)(void*, void*, uint32_t) = nullptr;
    inline void (__fastcall* addPackageChild)(void*, void*, void*) = nullptr;
    inline void* (__fastcall* wideAssign)(void*, void*, const wchar_t*, size_t) = nullptr;
    inline uint8_t (__fastcall* selectNode)(void*, void*) = nullptr;
    inline uint8_t (__fastcall* blurNode)(void*, void*) = nullptr;
    inline void* (__fastcall* topMenu)(void*, void*) = nullptr;
    inline void* (__fastcall* menuPackage)(void*, void*, const char*) = nullptr;
    inline void* (__fastcall* interaction)(void*, void*) = nullptr;
    inline void (__fastcall* addCursor)(void*, void*, void*) = nullptr;
    inline const wchar_t* (__fastcall* fullName)(void*, void*, wchar_t*) = nullptr;
    inline const wchar_t* (__cdecl* gameLanguage)() = nullptr;
    inline SafetyHookInline displayHook, tickHook, uninitializeHook, destroyHook, defaultsHook, navigateHook;
    inline bool busy = false;
    struct BusyGuard
    {
        bool previous = busy;
        BusyGuard() { busy = true; }
        ~BusyGuard() { busy = previous; }
    };
    inline RWGTAMenu::Model model;

    template<class T> T Read(void* object, size_t offset)
    { return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset); }
    template<class T> void Write(void* object, size_t offset, T value)
    { *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) = value; }
    template<class Result, class... Args> Result Virtual(void* object, size_t slot, Args... args)
    {
        auto method = Read<void*>(Read<void*>(object, 0), slot * 4);
        return reinterpret_cast<Result (__fastcall*)(void*, void*, Args...)>(method)(object, nullptr, args...);
    }
    inline void Delete(void* object) { if (object) Virtual<void*>(object, 0, int(1)); }
    inline uint32_t Id(std::string_view text)
    {
        uint32_t crc = 0xFFFFFFFF;
        for (auto c : text)
        {
            crc ^= uint8_t(c);
            for (int i = 0; i < 8; ++i) crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320 : 0);
        }
        return ~crc;
    }
    inline void LogOnce(const std::string& phase, void*, const std::string& reason)
    {
        static std::set<std::pair<std::string, std::string>> written;
        if (!written.emplace(phase, reason).second) return;
        auto line = "DA native menu " + phase + ": " + reason + "\n";
        OutputDebugStringA(line.c_str());
    }
    inline void* Factory()
    { return FactorySlot; }
    inline void* Data(void* node) { return node ? Read<void*>(node, 8) : nullptr; }
    inline int Kind(void* node) { return Data(node) ? Virtual<int>(Data(node), 3) : -1; }
    inline bool Nodes(void* area, std::vector<void*>& out)
    {
        auto start = Read<void**>(area, 12), end = Read<void**>(area, 16), limit = Read<void**>(area, 20);
        auto first = uintptr_t(start), last = uintptr_t(end), capacity = uintptr_t(limit);
        if (last < first || capacity < last || (last - first) % 4 || (last - first) / 4 > 256 || (!start && end)) return false;
        out.clear(); if (start) out.assign(start, end); return true;
    }
    inline std::string Language();
    inline DAText::Metrics TextMetrics(void* node)
    {
        DAText::Metrics metrics;
        auto data = Data(node);
        if (!data || Kind(node) != 8) return metrics;
        auto point = Read<void*>(data, 4);
        auto font = Read<void*>(data, 8);
        if (!point || !font) return metrics;
        const auto baseHeight = Read<uint8_t>(font, 73);
        const auto height = Read<uint16_t>(point, 22);
        if (!baseHeight || !height) return metrics;
        // Native Magma text uses two thirds of the requested font-height ratio.
        const auto scaled = Virtual<uint8_t>(font, 5) != 0;
        metrics.scale = scaled ? double(height) / baseHeight * (2.0 / 3.0) : 1.0;
        metrics.spacing = metrics.scale * Read<int16_t>(point, 18) * baseHeight * 0.001;
        metrics.width = Read<int16_t>(point, 10) - Read<int16_t>(point, 8) - 4.0;
        if (!std::isfinite(metrics.scale) || metrics.scale <= 0 || metrics.scale > 64 ||
            !std::isfinite(metrics.spacing) || !std::isfinite(metrics.width) || metrics.width <= 0) return {};
        metrics.font = font;
        metrics.available = [font](wchar_t c)
        {
            auto page = Read<void*>(font, 88 + (unsigned(c) >> 8) * 4);
            if (!page) return false;
            const auto glyph = Read<uint16_t>(page, (unsigned(c) & 255) * 2);
            return glyph < Read<uint16_t>(font, 80) && (glyph != 0 || c == L' ');
        };
        return metrics;
    }
    inline std::wstring Wide(const std::string& text)
    {
        if (text.size() > 65536) return {};
        int length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), nullptr, 0);
        if (length <= 0) return {};
        std::wstring wide(length, L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), int(text.size()), wide.data(), length)) return {};
        return wide;
    }
    inline std::wstring Styled(const std::string& text, const DAText::Metrics& metrics)
    {
        auto wide = Wide(text);
        for (auto& c : wide) if (c < 32 || c == 8232 || c == 8233) c = L' ';
        auto tag = Language();
        std::wstring locale(tag.begin(), tag.end());
        return DAText::Style(std::move(wide), metrics, locale.c_str());
    }
    inline bool CaptionWide(void* node, const std::wstring& text)
    {
        const auto metrics = TextMetrics(node);
        if (!metrics.font) return false;
        const auto fitted = DAText::Fit(text, metrics, metrics.width);
        wideAssign(static_cast<uint8_t*>(Data(node)) + 12, nullptr, fitted.c_str(), fitted.size());
        return true;
    }
    inline bool Caption(void* node, const std::string& text)
    {
        const auto metrics = TextMetrics(node);
        return metrics.font && CaptionWide(node, Styled(text, metrics));
    }
    struct Rect { int16_t left, top, right, bottom; };
    inline Rect Bounds(void* node)
    {
        // Magma stores opposite edges together, unlike Win32 RECT.
        struct NativeRect { int16_t left, right, top, bottom; };
        NativeRect rect{};
        Virtual<void*>(Data(node), 4, &rect);
        return { rect.left, rect.top, rect.right, rect.bottom };
    }
    inline void* Font(void* area, int depth = 0)
    {
        if (!area || depth > 16) return nullptr;
        std::vector<void*> nodes;
        if (!Nodes(area, nodes)) return nullptr;
        for (auto node : nodes)
        {
            auto kind = Kind(node);
            if (kind == 8 && Read<void*>(Data(node), 8)) return Read<void*>(Data(node), 8);
            if (kind == 0 || kind == 1 || kind == 2 || kind == 3 || kind == 4 || kind == 9 || kind == 10 || kind == 12)
                if (auto font = Font(Read<void*>(Data(node), 16), depth + 1)) return font;
        }
        return nullptr;
    }
    inline bool Pause(void* manager, bool& pause)
    {
        auto owner = interaction(manager, nullptr);
        if (!owner) return false;
        wchar_t buffer[1024]{};
        auto name = fullName(owner, nullptr, buffer);
        if (!name) return false;
        std::wstring_view type(name);
        if (type.starts_with(L"SCMagmaInteractionPREGame ")) { pause = false; return true; }
        if (type.starts_with(L"SCMagmaInteractionSPGame ")) { pause = true; return true; }
        return false;
    }
    inline bool Modal(void* manager)
    {
        // Native IsMessageBoxPresent filters a single message type. The
        // verified manager list covers every modal before we handle any input.
        int count = Read<int>(manager, 80);
        return count < 0 || count > 64 || count > 0;
    }
    inline std::string Language()
    {
        const auto chosen = FixSettings().Get("MAIN.TextLanguage");
        if (!chosen.empty() && chosen != "auto") return chosen;
        auto value = gameLanguage ? gameLanguage() : nullptr;
        if (!value) return "en";
        if (!_wcsicmp(value, L"fra") || !_wcsicmp(value, L"fre")) return "fr";
        if (_wcsicmp(value, L"deu") == 0) return "de";
        if (_wcsicmp(value, L"ita") == 0) return "it";
        if (!_wcsicmp(value, L"esp") || !_wcsicmp(value, L"spa")) return "es";
        if (_wcsicmp(value, L"rus") == 0) return "ru";
        return "en";
    }
    inline void Visible(void* node, bool visible)
    {
        auto flags = Read<uint8_t>(node, 24);
        Write(node, 24, uint8_t((flags & ~1) | (visible ? 1 : 0)));
    }
    inline void Enabled(void* node, bool enabled)
    {
        auto flags = Read<uint8_t>(node, 60);
        Write(node, 60, uint8_t((flags & ~2) | (enabled ? 2 : 0)));
    }
    struct TextNode { void* node = nullptr; void* frame = nullptr; };
    inline TextNode Text(void* font, int x, int y, int width, int height, int fontHeight, int align)
    {
        TextNode result;
        auto factory = Factory();
        result.node = factory ? createNode(factory, nullptr, 8) : nullptr;
        auto frame = result.node ? createFrame(8) : nullptr;
        if (!frame) { Delete(result.node); return {}; }
        result.frame = Read<void*>(frame, 4);
        if (!Data(result.node) || !Read<void*>(Data(result.node), 4) || !result.frame || Kind(result.node) != 8)
        { Delete(frame); Delete(result.node); return {}; }
        auto data = Data(result.node);
        Write(data, 8, font);
        Write(data, 40, uint32_t((Read<uint32_t>(data, 40) & ~7) | (align & 7)));
        Write(data, 44, uint8_t(0x10));
        Write(result.frame, 8, int16_t(x)); Write(result.frame, 10, int16_t(x + width));
        Write(result.frame, 12, int16_t(y)); Write(result.frame, 14, int16_t(y + height));
        Write(result.frame, 22, int16_t(fontHeight));
        addFrame(result.node, nullptr, frame);
        return result;
    }
    inline void Color(TextNode& text, uint32_t rgba)
    {
        const bool image = Kind(text.node) == 5;
        const auto color = image ? 0xFFFFFFFFu : rgba;
        Write(text.frame, 4, color);
        auto current = Read<void*>(Data(text.node), 4);
        if (current) Write(current, 4, color);
        if (image)
        {
            for (size_t offset = 36; offset < 52; offset += 4)
            {
                Write(text.frame, offset, rgba);
                if (current) Write(current, offset, rgba);
            }
        }
    }
    // An image or text of a list row in its three looks: normal, focused, unavailable
    struct RowPart
    {
        TextNode part;
        size_t size = 0;
        std::array<std::array<uint8_t, 52>, 3> looks{};
    };
    struct Button
    {
        void* node = nullptr;
        void* theme = nullptr;
        TextNode label, value;
        std::vector<RowPart> parts;
        int16_t labelRight = 0, valueRight = 0;
    };
    // kind 2: a button; kind 1: an element group, drawn like a button but never under the mouse or focused
    inline Button CreateButton(void* font, uint32_t id, int x, int y, int width, int height, int fontHeight, bool value, int kind = 2)
    {
        Button result;
        result.theme = createArea(2);
        auto factory = Factory();
        result.node = factory ? createNode(factory, nullptr, kind) : nullptr;
        auto frame = result.node ? createFrame(kind) : nullptr;
        if (!result.theme || !result.node || !frame || !Read<void*>(frame, 4) || !Data(result.node) || !Read<void*>(Data(result.node), 4) || Kind(result.node) != kind)
        { Delete(frame); Delete(result.node); Delete(result.theme); return {}; }
        result.label = Text(font, 0, 0, value ? width * 62 / 100 : width, height, fontHeight, 0);
        if (value) result.value = Text(font, width * 62 / 100, 0, width - width * 62 / 100, height, fontHeight, 2);
        if (!result.label.node || (value && !result.value.node))
        {
            Delete(result.label.node); Delete(result.value.node); Delete(frame); Delete(result.node); Delete(result.theme); return {};
        }
        addNode(result.theme, nullptr, result.label.node);
        if (result.value.node) addNode(result.theme, nullptr, result.value.node);
        Write(result.node, 4, id);
        Write(Data(result.node), 16, result.theme);
        auto point = Read<void*>(frame, 4);
        Write(point, 8, int16_t(x)); Write(point, 10, int16_t(y));
        addFrame(result.node, nullptr, frame);
        areaTime(result.theme, nullptr, 0, 1);
        return result;
    }
    // The page's own setting rows (Invert look, Draw gun): a bar, a box, the focus arrow, a label and a value, each
    // with keyframes at 0 (normal), 10 (focused) and 20 (unavailable)
    struct RowStyle
    {
        struct Part { void* node; int kind; size_t size; std::array<std::array<uint8_t, 52>, 3> looks; };
        std::vector<Part> parts;
        int width = 0, height = 0, pitch = 0;
        size_t label = SIZE_MAX, value = SIZE_MAX;
    };
    inline std::optional<RowStyle> FindRowStyle(void* parent)
    {
        std::vector<void*> nodes, children;
        if (!Nodes(parent, nodes)) return {};
        std::optional<RowStyle> found;
        int top = 0;
        for (auto node : nodes)
        {
            if (Kind(node) != 2 || !Nodes(Read<void*>(Data(node), 16), children) || children.size() != 6) continue;
            RowStyle style;
            size_t images = 0;
            for (auto child : children)
            {
                const auto kind = Kind(child);
                auto list = Read<void*>(child, 12);
                auto first = list ? Read<void**>(list, 4) : nullptr, last = list ? Read<void**>(list, 8) : nullptr;
                if ((kind != 5 && kind != 8) || !first || last - first < 3 || last - first > 16) break;
                RowStyle::Part part{ child, kind, size_t(kind == 5 ? 52 : 28), {} };
                bool timed = true;
                for (int i = 0; i < 3; ++i)
                {
                    // Keyframe times are the low 12 bits, signed
                    timed &= (int(Read<uint32_t>(first[i], 12) << 20) >> 20) == i * 10;
                    std::memcpy(part.looks[i].data(), Read<uint8_t*>(first[i], 4), part.size);
                }
                if (!timed) break;
                if (kind == 5) ++images;
                else (style.label == SIZE_MAX ? style.label : style.value) = style.parts.size();
                style.parts.push_back(part);
            }
            if (style.parts.size() != 6 || images != 4 || style.value == SIZE_MAX) continue;
            const auto bounds = Bounds(node);
            if (found)
            {
                // The next row of the same style gives the spacing
                if (bounds.top > top && bounds.right - bounds.left == found->width) { found->pitch = bounds.top - top; break; }
                continue;
            }
            style.width = bounds.right - bounds.left; style.height = bounds.bottom - bounds.top;
            if (style.width < 200 || style.height < 12) continue;
            found = std::move(style); top = bounds.top;
        }
        if (found && found->pitch <= found->height) found->pitch = found->height + found->height / 2;
        return found;
    }
    // Copies a look into the part's keyframe and current state
    inline void RowLook(RowPart& part, int look)
    {
        for (auto point : { part.part.frame, Read<void*>(Data(part.part.node), 4) })
            std::memcpy(static_cast<uint8_t*>(point) + 4, part.looks[look].data() + 4, part.size - 4);
    }
    // A row of the page's own style, stretched to width: the bar and its fill span the row, the value keeps its
    // distance to the right end
    inline Button CreateRow(const RowStyle& style, uint32_t id, int x, int y, int width)
    {
        Button result;
        result.theme = createArea(2);
        auto factory = Factory();
        result.node = factory ? createNode(factory, nullptr, 2) : nullptr;
        auto frame = result.node ? createFrame(2) : nullptr;
        auto fail = [&] { for (auto& part : result.parts) Delete(part.part.node); Delete(frame); Delete(result.node); Delete(result.theme); return Button{}; };
        if (!result.theme || !result.node || !frame || !Read<void*>(frame, 4) || !Data(result.node) || !Read<void*>(Data(result.node), 4) || Kind(result.node) != 2)
            return fail();
        const int16_t grow = int16_t(width - style.width);
        for (size_t i = 0; i < style.parts.size(); ++i)
        {
            const auto& source = style.parts[i];
            auto node = createNode(factory, nullptr, source.kind);
            auto partFrame = node ? createFrame(source.kind) : nullptr;
            if (!node || !partFrame || !Data(node) || !Read<void*>(Data(node), 4) || !Read<void*>(partFrame, 4) || Kind(node) != source.kind ||
                Read<void*>(Read<void*>(partFrame, 4), 0) != Read<void*>(Read<void*>(Data(node), 4), 0))
            { Delete(partFrame); Delete(node); return fail(); }
            RowPart part{ { node, Read<void*>(partFrame, 4) }, source.size, source.looks };
            for (auto& look : part.looks)
            {
                // Magma rectangles: left, right, top, bottom
                auto rect = reinterpret_cast<int16_t*>(look.data() + 8);
                if (source.kind == 5 && rect[0] == 0 && rect[1] == style.width) rect[1] = int16_t(width);
                else if (i == style.value) { rect[0] += grow; rect[1] += grow; }
                else if (i == style.label) rect[1] += grow;
            }
            const auto sourceData = Data(source.node), data = Data(node);
            if (source.kind == 5)
            {
                Write(data, 8, Read<void*>(sourceData, 8));
                Write(data, 12, Read<uint8_t>(sourceData, 12));
                Write(data, 16, Read<uint8_t>(sourceData, 16));
            }
            else
            {
                Write(data, 8, Read<void*>(sourceData, 8));
                Write(data, 40, uint32_t((Read<uint32_t>(data, 40) & ~7) | (Read<uint32_t>(sourceData, 40) & 7)));
                Write(data, 44, Read<uint8_t>(sourceData, 44));
            }
            Write(node, 20, Read<uint32_t>(source.node, 20));
            RowLook(part, 0);
            addFrame(node, nullptr, partFrame);
            addNode(result.theme, nullptr, node);
            result.parts.push_back(part);
        }
        result.label = result.parts[style.label].part;
        result.value = result.parts[style.value].part;
        result.labelRight = Read<int16_t>(result.label.frame, 10);
        result.valueRight = Read<int16_t>(result.value.frame, 10);
        Write(result.node, 4, id);
        Write(Data(result.node), 16, result.theme);
        auto point = Read<void*>(frame, 4);
        Write(point, 8, int16_t(x)); Write(point, 10, int16_t(y));
        addFrame(result.node, nullptr, frame);
        areaTime(result.theme, nullptr, 0, 1);
        return result;
    }
    // A row's look: 0 normal, 1 focused, 2 unavailable; bare shows the text only (help lines)
    inline void RowState(Button& row, int look, bool bare = false)
    {
        for (auto& part : row.parts)
        {
            RowLook(part, look);
            if (Kind(part.part.node) == 5) Visible(part.part.node, !bare);
        }
    }
    inline void* FirstFramePoint(void* node)
    {
        auto frames = Read<void*>(node, 12);
        auto first = frames ? Read<void**>(frames, 4) : nullptr;
        auto last = frames ? Read<void**>(frames, 8) : nullptr;
        return first && last > first && last - first <= 16 ? Read<void*>(*first, 4) : nullptr;
    }
    inline void CopyRenderMode(void* node, void* source)
    {
        Write(node, 20, (Read<uint32_t>(node, 20) & ~0xF000u) | (Read<uint32_t>(source, 20) & 0xF000u));
    }
    inline TextNode Rectangle(void* source)
    {
        if (!source || Kind(source) != 5) return {};
        auto factory = Factory();
        auto node = factory ? createNode(factory, nullptr, 5) : nullptr;
        auto frame = node ? createFrame(5) : nullptr;
        if (!node || !frame) { Delete(frame); Delete(node); return {}; }
        auto data = Data(node);
        auto point = Read<void*>(frame, 4);
        auto current = data ? Read<void*>(data, 4) : nullptr;
        auto originalData = Data(source);
        // The shown state: the first keyframe of the page's opening animation is the frame still collapsed at the
        // centre of the screen
        auto original = Read<void*>(originalData, 4);
        if (!original) original = FirstFramePoint(source);
        auto image = Read<void*>(originalData, 8);
        if (!image || !data || !point || !current || !original || Kind(node) != 5 ||
            Read<void*>(point, 0) != Read<void*>(original, 0) || Read<void*>(current, 0) != Read<void*>(original, 0))
        { Delete(frame); Delete(node); return {}; }
        // This native parameter is 52 bytes of color, rectangle, transforms and
        // corner colors. Keep the factory's vtable and its independent ownership.
        for (size_t offset = 4; offset < 52; offset += 4)
        {
            const auto value = Read<uint32_t>(original, offset);
            Write(point, offset, value); Write(current, offset, value);
        }
        // Native ImageUnreal destruction releases only its parameter; this
        // package-owned image handle is borrowed just like the native font.
        Write(data, 8, image);
        Write(data, 12, Read<uint8_t>(originalData, 12));
        Write(data, 16, Read<uint8_t>(originalData, 16));
        CopyRenderMode(node, source);
        // The image's draw flags (blend, tint) as the source has them
        Write(node, 20, Read<uint32_t>(source, 20));
        addFrame(node, nullptr, frame);
        return { node, point };
    }
    struct Decorated
    {
        Button button;
        TextNode background, mask;
        uint32_t maskColor = 0xFF000000u;
        std::vector<std::pair<TextNode, uint32_t>> borders;
        uint32_t textColor = 0xFFA7D2A7, selectionColor = 0xFFA7D2A7;
        void Focus(bool focused)
        {
            if (focused)
            {
                Color(button.label, background.node ? 0xFF000000u : 0xFFFFFFFFu);
                if (mask.node) Color(mask, 0u);
                if (background.node) Color(background, selectionColor);
                for (auto& [border, color] : borders) Color(border, color & 0xFF000000u);
            }
            else
            {
                Color(button.label, textColor);
                if (mask.node) Color(mask, maskColor);
                if (background.node) Color(background, 0u);
                for (auto& [border, color] : borders) Color(border, color);
            }
        }
    };
    struct Item
    {
        RWGTAMenu::Row row;
        std::string label, value, help;
        int link = 0;
        bool enabled = true;
    };
    struct Host : std::enable_shared_from_this<Host>
    {
        void* manager = nullptr;
        void* parent = nullptr;
        void* package = nullptr;
        void* page = nullptr;
        // The Plugins entry on Options, and Back in the bottom button row of the plugin pages: buttons dressed like
        // the page's own (frame, fill and the highlight when focused)
        Decorated entry, back;
        uint32_t selectionColor = 0xFFA7D2A7, normalTextColor = 0xFFDADADA;
        TextNode title, hint, joinedHint;
        // Scroll marks beside the first and the last row while more rows are above or below
        std::array<TextNode, 2> marks;
        bool markImages = false;
        bool details = false, notice = false;
        size_t detailsFirst = 0;
        std::vector<std::wstring> detailsLines;
        RWGTAMenu::Model joinedModel;
        std::vector<Button> joined;
        size_t joinedSelected = SIZE_MAX;
        bool joinedRefresh = false, joinedRestart = false;
        ULONGLONG joinedUpdated = 0;
        std::vector<Button> rows;
        std::vector<Item> items;
        void* savedFocus = nullptr;
        void* preceding = nullptr;
        void* following = nullptr;
        uint32_t previousNext = 0, followingPrevious = 0;
        int direction = 1;
        size_t pageIndex = RWGTAMenu::Model::RootPage, selected = 0;
        uint64_t generation = 0;
        bool pause = false, open = false, valid = true, refresh = false, backFocused = false;
    };
    inline std::unordered_map<void*, std::shared_ptr<Host>> hosts;
    struct PendingPage { void* manager; void* page; void* owner; unsigned attempts = 0; };
    inline std::vector<PendingPage> pendingPages;
    inline PendingPage* preparingPage = nullptr;
    inline Host* active = nullptr;
    inline void* menusManager = nullptr;
    inline constexpr size_t PageRows = 8;
    inline MenuList::WheelNotches wheel;
    inline std::atomic<int> wheelDelta{ 0 };
    struct Binding { std::weak_ptr<Host> host; size_t row; bool entry; bool joined = false; };
    inline constexpr size_t BackButton = SIZE_MAX;
    inline const std::set<std::string> ControlSettings{ "MAIN.RawInputMouse", "MAIN.MouseSensitivity", "MAIN.RawInputMouseRawData" };
    inline bool Live(const Host& host)
    {
        auto at = hosts.find(host.parent);
        return host.valid && at != hosts.end() && at->second.get() == &host;
    }
    struct Snapshot
    {
        Host& host;
        uint64_t generation;
        Host* owner;
        bool open;
        explicit Snapshot(Host& value) : host(value), generation(value.generation), owner(active), open(value.open) {}
        bool Current() const { return Live(host) && host.generation == generation && active == owner && host.open == open; }
        void Advance() { generation = host.generation; owner = active; open = host.open; }
    };
    enum class Action { Open, Activate, Cancel, JoinedChange, Defaults, Details, Move, Page, Edge, Scroll };
    struct Pending { std::weak_ptr<Host> host; Action action; size_t row; int direction; uint64_t generation; };
    inline std::vector<Pending> pending;
    inline bool Queue(const std::shared_ptr<Host>& host, Action action, size_t row = 0, int direction = 0)
    {
        if (!host || !Live(*host) || pending.size() >= 32) return false;
        pending.push_back({host, action, row, direction, host->generation}); return true;
    }
    inline std::unordered_map<void*, Binding> bindings;
    inline std::array<void*, 14> handlerTable{};
    inline void Populate(Host& host);
    inline void First(Host& host);
    inline void Open(Host& host);
    inline void Close(Host& host);
    inline void Activate(Host& host, size_t selected, int direction);
    inline bool Interactive(Host& host)
    { return Live(host) && host.open && active == &host && topMenu(host.manager, nullptr) == host.page && !Modal(host.manager); }
    inline bool JoinedInteractive(Host& host)
    { return Live(host) && !host.open && !host.joined.empty() && topMenu(host.manager, nullptr) == host.parent && !Modal(host.manager); }
    inline uint8_t __fastcall Defaults(void* controller, void*)
    {
        auto manager = Read<void*>(controller, 24);
        auto parent = topMenu(manager, nullptr);
        auto at = hosts.find(parent);
        auto host = at != hosts.end() ? at->second : std::shared_ptr<Host>{};
        auto generation = host ? host->generation : 0;
        auto result = defaultsHook.thiscall<uint8_t>(controller);
        if (result && host && Live(*host) && host->generation == generation && JoinedInteractive(*host))
            Queue(host, Action::Defaults);
        return result;
    }
    // Native focus/blur each pop one node argument (RET 4). Never invoke
    // plugin callbacks or remove engine objects while native input is unwinding.
    inline uint8_t __fastcall Focused(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at != bindings.end() && at->second.entry)
        {
            if (auto host = at->second.host.lock(); host && Live(*host))
                host->entry.Focus(true);
        }
        else if (at != bindings.end() && at->second.row == BackButton)
        {
            if (auto host = at->second.host.lock(); host && Live(*host)) { host->back.Focus(true); host->backFocused = true; host->refresh = true; }
        }
        else if (at != bindings.end())
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                if (at->second.joined) { host->joinedSelected = at->second.row; host->joinedRefresh = true; }
                else
                {
                    // The mouse over a row (or the focus the list set): the window's selection follows
                    host->selected = at->second.row;
                    if (!host->details && host->open) model.Window(host->pageIndex).Point(at->second.row);
                    host->refresh = true;
                }
            }
        return 0;
    }
    inline uint8_t __fastcall Blurred(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at != bindings.end() && at->second.entry)
        {
            if (auto host = at->second.host.lock(); host && Live(*host))
                host->entry.Focus(false);
        }
        else if (at != bindings.end() && at->second.row == BackButton)
        {
            if (auto host = at->second.host.lock(); host && Live(*host)) { host->back.Focus(false); host->backFocused = false; host->refresh = true; }
        }
        else if (at != bindings.end())
            if (auto host = at->second.host.lock(); host && Live(*host))
            {
                if (at->second.joined) { host->joinedSelected = SIZE_MAX; host->joinedRefresh = true; }
                else host->refresh = true;
            }
        return 0;
    }
    inline uint8_t __fastcall Accept(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || busy) return 0;
        auto binding = at->second;
        auto host = binding.host.lock();
        if (!host || !Live(*host) || Modal(host->manager)) return 0;
        if (binding.entry) return Queue(host, Action::Open);
        if (binding.joined) return JoinedInteractive(*host) && Queue(host, Action::JoinedChange, binding.row);
        if (binding.row == BackButton) return Interactive(*host) && Queue(host, Action::Cancel);
        return Interactive(*host) && Queue(host, Action::Activate, binding.row);
    }
    inline uint8_t __fastcall Cancel(void* handler, void*, void*)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || at->second.entry || at->second.joined || busy) return 0;
        auto host = at->second.host.lock();
        return host && Interactive(*host) && Queue(host, Action::Cancel);
    }
    inline uint8_t __fastcall Key(void* handler, void*, void*, void* key)
    {
        auto at = bindings.find(handler);
        if (at == bindings.end() || at->second.entry || busy || !key) return 0;
        auto host = at->second.host.lock();
        if (!host || (at->second.joined ? !JoinedInteractive(*host) : !Interactive(*host))) return 0;
        auto code = Read<uint32_t>(key, 0);
        if (code == VK_F1 && !at->second.joined) return Queue(host, Action::Details);
        if (!at->second.joined)
        {
            switch (code)
            {
                case VK_UP: return Queue(host, Action::Move, 0, -1);
                case VK_DOWN: return Queue(host, Action::Move, 0, 1);
                case VK_PRIOR: return Queue(host, Action::Page, 0, -1);
                case VK_NEXT: return Queue(host, Action::Page, 0, 1);
                case VK_HOME: return Queue(host, Action::Edge, 0, -1);
                case VK_END: return Queue(host, Action::Edge, 0, 1);
                default: break;
            }
        }
        if (code != 37 && code != 39) return 0;
        return Queue(host, at->second.joined ? Action::JoinedChange : Action::Activate, at->second.row, code == 37 ? -1 : 1);
    }
    inline bool WindowsHelp(bool activate)
    {
        if (busy || !active) return false;
        auto at = hosts.find(active->parent);
        if (at == hosts.end() || at->second.get() != active || !Interactive(*at->second)) return false;
        if (activate) Queue(at->second, Action::Details);
        return true;
    }
    inline bool WindowsWheel(int delta)
    {
        if (busy || !active) return false;
        auto at = hosts.find(active->parent);
        if (at == hosts.end() || at->second.get() != active || !Interactive(*at->second)) return false;
        wheelDelta += delta;
        return true;
    }
    inline bool Bind(Host& host, Button& button, size_t row, bool entry, bool joined = false)
    {
        auto handler = createHandler(1);
        if (!handler) return false;
        Write(handler, 0, handlerTable.data());
        bindings.emplace(handler, Binding{host.shared_from_this(), row, entry, joined});
        Write(handler, 4, uint32_t(1));
        addHandler(button.node, nullptr, handler);
        return true;
    }
    inline size_t Slots(const Host& host) { return host.rows.size(); }
    inline void Hint(Host& host)
    {
        auto retained = host.shared_from_this();
        auto hint = !host.details && host.selected < host.items.size() ? host.items[host.selected].help : "";
        if (model.RestartRequired()) hint = model.Text("menu.restartwarning");
        // All of the help when it fits the line; else its first sentences that do (F1 shows all of it); else the key
        const auto metrics = TextMetrics(host.hint.node);
        if (!metrics.font) return;
        const auto full = Styled(hint, metrics);
        auto text = full;
        for (size_t cut = full.size(); metrics.Measure(text) > metrics.width && cut > 0;)
        {
            cut = full.find_last_of(L".!?", cut - 1);
            if (cut == std::wstring::npos) { text = Styled("F1: " + model.Text("menu.details"), metrics); break; }
            if (cut + 1 < full.size() && !iswspace(full[cut + 1])) continue;
            text = full.substr(0, cut + 1);
        }
        CaptionWide(host.hint.node, text);
    }
    inline void Columns(Button& button, bool value)
    {
        const auto right = Read<int16_t>(button.value.frame, 10);
        const auto left = Read<int16_t>(button.label.frame, 8);
        // A styled row keeps the page's own columns; without a value the label reaches the row's end
        const auto edge = button.labelRight ? (value ? button.labelRight : button.valueRight) :
            value ? left + (right - left) * 62 / 100 : right;
        for (auto point : { button.label.frame, Read<void*>(Data(button.label.node), 4) })
            Write(point, 10, int16_t(edge));
        Visible(button.value.node, value);
    }
    // A styled row's value takes room from the label when it is longer than the page's value column
    inline void ValueColumn(Button& row, const std::string& value)
    {
        if (row.parts.empty() || value.empty()) return;
        const auto metrics = TextMetrics(row.value.node);
        if (!metrics.font) return;
        const auto measured = metrics.Measure(Styled(value, metrics));
        const auto left = Read<int16_t>(row.label.frame, 8);
        const auto current = Read<int16_t>(row.value.frame, 8);
        if (!std::isfinite(measured) || row.valueRight - current - 4.0 >= measured) return;
        const auto edge = int16_t(std::max<double>(left + 40, row.valueRight - measured - 8));
        for (auto point : { row.value.frame, Read<void*>(Data(row.value.node), 4) }) Write(point, 8, edge);
        for (auto point : { row.label.frame, Read<void*>(Data(row.label.node), 4) })
            Write(point, 10, std::min<int16_t>(Read<int16_t>(point, 10), int16_t(edge - 8)));
    }
    // The selected row's whole help in the list, Back under it; the lines scroll when there are more than fit
    inline void BeginDetails(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!Interactive(host) || host.details || host.selected >= host.items.size()) return;
        const auto item = host.items[host.selected];
        if (item.link || item.help.empty()) return;
        auto metrics = TextMetrics(host.rows.front().label.node);
        metrics.width = Read<int16_t>(host.rows.front().value.frame, 10) - Read<int16_t>(host.rows.front().label.frame, 8) - 4.0;
        if (!metrics.font || !std::isfinite(metrics.width) || metrics.width <= 0) return;
        auto title = item.label + (item.value.empty() ? "" : ": " + item.value);
        auto text = Styled(title, metrics) + L"\n" + Styled(item.help, metrics);
        auto lines = DAText::Wrap(text, metrics.width, [&](const auto& part)
        {
            const auto width = metrics.Measure(part);
            return std::isfinite(width) && (part.empty() || width > 0) ? width : INFINITY;
        });
        if (lines.empty() || !snapshot.Current()) return;
        host.details = true; host.detailsFirst = 0;
        host.detailsLines = std::move(lines);
        ++host.generation; snapshot.Advance();
        Populate(host); if (snapshot.Current()) First(host);
    }
    inline void EndDetails(Host& host)
    {
        auto retained = host.shared_from_this();
        if (!host.details || !Interactive(host)) return;
        if (host.notice)
        {
            host.notice = false;
            Close(host);
            return;
        }
        host.details = false; host.detailsLines.clear();
        ++host.generation; Snapshot snapshot(host);
        Populate(host);
        if (snapshot.Current()) First(host);
    }
    inline void Marks(Host& host)
    {
        bool above = false, below = false;
        if (host.details)
        {
            const auto shown = Slots(host) - 1;
            above = host.detailsFirst > 0;
            below = host.detailsFirst + shown < host.detailsLines.size();
        }
        else
        {
            const auto& window = model.Window(host.pageIndex);
            above = window.Above(); below = window.Below();
        }
        for (size_t i = 0; i < host.marks.size(); ++i)
        {
            auto& mark = host.marks[i];
            if (!mark.node) continue;
            if (host.markImages) { Visible(mark.node, i ? below : above); continue; }
            const auto metrics = TextMetrics(mark.node);
            // The font's own triangles, else its plainest arrows
            const wchar_t* glyphs = i ? L"\u25BC" : L"\u25B2";
            if (!metrics.font || !metrics.available || !metrics.available(glyphs[0])) glyphs = i ? L"v" : L"^";
            CaptionWide(mark.node, (i ? below : above) ? std::wstring(glyphs) : L"");
        }
    }
    inline void Populate(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!snapshot.Current() || !host.open || !model.IsOpen() || host.rows.empty()) return;
        if (host.pageIndex >= model.Pages().size()) host.pageIndex = RWGTAMenu::Model::RootPage;
        std::vector<Item> items;
        Caption(host.title.node, model.Pages()[host.pageIndex].title);
        size_t focus = 0;
        if (host.details)
        {
            // The lines in the rows above Back, scrolled by Up/Down
            const auto shown = Slots(host) - 1;
            const auto last = host.detailsLines.size() > shown ? host.detailsLines.size() - shown : 0;
            host.detailsFirst = std::min(host.detailsFirst, last);
            Caption(host.title.node, host.notice ? RWGTAMenu::PluginText(FixSettings(), Language(), "update.title") : model.Text("menu.details"));
            for (size_t i = 0; i < shown; ++i) items.push_back({ {}, "", "", "", -5, false });
            focus = shown;
        }
        else
        {
            for (auto row : model.Shown(host.pageIndex, false))
            {
                if (!row) { items.push_back({ {}, model.Text("menu.back"), "", "", -1 }); continue; }
                if (RWGTAMenu::Model::Opens(*row))
                {
                    auto label = model.Label(*row);
                    if (!snapshot.Current()) return;
                    items.push_back({ *row, std::move(label), "", "", 1 });
                    continue;
                }
                auto enabled = model.Enabled(*row, host.pause);
                if (!snapshot.Current()) return;
                auto label = model.Label(*row);
                if (!snapshot.Current()) return;
                auto value = enabled ? model.Value(*row) : model.Text("menu.unavailable");
                if (!snapshot.Current()) return;
                auto help = model.Help(*row);
                if (!snapshot.Current()) return;
                items.push_back({ *row, std::move(label), std::move(value), std::move(help), 0, enabled });
            }
            focus = model.Window(host.pageIndex).Slot();
        }
        if (!snapshot.Current()) return;
        if (host.details) items.push_back({ {}, model.Text("menu.back"), "", "", -1 });
        host.items = std::move(items);
        size_t count = host.items.size();
        host.selected = std::min(focus, count - 1);
        for (size_t i = 0; i < host.rows.size(); ++i)
        {
            auto& button = host.rows[i];
            Visible(button.node, i < count);
            bool enabled = i < count && host.items[i].enabled;
            Enabled(button.node, enabled);
            const bool line = host.details && i + 1 < count;
            const bool focused = enabled && i == host.selected && !host.backFocused;
            if (!button.parts.empty()) RowState(button, line ? 0 : focused ? 1 : enabled ? 0 : 2, line);
            Columns(button, i < count && !host.items[i].value.empty());
            if (i < count && !line) ValueColumn(button, host.items[i].value);
            if (host.details && i + 1 < count)
            {
                const auto line = host.detailsFirst + i;
                CaptionWide(button.label.node, line < host.detailsLines.size() ? host.detailsLines[line] : L"");
            }
            else Caption(button.label.node, i < count ? host.items[i].label : "");
            Caption(button.value.node, i < count ? host.items[i].value : "");
            if (button.parts.empty())
            {
                auto color = line ? host.normalTextColor : enabled ? (focused ? host.selectionColor : host.normalTextColor) : 0xFF777777u;
                Color(button.label, color); Color(button.value, color);
            }
            // The list moves itself (Move): the rows have no neighbours of their own, so the native focus never
            // jumps past the shown rows
            Write(button.node, 28, Read<uint32_t>(button.node, 4));
            Write(button.node, 32, Read<uint32_t>(button.node, 4));
            Write(button.node, 36, Read<uint32_t>(button.node, 4));
            Write(button.node, 40, Read<uint32_t>(button.node, 4));
        }
        linkNeighbors(host.page);
        Marks(host);
        Hint(host);
    }
    // The native focus on the selected row
    inline void First(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (host.selected >= host.rows.size()) return;
        auto node = host.rows[host.selected].node;
        if (Read<void*>(host.page, 36) == node) return;
        if (auto previous = Read<void*>(host.page, 36)) blurNode(previous, nullptr);
        if (!snapshot.Current()) return;
        Write(host.page, 36, node); selectNode(node, nullptr);
    }
    // Up/Down (Move, wrapping like the game's lists), Page Up/Down and the shoulders (Page), Home/End (Edge), the
    // wheel (Scroll); the help page scrolls its lines
    inline void Navigate(Host& host, Action action, int direction)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (host.details)
        {
            const auto before = host.detailsFirst;
            const auto step = action == Action::Page ? Slots(host) - 2 : action == Action::Edge ? host.detailsLines.size() : 1;
            host.detailsFirst = direction < 0 ? (host.detailsFirst > step ? host.detailsFirst - step : 0) : host.detailsFirst + step;
            Populate(host);
            if (before != host.detailsFirst && snapshot.Current()) First(host);
            return;
        }
        auto& window = model.Window(host.pageIndex);
        const bool changed = action == Action::Move ? window.Move(direction) : action == Action::Page ? window.Page(direction) :
            action == Action::Edge ? window.Edge(direction) : window.Scroll(direction);
        if (!changed) return;
        Populate(host);
        if (snapshot.Current()) First(host);
    }
    inline void Back(Host& host)
    {
        auto retained = host.shared_from_this();
        if (host.details) { EndDetails(host); return; }
        const auto parent = model.Back(host.pageIndex);
        if (parent == SIZE_MAX) { Close(host); return; }
        ++host.generation; Snapshot snapshot(host);
        host.pageIndex = parent;
        Populate(host); if (snapshot.Current()) First(host);
    }
    inline void Open(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (topMenu(host.manager, nullptr) != host.parent || Modal(host.manager) || !Pause(host.manager, host.pause)) return;
        if (active) Close(*active);
        if (active || !Live(host) || host.generation != snapshot.generation || host.open != snapshot.open) return;
        snapshot.Advance();
        host.savedFocus = Read<void*>(host.parent, 36);
        model.OpenMenu(hostModule, Language(), Slots(host), host.joined.empty() ? std::set<std::string>{} : ControlSettings);
        if (!snapshot.Current()) return;
        host.pageIndex = RWGTAMenu::Model::RootPage; host.selected = 0;
        model.Window(host.pageIndex).Reset();
        wheel.Clear(); wheelDelta = 0;
        ++host.generation;
        host.details = false; host.detailsLines.clear();
        host.open = true; active = &host; snapshot.Advance();
        Populate(host);
        if (!snapshot.Current()) return;
        displayHook.thiscall<uint8_t>(host.manager, host.page, true, true, int(0));
        if (!snapshot.Current()) return;
        First(host);
        if (!snapshot.Current()) return;
    }
    inline void Close(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (!snapshot.Current()) return;
        ++host.generation;
        host.open = false; host.details = false; host.notice = false; host.detailsLines.clear(); snapshot.Advance();
        if (topMenu(host.manager, nullptr) == host.page)
            displayHook.thiscall<uint8_t>(host.manager, host.page, false, true, int(0));
        if (!snapshot.Current()) return;
        if (active == &host) { active = nullptr; model.Close(); snapshot.Advance(); }
        if (!snapshot.Current()) return;
        if (host.savedFocus && topMenu(host.manager, nullptr) == host.parent)
        {
            // The mouse may already rest on another button of the page: one focus, as the page keeps it
            if (auto current = Read<void*>(host.parent, 36); current && current != host.savedFocus) blurNode(current, nullptr);
            Write(host.parent, 36, host.savedFocus); selectNode(host.savedFocus, nullptr);
        }
        if (snapshot.Current()) host.savedFocus = nullptr;
    }
    inline std::shared_ptr<Host> NoticeHost()
    {
        if (active)
            return {};
        for (const auto& [parent, host] : hosts)
            if (Live(*host) && !host->open && host->page && !host->rows.empty() &&
                topMenu(host->manager, nullptr) == parent && !Modal(host->manager))
                return host;
        return {};
    }
    inline bool NoticeReady()
    {
        return bool(NoticeHost());
    }
    inline bool ShowNotice(const std::wstring& text)
    {
        const auto host = NoticeHost();
        if (!host)
            return false;
        Snapshot snapshot(*host);
        auto metrics = TextMetrics(host->rows.front().label.node);
        metrics.width = Read<int16_t>(host->rows.front().value.frame, 10) - Read<int16_t>(host->rows.front().label.frame, 8) - 4.0;
        if (!metrics.font || !std::isfinite(metrics.width) || metrics.width <= 0)
            return false;
        const auto styled = DAText::Style(text, metrics, Wide(Language()).c_str());
        auto lines = DAText::Wrap(styled, metrics.width, [&](const auto& part)
        {
            const auto width = metrics.Measure(part);
            return std::isfinite(width) && (part.empty() || width > 0) ? width : INFINITY;
        });
        if (lines.empty() || !snapshot.Current())
            return false;
        Open(*host);
        if (!Interactive(*host))
            return false;
        snapshot.Advance();
        host->details = host->notice = true; host->detailsFirst = 0;
        host->detailsLines = std::move(lines);
        ++host->generation; snapshot.Advance();
        Populate(*host);
        if (!snapshot.Current())
            return false;
        First(*host);
        return snapshot.Current() && host->notice && Interactive(*host);
    }
    // A row: left (-1), right (+1) or accept (0); a plugin or a category opens, Back goes up a level
    inline void Activate(Host& host, size_t selected, int direction)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        if (selected >= host.items.size()) return;
        if (host.details)
        {
            if (!direction && host.items[selected].link == -1) EndDetails(host);
            return;
        }
        auto item = host.items[selected];
        model.Window(host.pageIndex).Point(selected);
        if (item.link == -1)
        {
            if (!direction) Back(host);
            return;
        }
        if (item.link == 1)
        {
            if (direction) return;
            const auto page = model.Enter(item.row);
            if (page == SIZE_MAX) return;
            host.pageIndex = page;
            ++host.generation; snapshot.Advance();
        }
        else
        {
            bool changed = model.Change(item.row, direction, host.pause);
            if (!snapshot.Current()) return;
            if (changed) { ++host.generation; snapshot.Advance(); }
        }
        Populate(host); if (snapshot.Current()) First(host);
    }
    inline void* OwningPackage(void* manager, void* parent)
    {
        // PC interactions load a second P_Options in pc_only. Its actual page
        // owns B_Video/B_Control/B_Sound; the options package has a different page.
        // Require exact page identity, including native decorated package names.
        for (auto name : { "pc_only_pc", "pc_only", "options_pc", "options" })
            if (auto package = menuPackage(manager, nullptr, name); package && findPage(package, nullptr, Id("P_Options")) == parent) return package;
        return nullptr;
    }
    inline bool Fits(Rect rect, int width, int height)
    {
        return rect.left >= 0 && rect.top >= 0 && rect.right <= width && rect.bottom <= height &&
            rect.right - rect.left >= 24 && rect.bottom - rect.top >= 10;
    }
    inline bool Empty(void* parent, Rect rect, bool text = false)
    {
        std::vector<void*> nodes;
        if (!Nodes(parent, nodes)) return false;
        for (auto node : nodes)
        {
            auto kind = Kind(node);
            if (kind != 2 && kind != 3 && kind != 4 && kind != 9 && kind != 10 && kind != 12 && !(text && kind == 8)) continue;
            auto occupied = Bounds(node);
            if (rect.left < occupied.right && occupied.left < rect.right && rect.top < occupied.bottom && occupied.top < rect.bottom) return false;
        }
        return true;
    }
    inline void RefreshJoined(Host& host)
    {
        auto retained = host.shared_from_this();
        Snapshot snapshot(host);
        auto& rows = host.joinedModel.JoinedRows();
        if (!JoinedInteractive(host) || rows.size() != host.joined.size()) return;
        for (size_t i = 0; i < rows.size(); ++i)
        {
            auto label = host.joinedModel.Label(rows[i]);
            if (!snapshot.Current()) return;
            auto enabled = host.joinedModel.Enabled(rows[i], host.pause);
            if (!snapshot.Current()) return;
            auto value = enabled ? host.joinedModel.Value(rows[i]) : host.joinedModel.Text("menu.unavailable");
            if (!snapshot.Current()) return;
            auto& button = host.joined[i];
            Enabled(button.node, enabled);
            if (!button.parts.empty()) { RowState(button, enabled ? (i == host.joinedSelected ? 1 : 0) : 2); Columns(button, true); ValueColumn(button, value); }
            Caption(button.label.node, label); Caption(button.value.node, value);
            if (button.parts.empty())
            {
                auto color = enabled ? (i == host.joinedSelected ? host.selectionColor : host.normalTextColor) : 0xFF777777u;
                Color(button.label, color); Color(button.value, color);
            }
        }
        auto help = host.joinedRestart || host.joinedModel.RestartRequired() ? host.joinedModel.Text("menu.restartwarning") :
            host.joinedSelected < rows.size() ? host.joinedModel.Help(rows[host.joinedSelected]) : "";
        if (!snapshot.Current()) return;
        Caption(host.joinedHint.node, help);
        host.joinedRefresh = false;
        host.joinedUpdated = GetTickCount64();
    }
    inline bool JoinControls(Host& host, void* font, int width, int height)
    {
        if (!defaultsHook || !host.joined.empty()) return !host.joined.empty();
        auto slider = findNode(host.parent, nullptr, Id("MouseSensitivity "));
        auto label = findNode(host.parent, nullptr, Id("L_MouseSensitivity "));
        auto footer = findNode(host.parent, nullptr, Id("B_Default"));
        auto title = findNode(host.parent, nullptr, Id("L_Title"));
        auto video = findNode(host.parent, nullptr, Id("B_Video"));
        if (!slider || !label || !footer || !title || !video || Kind(slider) != 12 || Kind(label) != 8 || Kind(footer) != 2) return false;
        auto sensor = Bounds(slider), caption = Bounds(label), bottom = Bounds(footer), heading = Bounds(title), first = Bounds(video);
        if (!Fits(sensor, width, height) || !Fits(caption, width, height) || !Fits(bottom, width, height) || !Fits(heading, width, height)) return false;
        int top = std::max(sensor.bottom, caption.bottom) + 6, gap = 4;
        int available = bottom.top - top - 6;
        int rowHeight = std::min(24, (available - gap * int(ControlSettings.size() - 1)) / int(ControlSettings.size()));
        int x = caption.left, rowWidth = width - x * 2, fontHeight = std::min(20, rowHeight - 4);
        // In the dress of the page's own setting rows when they fit inside the panel, centred as those are
        const auto style = FindRowStyle(host.parent);
        if (style)
        {
            int panelBottom = 0;
            std::vector<void*> nodes;
            if (Nodes(host.parent, nodes))
                for (auto node : nodes)
                    if (Kind(node) == 5) { auto r = Bounds(node); if (r.right - r.left > width / 2) panelBottom = std::max<int>(panelBottom, r.bottom); }
            const int count = int(ControlSettings.size());
            const int limit = std::min<int>(panelBottom ? panelBottom - 4 : bottom.top - 6, bottom.top - 6);
            const int pitch = std::min(style->pitch, (limit - top - style->height) / (count - 1));
            if (pitch >= style->height + 2)
            {
                rowHeight = style->height; gap = pitch - style->height;
                rowWidth = style->width; x = (width - rowWidth) / 2;
            }
        }
        const bool styled = style && rowWidth == style->width;
        Rect hint{ int16_t(x), int16_t(heading.bottom + 4), int16_t(width - x), int16_t(first.top - 8) };
        if (rowHeight < 18 || fontHeight < 14 || rowWidth < 300 || hint.bottom - hint.top < 18 || !Fits(hint, width, height) || !Empty(host.parent, hint, true)) return false;
        std::vector<Rect> places;
        for (size_t i = 0; i < ControlSettings.size(); ++i)
        {
            int y = top + int(i) * (rowHeight + gap);
            Rect rect{ int16_t(x), int16_t(y), int16_t(x + rowWidth), int16_t(y + rowHeight) };
            if (!Fits(rect, width, height) || !Empty(host.parent, rect, true)) return false;
            places.push_back(rect);
        }
        host.joinedModel.OpenPlugins(hostModule, Language(), 4096, ControlSettings);
        if (!Live(host) || host.joinedModel.JoinedRows().size() != places.size()) { host.joinedModel.Close(); return false; }
        struct Cleanup
        {
            Host& host;
            std::vector<Button> buttons;
            TextNode hint;
            bool published = false;
            ~Cleanup()
            {
                if (published) return;
                for (auto& button : buttons) { Delete(button.node); Delete(button.theme); }
                Delete(hint.node);
                std::erase_if(bindings, [this](const auto& item) { return item.second.joined && item.second.host.lock().get() == &host; });
                host.joinedModel.Close();
            }
        } cleanup{ host };
        for (size_t i = 0; i < places.size(); ++i)
        {
            auto rect = places[i];
            const auto id = Id("WFP_Mouse_" + std::to_string(i));
            auto button = styled ? CreateRow(*style, id, rect.left, rect.top, rowWidth) :
                CreateButton(font, id, rect.left, rect.top, rowWidth, rowHeight, fontHeight, true);
            if (!button.node) return false;
            cleanup.buttons.push_back(button);
            if (!Bind(host, cleanup.buttons.back(), i, false, true)) return false;
        }
        cleanup.hint = Text(font, hint.left, hint.top, hint.right - hint.left, hint.bottom - hint.top, 14, 0);
        if (!cleanup.hint.node) return false;
        auto next = Read<uint32_t>(slider, 32);
        auto following = next ? findNode(host.parent, nullptr, next) : footer;
        auto followingId = following ? Read<uint32_t>(following, 4) : Read<uint32_t>(footer, 4);
        for (size_t i = 0; i < cleanup.buttons.size(); ++i)
        {
            auto& button = cleanup.buttons[i];
            Write(button.node, 28, Read<uint32_t>(i ? cleanup.buttons[i - 1].node : slider, 4));
            Write(button.node, 32, i + 1 < cleanup.buttons.size() ? Read<uint32_t>(cleanup.buttons[i + 1].node, 4) : followingId);
            Write(button.node, 36, Read<uint32_t>(button.node, 4)); Write(button.node, 40, Read<uint32_t>(button.node, 4));
            addPackageChild(host.package, nullptr, button.theme); addNode(host.parent, nullptr, button.node);
        }
        addNode(host.parent, nullptr, cleanup.hint.node);
        Write(slider, 32, Read<uint32_t>(cleanup.buttons.front().node, 4));
        if (following && Read<uint32_t>(following, 28) == Read<uint32_t>(slider, 4))
            Write(following, 28, Read<uint32_t>(cleanup.buttons.back().node, 4));
        host.joined = std::move(cleanup.buttons); host.joinedHint = cleanup.hint;
        cleanup.published = true;
        linkNeighbors(host.parent); areaTime(host.parent, nullptr, Read<unsigned int>(host.parent, 24), 0);
        RefreshJoined(host);
        return true;
    }
    inline int NativeHeight(void* node, int fallback)
    {
        if (!node || Kind(node) != 8) return fallback;
        auto point = Read<void*>(Data(node), 4);
        const auto height = point ? Read<uint16_t>(point, 22) : 0;
        return height >= 8 && height <= 64 ? height : fallback;
    }
    inline int CategoryHeight(void* button, int fallback)
    {
        auto body = Data(button);
        auto area = body ? Read<void*>(body, 16) : nullptr;
        std::vector<void*> nodes;
        if (!area || !Nodes(area, nodes)) return fallback;
        for (auto node : nodes) if (Kind(node) == 8) return NativeHeight(node, fallback);
        return fallback;
    }
    inline int BodyHeight(void* parent, int fallback)
    {
        std::vector<void*> nodes;
        if (!Nodes(parent, nodes)) return fallback;
        for (auto node : nodes)
        {
            if (Kind(node) != 2) continue;
            auto area = Read<void*>(Data(node), 16);
            std::vector<void*> children;
            if (!area || !Nodes(area, children)) continue;
            std::vector<void*> labels;
            for (auto child : children) if (Kind(child) == 8) labels.push_back(child);
            if (labels.size() >= 2) return NativeHeight(labels.front(), fallback);
        }
        return fallback;
    }
    struct PageLayout
    {
        Rect title, hint;
        int rowY = 0, rowWidth = 0, rowHeight = 0, fontHeight = 0, hintHeight = 0, rows = 0;
    };
    // pitch: the spacing of the page's own rows (0: plain text rows)
    inline std::optional<PageLayout> Layout(Rect panel, Rect title, int fontHeight, int pitch)
    {
        PageLayout result;
        result.title = title;
        result.fontHeight = fontHeight;
        result.hintHeight = std::max(12, fontHeight - 4);
        result.rowY = title.bottom + 10;
        result.rowWidth = title.right - title.left;
        const int hintBottom = panel.bottom - 8;
        const int hintTop = hintBottom - (result.hintHeight * 2 + 16);
        result.rowHeight = pitch ? pitch : std::min(fontHeight + 12, (hintTop - result.rowY - 8) / int(PageRows));
        result.rows = pitch ? (hintTop - 8 - result.rowY) / pitch : int(PageRows);
        result.hint = { title.left, int16_t(hintTop), title.right, int16_t(hintBottom) };
        if (fontHeight < 12 || result.rowHeight < fontHeight + 2 || result.rowWidth < 300 || result.rows < 4 ||
            title.left < panel.left || title.right > panel.right || title.top < panel.top ||
            title.bottom <= title.top || result.rowY + result.rows * result.rowHeight > hintTop) return {};
        return result;
    }
    inline bool Decorate(Decorated& target, void* source);
    inline bool Build(Host& host, void* font, int width, int height)
    {
        host.page = createArea(0);
        if (!host.page) return false;
        Write(host.page, 4, Id("P_WFP_Advanced_Options"));
        std::vector<void*> nodes;
        if (!Nodes(host.parent, nodes)) { Delete(host.page); host.page = nullptr; return false; }
        auto heading = findNode(host.parent, nullptr, Id("L_Title"));
        if (!heading || Kind(heading) != 8) { Delete(host.page); host.page = nullptr; return false; }
        const auto title = Bounds(heading);
        Rect panel{};
        int largest = 0;
        size_t rectangles = 0;
        // The page's frame and rules are drawn inside element groups, as the buttons' own frames are (images of
        // a page draw only when they are areas). The game batches images until text is drawn and gives a batch the
        // colour of its first image, so the images keep the page's runs: those before its title, those between the
        // title and its first button, and the rest, each run drawn in the same place between texts as on Options
        std::array<Button, 3> frames;
        for (size_t i = 0; i < frames.size(); ++i)
        {
            auto& frame = frames[i];
            frame = CreateButton(font, Id("WFP_Advanced_Frame_" + std::to_string(i)), 0, 0, width, height, 14, false, 1);
            if (!frame.node) { Delete(host.page); host.page = nullptr; return false; }
            Caption(frame.label.node, "");
            // The group passes the images' own colours on (its tint is white, as the native buttons' is)
            if (auto point = FirstFramePoint(frame.node)) Write(point, 4, 0xFFFFFFFFu);
            if (auto current = Read<void*>(Data(frame.node), 4)) Write(current, 4, 0xFFFFFFFFu);
            addPackageChild(host.package, nullptr, frame.theme);
        }
        size_t run = 0;
        for (auto node : nodes)
        {
            if (node == heading) run = std::max<size_t>(run, 1);
            const auto kind = Kind(node);
            if (run == 1 && (kind == 2 || kind == 12)) run = 2;
            if (kind != 5) continue;
            auto& frame = frames[run];
            auto bounds = Bounds(node);
            if (bounds.left < 0 || bounds.top < 0 || bounds.right > width || bounds.bottom > height ||
                bounds.right <= bounds.left || bounds.bottom <= bounds.top) continue;
            const int area = (bounds.right - bounds.left) * (bounds.bottom - bounds.top);
            if (area > largest) { largest = area; panel = bounds; }
            if (++rectangles > 32) { Delete(host.page); host.page = nullptr; return false; }
            auto clone = Rectangle(node);
            if (!clone.node) { Delete(host.page); host.page = nullptr; return false; }
            addNode(frame.theme, nullptr, clone.node);
        }
        for (auto& frame : frames) areaTime(frame.theme, nullptr, 0, 1);
        // The rows in the dress of the page's own setting rows, centred in the panel as those are
        const auto style = FindRowStyle(host.parent);
        auto plan = largest ? Layout(panel, title, BodyHeight(host.parent, std::clamp(height / 30, 14, 24)), style ? style->pitch : 0) : std::nullopt;
        if (!plan) { Delete(host.page); host.page = nullptr; return false; }
        const int fontHeight = plan->fontHeight, rowHeight = plan->rowHeight;
        const int rowWidth = style ? style->width : plan->rowWidth, y = plan->rowY;
        const int x = style ? (panel.left + panel.right - rowWidth) / 2 : title.left;
        host.title = Text(font, title.left, title.top, rowWidth, title.bottom - title.top, NativeHeight(heading, fontHeight + 4), 0);
        host.hint = Text(font, plan->hint.left, plan->hint.top, rowWidth, plan->hint.bottom - plan->hint.top, plan->hintHeight, 0);
        if (!host.title.node || !host.hint.node)
        { Delete(host.title.node); Delete(host.hint.node); Delete(host.page); host.page = nullptr; return false; }
        const auto titlePoint = Read<void*>(Data(heading), 4);
        host.selectionColor = Read<uint32_t>(titlePoint, 4) | 0xFF000000u;
        Color(host.title, host.selectionColor);
        if (auto bodyLabel = findNode(host.parent, nullptr, Id("L_MouseSensitivity ")); bodyLabel && Kind(bodyLabel) == 8)
            host.normalTextColor = Read<uint32_t>(Read<void*>(Data(bodyLabel), 4), 4) | 0xFF000000u;
        addNode(host.page, nullptr, frames[0].node); addNode(host.page, nullptr, host.title.node);
        addNode(host.page, nullptr, frames[1].node); addNode(host.page, nullptr, host.hint.node);
        for (size_t i = 0; i < size_t(plan->rows); ++i)
        {
            const auto id = Id("WFP_Advanced_Row_" + std::to_string(i));
            auto button = style ? CreateRow(*style, id, x, y + int(i) * rowHeight, rowWidth) :
                CreateButton(font, id, x, y + int(i) * rowHeight, rowWidth, rowHeight, fontHeight, true);
            if (!button.node || !Bind(host, button, i, false))
            {
                Delete(button.node); Delete(button.theme);
                Delete(host.page); host.page = nullptr;
                for (auto& row : host.rows) Delete(row.theme);
                std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == &host; }); return false;
            }
            addNode(host.page, nullptr, button.node); host.rows.push_back(button);
        }
        addNode(host.page, nullptr, frames[2].node);
        // The rows' own arrow, pointing up and down, beside the first and the last row
        size_t arrow = SIZE_MAX;
        if (style)
            for (size_t i = 0; i < style->parts.size(); ++i)
                if (style->parts[i].kind == 5 && *reinterpret_cast<const float*>(style->parts[i].looks[1].data() + 16) != 0.0f) arrow = i;
        if (arrow != SIZE_MAX)
        {
            auto group = CreateButton(font, Id("WFP_Advanced_Marks"), 0, 0, width, height, 14, false, 1);
            if (group.node)
            {
                Caption(group.label.node, "");
                if (auto point = FirstFramePoint(group.node)) Write(point, 4, 0xFFFFFFFFu);
                if (auto current = Read<void*>(Data(group.node), 4)) Write(current, 4, 0xFFFFFFFFu);
                const auto& source = style->parts[arrow];
                const auto color = *reinterpret_cast<const uint32_t*>(style->parts[style->label].looks[0].data() + 4);
                for (size_t i = 0; i < host.marks.size(); ++i)
                {
                    auto node = createNode(Factory(), nullptr, 5);
                    auto frame = node ? createFrame(5) : nullptr;
                    if (!node || !frame || !Data(node) || !Read<void*>(Data(node), 4) || !Read<void*>(frame, 4)) { Delete(frame); Delete(node); continue; }
                    auto look = source.looks[1];
                    auto rect = reinterpret_cast<int16_t*>(look.data() + 8);
                    const int16_t w = rect[1] - rect[0], h = rect[3] - rect[2];
                    const int cx = x + rowWidth + 6 + w / 2, cy = y + (i ? plan->rows - 1 : 0) * rowHeight + style->height / 2;
                    rect[0] = int16_t(cx - w / 2); rect[1] = int16_t(rect[0] + w); rect[2] = int16_t(cy - h / 2); rect[3] = int16_t(rect[2] + h);
                    // The arrow's turn in degrees
                    *reinterpret_cast<float*>(look.data() + 16) = i ? 180.0f : 0.0f;
                    for (size_t offset = 36; offset < 52; offset += 4) *reinterpret_cast<uint32_t*>(look.data() + offset) = color;
                    for (auto point : { Read<void*>(frame, 4), Read<void*>(Data(node), 4) }) std::memcpy(static_cast<uint8_t*>(point) + 4, look.data() + 4, 48);
                    Write(Data(node), 8, Read<void*>(Data(source.node), 8));
                    Write(Data(node), 12, Read<uint8_t>(Data(source.node), 12));
                    Write(Data(node), 16, Read<uint8_t>(Data(source.node), 16));
                    Write(node, 20, Read<uint32_t>(source.node, 20));
                    addFrame(node, nullptr, frame);
                    addNode(group.theme, nullptr, node);
                    host.marks[i] = { node, Read<void*>(frame, 4) };
                }
                areaTime(group.theme, nullptr, 0, 1);
                addPackageChild(host.package, nullptr, group.theme);
                addNode(host.page, nullptr, group.node);
                host.markImages = true;
            }
        }
        for (size_t i = 0; i < host.marks.size() && !host.markImages; ++i)
        {
            const int markY = y + (i ? plan->rows - 1 : 0) * rowHeight;
            host.marks[i] = Text(font, x + rowWidth + 6, markY, 24, style ? style->height : rowHeight, fontHeight, 0);
            if (!host.marks[i].node) continue;
            Color(host.marks[i], host.normalTextColor);
            addNode(host.page, nullptr, host.marks[i].node);
        }
        // Back in the bottom button row, in the place and dress of the Options page's Default button
        if (auto footer = findNode(host.parent, nullptr, Id("B_Default")); footer && Kind(footer) == 2)
        {
            const auto place = Bounds(footer);
            host.back.button = CreateButton(font, Id("WFP_Advanced_Back"), place.left, place.top, place.right - place.left,
                place.bottom - place.top, CategoryHeight(footer, fontHeight + 4), false);
            if (host.back.button.node && Bind(host, host.back.button, BackButton, false))
            {
                if (!Decorate(host.back, footer)) Color(host.back.button.label, host.back.textColor);
                Enabled(host.back.button.node, true);
                Caption(host.back.button.label.node, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.back"));
                Write(host.back.button.node, 28, Read<uint32_t>(host.back.button.node, 4));
                Write(host.back.button.node, 32, Read<uint32_t>(host.back.button.node, 4));
                Write(host.back.button.node, 36, Read<uint32_t>(host.back.button.node, 4));
                Write(host.back.button.node, 40, Read<uint32_t>(host.back.button.node, 4));
                addPackageChild(host.package, nullptr, host.back.button.theme);
                addNode(host.page, nullptr, host.back.button.node);
            }
            else
            {
                Delete(host.back.button.node); Delete(host.back.button.theme);
                host.back.button = {};
                LogOnce("prepare", host.parent, "native Back button construction failed");
            }
        }
        for (auto& row : host.rows) addPackageChild(host.package, nullptr, row.theme);
        addPackageChild(host.package, nullptr, host.page);
        areaTime(host.page, nullptr, 0, 1);
        addCursor(host.manager, nullptr, host.page);
        return true;
    }
    inline bool Decorate(Decorated& target, void* source)
    {
        std::vector<void*> children;
        if (!source || Kind(source) != 2 || !Nodes(Read<void*>(Data(source), 16), children)) return false;
        std::vector<TextNode> rectangles;
        std::vector<std::pair<TextNode, uint32_t>> borders;
        TextNode background, mask;
        uint32_t maskColor = 0;
        void* caption = nullptr;
        size_t captionPosition = SIZE_MAX;
        uint32_t borderColor = 0, selectionColor = 0;
        for (auto child : children)
        {
            const auto kind = Kind(child);
            if (kind == 8) { caption = child; captionPosition = rectangles.size(); continue; }
            if (kind != 5) continue;
            auto rect = Bounds(child);
            const bool fill = rect.right - rect.left > 4 && rect.bottom - rect.top > 4;
            if (fill && background.node) continue;
            auto clone = Rectangle(child);
            if (!clone.node)
            {
                for (auto& item : rectangles) Delete(item.node);
                return false;
            }
            rectangles.push_back(clone);
            if (fill)
            {
                if (!mask.node)
                {
                    mask = clone;
                    maskColor = Read<uint32_t>(clone.frame, 36);
                    continue;
                }
                background = clone;
                auto frames = Read<void*>(child, 12);
                auto first = Read<void**>(frames, 4), last = Read<void**>(frames, 8);
                if (first && last >= first && last - first <= 16)
                {
                    for (auto frame = first; frame != last; ++frame)
                    {
                        auto point = Read<void*>(*frame, 4);
                        const auto color = Read<uint32_t>(point, 36);
                        if ((color >> 24) && (color & 0x00FFFFFFu))
                        {
                            selectionColor = color;
                            break;
                        }
                    }
                }
                for (auto point : { clone.frame, Read<void*>(Data(clone.node), 4) })
                    for (size_t offset = 36; offset < 52; offset += 4) Write(point, offset, uint32_t(0xFFFFFFFFu));
                Color(rectangles.back(), 0u);
            }
            else
            {
                borderColor = Read<uint32_t>(clone.frame, 36);
                Color(rectangles.back(), borderColor);
                borders.push_back({ clone, borderColor });
            }
        }
        if (!caption || captionPosition > rectangles.size() || !background.node || !mask.node || rectangles.size() != 6 || !borderColor || !selectionColor)
        {
            for (auto& item : rectangles) Delete(item.node);
            return false;
        }
        auto frameList = Read<void*>(target.button.node, 12);
        auto firstFrame = frameList ? Read<void**>(frameList, 4) : nullptr;
        auto lastFrame = frameList ? Read<void**>(frameList, 8) : nullptr;
        auto sourceButtonPoint = Read<void*>(Data(source), 4);
        if (!firstFrame || lastFrame != firstFrame + 1 || !sourceButtonPoint)
        {
            for (auto& item : rectangles) Delete(item.node);
            return false;
        }
        const auto tint = Read<uint32_t>(sourceButtonPoint, 4) | 0xFF000000u;
        auto ownFramePoint = Read<void*>(*firstFrame, 4);
        auto ownCurrentPoint = Read<void*>(Data(target.button.node), 4);
        Write(ownFramePoint, 4, tint); Write(ownCurrentPoint, 4, tint);
        const auto sourceBody = Data(caption);
        auto sourcePoint = FirstFramePoint(caption);
        if (!sourcePoint) sourcePoint = Read<void*>(sourceBody, 4);
        const auto labelBody = Data(target.button.label.node);
        Write(labelBody, 8, Read<void*>(sourceBody, 8));
        Write(labelBody, 40, uint32_t((Read<uint32_t>(labelBody, 40) & ~7) | (Read<uint32_t>(sourceBody, 40) & 7)));
        for (auto point : { target.button.label.frame, Read<void*>(labelBody, 4) })
            for (size_t offset = 8; offset < 28; offset += 4) Write(point, offset, Read<uint32_t>(sourcePoint, offset));
        CopyRenderMode(target.button.node, source);
        CopyRenderMode(target.button.label.node, caption);
        target.textColor = Read<uint32_t>(sourcePoint, 4) | 0xFF000000u;
        target.selectionColor = selectionColor;
        target.background = background;
        target.mask = mask;
        target.maskColor = maskColor;
        target.borders = std::move(borders);
        Color(target.button.label, target.textColor);
        for (auto& item : rectangles) addNode(target.button.theme, nullptr, item.node);
        // Preserve the native caption position between image batches.
        auto first = Read<void**>(target.button.theme, 12);
        size_t index = 0;
        for (size_t i = 0; i <= rectangles.size(); ++i)
        {
            if (i == captionPosition) Write(first, index++ * sizeof(void*), target.button.label.node);
            if (i < rectangles.size()) Write(first, index++ * sizeof(void*), rectangles[i].node);
        }
        areaTime(target.button.theme, nullptr, 0, 1);
        return true;
    }
    inline bool Prepare(void* manager, void* parent)
    {
        if (hosts.contains(parent)) return true;
        auto package = OwningPackage(manager, parent);
        auto video = findNode(parent, nullptr, Id("B_Video")), controls = findNode(parent, nullptr, Id("B_Control")), sound = findNode(parent, nullptr, Id("B_Sound"));
        bool pause = false;
        if (!package) { LogOnce("prepare", parent, "active options page has no verified owning package"); return false; }
        if (!video || !controls || !sound)
        {
            LogOnce("prepare", parent, "native options buttons missing: video=" + std::to_string(video != nullptr) +
                " controls=" + std::to_string(controls != nullptr) + " sound=" + std::to_string(sound != nullptr));
            return false;
        }
        if (Kind(video) != 2 || Kind(controls) != 2 || Kind(sound) != 2)
        {
            LogOnce("prepare", parent, "native options button kinds: " + std::to_string(Kind(video)) + "," +
                std::to_string(Kind(controls)) + "," + std::to_string(Kind(sound)));
            return false;
        }
        if (!Pause(manager, pause))
        {
            wchar_t buffer[1024]{};
            auto owner = interaction(manager, nullptr);
            auto name = owner ? fullName(owner, nullptr, buffer) : nullptr;
            auto type = name ? std::wstring(name) : L"<unavailable>";
            LogOnce("prepare", parent, "unsupported interaction " + std::string(type.begin(), type.end()));
            return false;
        }
        auto font = Font(Read<void*>(Data(video), 16));
        int width = Read<int16_t>(package, 96), height = Read<int16_t>(package, 98);
        auto first = Bounds(video), last = Bounds(sound);
        if (first.left <= 0 || last.left <= 0) return false;
        if (!font || width < 640 || width > 4096 || height < 475 || height > 2160 || !Fits(first, width, height) || !Fits(last, width, height))
        { LogOnce("prepare", parent, "native font/package/button geometry unavailable"); return false; }
        int rowHeight = std::max(24, int(last.bottom - last.top)), gap = std::max(6, rowHeight / 3);
        int entryWidth = last.right - last.left;
        if (entryWidth < 80) return false;
        std::vector<Rect> candidates = {
            {first.left, int16_t(last.bottom + gap), int16_t(first.left + entryWidth), int16_t(last.bottom + gap + rowHeight)},
            {int16_t(last.right + gap), last.top, int16_t(last.right + gap + entryWidth), last.bottom},
            {int16_t(width / 12), int16_t(height / 10), int16_t(width / 12 + entryWidth), int16_t(height / 10 + rowHeight)}
        };
        auto at = std::find_if(candidates.begin(), candidates.end(), [&](Rect rect) { return Fits(rect, width, height) && Empty(parent, rect); });
        if (at == candidates.end()) { LogOnce("prepare", parent, "no safe native category rectangle"); return false; }
        auto host = std::make_shared<Host>(); host->manager = manager; host->parent = parent; host->package = package; host->pause = pause;
        hosts.emplace(parent, host);
        if (!Build(*host, font, width, height)) { hosts.erase(parent); LogOnce("prepare", parent, "native page/row creation failed"); return false; }
        host->entry.button = CreateButton(font, Id("WFP_Advanced_Options"), at->left, at->top, at->right - at->left, rowHeight, CategoryHeight(video, std::clamp(height / 30, 14, 24)), false);
        if (!host->entry.button.node || !Bind(*host, host->entry.button, 0, true))
        {
            Delete(host->entry.button.node); Delete(host->entry.button.theme);
            // The fully constructed native page is already package-owned and
            // stays hidden until its normal package teardown.
            std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == host.get(); });
            hosts.erase(parent);
            LogOnce("prepare", parent, "native category construction failed"); return false;
        }
        if (!Decorate(host->entry, video))
        {
            LogOnce("prepare", parent, "native category decoration unavailable");
            Color(host->entry.button.label, host->entry.textColor);
        }
        Caption(host->entry.button.label.node, RWGTAMenu::PluginText(FixSettings(), Language(), "menu.plugins"));
        // Under the first category: between it and the row below it, as Up/Down go; else after the last one
        const bool below = at == candidates.begin();
        host->direction = below ? 1 : std::abs(first.top - last.top) < rowHeight ? 3 : 1;
        host->preceding = below ? video : sound;
        host->previousNext = Read<uint32_t>(host->preceding, 28 + host->direction * 4);
        host->following = findNode(parent, nullptr, host->previousNext);
        int previousDirection = host->direction == 3 ? 2 : 0;
        if (host->following) host->followingPrevious = Read<uint32_t>(host->following, 28 + previousDirection * 4);
        Write(host->entry.button.node, 28 + previousDirection * 4, Read<uint32_t>(host->preceding, 4));
        Write(host->entry.button.node, 28 + host->direction * 4, host->previousNext ? host->previousNext : Read<uint32_t>(video, 4));
        addPackageChild(package, nullptr, host->entry.button.theme); addNode(parent, nullptr, host->entry.button.node);
        Write(host->preceding, 28 + host->direction * 4, Read<uint32_t>(host->entry.button.node, 4));
        if (host->following && host->followingPrevious == Read<uint32_t>(host->preceding, 4))
            Write(host->following, 28 + previousDirection * 4, Read<uint32_t>(host->entry.button.node, 4));
        linkNeighbors(parent); areaTime(parent, nullptr, Read<unsigned int>(parent, 24), 0);
        if (!JoinControls(*host, font, width, height)) LogOnce("controls", parent, "native mouse geometry/defaults unavailable; settings remain in Plugins");
        return true;
    }
    inline void Forget(Host& host)
    {
        ++host.generation; host.valid = false; host.open = false; host.details = false; host.detailsLines.clear();
        host.joinedModel.Close();
        if (active == &host) { active = nullptr; model.Close(); }
        std::erase_if(bindings, [&host](auto item) { return item.second.host.lock().get() == &host; });
    }
    inline void QueuePage(void* manager, void* page)
    {
        if (!manager || !page || hosts.contains(page) || pendingPages.size() >= 64) return;
        if (std::any_of(pendingPages.begin(), pendingPages.end(), [&](const auto& item) { return item.manager == manager && item.page == page; })) return;
        if (auto owner = interaction(manager, nullptr)) pendingPages.push_back({ manager, page, owner });
    }
    inline void ForgetPages(void* manager, void* page)
    {
        auto matches = [&](const auto& item) { return (manager && item.manager == manager) || (page && item.page == page); };
        std::erase_if(pendingPages, matches);
        if (preparingPage && matches(*preparingPage)) *preparingPage = {};
    }
    inline bool BlocksCinematics(void* page)
    {
        if (!page)
            return false;
        if (active && active->open && page == active->page)
            return true;
        const auto id = Read<uint32_t>(page, 4);
        constexpr const char* pages[] = { "P_MainMenu", "P_Pause", "P_Options", "P_Video", "P_Control", "P_Control_P2",
            "P_Sound", "P_Profile", "P_ProfileManagement", "P_SaveLoadGame", "P_Save", "P_LoadGame",
            "P_Difficulty", "P_Training", "P_CreateProfile", "P_Adjust_display", "P_Pre_LoadingScreen", "P_LoadingScreen" };
        return std::any_of(std::begin(pages), std::end(pages), [id](const auto name) { return id == Id(name); });
    }
    // A focus move walks the neighbour links past disabled nodes. Two disabled nodes linking each other (a choice
    // list with greyed entries) kept the game's walk going forever; stop at a loop
    inline void* __fastcall NavigateFocus(void* node, void*, int direction)
    {
        auto next = node;
        for (int steps = 0; steps < 1024; ++steps)
        {
            next = Read<void*>(next, 0x2C + direction * 4);
            if (!next || next == node) return node;
            if (Read<uint8_t>(next, 0x3C) & 2)
            {
                Virtual<void>(node, 4);
                Virtual<void>(next, 3);
                return next;
            }
        }
        return node;
    }
    inline uint8_t __fastcall Display(void* manager, void*, void* page, bool show, bool hidePrevious, int priority)
    {
        auto result = displayHook.thiscall<uint8_t>(manager, page, show, hidePrevious, priority);
        NativeMenuBlocksCinematics.store(BlocksCinematics(topMenu(manager, nullptr)), std::memory_order_relaxed);
        menusManager = manager;
        if (!busy && result && show && page && Read<uint32_t>(page, 4) == Id("P_Options") && topMenu(manager, nullptr) == page)
            QueuePage(manager, page);
        return result;
    }
    inline void __fastcall Tick(void* object, void*, float delta)
    {
        tickHook.thiscall<void>(object, delta);
        // No screen drops over the menus: the top page, however it was shown or closed
        if (menusManager && interaction(menusManager, nullptr) == object) RainDroplets::MenuOpen(BlocksCinematics(topMenu(menusManager, nullptr)));
        if (busy) return;
        BusyGuard guard;
        // The controller localizes/positions Options after DisplayMenu returns.
        // Wait for its completed tick before borrowing stock styles or geometry.
        for (size_t i = 0; i < pendingPages.size();)
        {
            auto candidate = pendingPages[i];
            if (candidate.owner != object) { ++i; continue; }
            pendingPages.erase(pendingPages.begin() + i);
            if (interaction(candidate.manager, nullptr) != object || topMenu(candidate.manager, nullptr) != candidate.page) continue;
            preparingPage = &candidate;
            struct RetireAttempt { ~RetireAttempt() { preparingPage = nullptr; } } retireAttempt;
            if (!Prepare(candidate.manager, candidate.page) && candidate.page && ++candidate.attempts < 120)
                pendingPages.push_back(candidate);
            // A native teardown can change the queue during Prepare.
            break;
        }
        // Only here have the native mouse/key dispatch and post-Accept page
        // accesses returned. External setting/action callbacks may destroy UI.
        auto actions = std::move(pending); pending.clear();
        for (auto& action : actions)
        {
            auto host = action.host.lock();
            if (!host || !Live(*host) || action.generation != host->generation) continue;
            if (interaction(host->manager, nullptr) != object) { pending.push_back(action); continue; }
            if (action.action == Action::Defaults && JoinedInteractive(*host))
            {
                Snapshot snapshot(*host);
                auto restart = RWGTAMenu::RestoreDefaults(FixSettings(), ControlSettings);
                if (!snapshot.Current()) continue;
                host->joinedRestart |= restart; RefreshJoined(*host);
            }
            else if (action.action == Action::JoinedChange && JoinedInteractive(*host))
            {
                Snapshot snapshot(*host);
                auto& rows = host->joinedModel.JoinedRows();
                if (action.row >= rows.size()) continue;
                host->joinedModel.Change(rows[action.row], action.direction, host->pause);
                if (!snapshot.Current()) continue;
                RefreshJoined(*host);
            }
            else if (action.action == Action::Open) Open(*host);
            else if (Interactive(*host))
            {
                if (action.action == Action::Activate) Activate(*host, action.row, action.direction);
                else if (action.action == Action::Details) { if (host->details) EndDetails(*host); else BeginDetails(*host); }
                else if (action.action == Action::Move || action.action == Action::Page || action.action == Action::Edge ||
                    action.action == Action::Scroll) Navigate(*host, action.action, action.direction);
                else Back(*host);
            }
        }
        std::vector<std::shared_ptr<Host>> joinedHosts;
        auto now = GetTickCount64();
        for (const auto& item : hosts)
        {
            auto& joined = *item.second;
            if (!joined.joined.empty() && interaction(joined.manager, nullptr) == object && JoinedInteractive(joined) &&
                (joined.joinedRefresh || now - joined.joinedUpdated >= 100)) joinedHosts.push_back(item.second);
        }
        for (auto& joined : joinedHosts) if (Live(*joined) && JoinedInteractive(*joined)) RefreshJoined(*joined);
        std::shared_ptr<Host> host;
        if (active) { auto at = hosts.find(active->parent); if (at != hosts.end()) host = at->second; }
        // The wheel, a row per notch
        if (host && interaction(host->manager, nullptr) == object && Interactive(*host))
            for (int notches = wheel.Take(wheelDelta.exchange(0)); notches && Live(*host) && Interactive(*host); notches += notches > 0 ? -1 : 1)
                Navigate(*host, Action::Scroll, notches > 0 ? -1 : 1);
        if (host && interaction(host->manager, nullptr) == object && Interactive(*host) && (host->refresh || RWGTAMenu::Model::Changed().exchange(false)))
        {
            Snapshot snapshot(*host);
            host->refresh = false; Populate(*host);
            if (!snapshot.Current()) return;
            auto node = Read<void*>(host->page, 36);
            if (!node || !(Read<uint8_t>(node, 24) & 1) || !(Read<uint8_t>(node, 60) & 2)) First(*host);
        }
    }
    inline uint8_t __fastcall Uninitialize(void* manager, void*)
    {
        BusyGuard guard;
        if (menusManager == manager) { menusManager = nullptr; RainDroplets::MenuOpen(false); }
        ForgetPages(manager, nullptr);
        for (auto it = hosts.begin(); it != hosts.end();)
        {
            if (it->second->manager != manager) { ++it; continue; }
            auto host = it->second; it = hosts.erase(it); Forget(*host);
        }
        auto result = uninitializeHook.thiscall<uint8_t>(manager);
        return result;
    }
    inline void* __fastcall Destroy(void* object, void*, uint8_t release)
    {
        ForgetPages(nullptr, object);
        for (auto it = hosts.begin(); it != hosts.end();)
        {
            if (it->second->parent != object && it->second->page != object) { ++it; continue; }
            auto host = it->second; it = hosts.erase(it); Forget(*host);
        }
        return destroyHook.thiscall<void*>(object, release);
    }
    inline void Init()
    {
        auto engine = GetModuleHandleW(L"Engine.dll"), core = GetModuleHandleW(L"Core.dll");
        if (displayHook) return;
        auto find = [engine](const char* pattern) { return hook::module_pattern(engine, pattern).get_first<uint8_t>(); };
        auto exported = [engine](const char* name) { return reinterpret_cast<void*>(GetProcAddress(engine, name)); };
        fullName = reinterpret_cast<decltype(fullName)>(GetProcAddress(core, "?GetFullName@UObject@@QBEPBGPAG@Z"));
        if (!fullName) fullName = reinterpret_cast<decltype(fullName)>(GetProcAddress(core, "?GetFullName@UObject@@QBEPB_WPA_W@Z"));
        gameLanguage = reinterpret_cast<decltype(gameLanguage)>(GetProcAddress(core, "?GetLanguage@UObject@@SAPBGXZ"));
        if (!gameLanguage) gameLanguage = reinterpret_cast<decltype(gameLanguage)>(GetProcAddress(core, "?GetLanguage@UObject@@SAPB_WXZ"));
        if (!fullName || !gameLanguage)
            throw std::runtime_error("Core name/language exports unavailable");
        FactorySlot.SetAddress(*reinterpret_cast<void***>(find(Contracts::FactoryGetter) + 1));
        createArea = reinterpret_cast<decltype(createArea)>(find(Contracts::CreateArea));
        createNode = reinterpret_cast<decltype(createNode)>(find(Contracts::CreateNode));
        createFrame = reinterpret_cast<decltype(createFrame)>(find(Contracts::CreateFrame));
        addNode = reinterpret_cast<decltype(addNode)>(find(Contracts::AddNode));
        // The widescreen HUD may have detoured this function already
        auto ownAddFrame = NativeMenuAddFrameHook();
        addFrame = reinterpret_cast<decltype(addFrame)>(ownAddFrame ? reinterpret_cast<uint8_t*>(ownAddFrame->target_address()) : find(Contracts::AddFrame));
        linkNeighbors = reinterpret_cast<decltype(linkNeighbors)>(find(Contracts::LinkNeighbors));
        areaTime = reinterpret_cast<decltype(areaTime)>(find(Contracts::AreaTime));
        createHandler = reinterpret_cast<decltype(createHandler)>(find(Contracts::CreateHandler));
        addHandler = reinterpret_cast<decltype(addHandler)>(find(Contracts::AddHandler));
        findNode = reinterpret_cast<decltype(findNode)>(find(Contracts::FindNode));
        findPage = reinterpret_cast<decltype(findPage)>(find(Contracts::FindPage));
        addPackageChild = reinterpret_cast<decltype(addPackageChild)>(find(Contracts::AddPackageChild));
        wideAssign = reinterpret_cast<decltype(wideAssign)>(find(Contracts::WideAssign));
        selectNode = reinterpret_cast<decltype(selectNode)>(find(Contracts::SelectNode));
        blurNode = reinterpret_cast<decltype(blurNode)>(find(Contracts::BlurNode));
        topMenu = reinterpret_cast<decltype(topMenu)>(exported("?GetTopMenu@CMenusManager@@QAEPAVPage@magma@@XZ"));
        menuPackage = reinterpret_cast<decltype(menuPackage)>(exported("?GetMenuPackage@CMenusManager@@UAEPAVPackage@magma@@PBD@Z"));
        interaction = reinterpret_cast<decltype(interaction)>(exported("?GetInteraction@CMenusManager@@QAEPAVUSCMagmaInteraction@@XZ"));
        addCursor = reinterpret_cast<decltype(addCursor)>(exported("?AddCursorToPage@CMenusManager@@QAEXPAVPage@magma@@@Z"));
        auto native = *reinterpret_cast<void***>(find(Contracts::HandlerConstructor) + 25);
        std::copy_n(native, handlerTable.size(), handlerTable.begin());
        handlerTable[2] = reinterpret_cast<void*>(Focused); handlerTable[3] = reinterpret_cast<void*>(Blurred);
        handlerTable[4] = reinterpret_cast<void*>(Accept); handlerTable[5] = reinterpret_cast<void*>(Cancel);
        handlerTable[6] = reinterpret_cast<void*>(Key);
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&hostModule), &hostModule);
        if (!hostModule)
            throw std::runtime_error("Native menu host module unavailable");
        uninitializeHook = safetyhook::create_inline(exported("?Uninitialize@CMenusManager@@UAE_NXZ"), Uninitialize);
        destroyHook = safetyhook::create_inline(find(Contracts::PageDestroy), Destroy);
        tickHook = safetyhook::create_inline(exported("?ProcessTick@USCMagmaInteraction@@MAEXM@Z"), Tick);
        if (!uninitializeHook || !destroyHook || !tickHook)
        {
            throw std::runtime_error("Native menu lifetime or tick hook failed");
        }
        navigateHook = safetyhook::create_inline(find(Contracts::Navigate), NavigateFocus);
        displayHook = safetyhook::create_inline(exported("?DisplayMenu@CMenusManager@@QAE_NPAVPage@magma@@_N1W4EMENUPRIORITY@@@Z"), Display);
        if (!displayHook)
        {
            throw std::runtime_error("Native display hook failed");
        }
        auto module = GetModuleHandleW(L"EchelonMenus.dll");
        auto pattern = hook::module_pattern(module, "64 A1 00 00 00 00 6A FF 68 ? ? ? ? 50 64 89 25 00 00 00 00 83 EC 70 53 8B 1D ? ? ? ? 56 57 8B F9 8B 47 18 8B 48 08 85 C9 74 04 FF D3 EB 02 33 C0");
        defaultsHook = safetyhook::create_inline(pattern.get_first(), Defaults);
        if (!defaultsHook)
        {
            throw std::runtime_error("Native menu defaults hook failed");
        }
    }
}
