module;

#include <stdafx.h>

export module TimeOfDayLighting;

import ComVars;

// Interiors place "TimeOfDayLighting" objects around the parts that look outside (window panes, the street seen
// through them). While such an object is drawn, the console switches from the interior's lights to the outdoor
// time-of-day lighting and switches back afterwards. The PC port lost the object type: the interiors still carry
// these objects, the game finds no type for them and skips them.
namespace TimeOfDayLighting
{
    constexpr size_t ObjectBaseSize = 0xA0;     // the game's object header
    constexpr size_t KeyframeOffset = 0x190;    // the world's current time-of-day keyframe
    constexpr size_t KeyframeSize = 0x14C;
    constexpr size_t LightsSize = 0x1080;       // point lights, ambient, three directional lights

    // The object: the game's header followed by the state saved while it is drawn
    struct Object
    {
        uint8_t header[ObjectBaseSize];
        void* view;
        uint8_t interior;
        uint8_t pad[11];
        uint8_t keyframe[KeyframeSize];
        uint8_t pad2[4];
        uint8_t lights[LightsSize];
    };
    static_assert(offsetof(Object, keyframe) % 16 == 0 && offsetof(Object, lights) % 16 == 0);

    enum Message
    {
        MessageInit = 1,
        MessageDestroy = 4,
        MessageCreate = 8,
        MessageDraw = 16,       // before the object's draw with the view, after it with null
    };

    constexpr uint32_t FlagDrawMessages = 0x40;

    struct TypeNode
    {
        TypeNode* next;
        uint32_t hash;
        int (__cdecl* handler)(Object* object, int message, void* param);
        uint32_t reserved;
    };

    GameRef<TypeNode*> pObjectTypes;            // the list of script object types
    GameRef<uint8_t*> pGameState;               // interior flag at the offset below
    GameRef<uint8_t> world;                     // the time-of-day world state
    GameRef<uint8_t*> pLights;                  // the lights of the current draw
    uint32_t nInteriorOffset = 0;
    void* pObjectVTable = nullptr;

    uint32_t (__cdecl* HashName)(const char* name) = nullptr;
    void* (__cdecl* AllocObject)(size_t size) = nullptr;
    void (__thiscall* ConstructObject)(void* object) = nullptr;
    void (__thiscall* DestructObject)(void* object) = nullptr;
    void (__cdecl* FreeObject)(void* object) = nullptr;
    void (__cdecl* ResetLights)() = nullptr;
    void (__cdecl* ApplyLights)(void* view) = nullptr;
    void (__thiscall* UpdateTimeOfDay)(void* world, void* view, bool outdoors, bool sunLights) = nullptr;

    TypeNode node{};

    bool& InteriorFlag()
    {
        return *reinterpret_cast<bool*>(pGameState.get() + nInteriorOffset);
    }

    uint8_t* Keyframe()
    {
        return &world.get() + KeyframeOffset;
    }

    void BeginDraw(Object* object, void* view)
    {
        object->interior = InteriorFlag();
        InteriorFlag() = false;
        object->view = view;
        std::memcpy(object->keyframe, Keyframe(), KeyframeSize);
        std::memcpy(object->lights, pLights.get(), LightsSize);
        ResetLights();
        ApplyLights(view);
        UpdateTimeOfDay(&world.get(), view, true, true);
    }

    void EndDraw(Object* object)
    {
        InteriorFlag() = object->interior != 0;
        ResetLights();
        ApplyLights(object->view);
        std::memcpy(pLights.get(), object->lights, LightsSize);
        UpdateTimeOfDay(&world.get(), object->view, false, false);
        ApplyLights(object->view);
        std::memcpy(Keyframe(), object->keyframe, KeyframeSize);
        object->view = nullptr;
    }

