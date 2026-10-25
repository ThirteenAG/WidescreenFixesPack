module;

#include <stdafx.h>

export module ComVars;

// The game's design aspect: the HUD and menus are laid out for 1280x720
export constexpr float defaultAspectRatio = 16.0f / 9.0f;

// Game objects, found in dllmain before the modules initialize
export GameRef<uintptr_t> pGameMode;    // game mode manager; mode 1 = local split screen (+0x8F4)
export GameRef<uintptr_t> pRender;      // renderer: resolution +0x1E0, split viewport size +0x50, player viewports from +0x48

// The resolution the game renders at, caught when the game sets it (see Display)
export int32_t nResX = 0;
export int32_t nResY = 0;

export bool IsSplitScreenActive()
{
    auto ptr = pGameMode.get();
    return ptr && *(uint32_t*)(ptr + 0x8F4) == 1;
}

export int32_t GetResX()
{
    return nResX ? nResX : *(int32_t*)(pRender.get() + 0x1E0);
}

export int32_t GetResY()
{
    return nResY ? nResY : *(int32_t*)(pRender.get() + 0x1E4);
}

export float GetAspectRatio()
{
    const auto width = GetResX(), height = GetResY();
    return width > 0 && height > 0 ? (float)width / (float)height : defaultAspectRatio;
}

// Split screen: the size of a player's viewport, and the game's own split viewport (800x360 at 1280x720)
export namespace SplitScreen
{
    int32_t GetCurrentResX() { return *(int32_t*)(pRender.get() + 0x50); }
    int32_t GetCurrentResY() { return *(int32_t*)(pRender.get() + 0x54); }
    constexpr int32_t NativeResX = 936 - 136;
    constexpr int32_t NativeResY = 360;
}

// How much wider than the game's layout the view is: the screen against 16:9, in split screen against the game's
// 800x360 viewport
export float GetDiff()
{
    if (IsSplitScreenActive())
        return (GetAspectRatio() / defaultAspectRatio) * (1280.0f / (float)SplitScreen::NativeResX);
    return GetAspectRatio() / defaultAspectRatio;
}

// The class name of an MT Framework object: vtable slot 4 returns its type info, which holds the name at +4
export const char* GetClassName(void* object)
{
    auto vtable = *(uintptr_t**)object;
    auto getDTI = (void* (__fastcall*)(void*))vtable[4];
    auto dti = (uintptr_t)getDTI(object);
    return dti ? *(const char**)(dti + 4) : nullptr;
}
