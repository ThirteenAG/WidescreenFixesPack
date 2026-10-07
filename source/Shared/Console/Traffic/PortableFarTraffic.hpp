#pragma once
#include "FarTraffic.hpp"
#include "FrameStorage.hpp"
#include "../PortableLights.hpp"

// PSP (PPSSPP) submission of low-polygon distant traffic, shared by VCS and
// LCS. Vertex data goes to plugin storage paired with the game's two GE frame
// buffers; native Im3D functions emit the commands into the frame buffer.
namespace console::traffic {
struct PortableIm3D {
    uintptr_t arena;     // GE frame buffer: head +0, start +4, tail +12, cached BASE +0x14
    uintptr_t frame;     // CTimer::m_FrameCounter
    uintptr_t transform, indexed, end;
    uintptr_t farClip;   // CDraw::ms_fFarClipZ (follows time cycle, weather and fog)
};
template<class NativeVertex,size_t Storage=96*1024> class PortableFarTraffic {
    FarRenderer<NativeVertex> far_;
    FrameStorage<Storage,sizeof(NativeVertex)> storage_;
    PortableIm3D im3d_{};
    float usable_=300;
public:
#ifndef NDEBUG
    unsigned visible=0,submitted=0,refused=0;
    float lastUsable=0;
    bool noDepth=false; // test builds: show meshes through geometry
    float usableOverride=0,sizeScale=1;
    float testQuad=0; // test builds: upright 2 m quad this far ahead of the camera
    // Test builds: one extra vehicle at a fixed world position, compared with
    // a real game vehicle parked there.
    struct TestPopulation { static constexpr unsigned capacity=1; Vehicle vehicles[1]{}; } test;
#endif
    void initialize(const PortableIm3D& im3d) { im3d_=im3d; }

    template<class Population,class Convert>
    void draw(const Population& population,const portable::LightRenderer& r,const Vector3& forward,Convert convert) {
        auto* transform=reinterpret_cast<int (*)(NativeVertex*,unsigned,uintptr_t)>(im3d_.transform);
        auto* indexed=reinterpret_cast<int (*)(int,const short*,int)>(im3d_.indexed);
        auto* end=reinterpret_cast<int (*)()>(im3d_.end);
        const Vector3 camera=r.position();
        // Vehicles past the far clip are pulled in just inside it, keeping
        // their screen footprint; the GE would otherwise clip them.
        const float farClip=*reinterpret_cast<const float*>(im3d_.farClip);
        usable_=bounded(farClip,60.0f,4000.0f,300.0f)*0.94f;
#ifndef NDEBUG
        if(usableOverride>0)usable_=usableOverride;
        far_.sizeScale=sizeScale;
        lastUsable=usable_;
#endif
        if(!storage_.begin(*reinterpret_cast<const uint32_t*>(im3d_.arena+4),*reinterpret_cast<const uint32_t*>(im3d_.frame)))return;
        // Im3D selects the GE BASE for plugin storage and caches it; native
        // list commands (including JUMP/CALL targets) assume the frame's BASE.
        const uint32_t savedBase=*reinterpret_cast<const uint32_t*>(im3d_.arena+0x14);
        bool begun=false;
        far_.render(population,camera,nightIntensity(r.hour(),r.minute()),usable_,[&](const Vector3& p) {
            portable::ProjectedPoint world{p.x,p.y,p.z,0},screen;
            float w,h;
            const bool ok=r.project(&world,&screen,&w,&h,false)&&screen.z>0&&screen.x>-30&&screen.x<510&&screen.y>-30&&screen.y<302;
#ifndef NDEBUG
            if(ok)++visible;
#endif
            return ok;
        },[&](unsigned indexCount) {
            const auto head=*reinterpret_cast<const uint32_t*>(im3d_.arena);
            const auto tail=*reinterpret_cast<const uint32_t*>(im3d_.arena+12);
            // Each batch adds a few command words; keep the native reserve.
            if(tail<head||tail-head<128*1024||indexCount>FarRenderer<NativeVertex>::maxIndices) {
#ifndef NDEBUG
                ++refused;
#endif
                return false;
            }
            if(!begun) {
                begun=true;
                r.flush();r.state(1,0);r.state(r.zTest,1);r.state(r.zWrite,0);r.state(r.srcBlend,5);r.state(r.destBlend,6);
#ifndef NDEBUG
                if(noDepth)r.state(r.zTest,0);
#endif
            }
            return true;
        },convert,[&](NativeVertex* vertices,unsigned count,const short* indices,unsigned indexCount) {
            if(!storage_.fits(count,indexCount)) {
#ifndef NDEBUG
                ++refused;
#endif
                return false;
            }
#ifndef NDEBUG
            ++submitted;
#endif
            auto& tail=*reinterpret_cast<volatile uint32_t*>(im3d_.arena+12);
            const uint32_t nativeTail=tail;
            tail=uint32_t(storage_.top());
            if(transform(vertices,count,0)) {indexed(3,indices,int(indexCount));end();}
            const uintptr_t copiedStart=tail,copiedEnd=storage_.top();
            storage_.consumed(copiedStart);
            tail=nativeTail;
            sceKernelDcacheWritebackRange(reinterpret_cast<void*>(copiedStart),unsigned(copiedEnd-copiedStart));
            return true;
        },[](unsigned){return false;});
#ifndef NDEBUG
        if(test.vehicles[0].active) {
            FarRenderer<NativeVertex,64,96> one;
            one.render(test,camera,nightIntensity(r.hour(),r.minute()),usable_,[](const Vector3&){return true;},
                [&](unsigned){ if(!begun){begun=true;r.flush();r.state(1,0);r.state(r.zTest,1);r.state(r.zWrite,0);r.state(r.srcBlend,5);r.state(r.destBlend,6);} return true; },
                convert,[&](NativeVertex* vertices,unsigned count,const short* indices,unsigned indexCount) {
                    if(!storage_.fits(count,indexCount))return false;
                    auto& tail=*reinterpret_cast<volatile uint32_t*>(im3d_.arena+12);
                    const uint32_t nativeTail=tail;tail=uint32_t(storage_.top());
                    if(transform(vertices,count,0)) {indexed(3,indices,int(indexCount));end();}
                    const uintptr_t copiedStart=tail,copiedEnd=storage_.top();
                    storage_.consumed(copiedStart);tail=nativeTail;
                    sceKernelDcacheWritebackRange(reinterpret_cast<void*>(copiedStart),unsigned(copiedEnd-copiedStart));
                    return true;
                },[](unsigned){return false;});
        }
        if(testQuad>0&&begun) {
            const float fx=forward.x,fy=forward.y,l=__builtin_sqrtf(fx*fx+fy*fy);
            if(l>0.01f) for(unsigned k=0;k<4;++k) {
                // Four quads at testQuad*2^k, each the same apparent size.
                const float dist=testQuad*float(1u<<k),half=dist/16;
                const Vector3 side{-fy/l,fx/l,0};
                const float lateral=(float(k)-1.5f)*dist*0.3f;
                const Vector3 c{camera.x+fx/l*dist+side.x*lateral,camera.y+fy/l*dist+side.y*lateral,camera.z};
                NativeVertex q[4];
                const Vector3 p[4]={{c.x-side.x*half,c.y-side.y*half,c.z-2*half},{c.x+side.x*half,c.y+side.y*half,c.z-2*half},{c.x+side.x*half,c.y+side.y*half,c.z},{c.x-side.x*half,c.y-side.y*half,c.z}};
                static const uint32_t colours[4]={0xFF00FFFFu,0xFF00FF00u,0xFFFF0000u,0xFF0000FFu};
                for(unsigned i=0;i<4;++i)convert(q[i],PositionColor{p[i],colours[k]});
                static const short qi[6]={0,1,2,0,2,3};
                if(storage_.fits(4,6)) {
                    auto& tail=*reinterpret_cast<volatile uint32_t*>(im3d_.arena+12);
                    const uint32_t nativeTail=tail;tail=uint32_t(storage_.top());
                    if(transform(q,4,0)) {indexed(3,qi,6);end();}
                    const uintptr_t copiedStart=tail,copiedEnd=storage_.top();
                    storage_.consumed(copiedStart);tail=nativeTail;
                    sceKernelDcacheWritebackRange(reinterpret_cast<void*>(copiedStart),unsigned(copiedEnd-copiedStart));
                }
            }
        }
#endif
        if(!begun)return;
        auto& base=*reinterpret_cast<uint32_t*>(im3d_.arena+0x14);
        if(base!=savedBase) {
            auto& head=*reinterpret_cast<uint32_t*>(im3d_.arena);
            *reinterpret_cast<uint32_t*>(head)=0x10000000u|(savedBase&0x000F0000u);
            head+=4;base=savedBase;
        }
        r.state(1,0);r.state(r.zTest,1);r.state(r.zWrite,1);r.state(r.srcBlend,5);r.state(r.destBlend,6);
    }
};
}
