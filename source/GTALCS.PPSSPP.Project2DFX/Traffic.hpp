#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/Traffic/GraphPopulation.hpp"
#include "../Shared/Console/Traffic/LCSPaths.hpp"
#include "../Shared/Console/Traffic/PortableFarTraffic.hpp"
namespace lcsfx {
// Distant road and water traffic for LCS PSP; see Traffic/FarTraffic.hpp.
class DistantTraffic {
    // Native Im3D vertex (RwIm3DTransform 0x8868560, GE vertex type 0x1DF):
    // float UV, 8888 colour, 16-bit normal, float position at +20.
    struct Vertex3D { float u,v;uint32_t color;int16_t nx,ny,nz,pad;console::Vector3 position; };
    static_assert(sizeof(Vertex3D)==32&&offsetof(Vertex3D,position)==20);
    console::traffic::GraphPopulation<console::traffic::LCSPaths> population_;
    console::traffic::PortableFarTraffic<Vertex3D> far_;
    console::portable::LightRenderer renderer_{};
    uintptr_t paths_{},camera_{};
    bool enabled_{};
#ifndef NDEBUG
    unsigned debugFrames_{};
#endif
    static unsigned Limit(const char* key,unsigned maximum) {
        int value=inireader.ReadInteger("DISTANTTRAFFIC",key,int(maximum));
        return unsigned(value<0?0:value>int(maximum)?maximum:value);
    }
public:
    void initialize(const console::portable::LightRenderer& renderer) {
        renderer_=renderer;
        population_.range=console::bounded(inireader.ReadFloat("DISTANTTRAFFIC","DrawDistance",800),450,2000,800);
        population_.carLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantCars",1)?Limit("MaxCars",40):0;
        population_.boatLimit=inireader.ReadInteger("DISTANTTRAFFIC","DistantBoats",1)?Limit("MaxBoats",8):0;
        enabled_=population_.carLimit||population_.boatLimit;
        paths_=Address<0x8B3980C>();camera_=Address<0x8B833A0>();
        // CTimer::m_FrameCounter follows m_snTimeInMilliseconds by 0x30.
#ifndef NDEBUG
        far_.noDepth=inireader.ReadInteger("DISTANTTRAFFIC","DebugNoDepth",0)!=0;
        far_.usableOverride=inireader.ReadFloat("DISTANTTRAFFIC","DebugUsable",0);
        far_.sizeScale=inireader.ReadFloat("DISTANTTRAFFIC","DebugScale",1);
#endif
        far_.initialize({Address<0x8B8ECD0>(),Address<0x8B5E144>()+0x30,Address<0x8868560>(),Address<0x886885C>(),Address<0x8868844>(),Address<0x8B5E244>()});
    }
    void render() {
        if(!enabled_)return;
        auto& r=renderer_;
        const uintptr_t paths=*reinterpret_cast<const uint32_t*>(paths_);
        if(paths<0x08800000||paths>=0x0A000000) {population_.clear();return;}
        // Resident graph: nodes +0, links +8, car/boat node count +24,
        // link count +34 (same layout as the PS2 build).
        console::traffic::LCSPaths view{
            *reinterpret_cast<const console::traffic::LCSNode* const*>(paths),
            *reinterpret_cast<const uint16_t* const*>(paths+8),
            *reinterpret_cast<const uint32_t*>(paths+24),*reinterpret_cast<const uint16_t*>(paths+34)};
        // No interior check: LCS keeps interiors enclosed, and depth testing
        // hides vehicles outside their walls.
        population_.update(view,r.position(),*reinterpret_cast<const uint32_t*>(r.timer),true);
        const auto forward=*reinterpret_cast<const console::Vector3*>(camera_+0x10);
        far_.draw(population_,r,forward,[](Vertex3D& to,const console::traffic::PositionColor& from) {
            to={0,0,from.color,0,0,0x7FFF,0,from.position};
        });
#ifndef NDEBUG
        if(++debugFrames_%300==0) {
            unsigned cars=0,boats=0;
            for(const auto& v:population_.vehicles)if(v.active)v.water?++boats:++cars;
            const auto c=r.position();
            logger.WriteF("Traffic: cars %u boats %u usable %d visible %u batches %u refused %u at %d %d %d\n",cars,boats,
                int(far_.lastUsable),far_.visible,far_.submitted,far_.refused,int(c.x),int(c.y),int(c.z));
            far_.visible=far_.submitted=far_.refused=0;
        }
#endif
    }
};
}
