#include "Game.hpp"
#include "Projection.hpp"
#include "CinemaGeometry.hpp"
#include "LoadingGeometry.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
PCSX2F_CFG_SETTINGS()
PCSX2F_CFG_PEER_SETTINGS()
PCSX2F_STATE_LOAD_GENERATION()
PCSX2F_EFFECTIVE_ASPECT()
extern "C"
{
int CompatibleCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9)};
int CompatibleElfCRCList[] = {static_cast<int>(0xC0498D24), static_cast<int>(0xABE2FDE9), 0x0198F1AD, 0x6BD0E9C2};
int PCSX2Data[PCSX2Data_Size] = {1};
char FrameLimitUnthrottle = 0;
char OSDText[OSDStringNum][OSDStringSize] = {{1}};
}
namespace
{
SafetyMipsMid projection, viewProjection, launcherDraw;
safetymips::GameInline<uint64_t(void*, void*)> launcherCopy;
bool launcherPresenting;
// Whole-buffer copies (the finished frame, saved backgrounds) keep their size.
uint64_t CopyLauncherBuffer(void* target, void* source)
{
    launcherPresenting = true;
    const auto result = launcherCopy.call(target, source);
    launcherPresenting = false;
    return result;
}
constexpr console::ps2settings::Value defaults[] = {{"MAIN", "SkipIntro", "1"}, {"MAIN", "HudScale", "1.0"},
    {"MAIN", "ForceAspectRatio", "Auto"}, {"MAIN", "HudAspectRatioConstraint", "Auto"},
    {"MAIN", "FOVFactor", "1.0"}, {"MAIN", "FMVWidescreenMode", "1"},
    {"MAIN", "CutsceneBorders", "3"}, {"MAIN", "AnimatedCutsceneBorders", "1"}, {"MAIN", "TextLanguage", "auto"}
#ifdef WFP_PS2_DEBUG
    , {"DEBUG", "TraceDraws", "0"}
#endif
};
console::ps2settings::Value options[sizeof(defaults) / sizeof(*defaults)];
uint32_t lastAttemptGeneration;
unsigned projectionCalls, movieCalls;
bool ReadOptions(bool startup)
{
    decltype(options) next;
    std::memcpy(next, defaults, sizeof(next));
    if (!(startup ? console::ps2settings::load(next) : console::ps2settings::read(next))) return false;
    std::memcpy(options, next, sizeof(options));
    return true;
}
void ApplySettings()
{
    using namespace scda;
    forcedAspect = console::parse_forced_aspect(options[2].value);
    long mode = 1;
    videoMode = console::ps2settings::integer(options[5], mode) ? int(std::clamp(mode, 0L, 1L)) : 1;
    char* end = nullptr;
    const float scale = std::strtof(options[1].value, &end);
    while (end && (*end == ' ' || *end == '\t')) ++end;
    hudSize = end && end != options[1].value && !*end && std::isfinite(scale) ? std::clamp(scale, 0.5f, 1.5f) : 1;
    hudConstraint = console::parse_hud_aspect(options[3].value);
    const float fov = std::strtof(options[4].value, &end);
    const bool parsedFov = end != options[4].value;
    while (end && (*end == ' ' || *end == '\t')) ++end;
    fovFactor = parsedFov && end && !*end && std::isfinite(fov) ? std::clamp(fov, 0.5f, 2.0f) : 1.0f;
    long borders = 3;
    cinemaMode = console::ps2settings::integer(options[6], borders) ? int(std::clamp(borders, 0L, 3L)) : 3;
    cinemaAnimated = console::ps2settings::boolean(options[7], true);
    constexpr const char* languages[] = {"en", "fr", "de", "it", "es"};
    textLanguage = 0;
    for (int i = 0; i < 5; ++i)
    {
        if (!std::strcmp(options[8].value, languages[i])) textLanguage = i;
    }
#ifdef WFP_PS2_DEBUG
    traceDraws = console::ps2settings::boolean(options[9], false);
#endif
}
bool AimCamera(uint8_t mode)
{
    // First-person, scope, EEV, peek and fixed special-action cameras.
    switch (mode)
    {
    case 1: case 2: case 3: case 5: case 8: case 9: case 10: case 11:
    case 16: case 18: case 19: case 20: case 22: case 24: case 25: case 26:
    case 32: case 33: case 34: case 35:
        return true;
    default:
        return false;
    }
}
float ViewZoom(uintptr_t view)
{
    using namespace scda;
    float factor = fovFactor, cinema = 1.0f;
    const auto scene = view ? Read<uintptr_t>(view) : 0;
    const auto controller = scene ? Read<uintptr_t>(scene + 52) : 0;
    if (controller)
    {
        // Scripted cameras and the native orthographic views retain their lens.
        const auto projectionMode = Read<uint32_t>(controller + 0x3D4);
        if ((Read<uint32_t>(controller + 988) & 0x200000) || (projectionMode >= 13 && projectionMode <= 15))
            factor = 1.0f;
    }
    const auto camera = controller ? Read<uintptr_t>(controller + 0x780) : 0;
    if (camera && AimCamera(Read<uint8_t>(camera + 688))) factor = 1.0f;
    if (cinemaVisible < .999f)
    {
        factor = 1.0f;
        cinema = CinemaFit(Aspect(), cinemaAnimated ? cinemaVisible : 254.0f / 448.0f, cinemaMode).zoom;
    }
    // This is the same per-view lens that the native culling code reads.
    const float degrees = view ? Read<float>(view + 0x48C) : 0.0f;
    return FovZoom(degrees, factor) * cinema;
}
void Rejected(pcsx2_hook_status status)
{
    std::snprintf(OSDText[0], OSDStringSize, "Double Agent fix disabled: patch validation failed (%u)", unsigned(status));
}
enum class LauncherResult { NotLauncher, Installed, Unsupported };
LauncherResult Launcher(bool skip)
{
    const auto branch = pattern.get_first("00 00 00 00 ? ? ? ? 00 00 00 00 ? ? ? ? 2D 28 00 00 ? ? ? ? 2D 30 00 00 2D 38 00 00 E8 03 08 24 ? ? 08 0C 2D 48 00 00", -12);
    if (!branch) return LauncherResult::NotLauncher;
    // This reset breaks the LoadExecPS2 handoff in the tested PCSX2 fork.
    // Keep the argument setup and LoadExecPS2 call and omit only the reset.
    const auto reset = pattern.get_first("50 59 08 0C 2D 88 A0 00 96 59 08 0C 2D 80 40 00 2F 00 02 3C 2D 20 00 02", 8);
    // Launcher screens draw every sprite and glyph through one GIF register
    // writer (a1 register, a2 value). Fit them into the 4:3 frame around the
    // GS centre; untextured fills that reach the frame edges stay full width.
    const auto writer = pattern.get_first("00 00 82 8C 3C 28 05 00 3E 28 05 00 00 00 46 FC 08 00 42 24 00 00 45 FC 08 00 42 24 08 00 E0 03 00 00 82 AC", 0);
    const auto copy = pattern.get_first("60 FF BD 27 24 00 06 24 60 00 BF FF 50 00 B1 7F 40 00 B0 7F 2D 88 80 00 2D 80 A0 00 70 00 A4 27 ? ? ? 0C 2D 28 00 00 01 00 04 24 10 00 03 24", 0);
    if (!reset || !writer || !copy)
    {
        std::snprintf(OSDText[0], OSDStringSize, "%s", scda::Notice(0));
        return LauncherResult::Unsupported;
    }
    injector::MakeNOP(reset);
    if (skip)
    {
        injector::MakeNOP(branch + 48);
    }
    launcherCopy.create(copy, CopyLauncherBuffer);
    launcherDraw = safetymips::create_mid(writer, [](SafetyMipsContext& regs)
    {
        static bool textured;
        static int frameHalf;
        if (regs.a1 == 0) textured = regs.a2 & 0x10;                   // PRIM.TME
        else if (regs.a1 == 24) frameHalf = 0x8000 - int(regs.a2 & 0xFFFF); // XYOFFSET_1
        else if ((regs.a1 == 4 || regs.a1 == 5) && !launcherPresenting) { // XYZF2, XYZ2
            const int x = int(regs.a2 & 0xFFFF) - 0x8000;
            if (!textured && frameHalf > 0 && (x <= -frameHalf || x >= frameHalf)) return;
            regs.a2 = uint32_t(scda::FitLoadingVertex(regs.a2, scda::Aspect()));
        }
    });
    return LauncherResult::Installed;
}
}
void scda::UpdateSettings(bool movie)
{
    const uint32_t generation = *reinterpret_cast<volatile uint32_t*>(&PCSX2FStateLoadGeneration);
    const bool restored = generation != lastAttemptGeneration;
    // Check after 30 view builds or 60 movie uploads.
    unsigned& calls = movie ? movieCalls : projectionCalls;
    if (!restored && ++calls < (movie ? 60u : 30u)) return;
    if (restored)
    {
        lastAttemptGeneration = generation;
        // A refused read retries on the next polling interval in either path.
        projectionCalls = movieCalls = 0;
    }
    else
    {
        calls = 0;
    }
    if (ReadOptions(false)) ApplySettings();
}
extern "C" void init()
{
    using namespace scda;
    static constexpr const char* oldPlugins[] = {
        "SplinterCellDoubleAgent.PCSX2F.WidescreenFix.elf",
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText)) return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK) return;
    std::memcpy(options, defaults, sizeof(options));
    if (!ReadOptions(true))
        std::snprintf(OSDText[0], OSDStringSize, "CFG settings unavailable: update the plugin injector for CFG support.");
    ApplySettings();
    const auto launcher = Launcher(console::ps2settings::boolean(options[0], true));
    if (launcher != LauncherResult::NotLauncher)
    {
        if (launcher == LauncherResult::Installed)
        {
            InstallLauncherMovies();
        }
        injector::FlushCaches();
        return;
    }
    const auto us = pattern.get_first("00 60 80 44 80 3F 02 3C 00 70 82 44 B0 06 A4 27 46 63 00 46 40 5C 06 0C C6 63 00 46", 0);
    if (us)
    {
        game = US;
    }
    else
    {
        const auto eu = pattern.get_first("00 60 80 44 80 3F 02 3C 00 70 82 44 B0 06 A4 27 46 63 00 46 58 5C 06 0C C6 63 00 46", 0);
        if (!eu)
        {
            std::snprintf(OSDText[0], OSDStringSize, "%s", Notice(1));
            injector::FlushCaches();
            return;
        }
        game = EU;
    }
