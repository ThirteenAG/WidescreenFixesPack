#include "Game.hpp"
#include <array>
namespace vcsws {
namespace {
// Frontend images: fill the sides of contained full-screen artwork before the
// image binds its raster.
SafetyMipsInline imageHook, focusedImageHook;
void ImageBackground(const void* item) {
    const auto* rect = reinterpret_cast<const int*>(static_cast<const char*>(item) + 0xC);
    Drawing::Background(console::Rect{float(rect[0]), float(rect[1] + rect[3]), float(rect[0] + rect[2]), float(rect[1])});
}
void DrawImage(void* item) { ImageBackground(item); imageHook.call<void>(item); }
void DrawFocusedImage(void* item) { ImageBackground(item); focusedImageHook.call<void>(item); }
template<uintptr_t Reference, Anchor Mode, class Signature=void()>
void Phase() { console::portable::StoryDrawPhase<Drawing,Reference,Mode,Signature>::Install(Address<Reference>()); }

}
void InstallDrawing() {
    Drawing::Install({Address<0x8AF47C0>(),Address<0x8AF4B70>(),Address<0x8AF4EE4>(),Address<0x8AF49B4>(),Address<0x8AF4D64>(),Address<0x8AF4E54>(),
        Address<0x8AF4650>(),Address<0x8AF4770>(),Address<0x8AF46A4>(),Address<0x8AF5130>(),Address<0x8BDDFB0>(),Address<0x8BAFB38>(),Address<0x8BAFB3C>(),Address<0x8BB01B8>(),Address<0x8974960>(),
        Address<0x8861824>(),Address<0x8AA8C8C>(),Address<0x8AF51FC>()});
    Drawing::anchor=Anchor::None;
    imageHook=safetymips::create_inline(Address<0x8904384>(),DrawImage);
    focusedImageHook=safetymips::create_inline(Address<0x890441C>(),DrawFocusedImage);
    Phase<0x893570C,Anchor::Center,void(const char*,const char*,const char*,bool)>();
    Phase<0x8935AA4,Anchor::Center,void()>();
    Phase<0x89372C0,Anchor::Center,void()>();
    Phase<0x8936250,Anchor::Center,void()>();
    Phase<0x89ba6ec,Anchor::Center,void(void*)>();
    Phase<0x89bdf2c,Anchor::Center,void(void*)>();
    Phase<0x89bb3e0,Anchor::TopRight,void(void*)>();
    Phase<0x89bb5cc,Anchor::TopRight,void(void*)>();
    Phase<0x89bb9ec,Anchor::TopRight,void(void*)>();
    Phase<0x89bcf8c,Anchor::TopRight,void(void*)>();
    Phase<0x89bd108,Anchor::TopRight,void(void*)>();
    Phase<0x89bd59c,Anchor::TopRight,void(void*)>();
    Phase<0x89bda30,Anchor::TopRight,void(void*)>();
    Phase<0x89bdd20,Anchor::TopRight,void(void*)>();
    Phase<0x89bf4f0,Anchor::TopRight,void(void*)>();
    Phase<0x89bff80,Anchor::TopRight,void(void*)>();
    Phase<0x89c4858,Anchor::TopRight,void(void*)>();
    Phase<0x89c4f8c,Anchor::TopRight,void(void*)>();
    Phase<0x89c4b68,Anchor::BottomRight,void(void*)>();
    Phase<0x89c57b4,Anchor::BottomRight,void(void*)>();
    Phase<0x89bd32c,Anchor::Radar,void(void*)>();
    Phase<0x89C0468,Anchor::Center,void(void*)>();
    Phase<0x8810484,Anchor::World,int(void*,float)>();
    Phase<0x88F6E68,Anchor::World,void()>();
    Phase<0x8A18294,Anchor::World,void()>();
    Phase<0x89c5cfc,Anchor::None,void(void*)>();
    
    injector::MakeInlineLUIORI(Address<0x89baa8c>(),Drawing::settings.radarX);
    injector::MakeInlineLUIORI(Address<0x89baac0>(),Drawing::settings.radarY);
    injector::MakeInlineLUIORI(Address<0x89baab4>(),Drawing::settings.multiplayerRadarY);
}
}
