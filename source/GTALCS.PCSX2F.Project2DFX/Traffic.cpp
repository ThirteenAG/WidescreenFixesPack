#include "Game.hpp"
#include "../Shared/Console/Traffic/GraphPopulation.hpp"
#include "../Shared/Console/Traffic/FarTraffic.hpp"
#include "../Shared/Console/Traffic/LCSPaths.hpp"
#include "../Shared/Console/Traffic/RenderBudget.hpp"
extern "C" {
#include "../../external/injector/include/ps2/inireader.h"
}
namespace lcsfx {
namespace {
// Native Im3D vertex: 32-byte stride (Im3DRenderIndexedPrimitive 0x2B1E60
// reads 32*index; 0x2B1D88 copies position, u/v at +12/+16, RGBA at +20).
struct Vertex3D { Vector position;float u,v;uint32_t color;uint32_t unused[2]; };
static_assert(sizeof(Vertex3D)==32&&offsetof(Vertex3D,color)==20);
console::traffic::GraphPopulation<console::traffic::LCSPaths> population;
console::traffic::FarRenderer<Vertex3D> meshes;
pcsx2::GameFunction<uint64_t(Vertex3D*,int,uintptr_t)> transform;
pcsx2::GameFunction<uint64_t(int,const short*,int)> indexed;
pcsx2::GameFunction<uint64_t()> end;
pcsx2::GameFunction<uint64_t(int)> texturing;
pcsx2::GameFunction<uint64_t(int,uintptr_t)> state;
pcsx2::GameFunction<int(Vector*,Vector*,float*,float*,bool)> project;
pcsx2::GameFunction<int(void*)> usedBytes;
pcsx2::GameFunction<void()> flushSprites;
constexpr uintptr_t renderBuffer=0x43EA70;
constexpr int64_t renderBufferSize=943728;
// LCS has no free-space checks in Im3D: it moves the DMA top pointer and the
// bottom stream cursor unconditionally. CSprite::FlushSpriteBuffer and
// CParticle drop work once 0x257FF bytes or fewer remain. Keep that floor
// plus the same 128 KiB margin VCS keeps above its own native threshold.
constexpr int64_t reserve=0x25800+128*1024;
#ifdef LCSFX_DIAGNOSTICS
// Debug-only counters, read over PINE by build/console-rewrite/lcs-p2dfx-probe.py.
struct Diagnostics { uint32_t frames,admitted,refused;int32_t lowestFree;uint32_t visible;float screen[4][3];uint32_t noDepth; } diagnostics{0,0,0,0x7FFFFFFF,0,{},0};
#endif
unsigned Limit(const char* key,unsigned maximum) {
    int value=inireader.ReadInteger("DISTANTTRAFFIC",key,int(maximum));
    return unsigned(value<0?0:value>int(maximum)?maximum:value);
}
}
void RenderTraffic() {
    uintptr_t paths=read<uint32_t>(0x3D0314);
    if(!paths) {population.clear();return;}
    // Resident graph from GAME.DTZ: nodes +0, connections +8, car/boat node
    // count +24 (CRoadBlocks::Init 0x3506F0), connection count +34.
    console::traffic::LCSPaths view{
        reinterpret_cast<const console::traffic::LCSNode*>(read<uint32_t>(paths)),
        reinterpret_cast<const uint16_t*>(read<uint32_t>(paths+8)),
        read<uint32_t>(paths+24),read<uint16_t>(paths+34)};
    population.update(view,cameraPosition(),read<uint32_t>(0x3D9B70),read<uint32_t>(0x3D8430)==0);
#ifdef LCSFX_DIAGNOSTICS
    ++diagnostics.frames;diagnostics.visible=0;
#endif
    unsigned remaining=256*1024;
    bool begun=false;
    // Low-polygon meshes for every visible vehicle, batched; see
    // Traffic/FarTraffic.hpp. PS2 projection accepts the whole traffic range.
    meshes.render(population,cameraPosition(),console::nightIntensity(hour(),minute()),4000.0f,[](Vector p) {
        Vector screen;float w,h;
        bool visible=project(&p,&screen,&w,&h,false)&&screen.z>0&&screen.x>-30&&screen.x<670&&screen.y>-30&&screen.y<478;
#ifdef LCSFX_DIAGNOSTICS
        if(visible&&diagnostics.visible<4) {
            auto& out=diagnostics.screen[diagnostics.visible++];out[0]=screen.x;out[1]=screen.y;out[2]=screen.z;
        }
#endif
        return visible;
    },[&](unsigned indexCount) {
        // A triangle list costs five 16-byte vertices in three DMA streams per
        // triangle. Admit complete batches only.
        int64_t available=renderBufferSize-int64_t(usedBytes(reinterpret_cast<void*>(renderBuffer)));
        unsigned bytes=console::traffic::PS2MeshBytes(indexCount);
        if(bytes>remaining||available<int64_t(bytes)+reserve) {
#ifdef LCSFX_DIAGNOSTICS
            ++diagnostics.refused;
#endif
            return false;
        }
        remaining-=bytes;
#ifdef LCSFX_DIAGNOSTICS
        ++diagnostics.admitted;
        if(available<diagnostics.lowestFree)diagnostics.lowestFree=int32_t(available);
#endif
        if(!begun) {
            begun=true;
            flushSprites();
            state(1,0);state(4,1);state(6,0);state(8,5);state(9,6);
#ifdef LCSFX_DIAGNOSTICS
            if(diagnostics.noDepth)state(4,0);   // see occluded meshes in tests
#endif
            texturing(0);
        }
        return true;
    },[](Vertex3D& to,const console::traffic::PositionColor& from) {
        to={from.position,0,0,from.color,{0,0}};
    },[](Vertex3D* vertices,unsigned count,const short* indices,unsigned indexCount) {
        if(transform(vertices,int(count),0)) {
            for(unsigned first=0;first<indexCount;first+=480) {
                unsigned size=indexCount-first;if(size>480)size=480;
                indexed(3,indices+first,int(size));
            }
            end();
        }
        return true;
    },[](unsigned){return false;});
    if(!begun)return;
    texturing(1);
    // Native CCoronas::Render (0x258008) postconditions at this call site.
    state(1,0);state(4,1);state(6,1);state(8,2);state(9,2);
}
void InstallTraffic() {
    population.range=console::bounded(inireader.ReadFloat("DISTANTTRAFFIC","DrawDistance",800),450,2000,800);
    population.carLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantCars",1)?Limit("MaxCars",40):0;
    population.boatLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantBoats",1)?Limit("MaxBoats",8):0;
    transform.bind(0x2B1B40);indexed.bind(0x2B1E60);end.bind(0x2B1D68);texturing.bind(0x200788);
    state.bind(0x144B58);project.bind(0x2DFD70);
    usedBytes.bind(0x1D0318);flushSprites.bind(0x2DFF08);
}
}
