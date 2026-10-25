#pragma once
#include "NativeMenuABI.hpp"
#include <array>
#include <cmath>
#include <cstring>

namespace FarCryMenu
{
#ifdef _WIN64
    constexpr uintptr_t uiTableRva = 0x1BEE70, buttonTableRva = 0x1BCAC0, staticTableRva = 0x1BEB40;
    constexpr uintptr_t systemTableRva = 0xD9EE0;
    constexpr size_t uiBytes = 688, uiSystemOffset = 104, uiScriptOffset = 112, uiRendererOffset = 120;
    constexpr size_t verticalScaleOffset = 632, uiFlagsOffset = 92;
    constexpr size_t widgetsBeginOffset = 200, screensBeginOffset = 672, focusOffset = 360, modalOffset = 368;
    constexpr size_t widgetRectOffset = 136, widgetUiOffset = 304, widgetScreenOffset = 312;
    constexpr size_t systemFontOffset = 10208, uiScriptObjectOffset = 184, scriptOwnerUiOffset = 40, scriptOwnerServiceOffset = 24, screenActiveOffset = 112;
#else
    constexpr uintptr_t uiTableRva = 0xEEFC4, buttonTableRva = 0xED708, staticTableRva = 0xEEC68;
    constexpr uintptr_t systemTableRva = 0x6C958;
    constexpr size_t uiBytes = 420, uiSystemOffset = 92, uiScriptOffset = 96, uiRendererOffset = 100;
    constexpr size_t verticalScaleOffset = 384, uiFlagsOffset = 84;
    constexpr size_t widgetsBeginOffset = 128, screensBeginOffset = 412, focusOffset = 220, modalOffset = 224;
    constexpr size_t widgetRectOffset = 84, widgetUiOffset = 180, widgetScreenOffset = 184;
    constexpr size_t systemFontOffset = 10044, uiScriptObjectOffset = 124, scriptOwnerUiOffset = 20, scriptOwnerServiceOffset = 12, screenActiveOffset = 44;
#endif
    template<typename T> inline T Field(void* object, size_t offset)
    {
        T result{};
        if (Readable(object, offset + sizeof(T)))
            std::memcpy(&result, static_cast<unsigned char*>(object) + offset, sizeof(T));
        return result;
    }
    inline bool Member(void* ui, size_t vectorOffset, void* object)
    {
        if (!object || !Readable(ui, vectorOffset + 2 * sizeof(void*))) return false;
        const auto first = Field<uintptr_t>(ui, vectorOffset);
        const auto last = Field<uintptr_t>(ui, vectorOffset + sizeof(void*));
        if (last < first || (last - first) % sizeof(void*) || last - first > 4096 * sizeof(void*)) return false;
        if (last == first || !Readable(reinterpret_cast<void*>(first), last - first)) return false;
        for (auto current = first; current < last; current += sizeof(void*))
        {
            void* entry{};
            std::memcpy(&entry, reinterpret_cast<void*>(current), sizeof(entry));
            if (entry == object) return true;
        }
        return false;
    }
    struct NativeRect { float x{}, y{}, width{}, height{}; };
    inline bool Rect(void* widget, NativeRect expected)
    {
        if (!Readable(widget, widgetRectOffset + sizeof(NativeRect))) return false;
        const auto actual = Field<NativeRect>(widget, widgetRectOffset);
        for (const auto value : {actual.x, actual.y, actual.width, actual.height})
            if (!std::isfinite(value)) return false;
        // Native button layout can round the requested rectangle. Permit one
        // design unit while retaining the intended position and extent.
        return std::abs(actual.x - expected.x) <= 1.0f && std::abs(actual.y - expected.y) <= 1.0f &&
            std::abs(actual.width - expected.width) <= 1.0f && std::abs(actual.height - expected.height) <= 1.0f &&
            actual.width > 0 && actual.height > 0;
    }
    struct NativePool
    {
        void* ui{};
        void* script{};
        void* source{};
        void* page{};
        void* entry{};
        void* heading{};
        void* description{};
        void* prototype{};
        std::array<void*, 7> rows{};
        NativeRect entryBounds{};
        bool retired{};
        bool Owns(void* sender) const
        {
            return sender && (sender == page || sender == entry ||
                std::find(rows.begin(), rows.end(), sender) != rows.end());
        }
        bool Live(HMODULE game, HMODULE scriptModule) const
        {
            if (retired || !Table(ui, game, uiTableRva) || !Readable(ui, uiBytes) ||
                Field<void*>(ui, uiScriptOffset) != script || !Table(script, scriptModule, scriptTableRva) ||
                !Member(ui, screensBeginOffset, source) || !Member(ui, screensBeginOffset, page)) return false;
            const auto button = [&](void* object)
            {
                return Member(ui, widgetsBeginOffset, object) && Table(object, game, buttonTableRva) &&
                    Field<void*>(object, widgetUiOffset) == ui;
            };
            const auto decorative = [&](void* object)
            {
                return Member(ui, widgetsBeginOffset, object) && Table(object, game, staticTableRva) &&
                    Field<void*>(object, widgetUiOffset) == ui;
            };
            if (!button(entry) || !button(prototype) || !decorative(heading) || !decorative(description) ||
                Field<void*>(entry, widgetScreenOffset) != source || Field<void*>(prototype, widgetScreenOffset) != source ||
                Field<void*>(heading, widgetScreenOffset) != page || Field<void*>(description, widgetScreenOffset) != page ||
                !Rect(entry, entryBounds) || !Rect(heading, {220, 152, 540, 32}) ||
                !Rect(description, {220, 470, 540, 56})) return false;
            for (size_t i = 0; i < rows.size(); ++i)
                if (!button(rows[i]) || Field<void*>(rows[i], widgetScreenOffset) != page || !Rect(rows[i], {220, 200 + static_cast<float>(i) * 36, 540, 32})) return false;
            return true;
        }
        // GetFocusScreen's script wrapper dereferences this optional field.
        // Never call it: the installed variants prove the nullable native slot.
        bool Modal() const { return Field<void*>(ui, modalOffset) != nullptr; }
        bool Selected(size_t index) const
        {
            return index < rows.size() && Field<void*>(ui, focusOffset) == rows[index];
        }
    };
}
