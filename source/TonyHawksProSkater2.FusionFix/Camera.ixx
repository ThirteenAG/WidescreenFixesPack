module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Camera;

import ComVars;
import Movies;

export namespace THPS2Camera
{
    inline SafetyHookMid CopyHook, ProjectionHook, DestroyHook;
    inline SafetyHookInline WorldHook;
    inline std::array<bool, 2> Gameplay{};
    inline std::atomic<float> Factor{ 1.0f };
    inline const int32_t* AspectFov = nullptr;
    inline int DrawingPlayer = -1;
    inline bool Inset = false;

    constexpr int32_t ProjectionDivisor(int32_t width, int32_t height)
    {
        return std::max(1, static_cast<int32_t>(16384LL * height / (3LL * width)));
    }

    static_assert(ProjectionDivisor(16384, 1) == 1);
    static_assert(ProjectionDivisor(640, 480) == 4096);
    static_assert(ProjectionDivisor(1536, 432) == 1536);

    inline uint32_t Numerator(uint32_t original, float factor, int32_t aspectFov)
    {
        if (!std::isfinite(factor) || aspectFov <= 0 || factor == 1.0f || !original)
            return original;
        const float tangent = static_cast<float>(original) / (4096.0f * aspectFov);
        const float halfAngle = std::atan(tangent);
        const float limit = std::max(halfAngle, 1.30899694f);
        const float corrected = std::tan(std::clamp(halfAngle * std::clamp(factor, 0.5f, 2.0f), 0.008726646f, limit));
        return static_cast<uint32_t>(std::clamp(std::round(corrected * aspectFov), 1.0f, 65535.0f)) << 12;
    }

    inline int __cdecl DrawWorld(uint16_t* viewport, int player)
    {
        const auto previous = std::exchange(DrawingPlayer, player);
        const auto result = WorldHook.ccall<int>(viewport, player);
        DrawingPlayer = previous;
        return result;
    }

    inline void Install(const int32_t* aspectFov, HMODULE module = GetModuleHandleW(nullptr))
    {
        ObserveFixSetting("MAIN.FOVFactor", Factor, [](const std::string& value)
        {
            float factor{ 1.0f };
            asi_settings::detail::ParseFloat(value, factor);
            return factor;
        });
        AspectFov = aspectFov;
        auto copy = hook::module_pattern(module, "8B 81 A4 03 00 00 56 85 C0 57 0F 84 ? ? ? ? 83 B8 B8 30 00 00 05 75 ? A1");
        auto world = hook::module_pattern(module, "83 EC 54 53 55 6A 09 E8 ? ? ? ? 83 C4 04 BD 01 00 00 00 84 C0");
        auto replay = hook::module_pattern(module, "66 83 3D ? ? ? ? 00 7E 0F A0 ? ? ? ? 84 C0 75 06 B8 01 00 00 00 C3 33 C0 C3");
        auto projection = hook::module_pattern(module, "F7 3D ? ? ? ? 33 C9 66 8B 4E");
        auto destroy = hook::module_pattern(module, "6A FF 68 ? ? ? ? 64 A1 00 00 00 00 50 64 89 25 00 00 00 00 51 56 8B F1 89 74 24 04 C7 06 ? ? ? ? 68 ? ? ? ? C7 44 24 14 0D 00 00 00 E8");
        NativeIndex.SetAddress(*copy.get_first<int*>(26));
        ReplayPlayers.SetAddress(*replay.get_first<int16_t*>(3));
        Demo.SetAddress(*replay.get_first<uint8_t*>(11));
        CopyHook = safetyhook::create_mid(copy.get_first(), [](SafetyHookContext& regs)
        {
            const auto player = NativeIndex.get();
            if (player >= 0 && player < static_cast<int>(Gameplay.size()))
            {
                const auto camera = regs.ecx;
                const auto mode = *reinterpret_cast<int*>(camera + 1284);
                Gameplay[player] = (mode == 1 || mode == 2) && *reinterpret_cast<uintptr_t*>(camera + 932) &&
                    *reinterpret_cast<int*>(camera + 1036) == 12 && !*reinterpret_cast<uint16_t*>(camera + 1040);
            }
        });
        ProjectionHook = safetyhook::create_mid(projection.get_first(), [](SafetyHookContext& regs)
        {
            // Menu 3D views keep their 4:3 framing inside the 4:3 frame
            if (Inset && AspectFov)
            {
                regs.eax = static_cast<uint32_t>(static_cast<uint64_t>(regs.eax) * static_cast<uint32_t>(*AspectFov) / 4096);
                regs.edx = 0;
            }
            else if (DrawingPlayer >= 0 && DrawingPlayer < static_cast<int>(Gameplay.size()) && Gameplay[DrawingPlayer] &&
                ReplayPlayers <= 0 && !Demo && AspectFov)
            {
                regs.eax = Numerator(regs.eax, Factor.load(std::memory_order_relaxed), *AspectFov);
                regs.edx = 0;
            }
        });
        DestroyHook = safetyhook::create_mid(destroy.get_first(), [](SafetyHookContext&)
        {
            Gameplay.fill(false);
        });
        WorldHook = safetyhook::create_inline(world.get_first(), DrawWorld);
    }
}

