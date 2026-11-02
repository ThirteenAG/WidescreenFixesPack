module;

#include "stdafx.h"
#include <d3d9.h>

export module ComVars;

export float fTimeStep = 0.0f;
export bool bPaused = false;
export bool RawMouseEnabled = true;
export bool BlockGameInput = false;

export GameRef<HWND> hWnd;
export GameRef<void*> GuiService;
export GameRef<void*> FontService;
export GameRef<void*> LanguageService;
export GameRef<void*> GraphicsManager;
export GameRef<uintptr_t> dword_8AC4C8;
export GameRef<uint8_t> TextureManager;
export GameRef<uint8_t> Renderer;
export GameRef<float> TurnPerLookValue;
export GameRef<float> AimPerLookValue;

export struct AdapterInfo
{
    IDirect3D9* d3d;
    UINT adapter;
    bool initialized;
    alignas(4) std::byte identifier[1100];
    D3DDISPLAYMODE modes[500];
    uint32_t modeCount;
    D3DFORMAT formats[20];
    uint32_t formatCount;
};
static_assert(offsetof(AdapterInfo, modes) == 0x458 && sizeof(AdapterInfo) == 0x23F0);

export struct AdapterList
{
    bool initialized;
    IDirect3D9* d3d;
    AdapterInfo* adapters;
    uint32_t count;
};
export AdapterList* Adapters = nullptr;

export struct ModeList
{
    struct
    {
        UINT adapter;
        UINT mode;
    } entries[100];
    uint32_t count;
    bool built;
};
static_assert(offsetof(ModeList, count) == 800);
export GameRef<ModeList> GameModeList;
export GameRef<int> BackbufferWidth;
export GameRef<int> BackbufferHeight;
export GameRef<IDirect3DDevice9*> Direct3DDevice;
export GameRef<int32_t> RendererModeIndex; // index of the current display mode in the game's mode list

template<typename... Args>
class ResChange : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

export __declspec(noinline) ResChange<int, int>& onResChange()
{
    static ResChange<int, int> ResChangeEvent;
    return ResChangeEvent;
}

export bool IsKeyboardKeyPressed(int vkeycode)
{
    return (GetAsyncKeyState(vkeycode) & 0x8000) != 0;
}

// Game's DirectInput devices: keyboard, mouse and the selected joystick
export GameRef<uintptr_t> InputManager;

// Mouse movement of the latest input update (once per frame), in mouse counts (see Mouse.ixx)
export namespace MouseRead
{
    int32_t X = 0;
    int32_t Y = 0;
}

// "Joy But N" in the controls menu
export bool IsJoyButtonPressed(uint32_t number)
{
    uintptr_t manager = InputManager;
    if (!manager || number < 1 || number > 32)
        return false;

    return *reinterpret_cast<uint8_t*>(manager + 0x71B + number) != 0;
}

export uintptr_t GetSecondaryInputNameAddress()
{
    static const auto address = reinterpret_cast<uintptr_t>(hook::get_pattern("83 EC 18 53 56 8B 35 ? ? ? ? 57"));
    return address;
}

// Key binding tables, action names are in Territory\<Region>\Locale\<Language>\Text\controls.txt (textId)
export namespace Keymap
{
    enum Group : uint32_t
    {
        General = 0,
        FilmDirector = 1,
        Driving = 2,
        OnFoot = 3,
    };

