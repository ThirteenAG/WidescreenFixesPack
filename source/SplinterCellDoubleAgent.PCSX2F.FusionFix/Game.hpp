#pragma once
#include "../../external/injector/include/ps2/runtime.hpp"
#include "../../external/injector/include/ps2/game_abi.hpp"
#include "../Shared/Console/PS2Display.hpp"
extern "C" {
#include "../../external/injector/include/ps2/patterns.h"
#include "../../external/injector/include/ps2/log.h"
extern int PCSX2Data[];
extern char OSDText[OSDStringNum][OSDStringSize];
extern char FrameLimitUnthrottle;
}
namespace scda {
struct Addresses {
    uintptr_t projection, projectionCaller, image, text, border, rect, quad, vertex, context, engine;
    // Magma: node::Render(node {vtbl, capacity, count, children}), child::Render(child
    // {vtbl, id, object}), the vtable of the area node object (+32 its package) and
    // the element vtables: area {vtbl, colour, x, y}; image, text and solid rect
    // {vtbl, colour, left, right, top, bottom} (int16 coordinates).
    uintptr_t nodeRender, childRender, areaInstance, areaElement, imageElement, textElement, rectElement, perspectiveCaller;
    uintptr_t movieUpload, movieRingReset, movieRingBase, movieRingIndex, loadingDraw, loadingEnd;
};
inline constexpr Addresses US{0x197100, 0x25F5AC, 0x2EA4E0, 0x2EC3C0, 0x2EE8F0,
    0x26E4B0, 0x26E5F0, 0x26EDB0, 0x964520, 0x9645D8, 0x655B70, 0x66C960, 0x87E8A0,
    0x87F1A0, 0x87EF70, 0x87F470, 0x87F2C0, 0x25F888, 0x625400, 0x6446D0, 0x9512D0, 0x951320, 0x16A9D8, 0x16AAF0};
inline constexpr Addresses EU{0x197160, 0x25F61C, 0x2EA520, 0x2EC5B0, 0x2EEBE0,
    0x26E520, 0x26E650, 0x26EE10, 0x964A20, 0x964AD8, 0x655FF0, 0x66CDE0, 0x87EDA0,
    0x87F6A0, 0x87F470, 0x87F970, 0x87F7C0, 0x25F8F8, 0x625910, 0x644B50, 0x9517D0, 0x951820, 0x16AA28, 0x16AB40};
inline Addresses game{};
inline int textLanguage = 0;
inline const char* Notice(unsigned message)
{
    static constexpr const char* texts[3][5] = {
        {"Launcher fix disabled: unsupported game code.", "Correctif du lanceur desactive : version non prise en charge.", "Launcher-Fix deaktiviert: Spielversion nicht unterstuetzt.", "Correzione launcher disattivata: versione non supportata.", "Correccion del lanzador desactivada: version no compatible."},
        {"Double Agent fix disabled: unsupported game code.", "Correctif Double Agent desactive : version non prise en charge.", "Double-Agent-Fix deaktiviert: Spielversion nicht unterstuetzt.", "Correzione Double Agent disattivata: versione non supportata.", "Correccion de Double Agent desactivada: version no compatible."},
        {"Launcher videos: unsupported game code.", "Videos du lanceur : version non prise en charge.", "Launcher-Videos: Spielversion nicht unterstuetzt.", "Video launcher: versione non supportata.", "Videos del lanzador: version no compatible."}
    };
    return texts[message][textLanguage];
}

inline float hudSize = 1;
inline float forcedAspect = 0;
inline float hudConstraint = 0;
inline float fovFactor = 1;
inline int videoMode = 1;
inline int cinemaMode = 3;
inline bool cinemaAnimated = true;
inline float cinemaVisible = 1;
inline float Aspect() { return forcedAspect > 0 ? forcedAspect : console::ps2Aspect(PCSX2Data); }
inline bool traceDraws = false;
template<class T> T Read(uintptr_t address) { return *reinterpret_cast<const T*>(address); }
void InstallHud();
void InstallMenuMovie();
void InstallLauncherMovies();
void InstallLoading();
void UpdateSettings(bool movie = false);
}
