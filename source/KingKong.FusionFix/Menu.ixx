module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include "Shared/RWGTA/FixMenu.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"
#include "Shared/MenuList.hpp"
#include "Shared/PadInput.hpp"
#include <intrin.h>
#include <set>
#include <deque>
#include <memory>
#include <cmath>
#include <algorithm>
#include <functional>

export module Menu;

import ComVars;

// The fix's settings as an extra entry of the game's Options page (Gamer's Edition)
namespace KongMenu::Native
{
    // Game pointers that are not set yet read as zero
    template<class T> T Read(const void* object, size_t offset)
    {
        T value{};
        if (object)
            std::memcpy(&value, static_cast<const uint8_t*>(object) + offset, sizeof(value));
        return value;
    }

    // Gamer's Edition function addresses, resolved by Resolve() once the executable is recognised; its variables are
    // in ComVars
    struct Addresses
    {
        uintptr_t model, slot, setText, move, flags, color, size, align, measure, clearEffects, font, pressed, key;
        uintptr_t common, commonInput, whole, world, options, select, mouse, title, pause, temporary, object, behavior;
        uintptr_t language, subtitle, selectReturn, mouseReturn;
    };

    Addresses& Address()
    {
        static Addresses addresses{};
        return addresses;
    }

    void Resolve()
    {
        static const bool resolved = []()
        {
            auto& a = Address();
            auto at = [](const char* signature, ptrdiff_t offset = 0)
            {
                return reinterpret_cast<uintptr_t>(hook::get_pattern(signature, offset));
            };
            auto call = [](const char* signature)
            {
                return injector::GetBranchDestination(hook::get_pattern(signature)).as_int();
            };
            auto global = [](const char* signature, ptrdiff_t offset)
            {
                return *hook::get_pattern<uintptr_t>(signature, offset);
            };
            a.model = at("8B 44 24 04 85 C0 74 19 8B 08 F6 C5 40 74 12 8B 40 14 8B 08 8B 01 85 C0 74 07 8B 10 83 3A 0A 74 02 33 C0 C3");
            a.slot = at("8B 4C 24 04 8B 41 1C 85 C0 74 10 8B 44 24 08 85 C0");
            a.setText = at("53 8B 5C 24 10 85 DB 0F 84 ? ? ? ? 56 57 8B 7C 24 10");
            a.move = call("E8 ? ? ? ? 83 C4 0C EB 5F");
            a.flags = call("E8 ? ? ? ? 83 C4 0C EB 2D");
            a.color = call("E8 ? ? ? ? 83 C4 14 6A 10");
            a.size = at("8B 44 24 04 83 EC 08 85 C0 0F 84 ? ? ? ? 8B 08");
            a.align = at("8B 44 24 04 83 EC 14 85 C0 0F 84 ? ? ? ? 8B 08");
            a.measure = at("83 EC 18 57 8B 7C 24 30 85 FF 75 07 33 C0 5F 83 C4 18");
            a.clearEffects = at("8B 44 24 04 50 E8 ? ? ? ? 83 C4 04 85 C0 0F 84 ? ? ? ?");
            a.font = at("53 55 56 8B 74 24 14 83 FE 02 73 0D E8 ? ? ? ?");
            a.pressed = call("E8 ? ? ? ? 83 C4 04 EB 3A 8B 45 08");
            a.key = call("E8 ? ? ? ? 83 C4 04 85 C0 75 49");
            a.common = at("55 8B EC 83 EC 50 A1 ? ? ? ? 8B 48 40 89 4D FC 8B 15 ? ? ? ? 89 55 F8 8D 45 D4 50 E8 ? ? ? ? 83 C4 04 8B 4D D4 89 4D EC 8B 55 D8 89 55 F0 8B 45 DC 89 45 F4 8B 4D FC 81 C1 E4 01 00 00");
            a.commonInput = at("55 8B EC 83 EC 08 A1 ? ? ? ? 8B 48 40 89 4D FC 8B 15 ? ? ? ? 89 55 F8 E8 ? ? ? ? 83 F8 02");
            a.whole = at("55 8B EC 83 EC 24 68 ? ? ? ? E8 ? ? ? ? 83 C4 04 E8 ? ? ? ?");
            a.world = at("55 8B EC 81 EC 9C 00 00 00 8B 45 08 A3 ? ? ? ?");
            a.options = at("55 8B EC 81 EC C0 00 00 00 A1 ? ? ? ? 8B 48 40 89 4D F0 8B 15 ? ? ? ? 89 55 EC 8B 45 F0 83 78 08 00");
            a.select = at("55 8B EC 83 EC 08 A1 ? ? ? ? 8B 48 40 89 4D FC 8B 15 ? ? ? ? 89 55 F8 8B 45 FC 83 78 4C 01 75 38");
            a.mouse = at("55 8B EC 83 EC 08 A1 ? ? ? ? 8B 48 40 89 4D FC 8B 15 ? ? ? ? 89 55 F8 83 7D 08 06");
            a.title = at("55 8B EC 81 EC E8 00 00 00 A1 ? ? ? ? 8B 48 40 89 4D F8 8B 15 ? ? ? ? 89 55 F4 8B 45 F8 83 78 08 00");
            a.pause = at("55 8B EC 81 EC 2C 01 00 00 A1 ? ? ? ? 8B 48 40 89 4D 98");
            a.temporary = at("55 8B EC 83 EC 0C 8B 45 08 8B 08 83 E1 08 75 0F");
            a.object = at("55 8B EC 83 EC 1C 8B 45 08 8B 08 83 E1 08 75 19");
            a.behavior = at("55 8B EC 83 EC 18 83 7D 08 00 75 05 E9 ? ? ? ? 8B 45 08 83 38 00");
            a.language = at("55 8B EC 83 EC 08 6A 00 8B 45 08 50 E8 ? ? ? ? 83 C4 08 85 C0");
            a.subtitle = at("55 8B EC 83 EC 08 6A 01 8B 45 08 50 E8 ? ? ? ?");
            // Return addresses of the Options page's selection and mouse calls
            a.selectReturn = at("E8 ? ? ? ? 83 C4 08 8B 4D E8 51 E8 ? ? ? ? 83 C4 04 8B 55 F0", 5);
            a.mouseReturn = at("E8 ? ? ? ? 83 C4 04 8B 55 F0 8B 42 3C", 5);
            pCurrentBehavior.SetAddress(reinterpret_cast<void**>(global("A3 ? ? ? ? 8B 0D ? ? ? ? 83 C1 01", 1)));
            pCurrentMenuOwner.SetAddress(reinterpret_cast<void**>(global("8B 15 ? ? ? ? 8B 42 04 25 ? ? ? ?", 2)));
            nMenuState.SetAddress(reinterpret_cast<int*>(global("8B 15 ? ? ? ? A1 ? ? ? ? 8B 4D 08", 2)));
            nLocale.SetAddress(reinterpret_cast<uint32_t*>(global("A1 ? ? ? ? 89 45 D8 8B 4D D8 89 4D E8", 1)));
            pOptionsTarget.SetAddress(reinterpret_cast<void**>(global("A3 ? ? ? ? 68 F2 D7 02 49 E8 ? ? ? ?", 1)));
            aGlyphMap.SetAddress(reinterpret_cast<std::array<uint32_t, 256>*>(global("8B 14 85 ? ? ? ? 0B 54 24 14 41 89 16", 3)));
            pScratchObject.SetAddress(reinterpret_cast<void**>(global("A1 ? ? ? ? 85 C0 74 15 8B 40 14 8B 00", 1)));
            pTextureTable.SetAddress(reinterpret_cast<uintptr_t*>(global("8B 15 ? ? ? ? 0F BF 44 0A 06 8B 4D 08", 2)));
            pFontCache.SetAddress(reinterpret_cast<void**>(global("A1 ? ? ? ? 8B 44 06 10 3B C3 74 13 50", 1)));
            nFontCacheCapacity.SetAddress(reinterpret_cast<int*>(global("89 0D ? ? ? ? 6B C9 1C 3B C5 74 1F 6A 7A", 2)));
            nFontCacheCount.SetAddress(reinterpret_cast<int*>(global("A1 ? ? ? ? EB 02 33 C0 5D C3", 1)));
            bMenuReady.SetAddress(reinterpret_cast<uint8_t*>(global("83 3D ? ? ? ? 00 74 11 8B 55 08 83 7A 14 00", 2)));
            return true;
        }();
        (void)resolved;
    }

    struct Glyph
    {
        uint32_t symbol;
        int16_t x, y;
        uint16_t width, height;
        uint32_t color;
    };
    static_assert(sizeof(Glyph) == 16);

    struct TextSlot
    {
        uint32_t flags;
        uint16_t start, count, first, visible;
        float x, y, z;
        uint32_t remaining[6];
    };
    static_assert(sizeof(TextSlot) == 48);

    void* Model(void* object)
    {
        return reinterpret_cast<void* (__cdecl*)(void*)>(Address().model)(object);
    }

