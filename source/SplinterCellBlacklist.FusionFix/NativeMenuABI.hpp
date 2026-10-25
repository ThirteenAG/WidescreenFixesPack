#pragma once
#include <cstdint>
#include <cstddef>
#include <string>

namespace BlacklistMenuABI
{
    // Native TString<wchar_t> input and ListItemMenuCycler choice. These are
    // borrowed arguments only; the game copies them with its own allocator.
    struct Text
    {
        const wchar_t* data;
        int32_t count;
        int32_t capacity;
        explicit Text(const std::wstring& value)
            : data(value.c_str()), count(static_cast<int32_t>(value.size() + 1)), capacity(count) {}
    };
    struct Choice { Text text; int32_t value; };
    struct Slot
    {
        void* signal = nullptr;
        int32_t emitting = 0;
        uint8_t enabled = 1;
        uint8_t padding[3]{};
        void* owner = nullptr;
        void* callback = nullptr;
    };
    static_assert(sizeof(Text) == 12 && sizeof(Choice) == 16 && sizeof(Slot) == 20);
    static_assert(offsetof(Slot, owner) == 12 && offsetof(Slot, callback) == 16);

    inline void* clearMap9 = nullptr;
    inline void* hideRows9 = nullptr;
    inline void* cloneChoice9 = nullptr;
    inline void* clearChoices9 = nullptr;
    inline void* register9 = nullptr;
    inline void* registerInt9 = nullptr;
    inline void* popTier9 = nullptr;
    inline void* switchTier9 = nullptr;

    // DX9 whole-program optimization passes these private arguments in ESI,
    // EDI or EAX. Preserve the caller's nonvolatile registers explicitly.
    __declspec(naked) static void __stdcall ClearMap9(void*)
    {
        __asm {
            push esi
            mov esi, [esp + 8]
            call dword ptr [clearMap9]
            pop esi
            ret 4
        }
    }
    __declspec(naked) static void __stdcall HideRows9(void*)
    {
        __asm {
            push edi
            mov edi, [esp + 8]
            call dword ptr [hideRows9]
            pop edi
            ret 4
        }
    }
    __declspec(naked) static int32_t __stdcall CloneChoice9(void*, const Choice*)
    {
        __asm {
            push edi
            mov edi, [esp + 12]
            push dword ptr [esp + 8]
            call dword ptr [cloneChoice9]
            pop edi
            ret 8
        }
    }
    __declspec(naked) static void __stdcall ClearChoices9(void*)
    {
        __asm {
            push edi
            mov edi, [esp + 8]
            call dword ptr [clearChoices9]
            pop edi
            ret 4
        }
    }
    __declspec(naked) static void __stdcall Register9(void*, Slot*)
    {
        __asm {
            push esi
            mov esi, [esp + 8]
            push dword ptr [esp + 12]
            call dword ptr [register9]
            pop esi
            ret 8
        }
    }
    __declspec(naked) static void __stdcall RegisterInt9(void*, Slot*)
    {
        __asm {
            push esi
            mov esi, [esp + 8]
            push dword ptr [esp + 12]
            call dword ptr [registerInt9]
            pop esi
            ret 8
        }
    }
    __declspec(naked) static void __stdcall PopTier9(void*)
    {
        __asm {
            mov eax, [esp + 4]
            call dword ptr [popTier9]
            ret 4
        }
    }
    __declspec(naked) static void __stdcall SwitchTier9(void*)
    {
        __asm {
            push edi
            mov edi, [esp + 8]
            call dword ptr [switchTier9]
            pop edi
            ret 4
        }
    }

    struct Bridge
    {
        bool dx9 = false;
        void* addRow = nullptr;
        void* addTier = nullptr;
        void* clearList = nullptr;
        void* rehash = nullptr;
        void* resizeArray = nullptr;
        void* selectRow = nullptr;
        void* cloneChoice = nullptr;
        void* clearChoices = nullptr;
        void* connect = nullptr;
        void* connectInt = nullptr;
        void* disconnect = nullptr;
        void* pushTier = nullptr;
        void* popTier = nullptr;
        void* historyPush = nullptr;
        void* setTitle = nullptr;
        void* currentLanguage = nullptr;
        void* record = nullptr;
        void* gameMode = nullptr;
        void* levelInfo = nullptr;
        void* topFocus = nullptr;
        void* control = nullptr;

        template<class T> static T Read(void* object, ptrdiff_t offset)
        {
            return *reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset);
        }
        template<class T> static void Write(void* object, ptrdiff_t offset, T value)
        {
            ProtectedGameRef<T>(reinterpret_cast<T*>(static_cast<uint8_t*>(object) + offset)) = value;
        }
        static void* At(void* object, ptrdiff_t offset) { return static_cast<uint8_t*>(object) + offset; }
        template<class Return, class... Args> static Return Virtual(void* object, size_t byteOffset, Args... args)
        {
            auto table = Read<void**>(object, 0);
            return reinterpret_cast<Return(__thiscall*)(void*, Args...)>(table[byteOffset / 4])(object, args...);
        }

