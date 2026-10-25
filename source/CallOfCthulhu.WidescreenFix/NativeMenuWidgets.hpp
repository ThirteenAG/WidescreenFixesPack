#pragma once

#include "NativeMenuABI.hpp"
#include "NativeMenuContext.hpp"
#include <functional>

namespace CthulhuMenu
{
    template<class T> inline void Write(void* object, size_t offset, const T& value)
    {
        *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset) = value;
    }

    inline bool ReplaceLiteral(void* row, const std::string& text,
                               const std::function<bool()>& current, char*& previous)
    {
        previous = nullptr;
        if (!current() || !Readable(row, Native::TextRowSize) ||
            Read<uintptr_t>(row, 0) != 0x62C7AC || text.size() >= 4096)
            return false;
        for (unsigned char c : text)
            if (c < 32 || c == 127)
                return false;
        auto buffer = static_cast<char*>(Native::AllocateBuffer(
            text.size() + 1, 7, ".\\CoCControlTextOption.cpp", 83, 1));
        if (!buffer)
            return false;
        std::memcpy(buffer, text.c_str(), text.size() + 1);
        if (!current() || !Readable(row, Native::TextRowSize) || Read<uintptr_t>(row, 0) != 0x62C7AC)
        {
            Native::FreeBuffer(buffer);
            return false;
        }
        previous = Read<char*>(row, 328);
        Write<char*>(row, 328, buffer);
        // The row owns this checked allocation after publication. Its exact native
        // destructor frees it; the caller retains/restores the previous allocation.
        return true;
    }

    struct Entry
    {
        Entry() = default;
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;
        Entry(Entry&&) = delete;
        Entry& operator=(Entry&&) = delete;
        static constexpr int ActionID = 0x7F00;
        void* row = nullptr;
        char* original = nullptr;
        char* caption = nullptr;
        int action = 0;
        int extra = 0;
        uint32_t flags = 0;
        uint8_t selectable = 0;
        bool installed = false;

        bool SameSource(const Context& context) const
        {
            if (!row || row != context.rows[4] || !Readable(row, Native::TextRowSize) ||
                Read<uintptr_t>(row, 0) != 0x62C7AC || Read<void*>(row, 268) != context.options ||
                FindPage(context.manager, "OPTIONS") != context.options || !Readable(context.options, Native::PageSize))
                return false;
            auto children = Read<void**>(context.options, 8);
            return children == context.begin && Readable(children, 7 * sizeof(void*)) &&
                reinterpret_cast<uintptr_t>(Read<void**>(context.options, 12)) ==
                    reinterpret_cast<uintptr_t>(children) + 7 * sizeof(void*) &&
                Read<void*>(children, 4 * sizeof(void*)) == row;
        }

        bool Matches(const Context& context) const
        {
            return installed && SameSource(context) && Read<char*>(row, 328) == caption &&
                Read<int>(row, 264) == ActionID && Read<uint8_t>(row, 273) == 1;
        }

        bool CopyCaption(const std::string& text, const std::function<bool()>& current)
        {
            if (!current())
                return false;
            auto renderer = Read<void*>(row, 320);
            if (!Readable(renderer, 352) || Read<uintptr_t>(renderer, 0) != 0x62FAF0)
                return false;
            Native::CopyText(renderer, text.c_str());
            if (!current())
                return false;
            auto stored = Read<const char*>(renderer, 300);
            return Readable(stored, text.size() + 1) && !std::memcmp(stored, text.c_str(), text.size() + 1);
        }

        bool Build(const Context& context, const std::string& title, const FontMetrics& font,
                   const std::function<bool()>& current)
        {
            if (row || !current())
                return false;
            row = context.rows[4];
            if (!SameSource(context) || Read<int>(row, 264) || Read<uint8_t>(row, 273) ||
                Read<char>(row, 8) || Read<void*>(row, 312))
                return false;
            float measured = 0.0f;
            const auto width = Read<float>(row, 288) - Read<float>(row, 280);
            if (!font.Width(title, measured) || !std::isfinite(width) || measured >= width)
                return false;
            action = Read<int>(row, 264);
            extra = Read<int>(row, 4);
            flags = Read<uint32_t>(row, 308);
            selectable = Read<uint8_t>(row, 273);
            if (!ReplaceLiteral(row, title, current, original))
                return false;
            caption = Read<char*>(row, 328);
            if (!CopyCaption(title, current) || !current())
                return false;
            // The empty stock spacer can have a different style from row0.
            // Verify the actual copied entry's native layout/font, not only the
            // source scalar estimate used to bound construction before copying.
            auto renderer = Read<void*>(row, 320);
            auto backend = Read<void*>(reinterpret_cast<void*>(0x6F55B0), 0);
            if (!Readable(backend, 20) || Read<uintptr_t>(backend, 0) != 0x68AC3C)
                return false;
            Native::LayoutText(renderer);
            FontProbe probe;
            if (!current() || !SameSource(context) || !probe.Capture(row) ||
                !probe.Width(title, measured) || measured >= width)
                return false;
            Native::Action(row, ActionID, "", 0);
            if (!current())
                return false;
            Write<uint8_t>(row, 273, 1);
            installed = true;
            return Matches(context);
        }

        bool Restore(const Context& context, const std::function<bool()>& authorized)
        {
            if (!row)
                return true;
            if (!caption)
            {
                row = nullptr;
                return true;
            }
            if (!authorized() || !SameSource(context) || Read<char*>(row, 328) != caption)
                return false;
            // Restore the copied renderer caption before changing row semantics.
            // On allocation/copy failure keep the exact installed transaction for
            // a later safe retry instead of exposing stock actions with our label.
            auto renderer = Read<void*>(row, 320);
            if (renderer && ((original && (!Readable(original, 1) || *original)) ||
                !CopyCaption("", authorized)))
                return false;
            if (!authorized() || !SameSource(context) || Read<char*>(row, 328) != caption)
                return false;
            auto retained = caption;
            Write<char*>(row, 328, original);
            caption = nullptr;
            original = nullptr;
            Write<int>(row, 264, action);
            Write<int>(row, 4, extra);
            Write<uint32_t>(row, 308, flags);
            Write<uint8_t>(row, 273, selectable);
            installed = false;
            if (Read<void*>(context.manager, 48) == context.options && Read<int>(context.options, 540) == 4 &&
                !Read<void*>(context.manager, 44) && !Read<void*>(context.manager, 52))
                Native::Select(context.options, 1, 1);
            Native::FreeBuffer(retained);
            row = nullptr;
            return true;
        }

        void Forgotten()
        {
            // Native row retirement frees the active caption. The previous
            // allocation remains retained until an authorized outer cleanup.
            row = nullptr;
            caption = nullptr;
            installed = false;
        }

        void ReleaseOriginal(const std::function<bool()>& authorized)
        {
            if (!row && original && authorized())
            {
                auto retained = original;
                original = nullptr;
                Native::FreeBuffer(retained);
            }
        }
    };

    struct Page
    {
        Page() = default;
        Page(const Page&) = delete;
        Page& operator=(const Page&) = delete;
        Page(Page&&) = delete;
        Page& operator=(Page&&) = delete;
        static constexpr const char* Name = "WFPADV_CTHULHU";
        void* object = nullptr;
        void* node = nullptr;
        bool registered = false;
        bool constructed = false;
        bool publicationAttempted = false;

        bool Create(const Context& context, const std::function<bool()>& current)
        {
            if (object || !current())
                return false;
            void* collision = nullptr;
            if (!FindPageNode(context.manager, Name, collision) || collision)
                return false;
            object = Native::AllocateObject(Native::PageSize, 7, ".\\CoCMenuBuilder.cpp", 291, 1);
            if (!object)
                return false;
            if (!current())
                return false;
            Native::ConstructOptions(object);
            constructed = true;
            if (!current() || !Readable(object, Native::PageSize) ||
                Read<uintptr_t>(object, 0) != 0x62CAEC)
                return false;
            // Track the allocated object before any map publication. A failed or
            // replaced source context must not strand an unregistered native page.
            if (!FindPageNode(context.manager, Name, collision) || collision || !current())
                return false;
            publicationAttempted = true;
            Native::Publish(context.manager, Name, object);
            if (!current())
                return false;
            if (!FindPageNode(context.manager, Name, node) || !node || Read<void*>(node, 40) != object)
                return false;
            registered = true;
            return current();
        }

        bool Remove(void* manager, const std::function<bool()>& authorized)
        {
            if (!object)
                return true;
            // The host authorizes this only after complete input/update/render
            // unwind, with the exact native manager retained and teardown excluded.
            if (!authorized())
                return false;
            if (publicationAttempted)
            {
                void* current = nullptr;
                if (!Readable(manager, 72) || Read<uintptr_t>(manager, 0) != 0x62BD68 ||
                    Read<void*>(manager, 44) == object || Read<void*>(manager, 48) == object ||
                    Read<void*>(manager, 52) == object || !FindPageNode(manager, Name, current))
                    return false;
                const auto history = Read<uint32_t>(reinterpret_cast<void*>(0x6C8890), 0);
                if (history > 10)
                    return false;
                for (uint32_t i = 0; i < history; ++i)
                    if (Read<void*>(reinterpret_cast<void*>(0x6C8840), i * 8) == object)
                        return false;
                if (current && (Read<void*>(current, 40) != object || (node && current != node)))
                    return false;
                if (current)
                    Native::ErasePage(manager, current);
                if (!object)
                    return true;
                if (!authorized())
                    return false;
                node = nullptr;
                registered = false;
                publicationAttempted = false;
            }
            if (constructed && (!Readable(object, Native::PageSize) || Read<uintptr_t>(object, 0) != 0x62CAEC))
                return false;
            auto allocation = object;
            object = nullptr;
            if (constructed)
                Native::DeleteOptions(allocation);
            else
                Native::FreeObject(allocation);
            constructed = false;
            publicationAttempted = false;
            return true;
        }

        bool Alive(const Context& context) const
        {
            void* current = nullptr;
            return object && registered && context.Alive() &&
                FindPageNode(context.manager, Name, current) && current == node &&
                Read<void*>(current, 40) == object && Readable(object, Native::PageSize) &&
                Read<uintptr_t>(object, 0) == 0x62CAEC;
        }

        void Forgotten()
        {
            // Called before the native owner destroys/clears its registered page.
            // Never delete the same engine-owned allocation from the C++ host.
            object = nullptr;
            node = nullptr;
            registered = false;
            constructed = false;
            publicationAttempted = false;
        }
    };
}
