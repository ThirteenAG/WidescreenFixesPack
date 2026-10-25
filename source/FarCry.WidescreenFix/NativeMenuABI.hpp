#pragma once
#include <Windows.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

// CryScriptSystem has separate native layouts for 32-bit and 64-bit builds.
namespace FarCryMenu
{
    using ScriptFunction = int (__cdecl*)(void*);
#ifdef _WIN64
    constexpr uintptr_t scriptTableRva = 0x43490, handlerTableRva = 0x42460, objectTableRva = 0x430C0;
    constexpr size_t integerParamSlot = 11, booleanReturnSlot = 21, stringParamSlot = 9;
#else
    constexpr uintptr_t scriptTableRva = 0x18D50, handlerTableRva = 0x18410, objectTableRva = 0x18668;
    constexpr size_t integerParamSlot = 10, booleanReturnSlot = 20, stringParamSlot = 8;
#endif
    inline bool Readable(const void* address, size_t length)
    {
        if (!address || !length) return false;
        auto current = reinterpret_cast<uintptr_t>(address);
        if (length > UINTPTR_MAX - current) return false;
        const auto end = current + length;
        while (current < end)
        {
            MEMORY_BASIC_INFORMATION info{};
            if (!VirtualQuery(reinterpret_cast<void*>(current), &info, sizeof(info)) || info.State != MEM_COMMIT ||
                (info.Protect & (PAGE_NOACCESS | PAGE_GUARD))) return false;
            const auto next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
            if (next <= current) return false;
            current = next;
        }
        return true;
    }
    template<typename R, typename... A>
    inline R Virtual(void* object, size_t slot, A... arguments)
    {
#ifdef _WIN64
        using Function = R (__fastcall*)(void*, A...);
#else
        using Function = R (__thiscall*)(void*, A...);
#endif
        return reinterpret_cast<Function>((*static_cast<uintptr_t**>(object))[slot])(object, arguments...);
    }
    inline bool Table(void* object, HMODULE module, uintptr_t rva)
    {
        return module && Readable(object, sizeof(void*)) &&
            *static_cast<uintptr_t*>(object) == reinterpret_cast<uintptr_t>(module) + rva;
    }
    inline void* Handler(void* script) { return Virtual<void*>(script, 0); }
    inline void* State(void* script) { return Virtual<void*>(script, 1); }
    inline bool Execute(void* script, const std::string& buffer)
    {
        return Virtual<bool>(script, 3, buffer.c_str(), buffer.size());
    }
    inline void* CreateGlobal(void* script, const char* name) { return Virtual<void*>(script, 12, name); }
    inline bool NamespaceFree(void* script, HMODULE module)
    {
        // Installed slot9 creates a temporary wrapper around the globals;
        // object slot40 returns svtNull (0) for an absent/nil member. Neither
        // string-only probes nor ExecuteBuffer success can detect collisions.
        auto globals = Virtual<void*>(script, 9);
        if (!Table(globals, module, objectTableRva)) return false;
        bool free = true;
        for (const auto* name : {"WFP_Bridge", "WFP_Menu", "WFP_FindScreen", "WFP_FindWidget",
            "WFP_Identity", "WFP_OwnRow", "WFP_Command", "WFP_Key", "WFP_Focus", "WFP_Hover",
            "WFP_Released", "WFP_Entry", "WFP_Rect", "WFP_RectMatches", "WFP_Constructing",
            "WFP_Prepare", "WFP_Snapshot", "WFP_Commit", "WFP_Enter", "WFP_Select", "WFP_Describe",
            "WFP_Leave", "WFP_Retire", "g_WFPMenuStatus", "g_WFPMenuSelected", "g_WFPMenuPause", "g_WFPMenuResult"})
        {
            if (Virtual<int>(globals, 40, name) != 0) { free = false; break; }
        }
        Virtual<void>(globals, 78);
        return free;
    }

    inline bool AddFunction(void* object, const char* name, ScriptFunction function, int id)
    {
        // Non-overloaded slot74 retains the cdecl thunk in a native Lua closure.
        return Virtual<bool>(object, 74, name, function, id);
    }
    inline void ReleaseObject(void* object) { Virtual<void>(object, 78); }
    inline void Attach(void* handler, void* state) { Virtual<void>(handler, 0, state); }
    inline int ParamCount(void* handler) { return Virtual<int>(handler, 3); }
    inline bool Integer(void* handler, int index, int& value)
    {
        // MSVC orders these overload groups differently from lexical header
        // order. Installed32 slot10 and64 slot11 are proved integer getters.
        return Virtual<bool>(handler, integerParamSlot, index, &value);
    }
    inline bool String(void* handler, int index, std::string& value, size_t limit = 64)
    {
        const char* text = nullptr;
        if (!Virtual<bool>(handler, stringParamSlot, index, &text) || !text) return false;
        value.clear();
        for (size_t offset = 0; offset <= limit; ++offset)
        {
            if (!Readable(text + offset, 1)) return false;
            if (!text[offset]) return true;
            if (offset == limit) return false;
            value += text[offset];
        }
        return false;
    }
    inline void* NativeParameter(void* script, void* handler, int index, HMODULE module)
    {
        // Both installed variants have CreateEmptyObject at10, object param
        // at6 and GetNativeData at69. Release returns only this temporary
        // wrapper to the pool; its parent sink is null and borrowed controls
        // retain their native lifetime. All use sites revalidate membership.
        auto object = Virtual<void*>(script, 10);
        if (!Table(object, module, objectTableRva)) return nullptr;
        void* native = nullptr;
        if (Virtual<bool>(handler, 6, index, object)) native = Virtual<void*>(object, 69);
        ReleaseObject(object);
        return native;
    }
    inline int Return(void* handler, bool accepted) { return Virtual<int>(handler, booleanReturnSlot, accepted); }
    inline std::string NativeLanguage(void* script)
    {
        const char* value = nullptr;
        // Actual32/64 slot35 obtains the rooted global string and pops its
        // temporary Lua stack entry. Copy it before any foreign/API operation.
        if (!Virtual<bool>(script, 35, "g_language", &value) || !value) return "en";
        std::string language;
        for (size_t index = 0; index < 32; ++index)
        {
            if (!Readable(value + index, 1)) return "en";
            const auto c = value[index];
            if (!c) break;
            language += c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c;
        }
        for (const auto& pair : { std::pair{"english", "en"}, {"german", "de"}, {"french", "fr"},
            {"italian", "it"}, {"spanish", "es"}, {"russian", "ru"}, {"polish", "pl"}, {"japanese", "ja"} })
            if (language == pair.first) return pair.second;
        return "en";
    }
}