    TextSlot* Slot(void* model, int index)
    {
        return reinterpret_cast<TextSlot* (__cdecl*)(void*, int)>(Address().slot)(model, index);
    }

    void SetText(void* object, int index, char* text)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, char*)>(Address().setText)(object, index, text);
    }

    void Move(void* object, int index, const float* position)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, const float*)>(Address().move)(object, index, position);
    }

    void Flags(void* object, int index, int16_t flags)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int16_t)>(Address().flags)(object, index, flags);
    }

    void Color(void* object, int index, uint32_t color)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, uint32_t)>(Address().color)(object, index, 0, -1, color);
    }

    void Size(void* object, int index, const float* size)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, const float*)>(Address().size)(object, index, 0, -1, size);
    }

    void Align(void* object, int index, int mode)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, int)>(Address().align)(object, index, 0, -1, mode);
    }

    void Measure(void* object, int index, float* extent)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int, float*)>(Address().measure)(object, index, 0, -1, extent);
    }

    void ClearEffects(void* object, int index)
    {
        reinterpret_cast<void (__cdecl*)(void*, int, int, int)>(Address().clearEffects)(object, 1, -1, index);
    }

    void* Font(void* group, uint32_t index)
    {
        return reinterpret_cast<void* (__cdecl*)(void*, uint32_t)>(Address().font)(group, index);
    }

    bool Pressed(int action)
    {
        return reinterpret_cast<int (__cdecl*)(int)>(Address().pressed)(action) != 0;
    }

    bool Key(int code)
    {
        return reinterpret_cast<int (__cdecl*)(int)>(Address().key)(code) != 0;
    }

    // The setter parses percent substitutions before backslash commands. Encoding
    // each mapped glyph as a native 'a' command bypasses both literal grammars.
    bool Literal(const std::string& ascii, uint32_t fontGlyphCount, std::string& encoded)
    {
        encoded.clear();
        if (ascii.size() > 256 || !fontGlyphCount || fontGlyphCount > 4096)
            return false;
        for (unsigned char c : ascii)
        {
            if (c < 32 || c == 127)
                return false;
            const auto glyph = aGlyphMap[c];
            if (glyph < 32 || glyph > 255 || glyph - 32 >= fontGlyphCount)
                return false;
            encoded += "\\a" + std::to_string(glyph) + "\\";
        }
        return encoded.size() < 4095;
    }
}

namespace KongMenu
{
    // A text in capitals, as the menus' headings and entries are
    inline std::string Upper(const std::string& utf8)
    {
        auto wide = asi_settings::text::Widen(utf8);
        if (!wide.empty())
            CharUpperBuffW(wide.data(), DWORD(wide.size()));
        return asi_settings::text::Narrow(wide);
    }

    struct Context
    {
        void* world = nullptr;
        void* owner = nullptr;
        void* behavior = nullptr;
        void* parameters = nullptr;
        void* page = nullptr;
        void* text = nullptr;
        void* model = nullptr;
        void* descriptors = nullptr;
        void* glyphs = nullptr;
        void* fontGroup = nullptr;
        uint32_t glyphCapacity = 0;
        int first = 0;
        uint8_t slotCount = 0;
        uint16_t width = 0, height = 0;
        Native::TextSlot style{};
        Native::TextSlot titleStyle{};      // the page's title ("OPTIONS")
        uint32_t fontPage = 0;              // the font page of the page's rows
        uint32_t titleColor = 0xFFFFFFFF;

        // The right edge is at x (the page's labels are right-justified)
        static bool RightJustified(uint32_t flags) { return (flags & 16) != 0; }

        static bool ScratchReady()
        {
            // The explicit setters use a native scratch object's graphic pair
            // for their font-group assignment before walking any source glyphs.
            void* scratch = pScratchObject;
            auto graphic = Native::Read<void*>(scratch, 20);
            return Native::Read<void*>(graphic, 0) != nullptr;
        }

        void* FontReady() const
        {
            // Follow the exact index-zero resource route used by 9C2700, but
            // require an existing cache entry. No font registry allocation is
            // permitted while publishing a private page.
            auto resource = fontGroup;
            if (!resource) return nullptr;
            auto kind = Native::Read<void*>(resource, 0);
            if (!kind) return nullptr;
            if (Native::Read<uint32_t>(kind, 0) == 4)
            {
                auto count = Native::Read<uint32_t>(resource, 8);
                auto table = Native::Read<void*>(resource, 12);
                if (!count || count > 4096 || !table) return nullptr;
                // The font page's material; the Japanese ones of the first two pages follow at 22
                auto index = fontPage + (nLocale == 12 && fontPage < 2 ? 22U : 0U);
                if (index >= count) index = 0;
                resource = Native::Read<void*>(table, index * 4);
                if (!resource) return nullptr;
                kind = Native::Read<void*>(resource, 0);
                if (!kind) return nullptr;
            }
            int handle = -1;
            switch (Native::Read<uint32_t>(kind, 0))
            {
                case 3: handle = Native::Read<int>(resource, 32); break;
                case 5:
                {
                    auto pointer = Native::Read<void*>(resource, 32);
                    if (!pointer) return nullptr;
                    handle = Native::Read<int16_t>(pointer, 0);
                    break;
                }
                default: return nullptr;
            }
            void* texture = nullptr;
            if (handle != -1)
            {
                if (handle < 0 || handle > 65535) return nullptr;
                uintptr_t table = pTextureTable;
                if (!table) return nullptr;
                auto entry = reinterpret_cast<void*>(table + size_t(handle) * 12);
                if (Native::Read<uint8_t>(entry, 8) & 0x40)
                {
                    handle = Native::Read<int16_t>(entry, 6);
                    if (handle < 0) return nullptr;
                    entry = reinterpret_cast<void*>(table + size_t(handle) * 12);
                }
                texture = Native::Read<void*>(entry, 0);
                if (!texture) return nullptr;
            }
            void* cache = pFontCache;
            int capacity = nFontCacheCapacity;
            int count = nFontCacheCount;
            if (!cache || count <= 0 || capacity < count) return nullptr;
            // Native first searches the texture identity, then its resource key.
            for (int pass = 0; pass < 2; ++pass)
                for (int i = 0; i < count; ++i)
                {
                    auto entry = static_cast<uint8_t*>(cache) + size_t(i) * 28;
                    if ((pass == 0 && texture && (Native::Read<void*>(entry, 0) == texture ||
                                                Native::Read<void*>(entry, 4) == texture)) ||
                        (pass == 1 && Native::Read<void*>(entry, 8) == resource))
                        return entry;
                }
            return nullptr;
        }

        static void* TextModel(void* object, void** group = nullptr)
        {
            if (!object || !(Native::Read<uint32_t>(object, 0) & 0x4000))
                return nullptr;
            auto graphic = Native::Read<void*>(object, 20);
            auto pair = Native::Read<void*>(graphic, 0);
            auto candidate = Native::Read<void*>(pair, 0);
            auto descriptor = Native::Read<void*>(candidate, 0);
            if (!descriptor || Native::Read<uint32_t>(descriptor, 0) != 10)
                return nullptr;
            if (group)
                *group = Native::Read<void*>(pair, 4);
            return candidate;
        }

        bool Member() const
        {
            // The behavior iterator in 406340 uses eight-byte object entries.
            if (!world)
                return false;
            auto begin = Native::Read<uintptr_t>(world, 304);
            auto end = Native::Read<uintptr_t>(world, 292);
            if (!begin || end < begin)
                return false;
            for (auto at = begin; at < end; at += 8)
                if (Native::Read<void*>(reinterpret_cast<void*>(at), 0) == owner)
                    return true;
            return false;
        }

        bool Alive() const
        {
            if (!Member() || !owner || !behavior || !parameters || !page)
                return false;
            auto component = Native::Read<void*>(owner, 24);
            if (!component || Native::Read<void*>(component, 4) != behavior ||
                Native::Read<void*>(behavior, 64) != parameters ||
                Native::Read<void*>(parameters, 8) != text ||
                Native::Read<int>(parameters, 248) != first ||
                Native::Read<uintptr_t>(page, 20) != Native::Address().options)
                return false;
            void* group = nullptr;
            if (TextModel(text, &group) != model || group != fontGroup ||
                Native::Read<void*>(model, 28) != glyphs ||
                Native::Read<void*>(model, 32) != descriptors ||
                Native::Read<uint32_t>(model, 8) != glyphCapacity ||
                Native::Read<uint8_t>(model, 12) != slotCount ||
                Native::Read<uint16_t>(model, 24) != width ||
                Native::Read<uint16_t>(model, 26) != height)
                return false;
            return true;
        }

        bool Current() const
        {
            return Alive() && Native::Read<void*>(behavior, 36) == page &&
                !Native::Read<void*>(behavior, 32) &&
                Native::Read<uint32_t>(parameters, 24) &&
                !Native::Read<uint32_t>(parameters, 32) &&
                !Native::Read<uint32_t>(parameters, 216) &&
                !Native::Read<uint32_t>(parameters, 1388);
        }

