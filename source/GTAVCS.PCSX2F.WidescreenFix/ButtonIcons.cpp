#include "Controls.hpp"
#include "ButtonIcons.hpp"
#include "../Shared/Console/StoriesButtonText.hpp"
#include <cstring>
namespace vcs {
namespace {
namespace art = console::pcbuttons;
struct Raster { uint32_t unused[2]; const uint8_t* data; uint32_t flags; };
struct Texture { Raster* raster; uint8_t remaining[84]; };
constexpr unsigned artworkCount = unsigned(art::Key::Count);
static_assert(sizeof(art::artwork) / sizeof(art::artwork[0]) == artworkCount);
// The generated textures are already in the native PSMT8 + CSM1 CLUT layout
// and stay plugin-owned. The renderer borrows them only during a draw; no
// game dictionary or destructor ever acquires ownership.
Raster rasters[artworkCount]{};
Texture textures[artworkCount]{};
pcsx2::GameFunction<void(Sprite*,const Rect*,const Color*,float,float,float,float,float,float,float,float)> draw;
pcsx2::GameFunction<void()> flush;
pcsx2::GameFunction<uint64_t(int,uintptr_t)> state;
struct Glyph { int16_t left,width,right,height; uint16_t character,x,y; };
safetymips::GameInline<void(void*,const Glyph*,const Rect*,const Color*)> drawGlyph;
struct String { char *begin,*end; uint32_t unused; char* capacity; };
struct MenuImage {
    String name;
    int x,y,width,height;
    uint8_t fields[48];
    Color normal,focused;
    uint8_t remaining[132];
    Texture* texture;
    String dictionary,textureName;
};
static_assert(offsetof(MenuImage,normal)==80 && offsetof(MenuImage,textureName)==240);
safetymips::GameInline<void(MenuImage*,int)> drawImage,drawFocusedImage;
pcsx2::GameFunction<void(MenuImage*,int)> drawImageLabel,drawFocusedImageLabel;
using namespace console::stories;
bool ImageKey(const String& name,uint8_t& key) {
    static constexpr struct { const char* name; uint8_t key; int action; } mappings[]={
        {"btn_cross_PSP",code::enter,-1},{"btn_triangle_PSP",code::back,-1},
        {"btn_up_PSP",code::up,-1},{"btn_down_PSP",code::down,-1},
        {"btn_left_PSP",code::left,-1},{"btn_right_PSP",code::right,-1},
        {"btn_square_PSP",0,Act::Jump},{"btn_L_PSP",0,Act::Phone},{"btn_R_PSP",0,Act::Mission}
    };
    if(!name.begin || !name.end || name.end<name.begin)return false;
    const auto length=size_t(name.end-name.begin);
    for(const auto& mapping:mappings)
        if(length==std::strlen(mapping.name) && !std::memcmp(name.begin,mapping.name,length)) {
            key=mapping.action<0?mapping.key:bindings.primary(Act(mapping.action));
            return art::Find(key)!=nullptr;
        }
    return false;
}
// Button tokens of the help text (GXT C0* entries): X cross, T triangle,
// O circle, S square. Outside the menus a token names the action the button
// has on foot or in a vehicle; while a script holds the player (shops, save
// and replay prompts) cross is the GUI select, which Enter always triggers.
// Controls.cpp sets the raw buttons from the same keys, so scripts and GUI
// queries reading them accept the key shown.
bool GlyphKey(unsigned character,uint8_t& key) {
    const bool menu=MenuActive();
    const bool vehicle=!menu && PlayerInVehicle();
    const bool held=!menu && playerPad && playerPad->disabled;
    switch(character) {
    case 'X': key=menu||held?code::enter:bindings.primary(vehicle?Act::Accelerate:Act::Sprint);break;
    case 'T': key=menu?code::back:bindings.primary(Act::EnterVehicle);break;
    case 'O': key=bindings.primary(vehicle?Act::VehicleFire:Act::Attack);break;
    case 'S': key=bindings.primary(vehicle?Act::Brake:Act::Jump);break;
    case 'J': key=bindings.primary(Act::Aim);break;
    case 'K': key=bindings.primary(Act::Phone);break;
    case 'U': key=menu?code::up:bindings.primary(Act::Forward);break;
    case 'D': key=menu?code::down:bindings.primary(Act::Backward);break;
    case '<': key=menu?code::left:bindings.primary(Act::Left);break;
    case '>': key=menu?code::right:bindings.primary(Act::Right);break;
    default:return false;
    }
    return art::Find(key)!=nullptr;
}
// Key artwork inside text: the token ~<iconCharacter + artwork index>~ is an
// extended glyph that cFontSystem::GetGlyphInfoExtended resolves to a
// plugin-owned glyph sized from the font's cross button, so measuring,
// wrapping and drawing all go through the native extended-glyph path.
constexpr uint16_t iconCharacter=0xE100;
Glyph keyGlyphs[artworkCount]{};
safetymips::GameInline<const Glyph*(void*,int)> glyphExtended;
const Glyph* GlyphExtended(void* font,int character) {
    const unsigned index=unsigned(uint16_t(character))-iconCharacter;
    if(index>=artworkCount)return glyphExtended.call(font,character);
    const Glyph* cross=glyphExtended.call(font,'X');
    if(!cross)return nullptr;
    auto& glyph=keyGlyphs[index];
    glyph=*cross;
    glyph.width=int16_t(float(cross->width)*float(art::artwork[index].contentWidth)/float(art::height)+0.5f);
    glyph.character=uint16_t(character);
    return &glyph;
}
// Plain-text button names of the help text, for the PC scheme. CMessages::
// GetTokenPadKeyString looks up "C<pad mode><token>" for ~k~ ~TOKEN~; the
// entries that hold a ~X~-style glyph stay native (GlyphKey draws them).
constexpr TextRewrite tokens[]={
    {"AMMOV","{Forward} {Left} {Backward} {Right}"},{"AMLEF","{Left}"},{"AMRIG","{Right}"},
    {"ANS","{Phone}"},{"CVEIW","{Camera}"},{"DOSLR","{Left} {Right}"},{"DOSUD","{Forward} {Backward}"},
    {"FIREH","{TurretLeft} {TurretRight} {TurretUp} {TurretDown}"},{"FLDN","{Backward}"},{"FLUP","{Forward}"},
    {"FREE1","{Aim}"},{"FREE2","{mouse}"},{"HEPI","{Left} {Right}"},{"HERO","{LookLeft} {LookRight}"},
    {"PDBAK","{LookBehind}"},{"PDCTL","{PrevTarget} {NextTarget}"},{"PDCWE","{PrevWeapon} {NextWeapon}"},
    {"PDLO1","{mouse}"},{"PDLO2","{mouse}"},{"PDLOO","{mouse}"},{"PDLT","{Aim}"},{"PUCF","{Phone}"},
    {"PUTE","{Aim}"},{"SNSLO","{Phone}"},{"SWIMD","{Forward} {Left} {Backward} {Right}"},
    {"TAXJU","{Horn}"},{"TGSUB","{Mission}"},{"VECRS","{PrevRadio} {NextRadio}"},{"VEHB","{Handbrake}"},
    {"VEHN","{Horn}"},{"VELB","{LookBehind}"},{"VELL","{LookLeft}"},{"VELR","{LookRight}"},
    {"VESTR","{SteerLeft} {SteerRight}"},{"VETD","{mouse}"},{"VETU","{mouse}"},{"VEWE2","{Handbrake}"},
    {"VEWEA","{TurretLeft} {TurretRight}"},{"VEWEI","{LeanForward} {LeanBack}"},
    {"VEWEL","{TurretLeft} {TurretRight}"},{"VEWEU","{TurretUp} {TurretDown}"},
};
// The other plain-text button names in the GXT belong to debug menus
// (DBG*, EMHELP*, LS2_*, WTC_INS) that retail builds never show.
// Callers may keep the pointer (brief messages), so every entry owns its text.
char16_t tokenText[sizeof(tokens)/sizeof(*tokens)][48];
struct TextWriter {
    char16_t* out; unsigned used,limit;
    void put(char16_t c) { if(used+1<limit)out[used++]=c; }
    void key(uint8_t code) {
        if(const auto* image=art::Find(code)) {
            if(used+4<limit) { put(u'~');put(char16_t(iconCharacter+unsigned(image-art::artwork)));put(u'~'); }
            return;
        }
        char name[24];KeyText(code,name);
        for(const char* c=name;*c;++c)put(char16_t(uint8_t(*c)));
    }
};
template<size_t Size> const uint16_t* Format(const TextRewrite& rewrite,char16_t (&buffer)[Size]) {
    TextWriter writer{buffer,0,Size};
    FormatText(rewrite.text,bindings,writer);
    buffer[writer.used]=0;
    return reinterpret_cast<const uint16_t*>(buffer);
}
void Icon(const art::Artwork& image,const Rect& rect,const Color& color) {
    Sprite sprite{&textures[unsigned(&image-art::artwork)]};
    TextureScope texture(true);
    const float u=float(image.contentWidth)/float(image.width);
    draw(&sprite,&rect,&color,0,0,u,0,0,1,u,1);
}
template<bool focused> void DrawImage(MenuImage* item,int context) {
    uint8_t key=0;
    if(settings.pcControls && MenuActive() && ImageKey(item->textureName,key)) {
        if constexpr(focused)drawFocusedImageLabel(item,context);
        else drawImageLabel(item,context);
        flush();
        const auto& image=*art::Find(key);
        const float height=float(item->height),width=height*float(image.contentWidth)/float(art::height);
        // Keep the native hint's left edge and baseline; wider keys use the
        // spacing already reserved between the icon and its text.
        Rect rect{float(item->x),float(item->y)+height,float(item->x)+width,float(item->y)};
        Color white{255,255,255,focused?item->focused.alpha:item->normal.alpha};
        Icon(image,rect,white);
        state(1,item->texture && item->texture->raster?reinterpret_cast<uintptr_t>(item->texture->raster):0);
        return;
    }
    if constexpr(focused)drawFocusedImage.call(item,context);
    else drawImage.call(item,context);
}
void DrawGlyph(void* font,const Glyph* glyph,const Rect* rect,const Color* color) {
    // Standard glyphs can have the same character as an extended button token.
    // Only the font's distinct extended table is eligible for replacement.
    auto table=*reinterpret_cast<const Glyph**>(static_cast<char*>(font)+12);
    auto count=*reinterpret_cast<const uint16_t*>(static_cast<char*>(font)+2);
    const uintptr_t at=reinterpret_cast<uintptr_t>(glyph),first=reinterpret_cast<uintptr_t>(table);
    const uintptr_t keys=reinterpret_cast<uintptr_t>(keyGlyphs);
    if(glyph && at>=keys && at<keys+sizeof(keyGlyphs)) {
        flush();
        const auto fontSprite=*reinterpret_cast<Sprite**>(0x489F70);
        uintptr_t raster=fontSprite && fontSprite->texture ? *reinterpret_cast<uint32_t*>(fontSprite->texture):0;
        Color white{255,255,255,color->alpha};
        Icon(art::artwork[(at-keys)/sizeof(Glyph)],*rect,white);
        state(1,raster);
        return;
    }
    uint8_t key=0;
    if(settings.pcControls && table && glyph && at>=first && at-first<size_t(count)*sizeof(Glyph) && GlyphKey(glyph->character,key)) {
        flush();
        const auto fontSprite=*reinterpret_cast<Sprite**>(0x489F70);
        uintptr_t raster=fontSprite && fontSprite->texture ? *reinterpret_cast<uint32_t*>(fontSprite->texture):0;
        Color white{255,255,255,color->alpha};
        // The glyph's advance is fixed by the font, so keys fill its cell.
        Icon(*art::Find(key),*rect,white);
        state(1,raster);
        return;
    }
    drawGlyph.call(font,glyph,rect,color);
}
}
const uint16_t* ButtonText(const char* key) {
    if(!settings.pcControls || !key)return nullptr;
    if(key[0]=='C' && key[1]>='0' && key[1]<='9')
        if(const auto* rewrite=FindRewrite(tokens,key+2))return Format(*rewrite,tokenText[rewrite-tokens]);
    return nullptr;
}
float KeyIconWidth(uint8_t key,float height) {
    const auto* image=art::Find(key);
    return image?height*float(image->contentWidth)/float(art::height):0.0f;
}
float DrawKeyIcon(uint8_t key,float x,float y,float height,const Color& color) {
    const auto* image=art::Find(key);
    if(!image)return 0.0f;
    const float width=KeyIconWidth(key,height);
    Rect rect{x,y+height,x+width,y};
    Icon(*image,rect,color);
    return width;
}
void InstallButtonIcons() {
    for(unsigned i=0;i<artworkCount;++i) {
        const auto& image=art::artwork[i];
        rasters[i]={{0,0},image.data,art::Log2(image.width)|(art::log2Height<<6)|(8u<<12)|(1u<<20)};
        textures[i].raster=&rasters[i];
    }
    draw.bind(0x3DFF80);flush.bind(0x3DFDD8);state.bind(0x175198);
    drawGlyph=safetymips::create_inline_game(0x3B8EA0,DrawGlyph);
    glyphExtended=safetymips::create_inline_game(0x3E5E58,GlyphExtended);
    drawImageLabel.bind(0x410608);drawFocusedImageLabel.bind(0x410648);
    drawImage=safetymips::create_inline_game(0x1AAEA0,DrawImage<false>);
    drawFocusedImage=safetymips::create_inline_game(0x1AAF50,DrawImage<true>);
}
}
