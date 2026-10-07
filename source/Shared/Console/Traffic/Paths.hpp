#pragma once
#include "../Lights.hpp"
#include <cstddef>
#include <cstdint>

namespace console::traffic {
// VCS's packed road and water graph. This view never changes native path caches.
struct Node {
    int16_t x,y;
    int8_t z;
    uint8_t width;
    int16_t firstLink;
    uint16_t flags;
    Vector3 position() const { return {x/8.0f,y/8.0f,float(z<0?int(z)-100:int(z))}; }
    bool water() const { return flags&0x100; }
    bool disabled() const { return flags&0x20; }
    unsigned degree() const { return flags&15; }
};
static_assert(sizeof(Node)==10);
struct Paths {
    const Node* nodes{};
    const uint16_t* links{};
    unsigned count{},linkCount{};
    bool valid(unsigned n,bool water) const {
        if(!nodes||!links||n>=count)return false;
        const auto& p=nodes[n];
        return !p.disabled()&&p.water()==water&&p.degree()&&p.firstLink>=0&&
            unsigned(p.firstLink)<=linkCount&&p.degree()<=linkCount-unsigned(p.firstLink);
    }
    unsigned next(unsigned from,unsigned previous,bool water,uint32_t seed) const {
        if(!valid(from,water))return count;
        const auto& p=nodes[from];
        for(unsigned i=0;i<p.degree();++i) {
            unsigned to=links[p.firstLink+(seed+i)%p.degree()]&0x3fff;
            if(to!=previous&&to!=from&&valid(to,water))return to;
        }
        return valid(previous,water)?previous:count;
    }
};
struct Vehicle {
    unsigned from{},to{},previous{};
    uint32_t seed{};
    float along{},age{};
    Vector3 position{},forward{};
    float alpha{};
    bool active{},water{};
};
class Population {
    uint32_t seed_=0x56435332,stamp_{};
    unsigned cursor_{};
    const Node* graph_{};
    uint32_t random() { seed_^=seed_<<13;seed_^=seed_>>17;seed_^=seed_<<5;return seed_; }
public:
    static constexpr unsigned capacity=64;
    Vehicle vehicles[capacity]{};
    float range=800;
    unsigned carLimit=40,boatLimit=8;
    void clear() { for(auto& v:vehicles)v.active=false;graph_=nullptr;stamp_=0;cursor_=0; }
    void update(const Paths& paths,const Vector3& camera,uint32_t now,bool enabled) {
        if(!enabled||!paths.nodes||!paths.links||!paths.count||paths.count>65535) { clear();return; }
        if(graph_!=paths.nodes) { clear();graph_=paths.nodes;stamp_=now; }
        unsigned elapsed=now-stamp_;stamp_=now;
        if(!elapsed)return;
        float dt=float(elapsed>100?100:elapsed)*0.001f;
        unsigned cars=0,boats=0;
        for(auto& v:vehicles) {
            if(!v.active)continue;
            if(!paths.valid(v.from,v.water)||!paths.valid(v.to,v.water)) { v.active=false;continue; }
            v.age+=dt;v.along+=dt*(v.water?7.0f:13.0f);
            // A frame cannot traverse an unbounded number of short path edges.
            for(unsigned hops=0;hops<4;++hops) {
                Vector3 a=paths.nodes[v.from].position(),b=paths.nodes[v.to].position();
                float dx=b.x-a.x,dy=b.y-a.y;
                float length=__builtin_sqrtf(dx*dx+dy*dy);
                if(!(length>1.0f)) {v.active=false;break;}
                if(v.along<length) {
                    float t=v.along/length;
                    v.forward={dx/length,dy/length,0};
                    float lane=v.water?0.0f:1.5f;
                    v.position={a.x+dx*t+v.forward.y*lane,a.y+dy*t-v.forward.x*lane,a.z+(b.z-a.z)*t};
                    break;
                }
                v.along-=length;unsigned next=paths.next(v.to,v.from,v.water,random());
                v.previous=v.from;v.from=v.to;v.to=next;
                if(next>=paths.count||hops==3) {v.active=false;break;}
            }
            if(!v.active)continue;
            float dx=v.position.x-camera.x,dy=v.position.y-camera.y;
            float distance=__builtin_sqrtf(dx*dx+dy*dy);
            if(distance<300||distance>range+100) {v.active=false;continue;}
            v.alpha=saturate(v.age/1.2f)*saturate((distance-300)/100)*saturate((range-distance)/100);
            v.water?++boats:++cars;
        }
        unsigned spawned=0;
        // Scan a bounded slice even when all useful nodes are on another island.
        for(unsigned scan=0;scan<256&&spawned<4;++scan) {
            unsigned n=cursor_++%paths.count;bool water=paths.nodes[n].water();
            if((water?boats>=boatLimit:cars>=carLimit)||!paths.valid(n,water))continue;
            Vector3 p=paths.nodes[n].position();float dx=p.x-camera.x,dy=p.y-camera.y,d2=dx*dx+dy*dy;
            if(d2<400*400||d2>(range-50)*(range-50))continue;
            unsigned next=paths.next(n,paths.count,water,random());if(next>=paths.count)continue;
            bool occupied=false;
            for(const auto& v:vehicles)if(v.active&&v.water==water) {
                float x=v.position.x-p.x,y=v.position.y-p.y;
                if(x*x+y*y<60*60) {occupied=true;break;}
            }
            if(occupied)continue;
            for(auto& v:vehicles)if(!v.active) {
                v={};v.active=true;v.water=water;v.from=n;v.to=next;v.previous=paths.count;
                v.seed=random();v.position=p;water?++boats:++cars;++spawned;break;
            }
        }
    }
};
}
