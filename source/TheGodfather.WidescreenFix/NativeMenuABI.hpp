#pragma once
#include <algorithm>
#include <initializer_list>
#include <string>

namespace GodfatherMenu
{
    struct Value;
    struct String
    {
        void* data{};
        explicit String(const char* text);
        ~String();
        String(const String&) = delete;
        String& operator=(const String&) = delete;
    };

    struct Bindings
    {
        void(__thiscall* stringCreate)(String*, const char*){};
        void(__thiscall* stringDestroy)(String*){};
        void(__thiscall* retainRoot)(Value*){};
        void(__thiscall* releaseRoot)(Value*){};
        Value*(__cdecl* get)(Value*, const String*){};
        bool(__cdecl* set)(Value*, const String*, Value*){};
        Value*(__cdecl* function)(void*){};
        Value*(__cdecl* undefined)(){};
        Value*(__cdecl* parameter)(int){};
        Value*(__cdecl* text)(const char*){};
        Value*(__cdecl* number)(float){};
        float(__thiscall* toNumber)(Value*){};
        void(__thiscall* push)(void*, Value*){};
        void(__thiscall* pop)(void*, int){};
        Value*(__thiscall* at)(void*, int){};
        void(__thiscall* invoke)(void*, Value*, Value*, int){};
        int(__cdecl* saveScope)(){};
        void(__cdecl* restoreScope)(int){};
        void(__thiscall* transform)(Value*, int, float, uint32_t){};
        void* interpreter{};
        void** screens{};
        Value*(__cdecl* levelRoot)(int){};
        bool(__thiscall* isString)(Value*){};
        void*(__thiscall* asString)(Value*){};
        const char*(__thiscall* stringText)(void*){};
    };
    inline Bindings api;

    inline String::String(const char* text)
    {
        api.stringCreate(this, text);
    }
    inline String::~String()
    {
        api.stringDestroy(this);
    }

    inline void Reference(Value* value, bool retain)
    {
        if (!value)
            return;
        if (retain)
            api.retainRoot(value);
        else
            api.releaseRoot(value);
        auto table = *reinterpret_cast<void***>(value);
        reinterpret_cast<void(__thiscall*)(Value*)>(table[retain ? 0 : 1])(value);
    }
    inline Value* Get(Value* object, const char* key)
    {
        if (!object)
            return nullptr;
        String name(key);
        auto value = api.get(object, &name);
        return value == api.undefined() ? nullptr : value;
    }
    inline bool Set(Value* object, const char* key, Value* value)
    {
        if (!object || !value)
            return false;
        String name(key);
        return api.set(object, &name, value);
    }
    inline bool Set(Value* object, const char* key, const std::string& text)
    {
        auto value = api.text(text.c_str());
        const auto result = Set(object, key, value);
        if (value)
            api.releaseRoot(value);
        return result;
    }
    inline bool Set(Value* object, const char* key, float number)
    {
        return Set(object, key, api.number(number));
    }
    inline float Number(Value* object, const char* key)
    {
        auto value = Get(object, key);
        return value ? api.toNumber(value) : 0.0f;
    }

    // The interpreter takes arguments from the top of its own reference-counted
    // stack. Preserve the surrounding ActionScript call and leave its stack intact.
    inline Value* Call(Value* object, Value* function, std::initializer_list<Value*> arguments = {})
    {
        if (!object || !function)
            return nullptr;
        const auto count = *static_cast<int*>(api.interpreter);
        for (auto it = std::rbegin(arguments); it != std::rend(arguments); ++it)
            api.push(api.interpreter, *it);
        const auto scope = api.saveScope();
        api.invoke(api.interpreter, object, function, static_cast<int>(arguments.size()));
        api.restoreScope(scope);
        auto result = *static_cast<int*>(api.interpreter) > count ? api.at(api.interpreter, 0) : nullptr;
        Reference(result, true);
        const auto extra = *static_cast<int*>(api.interpreter) - count;
        if (extra > 0)
            api.pop(api.interpreter, extra);
        return result;
    }
    inline Value* Call(Value* object, const char* method, std::initializer_list<Value*> arguments = {})
    {
        return Call(object, Get(object, method), arguments);
    }
    inline void Execute(Value* object, const char* method, std::initializer_list<Value*> arguments = {})
    {
        Reference(Call(object, method, arguments), false);
    }