export namespace THPS2Frame
{
    // Menu pieces that the game places with the full screen size instead of the centred 4:3 frame
    inline SafetyHookInline ViewportHook, PreviewHook, AreaHook;
    inline SafetyHookMid CenterHook;
    inline float Width = 640.0f, Height = 480.0f, FrameWidth = 640.0f, FrameHeight = 480.0f;
    // The renderer retains the active viewport through the subsequent scene draws.
    inline std::array<uint16_t, 10> MenuViewport{};

    inline uint16_t Fit(uint16_t value, float size, float frame)
    {
        return static_cast<uint16_t>(std::clamp(std::lround(value * frame / size + (size - frame) * 0.5f), 0L, 65535L));
    }

    // 3D views inside menus (level-select globe, skater preview) get their viewport from menu coordinates.
    // Horizontally here; vertically (screens narrower than 4:3) through the clip area and the view centre.
    inline int __cdecl Viewport(int scene, uint16_t* rect, int list)
    {
        const bool inset = rect && THPS2Camera::DrawingPlayer < 0 && !(rect[2] == 0 && rect[0] >= Width);
        auto* activeViewport = rect;
        if (inset)
        {
            std::copy_n(rect, MenuViewport.size(), MenuViewport.begin());
            MenuViewport[0] = Fit(MenuViewport[0], Width, FrameWidth);
            MenuViewport[2] = Fit(MenuViewport[2], Width, FrameWidth);
            activeViewport = MenuViewport.data();
        }
        const auto previous = std::exchange(THPS2Camera::Inset, inset);
        const auto result = ViewportHook.ccall<int>(scene, activeViewport, list);
        THPS2Camera::Inset = previous;
        if (inset)
        {
            rect[5] = MenuViewport[5];
            std::copy_n(MenuViewport.begin() + 7, 3, rect + 7);
        }
        return result;
    }

    // The level-select preview movie quad
    inline uintptr_t __cdecl Preview(int16_t* quad)
    {
        if (!quad)
            return PreviewHook.ccall<uintptr_t>(quad);
        std::array<int16_t, 18> fitted;
        std::copy_n(quad, fitted.size(), fitted.begin());
        for (auto x : { 4, 8, 12, 16 })
        {
            fitted[x] = static_cast<int16_t>(std::lround(fitted[x] + (Width - FrameWidth) * 0.5f));
            fitted[x + 1] = static_cast<int16_t>(std::lround(fitted[x + 1] + (Height - FrameHeight) * 0.5f));
        }
        return PreviewHook.ccall<uintptr_t>(fitted.data());
    }

    // Clip areas in 512x240 units, on screens narrower than 4:3 (bars above and below the 2D frame)
    inline int __cdecl Area(int16_t* area)
    {
        if (!area)
            return AreaHook.ccall<int>(area);
        const int lowY = static_cast<uint8_t>(area[5]);
        const int y = area[5] < 0 && lowY ? lowY - 256 : lowY;
        if (y <= 0 && y + area[7] >= 240)
            return AreaHook.ccall<int>(area);
        std::array<int16_t, 8> fitted;
        std::copy_n(area, fitted.size(), fitted.begin());
        const float scale = FrameHeight / Height;
        fitted[5] = static_cast<int16_t>(std::lround(y * scale + (Height - FrameHeight) * 0.5f * 240.0f / Height));
        fitted[7] = static_cast<int16_t>(std::lround(area[7] * scale));
        return AreaHook.ccall<int>(fitted.data());
    }

