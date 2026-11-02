module;

#include <stdafx.h>
#include "FixSettings.hpp"
#include <atomic>


export module Cinema;

import Window;
import Device;

export namespace TrueCrimeCinema
{
    // Story scenes draw black bars whose picture height follows the screen width: 312 of 640 at full slide-in, so the
    // original 4:3 screen showed a 640:312 picture. The picture keeps that framing at any aspect ratio: on wider
    // screens it gets side bars (Pillarbox), and the borders setting can drop either pair of bars.
    inline constexpr float CinemaAspect = 640.0f / 312.0f;
    inline constexpr int FullSlide = 68;

    inline SafetyHookInline LetterboxHook;
    inline std::atomic<uint32_t> LastFrame{ 0 };
    inline thread_local int PictureWidth = 0;

    inline int Borders()
    {
        return std::clamp(FixSettings().Int("MAIN.CutsceneBorders"), 0, 3);
    }

    inline bool Active()
    {
        const uint32_t last = LastFrame.load(std::memory_order_relaxed);
        return last && TrueCrimeDevice::Frames.load(std::memory_order_relaxed) - last <= 2;
    }

    // Width of the picture area that shows the original framing (side bars cover the rest).
    inline float Width()
    {
        const auto width = static_cast<float>(TrueCrimeWindow::Width);
        const auto height = static_cast<float>(TrueCrimeWindow::Height);
        switch (Borders())
        {
            case 3:
                return std::min(width, height * CinemaAspect);
            case 2:
                return std::min(width, height * (4.0f / 3.0f));
            case 1:
                return width;
            default:
                return 0.0f;
        }
    }

    // Aspect ratio for the camera: Hor+ outside story scenes and with the borders off, otherwise the original
    // horizontal view across the picture area.
    inline float CameraAspect(float aspect)
    {
        if (!Active())
            return aspect;
        const float picture = Width();
        if (picture <= 0.0f)
            return aspect;
        return 4.0f / 3.0f * static_cast<float>(TrueCrimeWindow::Width) / picture;
    }

    // The game's 2D calls used by its bars
    inline void* Context = nullptr;
    inline uintptr_t (__fastcall* Begin)(void*, void*) = nullptr;
    inline void (__fastcall* Mode)(uintptr_t, void*, int) = nullptr;
    inline void (__fastcall* Blend)(uintptr_t, void*, int, int) = nullptr;
    inline void (__fastcall* Rectangle)(uintptr_t, void*, float, float, float, float, uint32_t, int) = nullptr;
    inline void (__fastcall* End)(uintptr_t, void*) = nullptr;

    inline void SideBars()
    {
        const auto width = static_cast<float>(TrueCrimeWindow::Width);
        const auto height = static_cast<float>(TrueCrimeWindow::Height);
        const float picture = Width();
        const float side = picture > 0.0f ? std::floor((width - picture) * 0.5f) : 0.0f;
        if (side <= 0.0f || !End)
            return;
        const auto renderer = Begin(Context, nullptr);
        Mode(renderer, nullptr, 2);
        Blend(renderer, nullptr, 7, 8);
        Rectangle(renderer, nullptr, 0.0f, 0.0f, side + 1.0f, height + 1.0f, 0xFF000000, 0);
        Rectangle(renderer, nullptr, width - side, 0.0f, width + 1.0f, height + 1.0f, 0xFF000000, 0);
        End(renderer, nullptr);
    }

    inline int __fastcall Letterbox(int* box, void*, int pass, int a2, int a3, int a4)
    {
        if (pass != 4)
            return LetterboxHook.thiscall<int>(box, pass, a2, a3, a4);
        LastFrame.store(std::max(1u, TrueCrimeDevice::Frames.load(std::memory_order_relaxed)), std::memory_order_relaxed);
        // box[1]: 1 sliding in, 2 sliding out; box[2]: slide position
        if (!FixSettings().Bool("MAIN.AnimatedCutsceneBorders"))
        {
            if (box[1] == 1)
                box[2] = FullSlide - 1;
            else if (box[1] == 2)
                box[2] = 1;
        }
        // The bar height is derived from the width the game reads; give it the picture width, or a width that makes
        // the bars empty when they are off.
        const int borders = Borders();
        const int state = box[1];
        const int slide = std::clamp(box[2] + (state == 1 ? 1 : state == 2 ? -1 : 0),
            0, FullSlide);
        const float visible = 448.0f - 2.0f * static_cast<float>(slide);
        if (borders == 0 || borders == 2)
            PictureWidth = static_cast<int>(std::ceil(static_cast<float>(TrueCrimeWindow::Height) * 640.0f / visible));
        else
            PictureWidth = static_cast<int>(std::lround(Width()));
        const int result = LetterboxHook.thiscall<int>(box, pass, a2, a3, a4);
        PictureWidth = 0;
        SideBars();
        return result;
    }

    // The bars' height is derived from the screen width read at this call
    inline int (__cdecl* ScreenWidth)() = nullptr;
    inline int __cdecl BarWidth()
    {
        return PictureWidth ? PictureWidth : ScreenWidth();
    }

    inline void Init()
    {
        auto letterbox = hook::pattern("8B 44 24 04 83 EC 14 83 F8 04 56 8B F1 0F 85 ? ? ? ? 53 55 E8");
        auto at = [&](ptrdiff_t offset) { return letterbox.get_first<uint8_t>(offset); };
        uint8_t* width = nullptr;
        // Calls inside the bar drawing: renderer, mode, blending, rectangles, end
        if (!letterbox.empty()) // TrueCrime_.exe
        {
            width = at(0x90);
            Context = *reinterpret_cast<void**>(at(0xD2));
            Begin = reinterpret_cast<decltype(Begin)>(injector::GetBranchDestination(at(0xEC)).as_int());
            Mode = reinterpret_cast<decltype(Mode)>(injector::GetBranchDestination(at(0xF7)).as_int());
            Blend = reinterpret_cast<decltype(Blend)>(injector::GetBranchDestination(at(0x102)).as_int());
            Rectangle = reinterpret_cast<decltype(Rectangle)>(injector::GetBranchDestination(at(0x132)).as_int());
            End = reinterpret_cast<decltype(End)>(injector::GetBranchDestination(at(0x176)).as_int());
        }
        else // TrueCrimeMB.exe: the bars go through a rectangle helper that calls the float rectangle
        {
            letterbox = hook::pattern("8B 44 24 04 83 EC 18 83 F8 04 56 8B F1 75 ? 53 33 DB 57");
            width = at(0x9E);
            Context = *reinterpret_cast<void**>(at(0xE1));
            Begin = reinterpret_cast<decltype(Begin)>(injector::GetBranchDestination(at(0xFB)).as_int());
            Mode = reinterpret_cast<decltype(Mode)>(injector::GetBranchDestination(at(0x106)).as_int());
            Blend = reinterpret_cast<decltype(Blend)>(injector::GetBranchDestination(at(0x111)).as_int());
            const auto helper = injector::GetBranchDestination(at(0x123)).as_int();
            Rectangle = reinterpret_cast<decltype(Rectangle)>(injector::GetBranchDestination(helper + 0x58).as_int());
            End = reinterpret_cast<decltype(End)>(injector::GetBranchDestination(at(0x138)).as_int());
        }
        ScreenWidth = reinterpret_cast<decltype(ScreenWidth)>(injector::GetBranchDestination(width).as_int());
        injector::MakeCALL(width, BarWidth, true);
        LetterboxHook = safetyhook::create_inline(letterbox.get_first(), Letterbox);
    }
}