        static bool Capture(void* world, Context& result)
        {
            Context next;
            next.world = world;
            next.owner = pCurrentMenuOwner;
            next.behavior = pCurrentBehavior;
            if (!next.behavior)
                return false;
            next.parameters = Native::Read<void*>(next.behavior, 64);
            next.page = Native::Read<void*>(next.behavior, 36);
            if (!next.parameters || !next.page)
                return false;
            next.text = Native::Read<void*>(next.parameters, 8);
            next.model = TextModel(next.text, &next.fontGroup);
            if (!next.model || !next.fontGroup)
                return false;
            next.first = Native::Read<int>(next.parameters, 248);
            next.slotCount = Native::Read<uint8_t>(next.model, 12);
            next.glyphCapacity = Native::Read<uint32_t>(next.model, 8);
            next.glyphs = Native::Read<void*>(next.model, 28);
            next.descriptors = Native::Read<void*>(next.model, 32);
            next.width = Native::Read<uint16_t>(next.model, 24);
            next.height = Native::Read<uint16_t>(next.model, 26);
            if (next.slotCount < 30 || next.first < 0 || next.first > int(next.slotCount) - 30 ||
                !next.glyphCapacity || next.glyphCapacity > 65535 || !next.glyphs || !next.descriptors ||
                next.width < 320 || next.height < 240 || next.width > 16384 || next.height > 16384)
                return false;
            auto slot = static_cast<Native::TextSlot*>(next.descriptors) + next.first + 5;
            std::memcpy(&next.style, slot, sizeof(next.style));
            std::memcpy(&next.titleStyle, static_cast<Native::TextSlot*>(next.descriptors) + next.first, sizeof(next.titleStyle));
            if (next.style.count && next.style.start < next.glyphCapacity)
                next.fontPage = (Native::Read<Native::Glyph>(static_cast<Native::Glyph*>(next.glyphs) + next.style.start, 0).symbol >> 12) & 63;
            if (next.titleStyle.count && next.titleStyle.start < next.glyphCapacity)
                next.titleColor = Native::Read<Native::Glyph>(static_cast<Native::Glyph*>(next.glyphs) + next.titleStyle.start, 0).color;
            if (!next.titleStyle.count || !std::isfinite(next.titleStyle.x) || !std::isfinite(next.titleStyle.y) ||
                next.titleStyle.x <= 0.0f || next.titleStyle.x > 1.0f || next.titleStyle.y < 0.0f || next.titleStyle.y > 1.0f)
                next.titleStyle = next.style;
            if (!std::isfinite(next.style.x) || !std::isfinite(next.style.y) ||
                !std::isfinite(next.style.z) || next.style.x < 0.0f || next.style.x > 1.0f ||
                next.style.y < 0.0f || next.style.y > 1.0f || !next.Current())
                return false;
            result = next;
            return true;
        }
    };
}

namespace KongMenu
{
    // Native text copies glyphs, not the caller's string. All font pointers are
    // reacquired inside a complete outer update; none survive a Model callback.
    class Text
    {
        Context context;
        std::function<bool()> current;

        bool Valid() const
        {
            return current && current() && context.Current() && Context::ScratchReady() && context.FontReady();
        }

        bool Capacity(int slot, size_t count) const
        {
            if (!Valid() || slot < context.first || slot >= context.first + 30 || count > 256)
                return false;
            uint32_t used = 0;
            const auto slots = static_cast<const Native::TextSlot*>(context.descriptors);
            for (int i = 0; i < context.slotCount; ++i)
            {
                const auto value = Native::Read<Native::TextSlot>(slots + i, 0);
                if (value.start > context.glyphCapacity ||
                    uint32_t(value.count) > context.glyphCapacity - value.start)
                    return false;
                if (i != slot)
                    used += value.count;
            }
            // Even an empty setter enters native positioning and reads the
            // first glyph. Retain one valid sentinel beyond the new text.
            return used < context.glyphCapacity && count < context.glyphCapacity - used;
        }

        bool Encode(const std::string& ascii, std::string& encoded) const
        {
            if (!Valid())
                return false;
            auto font = context.FontReady();
            if (!Valid() || !font)
                return false;
            const auto count = Native::Read<uint32_t>(font, 12);
            const auto uv = Native::Read<void*>(font, 16);
            const auto width = Native::Read<uint16_t>(font, 20);
            const auto height = Native::Read<uint16_t>(font, 22);
            if (!count || count > 4096 || !width || !height || !uv ||
                !Native::Literal(ascii, count, encoded))
                return false;
            // In the font page of the page's own rows
            if (context.fontPage)
                encoded = "\\P" + std::to_string(context.fontPage) + "\\" + encoded;
            for (unsigned char c : ascii)
            {
                const auto index = aGlyphMap[c] - 32;
                const auto at = static_cast<const uint8_t*>(uv) + index * 16;
                const auto left = Native::Read<float>(at, 0), right = Native::Read<float>(at, 4);
                const auto top = Native::Read<float>(at, 8), bottom = Native::Read<float>(at, 12);
                if (!std::isfinite(left) || !std::isfinite(right) ||
                    !std::isfinite(top) || !std::isfinite(bottom) ||
                    left < 0 || right < 0 || top < 0 || bottom < 0 ||
                    left > 1 || right > 1 || top > 1 || bottom > 1)
                    return false;
                if (c != ' ' && (std::fabs(right - left) * width < 1 ||
                                std::fabs(bottom - top) * height < 12))
                    return false;
            }
            return Valid();
        }

    public:
        Text(const Context& owner, std::function<bool()> valid) : context(owner), current(std::move(valid)) {}

        static bool NativeText(const std::string& utf8, std::string& result)
        {
            if (utf8.size() > 8192) return false;
            if (utf8.empty()) { result.clear(); return true; }
            const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), nullptr, 0);
            if (!count) return false;
            std::wstring wide(size_t(count), L'\0');
            if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), int(utf8.size()), wide.data(), count)) return false;
            BOOL substituted = FALSE;
            result.resize(size_t(count));
            const int bytes = WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, wide.data(), count,
                result.data(), int(result.size()), nullptr, &substituted);
            if (!bytes || substituted) return false;
            result.resize(size_t(bytes));
            return true;
        }

        static std::vector<std::string> Tokens(const std::string& utf8)
        {
            std::string native;
            if (!NativeText(utf8, native)) return {};
            std::vector<std::string> result;
            for (size_t at = 0; at < utf8.size();)
            {
                size_t end = at + 1;
                while (end < utf8.size() && (static_cast<unsigned char>(utf8[end]) & 0xC0) == 0x80) ++end;
                result.push_back(utf8.substr(at, end - at));
                at = end;
            }
            return result;
        }

        bool Split(const std::string& utf8, float maxWidth, float maxHeight,
                   std::vector<std::string>& pieces)
        {
            pieces.clear();
            // Measured where the aligned text fits on screen; the extent doesn't depend on the position
            const auto align = context.style.flags & 3;
            const float measureX = Context::RightJustified(context.style.flags) || align == 2 ? maxWidth : align == 1 ? maxWidth / 2 : 0.0f;
            auto tokens = Tokens(utf8);
            if (tokens.empty() && !utf8.empty()) return false;
            if (tokens.empty()) { pieces.emplace_back(); return true; }
            for (size_t at = 0; at < tokens.size();)
            {
                size_t low = 0, high = std::min(size_t(256), tokens.size() - at);
                std::string best;
                while (low < high)
                {
                    const auto trial = low + (high - low + 1) / 2;
                    std::string candidate;
                    for (size_t i = 0; i < trial; ++i) candidate += tokens[at + i];
                    const bool fits = candidate.size() <= 256 &&
                        Put(context.first + 29, candidate, measureX, 0.0f,
                            maxWidth, maxHeight, 0xFFFFFFFF);
                    if (!Valid()) return false;
                    if (fits) { low = trial; best = std::move(candidate); }
                    else high = trial - 1;
                }
                if (!low || best.empty()) { Hide(context.first + 29); return false; }
                if (at + low < tokens.size())
                {
                    size_t word = low;
                    while (word > 0 && tokens[at + word - 1] != " ") --word;
                    if (word > 0)
                    {
                        low = word;
                        best.clear();
                        for (size_t i = 0; i < low; ++i) best += tokens[at + i];
                    }
                }
                pieces.push_back(std::move(best));
                if (pieces.size() > 2048) { Hide(context.first + 29); return false; }
                at += low;
            }
            return Hide(context.first + 29);
        }

        bool Put(int slot, const std::string& utf8, float x, float y,
                 float maxWidth, float maxHeight, uint32_t color, int64_t look = -1)
        {
            const uint32_t flags = look >= 0 ? uint32_t(look) : context.style.flags;
            std::string ascii, encoded;
            if (!NativeText(utf8, ascii) || !Capacity(slot, ascii.size()) || !Encode(ascii, encoded) ||
                !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(maxWidth) ||
                !std::isfinite(maxHeight) || x < 0 || y < 0 || maxWidth <= 0 ||
                maxHeight <= 0 || y + maxHeight > 1)
                return false;
            const auto align = flags & 3;
            const float left = Context::RightJustified(flags) || align == 2 ? x - maxWidth : align == 1 ? x - maxWidth / 2 : x;
            if (left < 0 || left + maxWidth > 1)
                return false;
            Native::ClearEffects(context.text, slot);
            if (!Valid()) return false;
            Native::SetText(context.text, slot, encoded.data());
            if (!Valid()) return false;
            const auto descriptor = Native::Read<Native::TextSlot>(
                static_cast<Native::TextSlot*>(context.descriptors) + slot, 0);
            if (descriptor.count != ascii.size() || descriptor.start >= context.glyphCapacity ||
                uint32_t(descriptor.count) > context.glyphCapacity - descriptor.start)
                return false;
            Native::Flags(context.text, slot, int16_t(flags));
            if (!Valid()) return false;
            const float size[3]{ -1.0f, 0.0f, 0.0f };
            Native::Size(context.text, slot, size);
            if (!Valid()) return false;
            float extent[3]{};
            if (!ascii.empty()) Native::Measure(context.text, slot, extent);
            if (!Valid() || !std::isfinite(extent[0]) || !std::isfinite(extent[1]) ||
                extent[0] < 0 || extent[1] < 0 || extent[0] > maxWidth || extent[1] > maxHeight)
                return false;
            if (!ascii.empty())
            {
                auto glyphs = static_cast<const Native::Glyph*>(context.glyphs) + descriptor.start;
                for (size_t i = 0; i < ascii.size(); ++i)
                {
                    const auto glyph = Native::Read<Native::Glyph>(glyphs + i, 0);
                    if (((glyph.symbol >> 12) & 63) != context.fontPage || (glyph.symbol & 0xFFF) < 32 ||
                        (glyph.symbol & 0xFFF) > 255 || glyph.x < 0 || glyph.y < 0 ||
                        uint32_t(glyph.x) + glyph.width > uint32_t(context.width * maxWidth) ||
                        uint32_t(glyph.y) + glyph.height > uint32_t(context.height * maxHeight))
                        return false;
                }
            }
            const float position[3]{ x, y, context.style.z };
            Native::Move(context.text, slot, position);
            if (!Valid()) return false;
            Native::Color(context.text, slot, color);
            return Valid();
        }

        bool Hide(int slot)
        {
            if (!Capacity(slot, 0)) return false;
            Native::ClearEffects(context.text, slot);
            if (!Valid()) return false;
            char empty[] = "";
            Native::SetText(context.text, slot, empty);
            if (!Valid()) return false;
            Native::Flags(context.text, slot, 0);
            return Valid();
        }
    };
}

