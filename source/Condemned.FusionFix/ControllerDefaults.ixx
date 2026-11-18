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

    // The profile's bindings: one vector of 96-byte bindings per command, in the order of the game's command list
    // (84-byte entries: id at +8, name at +12). A binding holds the device and object names (std::wstring at +0 and
    // +28), then Default, Offset, Scale, DeadZoneMin, DeadZoneMax, DeadZoneValue, CommandMin, CommandMax.
    constexpr const wchar_t* PadDevice = L"Xidi Virtual Controller 1";
    uint8_t** commands = nullptr;
    void* (__thiscall* constructBinding)(void*) = nullptr;
    int(__thiscall* addBinding)(void*, const void*) = nullptr;
    void(__thiscall* destroyBinding)(void*) = nullptr;
    void* (__thiscall* assignText)(void*, const wchar_t*) = nullptr;
    void(__thiscall* applyBindings)(void*) = nullptr;
    void(__thiscall* applyInputSettings)(void*) = nullptr;
    SafetyHookInline triggerName;

    const wchar_t* Text(const uint8_t* text)
    {
        const auto capacity = *reinterpret_cast<const uint32_t*>(text + 24);
        return capacity < 8 ? reinterpret_cast<const wchar_t*>(text + 4) : *reinterpret_cast<const wchar_t* const*>(text + 4);
    }

    struct BindingList
    {
        uint32_t allocator;
        uint8_t* first;
        uint8_t* last;
        uint8_t* end;
    };

    size_t CommandCount()
    {
        return commands[0] ? (commands[1] - commands[0]) / 84 : 0;
    }

    BindingList& ProfileBindings(void* profile, size_t command)
    {
        return reinterpret_cast<BindingList*>(*reinterpret_cast<uint8_t**>(static_cast<uint8_t*>(profile) + 0x150))[command];
    }

    // A profile without controller bindings (made before the controller defaults existed) gets them next to its own
    bool HasOtherDevices(void* profile)
    {
        for (size_t command = 0; command < CommandCount(); ++command)
        {
            const auto& list = ProfileBindings(profile, command);
            for (auto binding = list.first; binding && binding < list.last; binding += 96)
            {
                const auto device = Text(binding);
                if (device && _wcsicmp(device, L"Keyboard") != 0 && _wcsicmp(device, L"Mouse") != 0)
                    return true;
            }
        }
        return false;
    }

    void AddPadBindings(void* profile)
    {
        if (HasOtherDevices(profile))
            return;
        for (const auto& pad : Bindings)
        {
            size_t command = 0;
            while (command < CommandCount() && _stricmp(reinterpret_cast<const char*>(commands[0] + command * 84 + 12), pad.action) != 0)
                ++command;
            if (command == CommandCount())
                continue;
            auto& list = ProfileBindings(profile, command);
            std::array<float, 10> values{ 0.0f, 0.0f, 1.0f, FLT_MAX, -FLT_MAX, 0.0f, 0.1f, FLT_MAX, 0.0f, 0.0f };
            if (pad.axis)
                values = { 0.0f, pad.offset, pad.scale, -0.4f, 0.4f, 0.0f, 0.1f, FLT_MAX, 0.0f, 0.0f };
            else if (list.first && list.first < list.last)
                std::memcpy(values.data(), list.first + 56, sizeof(values));
            alignas(8) uint8_t binding[96]{};
            constructBinding(binding);
            addBinding(&list, binding);
            destroyBinding(binding);
            const auto added = list.last - 96;
            assignText(added, PadDevice);
            assignText(added + 28, pad.object);
            std::memcpy(added + 56, values.data(), sizeof(values));
        }
    }

    // The input system knows the controller only some time after the profile is loaded: until then its bindings
    // would be dropped, so they are added once it is there
    void* pendingProfile = nullptr;
    void** input = nullptr;

    bool PadPresent()
    {
        using FindDevice = int(__thiscall*)(void*, const wchar_t*, int*);
        int device = 0;
        return *input && reinterpret_cast<FindDevice>((*reinterpret_cast<void***>(*input))[4])(*input, PadDevice, &device) == 0;
    }

    void __fastcall ApplyBindings(void* profile)
    {
        pendingProfile = HasOtherDevices(profile) ? nullptr : profile;
        applyBindings(profile);
    }
}

// Once per frame
export void UpdateControllerDefaults()
{
    if (!pendingProfile || !input || !PadPresent())
        return;
    const auto profile = std::exchange(pendingProfile, nullptr);
    AddPadBindings(profile);
    applyBindings(profile);
    applyInputSettings(profile);
}

