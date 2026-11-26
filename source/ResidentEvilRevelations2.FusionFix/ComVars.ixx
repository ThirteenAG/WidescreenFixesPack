module;

#include <stdafx.h>
#include <atomic>

export module ComVars;

// The game's design aspect: the HUD and menus are laid out for 1280x720
export constexpr float defaultAspectRatio = 16.0f / 9.0f;

// Game objects, found in dllmain before the modules initialize
export GameRef<uintptr_t> pGameMode;    // game mode manager; mode 1 = local split screen (+0x8F4)
export GameRef<uintptr_t> pRender;      // renderer: resolution +0x1E0, player viewports (left, top, right, bottom) from +0x48
export GameRef<uint8_t*> pCameraSystem; // sBioCamera: the players' cameras every 400 bytes from +52

// The backbuffer size is published together so a live device reset cannot mix two resolutions.
std::atomic<uint64_t> nBackBufferSize{ 0 };

export void SetBackBufferSize(uint32_t width, uint32_t height)
{
    if (!width || !height)
        return;
    nBackBufferSize.store((uint64_t(width) << 32) | height, std::memory_order_relaxed);
}

export bool IsSplitScreenActive()
{
    auto ptr = pGameMode.get();
    return ptr && *(uint32_t*)(ptr + 0x8F4) == 1;
}

export int32_t GetResX()
{
    const auto size = nBackBufferSize.load(std::memory_order_relaxed);
    return size ? int32_t(size >> 32) : *(int32_t*)(pRender.get() + 0x1E0);
}

export int32_t GetResY()
{
    const auto size = nBackBufferSize.load(std::memory_order_relaxed);
    return size ? int32_t(uint32_t(size)) : *(int32_t*)(pRender.get() + 0x1E4);
}

export struct RenderSize
{
    int32_t width{};
    int32_t height{};

    bool operator==(const RenderSize&) const = default;

    float Aspect() const
    {
        return width > 0 && height > 0 ? float(width) / float(height) : defaultAspectRatio;
    }
};

export RenderSize GetRenderSize()
{
    const auto size = nBackBufferSize.load(std::memory_order_relaxed);
    if (size)
        return { int32_t(size >> 32), int32_t(uint32_t(size)) };
    const auto render = pRender.get();
    return { *reinterpret_cast<const int32_t*>(render + 0x1E0), *reinterpret_cast<const int32_t*>(render + 0x1E4) };
}

export float GetAspectRatio()
{
    return GetRenderSize().Aspect();
}

// Split screen: the layout in use, the size of a player's viewport, and the game's own split viewport (800x360 at
// 1280x720)
export namespace SplitScreen
{
    enum class Layout
    {
        Original,       // the game's two staggered 800x360 viewports
        TopBottom,      // each player a half of the screen, one above the other
        SideBySide,     // each player a half of the screen, next to each other
    };

    constexpr int32_t NativeResX = 936 - 136;
    constexpr int32_t NativeResY = 360;
    constexpr float NativeAspect = float(NativeResX) / float(NativeResY);

    struct ViewPair
    {
        Layout layout{ Layout::TopBottom };
        bool swap{};
        RenderSize screen{};
        std::array<RECT, 2> viewports{};

        bool IsPlayerViewport(int32_t width, int32_t height) const
        {
            // A viewport can differ from the camera rectangle by a pixel of rounding.
            for (const auto& rect : viewports)
            {
                const LONG extent = layout == Layout::SideBySide ? rect.right - rect.left : rect.bottom - rect.top;
                const auto drawn = layout == Layout::SideBySide ? width : height;
                if (extent > 0 && std::abs(drawn - extent) <= 1)
                    return true;
            }
            return false;
        }

        float ViewportAspect() const
        {
            switch (layout)
            {
                case Layout::Original:
                    return NativeAspect;
                case Layout::SideBySide:
                    return screen.Aspect() / 2.0f;
                default:
                    return screen.Aspect() * 2.0f;
            }
        }

        float WidthScale() const
        {
            return layout == Layout::Original ? 1.0f : 720.0f * screen.Aspect() / float(NativeResX);
        }
    };

    std::atomic<ViewPair> installedPair{};
    ViewPair InstalledPair() { return installedPair.load(std::memory_order_relaxed); }
    void PublishPair(const ViewPair& pair) { installedPair.store(pair, std::memory_order_relaxed); }
}

// How much wider than the game's layout the view is: the screen against 16:9, in split screen against the game's
// 800x360 viewport
export float GetDiff()
{
    if (IsSplitScreenActive())
        return SplitScreen::InstalledPair().WidthScale();
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