namespace KongMenu
{
    enum class Kind { Link, Row, Info, Back };
    struct Item
    {
        Kind kind = Kind::Back;
        std::string text;
        RWGTAMenu::Row row{};
        size_t target = 0;
        bool enabled = true;
    };

    // The plugins list, a plugin's settings or categories, a category's settings, or the selected setting's details,
    // as the five items of the Options page. A row too long for the page continues on the next lines. Each list is one
    // list, Back last, that scrolls by a line through the five items (physical: the first line shown).
    struct View
    {
        static constexpr size_t Slots = 5;
        int page = int(RWGTAMenu::Model::RootPage);
        size_t physical = 0;
        size_t total = 0;                   // lines of the whole list
        bool details = false;
        size_t originPhysical = 0;
        RWGTAMenu::Row detailRow{};
        std::string notice;
        std::string title;
        std::vector<Item> items;
        std::map<int, std::pair<size_t, size_t>> kept;  // the first line and item a list was left on, by page

        bool Above() const { return physical > 0; }
        bool Below() const { return physical + items.size() < total; }

        bool Prepare(RWGTAMenu::Model& model, Text& font, bool pause,
                     const std::function<bool()>& current, float width, HMODULE hostModule)
        {
            std::vector<Item> content;
            auto pageTitle = [&](const RWGTAMenu::Page& source)
            {
                if (!source.plugin || source.plugin->module != hostModule) return source.title;
                const auto name = OtherPlugins::Name(*source.plugin);
                return source.title.rfind(name, 0) == 0 ?
                    std::string("King Kong") + source.title.substr(name.size()) : source.title;
            };
            auto append = [&](Item item, const std::string& source)
            {
                std::vector<std::string> pieces;
                if (!font.Split(source, width, 0.065f, pieces) || !current()) return false;
                for (auto& piece : pieces)
                {
                    item.text = std::move(piece);
                    content.push_back(item);
                    if (content.size() > 2048) return false;
                }
                return true;
            };
            if (!current() || !model.IsOpen() || model.Pages().empty() || page < 0 || size_t(page) >= model.Pages().size()) return false;
            if (details)
            {
                title = model.Text(notice.empty() ? "menu.details" : "menu.updatetitle");
                const auto help = notice.empty() ? model.Help(detailRow) : notice;
                if (!current() || !append(Item{ Kind::Info, {}, {}, 0, false }, help)) return false;
            }
            else
            {
                const auto& source = model.Pages()[size_t(page)];
                title = size_t(page) == RWGTAMenu::Model::RootPage ? model.Text("menu.title") : pageTitle(source);
                auto rows = model.VisibleRows(size_t(page));
                if (!current()) return false;
                // Copy before any further plugin call can rebuild Model pages.
                std::vector<RWGTAMenu::Row> copied;
                for (auto row : rows) copied.push_back(*row);
                for (auto row : copied)
                {
                    if (RWGTAMenu::Model::Opens(row))
                    {
                        if (row.target >= model.Pages().size()) return false;
                        if (!append(Item{ Kind::Link, {}, row, row.target, true }, pageTitle(model.Pages()[row.target]))) return false;
                        continue;
                    }
                    auto label = model.Label(row);
                    if (!current()) return false;
                    auto value = model.Value(row);
                    if (!current()) return false;
                    auto enabled = model.Enabled(row, pause);
                    if (!current()) return false;
                    if (!value.empty()) label += ": " + value;
                    if (!enabled) label += " " + model.Text("menu.unavailable");
                    if (!append(Item{ Kind::Row, {}, row, 0, enabled }, label)) return false;
                }
            }
            if (!current()) return false;
            content.push_back(Item{ Kind::Back, model.Text("menu.back"), {}, 0, true });
            total = content.size();
            physical = std::min(physical, total > Slots ? total - Slots : 0);
            items.assign(content.begin() + physical, content.begin() + std::min(total, physical + Slots));
            // Title itself also uses complete physical lines, never truncation.
            std::vector<std::string> headings;
            if (!font.Split(title, width, 0.065f, headings) || headings.size() != 1 || !current()) return false;
            title = std::move(headings[0]);
            return !items.empty() && items.size() <= Slots;
        }
    };
}

namespace KongMenu
{
    struct Host : std::enable_shared_from_this<Host>
    {
        Context context;
        HMODULE module = nullptr;
        RWGTAMenu::Model model;
        View view;
        uint64_t generation = 1, serial = 1;
        bool pause = true, invalid = false, open = false;
        bool busy = false, opening = false, ready = false, entryReady = false, restore = false, repaint = true;
        size_t selected = 0;
        size_t originSelected = 0;          // the item a setting's details were opened from
        std::map<std::pair<HMODULE, std::string>, std::string> restartBaseline;

        struct Pending
        {
            uint64_t generation, serial;
            int page;
            size_t physical, selected;
            Item item;
            int direction;
            bool help;
        };
        std::deque<Pending> pending;

        struct Foreign
        {
            Host& host;
            bool previous;
            explicit Foreign(Host& owner) : host(owner), previous(owner.busy) { host.busy = true; }
            ~Foreign() { host.busy = previous; }
        };

        template<class T> static void Write(void* object, size_t offset, T value)
        {
            injector::WriteMemory<T>(static_cast<uint8_t*>(object) + offset, value, true);
        }

        bool Current() const { return !invalid && context.Current(); }
        // Native text extends from x by its alignment: 0 left, 1 centered, 2 right
        int Align() const { return int(context.style.flags & 3); }
        float Width() const
        {
            const float x = context.style.x;
            if (Context::RightJustified(context.style.flags))
                return std::min(x - 0.05f, 0.6f);
            switch (Align())
            {
                case 1: return std::min(2.0f * std::min(x, 1.0f - x) - 0.05f, 0.6f);
                case 2: return std::min(x - 0.05f, 0.5f);
                default: return std::min(1.0f - x - 0.025f, 0.5f);
            }
        }
        float Left() const { return Context::RightJustified(context.style.flags) || Align() == 2 ? context.style.x - Width() : Align() == 1 ? context.style.x - Width() / 2 : context.style.x; }
        float Right() const { return Left() + Width(); }

