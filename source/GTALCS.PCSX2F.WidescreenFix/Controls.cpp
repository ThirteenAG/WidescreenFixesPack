#include "Controls.hpp"
#include <cstring>
extern "C" {
#include "../../external/injector/include/ps2/inireader.h"
#include "../../external/injector/include/ps2/plugin_settings.h"
}

// Keyboard and mouse control scheme for GTA LCS PS2.
//
// CPad::Update still runs natively (pad phases, horn history, idle/disconnect
// state); afterwards the player's current state is replaced by a controller
// synthesized from the keyboard. Native queries then keep their own context
// rules (vehicle/camera/mission guards). Queries whose PS2 button has
// different meanings on a keyboard, and every analogue axis, are replaced.
// Mouse look is applied directly to the cameras (MouseCamera.cpp). Keys come
// from the rebindable table (StoriesBindings.hpp). Every hook is installed and
// checks the scheme at runtime, so the menu toggle applies immediately.
namespace lcs {
namespace {
safetymips::GameInline<uint64_t(Pad*, int16_t)> updatePad;
Controller keyboardController{};
unsigned zoomInFrames = 0, zoomOutFrames = 0;

enum class Query {
    SteeringX, SteeringY, CarGunY, CarGunX, WalkX, WalkY, LookBehindCar,
    Handbrake, Brake, Jump, Accelerate, RadioUp, RadioDown, WeaponLeft, WeaponRight,
    Target, TargetDown, Sprint, CameraBehind, ZoomIn, ZoomOut, Look, FreeAim, Oddjob
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
    case Query::Look: return 0;
    // Aiming with the right button is always the free (manual) aim.
    case Query::FreeAim: return 1;
    case Query::Oddjob: return b.held(input, Act::Mission);
    }
    return 0;
}
template<uintptr_t Address, Query Which> struct Hook {
    inline static safetymips::GameInline<int16_t(Pad*)> original;
    static int16_t Read(Pad* pad) {
        if (!settings.pcControls || pad != playerPad) return original.call(pad);
        if (pad->disabled && Which != Query::FreeAim) return 0;
        // Entering vehicles keeps the native blocking flags.
        return PCQuery(Which, pad);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
template<uintptr_t Address, Query Which> void Bind() { Hook<Address, Which>::Install(); }
// LookAroundLeftRight/UpDown read CPad::GetPad(0) themselves and take no pad.
template<uintptr_t Address> struct PadlessLook {
    inline static safetymips::GameInline<int16_t()> original;
    static int16_t Read() { return settings.pcControls ? 0 : original.call(); }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
int16_t Button(bool held) { return held ? 255 : 0; }

uint64_t Update(Pad* pad, int16_t number) {
    const auto result = updatePad.call(pad, number);
    if (number != 0) return result;
    // The pads update once per game frame, never inside the blocking loading
    // loop: loading-screen drawing enables unthrottling, gameplay ends it.
    FrameLimitUnthrottle = false;
    playerPad = pad;
    static_assert(sizeof(CMouseControllerState) == sizeof(console::Mouse));
    // Sampled for both schemes: the bindings page captures keys either way.
    input.sample(KeyboardState[0], reinterpret_cast<console::Mouse&>(MouseState[0]));
    mouseMotion.sample(input.mouse.x, input.mouse.y);
    TickMenu();
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
void InstallControls() {
    updatePad = safetymips::create_inline_game(0x16F8D8, Update);
    Bind<0x170C00, Query::SteeringX>(); Bind<0x170D18, Query::SteeringY>();
    Bind<0x170E10, Query::CarGunY>(); Bind<0x170EB0, Query::CarGunX>();
    Bind<0x170F50, Query::WalkX>(); Bind<0x171130, Query::WalkY>();
    Bind<0x171400, Query::LookBehindCar>();
    Bind<0x171768, Query::Handbrake>(); Bind<0x1717E0, Query::Brake>(); Bind<0x171870, Query::Jump>();
    Bind<0x171B38, Query::Accelerate>();
    Bind<0x171D28, Query::RadioUp>(); Bind<0x171DE8, Query::RadioDown>();
    Bind<0x171E40, Query::WeaponLeft>(); Bind<0x171EB0, Query::WeaponRight>();
    Bind<0x171F20, Query::Target>(); Bind<0x171F90, Query::TargetDown>();
    Bind<0x172020, Query::Sprint>(); Bind<0x172158, Query::CameraBehind>();
    Bind<0x172228, Query::ZoomIn>(); Bind<0x172298, Query::ZoomOut>();
    // SniperModeLookLeftRight/UpDown, LookAroundLeftRight/UpDown.
    Bind<0x172408, Query::Look>(); Bind<0x172498, Query::Look>();
    PadlessLook<0x172568>::Install(); PadlessLook<0x172588>::Install();
    Bind<0x172730, Query::Oddjob>(); Bind<0x1727A8, Query::FreeAim>();
    InstallMouseCamera();
    InstallButtonIcons();
}
void LoadBindings() {
    char value[64];
    for (unsigned i = 0; i < console::stories::ActionCount; ++i) {
        const auto& info = console::stories::actions[i];
        if (!(info.games & game)) continue;
        value[0] = 0;
        inireader.ReadString("BINDINGS", info.ini, "", value, sizeof(value));
        if (value[0]) bindings.parse(Act(i), value);
    }
    bindings.dirty = 0;
}
bool SaveBindings() {
    if (!bindings.dirty) return true;
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
            if (PCSX2F_IniRequest(&request) != PCSX2F_SETTINGS_OK) return false;
            request.count = 0;
        }
    }
    bindings.dirty = 0;
    return true;
}
}