    inline void Install(float width, float height, float frameWidth, float frameHeight, int32_t* frameWidthValue, int32_t* frameHeightValue)
    {
        Width = width;
        Height = height;
        FrameWidth = frameWidth;
        FrameHeight = frameHeight;
        auto viewport = hook::pattern("A1 ? ? ? ? 83 EC 08 A3 ? ? ? ? 8B 0D ? ? ? ? 3B C1 89 0D ? ? ? ? 74 06 FF 05");
        auto preview = hook::pattern("A1 ? ? ? ? 53 33 DB 56 3B C3 0F 84 ? ? ? ? 8B 74 24 0C 3B F3 0F 84 ? ? ? ? 8B 15 ? ? ? ? 55 57 B9 30 00");
        auto widget = hook::pattern("83 EC 08 53 55 8B 6C 24 14 56 8B F1 57 8B BE 40 01 00 00 8B 56 20 8B 9E 44 01 00 00");
        const auto start = reinterpret_cast<uintptr_t>(widget.get_first());
        const char* widths[] = {
            "0F AF 0D ? ? ? ? C1 E9 09 66 89 48 08",
            "0F AF 0D ? ? ? ? C1 E9 09 66 89 48 10",
            "0F AF 0D ? ? ? ? C1 E9 09 66 89 48 18",
            "0F AF 0D ? ? ? ? C1 E9 09 66 89 48 20"
        };
        const char* heights[] = {
            "0F AF 15 ? ? ? ? F7 E2 C1 EA 07 66 89 51 0A",
            "0F AF 15 ? ? ? ? F7 E2 C1 EA 07 66 89 51 12",
            "0F AF 15 ? ? ? ? F7 E2 C1 EA 07 66 89 51 1A",
            "0F AF 15 ? ? ? ? F7 E2 C1 EA 07 66 89 51 22"
        };
        std::array<int32_t**, 4> widthOperands{}, heightOperands{};
        for (size_t i = 0; i < widthOperands.size(); ++i)
        {
            widthOperands[i] = hook::range_pattern(start, start + 0x2D4, widths[i]).get_first<int32_t*>(3);
            heightOperands[i] = hook::range_pattern(start, start + 0x2D4, heights[i]).get_first<int32_t*>(3);
        }
        for (size_t i = 0; i < widthOperands.size(); ++i)
        {
            injector::WriteMemory<int32_t*>(widthOperands[i], frameWidthValue, true);
            injector::WriteMemory<int32_t*>(heightOperands[i], frameHeightValue, true);
        }
        ViewportHook = safetyhook::create_inline(viewport.get_first(), Viewport);
        PreviewHook = safetyhook::create_inline(preview.get_first(), Preview);
        if (frameHeight < height)
        {
            auto area = hook::pattern("53 55 56 57 8B 7C 24 14 0F BF 47 0A 25 FF 00 00 80 79 07 48 0D 00 FF FF FF 40");
            auto center = hook::pattern("66 8B 56 10 89 15 ? ? ? ? 66 8B 46 12 A3"); //vertical centre of the 3D view
            AreaHook = safetyhook::create_inline(area.get_first(), Area);
            CenterHook = safetyhook::create_mid(center.get_first(14), [](SafetyHookContext& regs)
            {
                if (THPS2Camera::Inset)
                    regs.eax = Fit(static_cast<uint16_t>(regs.eax), Height, FrameHeight);
            });
        }
    }
}

export void InstallCameraAndMovies()
{
    hook::pattern pattern("");
    //FOV
    Screen.FOV = THPS2Camera::ProjectionDivisor(Screen.Width, Screen.Height);
    pattern = hook::pattern("F7 3D ? ? ? ? 33 C9 66 8B 4E"); //45E9E9
    injector::WriteMemory<int32_t*>(pattern.get_first<int32_t*>(2), &Screen.FOV, true);

    THPS2Camera::Install(&Screen.FOV);

    THPS2Video::Install(Screen.fWidth, Screen.fHeight);
}
