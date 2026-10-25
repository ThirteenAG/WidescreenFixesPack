#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <string>
#include <utility>

// Game-local native contracts, independently verified against the readable
// HD2 1.12 image and its installed LS3DF.dll. These helpers are disconnected
// until the host has validated both modules and pinned its callback lifetime.
namespace HD2Menu::ABI
{
    struct StringData
    {
        uint32_t references;
        uint32_t length;
        char text[1];
    };

    struct Message
    {
        uint32_t target = 0;
        uint32_t event = 0;
        uint32_t value = 0;
        uint32_t owner = 0;
        uint32_t callback = 0;
        StringData* text = nullptr;
    };
    static_assert(sizeof(Message) == 24);

    template<class T> struct Array
    {
        uint32_t allocator = 0;
        T* begin = nullptr;
        T* end = nullptr;
        T* capacity = nullptr;
    };
    static_assert(sizeof(Array<void*>) == 16);

    // Borrowed descriptor arrays are consumed synchronously by the native
    // factory. Never call the game's owning descriptor destructor on these.
    struct Child
    {
        uintptr_t table = 0;
        StringData* name = nullptr;
        uint32_t id = 0;
        uint32_t type = 0;
        Array<Message> properties;
        Array<Message> callbacks;
        Array<Child*> children;
    };
    static_assert(sizeof(Child) == 64);

    struct Page
    {
        uintptr_t table = 0;
        Array<Message> properties;
        Array<Message> callbacks;
        Array<Child*> children;
        uint32_t unused = 0;
        StringData* resource = nullptr;
        StringData* name = nullptr;
        uint32_t id = 0;
    };
    static_assert(sizeof(Page) == 68);

    struct Vector { float x, y, z; };

    // 65C900 supplies ECX=owner and four stack arguments. The ordinary branch
    // duplicates owner in the first argument; its catch-all branch passes zero.
    using Action = uint8_t(__thiscall*)(void*, uintptr_t, void*, uint32_t, uint32_t);

    inline uintptr_t game = 0;
    inline bool ready = false;

    template<class F> F Function(uintptr_t original)
    {
        return reinterpret_cast<F>(game + original - 0x400000);
    }

    template<class F> F Virtual(void* object, size_t offset)
    {
        return reinterpret_cast<F>((*reinterpret_cast<uintptr_t**>(object))[offset / sizeof(uintptr_t)]);
    }

    inline void* Allocate(size_t size)
    {
        if (!ready || size > static_cast<size_t>((std::numeric_limits<int32_t>::max)()))
            return nullptr;
        return Function<void*(__cdecl*)(size_t)>(0x7E520F)(size);
    }

    inline void Free(void* pointer)
    {
        if (pointer)
            Function<int32_t(__cdecl*)(void*)>(0x7E1CC0)(pointer);
    }

    class String
    {
    public:
        String() = default;
        String(const String&) = delete;
        String& operator=(const String&) = delete;
        String(String&& other) noexcept : data(std::exchange(other.data, nullptr)) {}
        String& operator=(String&& other) noexcept
        {
            if (this != &other)
            {
                Reset();
                data = std::exchange(other.data, nullptr);
            }
            return *this;
        }
        ~String() { Reset(); }

        bool Assign(const std::string& text)
        {
            Reset();
            if (text.size() > static_cast<size_t>((std::numeric_limits<int32_t>::max)()) - 9)
                return false;
            data = static_cast<StringData*>(Allocate(text.size() + 9));
            if (!data)
                return false;
            // Fresh, uniquely owned native allocation: this is initialization,
            // not a patch of an existing game object or a borrowed string.
            data->references = 1;
            data->length = static_cast<uint32_t>(text.size());
            std::memcpy(data->text, text.data(), text.size());
            data->text[text.size()] = 0;
            return true;
        }

        StringData* Get() const { return data; }

        void Reset()
        {
            if (data)
            {
                auto current = std::exchange(data, nullptr);
                Function<void(__thiscall*)(StringData*)>(0x4050E0)(current);
            }
        }
    private:
        StringData* data = nullptr;
    };

