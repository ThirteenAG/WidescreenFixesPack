module;

#include "stdafx.h"

export module ComVars;

export struct Screen
{
    int32_t nWidth;
    int32_t nHeight;
    float fWidth;
    float fHeight;
    float fAspectRatio;
    // The game's 640x448 2D frame shown at 4:3: its size and origin in pixels
    float fBoxWidth;
    float fBoxHeight;
    float fBoxLeft;
    float fBoxTop;
    // HUD elements anchored to a side move this many pixels outward from the 4:3 frame
    float fHudShift;
} Screen;

// A camera's viewport in pixels
export struct CameraViewport
{
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
};

export using SpriteFn = void* (__cdecl*)(int u, int v, int tw, int th, int x, int y, int z, int w, int h);
export using TextFn = int (__cdecl*)(int** format, void* a2, int measure, int* width, void* a5);
export using TextureFn = void* (__cdecl*)(int texture);
export using ColorFn = void* (__cdecl*)(int r, int g, int b, int a, int flag);
export using ViewportFn = void (__cdecl*)(void* camera, float x, float y, float w, float h);

// Display
export GameRef<int32_t> nResolutionWidth;
export GameRef<int32_t> nResolutionHeight;
export GameRef<uint8_t> bWindowed;
export GameRef<float> fAspectRatioDivisor;
export GameRef<float> fAspectRatio;

// 2D commands: the game's command functions and the text state
export GameRef<SpriteFn> pfnDrawSprite;
export GameRef<TextFn> pfnDrawText;
export GameRef<TextureFn> pfnSetTexture;
export GameRef<ColorFn> pfnSetColor;
export GameRef<int32_t> nTextCursorX;
export GameRef<int32_t> nTextCursorY;
export GameRef<uint8_t> bVerticalText;
export GameRef<int32_t> nFrontendFont;

// Cameras
export GameRef<int32_t> bNativeWidescreen;
export GameRef<void*> pCameraList;
export GameRef<CameraViewport> MainCamera;
export GameRef<ViewportFn> pfnSetCameraViewport;

// The options tool's selected display mode
export ProtectedGameRef<uint32_t> nOptionsMode;

// MatrixConfig.ini next to the game
export std::filesystem::path MatrixNativeConfigPath()
{
    std::wstring path(32768, L'\0');
    const auto length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size())
        return {};
    path.resize(length);
    return std::filesystem::path(path).parent_path() / L"MatrixConfig.ini";
}

// The resolution MatrixConfig.ini asks for, the desktop's when it has none
export std::pair<int32_t, int32_t> MatrixNativeResolution()
{
    auto [width, height] = GetDesktopRes();
    if (width <= 0 || height <= 0)
    {
        width = 640;
        height = 480;
    }
    const auto path = MatrixNativeConfigPath();
    if (!path.empty())
    {
        wchar_t value[64] = {};
        const auto fallback = std::format(L"{}x{}", width, height);
        GetPrivateProfileStringW(L"RENDERING", L"RESOLUTION", fallback.c_str(), value, static_cast<DWORD>(std::size(value)), path.c_str());
        int32_t requestedWidth = 0, requestedHeight = 0;
        if (swscanf_s(value, L"%dx%d", &requestedWidth, &requestedHeight) == 2 &&
            requestedWidth > 0 && requestedHeight > 0 && requestedWidth <= 32767 && requestedHeight <= 32767)
        {
            width = requestedWidth;
            height = requestedHeight;
        }
    }
    return { width, height };
}