        int32_t AddRow(void* list, const std::wstring& label, const int32_t* command, int32_t kind) const
        {
            if (dx9)
                return reinterpret_cast<int32_t(__fastcall*)(int32_t, const int32_t*, void*, const wchar_t*)>(addRow)(kind, command, list, label.c_str());
            return reinterpret_cast<int32_t(__thiscall*)(void*, const wchar_t*, const int32_t*, int32_t)>(addRow)(list, label.c_str(), command, kind);
        }
        void* Record(void* list, int32_t index) const
        {
            if (index < 0 || index >= Read<int32_t>(list, 680)) return nullptr;
            auto order = Read<int32_t*>(list, 676);
            if (!order) return nullptr;
            auto key = order[index];
            if (dx9) return reinterpret_cast<void*(__fastcall*)(void*, const int32_t*)>(record)(At(list, 652), &key);
            return reinterpret_cast<void*(__thiscall*)(void*, const int32_t*)>(record)(At(list, 652), &key);
        }
        int32_t GameMode() const
        {
            if (!dx9) return reinterpret_cast<int32_t(__cdecl*)()>(gameMode)();
            auto level = reinterpret_cast<void*(__cdecl*)()>(levelInfo)();
            auto mode = level ? Read<void*>(level, 1744) : nullptr;
            return mode ? Read<uint8_t>(mode, 892) : 11;
        }
        void* Control(void* widget) const
        {
            // RTTI identifies the native row wrapper as UI::ListItem. Its
            // GetControl body only reads the current kind's native pointer map.
            auto table = Read<void**>(widget, 0);
            if (!table || table[380 / 4] != control || Read<uint32_t>(widget, 616) > 22) return nullptr;
            return reinterpret_cast<void*(__thiscall*)(void*)>(control)(widget);
        }
        static void* Widget(void* list, int32_t index)
        {
            if (index < 0 || index >= Read<int32_t>(list, 644)) return nullptr;
            auto pool = Read<uint8_t*>(list, 640);
            return pool ? Read<void*>(pool, 12 * index) : nullptr;
        }
        void AddTier(void* tiered) const
        {
            reinterpret_cast<void(__thiscall*)(void*)>(addTier)(tiered);
        }
        void Clear(void* list) const
        {
            Write<uint8_t>(list, 696, 0);
            if (!dx9)
            {
                reinterpret_cast<void(__thiscall*)(void*)>(clearList)(list);
                return;
            }
            Write<int32_t>(list, 688, 0);
            ClearMap9(At(list, 652));
            Write<int32_t>(list, 668, 0);
            Write<int32_t>(list, 672, 0);
            reinterpret_cast<void(__thiscall*)(void*)>(rehash)(At(list, 652));
            Write<int32_t>(list, 680, 0);
            if (Read<int32_t>(list, 684))
            {
                Write<int32_t>(list, 684, 0);
                reinterpret_cast<void(__thiscall*)(void*, int32_t)>(resizeArray)(At(list, 676), 4);
            }
            reinterpret_cast<void(__thiscall*)(void*, int32_t, int32_t)>(selectRow)(list, -1, 1);
            HideRows9(list);
        }
        void ChoiceClear(void* array) const
        {
            if (dx9) ClearChoices9(array);
            else reinterpret_cast<void(__thiscall*)(void*, int32_t)>(clearChoices)(array, 0);
        }
        void ChoiceAdd(void* array, const std::wstring& label, int32_t value) const
        {
            Choice choice{ Text(label), value };
            if (dx9) CloneChoice9(array, &choice);
            else reinterpret_cast<int32_t(__thiscall*)(void*, const Choice*)>(cloneChoice)(array, &choice);
        }
        void Connect(void* signal, Slot& slot, bool integer) const
        {
            if (dx9)
            {
                if (integer) RegisterInt9(signal, &slot);
                else Register9(signal, &slot);
            }
            else reinterpret_cast<void(__thiscall*)(void*, Slot*)>(integer ? connectInt : connect)(signal, &slot);
        }
        void Disconnect(Slot& slot) const
        {
            if (slot.signal)
                reinterpret_cast<void(__thiscall*)(void*, Slot*)>(disconnect)(slot.signal, &slot);
            slot.signal = nullptr;
            slot.emitting = 0;
        }
        void PopTier(void* tiered) const
        {
            if (dx9) PopTier9(tiered);
            else reinterpret_cast<void(__thiscall*)(void*)>(popTier)(tiered);
        }
        void PushTier(void* tiered, int32_t index) const
        {
            if (!dx9)
            {
                reinterpret_cast<void(__thiscall*)(void*, int32_t)>(pushTier)(tiered, index);
                return;
            }
            // The stock DX9 Perform route first prepares one of its twelve
            // tiers. Our separately appended tier must never reach that
            // fixed scene array, so use the history/transition calls directly.
            reinterpret_cast<int32_t(__thiscall*)(void*, void*)>(historyPush)(At(tiered, 600), At(tiered, 592));
            Write<int32_t>(tiered, 596, index);
            SwitchTier9(tiered);
        }
        bool Caption(void* scene, ptrdiff_t elementOffset, const std::wstring& caption) const
        {
            auto element = Read<void*>(scene, elementOffset);
            if (!element) return false;
            auto facade = Read<void*>(element, 464); // native PropertyText, slot 5
            if (!facade) return false;
            Text input(caption);
            if (dx9)
                reinterpret_cast<void(__stdcall*)(void*, const Text*, int32_t, float, float)>(setTitle)(facade, &input, 0, 0.0f, 0.0f);
            else
                reinterpret_cast<void(__thiscall*)(void*, const Text*, int32_t, float, float)>(setTitle)(facade, &input, 0, 0.0f, 0.0f);
            return true;
        }
    };
}
