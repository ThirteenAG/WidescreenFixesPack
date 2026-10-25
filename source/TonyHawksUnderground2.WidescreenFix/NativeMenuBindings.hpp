#pragma once
#include "NativeMenuABI.hpp"
#include "NativeMenuContracts.hpp"
#include <cmath>

namespace THUG2Menu
{
    struct Registration { const char* name; bool (__cdecl* callback)(Structure*, void*); };
    static_assert(sizeof(Registration) == 8);

    // Addresses are used only after Contracts succeeds. NativeMenu owns the
    // guarded initialization, input deferral and native lifetime transaction.
    inline void Register(const Registration* table, uint32_t count)
    {
        reinterpret_cast<void (__cdecl*)(const Registration*, uint32_t)>(0x46EF90)(table, count);
    }
    inline void* Symbol(uint32_t name)
    {
        auto* buckets = Read<void*>(reinterpret_cast<void*>(0x6F4624));
        if (!Readable(buckets, 4096 * sizeof(uint32_t))) return nullptr;
        return reinterpret_cast<void* (__cdecl*)(uint32_t)>(0x478DB0)(name);
    }
    inline bool ScriptAvailable(const char* name)
    {
        auto* symbol = Symbol(Checksum(name));
        return Read<uint8_t>(symbol, 1) == 7 && Read<uintptr_t>(symbol, 12) != 0;
    }
    inline bool Run(const char* name, const Parameters& parameters)
    {
        if (!ScriptAvailable(name)) return false;
        // The type7 dispatcher returns cleanup's AL, not helper success. True
        // means issued only; callers must validate created weak identities and
        // geometry before exposing or continuing a native page.
        reinterpret_cast<bool (__cdecl*)(uint32_t, const Structure*, void*, char, const char*)>(0x475770)(Checksum(name), &parameters.value, nullptr, 0, name);
        return true;
    }
    inline bool Spawn(const char* name, const Parameters& parameters, uint32_t id)
    {
        // Parent menus may wait for native animation/gameframes. Queue them in
        // the engine's VM; the synchronous dispatcher must never run these.
        if (!ScriptAvailable(name)) return false;
        return reinterpret_cast<void* (__cdecl*)(uint32_t, const Structure*, uint32_t, const Structure*, uint32_t, uint32_t, char, char, char, char)>(0x475400)(Checksum(name), &parameters.value, 0, nullptr, 0, id, 0, 0, 0, 0) != nullptr;
    }
    inline bool Properties(const Parameters& parameters)
    {
        // SetProps' native wrapper assumes that the supplied id exists. Check
        // its identity through a temporary weak slot before calling it.
        auto* component = parameters.value.head;
        uint32_t id = 0;
        bool found = false;
        for (size_t n = 0; component && n < 256; ++n)
        {
            if (component->name == Checksum("id") && (component->encodedType >> 1) == 13)
            {
                id = component->value;
                found = true;
                break;
            }
            component = component->next;
        }
        if (!found) return false;
        WeakSlot slot;
        reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(0x4AAEE0)(&slot, id, 0);
        if (!slot.element) return false;
        auto result = reinterpret_cast<bool (__cdecl*)(const Structure*)>(0x4AD580)(&parameters.value);
        if (slot.element)
            reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(0x463780)(slot.element, &slot);
        return result;
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
                reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(0x463780)(slot.element, &slot);
            slot = {};
        }
        void Find(uint32_t id)
        {
            Reset();
            reinterpret_cast<WeakSlot* (__stdcall*)(WeakSlot*, uint32_t, int)>(0x4AAEE0)(&slot, id, 0);
        }
        void* Get() const { return slot.element; }
    };

    inline bool Create(uint32_t type, uint32_t id, const Parameters& parameters)
    {
        auto* manager = Read<void*>(reinterpret_cast<void*>(0x702440));
        if (!Readable(manager, 4) || (type != Checksum("TextElement") && type != Checksum("ContainerElement"))) return false;
        WeakSlot slot;
        reinterpret_cast<WeakSlot* (__thiscall*)(void*, WeakSlot*, uint32_t, uint32_t, const Structure*)>(0x4ACC30)(manager, &slot, type, id, &parameters.value);
        auto result = slot.element && Read<uint32_t>(slot.element, 20) == id && Read<uint32_t>(slot.element, 24) == type;
        if (slot.element)
            reinterpret_cast<void (__thiscall*)(void*, WeakSlot*)>(0x463780)(slot.element, &slot);
        return result;
    }
    inline bool Destroy(uint32_t id)
    {
        Weak target;
        target.Find(id);
        if (!target.Get()) return false;
        Parameters parameters;
        parameters.Name("id", id);
        return reinterpret_cast<bool (__cdecl*)(const Structure*, void*)>(0x4AD3C0)(&parameters.value, nullptr);
    }
    inline bool Fire(const char* event, uint32_t target, Structure* data = nullptr)
    {
        Weak object;
        object.Find(target);
        if (!object.Get()) return false;
        Parameters parameters;
        parameters.Name("type", event);
        parameters.Name("target", target);
        parameters.Name("source", target);
        if (data) parameters.Nested("data", data);
        return reinterpret_cast<bool (__cdecl*)(const Structure*, void*)>(0x463470)(&parameters.value, nullptr);
    }

    inline bool Integer(const Structure* parameters, const char* name, int& result)
    {
        return parameters && reinterpret_cast<bool (__thiscall*)(const Structure*, uint32_t, int*, int)>(0x4766F0)(parameters, Checksum(name), &result, 0);
    }

    struct Extent { float width = 0, height = 0; };
    inline bool Measure(const std::string& literal, Extent& extent)
    {
        if (literal.size() > 4096) return false;
        auto* font = reinterpret_cast<void* (__cdecl*)(uint32_t)>(0x48D7E0)(Checksum("small"));
        if (!Readable(font, 12) || Read<uintptr_t>(font) != 0x64B328) return false;
        auto* platform = Read<void*>(font, 8);
        if (!Readable(platform, 312)) return false;
        auto glyphs = Read<uintptr_t>(platform, 8);
        if (!glyphs) return false;
        // Literal text is escaped below, so the native metric never interprets
        // icon/control commands. Check every glyph it will actually access.
        for (unsigned char c : literal)
        {
            if (c == ' ') continue;
            auto index = Read<uint8_t>(platform, 12 + c);
            auto offset = size_t(index) * 24;
            if (offset > UINTPTR_MAX - glyphs || !Readable(reinterpret_cast<void*>(glyphs + offset), 24)) return false;
        }
        auto escaped = EscapeNativeText(literal, false);
        reinterpret_cast<void* (__thiscall*)(void*, const char*, float*, float*)>(0x48D0E0)(font, escaped.c_str(), &extent.width, &extent.height);
        return std::isfinite(extent.width) && std::isfinite(extent.height) && extent.width >= 0 && extent.height > 0;
    }
}
