#pragma once
namespace PadIcons {
using Button = console::padbuttons::Button;
struct Mapping { unsigned code; Button button; };
inline constexpr Mapping mapping[] {
 {0xa3,Button::Left}, {0xaf,Button::Right}, {0xa6,Button::Cross},
 {0xb7,Button::Circle}, {0xb4,Button::Square}, {0xac,Button::Triangle},
 {0xbd,Button::R1}, {0xb6,Button::L1}, {0xb1,Button::Up}, {0xb5,Button::Down},
 {0xa4,Button::Select}, {0xa2,Button::Start}
};
constexpr unsigned count=sizeof(mapping)/sizeof(mapping[0]);
struct Item {
 alignas(16) uint32_t glyph[8];
 alignas(16) unsigned char quad[20];
};
struct Texture {
 bool ready; unsigned key,w,h;
 alignas(16) unsigned char page[48];
 alignas(16) uint32_t pixels[32*32];
};
static_assert(sizeof(Item)==64 && __builtin_offsetof(Texture,page)==16 && __builtin_offsetof(Texture,pixels)==64 && sizeof(Texture)==4160);
inline Item items[count];
inline Texture textures[count*8];
inline int style{};
inline uintptr_t activeResource; inline unsigned originalCount;
inline uintptr_t* activePages;
inline SafetyMipsInline drawHook[2], lookupHook;
inline injector::hook_back<int(uintptr_t,uintptr_t)> draw[2];
inline injector::hook_back<int(uintptr_t,uintptr_t*,unsigned)> lookup;
inline uint32_t Pixel(const uint8_t* artwork,int x,int y) {
 // The shared Xbox Start art is a play triangle; the controller's Menu
 // button is the three-line symbol used for the PSP Start action.
 if (artwork == console::padbuttons::artwork[0][unsigned(Button::Start)]) {
  const int dx=x*2-63, dy=y*2-63, radius=dx*dx+dy*dy;
  const bool ring=radius>=44*44 && radius<=49*49;
  const bool lines=x>=20 && x<44 && ((y>=22 && y<26) || (y>=30 && y<34) || (y>=38 && y<42));
  return ring || lines ? 0xffffffffu : 0;
 }
 const unsigned index=artwork[y*64+x];
 const unsigned swizzled=(index & ~0x18u)|((index & 8)<<1)|((index & 16)>>1);
 uint32_t p; std::memcpy(&p,artwork+4096+swizzled*4,4);
 return (p&0xffffff)|(std::min(unsigned(p>>24)*255u/128u,255u)<<24);
}
inline Texture* Prepare(unsigned which,uintptr_t original) {
 const auto glyph=reinterpret_cast<const uint32_t*>(original);
 const auto q=reinterpret_cast<const int16_t*>(uintptr_t(glyph[4]));
 if (!q || glyph[5]-glyph[4]!=20) return nullptr;
 const int sourceWidth=q[1], sourceHeight=q[2]-q[3];
 if (sourceWidth<=0 || sourceHeight<=0) return nullptr;
 constexpr int w=32,h=32;
 const auto sourceFactors=reinterpret_cast<const float*>(q+6);
 const float displayWidth=sourceWidth*std::abs(sourceFactors[0]);
 const float displayHeight=sourceHeight*std::abs(sourceFactors[1]);
 if (displayWidth<=0.0f || displayHeight<=0.0f) return nullptr;
 // Fit the square controller artwork into the native glyph rectangle without
 // changing that rectangle's advance or stretching the artwork.
 const unsigned artWidth=unsigned(std::max(1L,std::lround(32.0f*std::min(displayHeight/displayWidth,1.0f))));
 const unsigned artHeight=unsigned(std::max(1L,std::lround(32.0f*std::min(displayWidth/displayHeight,1.0f))));
 const unsigned key=which+count*unsigned(style==3);
 Texture* texture=nullptr;
 for(auto& t:textures) if(t.ready && t.key==key && t.w==artWidth && t.h==artHeight) {texture=&t;break;}
 if(!texture) for(auto& t:textures) if(!t.ready) {texture=&t;break;}
 if(!texture) return nullptr;
 auto& item=items[which];
 std::memcpy(item.glyph,glyph,32);std::memcpy(item.quad,q,20);
 auto rgb=reinterpret_cast<uint16_t*>(item.glyph+1);rgb[0]=rgb[1]=rgb[2]=255;
 // The game's text advance and bounding rectangle use the quad dimensions.
 // Normalize the texture coordinates while preserving those dimensions.
 auto quad=reinterpret_cast<int16_t*>(item.quad);auto factors=reinterpret_cast<float*>(item.quad+12);
 quad[0]=0;quad[1]=int16_t(w);quad[2]=int16_t(h-32);quad[3]=-32;
 quad[5]=int16_t(int(quad[5])+sourceHeight-h);
 factors[0]*=float(sourceWidth)/float(w);factors[1]*=float(sourceHeight)/float(h);
 item.glyph[4]=uintptr_t(item.quad);item.glyph[5]=item.glyph[4]+20;item.glyph[6]=item.glyph[5];
 if(texture->ready) return texture;
 texture->key=key;texture->w=artWidth;texture->h=artHeight;
 const auto art=console::padbuttons::artwork[style==3 ? 1 : 0][unsigned(mapping[which].button)];
 const int left=(w-int(artWidth))/2, top=(h-int(artHeight))/2;
 for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
  const int ax=x-left, ay=y-top;
  texture->pixels[y*32+x]=ax>=0 && ax<int(artWidth) && ay>=0 && ay<int(artHeight)
   ? Pixel(art,ax*64/int(artWidth),(int(artHeight)-1-ay)*64/int(artHeight)) : 0;
 }
 *reinterpret_cast<uintptr_t*>(texture->page+12)=uintptr_t(texture->pixels);
 *reinterpret_cast<uint32_t*>(texture->page+16)=GU_PSM_8888;
 for(unsigned o:{20u,22u,24u}) *reinterpret_cast<uint16_t*>(texture->page+o)=32;
 texture->page[37]=8;texture->page[40]=texture->page[41]=1;
 *reinterpret_cast<uint16_t*>(texture->page+42)=0x10;
 sceKernelDcacheWritebackRange(texture,sizeof(*texture));texture->ready=true;return texture;
}
inline int Lookup(uintptr_t resource,uintptr_t* out,unsigned code) {
 int result=lookup.fun(resource,out,code);
 if(resource!=activeResource || !out[0] || out[1]) return result;
 for(unsigned i=0;i<count;++i) if(mapping[i].code==code) {
  if(auto texture=Prepare(i,out[0])) {
   auto& item=items[i];item.glyph[3]=(item.glyph[3]&~0x7fu)|(originalCount+i);
   activePages[originalCount+i]=uintptr_t(texture->page);out[0]=uintptr_t(item.glyph);
  }
  break;
 }
 return result;
}
inline int Draw(unsigned which,uintptr_t font,uintptr_t matrix) {
 const auto resource=*reinterpret_cast<uintptr_t*>(font+48);
 if(style==2 || !resource || activeResource) return draw[which].fun(font,matrix);
 auto begin=reinterpret_cast<uintptr_t**>(resource+24);
 auto end=reinterpret_cast<uintptr_t**>(resource+28);
 const auto oldBegin=*begin, oldEnd=*end;
 const unsigned n=unsigned(oldEnd-oldBegin);
 if(n+count>127) return draw[which].fun(font,matrix);
 uintptr_t pages[127]{};std::copy(oldBegin,oldEnd,pages);
 // The native draw batches by page. Extend its page list only for this call,
 // then restore the original vector before its owner can release it.
 *begin=pages;*end=pages+n+count;activeResource=resource;originalCount=n;activePages=pages;
 int result=draw[which].fun(font,matrix);
 activeResource=0;activePages=nullptr;*begin=oldBegin;*end=oldEnd;
 return result;
}
inline int Draw0(uintptr_t f,uintptr_t m){return Draw(0,f,m);}
inline int Draw1(uintptr_t f,uintptr_t m){return Draw(1,f,m);}
inline void Install() {
 const auto l=sites::Required("D0 FF BD 27 25 38 80 00 FF FF C4 30 25 30 E0 00 FF FF 87 30 20 00 B0 AF");
 lookupHook=safetymips::create_inline(l,Lookup);lookup.fun=lookupHook.original<decltype(lookup.fun)>();
 const auto a=sites::Required("20 FF BD 27 BC 00 B1 AF 25 88 80 00 60 00 2C C6 64 00 2D C6");
 const auto b=sites::Required("30 FF BD 27 AC 00 B1 AF 25 88 80 00 60 00 2C C6 64 00 2D C6");
 drawHook[0]=safetymips::create_inline(a,Draw0);draw[0].fun=drawHook[0].original<decltype(draw[0].fun)>();
 drawHook[1]=safetymips::create_inline(b,Draw1);draw[1].fun=drawHook[1].original<decltype(draw[1].fun)>();
}
}
