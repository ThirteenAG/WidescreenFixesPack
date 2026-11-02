module;

#include <stdafx.h>
#include "FixSettings.hpp"

export module Misc;

import Camera;
import Cutscenes;
import Hud;
import Movies;
import Renderer;
import Resolution;
import Window;
import ComVars;

class Misc
{
public:
    Misc()
    {
        WFP::onInitEvent() += []()
        {
            hook::pattern pattern;
            auto updateFOV = []()
            {
                const float factor = FixSettings().Float("MAIN.FOVFactor");
                SecondSightCamera::Factor.store(std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f,
                    std::memory_order_relaxed);
            };
            updateFOV();
            FixSettings().OnChange("MAIN.FOVFactor", updateFOV);

            pattern = hook::pattern("39 3D ? ? ? ? 74 ? 8B 06 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 85 DB");
            ActiveCutscene.SetAddress(*pattern.get_first<uint32_t*>(2));
            SecondSightCamera::InCutscene = []() { return ActiveCutscene.get() != 0; };

            pattern = hook::pattern("8B 44 24 08 D9 44 24 10 8B 4C 24 04 D8 0D ? ? ? ? 8B 54 24 0C 50 A1 ? ? ? ?");
            SecondSightCamera::ProjectionHook = safetyhook::create_mid(pattern.get_first(), &SecondSightCamera::Projection);
            auto frustum = hook::pattern("DB 05 ? ? ? ? D8 0D ? ? ? ? DB 05 ? ? ? ? D8 0D ? ? ? ? DE F9 DA 48 0C DA 70 20 8B 86 74 03 00 00");
            SecondSightCamera::FrustumAngleHook = safetyhook::create_mid(frustum.get_first(), &SecondSightCamera::FrustumAngle);
            SecondSightCamera::FrustumArgumentHook = safetyhook::create_mid(frustum.get_first(0x2F), &SecondSightCamera::FrustumArgument);
            pattern = hook::pattern("A3 ? ? ? ? E8 ? ? ? ? 50 E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? A3 ? ? ? ? E8 ? ? ? ? 83 C4 44"); //0x40F2CD
            SecondSightResolution::Init(pattern);
            SecondSightWindow::Init(FixSettings().Int("MAIN.WindowMode"));
            SecondSightRenderer::Init();
            SecondSightMovies::Init();

            // Main camera: renderer height * factor / (renderer width * c); the factor carries the aspect ratio
            pattern = hook::pattern("D8 0D ? ? ? ? 51 DB 05 ? ? ? ? 8B 0D ? ? ? ? D8 0D ? ? ? ? DE F9 D9 1C 24 50 51 E8");
            injector::WriteMemory<float*>(pattern.get_first<float*>(2), &Aspect::Factor, true);
            RenderWidth.SetAddress(*pattern.get_first<uint32_t*>(9));
            RenderHeight.SetAddress(RenderWidth.get_ptr() + 1);
            pattern = hook::pattern("68 ? ? ? ? 52 E8 ? ? ? ? 8D 44 24 60 83");
            FrontendAspect.SetAddress(pattern.get_first<float>(1)); //0x55BEB0
            pattern = hook::pattern("D8 0D ? ? ? ? D9 9C 24 88 00 00 00 E8");
            OverlayAspect.SetAddress(*pattern.get_first<float*>(2)); //0x71C714
            SecondSightHud::Init();
            SecondSightCutscenes::Init();
            WFP::onGameProcessEvent() += []()
            {
                Aspect::Update();
                SecondSightHud::Update();
            };
        };
    }
} Misc;
