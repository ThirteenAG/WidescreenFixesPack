#pragma once
namespace Startup
{
inline bool skipPressStart{ true }, advancing;
inline int (*complete)(uintptr_t);
inline SafetyMipsInline noticeHook;
inline injector::hook_back<int(const char*)> notice;
inline int Notice(const char* key)
{
    if (options.skipIntro && (std::strcmp(key, "text_autosave_warning") == 0 ||
        std::strcmp(key, "text_profile_loaded") == 0 ||
        std::strcmp(key, "text_create_new_profile") == 0))
        return 0;
    return notice.fun(key);
}
inline void Advance(uintptr_t screen)
{
    if (!screen)
        return;
    const auto name = *reinterpret_cast<const char* const*>(screen + 4);
    const bool start = std::strcmp(name, "StartScreen") == 0;
    if (!start)
        advancing = false;
    else if (skipPressStart && !advancing)
    {
        advancing = true;
        // Native completion retains profile creation and every genuine choice.
        complete(screen);
        logger.Write("Native press-start screen completed.\n");
    }
}
inline void Install()
{
    const auto completeSite = Hud::Required("F0 FF BD 27 00 00 B0 AF 04 00 B1 AF 25 80 80 00 ? ? 11 3C ? ? 24 8E 25 28 00 00 08 00 BF AF ? ? ? ? 25 30 00 00 0F 00 40 10 00 00 00 00");
    const auto noticeCall = Hud::Required("? ? A5 24 ? ? 05 3C 25 20 00 02 ? ? ? ? ? ? A5 24 ? ? 24 8E 01 00 05 34", 12);
    complete = reinterpret_cast<decltype(complete)>(completeSite);
    const uintptr_t wrapper = injector::GetBranchDestination(noticeCall).as_int();
    const uintptr_t noticeSite = injector::GetBranchDestination(wrapper + 8).as_int();
    noticeHook = safetymips::create_inline(noticeSite, Notice);
    notice.fun = noticeHook.original<decltype(notice.fun)>();
}
}
