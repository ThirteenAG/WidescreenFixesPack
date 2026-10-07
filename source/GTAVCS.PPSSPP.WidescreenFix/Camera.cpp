#include "Game.hpp"
#include "../Shared/Console/Cutscene.hpp"
namespace vcsws {
namespace {
injector::hook_back<void(void*,int,float,float)> cameraSize;
constexpr console::CutsceneFrame cinematic{16.0f / 9.0f};
console::CutsceneBorderAnimation borders;
bool CutsceneActive() {
    return *reinterpret_cast<const uint8_t*>(Address<0x8BC7E30>() + 0x804) != 0;
}
void CameraSize(void* camera, int flags, float viewWindow, float aspect) {
    if (autoAspect) Drawing::settings.aspect=console::portable::Aspect();
    cameraSize.fun(camera,flags,viewWindow,Drawing::settings.aspect);
}
void SetFov(float degrees) {
    const bool cutscene = CutsceneActive();
    const float aspect = cutscene ? cinematic.fovAspect(Drawing::settings.aspect) : Drawing::settings.aspect;
    degrees = console::horizontal_fov(degrees, cinematic.aspect, aspect);
    const float factor = cutscene && settings.restoreCutsceneFov ? 1.0f : settings.fov;
    *reinterpret_cast<float*>(Address<0x8BADB10>()) = console::bounded(degrees * factor, 1.0f, 175.0f, 70.0f);
}
void DrawCutsceneBorders(void*) {
    const auto camera = Address<0x8BC7E30>();
    const auto now = reinterpret_cast<uint32_t (*)()>(Address<0x8905378>())();
    const bool dark = *reinterpret_cast<const float*>(camera + 2644) >= 255.0f;
    borders.tick(now, CutsceneActive(), settings.cutsceneBorders, dark);
    Drawing::Scope scope(Anchor::None);
    Drawing::TextureScope texture(false);
    const console::portable::StoryColor black{0,0,0,255};
    borders.draw(cinematic.window(Drawing::settings.aspect, 480, 272), 480, 272,
        [&](const console::Rect& rect) { Drawing::solidHook.call<void>(&rect, &black, true); });
}
SafetyMipsMid littleWillie;
float LodDistance(uintptr_t camera) {
    *reinterpret_cast<float*>(camera+0x7A0)=*reinterpret_cast<float*>(camera+0x7A8)*settings.lod;
    return settings.lod;
}
}
void InstallCamera() {
    cameraSize.fun=injector::MakeCALL(Address<0x8934C58>(),CameraSize).get();
    injector::MakeCALL(Address<0x8A23E98>(),SetFov);
    // Run the transition after pickup text every frame, including its exit.
    // Other callers (such as credits) retain the native border routine.
    injector::MakeNOP(Address<0x89362D4>());
    injector::MakeCALL(Address<0x89362DC>(), DrawCutsceneBorders);
    littleWillie=safetymips::create_mid(Address<0x8A29130>(),[](SafetyMipsContext& regs) {
        uintptr_t camera=regs.s1;
        auto* distance=reinterpret_cast<float*>(camera+1944);
        uintptr_t vehicle=*reinterpret_cast<uintptr_t*>(camera+1984);
        if (vehicle && *distance==1.0f && *reinterpret_cast<int16_t*>(vehicle+86)==0xAD) *distance=2.0f;
    });
    if (settings.lod>0) {
        injector::MakeCALL(Address<0x8A24130>(),LodDistance);
        injector::WriteInstr(Address<0x8A24134>(),mips_asm::move(mips_asm::a0,mips_asm::s0));
        injector::MakeInlineLUIORI(Address<0x8b45ac0>(),60.0f*settings.lod);
        injector::MakeInlineLUIORI(Address<0x89cb3a0>(),51.0f*settings.lod);
        injector::MakeInlineLUIORI(Address<0x89cb3a8>(),25.0f*settings.lod);
        injector::MakeInlineLUIORI(Address<0x89cb3b0>(),80.0f*settings.lod);
    }
    injector::MakeNOP(Address<0x8ad4ef4>()); // Garage removal must not discard the saved vehicle.
    injector::WriteInstr(Address<0x8ae8e30>(),0x06200004); // Script commands accept zero-valued indices.
    injector::WriteInstr(Address<0x8a12a94>(),0x06200004);
}
}
