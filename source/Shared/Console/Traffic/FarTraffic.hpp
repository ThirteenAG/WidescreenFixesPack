#pragma once
#include "Geometry.hpp"

// Low-polygon distant traffic shared by the PS2 and PSP Project2DFX modules.
//
// Most distant vehicles cover a few pixels. Each one is a convex box body (a
// pointed prism for boats) plus a cabin; only the faces turned towards the
// camera are emitted, so a car costs at most 3 body faces, 3 cabin faces and
// one light quad, and convex parts never overlap themselves (the traffic pass
// does not write depth). Vehicles are drawn farthest first in batches.
//
// Beyond the game's usable far distance the native projection rejects points
// and the GE clips them. Such vehicles are pulled in along the camera ray:
// scaling about the camera keeps the exact screen footprint, ordering is kept,
// and nearer world geometry still occludes them through the depth test.
namespace console::traffic {
struct FarFace {
    Vector3 normal;
    uint8_t first, count;          // polygon corners in FarShape::corners (3..5)
    DistantCarMesh::Material material;
    uint8_t level = 0;             // 0 always, 1 cabin (nearer vehicles), 2 lamp (night)
};
struct FarShape {
    const Vector3* corners;
    const FarFace* faces;
    unsigned faceCount;
};
namespace far_mesh {
// Car (metres, +Y forward, +Z up). Body box without a bottom face, a cabin
// and head/tail lamp quads set just outside the front and back faces.
inline constexpr Vector3 carCorners[]={
    // body: 0..3 bottom ring, 4..7 top ring (rl, rr, fr, fl)
    {-0.90f,-2.30f,0.30f},{0.90f,-2.30f,0.30f},{0.90f,2.30f,0.30f},{-0.90f,2.30f,0.30f},
    {-0.90f,-2.30f,0.85f},{0.90f,-2.30f,0.85f},{0.90f,2.30f,0.85f},{-0.90f,2.30f,0.85f},
    // cabin: 8..11 base ring, 12..15 roof ring (rl, rr, fr, fl)
    {-0.80f,-1.20f,0.85f},{0.80f,-1.20f,0.85f},{0.80f,0.90f,0.85f},{-0.80f,0.90f,0.85f},
    {-0.70f,-0.95f,1.32f},{0.70f,-0.95f,1.32f},{0.70f,0.45f,1.32f},{-0.70f,0.45f,1.32f},
    // head lamps 16..19, tail lamps 20..23
    {-0.80f,2.32f,0.50f},{0.80f,2.32f,0.50f},{0.80f,2.32f,0.72f},{-0.80f,2.32f,0.72f},
    {0.80f,-2.32f,0.52f},{-0.80f,-2.32f,0.52f},{-0.80f,-2.32f,0.74f},{0.80f,-2.32f,0.74f},
};
inline constexpr uint8_t carPolygons[]={
    0,3,7,4,  1,5,6,2,  3,2,6,7,  0,4,5,1,  4,7,6,5,      // body: left, right, front, back, top
    8,11,15,12, 9,13,14,10, 11,10,14,15, 8,12,13,9, 12,15,14,13, // cabin
    16,17,18,19, 20,21,22,23,                             // lamps
};
inline constexpr FarFace carFaces[]={
    {{-1,0,0},0,4,DistantCarMesh::Paint},
    {{1,0,0},4,4,DistantCarMesh::Paint},
    {{0,1,0},8,4,DistantCarMesh::Paint},
    {{0,-1,0},12,4,DistantCarMesh::Paint},
    {{0,0,1},16,4,DistantCarMesh::Paint},
    {{-0.98f,0,0.2f},20,4,DistantCarMesh::Glass,1},
    {{0.98f,0,0.2f},24,4,DistantCarMesh::Glass,1},
    {{0,0.75f,0.66f},28,4,DistantCarMesh::Glass,1},
    {{0,-0.7f,0.71f},32,4,DistantCarMesh::Glass,1},
    {{0,0,1},36,4,DistantCarMesh::Paint,1},
    {{0,1,0},40,4,DistantCarMesh::Headlight,2},
    {{0,-1,0},44,4,DistantCarMesh::Taillight,2},
};
// Boat: pointed hull prism (5-sided deck) and a cabin box.
inline constexpr Vector3 boatCorners[]={
    // hull keel ring 0..4 (stern left, stern right, mid right, bow, mid left), deck ring 5..9
    {-1.00f,-3.00f,0.00f},{1.00f,-3.00f,0.00f},{1.00f,1.20f,0.00f},{0.00f,3.60f,0.10f},{-1.00f,1.20f,0.00f},
    {-1.25f,-3.00f,0.90f},{1.25f,-3.00f,0.90f},{1.25f,1.20f,0.95f},{0.00f,4.20f,1.10f},{-1.25f,1.20f,0.95f},
    // cabin 10..13 base, 14..17 roof
    {-0.80f,-1.80f,0.92f},{0.80f,-1.80f,0.92f},{0.80f,0.60f,0.95f},{-0.80f,0.60f,0.95f},
    {-0.75f,-1.70f,1.85f},{0.75f,-1.70f,1.85f},{0.75f,0.30f,1.85f},{-0.75f,0.30f,1.85f},
};
inline constexpr uint8_t boatPolygons[]={
    0,4,9,5,  4,3,8,9,  3,2,7,8,  2,1,6,7,  1,0,5,6,  5,9,8,7,6,
    10,13,17,14, 11,15,16,12, 13,12,16,17, 10,14,15,11, 14,17,16,15,
};
inline constexpr FarFace boatFaces[]={
    {{-1,0,0},0,4,DistantCarMesh::BoatHull},
    {{-0.92f,0.38f,0},4,4,DistantCarMesh::BoatHull},
    {{0.92f,0.38f,0},8,4,DistantCarMesh::BoatHull},
    {{1,0,0},12,4,DistantCarMesh::BoatHull},
    {{0,-1,0},16,4,DistantCarMesh::BoatHull},
    {{0,0,1},20,5,DistantCarMesh::Chrome},
    {{-1,0,0},25,4,DistantCarMesh::Chrome,1},
    {{1,0,0},29,4,DistantCarMesh::Chrome,1},
    {{0,1,0},33,4,DistantCarMesh::Glass,1},
    {{0,-1,0},37,4,DistantCarMesh::Chrome,1},
    {{0,0,1},41,4,DistantCarMesh::Chrome,1},
};
}
inline constexpr unsigned farFaceMaxTriangles=3; // a five-corner deck

// Pull a far point in along the camera ray (see the header comment).
inline float pullScale(const Vector3& camera,const Vector3& p,float usable) {
    const float dx=p.x-camera.x,dy=p.y-camera.y,dz=p.z-camera.z;
    const float distance=__builtin_sqrtf(dx*dx+dy*dy+dz*dz);
    return distance>usable&&distance>0?usable/distance:1.0f;
}
inline Vector3 pullIn(const Vector3& camera,const Vector3& p,float scale) {
    return {camera.x+(p.x-camera.x)*scale,camera.y+(p.y-camera.y)*scale,camera.z+(p.z-camera.z)*scale};
}

// Farthest distance along the view direction that accepted(point) still
// projects (the native projection rejects points past the far clip). A few
// bisection steps; the caller keeps a margin below the result.
template<class Accepted>
inline float usableDistance(const Vector3& camera,const Vector3& forward,Accepted accepted,float low=40,float high=4000) {
    const auto at=[&](float d) { return accepted(Vector3{camera.x+forward.x*d,camera.y+forward.y*d,camera.z+forward.z*d}); };
    if(at(high))return high;
    if(!at(low))return low;
    for(unsigned i=0;i<12;++i) { const float mid=(low+high)*0.5f; (at(mid)?low:high)=mid; }
    return low;
}

// Fixed-capacity batches: vertices and indices are module-owned, no heap.
template<class NativeVertex,unsigned MaxVertices=768,unsigned MaxIndices=1152> class FarRenderer {
    alignas(16) NativeVertex vertices_[MaxVertices];
    short indices_[MaxIndices];
    unsigned vertexCount_{},indexCount_{};
    template<class Admit,class Submit> bool flush(Admit& admit,Submit& submit) {
        bool ok=true;
        if(indexCount_) ok=admit(indexCount_)&&submit(vertices_,vertexCount_,indices_,indexCount_);
        vertexCount_=indexCount_=0;
        return ok;
    }
    static const FarShape& shape(bool water) {
        static const FarShape car{far_mesh::carCorners,far_mesh::carFaces,sizeof(far_mesh::carFaces)/sizeof(far_mesh::carFaces[0])};
        static const FarShape boat{far_mesh::boatCorners,far_mesh::boatFaces,sizeof(far_mesh::boatFaces)/sizeof(far_mesh::boatFaces[0])};
        return water?boat:car;
    }
    static const uint8_t* polygons(bool water) { return water?far_mesh::boatPolygons:far_mesh::carPolygons; }
public:
    static constexpr unsigned maxVehicles=64,maxIndices=MaxIndices;
    unsigned drawn=0;
    // usable: farthest distance the native projection and GE clip accept.
    // visible(point) projects a (pulled-in) point. admit(indexCount) reserves
    // budget for one batch before it is submitted; submit(vertices,count,indices,indexCount) draws it
    // and returns false if it could not. skip(index) lets a caller draw some
    // vehicles with its detailed mesh instead.
    // Cabins are drawn up to cabinDistance; lamps only at night (both cost
    // DMA on PS2 and are sub-pixel beyond that).
    float cabinDistance=500;
    float sizeScale=1; // test builds may enlarge meshes to inspect them
    template<class Population,class Visible,class Admit,class Convert,class Submit,class Skip>
    void render(const Population& population,const Vector3& camera,float night,float usable,
                Visible visible,Admit admit,Convert convert,Submit submit,Skip skip) {
        drawn=0;
        struct Item { float distance;uint8_t index; };
        Item items[maxVehicles];
        unsigned count=0;
        for(unsigned i=0;i<Population::capacity&&i<maxVehicles;++i) {
            const auto& v=population.vehicles[i];
            if(!v.active||!(v.alpha>0)||skip(i))continue;
            const float dx=v.position.x-camera.x,dy=v.position.y-camera.y;
            Item item{dx*dx+dy*dy,uint8_t(i)};
            unsigned at=count++;
            // Farthest first: later (nearer) vehicles draw over earlier ones.
            while(at&&items[at-1].distance<item.distance) {items[at]=items[at-1];--at;}
            items[at]=item;
        }
        vertexCount_=indexCount_=0;
        for(unsigned k=0;k<count;++k) {
            const auto& v=population.vehicles[items[k].index];
            const float scale=pullScale(camera,v.position,usable);
            if(!visible(pullIn(camera,v.position,scale)))continue;
            const auto& s=shape(v.water);const uint8_t* poly=polygons(v.water);
            // Camera direction in vehicle space (rotation only; scale 1.2/0.8 is uniform).
            const float cx=camera.x-v.position.x,cy=camera.y-v.position.y,cz=camera.z-v.position.z;
            const float lx=v.forward.y*cx-v.forward.x*cy,ly=v.forward.x*cx+v.forward.y*cy;
            unsigned needVertices=0,needIndices=0;
            bool facing[16]{};
            const bool cabin=items[k].distance<cabinDistance*cabinDistance,lamps=night>0.2f;
            for(unsigned f=0;f<s.faceCount&&f<16;++f) {
                const auto& face=s.faces[f];
                if((face.level==1&&!cabin)||(face.level==2&&!lamps))continue;
                const auto& c=s.corners[poly[face.first]];
                const float sx=v.water?0.8f:1.2f;
                // Back-face test against one corner of the face.
                const float d=face.normal.x*(lx-c.x*sx)+face.normal.y*(ly-c.y*sx)+face.normal.z*(cz-c.z*sx);
                facing[f]=d>0;
                if(!facing[f])continue;
                needVertices+=face.count;needIndices+=3*(face.count-2);
            }
            if(!needIndices)continue;
            if(vertexCount_+needVertices>MaxVertices||indexCount_+needIndices>MaxIndices) {
                if(!flush(admit,submit))return;
            }
            for(unsigned f=0;f<s.faceCount&&f<16;++f) {
                if(!facing[f])continue;
                const auto& face=s.faces[f];
                const unsigned base=vertexCount_;
                for(unsigned c=0;c<face.count;++c) {
                    DistantCarMesh::Vertex in{s.corners[poly[face.first+c]],face.normal,face.material};
                    auto out=vertex(in,v,night);
                    if(sizeScale!=1.0f)out.position={v.position.x+(out.position.x-v.position.x)*sizeScale,
                        v.position.y+(out.position.y-v.position.y)*sizeScale,v.position.z+(out.position.z-v.position.z)*sizeScale};
                    if(scale<1.0f)out.position=pullIn(camera,out.position,scale);
                    convert(vertices_[vertexCount_++],out);
                }
                for(unsigned c=2;c<face.count;++c) {
                    // Corner lists run clockwise from outside; emit the
                    // counter-clockwise order the detailed meshes use.
                    indices_[indexCount_++]=short(base);
                    indices_[indexCount_++]=short(base+c);
                    indices_[indexCount_++]=short(base+c-1);
                }
            }
            ++drawn;
        }
        flush(admit,submit);
    }
};
}
