#pragma once
#include "Geometry.hpp"

// Graph-independent form of console::traffic::Population and MeshRenderer.
// A Graph provides: count, identity(), valid(n,water), next(from,previous,water,seed),
// position(n) and water(n). It is a read-only view; native path data is never written.
namespace console::traffic {
template<class Graph> class GraphPopulation {
    uint32_t seed_=0x4C435332,stamp_{};
    unsigned cursor_{};
    const void* graph_{};
    uint32_t random() { seed_^=seed_<<13;seed_^=seed_>>17;seed_^=seed_<<5;return seed_; }
public:
    static constexpr unsigned capacity=64;
    Vehicle vehicles[capacity]{};
    float range=800;
    unsigned carLimit=40,boatLimit=8;
    void clear() { for(auto& v:vehicles)v.active=false;graph_=nullptr;stamp_=0;cursor_=0; }
    void update(const Graph& paths,const Vector3& camera,uint32_t now,bool enabled) {
        if(!enabled||!paths.identity()||!paths.count||paths.count>65535) { clear();return; }
        if(graph_!=paths.identity()) { clear();graph_=paths.identity();stamp_=now; }
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
                Vector3 a=paths.position(v.from),b=paths.position(v.to);
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
            unsigned n=cursor_++%paths.count;bool water=paths.water(n);
            if((water?boats>=boatLimit:cars>=carLimit)||!paths.valid(n,water))continue;
            Vector3 p=paths.position(n);float dx=p.x-camera.x,dy=p.y-camera.y,d2=dx*dx+dy*dy;
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

// Submits the nearest visible vehicles first, so a frame budget that admits
// only a few meshes spends it on the most visible ones and does not starve
// vehicles that happen to sit late in the slot array.
template<class NativeVertex> class NearestMeshRenderer {
    DistantCarMesh::Mesh car_;
    DistantBoatMesh::Mesh boat_;
    alignas(16) NativeVertex vertices_[1024];
public:
    static constexpr unsigned maxCars=6,maxBoats=2;
    // admit(indexCount) reserves budget for one complete mesh before any
    // conversion work; a refusal ends the frame's submissions.
    template<class Population,class Visible,class Admit,class Convert,class Submit>
    void render(const Population& population,const Vector3& camera,float night,Visible visible,Admit admit,Convert convert,Submit submit) {
        struct Item { float distance;uint8_t index; };
        Item items[Population::capacity];
        unsigned count=0;
        for(unsigned i=0;i<Population::capacity;++i) {
            const auto& v=population.vehicles[i];
            if(!v.active||!(v.alpha>0))continue;
            float dx=v.position.x-camera.x,dy=v.position.y-camera.y;
            Item item{dx*dx+dy*dy,uint8_t(i)};
            unsigned at=count++;
            while(at&&items[at-1].distance>item.distance) {items[at]=items[at-1];--at;}
            items[at]=item;
        }
        unsigned cars=0,boats=0;
        for(unsigned k=0;k<count&&(cars<maxCars||boats<maxBoats);++k) {
            const auto& v=population.vehicles[items[k].index];
            if(v.water?boats>=maxBoats:cars>=maxCars)continue;
            if(!visible(v.position))continue;
            const DistantCarMesh::Geometry& mesh=v.water?static_cast<const DistantCarMesh::Geometry&>(boat_):car_;
            if(mesh.vertices.empty()||mesh.vertices.size()>1024||mesh.indices.empty())continue;
            if(!admit(unsigned(mesh.indices.size())))break;
            for(unsigned i=0;i<mesh.vertices.size();++i)convert(vertices_[i],vertex(mesh.vertices[i],v,night));
            submit(vertices_,unsigned(mesh.vertices.size()),mesh.indices.data(),unsigned(mesh.indices.size()));
            v.water?++boats:++cars;
        }
    }
};
}
