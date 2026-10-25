module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

// The game's design aspect: the HUD, menus and cutscenes are laid out for 1280x720
export constexpr float defaultAspectRatio = 16.0f / 9.0f;

// Game objects, found in dllmain before the modules initialize
export GameRef<uintptr_t> pGameMode;        // game mode manager; mode 1 = local split screen (+0x86C)
export GameRef<uintptr_t> pRender;          // renderer: device +0x100, resolution +0x120, relative resolution +0x128
export GameRef<uintptr_t> pViewports;       // viewport manager: 400 bytes per player from +0x48, game mode +0xCE0

export bool IsSplitScreenActive()
{
    auto ptr = pGameMode.get();
    return ptr && *(uint32_t*)(ptr + 0x86C) == 1;
}

// The view layout: 1 and 4 = split screen, 2 and 3 = a cutscene
export bool IsCutsceneViewActive()
{
    auto ptr = pGameMode.get();
    if (!ptr)
        return false;
    auto mode = *(uint32_t*)(ptr + 0x86C);
    return mode == 2 || mode == 3;
}

export IDirect3DDevice9* GetD3DDevice()
{
    auto render = pRender.get();
    return render ? *(IDirect3DDevice9**)(render + 0x100) : nullptr;
}

export int32_t GetResX()
{
    return *(int32_t*)(pRender.get() + 0x120);
}

export int32_t GetResY()
{
    return *(int32_t*)(pRender.get() + 0x124);
}

export int32_t GetRelativeResX()
{
    return *(int32_t*)(pRender.get() + 0x128);
}

export int32_t GetRelativeResY()
{
    return *(int32_t*)(pRender.get() + 0x12C);
}

export float GetAspectRatio()
{
    const auto width = GetResX(), height = GetResY();
    return width > 0 && height > 0 ? (float)width / (float)height : defaultAspectRatio;
}

// How much wider than 16:9 the screen is
export float GetDiff()
{
    return GetAspectRatio() / defaultAspectRatio;
}

// The viewport of a player in screen pixels: left, top, right, bottom
export int32_t* GetPlayerViewport(int32_t player)
{
    return (int32_t*)(pViewports.get() + 400 * player + 0x48);
}

// Split screen: the size the game gave a player's viewport, and the size the fix gave it instead. The game lays out
// the split-screen HUD for its own viewport, an 800x360 box (20:9) at 1280x720.
export namespace SplitScreen
{
    int32_t nNativeResX = 0, nNativeResY = 0;
    int32_t nCurrentResX = 0, nCurrentResY = 0;

    void SetSizes(int32_t nativeW, int32_t nativeH, int32_t currentW, int32_t currentH)
    {
        nNativeResX = nativeW;
        nNativeResY = nativeH;
        nCurrentResX = currentW;
        nCurrentResY = currentH;
    }

    int32_t GetNativeResX() { return nNativeResX ? nNativeResX : GetResX(); }
    int32_t GetNativeResY() { return nNativeResY ? nNativeResY : GetResY() / 2; }
    int32_t GetCurrentResX() { return nCurrentResX ? nCurrentResX : GetResX(); }
    int32_t GetCurrentResY() { return nCurrentResY ? nCurrentResY : GetResY() / 2; }

    // How much wider the fix's viewport is than the game's
    float GetDiff()
    {
        return (float)GetCurrentResX() / (float)GetNativeResX();
    }

    int32_t GetRelativeResX()
    {
        return (int32_t)((float)::GetRelativeResX() / ((float)GetResX() / (float)GetCurrentResX()));
    }

    int32_t GetRelativeResY()
    {
        return (int32_t)((float)::GetRelativeResY() / 2.0f);
    }
}
