#include "Controls.hpp"
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <utility>
#include "Settings.hpp"
extern "C" {
#include "ps2/plugin_settings.h"
}

// Keyboard and mouse control scheme and the modern pad scheme for GTA LCS PS2.
//
// CPad::Update still runs natively (pad phases, horn history, idle/disconnect
// state); afterwards the player's current state is replaced by a controller
// synthesized from the keyboard. Native queries then keep their own context
// rules (vehicle/camera/mission guards). Queries whose PS2 button has
// different meanings on a keyboard, and every analogue axis, are replaced.
// Mouse look is applied directly to the cameras (MouseCamera.cpp). Keys come
// from the rebindable table (StoriesBindings.hpp). Every hook is installed and
// checks the scheme at runtime, so the menu toggle applies immediately.
//
// The modern pad scheme (CONTROLS.ModernControlScheme) drives, aims and shoots
// with the triggers: R2 accelerates and fires, L2 brakes and targets, Cross is
// the handbrake, the d-pad switches weapons, targets and radio stations, and
// the right stick aims. L1 and R1 take over looking left, right and behind.
extern "C" console::MouseMotion MouseStateV2;
extern "C" console::MouseTransitions MouseStateV3;

namespace lcs {
namespace {
safetymips::GameInline<uint64_t(Pad*, int16_t)> updatePad;
Controller keyboardController{};
Controller previousPhysicalController{};
constexpr uintptr_t directionalSwap = 0x3D08F8;
unsigned zoomInFrames = 0, zoomOutFrames = 0;
struct SettingsRefresh
{
    uint32_t appliedGeneration = 0;
    uint32_t attemptedGeneration = 0;
    unsigned retryDelay = 0;

