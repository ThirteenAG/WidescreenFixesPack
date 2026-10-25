#pragma once
#include "NativeMenuView.hpp"
#include "NativeMenuHostState.hpp"
#include <memory>

namespace CondemnedMenuNative
{
    struct SourceCallbacks
    {
        uintptr_t command{}, key{}, escape{}, left{}, right{}, focus{};
    };
    struct Owner
    {
        Ready ready;
        Array original, entryArray;
        std::array<uintptr_t, 34> shadow{}; //RTTI[-1] plus all33 native slots
        std::wstring caption;
        void* entry{};
        uint16_t entryIndex{}, originalSelected{};
        LogicalOwner logical;
        std::unique_ptr<Pool> active, staging, retiring;
        bool retired{}, headerHidden{}, published{};
        uint32_t entryCommand{}, rowCommand{};
        uintptr_t Table() const { return reinterpret_cast<uintptr_t>(shadow.data() + 1); }
        bool Registered() const
        {
            const auto& view = ready.view;
            if (Read<void*>(reinterpret_cast<void*>(view.module + 0x16F388), 0) != view.interfaceManager ||
                !Read<uint8_t>(view.interfaceManager, 14664).value_or(0) ||
                Read<uintptr_t>(view.interfaceManager, 0) != view.module + 0x142594 ||
                Read<uintptr_t>(view.manager, 0) != view.module + 0x147F94 ||
                Read<void*>(view.screen, 12) != view.manager || Read<uint32_t>(view.screen, 16) != view.screenId) return false;
            const auto table = Read<uintptr_t>(view.screen, 0).value_or(0);
            if (table != view.originalVtable && table != Table()) return false;
            const auto begin = Read<uintptr_t>(view.manager, 104).value_or(0), end = Read<uintptr_t>(view.manager, 108).value_or(0);
            if (!begin || end < begin || (end - begin) % 4 || (end - begin) / 4 > 64 ||
                !Readable(reinterpret_cast<void*>(begin), end - begin)) return false;
            for (auto at = begin; at < end; at += 4)
                if (Read<void*>(reinterpret_cast<void*>(at), 0) == view.screen) return true;
            return false;
        }
        bool Current(bool allowCovered = false) const
        {
            if (retired || !Registered() || !ready.SameProvider()) return false;
            const auto view = View::Get(reinterpret_cast<HMODULE>(ready.view.module), Table());
            return view && ready.view.Same(*view) && view->ScreenMode() &&
                (allowCovered || (!view->Covered() && !Read<void*>(view->screen, 216).value_or(nullptr)));
        }
        bool Private() const
        {
            const auto array = Array::Get(ready.view.screen);
            return active && array && array->Same(active->array);
        }
        bool OwnedNode(void* node) const
        {
            return node && (node == entry || (active && active->Contains(node)) ||
                (staging && staging->Contains(node)) || (retiring && retiring->Contains(node)));
        }
        std::optional<size_t> SelectedItem() const
        {
            if (!Current() || !Private()) return {};
            const auto physical = ready.view.Selected();
            if (!physical) return {};
            const auto selected = ready.view.SelectedRow();
            if (!Read<uint8_t>(selected, 20).value_or(0) || !Read<uint8_t>(selected, 65).value_or(0)) return {};
            return active->Item(*physical, selected);
        }
        void Cancel()
        {
            retired = true;
            logical.Invalidate();
        }
        template<class Current> bool PrepareEntry(Bindings& api, uint64_t identity, uint32_t command,
            uint32_t rowsCommand, const std::string& label, const SourceCallbacks& callbacks, Current current)
        {
            if (!callbacks.command || !callbacks.key || !callbacks.escape || !callbacks.left || !callbacks.right || !callbacks.focus) return false;
            const auto array = Array::Get(ready.view.screen);
            const auto text = Wide(label);
            const auto oldCaption = api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24);
            const auto client = Client(ready.view);
            if (!array || array->Size() >= 63 || !text || !oldCaption || !client || !current()) return false;
            original = *array; caption = *oldCaption;
            originalSelected = ready.view.Selected().value_or(UINT16_MAX);
            const auto nativeBack = reinterpret_cast<uint8_t*>(ready.view.manager) + 116;
            LONG lastBottom = client->top, footer = client->bottom;
            entryIndex = static_cast<uint16_t>(original.Size());
            for (size_t i = 0; i < original.Size(); ++i)
            {
                const auto rect = Read<RECT>(original.begin[i], 24);
                if (!rect || rect->bottom < rect->top) return false;
                if (original.begin[i] == nativeBack)
                {
                    footer = (std::min)(footer, rect->top);
                    entryIndex = static_cast<uint16_t>(i);
                }
                else lastBottom = (std::max)(lastBottom, rect->bottom);
            }
            const auto gap = static_cast<int64_t>(footer) - lastBottom;
            const auto margin = static_cast<LONG>((std::max)(1.0, std::ceil(2.0 / ready.scaleY)));
            const auto height = static_cast<LONG>((std::max)(static_cast<double>(ready.font.height), std::ceil(8.0 / ready.scaleY)));
            if (gap < static_cast<int64_t>(height) + 2 * margin) return false;
            Create params{}; params.command = command; params.handler = ready.view.screen;
            params.parameter2 = static_cast<uint32_t>(identity);
            params.rect = {client->left, lastBottom + margin, client->right, lastBottom + margin + height};
            entry = api.calls.MakeText(text->c_str(), ready.font, params);
            if (!entry || !current() || !api.controls.Scale(api.calls, entry, ready.scaleX, ready.scaleY) || !current() ||
                !FitControl(api, ready, entry, label, client->right - client->left, height, true, current)) return false;
            for (const auto& color : {std::pair<size_t, uint32_t>{68, ready.selectedColor}, {72, ready.enabledColor}, {76, ready.disabledColor}})
                std::memcpy(reinterpret_cast<uint8_t*>(entry) + color.first, &color.second, 4);
            if (!Readable(reinterpret_cast<void*>(ready.view.originalVtable - 4), shadow.size() * 4)) return false;
            std::memcpy(shadow.data(), reinterpret_cast<void*>(ready.view.originalVtable - 4), shadow.size() * 4);
            shadow[2] = callbacks.command; shadow[6] = callbacks.escape; shadow[11] = callbacks.key;
            shadow[29] = callbacks.left; shadow[30] = callbacks.right; shadow[9] = callbacks.focus;
            entryArray.begin = static_cast<void**>(api.Allocate((original.Size() + 1) * 4));
            if (!entryArray.begin || !current()) return false;
            for (size_t i = 0, j = 0; i <= original.Size(); ++i)
                entryArray.begin[i] = i == entryIndex ? entry : original.begin[j++];
            entryArray.end = entryArray.capacity = entryArray.begin + original.Size() + 1;
            entryCommand = command; rowCommand = rowsCommand; logical.identity = identity; logical.pool = 1;
            return current();
        }
        bool PublishEntry(const Bindings& api)
        {
            const auto present = Array::Get(ready.view.screen);
            if (!Current() || !present || !present->Same(original) || !entryArray.begin) return false;
            entryArray.Put(ready.view.screen);
            const auto pointer = Table(); std::memcpy(ready.view.screen, &pointer, 4);
            for (auto offset : {272u, 274u, 278u})
            {
                auto index = Read<uint16_t>(ready.view.screen, offset).value_or(UINT16_MAX);
                if (index != UINT16_MAX && index >= entryIndex) ++index;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
            }
            published = true;
            return Current() && Read<uint8_t>(entry, 20).value_or(0) && api.calls.Row(entry);
        }
        bool PublishPool(const Bindings& api)
        {
            if (!staging || !Current() || !logical.settings.IsOpen()) return false;
            const auto token = logical.settings.Revision();
            const auto before = Array::Get(ready.view.screen);
            if (!before || !(before->Same(entryArray) || (active && before->Same(active->array)))) return false;
            std::optional<size_t> preferred;
            const auto oldSelected = SelectedItem();
            if (oldSelected && active->presented.location == staging->presented.location)
            {
                const auto& old = active->presented.items[*oldSelected];
                for (size_t i = 0; i < staging->presented.items.size(); ++i)
                {
                    const auto& next = staging->presented.items[i];
                    const bool same = old.kind == next.kind && (old.kind == ItemKind::ModelRow ?
                        old.row.kind == next.row.kind && old.row.plugin == next.row.plugin && old.row.setting == next.row.setting :
                        old.destination == next.destination);
                    if (same && next.enabled) { preferred = i; break; }
                }
            }
            // Track suppression before native setter; a forced callback can cancel
            // construction and must still restore this exact source caption.
            headerHidden = true;
            api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, L"");
            if (!Current() || token != logical.settings.Revision() || !HasArray(*before)) return false;
            retiring = std::move(active);
            active = std::move(staging);
            active->array.Put(ready.view.screen);
            uint16_t selected = preferred ? active->indices[*preferred] : UINT16_MAX;
            if (!preferred)
                for (size_t i = 0; i < active->presented.items.size(); ++i)
                    if (active->presented.items[i].enabled) { selected = active->indices[i]; break; }
            if (selected == UINT16_MAX) return false; //Back is always enabled
            for (auto offset : {272u, 274u, 278u})
            {
                const auto value = offset == 272 ? selected : UINT16_MAX;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &value, 2);
            }
            const auto pool = active.get();
            const auto selectedNode = pool->nodes[selected];
            api.Select(selectedNode, true);
            if (!Current() || token != logical.settings.Revision() || active.get() != pool || !Private()) return false;
            if (Read<uint8_t>(selectedNode, 64).value_or(0) != 1) return false;
            if (!Current() || token != logical.settings.Revision() ||
                !logical.settings.Commit(active->presented, [&]() { return Current() && Private(); })) return false;
            ++logical.pool;
            return Current() && Private();
        }
        // These methods are used only after all original engine traversals return.
        // Resources are destroyed BEFORE related native interface/provider teardown.
        bool HasArray(const Array& expected) const
        {
            const auto present = Array::Get(ready.view.screen);
            return Registered() && present && present->Same(expected);
        }
        void ReturnToSource(const Bindings& api)
        {
            if (!Registered()) return;
            const auto array = Array::Get(ready.view.screen);
            if (!array || !(array->Same(entryArray) || (active && array->Same(active->array)))) return;
            const auto selected = Read<uint16_t>(ready.view.screen, 272).value_or(UINT16_MAX);
            if (selected < array->Size())
            {
                api.Select(array->begin[selected], false);
                if (!HasArray(*array)) return;
            }
            entryArray.Put(ready.view.screen);
            for (auto offset : {272u, 274u, 278u})
            {
                const uint16_t index = offset == 272 ? entryIndex : UINT16_MAX;
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
            }
            if (headerHidden && ready.SameProvider())
            {
                api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, caption.c_str());
                if (!HasArray(entryArray)) return;
                headerHidden = false;
            }
            if (!HasArray(entryArray) || entryIndex >= entryArray.Size() || entryArray.begin[entryIndex] != entry) return;
            api.Select(entry, true);
        }
        void Restore(const Bindings& api)
        {
            if (!Registered()) { published = false; return; }
            const auto array = Array::Get(ready.view.screen);
            const bool owned = array && (array->Same(entryArray) || (active && array->Same(active->array)));
            std::array<uint16_t, 3> indices{originalSelected, UINT16_MAX, UINT16_MAX};
            if (owned)
            {
                // Preserve current stock selection/hover when removing just the
                // inserted entry. Private rows map back to the prior stock row.
                if (array->Same(entryArray))
                {
                    for (size_t i = 0; i < indices.size(); ++i)
                    {
                        const auto index = Read<uint16_t>(ready.view.screen, std::array<size_t, 3>{272, 274, 278}[i]).value_or(UINT16_MAX);
                        indices[i] = index == UINT16_MAX || index == entryIndex ?
                            (i == 0 ? originalSelected : UINT16_MAX) :
                            static_cast<uint16_t>(index > entryIndex ? index - 1 : index);
                    }
                }
                const auto selected = Read<uint16_t>(ready.view.screen, 272).value_or(UINT16_MAX);
                if (selected < array->Size())
                {
                    api.Select(array->begin[selected], false);
                    if (!HasArray(*array)) return;
                }
                original.Put(ready.view.screen);
                for (size_t i = 0; i < indices.size(); ++i)
                {
                    if (indices[i] >= original.Size()) indices[i] = UINT16_MAX;
                    std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + std::array<size_t, 3>{272, 274, 278}[i], &indices[i], 2);
                }
            }
            if (!Registered()) return;
            if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
            if (headerHidden && ready.SameProvider())
            {
                api.StringText(reinterpret_cast<uint8_t*>(ready.view.screen) + 24, caption.c_str());
                if (!Registered() || (owned && !HasArray(original))) return;
                headerHidden = false;
            }
            // A foreign array is never evidence that cached original rows live.
            if (owned && HasArray(original) && indices[0] < original.Size())
                api.Select(original.begin[indices[0]], true);
            published = false;
        }
        bool PoolReferenced(const Pool& pool) const
        {
            const auto array = Array::Get(ready.view.screen);
            if (!array) return true;
            if (pool.array.begin && array->begin == pool.array.begin) return true;
            for (auto at = array->begin; at < array->end; ++at) if (pool.Contains(*at)) return true;
            return false;
        }
        bool DestroyPools(const Bindings& api)
        {
            for (auto pool : {active.get(), staging.get(), retiring.get()})
                if (pool && PoolReferenced(*pool)) return false;
            for (auto pool : {active.get(), staging.get(), retiring.get()}) if (pool) pool->Destroy(api);
            active.reset(); staging.reset(); retiring.reset();
            return true;
        }
        bool Referenced() const
        {
            if (Read<uintptr_t>(ready.view.screen, 0) == Table()) return true;
            const auto array = Array::Get(ready.view.screen);
            if (!array) return true; // unknown native vector is not detachment
            if (array->begin == entryArray.begin || (active && array->begin == active->array.begin) ||
                (staging && array->begin == staging->array.begin) ||
                (retiring && array->begin == retiring->array.begin)) return true;
            for (auto at = array->begin; at < array->end; ++at) if (OwnedNode(*at)) return true;
            return false;
        }
        bool DetachForTeardown()
        {
            // Only a queued highest related native owning operation authorizes
            // removal from a foreign vector. No native call/allocator/free occurs
            // here; all unrelated rows and the vector's native capacity survive.
            if (!Readable(ready.view.screen, 440) || Read<void*>(ready.view.screen, 12) != ready.view.manager ||
                Read<uint32_t>(ready.view.screen, 16) != ready.view.screenId) return false;
            const auto array = Array::Get(ready.view.screen);
            if (!array) return false;
            const bool ownedBuffer = array->begin == entryArray.begin ||
                (active && array->begin == active->array.begin) ||
                (staging && array->begin == staging->array.begin) ||
                (retiring && array->begin == retiring->array.begin);
            if (ownedBuffer)
            {
                // Never leave the engine pointing at a plugin pointer-buffer
                // that Dispose is about to free. Original storage was retained
                // through all hooked source deletion operations.
                if (!original.begin || !Readable(original.begin, original.Size() * sizeof(void*))) return false;
                for (auto at = original.begin; at < original.end; ++at)
                    if (!Read<uint8_t>(*at, 20).value_or(0)) return false;
                original.Put(ready.view.screen);
                for (auto offset : {272u, 274u, 278u})
                {
                    const uint16_t index = offset == 272 && originalSelected < original.Size() ? originalSelected : UINT16_MAX;
                    std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offset, &index, 2);
                }
                if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                    std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
                return !Referenced();
            }
            std::array<void*, 3> selected{};
            constexpr std::array<size_t, 3> offsets{272, 274, 278};
            for (size_t i = 0; i < selected.size(); ++i)
            {
                const auto index = Read<uint16_t>(ready.view.screen, offsets[i]).value_or(UINT16_MAX);
                if (index < array->Size()) selected[i] = array->begin[index];
            }
            auto write = array->begin;
            for (auto at = array->begin; at < array->end; ++at) if (!OwnedNode(*at)) *write++ = *at;
            const Array detached{array->begin, write, array->capacity};
            detached.Put(ready.view.screen);
            for (size_t i = 0; i < selected.size(); ++i)
            {
                uint16_t index = UINT16_MAX;
                for (auto at = detached.begin; at < detached.end; ++at)
                    if (*at == selected[i]) index = static_cast<uint16_t>(at - detached.begin);
                std::memcpy(reinterpret_cast<uint8_t*>(ready.view.screen) + offsets[i], &index, 2);
            }
            if (Read<uintptr_t>(ready.view.screen, 0) == Table())
                std::memcpy(ready.view.screen, &ready.view.originalVtable, 4);
            return !Referenced();
        }
        bool Dispose(const Bindings& api, bool forced = false)
        {
            Restore(api);
            if (Referenced() && !(forced && DetachForTeardown())) return false;
            if (!DestroyPools(api)) return false;
            if (entry) api.calls.Destroy(entry);
            entry = nullptr;
            if (entryArray.begin) api.Free(entryArray.begin);
            entryArray = {};
            return true;
        }
    };
}
