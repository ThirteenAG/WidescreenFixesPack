module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

export struct Screen
{
    float fWidth = 800.0f;
    float fHeight = 600.0f;
    float fAspectRatio = 4.0f / 3.0f;
    float fBaseWidth = 800.0f;
    float fInvBaseWidth = 1.0f / 800.0f;
} Screen;

export int nWindowMode = 0;
export int nWindowWidth = 0, nWindowHeight = 0;
export uintptr_t GameplayCamera = 0;
export bool GameplayFovEnabled = false;
export bool GameplayDriving = false;
export std::array<bool, 60> ModernPadActions{};
export bool bWidescreenMap;
export bool bNoCutsceneBorderAnimation;
export int nCutsceneBorders;
export bool bChangeDrawDistance;
export float fDrawDistance, fDrawDistanceMin, fDrawDistanceMax;
export float fCreditsTextX, fMapPlayerWidth, fMenuOffset, fMovieAspect = 1.0f;
export size_t InputBindingsOffset = 0;

// The native menu's objects (Mafia 1.2)
export namespace MafiaMenu
{
    struct Control;
    struct Record
    {
        uint32_t id, type;
        float x, y, width, height;
        uint32_t textKey, flags;
    };
    struct Control
    {
        uintptr_t* table;
        uint32_t id, flags, textKey;
        const char* text;
        float x, y, width, height, relativeX, relativeY;
        Control* parent;
        uint32_t allocator;
        Control** begin;
        Control** end;
        Control** capacity;
    };
    struct Page
    {
        uintptr_t* table;
        Page* parent;
        Control* root;
        Control* selected;
        uint32_t allocator;
        Control** begin;
        Control** end;
        Control** capacity;
        uint8_t closed;
        uint8_t closedPadding[3];
        int result, command;
        uint32_t flags;
    };
    static_assert(sizeof(Record) == 32);
    static_assert(sizeof(Control) == 64 && offsetof(Control, parent) == 44);
    static_assert(sizeof(Page) == 48 && offsetof(Page, selected) == 12 && offsetof(Page, result) == 36);
}

// The engine's Direct3D 8 present parameters
export namespace MafiaWindow
{
    struct PresentParameters
    {
        uint32_t width, height, format, buffers, multisample, swapEffect;
        HWND window;
        BOOL windowed, depthBuffer;
        uint32_t depthFormat, flags, refreshRate, interval;
    };
    static_assert(sizeof(PresentParameters) == 52);
}

// The game's resolution, set to the fix's one before the device is created
export ProtectedGameRef<int32_t> nResolutionWidth;
export ProtectedGameRef<int32_t> nResolutionHeight;

// LS3DF engine: start-up flags (windowed, pause on focus loss ...), the size it creates the device with, its device
// and present parameters
export ProtectedGameRef<uint32_t> nEngineFlags;
export ProtectedGameRef<uint32_t> nEngineWidth;
export ProtectedGameRef<uint32_t> nEngineHeight;
export GameRef<void*> pEngineDevice;
export ProtectedGameRef<MafiaWindow::PresentParameters> EnginePresentParameters;
export GameRef<IUnknown*> pD3D8Device;

// The IGraph interface as read by the HUD's cutscene draw and by the radar
export GameRef<void*> pHudIGraph;
export GameRef<void*> pRadarIGraph;

// Camera: control flags (bit 0: no player control), mouse sensitivity, inverted look
export GameRef<uint8_t> nControlFlags;
export GameRef<float> fMouseSensitivityX;
export GameRef<float> fMouseSensitivityY;
export GameRef<uint8_t> bInvertMouse;

// Native menu: the page on top of the stack, the pending command, the 800x600 scale and the language
export GameRef<MafiaMenu::Page*> pTopMenuPage;
export GameRef<int> nMenuPendingCommand;
export GameRef<float> fMenuScaleX;
export GameRef<float> fMenuScaleY;
export GameRef<uint32_t> nLanguage;

export template<class T> T& Field(uintptr_t object, size_t offset)
{
    return *reinterpret_cast<T*>(object + offset);
}

export HMODULE GetLS3DF()
{
    for (auto name : { L"LS3DF.dll", L"LSV11.dll", L"LSV10.dll" })
        if (auto hModule = GetModuleHandleW(name))
            return hModule;
    return nullptr;
}

