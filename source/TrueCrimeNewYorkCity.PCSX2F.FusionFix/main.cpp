#include "Game.hpp"
#include "Sites.hpp"
#include "../Shared/Console/PS2Settings.hpp"
#include "../Shared/Console/Ps2PluginConflict.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
PCSX2F_CFG_SETTINGS()
PCSX2F_CFG_PEER_SETTINGS()
PCSX2F_STATE_LOAD_GENERATION()

extern "C"
{
    int CompatibleCRCList[] = {0x42A9C4EC, 0x7A9D67B8, 0x77B4F13C, static_cast<int>(0xB1AC3BEB)};
    int CompatibleElfCRCList[] = {0x42A9C4EC, 0x1118ACD0, 0x7A9D67B8, static_cast<int>(0xB73CDCFA),
                                  0x77B4F13C, static_cast<int>(0xA4334B91), static_cast<int>(0xB1AC3BEB),
                                  static_cast<int>(0xB2D44C6C)};
    int PCSX2Data[PCSX2Data_Size] = {1};
    char OSDText[OSDStringNum][OSDStringSize] = {{1}};
    char FrameLimitUnthrottle;
}

namespace
{
    using namespace tcny;
    console::ps2settings::Value options[] = {
        {"MAIN", "SkipIntro", "1"},          {"MAIN", "ForceAspectRatio", "Auto"},
        {"MAIN", "HudAspectRatioConstraint", "Auto"}, {"MAIN", "FOVFactor", "1.0"},
        {"MAIN", "CutsceneBorders", "3"},    {"MAIN", "AnimatedCutsceneBorders", "1"},
        {"MAIN", "FPSLimit", "60"},          {"MAIN", "UnthrottleEmuDuringLoading", "1"}};
    console::ps2settings::StateLoads stateLoads;

    void ApplySettings()
    {
        forcedAspect = console::parse_forced_aspect(options[1].value);
        hudAspect = console::parse_hud_aspect(options[2].value);
        char* end = nullptr;
        const float factor = std::strtof(options[3].value, &end);
        fovFactor = end != options[3].value && std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
        cutsceneBorders = std::clamp(std::atoi(options[4].value), 0, 3);
        animatedBorders = console::ps2settings::boolean(options[5], true);
    }
    void Rejected(pcsx2_hook_status status)
    {
        std::snprintf(OSDText[0], OSDStringSize, "True Crime fix disabled: patch validation failed (%u)", unsigned(status));
    }
    uintptr_t Absolute(uintptr_t at)
    {
        return (uintptr_t(injector::ReadMemory<uint16_t>(at)) << 16) + int16_t(injector::ReadMemory<uint16_t>(at + 4));
    }
    bool Required(uintptr_t value, const char* name)
    {
        if (value)
        {
            return true;
        }
        std::snprintf(OSDText[0], OSDStringSize, "True Crime fix disabled: missing %s pattern", name);
        return false;
    }
} // namespace

namespace tcny
{
    // Once per frame: the CFG is read again about every half second and after a state load.
    void PollSettings()
    {
        static uint32_t frames;
        auto refresh = []
        {
            if (console::ps2settings::read(options))
            {
                ApplySettings();
            }
        };
        stateLoads.poll(refresh);
        if (++frames >= 30)
        {
            frames = 0;
            refresh();
        }
    }
} // namespace tcny

