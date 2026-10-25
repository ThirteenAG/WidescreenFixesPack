#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include <array>
#include <cmath>

namespace THPS4Menu
{
    struct Event { uint32_t type, source, target, flags; Structure* data; };
    static_assert(sizeof(Event) == 20);
    // All calls require supported-image Contracts and the native UI thread.
    // No owned CFunction is published as an event script:422D00 requires the
    // nonzero script result which4137E0 deliberately does not return for type8.
    inline void* Symbol(uint32_t name)
    {
        auto* table = Read<void*>(reinterpret_cast<void*>(0x5CA278));
        if (!Readable(table, 4096 * 20)) return nullptr;
        return reinterpret_cast<void* (__cdecl*)(uint32_t)>(0x419130)(name);
    }
    inline bool ScriptAvailable(const char* name)
    {
        auto* symbol = Symbol(Checksum(name));
        return Read<uint8_t>(symbol, 1) == 7 && Read<uintptr_t>(symbol, 12) != 0;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!ScriptAvailable(name)) return false;
        //413090's result is allocator cleanup, not a semantic helper result.
        // Only proved no-wait helpers; validate every created native identity.
        reinterpret_cast<int (__cdecl*)(uint32_t, const Structure*, void*, char, const char*)>(0x413090)(Checksum(name), &parameters.value, nullptr, 0, name);
        return true;
    }
    inline bool Spawn(const char* name, const Parameters& parameters, uint32_t id)
    {
        if (!ScriptAvailable(name)) return false;
        return reinterpret_cast<void* (__cdecl*)(uint32_t, const Structure*, uint32_t, const Structure*, uint32_t, uint32_t, char, char, char)>(0x4137E0)(Checksum(name), &parameters.value, 0, nullptr, 0, id, 0, 0, 0) != nullptr;
    }
    class Weak
    {
    public:
        WeakSlot slot;
        Weak() = default;
        Weak(const Weak&) = delete;
        Weak& operator=(const Weak&) = delete;
        Weak(Weak&&) = delete;
        Weak& operator=(Weak&&) = delete;
        ~Weak() { Reset(); }
        void Reset()
        {
            if (slot.element)
                reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(0x4287F0)(slot.element, &slot);
            slot = {};
        }
        void Find(uint32_t id)
        {
            Reset();
            //4748F0 explicitly replaces ECX with native5CD25C internally,
            // unlike the manager-thiscall named-param lookup4749B0.
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(0x4748F0)(&slot, id, 0);
            if (slot.element) return;
            // AssignAlias publishes into the registry's separate alias table.
            // Resolve that name as the stock named-param lookup does, then let
            // the ordinary constructor register a weak reference to its ID.
            auto* registry = Read<void*>(reinterpret_cast<void*>(0x5CD25C));
            auto* aliases = Read<void*>(registry, 12);
            if (!Readable(registry, 16) || !Readable(aliases, 12) || Read<uint32_t>(aliases) > 24
                || !Readable(Read<void*>(aliases, 4), 12)) return;
            auto* alias = reinterpret_cast<void* (__thiscall*)(void*, uint32_t)>(0x427B10)(registry, id);
            if (!Readable(alias, 24)) return;
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(0x4748F0)(&slot, Read<uint32_t>(alias, 20), 0);
            if (slot.element != alias) Reset();
        }
        void* Get() const { return slot.element; }
    };
    inline bool Name(const Structure* parameters, const char* name, uint32_t& value)
    {
        if (!Readable(parameters, sizeof(Structure))) return false;
        auto* component = parameters->head;
        auto hash = *name ? Checksum(name) : 0;
        for (size_t n = 0; component && n < 256; ++n)
        {
            if (!Readable(component, sizeof(Component))) return false;
            if (component->type == 13 && component->name == hash) { value = component->value; return true; }
            component = component->next;
        }
        return false;
    }
    inline bool Properties(const Parameters& parameters)
    {
        uint32_t id;
        if (!Name(&parameters.value, "id", id)) return false;
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        return reinterpret_cast<char (__cdecl*)(const Structure*)>(0x475D90)(&parameters.value) != 0;
    }
    inline bool Destroy(uint32_t id);
    inline bool Create(uint32_t type, uint32_t id, const Parameters& parameters)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0xAB4B80));
        if (!Readable(manager, 4) || (type != Checksum("TextElement") && type != Checksum("ContainerElement"))) return false;
        Weak existing, parent;
        existing.Find(id);
        uint32_t parentId;
        if (existing.Get() || !Name(&parameters.value, "parent", parentId)) return false;
        parent.Find(parentId);
        if (!parent.Get()) return false;
        WeakSlot slot;
        reinterpret_cast<WeakSlot* (__thiscall*)(void*, WeakSlot*, uint32_t, uint32_t, const Structure*)>(0x473F50)(manager, &slot, type, id, &parameters.value);
        Weak registered;
        registered.Find(id);
        auto exact = slot.element && registered.Get() == slot.element;
        auto result = exact && parent.Get() && Read<uint32_t>(slot.element, 20) == id
            && Read<uint32_t>(slot.element, 24) == type && Read<void*>(slot.element, 64) == parent.Get();
        if (!result && exact) Destroy(id); // Only the exact newly-created node.
        if (slot.element)
            reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(0x4287F0)(slot.element, &slot);
        return result;
    }
    inline bool Destroy(uint32_t id)
    {
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        Parameters parameters;
        parameters.Name("id", id);
        //475BC0 ->4746E0 ->4754D0 recursion only uses the optional script in
        // guarded non-null4115A0 detachment; engine owns every node deletion.
        return reinterpret_cast<char (__cdecl*)(const Structure*, void*)>(0x475BC0)(&parameters.value, nullptr) != 0;
    }
    inline bool Fire(const char* event, uint32_t target, Structure* data = nullptr)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0x5CD25C));
        Weak object;
        object.Find(target);
        if (!Readable(manager, 4) || !object.Get()) return false;
        //Direct manager Event dispatch: CFN428070 always dereferences script.
        //427C10 builds a synchronous20-byte event and invokes target virtual16.
        return reinterpret_cast<bool (__thiscall*)(void*, uint32_t, uint32_t, uint32_t, Structure*)>(0x427C10)(manager, Checksum(event), target, target, data);
    }
    struct Extent { float width = 0, height = 0; };
    inline bool Measure(const std::string& literal, Extent& extent)
    {
        auto* table = Read<void*>(reinterpret_cast<void*>(0xAAC5C0));
        auto bits = Read<uint32_t>(reinterpret_cast<void*>(0xAAC5BC));
        if (literal.size() > 4096 || bits > 16 || !Readable(table, size_t(12) << bits)
            || !Read<uint8_t>(reinterpret_cast<void*>(0x66D2E0))) return false;
        auto* font = reinterpret_cast<void* (__cdecl*)(uint32_t)>(0x4635E0)(Checksum("small"));
        if (!Readable(font, 12) || Read<uintptr_t>(font) != 0x5898F4) return false;
        auto* platform = Read<void*>(font, 8);
        if (!Readable(platform, 308)) return false;
        auto glyphs = Read<uintptr_t>(platform, 8);
        if (!glyphs) return false;
        for (unsigned char c : literal)
        {
            if (c == ' ') continue;
            auto index = Read<uint8_t>(platform, 12 + c);
            auto offset = size_t(index) * 24;
            if (offset > UINTPTR_MAX - glyphs || !Readable(reinterpret_cast<void*>(glyphs + offset), 24)) return false;
        }
        auto escaped = EscapeNativeText(literal, false);
        reinterpret_cast<void (__thiscall*)(void*, const char*, float*, float*)>(0x462E20)(font, escaped.c_str(), &extent.width, &extent.height);
        return std::isfinite(extent.width) && std::isfinite(extent.height) && extent.width >= 0 && extent.height > 0;
    }
}

