#pragma once
#include "NativeMenuFrames.hpp"
#include "NativeMenuGeometry.hpp"
#include "NativeMenuWidgets.hpp"
#include <algorithm>
#include <cfloat>

namespace HD2Menu
{
    // The native title factory uses this same native allocator/free pair for
    // the manager's pointer array. Window indices are encoded into IDs, so an
    // adopted window stays in its slot until the native manager deletes it.
    inline bool AppendWindow(const Context& context, Window& window, uint32_t index,
        const std::function<bool()>& current)
    {
        void** begin = nullptr;
        void** end = nullptr;
        void** capacity = nullptr;
        if (!window.Get() || !current() || !Context::Read(context.manager, 324, begin)
            || !Context::Read(context.manager, 328, end) || !Context::Read(context.manager, 332, capacity)) return false;
        auto first = reinterpret_cast<uintptr_t>(begin);
        auto last = reinterpret_cast<uintptr_t>(end);
        auto limit = reinterpret_cast<uintptr_t>(capacity);
        if (!first || last < first || limit < last || (last - first) % 4 || (limit - first) % 4
            || limit - first > 64 * 4 || (last - first) / 4 != index || index >= 64
            || !Contracts::Readable(begin, limit - first)) return false;
        if (end == capacity)
        {
            auto replacement = static_cast<void**>(ABI::Allocate(64 * sizeof(void*)));
            if (!replacement) return false;
            if (!current()) { ABI::Free(replacement); return false; }
            void** actual = nullptr;
            void** actualEnd = nullptr;
            if (!Context::Read(context.manager, 324, actual) || actual != begin
                || !Context::Read(context.manager, 328, actualEnd) || actualEnd != end)
            { ABI::Free(replacement); return false; }
            std::copy(begin, end, replacement);
            replacement[index] = window.Get();
            Write(context.manager, 324, replacement);
            Write(context.manager, 328, replacement + index + 1);
            Write(context.manager, 332, replacement + 64);
            window.Adopt();
            ABI::Free(begin);
            return current();
        }
        ProtectedGameRef<void*>{ end } = window.Get();
        Write(context.manager, 328, end + 1);
        window.Adopt();
        return current();
    }

    class Entry
    {
    public:
        Entry(uintptr_t graphics, void* driver) : frames(graphics, driver) {}
        Entry(const Entry&) = delete;
        Entry& operator=(const Entry&) = delete;

