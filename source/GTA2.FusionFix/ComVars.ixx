module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <ddraw.h>

export module ComVars;

// Settings
export int nResX;
export int nResY;
export std::optional<float> fHudAspectRatioConstraint; // HUD Aspect Ratio: a centred box of this aspect (Auto: the whole screen)
export int32_t nQuicksaveKey;
export int32_t nZoomIncreaseKey;
export int32_t nZoomDecreaseKey;
export float fZoom;
export bool bSkipMovie = true, bSkipCredits = true, bFillBackground = false;
export int windowMode = 0;          // MAIN.WindowMode: 0 fullscreen, 1 windowed, 2 borderless
export bool windowedMode = true;

// Game state
export GameRef<uint32_t> bRendererReady;         // the renderer is initialized
export GameRef<HWND> hWnd;
export GameRef<int> nClientWidth;               // the window's client size
export GameRef<int> nClientHeight;
export GameRef<uint32_t> dword_6732E0;         // the video mode setup's sizes
export GameRef<uint32_t> dword_673578;
export GameRef<uint32_t> dword_6732E8;
export GameRef<uint32_t> dword_6732E4;
export GameRef<uint32_t> ptrToGame;             // the game, for the ped shadows
export GameRef<uint32_t> dword_6644BC;         // the game's players, for the quicksave
export GameRef<uintptr_t> dword_672F40;         // the HUD, for the quicksave's message
export GameRef<int> nGammaRefresh;              // frames until the gamma is applied again
export GameRef<uint32_t> window_width;          // the configured window size
export GameRef<uint32_t> window_height;
export GameRef<char*> pVideoDriver;             // the video driver's state (dmavideo)
export GameRef<uint32_t> nMultiplayer;          // a network game
export GameRef<void*> pGame;                    // the running game
export void* Replay = nullptr;                  // the replay recorder
export bool (__fastcall* IsReplay)(void*, int) = nullptr;
export char* FrontendMenu = nullptr;            // the frontend, while its input is processed
// The renderer's loaded images (d3ddll)
export struct GTA2Image { DWORD used, width, height; IDirectDrawSurface4* surface; };
static_assert(sizeof(GTA2Image) == 16);
export GameRef<GTA2Image*> pImages;
export GameRef<int> nImageCount;
export GameRef<IUnknown*> pBinkDirectDraw;      // the DirectDraw interface Bink creates its buffers with
export GameRef<uint16_t> wFooterFont;           // the frontend's fonts
export GameRef<uint16_t> wSelectedFont;
export GameRef<uint16_t> wHeadingFont;          // the pause screen's fonts
export GameRef<uint16_t> wRowFont;
export GameRef<int> nBlockRangeFactor;          // the map block range's height factor (0.75)

// The renderer's exports, as the game calls them
export GameRef<void*> gbh_DrawTile;
export GameRef<void*> gbh_DrawTilePart;
export GameRef<void*> gbh_DrawQuad;
export GameRef<void*> gbh_DrawQuadClipped;
export GameRef<void*> gbh_DrawTriangle;

// The fullscreen mode's size in the video mode setup (rewritten when the resolution or the window mode changes)
export ProtectedGameRef<int> fullscreenWidth;
export ProtectedGameRef<int> fullscreenHeight;
export void (__cdecl* ReloadVideoMode)() = nullptr;
export std::function<void()> PatchWindowMode;

export template<class T> T& Field(void* object, size_t offset)
{
    return *reinterpret_cast<T*>(static_cast<char*>(object) + offset);
}

export int RenderDimension(int offset, GameRef<uint32_t>& configured, int fallback)
{
    // A bordered desktop-sized window can have a smaller client area. Match
    // the surface actually allocated by the video backend, including resize.
    char* video = pVideoDriver.is_initialized() ? pVideoDriver.get() : nullptr;
    if (video && *reinterpret_cast<void**>(video + 312))
    {
        int value = *reinterpret_cast<int*>(video + offset);
        if (value > 0 && value <= 7680) return value;
    }
    return configured.is_initialized() ? int(configured.get()) : fallback;
}

export constexpr float default_screen_width = 640.0f;
export constexpr float default_screen_height = 480.0f;
export constexpr float default_aspect_ratio = default_screen_width / default_screen_height;

export float screen_width() { return float(RenderDimension(72, window_width, 640)); }
export float screen_height() { return float(RenderDimension(76, window_height, 480)); }
export float screen_aspect_ratio() { return screen_width() / screen_height(); }

export constexpr float fone = 16384.0f;
export constexpr int one = 16384;
export float camera_scale() { return screen_width() / default_screen_width; }
export float hud_scale() { return screen_height() / default_screen_height; }
export float default_hud_scale() { return screen_width() / default_screen_width; }

export float hud_offset() { return screen_width() - screen_height() * default_aspect_ratio; }
