#pragma once
#include "NativeMenuBindings.hpp"
#include "NativeMenuFit.hpp"
#include "NativeMenuModel.hpp"
#include <array>

namespace CondemnedMenuNative
{
    struct Array
    {
        void** begin{}, **end{}, **capacity{};
        size_t Size() const { return begin && end >= begin ? static_cast<size_t>(end - begin) : 0; }
        static std::optional<Array> Get(void* screen)
        {
            Array out{Read<void**>(screen, 200).value_or(nullptr), Read<void**>(screen, 204).value_or(nullptr),
                Read<void**>(screen, 208).value_or(nullptr)};
            const auto b = reinterpret_cast<uintptr_t>(out.begin), e = reinterpret_cast<uintptr_t>(out.end),
                c = reinterpret_cast<uintptr_t>(out.capacity);
            if (!b || e < b || c < e || (e - b) % 4 || (c - b) % 4 || (c - b) / 4 > 64 ||
                !Readable(out.begin, c - b)) return {};
            return out;
        }
        bool Same(const Array& other) const { return begin == other.begin && end == other.end && capacity == other.capacity; }
        void Put(void* screen) const
        {
            // Fields are changed only at the complete verified service boundary.
            std::memcpy(reinterpret_cast<uint8_t*>(screen) + 200, this, sizeof(*this));
        }
    };
    static_assert(sizeof(Array) == 12);
    struct Pool
    {
        static constexpr size_t maxItems = 8; //5 root links+Previous+Next+Back
        std::array<void*, 2 + 2 * maxItems> nodes{};
        size_t count{};
        Array array;
        Prepared presented;
        std::array<uint16_t, maxItems> indices{};
        // No destructor calls into the game. Explicit cleanup requires the full
        // traversal/resource lease and the host's authorized deletion scope.
        void Destroy(const Bindings& api)
        {
            for (auto& node : nodes) { if (node) api.calls.Destroy(node); node = nullptr; }
            if (array.begin) api.Free(array.begin);
            array = {}; count = 0;
        }
        bool Contains(const void* object) const
        {
            return object && std::find(nodes.begin(), nodes.end(), object) != nodes.end();
        }
        std::optional<size_t> Item(uint16_t physical, void* selected) const
        {
            for (size_t i = 0; i < presented.items.size(); ++i)
                if (indices[i] == physical && nodes[physical] == selected) return i;
            return {};
        }
    };
    inline std::optional<RECT> Client(const View& view)
    {
        const auto rect = Read<RECT>(view.screen, 220);
        if (!rect || rect->right <= rect->left || rect->bottom <= rect->top ||
            static_cast<int64_t>(rect->right) - rect->left > 8192 ||
            static_cast<int64_t>(rect->bottom) - rect->top > 8192) return {};
        // The remaining width is the photograph, outside the paper list.
        auto content = *rect;
        content.right = content.left + (content.right - content.left) * 2 / 5;
        return content;
    }
    template<class Current> inline void* TextNode(const Bindings& api, const Ready& ready, Pool& pool,
        const RECT& rectangle, void* handler, uint32_t command, uint32_t item, uint32_t generation, bool selectable,
        Current& current)
    {
        if (!current() || pool.count == pool.nodes.size()) return nullptr;
        Create params{}; params.command = command; params.handler = handler;
        params.parameter1 = item; params.parameter2 = generation; params.rect = rectangle;
        const auto row = api.calls.MakeText(L" ", ready.font, params);
        if (!row) return nullptr;
        pool.nodes[pool.count++] = row; // retain even if the owner changed in Create
        if (!current() || !api.controls.Scale(api.calls, row, ready.scaleX, ready.scaleY) || !current()) return nullptr;
        for (const auto& color : {std::pair<size_t, uint32_t>{68, ready.selectedColor}, {72, ready.enabledColor},
            {76, selectable ? ready.disabledColor : ready.enabledColor}})
            std::memcpy(reinterpret_cast<uint8_t*>(row) + color.first, &color.second, sizeof(color.second));
        if (!api.controls.Enable(api.calls, row, selectable) || !current() ||
            Read<uint8_t>(row, 65).value_or(2) != static_cast<uint8_t>(selectable) ||
            Read<float>(row, 40) != ready.scaleX || Read<float>(row, 44) != ready.scaleY) return nullptr;
        return row;
    }
    template<class Current> inline bool FitControl(const Bindings& api, const Ready& ready, void* row,
        const std::string& utf8, int logicalWidth, int logicalHeight, bool whole, Current& current, const std::string& requiredTail = {})
    {
        const auto text = Wide(utf8), tail = Wide(requiredTail);
        if (!text || !tail || !current()) return false;
        if (text->empty())
        {
            if (!api.calls.SetText(row, L"") || !current()) return false;
            const auto copied = api.calls.Text(row);
            return copied && copied->empty() && current();
        }
        const auto minHeight = static_cast<uint32_t>((std::max)(1.0, std::ceil(8.0 / ready.scaleY)));
        if (minHeight > ready.font.height) return false;
        const auto fitted = Fit(api.calls, row, *text, ready.font.height, minHeight,
            static_cast<int>(logicalWidth * ready.scaleX), static_cast<int>(logicalHeight * ready.scaleY), whole, current, *tail);
        if (!fitted || !current()) return false;
        const auto copied = api.calls.Text(row);
        return copied && *copied == fitted->text && current();
    }
    template<class Current> inline bool PreparePool(const Bindings& api, const Ready& ready,
        Pool& out, Prepared prepared, const std::string& unavailable, uint32_t firstCommand, uint32_t generation,
        Current current)
    {
        const auto client = Client(ready.view);
        const auto count = prepared.items.size();
        if (!client || !count || count > Pool::maxItems || out.count || !current()) return false;
        // Two fixed native rows are reserved for title and description. Positive
        // physical height and final native metrics are both required; shrink
        // refuses unavailable full values rather than publishing a partial value.
        const auto width = client->right - client->left;
        const auto rowHeight = (client->bottom - client->top) / static_cast<LONG>(count + 2);
        if (width < 64 || rowHeight < 1 || rowHeight * ready.scaleY < 8) return false;
        const auto title = TextNode(api, ready, out, {client->left, client->top, client->right, client->top + rowHeight},
            nullptr, 0, 0, generation, false, current);
        const auto description = TextNode(api, ready, out,
            {client->left, client->bottom - rowHeight, client->right, client->bottom}, nullptr, 0, 0, generation, false, current);
        if (!title || !description || !FitControl(api, ready, title, prepared.title, width, rowHeight, false, current) ||
            !FitControl(api, ready, description, "", width, rowHeight, false, current)) return false;
        for (size_t i = 0; i < count; ++i)
        {
            auto& item = prepared.items[i];
            const auto top = client->top + rowHeight * static_cast<LONG>(i + 1);
            const bool hasValue = !item.value.empty();
            const auto labelWidth = hasValue ? width * 3 / 5 - 8 : width;
            const auto valueLeft = hasValue ? labelWidth + 8 : width;
            const auto valueWidth = width - valueLeft;
            const RECT labelRect{client->left, top, client->left + labelWidth, top + rowHeight};
            const RECT valueRect{client->left + valueLeft, top, client->right, top + rowHeight};
            out.indices[i] = static_cast<uint16_t>(out.count);
            const auto label = TextNode(api, ready, out, labelRect, ready.view.screen, firstCommand,
                static_cast<uint32_t>(i), generation, item.enabled, current);
            const auto value = TextNode(api, ready, out, valueRect, nullptr, 0, 0, generation, false, current);
            if (!label || !value || !FitControl(api, ready, label, item.label, labelWidth, rowHeight, false, current, item.requiredTail)) return false;
            if (!FitControl(api, ready, value, item.value, (std::max)(1L, valueWidth), rowHeight, true, current))
            {
                if (!current()) return false;
                item.enabled = false;
                if (!api.controls.Enable(api.calls, label, false) || !current() ||
                    Read<uint8_t>(label, 65).value_or(2) != 0 ||
                    !FitControl(api, ready, value, unavailable, (std::max)(1L, valueWidth), rowHeight, true, current)) return false;
            }
        }
        out.array.begin = static_cast<void**>(api.Allocate(out.count * sizeof(void*)));
        if (!out.array.begin || !current()) return false;
        std::memcpy(out.array.begin, out.nodes.data(), out.count * sizeof(void*));
        out.array.end = out.array.capacity = out.array.begin + out.count;
        out.presented = std::move(prepared);
        return current();
    }
}