    void Poll()
    {
        const auto generation = *reinterpret_cast<const volatile uint32_t*>(&PCSX2FStateLoadGeneration);
        if (generation == appliedGeneration)
            return;
        if (generation == attemptedGeneration && retryDelay)
        {
            --retryDelay;
            return;
        }

        attemptedGeneration = generation;
        if (RefreshSettings())
        {
            appliedGeneration = generation;
            retryDelay = 0;
        }
        else
        {
            // Retry on the 60th subsequent pad update without waiting for another load.
            retryDelay = 59;
        }
    }
};
SettingsRefresh stateLoads;

enum class Query {
    SteeringX, SteeringY, CarGunY, CarGunX, WalkX, WalkY, LookBehindCar,
    Handbrake, Brake, Jump, Accelerate, RadioUp, RadioDown, WeaponLeft, WeaponRight,
    Target, TargetDown, Sprint, CameraBehind, ZoomIn, ZoomOut, LookX, LookY, FreeAim, Oddjob,
    // Native with the PC scheme (it sets the buttons they read).
    Weapon, WeaponDown, ShiftLeft, ShiftRight
};
bool Moving() {
    const auto& b = bindings;
    return b.held(input, Act::Forward) || b.held(input, Act::Backward) || b.held(input, Act::Left) || b.held(input, Act::Right);
}
int16_t PCQuery(Query query, Pad* pad) {
    const auto& b = bindings;
    switch (query) {
    case Query::SteeringX: {
        // GetSteeringLeftRight feeds the drunk-cheat delay buffer itself.
        pad->steeringBuffer[0] = int16_t(b.axis(input, Act::SteerLeft, Act::SteerRight, 127));
        const auto index = unsigned(pad->drunkIndex) < 10 ? pad->drunkIndex : 0;
        return pad->steeringBuffer[index];
    }
    case Query::SteeringY: return int16_t(b.axis(input, Act::LeanForward, Act::LeanBack, 127));
    case Query::CarGunX: return int16_t(b.axis(input, Act::TurretLeft, Act::TurretRight, 127));
    case Query::CarGunY: return int16_t(b.axis(input, Act::TurretUp, Act::TurretDown, 127));
    case Query::WalkX: return int16_t(b.axis(input, Act::Left, Act::Right, b.held(input, Act::Walk) ? 64 : 127));
    case Query::WalkY: return int16_t(b.axis(input, Act::Forward, Act::Backward, b.held(input, Act::Walk) ? 64 : 127));
    case Query::LookBehindCar: return (b.held(input, Act::LookLeft) && b.held(input, Act::LookRight)) || b.held(input, Act::LookBehind);
    case Query::Handbrake: return b.held(input, Act::Handbrake) ? 255 : 0;
    case Query::Brake: return b.held(input, Act::Brake) ? 255 : 0;
    case Query::Jump: return b.pressed(input, Act::Jump);
    case Query::Accelerate: return b.held(input, Act::Accelerate) ? 255 : 0;
    case Query::RadioUp: return b.pressed(input, Act::NextRadio);
    case Query::RadioDown: return b.pressed(input, Act::PrevRadio);
    case Query::WeaponLeft: return b.pressed(input, Act::PrevWeapon);
    case Query::WeaponRight: return b.pressed(input, Act::NextWeapon);
    case Query::Target: return b.held(input, Act::Aim);
    case Query::TargetDown: return b.pressed(input, Act::Aim);
    // The native query also requires stick movement; keep that contract.
    case Query::Sprint: return b.held(input, Act::Sprint) && Moving();
    // Mouse look owns the camera; recentring would fight it every frame.
    case Query::CameraBehind: return 0;
    case Query::ZoomIn: return zoomInFrames != 0 || b.held(input, Act::ZoomIn);
    case Query::ZoomOut: return zoomOutFrames != 0 || b.held(input, Act::ZoomOut);
    // Mouse displacement is injected after the stick response (MouseCamera.cpp).
    case Query::LookX: case Query::LookY: return 0;
    // Aiming with the right button is always the free (manual) aim.
    case Query::FreeAim: return 1;
    case Query::Oddjob: return b.held(input, Act::Mission);
    default: return 0;
    }
}
bool Edge(int16_t Controller::* button, const Pad* pad) {
    return pad->current.*button && !(pad->previous.*button);
}
// MAIN.GamepadCameraSpeed and MAIN.InvertGamepadY on a right stick value.
int16_t CameraStick(int value, bool vertical) {
    if (vertical && settings.invertPad) value = -value;
    const float scaled = float(value) * settings.padCameraSpeed;
    return int16_t(scaled > 32767.0f ? 32767.0f : scaled < -32767.0f ? -32767.0f : scaled);
}
int16_t RightStick(int16_t value) { return value > 10 || value < -10 ? value : 0; }
// The game's own Invert Look setting.
bool GameInvertLook() { return *reinterpret_cast<const int32_t*>(0x3D08F4) != 0; }
bool ModernQuery(Query query, Pad* pad, int16_t& value) {
    const auto& now = pad->current;
    switch (query) {
    case Query::Accelerate: case Query::Weapon: value = now.r2; break;
    case Query::WeaponDown: value = Edge(&Controller::r2, pad); break;
    case Query::Brake: value = now.l2; break;
    case Query::Handbrake: value = now.cross; break;
    case Query::Target: value = now.l2 != 0; break;
    case Query::TargetDown: value = Edge(&Controller::l2, pad); break;
    case Query::RadioUp: case Query::WeaponRight: case Query::ShiftRight: value = Edge(&Controller::right, pad); break;
    case Query::RadioDown: case Query::WeaponLeft: case Query::ShiftLeft: value = Edge(&Controller::left, pad); break;
    case Query::ZoomIn: value = now.square || now.leftY < -10; break;
    case Query::ZoomOut: value = now.cross || now.leftY > 10; break;
    // Aiming (sniper, free aim) with the right stick.
    case Query::LookX: value = CameraStick(RightStick(now.rightX), false); break;
    case Query::LookY: value = CameraStick(GameInvertLook() ? -RightStick(now.rightY) : RightStick(now.rightY), true); break;
    case Query::FreeAim: value = now.l3 || RightStick(now.rightX) || RightStick(now.rightY); break;
    default: return false;
    }
    return true;
}
bool WeaponQuery(Query query) {
    return query == Query::Weapon || query == Query::WeaponDown || query == Query::WeaponLeft ||
        query == Query::WeaponRight || query == Query::Jump || query == Query::FreeAim || query == Query::Sprint;
}
template<uintptr_t Address, Query Which> struct Hook {
    inline static safetymips::GameInline<int16_t(Pad*)> original;
    static int16_t Read(Pad* pad) {
        if (pad != playerPad) return original.call(pad);
        if (KeyboardActive()) {
            if (Which >= Query::Weapon) return original.call(pad);
            if (pad->disabled && Which != Query::FreeAim) return 0;
            // Entering vehicles keeps the native blocking flags.
            return PCQuery(Which, pad);
        }
        if (settings.modernControls)
        {
            if constexpr (Which == Query::WalkX || Which == Query::WalkY)
            {
                // Hide the D-pad only for movement queries. The game's
                // directional swap selects the alternate button bank.
                const bool swapped = *reinterpret_cast<const int32_t*>(directionalSwap) != 0;
                const auto offset = swapped ? offsetof(Controller, alternateDpad) : offsetof(Controller, up);
                auto* directions = reinterpret_cast<unsigned char*>(&pad->current) + offset;
                unsigned char saved[sizeof(Controller::alternateDpad)];
                std::memcpy(saved, directions, sizeof(saved));
                std::memset(directions, 0, sizeof(saved));
                const auto result = original.call(pad);
                std::memcpy(directions, saved, sizeof(saved));
                return result;
            }
            int16_t value;
            if (ModernQuery(Which, pad, value))
                return !pad->disabled && (!WeaponQuery(Which) || pad->mode < 4) ? value : 0;
        }
        return original.call(pad);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
// LookLeft/LookRight/LookBehindForCar: the modern scheme keeps the game's
// camera-mode rules and moves them from L2/R2 to L1/R1 (and R3 looks behind).
template<uintptr_t Address, bool Behind> struct ShoulderQuery {
    inline static safetymips::GameInline<int16_t(Pad*)> original;
    static void Swap(Pad* pad) {
        std::swap(pad->current.l1, pad->current.l2); std::swap(pad->previous.l1, pad->previous.l2);
        std::swap(pad->current.r1, pad->current.r2); std::swap(pad->previous.r1, pad->previous.r2);
    }
    static int16_t Read(Pad* pad) {
        if (pad != playerPad) return original.call(pad);
        if (KeyboardActive()) return Behind ? (pad->disabled ? 0 : PCQuery(Query::LookBehindCar, pad)) : original.call(pad);
        if (!settings.modernControls) return original.call(pad);
        Swap(pad);
        const int16_t result = original.call(pad);
        Swap(pad);
        return result || (Behind && !pad->disabled && pad->current.r3);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
template<uintptr_t Address, Query Which> void Bind() { Hook<Address, Which>::Install(); }
// LookAroundLeftRight/UpDown read CPad::GetPad(0) themselves and take no pad.
// The pad camera: GamepadCameraSpeed and InvertGamepadY with either pad scheme.
template<uintptr_t Address, bool Vertical> struct PadlessLook {
    inline static safetymips::GameInline<int16_t()> original;
    static int16_t Read() {
        if (KeyboardActive()) return 0;
        int16_t value = original.call();
        if (settings.modernControls) value = RightStick(value);
        return CameraStick(value, Vertical);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
int16_t Button(bool held) { return held ? 255 : 0; }
bool PadTouched(const Controller& current, const Controller& previous, bool swapped)
{
    static int16_t stickBaseline[4]{};
    const int16_t axes[] = {current.leftX, current.leftY, current.rightX, current.rightY};
    bool touched = false;
    for (unsigned i = 0; i < 4; ++i)
    {
        constexpr int deadzone = 40;
        constexpr int minimumChange = 8;
        const int value = axes[i];
        auto& baseline = stickBaseline[i];
        if (std::abs(value) <= deadzone)
            baseline = axes[i];
        else if (std::abs(int(baseline)) <= deadzone || std::abs(value - baseline) >= minimumChange)
        {
            baseline = axes[i];
            touched = true;
        }
    }
    if (touched)
        return true;

    const int16_t directions[] = {current.up, current.down, current.left, current.right};
    const int16_t oldDirections[] = {previous.up, previous.down, previous.left, previous.right};
    const auto* active = swapped ? current.alternateDpad : directions;
    const auto* oldActive = swapped ? previous.alternateDpad : oldDirections;
    for (unsigned i = 0; i < 4; ++i)
    {
        if (active[i] && !oldActive[i])
            return true;
    }

    static constexpr int16_t Controller::* buttons[] = {
        &Controller::l1, &Controller::l2, &Controller::r1, &Controller::r2,
        &Controller::start, &Controller::select, &Controller::square, &Controller::triangle,
        &Controller::cross, &Controller::circle, &Controller::l3, &Controller::r3
    };
    for (const auto button : buttons)
    {
        if (current.*button && !(previous.*button))
            return true;
    }
    return false;
}

uint64_t Update(Pad* pad, int16_t number) {
    const auto result = updatePad.call(pad, number);
    if (number != 0) return result;
    // The pads update once per game frame, never inside the blocking loading
    // loop: loading-screen drawing enables unthrottling, gameplay ends it.
    FrameLimitUnthrottle = false;
    stateLoads.Poll();
    playerPad = pad;
    static_assert(sizeof(CMouseControllerState) == sizeof(console::Mouse));
    // Sampled for both schemes: the bindings page captures keys either way.
    input.sample(KeyboardState[0], reinterpret_cast<console::Mouse&>(MouseState[0]), MouseStateV2, MouseStateV3);
    mouseMotion.sample(input.mouse.x, input.mouse.y);
    const bool swapped = *reinterpret_cast<const int32_t*>(directionalSwap) != 0;
    const bool padTouched = PadTouched(pad->current, previousPhysicalController, swapped);
    previousPhysicalController = pad->current;
    TickMenu();
    UpdateCheats();
    if (!settings.pcControls) {
        // A key capture on the bindings page owns the input.
        if (capture.active()) pad->current = {};
        // The keyboard can always navigate the menus, so switching the PC
        // scheme off with it never strands a keyboard-only player.
        else if (MenuActive()) {
            using namespace key;
            auto& now = pad->current;
            if (input.held(up)) now.up = 255;
            if (input.held(down)) now.down = 255;
            if (input.held(left)) now.left = 255;
            if (input.held(right)) now.right = 255;
            if (input.held(enter)) now.cross = 255;
            if (input.held(back)) now.triangle = 255;
            if (input.held(escape)) now.start = 255;
        }
        return result;
    }
    // The native update read the pad into the current state.
    if (input.touched()) keyboardInUse = true;
    else if (padTouched) keyboardInUse = false;
    if (!keyboardInUse && !capture.active()) {
        keyboardController = pad->current;
        return result;
    }
    const auto& b = bindings;
    if (b.pressed(input, Act::ZoomIn)) zoomInFrames = 6, zoomOutFrames = 0;
    else if (b.pressed(input, Act::ZoomOut)) zoomOutFrames = 6, zoomInFrames = 0;
    else { if (zoomInFrames) --zoomInFrames; if (zoomOutFrames) --zoomOutFrames; }
    // The native update copied last frame's (keyboard) state to previous.
    pad->previous = keyboardController;
    auto& now = keyboardController;
    now = {};
    if (!capture.active()) {
        using namespace key;
        const bool menu = MenuActive();
        // Menus and unreplaced queries read the synthesized LCS mode 0 layout.
        now.leftX = int16_t(b.axis(input, Act::Left, Act::Right, 127));
        now.leftY = int16_t(b.axis(input, Act::Forward, Act::Backward, 127));
        now.up = Button(input.held(up)); now.down = Button(input.held(down));
        now.left = Button(input.held(left)); now.right = Button(input.held(right));
        now.l1 = Button(b.held(input, Act::Phone));                                    // phone / collect
        now.l2 = Button(b.held(input, Act::PrevTarget) || b.held(input, Act::LookLeft)); // look left, previous target
        now.r1 = Button(b.held(input, Act::Aim));                                      // target
        now.r2 = Button(b.held(input, Act::NextTarget) || b.held(input, Act::LookRight)); // look right, next target
        // Esc always leaves the menus, whatever Pause is bound to.
        now.start = Button(b.held(input, Act::Pause) || (menu && input.held(escape)));
        now.select = Button(b.held(input, Act::Camera));                               // camera mode
        // Native Select checks both pad samples. A new tap can follow a pulse
        // on the very next frame, so use the binding edge as well as the level.
        if (b.pressed(input, Act::Camera)) pad->previous.select = 0;
        now.square = Button(b.held(input, Act::Jump));                                 // jump
        now.triangle = Button(b.held(input, Act::EnterVehicle) || (menu && input.held(back))); // vehicle, menu back
        now.cross = Button(input.held(enter));                                         // menu select, skip
        now.circle = Button(b.held(input, Act::Attack) || b.held(input, Act::VehicleFire)); // fire
        now.l3 = Button(b.held(input, Act::Horn));                                     // horn
        now.r3 = Button(b.held(input, Act::LookBehind));                               // look behind
    }
    pad->current = now;
    if (input.active()) pad->lastTouched = *reinterpret_cast<const uint32_t*>(address::timeInMilliseconds);
    return result;
}
}
const char* ModernTextKey(const char* key) {
    if (!settings.modernControls || KeyboardActive() || !key || key[0] != 'C' || key[1] < '0' || key[1] > '3') return nullptr;
    // Help texts name the button of an action (C<pad config><action>); these
    // actions moved. The replacement is the game's own text for that button.
    static constexpr struct { const char* action; const char* text; } moved[] = {
        {"VEACC", "C0VELR"}, {"PDFW", "C0VELR"},   // R2
        {"VEBRK", "C0VELL"}, {"PDLT", "C0VELL"},   // L2
        {"VEHB", "C0PDSPR"},                       // Cross
        {"VELL", "C0PUCF"}, {"VELR", "C0PDLT"},    // L1, R1
    };
    for (const auto& entry : moved) if (!std::strcmp(key + 2, entry.action)) return entry.text;
    return nullptr;
}
void InstallControls() {
    updatePad = safetymips::create_inline_game(0x16F8D8, Update);
    Bind<0x170C00, Query::SteeringX>(); Bind<0x170D18, Query::SteeringY>();
    Bind<0x170E10, Query::CarGunY>(); Bind<0x170EB0, Query::CarGunX>();
    Bind<0x170F50, Query::WalkX>(); Bind<0x171130, Query::WalkY>();
    ShoulderQuery<0x171310, false>::Install(); ShoulderQuery<0x171388, false>::Install();
    ShoulderQuery<0x171400, true>::Install();
    Bind<0x171768, Query::Handbrake>(); Bind<0x1717E0, Query::Brake>(); Bind<0x171870, Query::Jump>();
    Bind<0x171A28, Query::Weapon>(); Bind<0x171A98, Query::WeaponDown>();
    Bind<0x171B38, Query::Accelerate>();
    Bind<0x171D28, Query::RadioUp>(); Bind<0x171DE8, Query::RadioDown>();
    Bind<0x171E40, Query::WeaponLeft>(); Bind<0x171EB0, Query::WeaponRight>();
    Bind<0x171F20, Query::Target>(); Bind<0x171F90, Query::TargetDown>();
    Bind<0x172020, Query::Sprint>(); Bind<0x172158, Query::CameraBehind>();
    Bind<0x172228, Query::ZoomIn>(); Bind<0x172298, Query::ZoomOut>();
    Bind<0x172308, Query::ShiftLeft>(); Bind<0x172338, Query::ShiftRight>();
    // SniperModeLookLeftRight/UpDown, LookAroundLeftRight/UpDown.
    Bind<0x172408, Query::LookX>(); Bind<0x172498, Query::LookY>();
    PadlessLook<0x172568, false>::Install(); PadlessLook<0x172588, true>::Install();
    Bind<0x172730, Query::Oddjob>(); Bind<0x1727A8, Query::FreeAim>();
    InstallMouseCamera();
    InstallButtonIcons();
}
namespace {
constexpr console::ps2settings::Value bindingDefaults[] = {
    {"BINDINGS", "Forward", "W"},
    {"BINDINGS", "Backward", "S"},
    {"BINDINGS", "Left", "A"},
    {"BINDINGS", "Right", "D"},
    {"BINDINGS", "Sprint", "SHIFT"},
    {"BINDINGS", "Walk", "ALT"},
    {"BINDINGS", "Jump", "SPACE"},
    {"BINDINGS", "Attack", "LMB, NUM0"},
    {"BINDINGS", "Aim", "RMB"},
    {"BINDINGS", "NextTarget", "E"},
    {"BINDINGS", "PrevTarget", "Q"},
    {"BINDINGS", "EnterVehicle", "F"},
    {"BINDINGS", "NextWeapon", "WHEELDOWN"},
    {"BINDINGS", "PrevWeapon", "WHEELUP"},
    {"BINDINGS", "LookBehind", "C, MMB"},
    {"BINDINGS", "Camera", "V"},
    {"BINDINGS", "ZoomIn", "WHEELUP, PAGEUP"},
    {"BINDINGS", "ZoomOut", "WHEELDOWN, PAGEDOWN"},
    {"BINDINGS", "Phone", "TAB"},
    {"BINDINGS", "Mission", "2, NUMPLUS"},
    {"BINDINGS", "Accelerate", "W"},
    {"BINDINGS", "Brake", "S"},
    {"BINDINGS", "SteerLeft", "A"},
    {"BINDINGS", "SteerRight", "D"},
    {"BINDINGS", "Handbrake", "SPACE"},
    {"BINDINGS", "Horn", "H"},
    {"BINDINGS", "VehicleFire", "LMB, NUM0"},
    {"BINDINGS", "NextRadio", "WHEELUP"},
    {"BINDINGS", "PrevRadio", "WHEELDOWN"},
    {"BINDINGS", "LookLeft", "Q"},
    {"BINDINGS", "LookRight", "E"},
    {"BINDINGS", "LeanForward", "SHIFT, NUM8"},
    {"BINDINGS", "LeanBack", "CTRL, NUM5"},
    {"BINDINGS", "TurretLeft", "NUM4"},
    {"BINDINGS", "TurretRight", "NUM6"},
    {"BINDINGS", "TurretUp", "NUM8"},
    {"BINDINGS", "TurretDown", "NUM5"},
    {"BINDINGS", "Pause", "ESC, BACKTICK"},
};
console::ps2settings::Value bindingValues[sizeof(bindingDefaults) / sizeof(*bindingDefaults)];
void ParseBindings() {
    for (const auto& value : bindingValues) {
        for (unsigned i = 0; i < console::stories::ActionCount; ++i) {
            const auto& info = console::stories::actions[i];
            if ((info.games & game) && !std::strcmp(info.ini, value.key)) bindings.parse(Act(i), value.value);
        }
    }
    bindings.dirty = 0;
}
}
void LoadBindings() {
    std::memcpy(bindingValues, bindingDefaults, sizeof(bindingValues));
    (void)console::ps2settings::load(bindingValues);
    ParseBindings();
}
bool ReloadBindings()
{
    std::memcpy(bindingValues, bindingDefaults, sizeof(bindingValues));
    if (!console::ps2settings::read(bindingValues))
        return false;
    ParseBindings();
    return true;
}
bool SaveBindings() {
    if (!bindings.dirty) return true;
    if (PCSX2FSettingsVersion != 2) return false;
    // The host only accepts requests inside the module image (not a game stack).
    static PCSX2FIniRequest request;
    request = {};
    request.size = sizeof(request); request.version = 1; request.operation = PCSX2F_SETTINGS_WRITE;
    for (unsigned i = 0; i < console::stories::ActionCount; ++i) {
        const auto& info = console::stories::actions[i];
        if (((bindings.dirty >> i) & 1) && (info.games & game)) {
            auto& entry = request.entries[request.count++];
            std::strcpy(entry.section, "BINDINGS"); std::strcpy(entry.key, info.ini);
            bindings.format(Act(i), entry.value);
        }
        if (request.count && (request.count == PCSX2F_SETTINGS_MAX_ENTRIES || i + 1 == console::stories::ActionCount)) {
            if (console::ps2settings::request(request) != PCSX2F_SETTINGS_OK) return false;
            request.count = 0;
        }
    }
    bindings.dirty = 0;
    return true;
}
}