    enum Action : uint32_t
    {
        Pause = 0, // fixed
        PauseOrientation = 9,
        PauseZoomIn = 10,
        PauseZoomOut = 11,
        PauseMoveLeft = 12,
        PauseMoveRight = 13,
        PauseMoveUp = 14,
        PauseMoveDown = 15,
        PauseRotateCW = 16,
        PauseRotateCCW = 17,
        PauseToggleInterest = 18,
        CameraChange = 20,
        ThrillCam = 21,
        DriveLookLeftRight = 22,
        DriveLookBackForward = 23,
        DirectorSecondSpeed = 24,
        DirectorMoveForwardBack = 25,
        DirectorMoveLeftRight = 26,
        DirectorMoveUpDown = 27,
        DirectorRotateLeftRight = 28,
        DirectorRotateUpDown = 29,
        AnalogueSteer = 30,
        AnalogueLean = 31,
        ActionButton = 32,
        SkipSequence = 33,
        Accelerate = 34,
        BurnOut = 35,
        HandBrake = 36,
        Reverse = 37,
        GetOutVehicle = 38,
        Horn = 39,
        WalkBackForward = 41,
        TurnLeftRight = 42,
        GetInVehicle = 43,
        FootLookLeftRight = 44,
        FootLookUpDown = 45,
        MouseLookLeftRight = 46, // fixed
        MouseLookUpDown = 47, // fixed
        DrawHolsterGun = 48,
        Shoot = 49,
        WeaponToggle = 50,
        Reload = 51,
        Jump = 52,
        CrouchRoll = 54,
        CraneUp = 55,
        CraneDown = 56,
        CraneLeft = 57,
        CraneRight = 58,
        CraneGrab = 59,
        DigitalSteerLeft = 60,
        DigitalSteerRight = 61,
        DigitalLeanForward = 62,
        DigitalLeanBack = 63,
    };

    enum Axis : uint32_t
    {
        AxisX = 0,
        AxisY = 1,
        AxisZ = 2,
        AxisRX = 3,
        AxisRY = 4,
        AxisRZ = 5,
        AxisSlider1 = 6,
        AxisSlider2 = 7,
        AxisMouseX = 8,
        AxisMouseY = 9,
        AxisMouseWheel = 10,
    };

    constexpr uint32_t None = 0x4000;
    constexpr uint32_t PovUp = 0x1000;
    constexpr uint32_t PovDown = 0x1001;
    constexpr uint32_t PovLeft = 0x1002;
    constexpr uint32_t PovRight = 0x1003;
    constexpr uint32_t JoyButton(uint32_t number) { return 0x1003 + number; } // "Joy But N" in the controls menu
    constexpr uint32_t MouseButton(uint32_t number) { return 0x1FFF + number; } // "Mouse But N"

    struct Binding
    {
        uint32_t key;    // DIK_ scancode, MouseButton(n) or None
        uint32_t isAxis; // whether code is an Axis
        uint32_t code;   // PovUp..PovRight, JoyButton(n), MouseButton(n), None or an Axis
        bool positive;   // axis direction
    };
    static_assert(sizeof(Binding) == 16);

    struct Entry
    {
        Group group;
        Action action;
        uint32_t twoWay;    // binding[0] is the negative direction, binding[1] the positive one
        Binding binding[2]; // one-way actions only use binding[0]
        int32_t textId[2];
    };
    static_assert(sizeof(Entry) == 52);

    constexpr size_t EntryCount = 50;
    constexpr size_t FixedEntryCount = 9;

    Entry* Find(Entry* table, size_t count, Action action)
    {
        if (!table)
            return nullptr;

        for (size_t i = 0; i < count; ++i)
        {
            if (table[i].action == action)
                return &table[i];
        }
        return nullptr;
    }

    // Bindings in use, copied to and from profiles
    GameRef<Entry> LiveTable;
    Entry* Live()
    {
        static const bool initialized = []()
        {
            auto pattern = hook::pattern("8B 44 24 04 C7 00 28 0A 00 00 B8");
            LiveTable.SetAddress(*pattern.get_first<Entry*>(11));
            return true;
        }();
        (void)initialized;
        return LiveTable.get_ptr();
    }

    // Copied over the live table by Reset in the controls menu
    GameRef<Entry> DefaultsTable;
    Entry* Defaults()
    {
        static const bool initialized = []()
        {
            auto pattern = hook::pattern("FF 50 70 8B 4C 24 ? 8B F8 8B C1 C1 E9 02 BE");
            DefaultsTable.SetAddress(*pattern.get_first<Entry*>(15));
            return true;
        }();
        (void)initialized;
        return DefaultsTable.get_ptr();
    }

    // Menu and pause bindings that are not listed in the controls menu
    GameRef<Entry> FixedTable;
    Entry* Fixed()
    {
        static const bool initialized = []()
        {
            auto pattern = hook::pattern("8B 54 24 04 B9 ? ? ? ? 33 C0 EB"); // every lookup function has it
            FixedTable.SetAddress(*pattern.get_first<Entry*>(5));
            return true;
        }();
        (void)initialized;
        return FixedTable.get_ptr();
    }
}