        void Retire()
        {
            if (invalid) return;
            invalid = true;
            ++generation;
            ready = entryReady = open = false;
            pending.clear();
            // Outstanding model reads survive Close, and presets abort
            // immediately. No native resource or UI is accessed here.
            model.Close();
        }

        void Cancel() noexcept
        {
            ++generation;
            ready = false;
            restore = !invalid;
            pending.clear();
            model.Close();
        }

        std::function<bool()> Snapshot(bool privatePage)
        {
            auto lease = shared_from_this();
            auto version = generation;
            auto rows = serial;
            const auto focus = Native::Read<int>(context.parameters, 60);
            return [lease, version, rows, privatePage, focus]
            {
                return lease->generation == version && lease->serial == rows &&
                    lease->Current() && (!privatePage || lease->open) &&
                    (!privatePage || Native::Read<int>(lease->context.parameters, 60) == focus);
            };
        }

        bool RestartRequired(const std::function<bool()>& current)
        {
            Foreign foreign(*this);
            const auto pages = model.Pages();
            for (const auto& page : pages)
            {
                auto rows = page.rows;
                for (const auto& row : rows)
                {
                    if (!row.plugin || !row.setting || row.setting->apply != "restart") continue;
                    auto key = std::make_pair(row.plugin->module, row.setting->id);
                    const auto initial = restartBaseline.find(key);
                    if (initial == restartBaseline.end()) continue;
                    const auto value = OtherPlugins::Get(*row.plugin, *row.setting);
                    if (!current()) return false;
                    if (!asi_settings::text::IEquals(initial->second, value)) return true;
                }
            }
            return false;
        }

        bool Paint(const View& candidate, size_t focus, const std::function<bool()>& current)
        {
            if (!current() || candidate.items.empty() || candidate.items.size() > 5 ||
                Width() <= 0 || focus >= candidate.items.size()) return false;
            Text text(context, current);
            // Clear only this Options page's verified thirty-slot range. The
            // cursor is a separate native object managed by the original VM.
            for (int i = 0; i < 29; ++i)
                if (!text.Hide(context.first + i)) return false;
            // The title where the Options page has its own, in capitals like it
            const auto& title = context.titleStyle;
            const float titleWidth = Context::RightJustified(title.flags) ? std::min(title.x - 0.05f, 0.6f) : std::min(1.0f - title.x - 0.025f, 0.6f);
            if (!text.Put(context.first, Upper(candidate.title), title.x, title.y,
                          titleWidth, 0.065f, context.titleColor, title.flags)) return false;
            for (size_t i = 0; i < candidate.items.size(); ++i)
            {
                const auto color = !candidate.items[i].enabled ? 0xFF606060 :
                    (i == focus ? 0xFFFFFFFF : 0xFFB0B0B0);
                // Plugins and categories in capitals like the Options page's entries; settings as the Video page's
                const auto& item = candidate.items[i];
                if (!text.Put(context.first + 5 + int(i), item.kind == Kind::Link ? Upper(item.text) : item.text,
                              context.style.x, 0.40f + 0.08f * float(i),
                              Width(), 0.065f, color)) return false;
            }
            // Lines above or below the five shown: the font's dots right of the first and last item (the page's
            // font has no arrows)
            const float markX = std::min(context.style.x + 0.035f, 0.97f);
            if (candidate.Above() && !text.Put(context.first + 1, "...", markX, 0.40f, 0.03f, 0.065f, 0xFFB0B0B0))
                return false;
            if (candidate.Below() && !text.Put(context.first + 2, "...", markX,
                0.40f + 0.08f * float(candidate.items.size() - 1), 0.03f, 0.065f, 0xFFB0B0B0))
                return false;
            if (!candidate.details && std::any_of(candidate.items.begin(), candidate.items.end(), [](const Item& item) { return item.kind == Kind::Row; }) &&
                !text.Put(context.first + 11, model.Text("menu.detailskey"), context.style.x, 0.80f,
                    Width(), 0.065f, 0xFFB0B0B0)) return false;
            if (RestartRequired(current))
            {
                std::vector<std::string> lines;
                if (!text.Split(model.Text("menu.restartwarning"), Width(), 0.065f, lines) || lines.size() > 2)
                    return false;
                for (size_t i = 0; i < lines.size(); ++i)
                    if (!text.Put(context.first + 12 + int(i), lines[i], context.style.x,
                        0.87f + 0.06f * float(i), Width(), 0.065f, 0xFFFFFFFF)) return false;
            }
            return current();
        }

        void Close()
        {
            auto lease = shared_from_this();
            const bool owned = Current();
            ++generation;
            ++serial;
            open = ready = false;
            pending.clear();
            model.Close();
            restore = !owned && context.Alive();
            view = {};
            selected = 0;
            if (owned && Current())
            {
                // Native Options will recreate every stock label on its next
                // call and preserve its normal title/pause return dispatch.
                Write<uint32_t>(context.parameters, 480, 1);
                Write<int>(context.parameters, 60, 4);
                repaint = true;
            }
        }

        bool Populate(View candidate, size_t focus)
        {
            auto lease = shared_from_this();
            if (!Current() || !open) return false;
            ready = false;
            auto current = Snapshot(true);
            Text text(context, current);
            {
                Foreign foreign(*this);
                if (!candidate.Prepare(model, text, pause, current, Width(), module) || !current()) return false;
            }
            focus = std::min(focus, candidate.items.size() - 1);
            if (candidate.items[focus].kind == Kind::Info)
                for (size_t i = 0; i < candidate.items.size(); ++i)
                    if (candidate.items[i].kind != Kind::Info) { focus = i; break; }
            if (!Paint(candidate, focus, current) || !current()) return false;
            // View mappings and widget contents publish together. Advancing
            // the serial discards all input copied from the previous rows.
            view = std::move(candidate);
            selected = focus;
            Write<int>(context.parameters, 60, int(selected));
            ++serial;
            pending.clear();
            ready = true;
            repaint = false;
            return true;
        }

        bool Open(const std::string& language)
        {
            auto lease = shared_from_this();
            if (!Current() || open || !entryReady ||
                Native::Read<int>(context.parameters, 60) != 4) return false;
            nMenuWheel.exchange(0);
            ++generation;
            open = true;
            ready = false;
            struct Construction
            {
                Host& host;
                bool previous;
                explicit Construction(Host& owner) : host(owner), previous(owner.opening) { host.opening = true; }
                ~Construction() { host.opening = previous; }
            } construction(*this);
            view = {};
            selected = 0;
            auto current = Snapshot(true);
            {
                Foreign foreign(*this);
                model.OpenMenu(module, language, View::Slots);
                auto pages = model.Pages();
                for (const auto& page : pages)
                    for (const auto& row : page.rows)
                    {
                        if (!row.plugin || !row.setting || row.setting->apply != "restart") continue;
                        const auto key = std::make_pair(row.plugin->module, row.setting->id);
                        if (restartBaseline.contains(key)) continue;
                        const auto value = OtherPlugins::Get(*row.plugin, *row.setting);
                        if (!current()) return false;
                        restartBaseline.emplace(key, value);
                    }
            }
            if (!current() || Native::Read<int>(context.parameters, 60) != 4 ||
                !Populate(view, 0))
            {
                if (!invalid) Close();
                return false;
            }
            return true;
        }

        bool Queue(int direction, bool back = false, bool help = false)
        {
            auto lease = shared_from_this();
            if (!Current() || !open || (!ready && !busy) || pending.size() >= 32) return false;
            if (opening && busy && back && direction == 0)
            {
                if (Native::Read<int>(context.parameters, 60) != 4) return false;
                Cancel();
                return true;
            }
            if (Native::Read<int>(context.parameters, 60) != int(selected)) return false;
            if (!back && direction == 0 && selected < view.items.size() &&
                view.items[selected].kind == Kind::Back) back = true;
            if (back && busy)
            {
                // An accepted native Back cancels a foreign preset/read burst
                // immediately; restoring native labels waits for full unwind.
                ++generation;
                ready = false;
                restore = true;
                pending.clear();
                model.Close();
                return true;
            }
            if (!ready || selected >= view.items.size()) return false;
            auto item = view.items[selected];
            if (back) item = Item{ Kind::Back, {}, {}, 0, true };
            pending.push_back(Pending{ generation, serial, view.page, view.physical,
                                       selected, std::move(item), direction, help });
            return true;
        }

