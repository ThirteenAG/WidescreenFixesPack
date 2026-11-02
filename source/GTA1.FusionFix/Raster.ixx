module;

#include <stdafx.h>
#include <array>

export module Raster;

import ComVars;
import Log;

export namespace GTA1Raster
{
    // The original row-address table overlaps other globals above legacy modes.
    // Keep every software blitter and its table builder on the same larger table.
    // Reserve the preceding entry too: the text renderer indexes rows[y - 1].
    // Large enough for the rotating camera's square canvas (the screen diagonal).
    std::array<uint32_t, 8818> Rows{};
    uint32_t OriginalRows = 0;
    void Install()
    {
        auto builder = hook::pattern("89 04 8D ? ? ? ? 8B 45 0C 01 45 08");

        auto original = *builder.get_first<uint32_t>(3);
        OriginalRows = original;
        char signature[32]{};
        size_t changed = 0;
        for (int offset : { 0, -4 })
        {
            auto base = original + offset;
            sprintf_s(signature, "%02X %02X %02X %02X", base & 255, (base >> 8) & 255,
                (base >> 16) & 255, base >> 24);
            auto references = hook::pattern(signature);
            for (size_t i = 0; i < references.size(); ++i)
            {
                auto operand = references.get(i).get<uint8_t>();
                // Absolute, DWORD-indexed x86 SIB operand, validated against the
                // native blitters. Exclude coincidental bytes in data or immediates.
                if ((operand[-1] & 0xC7) != 0x85 || (operand[-2] & 0xC7) != 0x04 ||
                    (operand[-3] != 0x8B && operand[-3] != 0x89 && operand[-3] != 0x03))
                    continue;
                injector::WriteMemory<uint32_t>(reinterpret_cast<uint32_t*>(operand), reinterpret_cast<uint32_t>(Rows.data() + 1) + offset, true);
                ++changed;
            }
        }
        char diagnostic[96]{};
        sprintf_s(diagnostic, "GTA1 raster row-table references replaced: %zu", changed);
        Log::Write(diagnostic);
    }
}
