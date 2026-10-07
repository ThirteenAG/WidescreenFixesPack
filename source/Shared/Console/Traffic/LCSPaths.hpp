#pragma once
#include "../Lights.hpp"
#include <cstddef>
#include <cstdint>

namespace console::traffic {
// Liberty City Stories CPathNode (20 bytes, reLCS PathFind.h). Car and boat
// nodes come first in the node array; pedestrian nodes follow them.
struct LCSNode {
    int16_t previous,next;
    int16_t x,y,z;
    int16_t distance;
    int16_t firstLink;
    uint8_t width;
    int8_t group;
    uint8_t flags;      // numLinks:4, deadEnd, disabled, betweenLevels, roadBlock
    uint8_t flags2;     // waterPath, onlySmallBoats, selected, speedLimit:2
    uint8_t flags3,pad;
    Vector3 position() const { return {x/8.0f,y/8.0f,z/8.0f}; }
    bool water() const { return flags2&1; }
    bool disabled() const { return flags&0x20; }
    unsigned degree() const { return flags&15; }
};
static_assert(sizeof(LCSNode)==20&&offsetof(LCSNode,firstLink)==12&&offsetof(LCSNode,flags)==16);

// Read-only view of the resident path graph. count is the number of car/boat
// nodes; links are the 14-bit node indices of the connection array.
struct LCSPaths {
    const LCSNode* nodes{};
    const uint16_t* links{};
    unsigned count{},linkCount{};
    const void* identity() const { return nodes&&links?nodes:nullptr; }
    Vector3 position(unsigned n) const { return nodes[n].position(); }
    bool water(unsigned n) const { return nodes[n].water(); }
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
}
