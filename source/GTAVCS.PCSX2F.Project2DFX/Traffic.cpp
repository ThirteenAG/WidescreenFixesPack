#include "Game.hpp"
#include "../Shared/Console/Traffic/FarTraffic.hpp"
#include "../Shared/Console/Traffic/RenderBudget.hpp"
extern "C" {
#include "../../external/injector/include/ps2/inireader.h"
}
namespace vcsfx {
namespace {
struct Vertex3D { Vector position;float u,v;uint32_t color; };
static_assert(sizeof(Vertex3D)==24&&offsetof(Vertex3D,color)==20);
console::traffic::Population population;
console::traffic::FarRenderer<Vertex3D> meshes;
pcsx2::GameFunction<uint64_t(Vertex3D*,int,uintptr_t)> transform;
pcsx2::GameFunction<uint64_t(int,const short*,int)> indexed;
pcsx2::GameFunction<void()> end;
pcsx2::GameFunction<void(int)> begin;
pcsx2::GameFunction<uint64_t(int,uintptr_t)> state;
pcsx2::GameFunction<int(Vector*,Vector*,float*,float*,bool)> project;
pcsx2::GameFunction<int64_t(void*)> freeBytes;
pcsx2::GameFunction<void()> flush;
unsigned Limit(const char* key,unsigned maximum) {
    int value=inireader.ReadInteger("DISTANTTRAFFIC",key,int(maximum));
    return unsigned(value<0?0:value>int(maximum)?maximum:value);
}
}
void RenderTraffic() {
    uintptr_t paths=read<uint32_t>(0x487490);
    if(!paths) {population.clear();return;}
    console::traffic::Paths view{
        reinterpret_cast<const console::traffic::Node*>(read<uint32_t>(paths)),
        reinterpret_cast<const uint16_t*>(read<uint32_t>(paths+31648)),
        read<uint32_t>(paths+16),read<uint16_t>(paths+26)};
    population.update(view,cameraPosition(),read<uint32_t>(0x4CD104),read<uint32_t>(0x489F7C)==0);
    flush();
    unsigned remaining=256*1024;
    state(1,0);state(4,1);state(6,0);state(8,5);state(9,6);
    begin(0);
    // Low-polygon meshes for every visible vehicle, batched; see
    // Traffic/FarTraffic.hpp. PS2 projection accepts the whole traffic range.
    meshes.render(population,cameraPosition(),console::nightIntensity(hour(),minute()),4000.0f,[](Vector p) {
        Vector screen;float w,h;
        return project(&p,&screen,&w,&h,false)&&screen.z>0&&screen.x>-30&&screen.x<670&&screen.y>-30&&screen.y<478;
    },[&remaining](unsigned indexCount) {
        // Native triangle lists expand each triangle to five 16-byte vertices
        // in three DMA streams. The allocator returns null when its fixed
        // frame arena is full, but the caller still writes through that pointer.
        // Admit complete batches only, reserving space for native coronas/UI.
        return console::traffic::AdmitPS2Mesh(indexCount,freeBytes(reinterpret_cast<void*>(0x520F90)),remaining);
    },[](Vertex3D& to,const console::traffic::PositionColor& from) {
        to={from.position,0,0,from.color};
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
    begin(1);
    // Native CCoronas::Render's postconditions at this exact hook site.
    state(1,0);state(4,1);state(6,1);state(8,5);state(9,6);
}
void InstallTraffic() {
    population.range=console::bounded(inireader.ReadFloat("DISTANTTRAFFIC","DrawDistance",800),450,2000,800);
    population.carLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantCars",1)?Limit("MaxCars",40):0;
    population.boatLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantBoats",1)?Limit("MaxBoats",8):0;
    transform.bind(0x2FBFA8);indexed.bind(0x2FC2E8);end.bind(0x2FC1F0);begin.bind(0x23E720);
    state.bind(0x175198);project.bind(0x39FC48);
    freeBytes.bind(0x478628);flush.bind(0x3DFDD8);
}
}
