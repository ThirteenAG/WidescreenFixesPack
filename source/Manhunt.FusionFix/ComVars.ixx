module;

#include <stdafx.h>
#include <cstdint>

export module ComVars;

// The platform part of the skeleton: the game's window
export struct PsGlobalType
{
    void** vtable;
    int32_t fullScreen;
    uint8_t pad08[16];
    HWND window;
    HINSTANCE instance;
};

export struct RsGlobalType
{
    const char* appName;
    int32_t width;
    int32_t height;
    PsGlobalType* ps;
    int32_t maximumWidth;
    int32_t maximumHeight;
    int32_t pad18;
    int32_t quit;
};

export GameRef<RsGlobalType> RsGlobal;

export namespace RwEngine
{
    GameRef<uint8_t*> Instance;
}

export namespace CCutScene
{
    GameRef<int32_t> ms_bCutSceneInProgess;
}

export namespace CScene
{
    GameRef<float> m_zoom;
}

export GameRef<float> fViewWindowWidth;
export GameRef<float> fViewWindowHeight;
export GameRef<int32_t> ActiveMenuOverlay;
export GameRef<float> HudStretch;
export GameRef<float> MouseMoveX;
export GameRef<float> MouseMoveY;
export GameRef<uint8_t> focused;
export GameRef<int32_t> active;
export GameRef<uint32_t> adapterIndex;
export GameRef<int32_t> VideoModeIndex;

export namespace MHIcons
{
    struct PrintState
    {
        float x, y, scaleX, scaleY, width;
        int font;
        std::array<std::array<int, 4>, 4> colors;
    };
}

export GameRef<MHIcons::PrintState> printState;
export GameRef<int> printFont;
export GameRef<float> printHeights;
export GameRef<int> printMeasuring;
export GameRef<int32_t> inputEnabled;
export GameRef<int32_t> mouseMode;
export GameRef<uint32_t> nativePad;

export template<class T> GameRef<T>* BindGameRef(GameRef<T>& ref, T* address)
{
    if (!address) return nullptr;
    ref.SetAddress(address);
    return &ref;
}

export GameRef<int32_t> menuCurrentMenu;
export GameRef<int32_t> menuSettingsSelection;
export GameRef<int32_t> menuSettingsRows;
export GameRef<int32_t> menuSettingsPaused;
export GameRef<int32_t> menuVideoSelection;
export GameRef<float> menuX;
export GameRef<float> menuY;
export GameRef<float> menuScaleX;
export GameRef<float> menuScaleY;
export GameRef<float> menuSpacing;
export GameRef<int32_t> menuFooterBack;
export GameRef<int32_t> menuFooterAccept;
export GameRef<int32_t> menuMainRow;
export GameRef<int32_t> menuLanguage;
export GameRef<float> menuDetailRows;
export GameRef<int32_t> menuResolutionCount;
export GameRef<int32_t> menuDetailCount;
export std::array<GameRef<int32_t>, 5> menuVideoValues;