        bool Build(Context& context, Geometry& geometry, void* callbackOwner, ABI::Action accept,
            const std::string& caption, const std::function<bool()>& current)
        {
            failure = "category initial identity";
            if (widget || !callbackOwner || !accept || !current()) return false;
            void* source = nullptr;
            void* parent = nullptr;
            uint32_t groupID = 0;
            if (!Context::Read(context.buttons.front(), 80, source)
                || !Context::Read(context.group, 80, parent)
                || !Context::Read(context.group, 96, groupID)) return false;
            failure = "category frame clone";
            auto clone = frames.Clone(source);
            if (!clone || !current()) return false;
            ABI::NameFrame(clone, "FusionFixHubCategory");
            failure = "category frame link/registration/name";
            if (!current() || ABI::LinkFrame(clone, parent) < 0 || !current()
                || !frames.Register(context.asset) || !current() || !name.Assign("FusionFixHubCategory") || !current()) return false;
            ABI::Child child{};
            child.type = 20;
            child.id = groupID | (7u << 12);
            child.name = name.Get();
            ABI::Message binding{};
            binding.event = 0x01000007;
            binding.value = 0x04001000;
            binding.owner = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(callbackOwner));
            binding.callback = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(accept));
            child.properties.begin = &binding;
            child.properties.end = &binding + 1;
            child.properties.capacity = &binding + 1;
            pendingChild = true;
            failure = "category native control factory";
            ABI::RealizeControl(context.group, context.asset, parent, child);
            void** children = nullptr;
            size_t count = 0;
            // Record the exact appended native identity before a continuation
            // check can refuse construction. Native destruction clears this
            // weak widget through the host's deferred retirement observer.
            failure = "category source children identity";
            if (!context.Alive() || !Context::Children(context.group, children, count)) return false;
            failure = "category appended-child count";
            if (count != 7)
            {
                if (count == 6) pendingChild = false;
                return false;
            }
            auto appended = children[6];
            uint32_t actualID = 0;
            failure = "category appended-child table/id";
            if (!Context::Table(appended, 0x814CB0) || !Context::Read(appended, 96, actualID) || actualID != child.id)
                return false;
            widget = appended;
            id = actualID;
            failure = "category appended-child continuation";
            if (!current()) return false;
            // The native group uses dynamic children. Preserve all six stock
            // controls, inserting the owned category immediately before Back.
            context.entry = widget;
            int32_t selected = -1;
            Context::Read(context.group, 108, selected);
            ProtectedGameRef<void*>{ children + 5 } = widget;
            ProtectedGameRef<void*>{ children + 6 } = context.buttons.back();
            if (selected == 5) Write(context.group, 108, int32_t(6));
            failure = "category reordered source identity";
            if (!current()) return false;
            ABI::Virtual<uint32_t(__thiscall*)(void*, uint32_t)>(widget, 52)(widget, id);
            failure = "category native focus identity";
            if (!current()) return false;
            void* text = nullptr;
            ABI::String copied;
            failure = "category initial text/style";
            if (!Context::Read(widget, 108, text) || !copied.Assign(caption) || !current()
                || !ABI::SetText(text, copied.Get(), 13) || !current()
                || !ABI::FontStyle(text, 13) || !current()) return false;
            failure = "category native placement/bounds";
            if (!geometry.Apply(context, widget, current)) return false;
            // Re-measure actual reflowed controls, including the new row.
            float previousBottom = -FLT_MAX;
            for (auto button : std::array<void*, 7>{ context.buttons[0], context.buttons[1], context.buttons[2],
                context.buttons[3], context.buttons[4], widget, context.buttons[5] })
            {
                TextProbe probe;
                float a = 0, b = 0;
                failure = "category positioned text metrics";
                if (!current() || !probe.Capture(button, current) || !Context::Read(probe.text, 12, a)
                    || !Context::Read(probe.text, 20, b) || !std::isfinite(a) || !std::isfinite(b)) return false;
                // Native rectangle height is Text12-Text20, not20-12.
                auto top = (std::min)(a * geometry.direction, b * geometry.direction);
                auto bottom = (std::max)(a * geometry.direction, b * geometry.direction);
                failure = "category actual row overlap";
                if (bottom <= top || top < previousBottom + 4) return false;
                previousBottom = bottom;
            }
            TextProbe probe;
            failure = "category final native glyph/fit";
            return current() && probe.Capture(widget, current) && probe.Fits(caption) && current();
        }

        void Remove(Context& context, Geometry& geometry)
        {
            // An unexpected factory continuation is not a validated owned
            // widget. Retain its borrowed frames until native owner teardown;
            // neither destroy a foreign alias nor free frames beneath it.
            if (!widget) { if (!pendingChild) frames.Reset(); return; }
            if (!context.Alive()) return; // Native owner teardown owns its child.
            void** children = nullptr;
            size_t count = 0;
            if (!Context::Children(context.group, children, count) || count != 7) return;
            bool reordered = children[5] == widget && children[6] == context.buttons.back();
            bool appended = children[5] == context.buttons.back() && children[6] == widget;
            if (!reordered && !appended) return;
            int32_t selected = -1;
            Context::Read(context.group, 108, selected);
            if (selected == (reordered ? 5 : 6))
                ABI::Virtual<uint32_t(__thiscall*)(void*, uint32_t)>(context.group, 52)(context.group, id);
            // Restore the exact source array and positions before deleting only
            // the owned control. Queued callback storage is retained by Host.
            ProtectedGameRef<void*>{ children + 5 } = context.buttons.back();
            Write(context.group, 12, children + 6);
            if (reordered && selected == 6) Write(context.group, 108, int32_t(5));
            context.entry = nullptr;
            geometry.Restore(context);
            ABI::DestroyWidget(std::exchange(widget, nullptr));
            pendingChild = false;
            frames.Reset();
        }

        void NativeDestroyed() { widget = nullptr; pendingChild = false; }
        void ResetFrames() { frames.Reset(); }
        void* Get() const { return widget; }
        const char* Failure() const { return failure; }
        uint32_t ID() const { return id; }
    private:
        FrameTree frames;
        ABI::String name;
        const char* failure = "seventh native category/actual layout rollback";
        void* widget = nullptr;
        uint32_t id = 0;
        bool pendingChild = false;
    };
}
