#include "Controls.hpp"
#include <utility>
#include "../../external/injector/include/ps2/patches.hpp"
#include <cstring>
extern "C" {
#include "../../external/injector/include/ps2/inireader.h"
#include "../../external/injector/include/ps2/plugin_settings.h"
}

namespace vcs {
namespace {
safetymips::GameInline<void(Pad*, int16_t)> updatePad;
pcsx2::GameCallback<int16_t(Pad*)> rightX, rightY, flyingLeft, flyingRight;
Controller keyboardController{};
enum class Action {
    SteeringX, SteeringY, CarGunY, CarGunX, WalkX, WalkY,
    LookLeft, LookRight, LookBackCar, LookBackPed, Horn, HornDown,
    CarGun, CarGunDown, CarGunUp, Handbrake, Brake, Jump, Exit, ExitDown,
    Weapon, WeaponDown, Accelerate, Camera, RadioUp, RadioDown, WeaponLeft,
    WeaponRight, Target, TargetDown, Duck, Sprint, CameraBehind, ZoomIn, ZoomOut,
    ShiftLeft, ShiftRight, LookX, LookY, Skip, Oddjob, FreeAim, LeftX, LeftY
};
bool Edge(int16_t Controller::* button, const Pad* pad) {
    return pad->current.*button && !(pad->previous.*button);
}
int16_t PCQuery(Action action) {
    const auto& b = bindings;
    switch (action) {
    case vcs::Action::SteeringX: return b.axis(input, Act::SteerLeft, Act::SteerRight, 127);
    case vcs::Action::SteeringY: return b.axis(input, Act::LeanForward, Act::LeanBack, 127);
    case vcs::Action::CarGunX: return b.axis(input, Act::TurretLeft, Act::TurretRight, 127);
    case vcs::Action::CarGunY: return b.axis(input, Act::TurretUp, Act::TurretDown, 127);
    case vcs::Action::WalkX: case vcs::Action::LeftX: return b.axis(input, Act::Left, Act::Right, b.held(input, Act::Walk) ? 64 : 127);
    case vcs::Action::WalkY: case vcs::Action::LeftY: return b.axis(input, Act::Forward, Act::Backward, b.held(input, Act::Walk) ? 64 : 127);
    case vcs::Action::LookLeft: return b.held(input, Act::LookLeft);
    case vcs::Action::LookRight: return b.held(input, Act::LookRight);
    case vcs::Action::LookBackCar: return (b.held(input, Act::LookLeft) && b.held(input, Act::LookRight)) || b.held(input, Act::LookBehind);
    case vcs::Action::LookBackPed: return b.held(input, Act::LookBehind);
    case vcs::Action::Horn: return b.held(input, Act::Horn);
    case vcs::Action::HornDown: return b.pressed(input, Act::Horn);
    case vcs::Action::CarGun: return b.held(input, Act::VehicleFire);
    case vcs::Action::CarGunDown: return b.held(input, Act::VehicleFire) && !b.was(input, Act::VehicleFire);
    case vcs::Action::CarGunUp: return !b.held(input, Act::VehicleFire) && b.was(input, Act::VehicleFire);
    case vcs::Action::Handbrake: return b.held(input, Act::Handbrake) ? 255 : 0;
    case vcs::Action::Brake: return b.held(input, Act::Brake) ? 255 : 0;
    case vcs::Action::Jump: return b.pressed(input, Act::Jump);
    case vcs::Action::Exit: return b.held(input, Act::EnterVehicle) ? 255 : 0;
    case vcs::Action::ExitDown: return b.pressed(input, Act::EnterVehicle);
    case vcs::Action::Weapon: return b.held(input, Act::Attack) ? 255 : 0;
    case vcs::Action::WeaponDown: return b.pressed(input, Act::Attack);
    case vcs::Action::Accelerate: return b.held(input, Act::Accelerate) ? 255 : 0;
    case vcs::Action::Camera: return b.pressed(input, Act::Camera);
    case vcs::Action::RadioUp: return b.pressed(input, Act::NextRadio);
    case vcs::Action::RadioDown: return b.pressed(input, Act::PrevRadio);
    case vcs::Action::WeaponLeft: return b.pressed(input, Act::PrevWeapon);
    case vcs::Action::WeaponRight: return b.pressed(input, Act::NextWeapon);
    case vcs::Action::Target: return b.held(input, Act::Aim);
    case vcs::Action::TargetDown: return b.pressed(input, Act::Aim);
    case vcs::Action::Duck: return b.pressed(input, Act::Crouch);
    case vcs::Action::Sprint: return b.held(input, Act::Sprint);
    case vcs::Action::CameraBehind: return 0;
    case vcs::Action::ZoomIn: return b.held(input, Act::ZoomIn);
    case vcs::Action::ZoomOut: return b.held(input, Act::ZoomOut);
    case vcs::Action::ShiftLeft: return b.pressed(input, Act::PrevTarget);
    case vcs::Action::ShiftRight: return b.pressed(input, Act::NextTarget);
    // Mouse displacement is injected after the stick response, not rounded to
    // an integer here or passed through the pad's quadratic acceleration.
    case vcs::Action::LookX: case vcs::Action::LookY: return 0;
    case vcs::Action::Skip: return input.pressed(key::enter) || input.pressed(key::space);
    case vcs::Action::Oddjob: return b.pressed(input, Act::Mission);
    case vcs::Action::FreeAim: return 1; // Keep the previous PC mode's free camera active.
    }
    return 0;
}
bool ModernQuery(Action action, Pad* pad, int16_t& value) {
    auto& now = pad->current;
    switch (action) {
    case Action::Handbrake: value = now.cross; break;
    case Action::Brake: value = now.l2; break;
    case Action::Weapon: case Action::Accelerate: value = now.r2; break;
    case Action::WeaponDown: value = Edge(&Controller::r2, pad); break;
    case Action::RadioUp: case Action::WeaponRight: case Action::ShiftRight: value = Edge(&Controller::right, pad); break;
    case Action::RadioDown: case Action::WeaponLeft: case Action::ShiftLeft: value = Edge(&Controller::left, pad); break;
    case Action::Target: value = now.l2 != 0; break;
    case Action::TargetDown: value = Edge(&Controller::l2, pad); break;
    case Action::Duck: value = Edge(&Controller::down, pad); break;
    case Action::ZoomIn: value = now.square || now.leftY < -10; break;
    case Action::ZoomOut: value = now.cross || now.leftY > 10; break;
    case Action::LookX: value = now.rightX; break;
    case Action::LookY: value = *reinterpret_cast<int*>(0x487A58) ? -now.rightY : now.rightY; break;
    case Action::Oddjob: value = now.up != 0; break;
    case Action::FreeAim: value = now.l3 || now.rightX || now.rightY; break;
    case Action::LeftX: value = now.leftX; break;
    case Action::LeftY: value = now.leftY; break;
    default: return false;
    }
    return true;
}
bool WeaponAction(Action action) {
    return action == Action::Weapon || action == Action::WeaponDown || action == Action::WeaponLeft ||
        action == Action::WeaponRight || action == Action::Jump || action == Action::FreeAim || action == Action::Sprint;
}
template<uintptr_t Address, Action Which> struct Query {
    inline static safetymips::GameInline<int16_t(Pad*)> original;
    static int16_t Read(Pad* pad) {
        if (settings.pcControls) return !pad->disabled && (!WeaponAction(Which) || pad->mode < 4) ? PCQuery(Which) : 0;
        int16_t value;
        if (settings.modernControls && ModernQuery(Which, pad, value))
            return !pad->disabled && (!WeaponAction(Which) || pad->mode < 4) ? value : 0;
        return original.call(pad);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
template<uintptr_t Address, Action Which> void Bind() { Query<Address, Which>::Install(); }

// Preserve the game's mode-specific drive-by/RC rules; only change which
// shoulder buttons those rules see, including their matching old state.
template<uintptr_t Address, bool Back> struct ShoulderQuery {
    inline static safetymips::GameInline<int16_t(Pad*)> original;
    static int16_t Read(Pad* pad) {
        if (settings.pcControls) return pad->disabled ? 0 : PCQuery(Back ? Action::LookBackCar :
            (Address == 0x285940 ? Action::LookLeft : Action::LookRight));
        if (!settings.modernControls) return original.call(pad);
        std::swap(pad->current.l1, pad->current.l2); std::swap(pad->previous.l1, pad->previous.l2);
        std::swap(pad->current.r1, pad->current.r2); std::swap(pad->previous.r1, pad->previous.r2);
        int16_t result = original.call(pad);
        std::swap(pad->current.l1, pad->current.l2); std::swap(pad->previous.l1, pad->previous.l2);
        std::swap(pad->current.r1, pad->current.r2); std::swap(pad->previous.r1, pad->previous.r2);
        return result || (Back && !pad->disabled && pad->current.r3);
    }
    static void Install() { original = safetymips::create_inline_game(Address, Read); }
};
uintptr_t leftStickX = 0, leftStickY = 0;
bool Remapped() { return settings.modernControls || settings.pcControls; }
int16_t RightX(Pad* pad) {
    if (!Remapped()) return reinterpret_cast<int16_t (*)(Pad*)>(leftStickX)(pad);
    return pad->disabled || settings.pcControls ? 0 : pad->current.rightX;
}
int16_t RightY(Pad* pad) {
    if (!Remapped()) return reinterpret_cast<int16_t (*)(Pad*)>(leftStickY)(pad);
    return pad->disabled || settings.pcControls ? 0 : pad->current.rightY;
}
// Aircraft rudder: without the PC scheme the native code reads the
// (hooked) look-left/right queries at these call sites.
int16_t FlyingLeft(Pad* pad) {
    if (!settings.pcControls) return reinterpret_cast<int16_t (*)(Pad*)>(0x285940)(pad);
    return pad->disabled ? 0 : bindings.held(input, Act::LookLeft) || bindings.held(input, Act::TurretLeft);
}
int16_t FlyingRight(Pad* pad) {
    if (!settings.pcControls) return reinterpret_cast<int16_t (*)(Pad*)>(0x285AB0)(pad);
    return pad->disabled ? 0 : bindings.held(input, Act::LookRight) || bindings.held(input, Act::TurretRight);
}
pcsx2::Patch dpadPatches[4];
void Sample() {
    static_assert(sizeof(CMouseControllerState) == sizeof(console::Mouse));
    input.sample(KeyboardState[0], reinterpret_cast<console::Mouse&>(MouseState[0]));
    // The player and camera can both query mouse motion in one frame. Preserve
    // the old raw-input contract: each axis is applied once, by its first user.
    mouseMotion.sample(input.mouse.x, input.mouse.y);
}
void Update(Pad* pad, int16_t number) {
    if (number != 0 || !settings.pcControls) {
        updatePad.call(pad, number);
        if (number != 0) return;
        playerPad = pad;
        // The keyboard still drives a rebinding capture on the bindings page.
        Sample();
        if (capture.active()) { pad->previous = pad->current; pad->current = {}; }
        // The keyboard can always navigate the menus, so switching the PC
        // scheme off with it never strands a keyboard-only player.
        else if (MenuActive()) {
            auto& now = pad->current;
            if (input.held(key::up)) now.up = 1;
            if (input.held(key::down)) now.down = 1;
            if (input.held(key::left)) now.left = 1;
            if (input.held(key::right)) now.right = 1;
            if (input.held(key::enter)) { now.cross = 1; }
            if (input.held(key::back)) now.triangle = 1;
            if (input.held(key::escape)) now.start = 1;
        }
        return;
    }
    playerPad = pad;
    Sample();
    pad->previous = keyboardController;
    auto& now = keyboardController;
    now = {};
    const auto& b = bindings;
    // While a key is being captured for a binding, the menu sees no input.
    if (!capture.active()) {
        const bool menu = MenuActive();
        now.leftX = int16_t(b.axis(input, Act::Left, Act::Right, 127));
        now.leftY = int16_t(b.axis(input, Act::Forward, Act::Backward, 127));
        now.up = input.held(key::up) || b.held(input, Act::Forward);
        now.down = input.held(key::down) || b.held(input, Act::Backward);
        now.left = input.held(key::left) || b.held(input, Act::Left);
        now.right = input.held(key::right) || b.held(input, Act::Right);
        now.l1 = b.held(input, Act::Phone); now.l2 = input.held('1'); now.r1 = b.held(input, Act::Mission);
        now.r2 = b.held(input, Act::Aim);
        // Esc always leaves the menus, whatever Pause is bound to.
        now.start = b.held(input, Act::Pause) || (menu && input.held(key::escape));
        now.select = b.held(input, Act::Camera);
        // Face buttons that scripts and the GUI queries (shops, prompts) read
        // directly also follow the keys their text icons show (ButtonIcons.cpp):
        // cross = sprint/accelerate, square = jump/brake, triangle = enter/exit
        // vehicle, circle = fire. Enter and Backspace keep working everywhere.
        const bool vehicle = !menu && PlayerInVehicle();
        now.square = b.held(input, Act::Jump) || (vehicle && b.held(input, Act::Brake));
        now.triangle = input.held(key::back) || (!menu && b.held(input, Act::EnterVehicle));
        now.cross = input.held(key::enter) ||
            (!menu && !pad->disabled && b.held(input, vehicle ? Act::Accelerate : Act::Sprint));
        now.circle = b.held(input, Act::Attack) || (vehicle && b.held(input, Act::VehicleFire));
        now.l3 = b.held(input, Act::Horn); now.r3 = input.held(key::plus);
    }
    pad->current = now;
    // Match CPad::Update's history/idle contracts without requiring a physical
    // DualShock, its asynchronous setup phases or its disconnect screen.
    const uint32_t time = *reinterpret_cast<const uint32_t*>(MenuActive() ? 0x488280 : 0x4CD104);
    if (input.active()) pad->lastTouched = time;
    if (now.cross && !pad->previous.cross) pad->confirmTime = time;
    else if (!now.cross && pad->previous.cross) pad->confirmTime = -1;
    pad->hornIndex = (pad->hornIndex + 1) & 7;
    pad->hornHistory[pad->hornIndex] = !pad->disabled && now.l3;
    // The native driving query reads the two nine-sample delay buffers.
    auto buffer = reinterpret_cast<int16_t*>(pad->pad98);
    for (int i = 8; i >= 1; --i) { buffer[i] = buffer[i - 1]; buffer[10 + i] = buffer[9 + i]; }
    buffer[0] = now.leftX; buffer[10] = now.leftY;
    if (pad->frontendFrames) --pad->frontendFrames;
    *reinterpret_cast<uint32_t*>(0x4CD438) = *reinterpret_cast<const uint32_t*>(0x4CD338);
    *reinterpret_cast<uint32_t*>(0x4CD338) = 0;
    *reinterpret_cast<uint32_t*>(0x4CD33C) = 0;
    *reinterpret_cast<uint32_t*>(0x489F78) = 0;
}
uintptr_t CallTarget(uintptr_t site) {
    const uint32_t word = *reinterpret_cast<const uint32_t*>(site);
    return (word >> 26) == 3 ? ((site + 4) & 0xF0000000u) | ((word & 0x03FFFFFFu) << 2) : 0;
}
}
void ApplyPcControls() {
    for (auto& patch : dpadPatches) if (patch) (void)(Remapped() ? patch.enable() : patch.disable());
    ApplyMouseCamera();
    (void)injector::FlushCaches();
}
void InstallControls() {
    updatePad = safetymips::create_inline_game(0x2843D8, Update);
    // Every query is installed; each one reads the current scheme at runtime,
    // so the PC controls toggle in the menu applies without a restart.
    Bind<0x2852C0, Action::SteeringX>(); Bind<0x285460, Action::SteeringY>();
    Bind<0x285518, Action::CarGunY>(); Bind<0x285600, Action::CarGunX>();
    Bind<0x285690, Action::WalkX>(); Bind<0x2857E8, Action::WalkY>();
    ShoulderQuery<0x285940, false>::Install(); ShoulderQuery<0x285AB0, false>::Install();
    ShoulderQuery<0x285C20, true>::Install();
    Bind<0x285DD0, Action::LookBackPed>(); Bind<0x285E08, Action::Horn>(); Bind<0x285E80, Action::HornDown>();
    Bind<0x285F20, Action::CarGun>(); Bind<0x285F40, Action::CarGunDown>(); Bind<0x285F70, Action::CarGunUp>();
    Bind<0x285FA0, Action::Handbrake>(); Bind<0x286030, Action::Brake>(); Bind<0x286050, Action::Jump>();
    Bind<0x2860B0, Action::Exit>(); Bind<0x286100, Action::ExitDown>();
    Bind<0x286148, Action::Weapon>(); Bind<0x286180, Action::WeaponDown>();
    Bind<0x2861D0, Action::Accelerate>(); Bind<0x286220, Action::Camera>();
    Bind<0x286328, Action::RadioUp>(); Bind<0x2863E0, Action::RadioDown>();
    Bind<0x286408, Action::WeaponLeft>(); Bind<0x286458, Action::WeaponRight>();
    Bind<0x2864A8, Action::Target>(); Bind<0x286500, Action::TargetDown>(); Bind<0x286578, Action::Duck>();
    Bind<0x286580, Action::Sprint>(); Bind<0x286630, Action::CameraBehind>();
    Bind<0x286768, Action::ZoomIn>(); Bind<0x2867A8, Action::ZoomOut>();
    Bind<0x286808, Action::ShiftLeft>(); Bind<0x286850, Action::ShiftRight>();
    Bind<0x286AF0, Action::LookX>(); Bind<0x286B20, Action::LookY>();
    Bind<0x286DC8, Action::Skip>(); Bind<0x286DE8, Action::Oddjob>(); Bind<0x286E30, Action::FreeAim>();
    Bind<0x287640, Action::LeftX>(); Bind<0x287678, Action::LeftY>();
    // D-pad steering/walking is disabled for both remapped schemes.
    static constexpr uint32_t nop = 0;
    const uintptr_t dpadSites[4] = {0x285764, 0x285770, 0x2858BC, 0x2858C8};
    for (unsigned i = 0; i < 4; ++i) (void)dpadPatches[i].create(injector::detail::backend, dpadSites[i], &nop, 1);
    leftStickX = CallTarget(0x286938); leftStickY = CallTarget(0x286A40);
    if (leftStickX && leftStickY && CallTarget(0x286A6C) == leftStickY) {
        rightX.bind(RightX); rightY.bind(RightY);
        injector::MakeCALL(0x286938, rightX.address());
        injector::MakeCALL(0x286A40, rightY.address()); injector::MakeCALL(0x286A6C, rightY.address());
    }
    flyingLeft.bind(FlyingLeft); flyingRight.bind(FlyingRight);
    for (auto site : {0x3ECDC8, 0x3ECEA0, 0x2E71B0, 0x2E71F4}) injector::MakeCALL(site, flyingLeft.address());
    for (auto site : {0x3ECD8C, 0x3ECF08, 0x2E7170, 0x2E7234}) injector::MakeCALL(site, flyingRight.address());
    InstallMouseCamera();
    for (auto& patch : dpadPatches) if (patch && Remapped()) (void)patch.enable();
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
