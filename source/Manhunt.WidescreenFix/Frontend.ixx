module;

#include <stdafx.h>
#include "../Shared/CheckedPattern.hpp"
#include "common.h"
#include "NativeMenu.hpp"

export module Frontend;

import Skeleton;
import Draw;

class Frontend
{
public:
    Frontend()
    {
        WFP::onInitEvent() += []()
        {
            // CFrontendMenu: the menu cursor moves by the mouse's movement in screen units. The menus are drawn in
            // the 4:3 box, so a sideways movement becomes larger in the box's units: the cursor follows the mouse
            // at the same speed in both directions, and it stays over what it points at.
            auto pattern = hook::pattern("D9 04 24 D8 35 ? ? ? ? 85 ED D9 1D ? ? ? ? D9 44 24 04");
            if (!pattern.empty())
            {
                static auto MouseMoveX = *pattern.get_first<float*>(13);
                static auto MouseMove = safetyhook::create_mid(pattern.get_first(17), [](SafetyHookContext& regs)
                {
                    const float aspect = GetScreenAspectRatio();
                    if (aspect > 0.0f)
                        *MouseMoveX /= CDraw::GetDesignScale(aspect);
                });
            }
            MHMenu::Init();
        };
        WFP::onGameProcessEvent() += MHMenu::Tick;
    }
} Frontend;
