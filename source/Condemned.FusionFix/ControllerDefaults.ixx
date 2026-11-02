module;

#include <stdafx.h>
#include <array>
#include <bit>
#include <cfloat>
#include <mutex>

export module ControllerDefaults;

import Xidi;

namespace
{
    struct Binding
    {
        const char* action;
        const wchar_t* object;
        bool axis;
        float offset;
        float scale;
    };

    constexpr std::array<Binding, 18> Bindings{{
        { "Activate", L"(A)", false, 0.0f, 1.0f },
        { "Ammo Check", L"(Y)", false, 0.0f, 1.0f },
        { "Block", L"(LT)", false, 0.0f, 1.0f },
        { "D-Pad Up", L"(D-pad Up)", false, 0.0f, 1.0f },
        { "D-Pad Down", L"(D-pad Down)", false, 0.0f, 1.0f },
        { "D-Pad Left", L"(D-pad Left)", false, 0.0f, 1.0f },
        { "D-Pad Right", L"(D-pad Right)", false, 0.0f, 1.0f },
        { "Fire", L"(RT)", false, 0.0f, 1.0f },
        { "Flashlight", L"(B)", false, 0.0f, 1.0f },
        { "Run", L"(LS)", false, 0.0f, 1.0f },
        { "Kick", L"(RS)", false, 0.0f, 1.0f },
        { "Taser", L"(LB)", false, 0.0f, 1.0f },
        { "Toggle Melee", L"(RB)", false, 0.0f, 1.0f },
        { "Tools", L"(X)", false, 0.0f, 1.0f },
        { "ForwardAxis", L"(Left Stick Y)", true, 1.4f, -2.8f },
        { "StrafeAxis", L"(Left Stick X)", true, -1.4f, 2.8f },
        { "PitchAccel", L"(Right Stick Y)", true, -1.4f, 2.8f },
        { "YawAccel", L"(Right Stick X)", true, -1.4f, 2.8f },
    }};

    constexpr std::array<const char*, 10> Fields{{
        "Bindings.0.Device", "Bindings.0.Object", "Bindings.0.Default", "Bindings.0.Offset", "Bindings.0.Scale",
        "Bindings.0.DeadZoneMin", "Bindings.0.DeadZoneMax", "Bindings.0.DeadZoneValue",
        "Bindings.0.CommandMin", "Bindings.0.CommandMax",
    }};

    struct Attribute
    {
        void* name;
        uint32_t flags;
        uint16_t count;
        uint8_t type;
        uint8_t padding;
        uint32_t* values;
    };
    static_assert(sizeof(Attribute) == 16);

    struct Defaults
    {
        std::array<Attribute*, Bindings.size() * Fields.size()> attributes{};
        std::array<uint32_t*, Bindings.size() * Fields.size()> originalValues{};
        std::array<std::array<uint32_t, 2>, Bindings.size() * Fields.size()> values{};
        bool applied = false;

        ~Defaults()
        {
            if (!applied) return;
            for (size_t i = 0; i < attributes.size(); ++i)
            {
                attributes[i]->values = originalValues[i];
                attributes[i]->count = 1;
            }
        }
    };

    GameRef<uintptr_t> pGameDatabase;
    SafetyHookInline loadBindings;
    std::mutex defaultsMutex;

    using Find = void* (__thiscall*)(void*, void*, const char*);

    void* FindItem(void* manager, size_t offset, void* parent, const char* name)
    {
        const auto table = *reinterpret_cast<void***>(manager);
        return reinterpret_cast<Find>(table[offset / sizeof(void*)])(manager, parent, name);
    }

    void AddDefaults(void* manager, void* database, Defaults& state)
    {
        const auto category = FindItem(manager, 28, database, "Bindings");
        if (!category)
            throw std::runtime_error("Condemned: default controller bindings category is missing");
        for (size_t i = 0; i < Bindings.size(); ++i)
        {
            const auto record = FindItem(manager, 56, category, Bindings[i].action);
            if (!record)
                throw std::runtime_error("Condemned: default controller action is missing");
            for (size_t j = 0; j < Fields.size(); ++j)
            {
                const auto k = i * Fields.size() + j;
                auto attribute = static_cast<Attribute*>(FindItem(manager, 88, record, Fields[j]));
                if (!attribute || !attribute->values)
                    throw std::runtime_error("Condemned: default controller attribute is missing");
                if (attribute->count != 1 || attribute->type != (j < 2 ? 5 : 2))
                    throw std::runtime_error("Condemned: unexpected default controller attribute");
                state.attributes[k] = attribute;
                state.originalValues[k] = attribute->values;
                state.values[k][0] = attribute->values[0];
                uint32_t value = attribute->values[0];
                if (j == 0)
                    value = reinterpret_cast<uint32_t>(L"Xidi Virtual Controller 1");
                else if (j == 1)
                    value = reinterpret_cast<uint32_t>(Bindings[i].object);
                else if (Bindings[i].axis)
                {
                    switch (j)
                    {
                        case 2: value = std::bit_cast<uint32_t>(0.0f); break;
                        case 3: value = std::bit_cast<uint32_t>(Bindings[i].offset); break;
                        case 4: value = std::bit_cast<uint32_t>(Bindings[i].scale); break;
                        case 5: value = std::bit_cast<uint32_t>(-0.4f); break;
                        case 6: value = std::bit_cast<uint32_t>(0.4f); break;
                        case 7: value = std::bit_cast<uint32_t>(0.0f); break;
                        case 8: value = std::bit_cast<uint32_t>(0.1f); break;
                        case 9: value = std::bit_cast<uint32_t>(FLT_MAX); break;
                    }
                }
                state.values[k][Bindings[i].axis ? 0 : 1] = value;
            }
        }
        for (size_t i = 0; i < Bindings.size(); ++i)
        {
            for (size_t j = 0; j < Fields.size(); ++j)
            {
                const auto k = i * Fields.size() + j;
                state.attributes[k]->values = state.values[k].data();
                state.attributes[k]->count = Bindings[i].axis ? 1 : 2;
            }
        }
        state.applied = true;
    }

    int __fastcall LoadBindings(void* profile, void*, void* database)
    {
        const auto name = *reinterpret_cast<const char* const*>(static_cast<uint8_t*>(database) + 4);
        if (!name || _stricmp(name, "ProfileDatabase\\Defaults.Gamdb00p") != 0)
            return loadBindings.unsafe_thiscall<int>(profile, database);
        const std::lock_guard lock(defaultsMutex);
        InitXidi();
        UpdateXidi();
        Defaults defaults;
        AddDefaults(reinterpret_cast<void*>(pGameDatabase.get()), database, defaults);
        return loadBindings.unsafe_thiscall<int>(profile, database);
    }
}

class ControllerDefaults
{
public:
    ControllerDefaults()
    {
        WFP::onGameInitEvent() += []()
        {
            auto gameClient = GetModuleHandleW(L"GameClient.dll");
            auto pattern = hook::pattern(gameClient, "81 EC 98 00 00 00 8B 94 24 9C 00 00 00 53 55 56 57 8B D9 8B 0D ? ? ? ? 8B 01 68 ? ? ? ? 52 FF 50 1C");
            pGameDatabase.SetAddress(*pattern.get_first<uintptr_t*>(21));
            loadBindings = safetyhook::create_inline(pattern.get_first(), LoadBindings);
            if (!loadBindings)
                throw std::runtime_error("Condemned: could not install the default controller binding hook");
        };
    }
} ControllerDefaults;

