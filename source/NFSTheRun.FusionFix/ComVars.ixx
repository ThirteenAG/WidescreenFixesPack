module;

#include "stdafx.h"

export module ComVars;

export struct bVector3
{
    float x;
    float y;
    float z;
};

export struct MouseData
{
    int   DeltaX = 0;
    int   DeltaY = 0;
    int   Wheel = 0;
} g_Mouse;

export struct StickData
{
    int   RawX = 0;
    int   RawY = 0;
    float X = 0.0f;
    float Y = 0.0f;
} g_RightStick;

export bool bHideCursorForMouseLook = false;

export GameRef<HWND> hWnd;
export GameRef<void*> pRenderDevice;

export bool bBothSticksClicked = false;

export namespace fb
{
    namespace GameTimer
    {
        float actualDeltaTime = 0.0f;   // the game's time step, 1/30 s
        float frameDeltaTime = 0.0f;    // the real time between two frames
    }

    // The player's control of the car, updated while in a race; quick-time events and cutscenes take it away
    namespace Vehicle
    {
        bool bHasControl = false;
        ULONGLONG LastUpdate = 0;     // GetTickCount64 of the last update

        bool InRaceWithoutControl()
        {
            return !bHasControl && GetTickCount64() - LastUpdate < 500;
        }
    }

    GameRef<void**> g_profileOptionsManager;

    namespace ProfileOptions
    {
        int (__thiscall* getInt)(void* ProfileOptions, const char* name, int defaultValue) = nullptr;
    }

    // The engine's settings objects (GameTimeSettings, WorldRenderSettings, ...), found by the name of their type,
    // and their fields by name, from the type information the game carries. Nothing depends on a game version.
    namespace Settings
    {
        void** pManager = nullptr;
        void* (__thiscall* getContainer)(void* manager, void* typeInfo) = nullptr;

        struct ClassInfoData
        {
            const char* name;
            uint16_t flags;
            uint16_t size;
            void* module;
            uint8_t alignment;
            uint8_t fieldCount;
            uint16_t pad;
            void* super;
            void* create;
            struct FieldInfoData
            {
                const char* name;
                uint16_t flags;
                uint16_t offset;
                void* type;
            }* fields;
        };

        struct Region { uintptr_t begin, end; };

        std::vector<Region> ReadableRegions()
        {
            std::vector<Region> regions;
            auto base = (uintptr_t)GetModuleHandle(NULL);
            auto nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
            auto end = base + nt->OptionalHeader.SizeOfImage;
            MEMORY_BASIC_INFORMATION mbi;
            for (auto address = base; address < end && VirtualQuery((void*)address, &mbi, sizeof(mbi)); address = (uintptr_t)mbi.BaseAddress + mbi.RegionSize)
            {
                constexpr DWORD readable = PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
                if (mbi.State == MEM_COMMIT && (mbi.Protect & readable) && !(mbi.Protect & PAGE_GUARD))
                    regions.push_back({ (uintptr_t)mbi.BaseAddress, std::min((uintptr_t)mbi.BaseAddress + mbi.RegionSize, end) });
            }
            return regions;
        }

        bool InImage(const std::vector<Region>& regions, uintptr_t address, size_t size = 4)
        {
            for (auto& r : regions)
                if (address >= r.begin && address + size <= r.end)
                    return true;
            return false;
        }

        template<typename Fn>
        void ForEachDword(const std::vector<Region>& regions, uintptr_t value, Fn&& fn)
        {
            for (auto& r : regions)
                for (auto p = (r.begin + 3) & ~uintptr_t(3); p + 4 <= r.end; p += 4)
                    if (*(uintptr_t*)p == value && fn(p))
                        return;
        }

        struct TypeEntry
        {
            void* typeInfo = nullptr;
            ClassInfoData* data = nullptr;
        };

        TypeEntry FindType(const char* name)
        {
            static std::map<std::string, TypeEntry, std::less<>> cache;
            if (auto it = cache.find(name); it != cache.end())
                return it->second;

            TypeEntry entry;
            auto regions = ReadableRegions();
            std::string needle = std::string(1, '\0') + name + std::string(1, '\0');
            for (auto& r : regions)
            {
                auto first = (const char*)r.begin, last = (const char*)r.end;
                for (auto it = std::search(first, last, needle.begin(), needle.end()); it != last && !entry.data; it = std::search(it + 1, last, needle.begin(), needle.end()))
                {
                    auto string = (uintptr_t)(it + 1);
                    ForEachDword(regions, string, [&](uintptr_t candidate)
                    {
                        auto data = (ClassInfoData*)candidate;
                        if (!InImage(regions, candidate, sizeof(ClassInfoData)) || data->fieldCount == 0 || !InImage(regions, (uintptr_t)data->fields, data->fieldCount * sizeof(ClassInfoData::FieldInfoData)))
                            return false;
                        // The type's TypeInfo: its data, the next type in the list and its id
                        ForEachDword(regions, candidate, [&](uintptr_t typeInfo)
                        {
                            if (!InImage(regions, typeInfo, 16) || ((uint32_t*)typeInfo)[2] >= 0x100000)
                                return false;
                            entry.typeInfo = (void*)typeInfo;
                            entry.data = data;
                            return true;
                        });
                        return entry.data != nullptr;
                    });
                }
                if (entry.data)
                    break;
            }

            cache.emplace(name, entry);
            return entry;
        }

        // The settings object of the type, or nullptr; on the game's thread
        void* Get(const char* typeName)
        {
            if (!pManager || !*pManager || !getContainer)
                return nullptr;
            auto type = FindType(typeName);
            return type.typeInfo ? getContainer(*pManager, type.typeInfo) : nullptr;
        }

        // The field's offset in the settings object, or -1 when the type has no such field
        int FieldOffset(const char* typeName, const char* fieldName)
        {
            auto type = FindType(typeName);
            if (!type.data)
                return -1;
            for (int i = 0; i < type.data->fieldCount; ++i)
                if (type.data->fields[i].name && std::string_view(type.data->fields[i].name) == fieldName)
                    return type.data->fields[i].offset;
            return -1;
        }

        // A field of a settings object, or nullptr
        template<typename T>
        T* Field(const char* typeName, const char* fieldName)
        {
            static_assert(sizeof(T) <= 8);
            auto settings = Get(typeName);
            int offset = FieldOffset(typeName, fieldName);
            return (settings && offset >= 0) ? (T*)((uintptr_t)settings + offset) : nullptr;
        }
    }
}

export std::pair<int, int> GetRes()
{
    // The profile options don't exist yet when the game creates its window
    int Width = 0, Height = 0;
    if (fb::g_profileOptionsManager.get() && fb::ProfileOptions::getInt)
    {
        Width = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "ResolutionWidth", 0);
        Height = fb::ProfileOptions::getInt(fb::g_profileOptionsManager + 0x14, "ResolutionHeight", 0);
    }

    if (Width <= 0 || Height <= 0)
    {
        Width = PtrWalkthrough<int>(&pRenderDevice, 0, 0x84, 0x18).value_or(1280);
        Height = PtrWalkthrough<int>(&pRenderDevice, 0, 0x84, 0x20).value_or(720);
    }

    return { Width, Height };
}