    inline void Send(void* widget, Message& message)
    {
        Virtual<uint8_t(__thiscall*)(void*, Message*)>(widget, 108)(widget, &message);
    }

    inline void* ConstructWindow(void* memory, void* scene, void* frame)
    {
        return Function<void*(__thiscall*)(void*, void*, void*)>(0x668120)(memory, scene, frame);
    }

    inline bool RealizeWindow(void* window, Page& description)
    {
        return Function<uint8_t(__thiscall*)(void*, Page*)>(0x668470)(window, &description) != 0;
    }

    // AL is not a creation guarantee: 65D150 returns one even after deleting a
    // failed control. The host must verify its exact appended child identity.
    inline void RealizeControl(void* parent, void* assetRoot, void* parentFrame, Child& description)
    {
        Function<uint8_t(__thiscall*)(void*, void*, void*, Child*)>(0x65D150)(parent, assetRoot, parentFrame, &description);
    }

    inline bool FontStyle(void* text, uint32_t style)
    {
        return Function<uint8_t(__thiscall*)(void*, uint32_t)>(0x644CF0)(text, style) != 0;
    }

    inline int64_t FontHeight(void* text)
    {
        return Function<int64_t(__thiscall*)(void*)>(0x644EB0)(text);
    }

    inline int32_t FontWidth(void* text, StringData* value, int32_t height)
    {
        // Machine stores incoming ECX, then reuses it for each glyph. HexRays'
        // inferred standalone stdcall signature omits that required receiver.
        return Function<int32_t(__thiscall*)(void*, StringData**, int32_t)>(0x644D60)(text, &value, height);
    }

    inline bool SetText(void* text, StringData* value, uint32_t style, int32_t line = 0)
    {
        return Function<uint8_t(__thiscall*)(void*, StringData**, uint32_t, int32_t)>(0x6443C0)(text, &value, style, line) != 0;
    }

    inline void DestroyWidget(void* widget)
    {
        Virtual<void*(__thiscall*)(void*, uint32_t)>(widget, 128)(widget, 1);
    }

    inline void* CreateFrame(void* driver, uint32_t type)
    {
        return Virtual<void*(__stdcall*)(void*, uint32_t)>(driver, 84)(driver, type);
    }

    inline void* CreateVisual(void* driver, uint32_t fourcc)
    {
        return Virtual<void*(__stdcall*)(void*, uint32_t)>(driver, 80)(driver, fourcc);
    }

    inline void* FindFrame(void* frame, const char* name, uint32_t flags)
    {
        return Virtual<void*(__stdcall*)(void*, const char*, uint32_t)>(frame, 72)(frame, name, flags);
    }

    inline int32_t CopyFrame(void* destination, void* source)
    {
        return Virtual<int32_t(__stdcall*)(void*, void*)>(destination, 76)(destination, source);
    }

    inline int32_t LinkFrame(void* frame, void* parent, uint32_t flags = 0)
    {
        // This is an intrusive parent link, not a retained reference. Own the
        // allocation separately and detach it before native parent teardown.
        return Virtual<int32_t(__stdcall*)(void*, void*, uint32_t)>(frame, 56)(frame, parent, flags);
    }

    inline void PositionFrame(void* frame, const Vector& position)
    {
        Virtual<int32_t(__stdcall*)(void*, const Vector*)>(frame, 8)(frame, &position);
    }

    inline void NameFrame(void* frame, const char* name)
    {
        Virtual<int32_t(__stdcall*)(void*, const char*)>(frame, 52)(frame, name);
    }

    inline void EnableFrame(void* frame, bool enabled)
    {
        Virtual<int32_t(__stdcall*)(void*, uint32_t)>(frame, 48)(frame, enabled ? 1u : 0u);
    }

    inline void ReleaseFrame(void* frame)
    {
        if (frame)
            Virtual<int32_t(__stdcall*)(void*)>(frame, 0)(frame);
    }
}
