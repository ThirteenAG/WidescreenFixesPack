#pragma once
#include "NativeMenuState.hpp"
#include <cmath>

// Disconnected readiness snapshot. The provider's methods are reached only by
// guarded GameClient wrappers; this does not invent an engine-provider ABI.
// Probe is permitted only under the future host's complete traversal lease.
namespace CondemnedMenuNative
{
    struct Ready
    {
        View view;
        void* sourceRow{};
        void* provider{};
        uintptr_t providerTable{};
        Font font;
        float scaleX{}, scaleY{};
        RECT sourceRect{};
        uint32_t selectedColor{}, enabledColor{}, disabledColor{};
        static bool Executable(uintptr_t address)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
                info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
            const auto protection = info.Protect & 0xff;
            return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
                protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
        }
        static bool Provider(void* object, uintptr_t table)
        {
            if (!object || !table || Read<uintptr_t>(object, 0) != table ||
                !Readable(reinterpret_cast<void*>(table), 88)) return false;
            // Saved StringBuild uses provider16/20; Bounds uses36. These are
            // readiness checks, not a new direct-call declaration or build guard.
            for (auto offset : {16u, 20u, 36u})
                if (!Executable(Read<uintptr_t>(reinterpret_cast<void*>(table), offset).value_or(0))) return false;
            return true;
        }
        static std::optional<Ready> Capture(const Calls& calls, const View& source, void* row)
        {
            if (!source.ScreenMode() || source.Covered() || !calls.Row(row)) return {};
            const auto begin = Read<uintptr_t>(source.screen, 200).value_or(0);
            const auto end = Read<uintptr_t>(source.screen, 204).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64 ||
                !Readable(reinterpret_cast<void*>(begin), end - begin)) return {};
            bool member{};
            for (auto at = begin; at < end; at += 4)
                if (Read<void*>(reinterpret_cast<void*>(at), 0) == row) member = true;
            if (!member || !Read<uint8_t>(row, 20).value_or(0) || !Read<uintptr_t>(row, 124).value_or(0)) return {};
            Ready out{}; out.view = source; out.sourceRow = row;
            out.provider = Read<void*>(reinterpret_cast<void*>(source.module + 0x169EC0), 0).value_or(nullptr);
            out.providerTable = Read<uintptr_t>(out.provider, 0).value_or(0);
            if (!Provider(out.provider, out.providerTable)) return {};
            const auto font = Read<Font>(row, 84);
            const auto baseHeight = Read<uint32_t>(row, 208);
            const auto x = Read<float>(row, 40), y = Read<float>(row, 44);
            const auto rect = Read<RECT>(row, 24);
            if (!font || !baseHeight || !*baseHeight || *baseHeight > 256 ||
                !font->face[0] || !std::memchr(font->face, 0, sizeof(font->face)) ||
                !x || !y || !std::isfinite(*x) || !std::isfinite(*y) || *x <= 0 || *y <= 0 ||
                !rect || rect->right <= rect->left || rect->bottom <= rect->top) return {};
            out.font = *font; out.font.height = *baseHeight;
            out.scaleX = *x; out.scaleY = *y; out.sourceRect = *rect;
            out.selectedColor = Read<uint32_t>(row, 68).value_or(0);
            out.enabledColor = Read<uint32_t>(row, 72).value_or(0);
            out.disabledColor = Read<uint32_t>(row, 76).value_or(0);
            return out;
        }
        bool SameProvider() const
        {
            return Read<void*>(reinterpret_cast<void*>(view.module + 0x169EC0), 0) == provider &&
                Provider(provider, providerTable);
        }
        template<class Current> bool Probe(const Calls& calls, Current current) const
        {
            if (!current() || !SameProvider() || !calls.Row(sourceRow)) return false;
            const auto text = calls.Text(sourceRow);
            if (!text || text->empty()) return false;
            const auto bounds = calls.Bounds(sourceRow);
            // Actual native dimensions are physical pixels. Source readiness
            // does not claim a whole-value or accented-glyph rendering pass.
            return current() && SameProvider() && bounds &&
                static_cast<int64_t>(bounds->right) - bounds->left > 0 &&
                static_cast<int64_t>(bounds->bottom) - bounds->top >= 8;
        }
    };
}