    int __cdecl Handler(Object* object, int message, void* param)
    {
        switch (message)
        {
            case MessageInit:
                *reinterpret_cast<uint32_t*>(object->header + 0x1C) |= FlagDrawMessages;
                object->view = nullptr;
                return 0;
            case MessageDestroy:
                if (object)
                {
                    DestructObject(object);
                    FreeObject(object);
                }
                return -1;
            case MessageCreate:
            {
                auto created = static_cast<uint8_t*>(AllocObject(sizeof(Object)));
                if (!created)
                    return 0;
                *reinterpret_cast<void**>(created) = pObjectVTable;
                *reinterpret_cast<void**>(created + 0x18) = reinterpret_cast<void*>(&Handler);
                *reinterpret_cast<uint32_t*>(created + 0x40) = 0;
                ConstructObject(created);
                *reinterpret_cast<void**>(created) = pObjectVTable;
                return reinterpret_cast<int>(created);
            }
            case MessageDraw:
                if (param)
                {
                    BeginDraw(object, param);
                    return 1;
                }
                if (object->view)
                    EndDraw(object);
                return -1;
            default:
                return 0;
        }
    }
}

class TimeOfDayLightingInit
{
public:
    TimeOfDayLightingInit()
    {
        WFP::onInitEvent() += []()
        {
            using namespace TimeOfDayLighting;

            // The type lookup by name hash
            auto pattern = hook::pattern("8B 44 24 14 85 C0 56 8B F1 74 16 50 E8 ? ? ? ? 83 C4 04 50 B9 ? ? ? ? E8");
            HashName = reinterpret_cast<decltype(HashName)>(injector::GetBranchDestination(pattern.get_first(12)).as_int());
            pObjectTypes.SetAddress(*pattern.get_first<TypeNode**>(22));

            // Per frame: interior flag, the world's time-of-day update
            pattern = hook::pattern("A1 ? ? ? ? 8A 88 ? ? ? ? 3A CB 74 0C 38 98 ? ? ? ? 75 04 32 C0 EB 02 B0 01 50 3A CB 0F 94 C0 B9 ? ? ? ? 50 68");
            pGameState.SetAddress(*pattern.get_first<uint8_t**>(1));
            nInteriorOffset = *pattern.get_first<uint32_t>(7);
            world.SetAddress(*pattern.get_first<uint8_t*>(36));

            pattern = hook::pattern("55 8B EC 83 E4 F0 81 EC 64 06 00 00 80 7D 0C 00");
            UpdateTimeOfDay = reinterpret_cast<decltype(UpdateTimeOfDay)>(pattern.get_first());

            // Clearing the lights and handing them to a view
            pattern = hook::pattern("8B 0D ? ? ? ? E9 ? ? ? ? CC CC CC CC CC 8B 44 24 04 8B 0D ? ? ? ? 50 E8 ? ? ? ? C3");
            pLights.SetAddress(*pattern.get_first<uint8_t**>(2));
            ResetLights = reinterpret_cast<decltype(ResetLights)>(pattern.get_first());
            ApplyLights = reinterpret_cast<decltype(ApplyLights)>(pattern.get_first(16));

            // An object type's constructor and destructor
            pattern = hook::pattern("C7 06 ? ? ? ? C7 46 18 ? ? ? ? 89 74 24 04 89 46 40 89 44 24 10 E8");
            pObjectVTable = *pattern.get_first<void*>(2);
            ConstructObject = reinterpret_cast<decltype(ConstructObject)>(injector::GetBranchDestination(pattern.get_first(24)).as_int());

            pattern = hook::pattern("8B CE E8 ? ? ? ? 8B CE E8 ? ? ? ? 33 C0 8B 4C 24 04 64 89 0D 00 00 00 00 5E 83 C4 0C C2 08 00 85 F6 74 ? 8B CE E8 ? ? ? ? 56 E8");
            DestructObject = reinterpret_cast<decltype(DestructObject)>(injector::GetBranchDestination(pattern.get_first(40)).as_int());
            FreeObject = reinterpret_cast<decltype(FreeObject)>(injector::GetBranchDestination(pattern.get_first(46)).as_int());

            pattern = hook::pattern("68 A0 00 00 00 E8 ? ? ? ? 83 C4 04");
            AllocObject = reinterpret_cast<decltype(AllocObject)>(injector::GetBranchDestination(pattern.get_first(5)).as_int());

            node.hash = HashName("TimeOfDayLighting");
            node.handler = &Handler;
            node.next = pObjectTypes;
            pObjectTypes = &node;
        };
    }
} TimeOfDayLightingInit;
