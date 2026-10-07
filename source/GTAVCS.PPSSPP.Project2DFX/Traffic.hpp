#pragma once
#include "Addresses.hpp"
#include "../Shared/Console/Traffic/PortableFarTraffic.hpp"
namespace vcsfx {
// Distant road and water traffic: low-polygon meshes, pulled in along the
// camera ray past the game's far clip (see Traffic/FarTraffic.hpp).
class DistantTraffic {
    struct Vertex3D { float u,v;uint32_t color;console::Vector3 position; };
    static_assert(sizeof(Vertex3D)==24&&offsetof(Vertex3D,position)==12);
    console::traffic::Population population_;
    console::traffic::PortableFarTraffic<Vertex3D> far_;
    console::portable::LightRenderer renderer_{};
    uintptr_t paths_{},area_{},camera_{};
    bool enabled_{};
#ifndef NDEBUG
    int debugAnchor_{};
    console::Vector3 debugAt_{};float debugHeading_{};
    float debugHeight_=0.9f;
#endif
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
        paths_=Address<0x8BADB40>();area_=Address<0x8BB194C>();camera_=Address<0x8BC7E30>();
#ifndef NDEBUG
        far_.noDepth=inireader.ReadInteger("DISTANTTRAFFIC","DebugNoDepth",0)!=0;
        far_.usableOverride=inireader.ReadFloat("DISTANTTRAFFIC","DebugUsable",0);
        far_.sizeScale=inireader.ReadFloat("DISTANTTRAFFIC","DebugScale",1);
        far_.testQuad=inireader.ReadFloat("DISTANTTRAFFIC","DebugTestQuad",0);
        debugAnchor_=inireader.ReadInteger("DISTANTTRAFFIC","DebugAnchor",0);
        debugAt_={inireader.ReadFloat("DISTANTTRAFFIC","DebugAnchorX",0),inireader.ReadFloat("DISTANTTRAFFIC","DebugAnchorY",0),inireader.ReadFloat("DISTANTTRAFFIC","DebugAnchorZ",0)};
        debugHeading_=inireader.ReadFloat("DISTANTTRAFFIC","DebugAnchorHeading",0);
        debugHeight_=inireader.ReadFloat("DISTANTTRAFFIC","DebugAnchorHeight",0.9f);
#endif
        far_.initialize({Address<0x8BC7370>(),Address<0x8BB3BB4>(),Address<0x8AEE610>(),Address<0x8AEE8E0>(),Address<0x8AEE8D0>(),Address<0x8BB3BD4>()});
    }
    void render() {
        if(!enabled_)return;
        auto& r=renderer_;uintptr_t paths=*reinterpret_cast<uint32_t*>(paths_);
        if(!paths) {population_.clear();return;}
        console::traffic::Paths view{
            *reinterpret_cast<const console::traffic::Node**>(paths),
            *reinterpret_cast<const uint16_t**>(paths+30040),
            *reinterpret_cast<uint32_t*>(paths+16),*reinterpret_cast<uint16_t*>(paths+26)};
        population_.update(view,r.position(),*reinterpret_cast<uint32_t*>(r.timer),*reinterpret_cast<uint32_t*>(area_)==0);
        // TheCamera's matrix: forward row at +0x10.
        const auto forward=*reinterpret_cast<const console::Vector3*>(camera_+0x10);
#ifndef NDEBUG
        if(debugAnchor_==2) {
            auto& v=far_.test.vehicles[0];
            v.active=true;v.alpha=1;v.water=false;v.seed=3;
            v.forward={__builtin_cosf(debugHeading_),__builtin_sinf(debugHeading_),0};
            v.position=debugAt_;
        } else if(debugAnchor_) {
            // Test builds (US addresses): while the player drives, the test
            // vehicle copies the car's matrix, minus its height above the road.
            const uintptr_t ped=*reinterpret_cast<const uint32_t*>(0x8BDE4B0);
            const uintptr_t car=ped?*reinterpret_cast<const uint32_t*>(ped+0x480):0;
            if(car) {
                auto& v=far_.test.vehicles[0];
                const auto* m=reinterpret_cast<const float*>(car);
                v.active=true;v.alpha=1;v.water=false;v.seed=3;
                v.forward={m[4],m[5],0};
                const float l=__builtin_sqrtf(m[4]*m[4]+m[5]*m[5]);
                if(l>0.01f){v.forward.x/=l;v.forward.y/=l;}
                v.position={m[12],m[13],m[14]-debugHeight_};
            }
        }
#endif
        far_.draw(population_,r,forward,[](Vertex3D& to,const console::traffic::PositionColor& from) {
            to={0,0,from.color,from.position};
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