        bool Activate(const Pending& event)
        {
            auto lease = shared_from_this();
            if (!Current() || !open || !ready || event.generation != generation ||
                event.serial != serial || event.page != view.page ||
                event.physical != view.physical || event.selected != selected) return false;
            auto current = Snapshot(true);
            if (event.item.kind == Kind::Row)
            {
                Foreign foreign(*this);
                const auto rows = model.VisibleRows(size_t(view.page));
                if (!current()) return false;
                const auto visible = std::any_of(rows.begin(), rows.end(), [&](const auto* row)
                {
                    return row->kind == event.item.row.kind && row->plugin == event.item.row.plugin &&
                        row->setting == event.item.row.setting;
                });
                if (!visible)
                {
                    repaint = true;
                    return true;
                }
            }
            View candidate = view;
            size_t focus = selected;
            if ((event.help || (event.direction == 0 && event.item.row.kind == RWGTAMenu::RowKind::Setting)) &&
                event.item.kind == Kind::Row)
            {
                candidate.details = true;
                candidate.notice.clear();
                candidate.detailRow = event.item.row;
                candidate.originPhysical = candidate.physical;
                originSelected = selected;
                candidate.physical = 0;
                focus = 0;
            }
            else switch (event.item.kind)
            {
                case Kind::Info:
                    return true;
                case Kind::Link:
                {
                    if (event.direction) return true;
                    size_t target = model.Enter(event.item.row);
                    if (target >= model.Pages().size()) return false;
                    candidate.kept[candidate.page] = { candidate.physical, selected };
                    candidate.page = int(target);
                    candidate.physical = 0;
                    focus = 0;
                    break;
                }
                case Kind::Back:
                {
                    if (event.direction) return true;
                    if (candidate.details)
                    {
                        candidate.details = false;
                        candidate.notice.clear();
                        candidate.physical = candidate.originPhysical;
                        focus = originSelected;
                        break;
                    }
                    size_t parent = model.Back(size_t(candidate.page));
                    if (parent == SIZE_MAX) { Close(); return true; }
                    candidate.page = int(parent);
                    const auto kept = candidate.kept.find(candidate.page);
                    candidate.physical = kept != candidate.kept.end() ? kept->second.first : 0;
                    focus = kept != candidate.kept.end() ? kept->second.second : 0;
                    break;
                }
                case Kind::Row:
                {
                    Foreign foreign(*this);
                    if (!model.Enabled(event.item.row, pause) || !current()) return current();
                    model.Change(event.item.row, event.direction, pause);
                    if (!current()) return false;
                    break;
                }
            }
            return current() && Populate(std::move(candidate), focus);
        }

        void Frame()
        {
            auto lease = shared_from_this();
            if (invalid) return;
            if (!context.Alive()) { Retire(); return; }
            if (!Current())
            {
                if (open) Close();
                entryReady = false;
                return;
            }
            if (restore) { Close(); return; }
            if (open)
            {
                while (!pending.empty() && Current() && open)
                {
                    auto event = std::move(pending.front());
                    pending.pop_front();
                    if (!Activate(event))
                    {
                        if (open && Current()) Close();
                        return;
                    }
                }
                if (Current() && open && (repaint || RWGTAMenu::Model::Changed().exchange(false)))
                    if (!Populate(view, selected) && Current()) Close();
            }
        }
    };
}

namespace KongMenu
{
    std::shared_ptr<Host> host;
    HMODULE module = nullptr;
    void* world = nullptr;
    unsigned outerDepth = 0, worldDepth = 0, optionDepth = 0;
    bool draining = false;
    std::atomic<bool> installed = false;
    std::atomic<bool> attempted = false;
    uint32_t inputMask = 0;
    SafetyHookInline wholeHook, worldHook, optionsHook, selectHook, mouseHook;
    SafetyHookInline titleHook, pauseHook, temporaryHook, objectHook, behaviorHook, languageHook, subtitleHook;

    struct Origin
    {
        void* world = nullptr;
        void* owner = nullptr;
        void* behavior = nullptr;
        void* parameters = nullptr;
        void* target = nullptr;
        bool pause = true;
    };
    Origin origin;
    std::map<std::pair<HMODULE, std::string>, std::string> startupRestart;
    std::weak_ptr<Host> requestedOpen;
    uint64_t requestedGeneration = 0;

    struct Depth
    {
        unsigned& value;
        explicit Depth(unsigned& counter) : value(counter) { ++value; }
        ~Depth() { --value; }
    };

    void Log(const char* message) noexcept
    {
        try
        {
            static std::set<std::string> emitted;
            if (emitted.size() >= 24 || !emitted.emplace(message).second) return;
            OutputDebugStringA(("King Kong native menu: " + std::string(message) + "\n").c_str());
        }
        catch (...) {}
    }

    void Cancel(const char* reason) noexcept
    {
        auto lease = host;
        if (lease) lease->Cancel();
        requestedOpen.reset();
        Log(reason);
    }

    bool Global(const std::shared_ptr<Host>& owner)
    {
        return owner && owner == host && owner->Current() &&
            pCurrentBehavior == owner->context.behavior &&
            pCurrentMenuOwner == owner->context.owner &&
            nMenuState == 3;
    }

    bool Once(uint32_t bit)
    {
        if (inputMask & bit) return false;
        inputMask |= bit;
        return true;
    }

    bool BackPressed() { return Native::Pressed(3) || Native::Key(259) || Native::Key(2); }
    bool AcceptPressed() { return Native::Pressed(0) || Native::Pressed(7); }

    std::string MenuLanguage()
    {
        auto chosen = FixSettings().Get("MAIN.TextLanguage");
        for (auto tag : { "en", "fr", "de", "it", "es" })
            if (chosen == tag) return chosen;
        // 8FD260 returns this exact native locale field. Only the established
        // English enum has a known tag; other enums use the resource fallback.
        if (!Screen.bSignatureEdition && nLocale != 1)
            Log("native locale tag unproved; using English menu resources");
        return "en";
    }

    void Invalidate(void* object, void* behavior = nullptr)
    {
        auto lease = host;
        if (lease && (object == lease->context.owner || object == lease->context.text ||
                      behavior == lease->context.behavior)) lease->Retire();
        if (object == origin.owner || behavior == origin.behavior) origin = {};
    }

    int OriginCall(SafetyHookInline& hook, bool pause)
    {
        if (!installed) return hook.unsafe_ccall<int>();
        void* owner = pCurrentMenuOwner;
        void* behavior = pCurrentBehavior;
        auto parameters = Native::Read<void*>(behavior, 64);
        void* target = pOptionsTarget;
        const bool eligible = world && parameters && target && Native::Read<uintptr_t>(target, 20) == Native::Address().options &&
            nMenuState == 3;
        auto result = hook.unsafe_ccall<int>();
        if (eligible && pCurrentMenuOwner == owner &&
            pCurrentBehavior == behavior &&
            Native::Read<void*>(behavior, 64) == parameters &&
            Native::Read<void*>(behavior, 32) == target)
            origin = Origin{ world, owner, behavior, parameters, target, pause };
        return result;
    }

    int __cdecl Title() { return OriginCall(titleHook, false); }
    int __cdecl Pause() { return OriginCall(pauseHook, true); }

    uintptr_t __cdecl Select(int count, int wrap)
    {
        auto lease = host;
        if (installed && _ReturnAddress() == reinterpret_cast<void*>(Native::Address().selectReturn) &&
            optionDepth && count == 4 && wrap == 1 && Global(lease) &&
            !lease->open && lease->entryReady)
            count = 5;
        return selectHook.unsafe_ccall<uintptr_t>(count, wrap);
    }

    void __cdecl Mouse(int style)
    {
        auto caller = _ReturnAddress();
        mouseHook.unsafe_ccall(style);
        auto lease = host;
        if (installed && caller == reinterpret_cast<void*>(Native::Address().mouseReturn) && optionDepth && Global(lease) &&
            !lease->open && lease->entryReady)
        {
            const auto y = Native::Read<float>(lease->context.parameters, 472);
            const auto x = Native::Read<float>(lease->context.parameters, 468);
            if ((style == 5 || style == 6) && std::isfinite(x) && std::isfinite(y) &&
                x > lease->Left() && x < lease->Right() &&
                y > 0.68f && y < 0.76f)
                Host::Write<int>(lease->context.parameters, 60, 4);
        }
    }

    bool backHeld = false, acceptHeld = false;