extern "C" void init()
{
    static constexpr console::ps2conflict::OldPlugin oldPlugins[] = {
        {"TrueCrimeNewYorkCity.PCSX2F.WidescreenFix.elf", "MAIN", "ForceAspectRatio"},
    };
    if (console::ps2conflict::Found(oldPlugins, OSDText))
        return;
    if (injector::InitializeCheckedRuntime(Rejected) != PCSX2_HOOK_OK)
    {
        return;
    }
    if (!console::ps2settings::load(options))
    {
        std::snprintf(OSDText[0], OSDStringSize, "CFG settings unavailable: update the plugin injector for CFG support.");
    }
    ApplySettings();
    const bool skipIntro = console::ps2settings::boolean(options[0], true);
    enable60 = std::strtol(options[6].value, nullptr, 10) != 30;
    unthrottle = console::ps2settings::boolean(options[7], true);
    // Fast-forward from boot until the first engine frame reports its loading state.
    FrameLimitUnthrottle = unthrottle;
    // The emulator reports 128 MB of RAM; before starting the next program the game then
    // takes the devkit TLB path, which a retail BIOS doesn't have (the jump to 0x00400397).
    // Both programs keep the retail path.
    const auto memorySize = sites::at_130528();
    if (!Required(memorySize, "memory size check"))
    {
        FrameLimitUnthrottle = 0;
        return;
    }
    injector::WriteMemory<uint32_t>(memorySize + 8, 0x3C020200); // LUI v0, 0x0200 (32 MB)
    // The launcher and the streamed TC2 engine are separate ELF generations.
    if (const auto intro = sites::at_100350())
    {
        if (skipIntro)
        {
            injector::MakeNOP(intro);
        }
        const auto sprite = sites::at_104C90(), picture = sites::at_111F08(), splash = sites::at_105948();
        if (Required(sprite, "start-up sprites") && Required(picture, "start-up video") && Required(splash, "copyright screen"))
        {
            InstallLauncher(sprite, picture, splash);
        }
        injector::FlushCaches();
        return;
    }
    const auto load = sites::at_1FD788(), processCall = sites::at_1FE0C4();
    const auto frameCall = sites::at_4A1C60(), intervalCall = sites::at_4A1CC0();
    const auto wide = sites::at_204EBC(), scale = sites::at_204DD8();
    const auto width = sites::at_2346E8(), layout = sites::at_23C638();
    const auto subtitle = sites::at_20BA48(), models = sites::at_255E88(), map = sites::at_265744();
    const auto menuBlips = sites::at_265224(), radarBlips = sites::at_267F74(), blips = sites::at_268094();
    const auto radar = sites::at_2517A8(), radarDisc = sites::at_2478FC(), radarClip = sites::at_2519D0();
    const auto barsBegin = sites::at_20B4C0(), barsHeight = sites::at_20B66C(), barsEnd = sites::at_20B780();
    const auto barsReset = sites::at_20B430(), cinemaStart = sites::at_1F1318();
    const auto cameraConstructor = sites::at_16B4D8(), projection = sites::at_4A1924(), manager = sites::at_204E1C();
    const uintptr_t required[] = {load,       processCall, frameCall,  intervalCall, wide,       scale,
                                  width,      layout,      subtitle,   models,       map,        menuBlips,
                                  radarBlips, blips,       radar,      radarDisc,    radarClip,  barsBegin,
                                  barsHeight, barsEnd,     barsReset,  cinemaStart,  cameraConstructor, projection, manager};
    const char* names[] = {"loading",    "game state",    "frame",       "interval",       "native widescreen",
                           "projection", "HUD width",     "HUD layout",  "subtitles",      "models",
                           "map",        "menu blips",    "radar blips", "blip origin",    "radar",
                           "radar disc", "radar clip",    "bars",        "bar height",     "bar drawing",
                           "bar reset",  "cinematic",     "game camera", "world camera", "2D manager"};
    for (unsigned i = 0; i < sizeof(required) / sizeof(*required); ++i)
    {
        // Without the per-frame loading check, boot fast-forward would never end.
        if (!Required(required[i], names[i]))
        {
            FrameLimitUnthrottle = 0;
            injector::FlushCaches();
            return;
        }
    }
    display.width = sites::at_204DB0() ? 512.0f : 640.0f;
    display.refresh();
    loading = reinterpret_cast<int*>(Absolute(load));
    mapCallers[0] = menuBlips + 8;
    mapCallers[1] = radarBlips + 8;
    // Cinematic start: sw v0, flag(s0) with s0 = the world; the bars reset reads the world pointer.
    cinemaOffset = injector::ReadMemory<uint16_t>(cinemaStart + 8);
    worldPointer = Absolute(barsReset + 0x18);
    // Game camera constructor: lui v1, hi ... addiu v1, v1, lo (its update function).
    gameCamera = (uintptr_t(injector::ReadMemory<uint16_t>(cameraConstructor + 0xC)) << 16) +
                 int16_t(injector::ReadMemory<uint16_t>(cameraConstructor + 0x18));
    hudManager = (uintptr_t(injector::ReadMemory<uint16_t>(manager)) << 16) +
                 int16_t(injector::ReadMemory<uint16_t>(manager + 0x14));
    const auto process = injector::GetBranchDestination(processCall).as_int();
    InstallTiming(process, frameCall + 16, intervalCall, wide);
    InstallHud(width, layout, subtitle, models, map, blips, radar, radarDisc, scale);
    InstallCutscene(barsBegin, barsHeight, barsEnd, barsReset);
    InstallCamera(projection);
    injector::WriteMemory<uint32_t>(wide, 0x0000282B);      // SLTU a1, zero, zero
    injector::WriteMemory<uint32_t>(radarClip, 0x00073A00); // SLL a3, a3, 8
    // A rejected session installs nothing, including the frame hook that ends boot fast-forward.
    if (injector::FlushCaches() != PCSX2_HOOK_OK)
    {
        FrameLimitUnthrottle = 0;
    }
}
extern "C" int main()
{
    return 0;
}
