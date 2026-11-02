module;
#include <stdafx.h>
#include <Windows.h>
#include <cstddef>
#include <cstdint>

export module NativeMenuABI;



// Disconnected foundation: not included by dllmain and performs no registration.
// No menu host, full traversal/deletion lease, font readiness or runtime acceptance is implied.
export namespace ParadiseMenuABI
{
    static_assert(sizeof(void*) == 4, "Paradise Cracked native ABI is Win32 only");

    struct NativeString
    {
        char* begin;
        char* end;
        char* capacity;
    };

    struct Widget
    {
        void** table;                 // 0
        NativeString name;             // 4, copied by 4044AA
        Widget* parent;                // 16, written by 45F728
        uint8_t field20;
        uint8_t field21;
        uint8_t padding22[2];
        void* field24;
        void* rectangle;               // 28, independently allocated
        void* field32;
        void* field36;
        uint32_t field40;
        NativeString field44;
        uint8_t field56;
        uint8_t padding57[3];
        void* children;                // 60, native linked-container sentinel
    };

    struct Button
    {
        Widget widget;
        uint32_t field64;
        void* images[3];               // 68/72/76
        void* sounds[3];               // 80/84/88
        uint8_t field92;
        uint8_t field93;
        uint8_t padding94[2];
    };

    struct Text
    {
        Widget widget;
        int32_t width;                 // 64
        int32_t height;                // 68
        uint8_t layout[44];            // 72..115, not an invented font type
        uint32_t field116;
        NativeString fontName;         // 120
        uint32_t fontParameter;        // 132
    };

    static_assert(sizeof(NativeString) == 12);
    static_assert(sizeof(Widget) == 64);
    static_assert(sizeof(Button) == 96);
    static_assert(sizeof(Text) == 136);
    static_assert(offsetof(Widget, parent) == 16);
    static_assert(offsetof(Widget, children) == 60);
    static_assert(offsetof(Text, fontName) == 120);

    using ButtonConstructor = Button* (__thiscall*)(Button*, const char*);
    using TextConstructor = Text* (__thiscall*)(Text*, const char*);
    using ButtonScalarDestructor = Button* (__thiscall*)(Button*, uint32_t);
    using TextScalarDestructor = Text* (__thiscall*)(Text*, uint32_t);
    using SetFont = uint32_t (__thiscall*)(Text*, const char*, uint32_t);
    using SetText = void (__thiscall*)(Text*, const char*);
    using Attach = Widget* (__thiscall*)(Widget*, Widget*);

    // Only typed addresses are returned. The future host must establish native
    // allocator, unique name/parent, loaded resource and highest lifetime leases
    // before calling any function; a root pointer does not lease its children.
    struct API
    {
        ButtonConstructor constructButton = nullptr;
        TextConstructor constructText = nullptr;
        ButtonScalarDestructor destroyButton = nullptr;
        TextScalarDestructor destroyText = nullptr;
        SetFont setFont = nullptr;
        SetText setText = nullptr;
        Attach attach = nullptr;
    };

    // Function addresses from patterns; the scalar destructors are the first slot of the vtables the
    // constructors store.
    inline const API& Bind()
    {
        static const API api = []
        {
            API result;
            auto button = hook::pattern("55 8B EC 56 57 FF 75 08 8B F1 E8 ? ? ? ? 33 C9 33 C0 88 4E 5C 88 4E 5D C7 06");
            auto text = hook::pattern("B8 ? ? ? ? E8 ? ? ? ? 51 53 56 57 FF 75 08 8B F1 89 75 F0 E8 ? ? ? ? 33 DB 8D 4E 48 89 5D FC E8 ? ? ? ? 8D 4E 78 C6 45 FC 01 E8 ? ? ? ? 8B 46 24 C7 06");
            result.constructButton = reinterpret_cast<ButtonConstructor>(button.get_first());
            result.constructText = reinterpret_cast<TextConstructor>(text.get_first());
            result.destroyButton = reinterpret_cast<ButtonScalarDestructor>(**button.get_first<uintptr_t*>(27));
            result.destroyText = reinterpret_cast<TextScalarDestructor>(**text.get_first<uintptr_t*>(57));
            result.setFont = reinterpret_cast<SetFont>(hook::pattern("55 8B EC 56 8B F1 57 8B 7D 08 83 C9 FF 33 C0 F2 AE F7 D1 49").get_first());
            result.setText = reinterpret_cast<SetText>(hook::pattern("55 8B EC 53 56 8B F1 8B 06 57 FF 50 54 8B F8 8B 06 8B CE 83").get_first());
            result.attach = reinterpret_cast<Attach>(hook::pattern("55 8B EC 51 53 56 8B 75 08 85 F6 57 8B F9 74 7E 8B 06 8B CE").get_first());
            return result;
        }();
        return api;
    }
}