    inline void* find_pattern(const char* bytes)
    {
        auto pattern = hook::pattern(bytes);
        return pattern.size() == 1 ? pattern.get_first() : nullptr;
    }
    template <class T> bool Export(T& target, const char* name)
    {
        target = reinterpret_cast<T>(GetProcAddress(GetModuleHandleW(nullptr), name));
        return target != nullptr;
    }
    inline bool Bind()
    {
        bool valid = true;
        valid &= Export(api.stringCreate, "??0EAStringC@@QAE@QBD@Z");
        valid &= Export(api.stringDestroy, "?Invalidate@EAStringC@@QAEXXZ");
        valid &= Export(api.retainRoot, "?incGCRoot@AptValue@@QAEXXZ");
        valid &= Export(api.releaseRoot, "?decGCRoot@AptValue@@QAEXXZ");
        valid &= Export(api.get, "?GetVariable@AptExtObject@@KAPAVAptValue@@PAV2@QBVEAStringC@@@Z");
        valid &= Export(api.set, "?SetVariable@AptExtObject@@KA_NPAVAptValue@@QBVEAStringC@@QAV2@@Z");
        valid &= Export(api.function, "?CreateNewAptFunction@AptExtObject@@KAPAVAptNativeFunction@@PAX@Z");
        valid &= Export(api.undefined, "?GetUndefinedValue@AptExtObject@@KAPAVAptValue@@XZ");
        valid &= Export(api.parameter, "?GetParam@AptExtObject@@KAPAVAptValue@@H@Z");
        valid &= Export(api.push, "?stackPush@AptActionInterpreter@@QAEXQAVAptValue@@@Z");
        valid &= Export(api.pop, "?stackPop@AptActionInterpreter@@QAEXH@Z");
        valid &= Export(api.at, "?stackAt@AptActionInterpreter@@QBEPAVAptValue@@H@Z");
        valid &= Export(api.isString, "?isString@AptValue@@QBE_NXZ");
        valid &= Export(api.asString, "?c_string@AptValue@@QBEPAVAptString@@XZ");
        valid &= Export(api.stringText, "?c_str@EAStringC@@QBEPBDXZ");
        api.text =
            reinterpret_cast<decltype(api.text)>(find_pattern("53 55 56 8B 74 24 10 0F BE 06 85 C0 57 B9 C5 9D 1C 81"));
        api.invoke = reinterpret_cast<decltype(api.invoke)>(
            find_pattern("83 EC 14 53 55 8B 6C 24 28 56 8B F1 8B 1E 8B 4C 24 28 2B DD"));
        api.toNumber = reinterpret_cast<decltype(api.toNumber)>(
            find_pattern("8B 41 04 8B D0 C1 EA 04 F6 D2 F6 C2 01 74 07 D9 05"));
        api.levelRoot = reinterpret_cast<decltype(api.levelRoot)>(
            find_pattern("6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 A1 ? ? ? ? 56 33 F6 3B C6 0F "
                         "84 90 00 00 00 8B 40 30"));
        api.transform = reinterpret_cast<decltype(api.transform)>(
            find_pattern("8B 44 24 04 83 EC 14 83 F8 0B 56 8B F1 0F 87 ? ? ? ? 57 FF 24 85"));
        auto getter = reinterpret_cast<uint8_t*>(api.get);
        if (getter && getter[18] == 0xB9)
            api.interpreter = *reinterpret_cast<void**>(getter + 19);
        api.saveScope = reinterpret_cast<decltype(api.saveScope)>(
            find_pattern("A1 ? ? ? ? 8B 0D ? ? ? ? 8D 14 88 89 15 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00 C3"));
        api.restoreScope = reinterpret_cast<decltype(api.restoreScope)>(
            find_pattern("A1 ? ? ? ? 56 33 F6 85 C0 7E 24 8D 64 24 00 A1 ? ? ? ? 8B 0C B0"));
        auto number = find_pattern("D8 0D ? ? ? ? 51 D8 05 ? ? ? ? D9 1C 24 E8 ? ? ? ? 83 C4 04 5E C3");
        if (number)
            api.number = reinterpret_cast<decltype(api.number)>(
                injector::GetBranchDestination(static_cast<uint8_t*>(number) + 16).as_int());
        auto gamma =
            static_cast<uint8_t*>(find_pattern("A1 ? ? ? ? D9 05 ? ? ? ? 56 8B 70 08 85 F6 74 29 8B 86 A4 00 00 00"));
        if (gamma)
            std::memcpy(&api.screens, gamma + 1, sizeof(api.screens));
        return valid && api.text && api.invoke && api.toNumber && api.interpreter && api.number && api.saveScope &&
               api.restoreScope && api.transform && api.levelRoot && api.screens;
    }
} // namespace GodfatherMenu
