#pragma once

#include <cstddef>
#include <cstdint>

namespace CthulhuMenu::Native
{
    // Native menu functions and object sizes of the supported executable.
    inline constexpr size_t PageSize = 2240;
    inline constexpr size_t TextRowSize = 360;
    inline constexpr size_t TargetCapacity = 256;

    inline void* AllocateBuffer(size_t size, int category, const char* file, int line, int flags)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t, int, const char*, int, int)>(0x40C700)(
            size, category, file, line, flags);
    }

    inline void FreeBuffer(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(0x40C750)(memory);
    }

    inline void* AllocateObject(size_t size, int category, const char* file, int line, int flags)
    {
        return reinterpret_cast<void* (__cdecl*)(size_t, int, const char*, int, int)>(0x40C690)(
            size, category, file, line, flags);
    }

    inline void FreeObject(void* memory)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(0x40C6E0)(memory);
    }

    inline void* ConstructOptions(void* allocation)
    {
        return reinterpret_cast<void* (__thiscall*)(void*)>(0x41BF30)(allocation);
    }

    inline void* AddRow(void* page, const char* type)
    {
        return reinterpret_cast<void* (__thiscall*)(void*, const char*)>(0x417910)(page, type);
    }

    inline void Action(void* row, int action, const char* target, int extra)
    {
        // The native warning does not prevent an oversized copy. Callers must
        // validate the complete target length before entering this method.
        reinterpret_cast<void (__thiscall*)(void*, int, const char*, int)>(0x4196A0)(
            row, action, target, extra);
    }

    inline void Label(void* row, int resource)
    {
        reinterpret_cast<void (__thiscall*)(void*, int)>(0x419A30)(row, resource);
    }

    inline void Bounds(void* page, const float* rectangle)
    {
        reinterpret_cast<void (__thiscall*)(void*, const float*)>(0x416400)(page, rectangle);
    }

    inline void HeadingBounds(void* page, const float* rectangle)
    {
        reinterpret_cast<void (__thiscall*)(void*, const float*)>(0x416440)(page, rectangle);
    }

    inline void BuildRenderers(void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(0x417320)(page);
    }

    inline void ClearRows(void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(0x416EF0)(page);
    }

    inline void CopyText(void* renderer, const char* text)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*)>(0x43AAC0)(renderer, text);
    }

    inline void LayoutText(void* renderer)
    {
        reinterpret_cast<void (__thiscall*)(void*)>(0x43AE10)(renderer);
    }

    inline void HeaderBack(void* page, int configuration, int action, const char* target)
    {
        reinterpret_cast<void (__thiscall*)(void*, int, int, const char*)>(0x4160F0)(
            page, configuration, action, target);
    }

    inline void HeaderStart(void* page, int configuration, int action, const char* target)
    {
        // thiscall with three stack arguments (ret 12).
        reinterpret_cast<void (__thiscall*)(void*, int, int, const char*)>(0x416190)(
            page, configuration, action, target);
    }

    inline void Select(void* page, int direction, int quiet)
    {
        reinterpret_cast<int (__thiscall*)(void*, int, int)>(0x417180)(page, direction, quiet);
    }

    inline void RemoveHistory(void* page)
    {
        reinterpret_cast<void (__cdecl*)(void*)>(0x417C50)(page);
    }

    inline void* Manager()
    {
        return reinterpret_cast<void* (__cdecl*)()>(0x40FED0)();
    }

    inline void Publish(void* manager, const char* name, void* page)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*, void*)>(0x40FD70)(manager, name, page);
    }

    inline void ErasePage(void* manager, void* node)
    {
        void* next = nullptr;
        reinterpret_cast<void* (__thiscall*)(void*, void**, void*)>(0x4201B0)(
            static_cast<uint8_t*>(manager) + 60, &next, node);
    }

    inline void DeleteOptions(void* page)
    {
        reinterpret_cast<void* (__thiscall*)(void*, unsigned)>(0x41BF50)(page, 1);
    }

    inline void ShowPage(void* manager, const char* name, int mode, int flags)
    {
        reinterpret_cast<void (__thiscall*)(void*, const char*, int, int)>(0x40F450)(
            manager, name, mode, flags);
    }
}
