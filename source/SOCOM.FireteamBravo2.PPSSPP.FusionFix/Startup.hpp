#pragma once

namespace Startup
{
inline bool active{ true }, logosSkipped, startCompleted;
inline uintptr_t title;
inline int startupFrame{ -1 }, idleFrame{ -1 };
inline SafetyMipsInline keyHook;
inline injector::hook_back<void(uintptr_t)> keys;
inline int (*labelAt)(const char*, uintptr_t);
inline uintptr_t* popupHead;
inline int (*closePopup)(const uint32_t*, int);

inline void Frame()
{
    if (!options.skipIntro || !active || !*popupHead || *popupHead == uintptr_t(popupHead))
        return;
    const auto popup = *popupHead + 8;
    const int kind = *reinterpret_cast<const int*>(popup);
    // Native types zero and two are single OK/Continue acknowledgments.
    // Type one has two choices; type three is a progress message.
    if ((kind != 0 && kind != 2) || *reinterpret_cast<const int*>(popup + 12) != 1)
        return;
    *reinterpret_cast<int*>(popup + 8) = 0;
    const uint32_t owner = *reinterpret_cast<const uint32_t*>(popup + 540);
    closePopup(&owner, 0);
    logger.Write("Native startup acknowledgment completed.\n");
}
inline void Keys(uintptr_t movie)
{
    keys.fun(movie);
    if (!options.skipIntro || !active || startCompleted || movie != title)
        return;
    const auto clip = *reinterpret_cast<const uintptr_t*>(movie + 3164);
    if (!clip || *reinterpret_cast<const int*>(clip + 16) != idleFrame)
        return;
    // Complete the title's own confirm action once. Its script chooses between
    // loading an existing profile and opening the native profile-name keyboard.
    *reinterpret_cast<uint32_t*>(movie + 3192) |= 0x10;
    *reinterpret_cast<uint32_t*>(movie + 3200) |= 0x10;
    startCompleted = true;
    logger.Write("Native title start action completed.\n");
}
inline void Movie(uint32_t hash, uintptr_t movie)
{
    if (hash == Hud::Hash("lobby_Main.swf") || hash == Hud::Hash("mainMenu.swf"))
        active = false;
    if (!options.skipIntro || !active || hash != Hud::Hash("splash.swf"))
        return;
    const auto clip = *reinterpret_cast<const uintptr_t*>(movie + 3164);
    if (!clip)
        return;
    if (title != movie)
    {
        title = movie;
        startupFrame = labelAt("startup", clip);
        idleFrame = labelAt("SplashIdle", clip);
        if (startupFrame < 0 || idleFrame < 0)
        {
            active = false;
            static constexpr const char* messages[] = {
                "Fusion Fix: startup skip unavailable; title data not recognized.\n",
                "Fusion Fix : saut du demarrage indisponible ; donnees du titre inconnues.\n",
                "Fusion Fix: Startueberspringen nicht verfuegbar; Titeldaten unbekannt.\n",
                "Fusion Fix: salto avvio non disponibile; dati del titolo sconosciuti.\n",
                "Fusion Fix: salto de inicio no disponible; datos del titulo desconocidos.\n"
            };
            PatchNotice(messages[NativeMenu::Language()]);
            return;
        }
    }
    if (options.skipIntro && !logosSkipped && *reinterpret_cast<const int*>(clip + 16) < startupFrame)
    {
        // Keep the first frame's script initialization, then enter the game's
        // normal profile initialization after the publisher-title animation.
        *reinterpret_cast<int*>(clip + 20) = startupFrame;
        *reinterpret_cast<int*>(clip + 24) = 0x81;
        logosSkipped = true;
        logger.Write("Native publisher-title animation completed.\n");
    }
}
inline void Install()
{
    const auto closeSite = sites::Required("60 FD BD 27 7C 02 B0 AF 25 80 80 00 80 02 B1 AF FF 00 B1 30 ? ? 04 3C ? ? 85 8C 84 02 B2 AF");
    const auto labelSite = sites::Required("D0 FF BD 27 20 00 B4 AF 1C 00 B4 8C 14 00 B1 AF 04 00 86 96 1C 00 B3 AF 00 00 13 34 25 88 80 00");
    const auto keySite = sites::Required("F0 FF BD 27 B0 0C 85 8C 00 00 B0 AF 25 80 80 00 2B 20 05 00 FF 00 84 30 04 00 BF AF 06 00 80 54");
    labelAt = reinterpret_cast<decltype(labelAt)>(labelSite);
    popupHead = reinterpret_cast<uintptr_t*>(Absolute(closeSite, 20, 24) - 12);
    closePopup = reinterpret_cast<decltype(closePopup)>(closeSite);
    keyHook = safetymips::create_inline(keySite, Keys);
    keys.fun = keyHook.original<decltype(keys.fun)>();
    Hud::afterMovie = Movie;
    Hud::afterFrame = Frame;
}
}
