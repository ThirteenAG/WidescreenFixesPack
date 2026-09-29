module;

#include <stdafx.h>

export module x_helpersmfc;

import ComVars;
import e2mfc;
import x_inputmfc;

export void InitX_HelpersMFC()
{
    auto X_HelpersMFC = GetModuleHandle(L"X_HelpersMFC");

    // X_QuadRenderer draws a sprite covering the whole screen or render target (post-processing,
    // shadow blur) with a camera of its own
    static auto X_QuadRendererRenderHook = safetyhook::create_mid(GetProcAddress(X_HelpersMFC, "?render@X_QuadRenderer@@QAEXXZ"), [](SafetyHookContext& regs)
    {
        KeepOriginalProjection(*(void**)(regs.ecx + 0x08));
    });

    // Loading screens cover the 4:3 area and leave the previous frame on the sides
    auto pattern = hook::module_pattern(X_HelpersMFC, "E8 ? ? ? ? A1 ? ? ? ? 8B 08 FF 15 ? ? ? ? 8B 0D"); // X_ProgressBar::updateProgressBar, before P_Driver::endScene
    static auto X_ProgressBarUpdateProgressBarHook = safetyhook::create_mid(pattern.get_first(5), [](SafetyHookContext& regs) //0x10003051
    {
        RefreshScreenResolution();
        Draw4by3Borders(1);
        UpdateVibration();
    });
}
