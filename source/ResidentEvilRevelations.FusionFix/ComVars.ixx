module;

#include <stdafx.h>
#include <d3d9.h>

export module ComVars;

// The game's design aspect: the HUD and menus are laid out for 1280x720
export constexpr float defaultAspectRatio = 16.0f / 9.0f;

// Game objects, found in dllmain before the modules initialize
export GameRef<uintptr_t> pRender;  // renderer: resolution +0xB8, the device +0x98

// The resolution the game renders at, caught when the game sets it (see Display)
export int32_t nResX = 0;
export int32_t nResY = 0;

export int32_t GetResX()
{
    return nResX ? nResX : *(int32_t*)(pRender.get() + 0xB8);
}

export int32_t GetResY()
{
    return nResY ? nResY : *(int32_t*)(pRender.get() + 0xBC);
}

export IDirect3DDevice9* GetD3DDevice()
{
    auto render = pRender.get();
    return render ? *(IDirect3DDevice9**)(render + 0x98) : nullptr;
}

export float GetAspectRatio()
{
    const auto width = GetResX(), height = GetResY();
    return width > 0 && height > 0 ? (float)width / (float)height : defaultAspectRatio;
}

// How much wider than 16:9 the screen is; screens narrower than 16:9 keep the game's own layout
export float GetDiff()
{
    return (std::max)(GetAspectRatio() / defaultAspectRatio, 1.0f);
}

// The class name of an MT Framework object: vtable slot 4 returns its type info, which holds the name at +4
export const char* GetClassName(void* object)
{
    auto vtable = *(uintptr_t**)object;
    auto getDTI = (void* (__fastcall*)(void*))vtable[4];
    auto dti = (uintptr_t)getDTI(object);
    return dti ? *(const char**)(dti + 4) : nullptr;
}