    void PrivateInput(const std::shared_ptr<Host>& lease)
    {
        if (!Global(lease) || !lease->open) return;
        // These globals-shaped common helpers run only in the real native
        // Options callback. Deferred publication never invokes either helper.
        const auto common = reinterpret_cast<int (__cdecl*)()>(Native::Address().common)();
        reinterpret_cast<int (__cdecl*)()>(Native::Address().commonInput)();
        if (!common || !Global(lease)) return;
        // Back and accept once per press (the game reports them while they are held)
        const bool backDown = BackPressed(), back = backDown && !backHeld;
        backHeld = backDown;
        if (back && Once(1)) { lease->Queue(0, true); return; }
        const bool acceptDown = AcceptPressed(), accept = acceptDown && !acceptHeld;
        acceptHeld = acceptDown;
        static bool previousHelp = false;
        const bool help = GetForegroundWindow() == GetActiveWindow() && (GetAsyncKeyState(VK_F1) & 0x8000);
        const bool showHelp = help && !previousHelp;
        previousHelp = help;
        if (showHelp && Once(64)) { lease->Queue(0, false, true); return; }
        if (lease->busy)
        {
            if (lease->selected < lease->view.items.size() &&
                Native::Read<int>(lease->context.parameters, 60) == int(lease->selected) &&
                AcceptPressed() && lease->view.items[lease->selected].kind == Kind::Back && Once(2))
                lease->Queue(0, true);
            return;
        }
        if (!lease->ready || lease->selected >= lease->view.items.size() ||
            Native::Read<int>(lease->context.parameters, 60) != int(lease->selected)) return;
        const auto count = lease->view.items.size();
        auto previous = lease->selected;
        const auto previousFirst = lease->view.physical;
        // Up/Down: the next item that can be chosen, the list scrolling a line at the first or last item and wrapping
        // at its ends; Page Up/Page Down five lines, Home/End the ends
        auto move = [&](int direction)
        {
            auto& view = lease->view;
            const size_t total = view.total;
            size_t line = view.physical + lease->selected;
            if (!view.details)
                line = (line + total + size_t(direction)) % total;
            else
                line = std::clamp<size_t>(line + size_t(direction), 0, total ? total - 1 : 0);
            if (line < view.physical) view.physical = line;
            else if (line >= view.physical + View::Slots) view.physical = line - View::Slots + 1;
            lease->selected = line - view.physical;
        };
        // Held Up/Down (keys, d-pad, stick) repeat; the page takes the first press itself
        static MenuList::Repeat downHeld, upHeld;
        static MenuList::StickAxis stick;
        const bool front = GetForegroundWindow() == GetActiveWindow();
        XINPUT_STATE pad{};
        DWORD padSlot = 0;
        const bool padRead = front && PadInput::Read(pad, padSlot);
        const int stickY = padRead ? stick.Direction(pad.Gamepad.sThumbLY) : 0;
        const WORD buttons = padRead ? pad.Gamepad.wButtons : 0;
        const auto now = GetTickCount64();
        const bool downRepeat = downHeld.Step(front && ((GetAsyncKeyState(VK_DOWN) & 0x8000) || (buttons & XINPUT_GAMEPAD_DPAD_DOWN) || stickY < 0), now, false);
        const bool upRepeat = upHeld.Step(front && ((GetAsyncKeyState(VK_UP) & 0x8000) || (buttons & XINPUT_GAMEPAD_DPAD_UP) || stickY > 0), now, false);
        if ((Native::Read<int>(lease->context.parameters, 76) == 1 && Once(4)) || downRepeat)
        {
            if (lease->view.details && lease->view.Below()) ++lease->view.physical;
            else if (!lease->view.details) move(1);
        }
        if ((Native::Read<int>(lease->context.parameters, 80) == 1 && Once(8)) || upRepeat)
        {
            if (lease->view.details && lease->view.Above()) --lease->view.physical;
            else if (!lease->view.details) move(-1);
        }
        // The wheel scrolls the list a line per notch; the selection stays on its line while it is shown
        static MenuList::WheelNotches wheel;
        for (int notches = wheel.Take(nMenuWheel.exchange(0)); notches; notches += notches > 0 ? -1 : 1)
        {
            auto& view = lease->view;
            const size_t line = view.physical + lease->selected;
            const size_t last = view.total > View::Slots ? view.total - View::Slots : 0;
            if (notches > 0 && view.physical > 0) --view.physical;
            else if (notches < 0 && view.physical < last) ++view.physical;
            else continue;
            lease->selected = std::clamp(line, view.physical, view.physical + View::Slots - 1) - view.physical;
        }
        static bool pageUpHeld = false, pageDownHeld = false, homeHeld = false, endHeld = false;
        auto key = [](int vk) { return GetForegroundWindow() == GetActiveWindow() && (GetAsyncKeyState(vk) & 0x8000) != 0; };
        const bool pageUp = key(VK_PRIOR), pageDown = key(VK_NEXT), home = key(VK_HOME), end = key(VK_END);
        if (!lease->view.details)
        {
            for (int i = 0; pageUp && !pageUpHeld && i < int(View::Slots) - 1 && lease->view.physical + lease->selected > 0; ++i) move(-1);
            for (int i = 0; pageDown && !pageDownHeld && i < int(View::Slots) - 1 &&
                lease->view.physical + lease->selected + 1 < lease->view.total; ++i) move(1);
            while (home && !homeHeld && lease->view.physical + lease->selected > 0) move(-1);
            while (end && !endHeld && lease->view.physical + lease->selected + 1 < lease->view.total) move(1);
        }
        pageUpHeld = pageUp; pageDownHeld = pageDown; homeHeld = home; endHeld = end;
        if (lease->view.physical != previousFirst)
        {
            // The list scrolled: its items are painted again with the selection in its slot
            lease->repaint = true;
            Host::Write<int>(lease->context.parameters, 60, int(lease->selected));
            return;
        }
        const auto x = Native::Read<float>(lease->context.parameters, 468);
        const auto y = Native::Read<float>(lease->context.parameters, 472);
        const auto dx = Native::Read<float>(lease->context.parameters, 484);
        const auto dy = Native::Read<float>(lease->context.parameters, 488);
        if (std::isfinite(x) && std::isfinite(y) && std::isfinite(dx) && std::isfinite(dy) &&
            (dx != 0 || dy != 0 || Native::Pressed(7)) &&
            x > lease->Left() && x < lease->Right())
            for (size_t i = 0; i < count; ++i)
                if (lease->view.items[i].kind != Kind::Info && y > 0.35f + 0.08f * float(i) && y < 0.44f + 0.08f * float(i)) lease->selected = i;
        if (previous != lease->selected)
        {
            Host::Write<int>(lease->context.parameters, 60, int(lease->selected));
            lease->repaint = true;
        }
        if (Native::Read<int>(lease->context.parameters, 84) == 1 && Once(16)) lease->Queue(-1);
        else if (Native::Read<int>(lease->context.parameters, 88) == 1 && Once(32)) lease->Queue(1);
        else if (accept && Once(2)) lease->Queue(0);
    }

    int __cdecl Options()
    {
        if (!installed) return optionsHook.unsafe_ccall<int>();
        auto lease = host;
        try
        {
        if (lease && lease->restore && lease->Current()) lease->Close();
        if (lease && lease->open && Global(lease))
        {
            PrivateInput(lease);
            return 0;
        }
        if (lease && lease->open && !lease->Current()) lease->Close();
        }
        catch (...)
        {
            Cancel("private input allocation failed; deferred stock restoration requested");
            if (lease && lease->open) return 0;
        }
        int result;
        {
            Depth depth(optionDepth);
            result = optionsHook.unsafe_ccall<int>();
        }
        try
        {
        Context context;
        if (!world || !Context::Capture(world, context))
        {
            Log("Options source context/range/committed owner unavailable");
            return result;
        }
        if (!host || host->invalid || host->context.owner != context.owner ||
            host->context.behavior != context.behavior || host->context.parameters != context.parameters ||
            host->context.text != context.text || host->context.model != context.model ||
            host->context.first != context.first)
        {
            if (host) host->Retire();
            host = std::make_shared<Host>();
            host->context = context;
            host->module = module;
            host->restartBaseline = startupRestart;
            host->pause = !(origin.world == context.world && origin.owner == context.owner &&
                origin.behavior == context.behavior && origin.parameters == context.parameters &&
                origin.target == context.page && !origin.pause);
        }
        lease = host;
        lease->repaint = true;
        if (lease->entryReady && Global(lease) && Native::Read<int>(context.parameters, 60) == 4 &&
            !Native::Read<int>(context.parameters, 516) && !BackPressed() && AcceptPressed() && Once(2))
        {
            requestedOpen = lease;
            requestedGeneration = lease->generation;
            // The press that opens the list doesn't also choose its first row
            acceptHeld = true;
        }
        return result;
        }
        catch (...)
        {
            Cancel("Options observation allocation failed; stock result preserved");
            return result;
        }
    }

    void Service()
    {
        auto lease = host;
        if (!lease || lease->invalid) return;
        auto request = requestedOpen.lock();
        requestedOpen.reset();
        if (request == lease && requestedGeneration == lease->generation && lease->Current())
        {
            // Read TextLanguage on reopen; unknown native locale enums use
            // the English resource fallback.
            if (!lease->Open(MenuLanguage())) Log("private Open discovery/font/layout transaction refused; stock retained");
        }
        lease->Frame();
        RWGTA::UpdateCheck::Tick();
        if (!lease->Current() || lease->open || lease->invalid) return;
        auto current = lease->Snapshot(false);
        Text text(lease->context, current);
        auto texts = FixSettings().GetTexts();
        texts.Select(MenuLanguage());
        const auto label = Upper(texts.Resolve("@menu.title"));
        const auto color = Native::Read<int>(lease->context.parameters, 60) == 4 ? 0xFFFFFFFF : 0xFF606060;
        if (text.Put(lease->context.first + 9, label, lease->context.style.x, 0.72f,
                     lease->Width(), 0.065f, color))
        {
            lease->entryReady = true;
        }
        else
        {
            lease->entryReady = false;
            if (current())
            {
                text.Hide(lease->context.first + 9);
                if (Native::Read<int>(lease->context.parameters, 60) == 4)
                    Host::Write<int>(lease->context.parameters, 60, 3);
            }
            Log("category native glyph/font/scratch/capacity/layout refused");
        }
    }