#ifdef WFP_PS2_DEBUG // build-module.ps1 -Defines WFP_PS2_DEBUG
    logger.SetBuffer(OSDText, OSDStringNum, OSDStringSize);
#endif
    // Refresh before any row in this view, so live settings apply to both
    // the displayed matrix and the culling matrix for the whole view.
    viewProjection = safetymips::create_mid(game.viewProjection, [](SafetyMipsContext&)
    {
        UpdateSettings();
    });
    projection = safetymips::create_mid(game.projection, [](SafetyMipsContext& regs)
    {
        // Return addresses of the viewport's four matrix paths and the
        // orthographic and perspective culling rows.
        switch (uintptr_t(regs.ra) - game.viewProjection)
        {
        case 0x538: case 0x678: case 0x798: case 0x8CC:
        case 0xAA4: case 0xBA8:
            regs.f12 = regs.f12 * ((4.0f / 3.0f) / Aspect()) * ViewZoom(uintptr_t(regs.s2));
            break;
        case 0x51C: case 0x640: case 0x77C: case 0x8B0:
        case 0xAD0: case 0xBD4:
            regs.f13 = regs.f13 * ViewZoom(uintptr_t(regs.s2));
            break;
        default:
            break;
        }
    });
    InstallHud();
    InstallMenuMovie();
    InstallLoading();
    injector::FlushCaches();
}
extern "C" int main() { return 0; }
