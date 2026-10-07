#pragma once
#include <cstddef>
#include <cstdint>

namespace console::traffic {
// Match the game's two DMA buffers, whose reuse is protected by its GE sync.
// Changing buffer starts selects the corresponding storage; an unexpected third
// native buffer fails closed instead of reusing vertices still consumed by GE.
template<size_t Capacity,size_t VertexSize=24> class FrameStorage {
    static_assert(Capacity%16==0,"GE storage must end on a 16-byte boundary");
    alignas(16) uint8_t data_[2][Capacity]{};
    uintptr_t native_[2]{};
    uintptr_t bottom_{},top_{};
    uint32_t frame_[2]{};
    bool used_[2]{};
public:
    // frame: the game's frame counter. A second pass in the same frame keeps
    // the data already referenced by that frame's GE commands.
    bool begin(uintptr_t native,uint32_t frame) {
        if(!native)return false;
        for(unsigned i=0;i<2;++i) {
            if(!native_[i])native_[i]=native;
            if(native_[i]!=native)continue;
            const uintptr_t bottom=reinterpret_cast<uintptr_t>(data_[i]);
            if(!used_[i] || frame_[i]!=frame || bottom_!=bottom) top_=bottom+Capacity;
            bottom_=bottom;frame_[i]=frame;used_[i]=true;
            return true;
        }
        bottom_=top_=0;return false;
    }
    uintptr_t top() const {return top_;}
    bool fits(unsigned vertices,unsigned indices) const {
        // Both native allocations round down to a 16-byte boundary.
        size_t bytes=size_t(vertices)*VertexSize+size_t(indices)*2+32;
        return top_>=bottom_&&bytes<=top_-bottom_;
    }
    void consumed(uintptr_t tail) {top_=tail;}
};
}
