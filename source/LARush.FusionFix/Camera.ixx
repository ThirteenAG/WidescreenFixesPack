module;
#include <stdafx.h>
#include "FixSettings.hpp"
#include "d3d9.h"

export module Camera;
import ComVars;
import Window;
import DeveloperOverlay;


export namespace LARushCamera
{
    inline SafetyHookInline FovHook;
    inline std::atomic<bool> Refresh{ false };
    inline std::atomic<bool> LowriderActive{ false };
    inline std::atomic<float> FovFactor{ 1.0f };
    inline std::atomic<int> BorderMode{ 3 };
    inline int Width = 0, Height = 0;
    inline uint8_t* BorderedCamera = nullptr;
    inline int32_t SavedViewport[4]{};
    inline float SavedAspect = 0.0f;
    inline bool MovieFrame = false;

    // Called before Present: the game may still have its own scene target bound, so clear the back buffer itself.
    inline void ClearOutside(IDirect3DDevice9* device, int left, int top, int boxWidth, int boxHeight)
    {
        RECT bars[4]{};
        size_t count = 0;
        if (left > 0) { bars[count++] = { 0, 0, left, Height }; bars[count++] = { left + boxWidth, 0, Width, Height }; }
        if (top > 0) { bars[count++] = { left, 0, left + boxWidth, top }; bars[count++] = { left, top + boxHeight, left + boxWidth, Height }; }
        if (!count)
            return;
        IDirect3DSurface9* back = nullptr;
        if (FAILED(device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &back)))
            return;
        D3DSURFACE_DESC desc{};
        if (SUCCEEDED(back->GetDesc(&desc)) && desc.Width == static_cast<UINT>(Width) && desc.Height == static_cast<UINT>(Height))
        {
            for (size_t i = 0; i < count; ++i)
                if (FAILED(device->ColorFill(back, &bars[i], 0xFF000000)))
                    break;
        }
        back->Release();
    }

    inline double __fastcall Fov(uint8_t* camera, void*)
    {
        const double original = FovHook.thiscall<double>(camera);
        const int index = CurrentCamera;
        const auto player = Player.get();
        // The player camera controller marks special/scripted views separately from normal driving.
        if (LARushWindow::GameState != 56 || index < 0 || index >= 26 ||
            camera != Cameras.get_ptr() + index * 832 || !player || player[1704] ||
            (player[1712] && !LowriderActive.load(std::memory_order_relaxed)) || player[1722])
            return original;
        return std::clamp(original * std::clamp(FovFactor.load(std::memory_order_relaxed), 0.5f, 2.0f), 0.05, 2.2);
    }

    inline void Frame()
    {
        const int boxWidth = std::min(Width, static_cast<int>(std::lround(Height * (4.0f / 3.0f))));
        const int boxHeight = std::min(Height, static_cast<int>(std::lround(Width * (3.0f / 4.0f))));
        const int left = (Width - boxWidth) / 2;
        const int top = (Height - boxHeight) / 2;
        const int state = LARushWindow::GameState;
        // Pause pages, their confirmations and the big map are 4:3 pages over the frozen race.
        const bool movie = std::exchange(MovieFrame, false);
        const bool paused = LARushOverlay::GamePaused.is_initialized() && LARushOverlay::GamePaused && !LARushOverlay::PausedByOverlay;
        if (state != 56 && !movie && (state == 57 || state == 58 || state == 132 || paused) && LARushWindow::Device.get())
            ClearOutside(LARushWindow::Device.get(), left, top, boxWidth, boxHeight);

        const int index = CurrentCamera;
        const auto player = Player.get();
        // Cutscene borders: 1 letterbox (bars above and below on screens narrower than 4:3), 2 pillarbox (side bars on
        // wider screens), 3 both. The story cameras then keep their 4:3 framing inside the box.
        const int borderMode = std::clamp(BorderMode.load(std::memory_order_relaxed), 0, 3);
        const int sceneWidth = (borderMode & 2) ? boxWidth : Width;
        const int sceneHeight = (borderMode & 1) ? boxHeight : Height;
        const int sceneLeft = (Width - sceneWidth) / 2;
        const int sceneTop = (Height - sceneHeight) / 2;
        const bool borders = sceneWidth < Width || sceneHeight < Height;
        const bool lowrider = LARushWindow::IsLowrider && LARushWindow::IsLowrider();
        LowriderActive.store(lowrider, std::memory_order_relaxed);
        const bool special = LARushWindow::GameState == 56 && player && player[1712] && !lowrider && index >= 0 && index < 26;
        auto camera = special ? Cameras.get_ptr() + index * 832 : nullptr;
        if (BorderedCamera && (!borders || !special || camera != BorderedCamera))
        {
            std::copy(std::begin(SavedViewport), std::end(SavedViewport), reinterpret_cast<int32_t*>(BorderedCamera + 400));
            *reinterpret_cast<float*>(BorderedCamera + 452) = SavedAspect;
            *reinterpret_cast<int32_t*>(BorderedCamera + 808) = 1;
            *reinterpret_cast<int32_t*>(BorderedCamera + 812) = 1;
            BorderedCamera = nullptr;
        }
        if (borders && special)
        {
            auto viewport = reinterpret_cast<int32_t*>(camera + 400);
            if (!BorderedCamera)
            {
                std::copy(viewport, viewport + 4, std::begin(SavedViewport));
                SavedAspect = *reinterpret_cast<float*>(camera + 452);
                BorderedCamera = camera;
            }
            viewport[0] = sceneLeft; viewport[1] = sceneTop; viewport[2] = sceneWidth; viewport[3] = sceneHeight;
            *reinterpret_cast<float*>(camera + 452) = static_cast<float>(sceneWidth) / sceneHeight;
            *reinterpret_cast<int32_t*>(camera + 808) = 1;
            *reinterpret_cast<int32_t*>(camera + 812) = 1;
            if (LARushWindow::Device.get())
                ClearOutside(LARushWindow::Device.get(), sceneLeft, sceneTop, sceneWidth, sceneHeight);
        }
        if (Refresh.exchange(false, std::memory_order_acquire) && index >= 0 && index < 26)
        {
            const auto current = Cameras.get_ptr() + index * 832;
            *reinterpret_cast<int32_t*>(current + 808) = 1;
            *reinterpret_cast<int32_t*>(current + 812) = 1;
        }
    }

    inline void Init(int width, int height)
    {
        Width = width; Height = height;
        auto camera = hook::pattern("A3 ? ? ? ? 69 C0 40 03 00 00 05 ? ? ? ? 50 B9 ? ? ? ? E8 ? ? ? ? C3");
        CurrentCamera.SetAddress(*camera.get_first<int32_t*>(1));
        Cameras.SetAddress(*camera.get_first<uint8_t*>(12));
        auto player = hook::pattern("53 8B 1D ? ? ? ? 33 D2 E8 ? ? ? ? 8B 8B 9C 06 00 00 33 C0 3B C8");
        Player.SetAddress(*player.get_first<uint8_t**>(3));
        auto fov = hook::pattern("51 56 8B F1 D9 86 BC 01 00 00 83 EC 08 D8 35 ? ? ? ? D9 5C 24 0C");
        FovHook = safetyhook::create_inline(fov.get_first(), Fov);
        LARushWindow::CameraFrame = Frame;
        BindLiveSetting("MAIN.FOVFactor", FovFactor, []() { Refresh.store(true, std::memory_order_release); });
        BindLiveSetting("MAIN.CutsceneBorders", BorderMode);
    }
}