namespace
{
    // Button prompts: the controller's binding of the command, named for the chosen controller, after the pad was
    // used last
    const wchar_t* PadName(const wchar_t* object)
    {
        static constexpr std::array<const wchar_t*, 14> labels{ L"(A)", L"(B)", L"(X)", L"(Y)", L"(LB)", L"(RB)", L"(LT)",
            L"(RT)", L"(Back)", L"(Start)", L"(LS)", L"(RS)", L"(D-pad Up)", L"(D-pad Down)" };
        static constexpr std::array<const wchar_t*, 14> xbox{ L"A", L"B", L"X", L"Y", L"LB", L"RB", L"LT", L"RT", L"View",
            L"Menu", L"LS", L"RS", L"D-pad Up", L"D-pad Down" };
        static constexpr std::array<const wchar_t*, 14> playstation{ L"Cross", L"Circle", L"Square", L"Triangle", L"L1", L"R1",
            L"L2", L"R2", L"Share", L"Options", L"L3", L"R3", L"D-pad Up", L"D-pad Down" };
        static constexpr std::array<const wchar_t*, 14> nintendo{ L"B", L"A", L"Y", L"X", L"L", L"R", L"ZL", L"ZR", L"-", L"+",
            L"LS", L"RS", L"D-pad Up", L"D-pad Down" };
        static constexpr std::array<std::pair<const wchar_t*, const wchar_t*>, 2> sides{ { { L"(D-pad Left)", L"D-pad Left" },
            { L"(D-pad Right)", L"D-pad Right" } } };
        for (size_t i = 0; i < labels.size(); ++i)
        {
            if (_wcsicmp(object, labels[i]) == 0)
            {
                const int family = PadIcons();
                return family == 2 ? playstation[i] : family == 3 ? nintendo[i] : xbox[i];
            }
        }
        for (const auto& [label, name] : sides)
        {
            if (_wcsicmp(object, label) == 0)
                return name;
        }
        return nullptr;
    }

    const wchar_t* __fastcall TriggerName(void* profile, void*, int id)
    {
        const auto original = triggerName.unsafe_thiscall<const wchar_t*>(profile, id);
        if (!PadUsedLast())
            return original;
        for (size_t command = 0; command < CommandCount(); ++command)
        {
            if (*reinterpret_cast<const int*>(commands[0] + command * 84 + 8) != id)
                continue;
            const auto& list = ProfileBindings(profile, command);
            for (auto binding = list.first; binding && binding < list.last; binding += 96)
            {
                if (_wcsicmp(Text(binding), PadDevice) != 0)
                    continue;
                if (const auto name = PadName(Text(binding + 28)))
                {
                    static wchar_t text[64]{};
                    wcsncpy_s(text, name, _TRUNCATE);
                    return text;
                }
            }
            break;
        }
        return original;
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
            auto pattern = hook::pattern(gameClient, "81 EC 98 00 00 00 8B 94 24 9C 00 00 00 53 55 56 57 8B D9 8B 0D ? ? ? ? 8B 01 68 ? ? ? ? 52 FF 50 1C 33 ED 89 44 24 38 89 6C 24 10 89 6C 24 14 A1");
            pGameDatabase.SetAddress(*pattern.get_first<uintptr_t*>(21));
            commands = *pattern.get_first<uint8_t**>(51);
            loadBindings = safetyhook::create_inline(pattern.get_first(), LoadBindings);

            pattern = hook::pattern(gameClient, "8B B3 50 01 00 00 8B 44 24 10 8D 4C 24 48 03 F0 E8 ? ? ? ? 50 8B CE E8 ? ? ? ? 8D 4C 24 48 E8");
            constructBinding = reinterpret_cast<decltype(constructBinding)>(injector::GetBranchDestination(pattern.get_first(16)).as_int());
            addBinding = reinterpret_cast<decltype(addBinding)>(injector::GetBranchDestination(pattern.get_first(24)).as_int());
            destroyBinding = reinterpret_cast<decltype(destroyBinding)>(injector::GetBranchDestination(pattern.get_first(33)).as_int());
            // The loader's own std::wstring assignment (import)
            pattern = hook::pattern(gameClient, "83 EE 60 FF 90 88 00 00 00 50 8B CE FF 15");
            assignText = **pattern.get_first<decltype(assignText)*>(14);

            // The profile loader hands its bindings to the input system at its end
            pattern = hook::pattern(gameClient, "8B CB 89 43 1C E8 ? ? ? ? 8B CB E8");
            applyBindings = reinterpret_cast<decltype(applyBindings)>(injector::GetBranchDestination(pattern.get_first(5)).as_int());
            applyInputSettings = reinterpret_cast<decltype(applyInputSettings)>(injector::GetBranchDestination(pattern.get_first(12)).as_int());
            injector::MakeCALL(pattern.get_first(5), ApplyBindings, true);
            pattern = hook::pattern(gameClient, "A1 ? ? ? ? 8B 28 8D 4C 24 10 51 8B CE FF 15 ? ? ? ? 8B 0D ? ? ? ? 50 FF 55 10");
            input = *pattern.get_first<void**>(1);

            triggerName = safetyhook::create_inline(hook::pattern(gameClient, "81 EC 44 08 00 00 53 56 E8 ? ? ? ? 8A 0D").get_first(), TriggerName);
        };
    }
} ControllerDefaults;

