#pragma once

#include "NativeMenuContext.hpp"
#include <algorithm>
#include <functional>

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
            if (!Valid() || !Native::Readable(font, 24))
                return false;
            const auto count = Native::Read<uint32_t>(font, 12);
            const auto uv = Native::Read<void*>(font, 16);
            const auto width = Native::Read<uint16_t>(font, 20);
            const auto height = Native::Read<uint16_t>(font, 22);
            if (!count || count > 4096 || !width || !height ||
                !Native::Readable(uv, size_t(count) * 16) ||
                !Native::Literal(ascii, count, encoded))
                return false;
            for (unsigned char c : ascii)
            {
                const auto index = Native::Read<uint32_t>(reinterpret_cast<void*>(0xCC09E0), c * 4) - 32;
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

        static std::vector<std::string> Tokens(const std::string& utf8)
        {
            // Only this complete printable ASCII path has verified native
            // glyph coverage. Unsupported text refuses the page, never a
            // substituted label or a truncated current value.
            std::vector<std::string> result;
            if (utf8.size() > 8192) return result;
            for (unsigned char byte : utf8)
            {
                if (byte < 32 || byte > 126) return {};
                result.emplace_back(1, char(byte));
            }
            return result;
        }

        bool Split(const std::string& utf8, float maxWidth, float maxHeight,
                   std::vector<std::string>& pieces)
        {
            pieces.clear();
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
                        Put(context.first + 29, candidate, 0.0f, 0.0f,
                            maxWidth, maxHeight, 0xFFFFFFFF);
                    if (!Valid()) return false;
                    if (fits) { low = trial; best = std::move(candidate); }
                    else high = trial - 1;
                }
                if (!low || best.empty()) { Hide(context.first + 29); return false; }
                pieces.push_back(std::move(best));
                if (pieces.size() > 2048) { Hide(context.first + 29); return false; }
                at += low;
            }
            return Hide(context.first + 29);
        }

        bool Put(int slot, const std::string& ascii, float x, float y,
                 float maxWidth, float maxHeight, uint32_t color)
        {
            std::string encoded;
            if (!Capacity(slot, ascii.size()) || !Encode(ascii, encoded) ||
                !std::isfinite(x) || !std::isfinite(y) || !std::isfinite(maxWidth) ||
                !std::isfinite(maxHeight) || x < 0 || y < 0 || maxWidth <= 0 ||
                maxHeight <= 0 || x + maxWidth > 1 || y + maxHeight > 1)
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
            Native::Flags(context.text, slot, int16_t(context.style.flags));
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
                    if (((glyph.symbol >> 12) & 63) || (glyph.symbol & 0xFFF) < 32 ||
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
