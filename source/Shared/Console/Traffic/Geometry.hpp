#pragma once
#include "Paths.hpp"
#include "DistantCarMesh.hpp"
#include "DistantBoatMesh.hpp"

namespace console::traffic {
struct PositionColor { Vector3 position;uint32_t color; };
inline PositionColor vertex(const DistantCarMesh::Vertex& input,const Vehicle& vehicle,float night) {
    const auto& p=input.position;
    float scale=vehicle.water?0.8f:1.2f;
    Vector3 position{
        vehicle.position.x+scale*(vehicle.forward.y*p.x+vehicle.forward.x*p.y),
        vehicle.position.y+scale*(-vehicle.forward.x*p.x+vehicle.forward.y*p.y),
        vehicle.position.z+scale*p.z};
    static constexpr uint32_t paints[]={0x304C70,0x647E88,0xA4A4A0,0x38383C,0x807058,0x305048,0x782C30,0x907028};
    uint32_t color=input.color;
    bool emissive=false;
    switch(input.material) {
    case DistantCarMesh::Paint:color=paints[vehicle.seed%8];break;
    case DistantCarMesh::Glass:color=0x303848;break;
    case DistantCarMesh::Rubber:color=0x181818;break;
    case DistantCarMesh::Chrome:color=0xB0B0B0;break;
    case DistantCarMesh::Headlight:color=night>0.2f?0xD8E8FF:0x989898;emissive=night>0.2f;break;
    case DistantCarMesh::Taillight:color=0x2020B0;emissive=true;break;
    default:break;
    }
    float lighting=emissive?1.0f:(0.45f+0.4f*saturate(input.normal.z*0.6f+0.4f))*(1.0f-0.65f*night);
    uint32_t r=unsigned((color&255)*lighting),g=unsigned(((color>>8)&255)*lighting),b=unsigned(((color>>16)&255)*lighting);
    return {position,r|(g<<8)|(b<<16)|(unsigned(vehicle.alpha*255)<<24)};
}
template<class NativeVertex> class MeshRenderer {
    DistantCarMesh::Mesh car_;
    DistantBoatMesh::Mesh boat_;
    alignas(16) NativeVertex vertices_[1024];
public:
    template<class Visible,class Convert,class Submit>
    void render(const Population& population,float night,Visible visible,Convert convert,Submit submit) {
        unsigned cars=0,boats=0;
        for(const auto& v:population.vehicles) {
            if(!v.active||v.alpha<=0||!visible(v.position))continue;
            if(v.water?boats>=2:cars>=6)continue;
            const DistantCarMesh::Geometry& mesh=v.water?static_cast<const DistantCarMesh::Geometry&>(boat_):car_;
            if(mesh.vertices.empty()||mesh.vertices.size()>1024||mesh.indices.empty())continue;
            for(unsigned i=0;i<mesh.vertices.size();++i)convert(vertices_[i],vertex(mesh.vertices[i],v,night));
            submit(vertices_,unsigned(mesh.vertices.size()),mesh.indices.data(),unsigned(mesh.indices.size()));
            v.water?++boats:++cars;
        }
    }
};
}
