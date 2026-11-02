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
    bool  Raw = false;    // the deltas are the mouse's raw movement
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

    // The race's pause menu: when it was last opened (GetTickCount64)
    namespace PauseMenu
    {
        ULONGLONG OpenedAt = 0;
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

        struct TypeEntry
        {
            void* typeInfo = nullptr;
            ClassInfoData* data = nullptr;
        };

        std::pair<uintptr_t, uintptr_t> ImageSection(const char* name)
        {
            const auto base = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
            const auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
            const auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
            const auto sections = IMAGE_FIRST_SECTION(nt);
            for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i)
            {
                if (std::strncmp(reinterpret_cast<const char*>(sections[i].Name), name, IMAGE_SIZEOF_SHORT_NAME) == 0)
                {
                    const auto begin = base + sections[i].VirtualAddress;
                    return { begin, begin + sections[i].Misc.VirtualSize };
                }
            }
            std::terminate();
        }

        std::string PatternBytes(const void* bytes, size_t length)
        {
            static constexpr char digits[] = "0123456789ABCDEF";
            std::string pattern;
            const auto data = static_cast<const uint8_t*>(bytes);
            for (size_t i = 0; i < length; ++i)
            {
                pattern += digits[data[i] >> 4];
                pattern += digits[data[i] & 15];
                pattern += ' ';
            }
            return pattern;
        }

        TypeEntry FindType(const char* name)
        {
            static std::map<std::string, TypeEntry, std::less<>> cache;
            static std::mutex cacheMutex;
            std::lock_guard lock(cacheMutex);
            auto it = cache.find(name);
            if (it == cache.end())
            {
                const auto [stringsBegin, stringsEnd] = ImageSection(".rdata");
                const auto [typesBegin, typesEnd] = ImageSection("typeinfo");
                const std::string typeName = std::string(1, '\0') + name + std::string(1, '\0');
                auto stringAddress = reinterpret_cast<uintptr_t>(hook::range_pattern(stringsBegin, stringsEnd,
                    PatternBytes(typeName.data(), typeName.size())).get_first(1));
                auto data = hook::range_pattern(typesBegin, typesEnd, PatternBytes(&stringAddress, sizeof(stringAddress))).get_first<ClassInfoData>();
                const auto dataAddress = reinterpret_cast<uintptr_t>(data);
                const auto initializer = hook::pattern("68 " + PatternBytes(&dataAddress, sizeof(dataAddress)) + "B9 ? ? ? ? E8 ? ? ? ?").get_first<uint8_t>();
                auto typeInfo = *reinterpret_cast<void**>(initializer + 6);
                it = cache.emplace(name, TypeEntry{ typeInfo, data }).first;
            }
            // The initializer constructs the TypeInfo before the settings manager can use it.
            return *reinterpret_cast<ClassInfoData**>(it->second.typeInfo) == it->second.data ? it->second : TypeEntry{};
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
        Width = 1280;
        Height = 720;
        if (pRenderDevice.is_initialized())
        {
            const auto device = static_cast<uint8_t*>(pRenderDevice.get());
            if (device)
            {
                const auto settings = *reinterpret_cast<uint8_t**>(device + 0x84);
                if (settings)
                {
                    const int width = *reinterpret_cast<int*>(settings + 0x18);
                    const int height = *reinterpret_cast<int*>(settings + 0x20);
                    if (width > 0 && height > 0)
                    {
                        Width = width;
                        Height = height;
                    }
                }
            }
        }
    }

    return { Width, Height };
}
