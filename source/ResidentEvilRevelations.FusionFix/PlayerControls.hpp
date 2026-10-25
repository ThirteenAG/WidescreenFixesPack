#pragma once

namespace RevPlayer
{
    template<typename T>
    const T& Field(uintptr_t object, size_t offset)
    {
        return *reinterpret_cast<const T*>(object + offset);
    }

    template<typename T>
    void Write(uintptr_t object, size_t offset, T value)
    {
        *reinterpret_cast<T*>(object + offset) = value;
    }

    inline uintptr_t* pSystem{};
    inline bool (__thiscall* IsOnline)(uintptr_t){};
    inline bool (__thiscall* ChangeState)(uintptr_t, int, int, void*){};

    inline bool Init()
    {
        static const bool ready = []()
        {
            auto online = find_pattern("E8 ? ? ? ? 85 C0 75 03 32 C0 C3 8A 80 88 AA 00 00 24 01 C3");
            auto system = find_pattern("56 8B F1 8B 0D ? ? ? ? E8 ? ? ? ? 3C 01 75 ? 8B 76 70");
            auto change = find_pattern("56 8B F1 80 7E 2D 01 75 06 32 C0 5E C2 0C 00 83 7E 30 00 8D 4E 30");
            if (online.size() != 1 || system.size() != 1 || change.size() != 1)
                return false;
            pSystem = *system.get_first<uintptr_t*>(5);
            IsOnline = reinterpret_cast<decltype(IsOnline)>(online.get_first());
            ChangeState = reinterpret_cast<decltype(ChangeState)>(change.get_first());
            return true;
        }();
        return ready;
    }

    inline const char* TypeName(uintptr_t object)
    {
        if (!object)
            return nullptr;
        auto table = Field<uintptr_t*>(object, 0);
        auto type = reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(table[4])(object);
        return type ? Field<const char*>(type, 4) : nullptr;
    }

    inline bool Local(uintptr_t ch)
    {
        return ch && Field<uint32_t>(ch, 0xE30) == 1 && Field<float>(ch, 0xE38) > 0.0f &&
            ((Field<uint32_t>(ch, 12) & 7) == 1 || (Field<uint32_t>(ch, 12) & 7) == 2) && Field<uintptr_t>(ch, 0xE80);
    }

    inline bool Offline(uintptr_t ch)
    {
        return Local(ch) && pSystem && *pSystem && IsOnline && !IsOnline(*pSystem);
    }

    inline int Lower(uintptr_t ch)
    {
        auto machine = Field<uintptr_t>(ch, 0xE80);
        return machine ? Field<int>(machine, 60) : -1;
    }

    inline int Upper(uintptr_t ch)
    {
        return Field<int>(ch, 3948 + 60);
    }

    inline double Now()
    {
        static const double frequency = []()
        {
            LARGE_INTEGER value{};
            QueryPerformanceFrequency(&value);
            return static_cast<double>(value.QuadPart);
        }();
        LARGE_INTEGER value{};
        QueryPerformanceCounter(&value);
        return static_cast<double>(value.QuadPart) / frequency;
    }

    template<typename Hook>
    void SetEnabled(Hook& hook, bool enabled)
    {
        if (hook)
        {
            if (enabled)
                (void)hook.enable();
            else
                (void)hook.disable();
        }
    }
}