    void __cdecl Whole()
    {
        if (!installed) { wholeHook.unsafe_ccall(); return; }
        const bool outer = outerDepth == 0;
        if (outer && !draining) inputMask = 0;
        {
            Depth depth(outerDepth);
            wholeHook.unsafe_ccall();
        }
        if (!outerDepth && !worldDepth && !optionDepth && !draining)
        {
            struct Drain { Drain() { draining = true; } ~Drain() { draining = false; } } drain;
            try { Service(); }
            catch (...) { Cancel("page transaction allocation failed; deferred stock restoration requested"); }
        }
    }

    uint32_t __cdecl World(void* value)
    {
        if (!installed) return worldHook.unsafe_ccall<uint32_t>(value);
        struct Scope
        {
            void* previous = world;
            Depth depth{ worldDepth };
            explicit Scope(void* next) { world = next; }
            ~Scope() { world = previous; }
        } scope(value);
        return worldHook.unsafe_ccall<uint32_t>(value);
    }

    void __cdecl Temporary(void* value, char mode)
    {
        if (installed) Invalidate(value);
        temporaryHook.unsafe_ccall(value, mode);
    }
    void __cdecl Object(void* value, char mode)
    {
        if (installed) Invalidate(value);
        objectHook.unsafe_ccall(value, mode);
    }
    void __cdecl Behavior(void* value)
    {
        if (installed) Invalidate(nullptr, value);
        behaviorHook.unsafe_ccall(value);
    }
    int __cdecl Language(unsigned value)
    {
        if (installed) { if (host) host->Retire(); origin = {}; }
        return languageHook.unsafe_ccall<int>(value);
    }
    int __cdecl Subtitle(unsigned value)
    {
        if (installed) { if (host) host->Retire(); origin = {}; }
        return subtitleHook.unsafe_ccall<int>(value);
    }

    void Rollback()
    {
        installed = false;
        if (host) host->Retire();
        host.reset(); requestedOpen.reset(); origin = {};
        // Restore targets but retain original trampolines for any forwarding
        // callback already on-stack. The supported module is process-pinned.
        (void)subtitleHook.disable(); (void)languageHook.disable(); (void)behaviorHook.disable();
        (void)objectHook.disable(); (void)temporaryHook.disable(); (void)pauseHook.disable();
        (void)titleHook.disable(); (void)mouseHook.disable(); (void)selectHook.disable();
        (void)optionsHook.disable(); (void)worldHook.disable(); (void)wholeHook.disable();
    }

    void WINAPI Exit(UINT code)
    {
        RWGTA::UpdateCheck::OnExit();
        ExitProcess(code);
    }

    void InitUpdates()
    {
        FixSettings().Availability("MAIN.CheckForUpdates", [](std::string_view value)
        {
            return value == "0" || Screen.bSignatureEdition || installed.load();
        });
        if (!Screen.bSignatureEdition && !installed) return;
        IATHook::Replace(GetModuleHandleW(nullptr), "KERNEL32.dll", std::forward_as_tuple("ExitProcess", Exit));
        RWGTA::UpdateCheck::Config config;
        config.pluginId = "KingKong.FusionFix";
        config.fileVersion = rsc_FileVersion;
        config.settings = &FixSettings();
        config.format = [](const std::string& latest, bool hub)
        {
            auto texts = FixSettings().GetTexts();
            texts.Select(MenuLanguage());
            auto message = texts.Resolve(hub ? "@update.availablehub" : "@update.availablesite");
            for (const auto& [key, value] : { std::pair<std::string, std::string>{ "{name}", "King Kong" }, { "{version}", latest } })
                if (const auto at = message.find(key); at != std::string::npos) message.replace(at, key.size(), value);
            return asi_settings::text::Widen(message);
        };
        config.playable = []()
        {
            if (Screen.bSignatureEdition)
            {
                const auto window = WindowedModeWrapper::GameHWND;
                return IsWindowVisible(window) && GetForegroundWindow() == window;
            }
            return host && host->Current() && host->open && host->ready && !host->view.details;
        };
        config.show = [](const std::wstring& message)
        {
            if (Screen.bSignatureEdition)
            {
                const auto window = WindowedModeWrapper::GameHWND;
                auto texts = FixSettings().GetTexts();
                texts.Select(MenuLanguage());
                const auto title = asi_settings::text::Widen(texts.Resolve("@menu.updatetitle"));
                MessageBoxW(window, message.c_str(), title.c_str(), MB_OK | MB_ICONINFORMATION);
                return;
            }
            auto owner = host;
            if (!owner || !owner->Current() || !owner->open) return;
            auto view = owner->view;
            view.details = true;
            view.notice = asi_settings::text::Narrow(message);
            view.originPhysical = view.physical;
            owner->originSelected = owner->selected;
            view.physical = 0;
            if (!owner->Populate(std::move(view), 0) && owner->Current()) owner->Close();
        };
        RWGTA::UpdateCheck::Init(std::move(config));
        if (Screen.bSignatureEdition)
        {
            SignatureUpdateTick = []()
            {
                static bool ticking = false;
                if (ticking) return;
                ticking = true;
                RWGTA::UpdateCheck::Tick();
                ticking = false;
            };
        }
    }

    void Init()
    {
        if (attempted.exchange(true)) return;
        HMODULE own = nullptr;
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               reinterpret_cast<LPCWSTR>(&Init), &own)) return;
        module = own;
        for (const auto& setting : FixSettings().GetDescription().settings)
            if (setting.apply == "restart" && setting.HasValue())
                startupRestart.emplace(std::make_pair(module, setting.id), FixSettings().Wanted(setting.id));
        // The native menu is built for Gamer's Edition.
        if (Screen.bSignatureEdition)
            return;
        Native::Resolve();
        const auto& a = Native::Address();
        if (bMenuReady != 1)
        {
            Log("native menu not available");
            return;
        }
        // Native retained callbacks require process lifetime. Dynamic unload is
        // deliberately unsupported; pin before the first hook or retained data.
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                               reinterpret_cast<LPCWSTR>(&Init), &module))
        {
            Log("process lifetime pin failed; bridge not exposed");
            return;
        }
        try
        {
        wholeHook = safetyhook::create_inline(reinterpret_cast<void*>(a.whole), Whole, SafetyHookInline::StartDisabled);
        worldHook = safetyhook::create_inline(reinterpret_cast<void*>(a.world), World, SafetyHookInline::StartDisabled);
        optionsHook = safetyhook::create_inline(reinterpret_cast<void*>(a.options), Options, SafetyHookInline::StartDisabled);
        selectHook = safetyhook::create_inline(reinterpret_cast<void*>(a.select), Select, SafetyHookInline::StartDisabled);
        mouseHook = safetyhook::create_inline(reinterpret_cast<void*>(a.mouse), Mouse, SafetyHookInline::StartDisabled);
        titleHook = safetyhook::create_inline(reinterpret_cast<void*>(a.title), Title, SafetyHookInline::StartDisabled);
        pauseHook = safetyhook::create_inline(reinterpret_cast<void*>(a.pause), Pause, SafetyHookInline::StartDisabled);
        temporaryHook = safetyhook::create_inline(reinterpret_cast<void*>(a.temporary), Temporary, SafetyHookInline::StartDisabled);
        objectHook = safetyhook::create_inline(reinterpret_cast<void*>(a.object), Object, SafetyHookInline::StartDisabled);
        behaviorHook = safetyhook::create_inline(reinterpret_cast<void*>(a.behavior), Behavior, SafetyHookInline::StartDisabled);
        languageHook = safetyhook::create_inline(reinterpret_cast<void*>(a.language), Language, SafetyHookInline::StartDisabled);
        subtitleHook = safetyhook::create_inline(reinterpret_cast<void*>(a.subtitle), Subtitle, SafetyHookInline::StartDisabled);
        }
        catch (...)
        {
            Rollback();
            Log("hook allocation failed; installation rolled back");
            return;
        }
        // Every original trampoline exists before any branch is enabled.
        for (auto hook : { &wholeHook, &worldHook, &optionsHook, &selectHook, &mouseHook, &titleHook, &pauseHook, &temporaryHook,
            &objectHook, &behaviorHook, &languageHook, &subtitleHook })
            (void)hook->enable();
        installed = true;
    }
}

// Installed after every onInitEvent handler (dllmain): RawInput's and Hud's hooks in the menu code go in first
export void InitMenu()
{
    KongMenu::Init();
    KongMenu::InitUpdates();
}
