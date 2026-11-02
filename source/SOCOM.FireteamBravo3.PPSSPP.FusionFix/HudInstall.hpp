#pragma once
namespace Hud
{
inline uintptr_t Required(const char* signature, int offset = 0)
{
    const auto address = pattern.get_first(signature, offset);
    if (!address)
    {
        logger.Write("Fusion Fix: required display signature missing.\n");
        std::abort();
    }
    return address;
}
inline void ReadOptions()
{
    float value = inireader.ReadFloat("MAIN", "HudScale", 1.0f);
    if (!std::isfinite(value))
        value = 1.0f;
    scale = std::clamp(value, 0.5f, 1.5f);
    char buffer[64]{};
    const auto text = inireader.ReadString("MAIN", "HudAspectRatioConstraint", "Auto", buffer, sizeof(buffer));
    constraint = console::parse_hud_aspect(text);
}
inline void Install()
{
    const auto projectionSite = Required("30 FF BD 27 B4 00 B0 AF B8 00 B1 AF BC 00 B2 AF C0 00 BF AF 00 3E 05 3C");
    cached = reinterpret_cast<const float*>(Absolute(Required("? ? 05 3C ? ? A5 24 00 00 80 D8 10 00 81 D8 20 00 82 D8 30 00 83 D8 ? ? A0 F8 ? ? A1 F8 ? ? A2 F8 ? ? A3 F8 80 3F 05 3C")));
    const auto projectSite = Required("80 FF BD 27 00 00 C4 D8 10 00 C5 D8 20 00 C6 D8 30 00 C7 D8 00 00 E8 D8");
    const auto fillSite = Required("F0 FF BD 27 80 3F 09 3C 00 00 BF AF ? ? ? ? 00 60 89 44 00 00 BF 8F");
    const auto textSite = Required("C0 FF BD 27 0C 00 8B 8C 10 00 B0 AF 14 00 B1 AF 18 00 B2 AF 1C 00 B3 AF");
    const auto promptSite = Required("C0 FF BD 27 00 00 AA AF 1C 00 B4 AF 28 00 B7 AF 25 A0 80 00 40 00 B7 8F");
    const auto dispatchSite = Required("0C 00 86 8C 25 28 00 02 20 00 C6 24 00 00 C7 84 04 00 C6 8C 09 F8 C0 00", 20);
    const auto scissorSite = Required("80 42 05 00 00 D4 09 3C 25 40 09 01 ? ? 09 3C ? ? 2A 8D 21 28 E5 00");
    const auto statsSite = Required("00 60 80 44 D6 01 12 34 00 00 AC E7 3C 00 04 8E 00 04 84 30");
    const auto menuSite = Required("F0 FF BD 27 04 00 B0 AF 25 80 80 00 00 00 B4 E7 08 00 BF AF ? ? ? ? 06 65 00 46 40 00 04 8E 2A 20 04 00 FF 00 84 30 17 00 80 14 25 28 40 00");
    const auto screenSite = Required("F0 FF BD 27 00 00 B0 AF 25 80 80 00 04 00 B1 AF 08 00 BF AF ? ? ? ? FF 00 B1 30 25 20 40 00");
    const auto loadingSite = Required("40 FF BD 27 90 00 B0 AF ? ? 04 3C 10 00 B0 27 94 00 B1 AF 98 00 B2 AF 9C 00 B3 AF A0 00 B4 AF");
    const auto loadingMPSite = Required("40 FF BD 27 A4 00 B0 AF ? ? 04 3C 10 00 B0 27 A8 00 B1 AF AC 00 B2 AF B0 00 B3 AF B4 00 B4 AF");
    for (auto& profile : profiles)
        profile.address = Required(profile.pattern);
    projectionHook = safetymips::create_inline(projectionSite, Projection);
    submit.fun = projectionHook.original<decltype(submit.fun)>();
    projectHook = safetymips::create_inline(projectSite, Project);
    project.fun = projectHook.original<decltype(project.fun)>();
    fillHook = safetymips::create_inline(fillSite, Fill);
    fill.fun = fillHook.original<decltype(fill.fun)>();
    textHook = safetymips::create_inline(textSite, Text);
    text.fun = textHook.original<decltype(text.fun)>();
    promptHook = safetymips::create_inline(promptSite, Prompt);
    prompt.fun = promptHook.original<decltype(prompt.fun)>();
    scissorHook = safetymips::create_inline(scissorSite, Scissor);
    scissor.fun = scissorHook.original<decltype(scissor.fun)>();
    menuHook = safetymips::create_inline(menuSite, Menu);
    menu.fun = menuHook.original<decltype(menu.fun)>();
    screenHook = safetymips::create_inline(screenSite, Screen);
    screen.fun = screenHook.original<decltype(screen.fun)>();
    loadingHook = safetymips::create_inline(loadingSite, Loading);
    loading.fun = loadingHook.original<decltype(loading.fun)>();
    loadingMPHook = safetymips::create_inline(loadingMPSite, LoadingMP);
    loadingMP.fun = loadingMPHook.original<decltype(loadingMP.fun)>();
    safetymips::Options replace;
    replace.execute_original = false;
    replace.preserve = 0;
    dispatchHook = safetymips::create_mid(dispatchSite, [](SafetyMipsContext& regs)
    {
        regs.v0 = Draw(regs.a0 + regs.a3, regs.a1, regs.a2);
    }, replace);
    // The native statistics renderer joins its top panels before starting
    // the bottom timer and bomb indicator. Their text and art move together.
    safetymips::Options scalar;
    scalar.preserve = PSP_HOOK_SAVE_FPU;
    statsHook = safetymips::create_mid(statsSite, [](SafetyMipsContext&)
    {
        StatsBottom();
    }, scalar);
}
}
