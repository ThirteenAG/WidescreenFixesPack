#include "Game.hpp"
#include <array>
namespace lcsws {
namespace {
template<uintptr_t Reference, Anchor Mode, class Signature=void()>
void Phase() { console::portable::StoryDrawPhase<Drawing,Reference,Mode,Signature>::Install(Address<Reference>()); }
std::array<SafetyMipsMid,6> boundaries;
void QueuedText(SafetyMipsContext& regs) {
    // The complete 60-byte font record still lives in a0 here. Correct it once
    // before the native renderer can flush; do not transform its glyphs again.
    auto* record=reinterpret_cast<float*>(uintptr_t(regs.a0));
    // The clock and the zone/vehicle name share one CHud section; text in the
    // lower half of a right-anchored section keeps to the bottom edge.
    Drawing::Scope scope(Drawing::anchor==Anchor::TopRight && record[2]>136.0f ? Anchor::BottomRight : Drawing::anchor);
    auto transform=Drawing::TransformFor(false,record[1],record[2]);
    record[1]=transform.x(record[1]); record[2]=transform.y(record[2]);
    record[3]*=transform.scaleX; record[4]*=transform.scaleY;
    record[6]*=transform.scaleX;
    record[7]*=transform.scaleY/transform.scaleX;
    record[8]=transform.x(record[8]); record[9]=transform.y(record[9]);
}
// Frontend artwork (menu backgrounds) draws a full-screen textured rectangle
// through the untextured vertex path; contain it and fill the sides.
SafetyMipsInline menuSprite;
void MenuSprite(void* texture, const void* color, float x, float y, float width, float height) {
    Drawing::Background(console::Rect{x, y + height, x + width, y});
    Drawing::TextureScope scope(true);
    menuSprite.call<void>(texture, color, x, y, width, height);
}
// CHud draws the weapon icon (and the centred aiming dots) with the
// translucent sprite renderer that coronas and particles also use. Correct it
// only while CHud runs; elsewhere it draws world-projected sprites.
SafetyMipsInline hudHook, spriteHook;
bool inHud = false;
int Hud() {
    Drawing::Scope scope(Anchor::Center);
    inHud = true;
    const int result = hudHook.call<int>();
    inHud = false;
    return result;
}
uint64_t TranslucentSprite(uint8_t red, uint8_t green, uint8_t blue, int16_t intensity, uint8_t alpha,
                           float x, float y, float z, float halfWidth, float halfHeight, float recipZ) {
    if (inHud) {
        const auto transform = Drawing::TransformFor(true, x, y);
        x = transform.x(x); y = transform.y(y);
        halfWidth *= transform.scaleX; halfHeight *= transform.scaleY;
    }
    return spriteHook.call<uint64_t>(red, green, blue, intensity, alpha, x, y, z, halfWidth, halfHeight, recipZ);
}
float RadarX() { return Drawing::settings.radarX; }
float RadarY() {
    return *reinterpret_cast<uint8_t*>(Address<0x8B56C7C>()) ? Drawing::settings.multiplayerRadarY : Drawing::settings.radarY;
}
}
void InstallDrawing() {
    Drawing::Install({Address<0x8AD4114>(),Address<0x8AD43D4>(),Address<0x8AD45B4>(),Address<0x8AD42A4>(),Address<0x8AD3C34>(),Address<0x8AD3D10>(),
        Address<0x8AD3ABC>(),Address<0x8AD3BE4>(),Address<0x8AD3B10>(),Address<0x8AD3DD0>(),Address<0x8E4C460>(),Address<0x8B56ACC>(),Address<0x8B56AD0>(),Address<0x8B56C7C>()});
    Drawing::anchor=Anchor::Center;
    Phase<0x89c2b60,Anchor::Center,void()>();
    hudHook=safetymips::create_inline(Address<0x8988940>(),Hud);
    spriteHook=safetymips::create_inline(Address<0x8A26024>(),TranslucentSprite);
    Phase<0x898e624,Anchor::Center,void()>();
    Phase<0x89870bc,Anchor::TopRight,int(int16_t)>();
    Phase<0x8987568,Anchor::TopRight,int(int16_t)>();
    Phase<0x8987B94,Anchor::TopRight,void()>(); // Money, drawn before the first section boundary
    Phase<0x8a5619c,Anchor::None,void()>();
    Phase<0x8A86330,Anchor::World,void()>(); // Projected pickup labels
    // Native CHud is one function on LCS; these are its section boundaries.
    boundaries[0]=safetymips::create_mid(Address<0x8989940>(),[](SafetyMipsContext&){Drawing::anchor=Anchor::TopRight;});
    boundaries[1]=safetymips::create_mid(Address<0x898ABDC>(),[](SafetyMipsContext&){Drawing::anchor=Anchor::TopRight;}); // Clock, zone/vehicle name
    boundaries[2]=safetymips::create_mid(Address<0x898B9A4>(),[](SafetyMipsContext&){Drawing::anchor=Anchor::TopRight;});
    boundaries[3]=safetymips::create_mid(Address<0x898C20C>(),[](SafetyMipsContext&){Drawing::anchor=Anchor::Radar;});
    boundaries[4]=safetymips::create_mid(Address<0x898C398>(),[](SafetyMipsContext&){Drawing::anchor=Anchor::Center;});
    boundaries[5]=safetymips::create_mid<&QueuedText>(Address<0x8A569A0>());
    menuSprite=safetymips::create_inline(Address<0x8AD39E0>(),MenuSprite);
    injector::MakeJMP(Address<0x8986FE4>(),RadarX);
    injector::MakeJMP(Address<0x8987008>(),RadarY);
}
}
