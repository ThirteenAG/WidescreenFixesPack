module;

#include <stdafx.h>
#include "common.h"
#include "TextOverlay.hpp"
#include "Shared/RWGTA/UpdateCheck.hpp"

export module Frontend;

import NativeMenuView;


import NativeMenu;
import Updates;
import Controller;
import Vibration;
import ButtonIcons;

import Skeleton;
import Draw;
import Renderer;


class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            // CFrontendMenu: the menu cursor moves by the mouse's movement in screen units. The menus are drawn in
            // the 4:3 box, so movement follows the same scaling as its vertices, on wide and narrow screens.
            auto pattern = hook::pattern("D9 04 24 D8 35 ? ? ? ? 85 ED D9 1D ? ? ? ? D9 44 24 04 D8 35 ? ? ? ? D9 1D ? ? ? ? 74 0E");
            static const auto mouseMoveBound = [&]()
            {
                MouseMoveX.SetAddress(*pattern.get_first<float*>(13));
                MouseMoveY.SetAddress(*pattern.get_first<float*>(29));
                return true;
            }();
            static auto MouseMove = safetyhook::create_mid(pattern.get_first(33), [](SafetyHookContext& regs)
            {
                const float aspect = GetScreenAspectRatio();
                if (aspect > 0.0f)
                    injector::WriteMemory<float>(MouseMoveX.get_ptr(), MouseMoveX.get() / CDraw::GetDesignScale(aspect), true);
                if (aspect > 0.0f && aspect < DEFAULT_ASPECT_RATIO)
                    injector::WriteMemory<float>(MouseMoveY.get_ptr(), MouseMoveY.get() / (aspect / DEFAULT_ASPECT_RATIO), true);
            });
            MHMenu::Init();
            MHUpdates::Install();
            MHPad::Install();
            MHIcons::Install();
            if (MHMenu::Storage() && MHMenu::Storage()->enabled)
            {
                MHRumble::inputActive = []() { return MHPad::lastPad.load(); };
                MHRumble::Install();
            }
        };
        WFP::onGameProcessEvent() += MHMenu::Tick;
        WFP::onGameProcessEvent() += RWGTA::UpdateCheck::Tick;
        WFP::onShutdownEvent() += MHRumble::Shutdown;
    }
} Frontend;
