#pragma once

namespace BullyCutscene
{
    SafetyHookMid RenderHook, TimeHook;
    uint8_t* Manager = nullptr;
    bool* Widescreen = nullptr;
    float* TimeStep = nullptr;
    float UnclampedStep = 1.0f;
    bool HaveStep = false;
    const int* Mode = nullptr;
    const int* Animated = nullptr;
    const float* Aspect = nullptr;
    int(__cdecl* DrawRectangle)(const float*, const uint32_t*) = nullptr;
    uintptr_t Resume = 0;

    struct Layout
    {
        float progress = 1.0f;
        float top = 0.0f;
        float side = 0.0f;
        float fovScale = 1.0f;
        bool active = false;
    };

    bool Active()
    {
        return Manager && (*reinterpret_cast<int*>(Manager + 164) != 0 || *reinterpret_cast<int*>(Manager + 168) != 0);
    }

    Layout Calculate(float aspect)
    {
        Layout layout;
        if (!Active() || !Mode || !Animated || !Widescreen || !std::isfinite(aspect) || aspect <= 0.0f)
            return layout;
        layout.active = true;
        const int current = *reinterpret_cast<int*>(Manager + 164);
        const int previous = *reinterpret_cast<int*>(Manager + 168);
        const float progress = *reinterpret_cast<float*>(Manager + 172);
        // Native time steps use 1/50-second units.
        const float elapsed = HaveStep ? UnclampedStep : TimeStep ? *TimeStep : 50.0f / 30.0f;
        const float step = std::isfinite(elapsed) ? std::clamp(elapsed, 0.0f, 12.5f) * 0.09f : 0.15f;
        layout.progress = !*Animated || current == previous ? 1.0f :
            std::clamp((std::isfinite(progress) ? progress : 0.0f) + step, 0.0f, 1.0f);
        const float phase = current == previous ? 1.0f : layout.progress;
        const float scene = current == previous ? float(current != 0) :
            current != 0 ? phase : 1.0f - phase;
        const float letterbox = current == previous ? float(current == 1) :
            previous == 1 && current != 1 ? 1.0f - phase : current == 1 ? phase : 0.0f;
        const float nativeTop = *Widescreen ? 0.0f : 50.0f * letterbox;
        const int mode = std::clamp(*Mode, 0, 3);
        layout.top = (mode & 1) ? nativeTop : 0.0f;
        const float nativeVisible = 1.0f - nativeTop / 240.0f;
        const float visible = 1.0f - layout.top / 240.0f;
        const float reference = *Widescreen ? 16.0f / 9.0f : 4.0f / 3.0f;
        const float boxAspect = reference * visible / nativeVisible;
        if (mode & 2)
            layout.side = std::max(0.0f, 1.0f - boxAspect / aspect) * 320.0f * scene;
        const float visibleWidth = 1.0f - layout.side / 320.0f;
        layout.fovScale = std::min(aspect / reference * nativeVisible / visible, 1.0f / visibleWidth);
        return layout;
    }

    float FOV(float original, float aspect)
    {
        const auto layout = Calculate(aspect);
        if (!layout.active)
            return original;
        constexpr float radians = 0.017453292519943295f;
        const float value = std::isfinite(original) ? std::clamp(original, 1.0f, 179.0f) : 60.0f;
        return std::clamp(2.0f * std::atan(std::tan(value * radians * 0.5f) * layout.fovScale) / radians, 1.0f, 179.0f);
    }

    void Render()
    {
        if (!Manager || !Aspect || !DrawRectangle)
            return;
        const auto layout = Calculate(*Aspect);
        if (!layout.active)
            return;
        const int current = *reinterpret_cast<int*>(Manager + 164);
        const int previous = *reinterpret_cast<int*>(Manager + 168);
        if (current != previous)
        {
            *reinterpret_cast<float*>(Manager + 172) = layout.progress;
            if (layout.progress >= 1.0f)
                *reinterpret_cast<int*>(Manager + 168) = current;
        }
        const uint32_t black = 0xFF000000;
        auto rectangle = [&](float left, float top, float right, float bottom)
        {
            if (right > left && bottom > top)
            {
                const float bounds[] = { left, top, right, bottom };
                DrawRectangle(bounds, &black);
            }
        };
        rectangle(0.0f, 0.0f, 640.0f, layout.top);
        rectangle(0.0f, 480.0f - layout.top, 640.0f, 480.0f);
        rectangle(0.0f, layout.top, layout.side, 480.0f - layout.top);
        rectangle(640.0f - layout.side, layout.top, 640.0f, 480.0f - layout.top);
    }

    void ResetHooks()
    {
        RenderHook.reset();
        TimeHook.reset();
        Manager = nullptr;
        HaveStep = false;
    }

    void Install(const int& mode, const int& animated, const float& aspect, HMODULE module = GetModuleHandle(nullptr))
    {
        auto render = hook::module_pattern(module, "A1 ? ? ? ? 83 F8 01 74 08 3B 05 ? ? ? ? 74 13 80 3D ? ? ? ? 00 75 0A B9 ? ? ? ? E8 ? ? ? ? A1");
        auto rectangle = hook::module_pattern(module, "83 EC 0C 56 8D 44 24 04 50 6A 08 E8 ? ? ? ? 8D 4C 24 10 51 6A 06 E8");
        auto time = hook::module_pattern(module, "D9 1D ? ? ? ? DB 05 ? ? ? ? DE C2 D9 C9 E8 ? ? ? ? D9 05 ? ? ? ? 8B F0 D9 05 ? ? ? ? 8B 0D");
        if (render.size() != 1 || rectangle.size() != 1 || time.size() != 1)
        {
            OutputDebugStringA("Bully: cutscene framing patterns unavailable.\n");
            return;
        }
        Manager = *render.get_first<uint8_t*>(28);
        if (*render.get_first<uint8_t*>(1) != Manager + 164 || *render.get_first<uint8_t*>(12) != Manager + 168)
        {
            Manager = nullptr;
            OutputDebugStringA("Bully: cutscene framing state unavailable.\n");
            return;
        }
        Widescreen = *render.get_first<bool*>(20);
        TimeStep = *time.get_first<float*>(2);
        Mode = &mode;
        Animated = &animated;
        Aspect = &aspect;
        DrawRectangle = reinterpret_cast<decltype(DrawRectangle)>(rectangle.get_first());
        Resume = reinterpret_cast<uintptr_t>(render.get_first(37));
        TimeHook = safetyhook::create_mid(time.get_first(6), [](SafetyHookContext&)
        {
            UnclampedStep = *TimeStep;
            HaveStep = true;
        });
        RenderHook = safetyhook::create_mid(render.get_first(), [](SafetyHookContext& regs)
        {
            Render();
            regs.eip = Resume;
        });
        if (!RenderHook || !TimeHook)
        {
            ResetHooks();
            OutputDebugStringA("Bully: cutscene framing hook unavailable.\n");
        }
    }
}
