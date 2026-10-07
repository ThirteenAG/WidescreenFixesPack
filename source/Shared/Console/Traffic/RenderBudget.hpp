#pragma once
#include <cstdint>
namespace console::traffic {
inline constexpr unsigned PS2MeshBytes(unsigned indices) {
    return (indices/3)*5*16*3+16*1024;
}
inline bool AdmitPS2Mesh(unsigned indices,int64_t available,unsigned& remaining) {
    const unsigned bytes=PS2MeshBytes(indices);
    if(bytes>remaining || available<0 || uint64_t(available)<uint64_t(bytes)+192*1024)return false;
    remaining-=bytes;return true;
}
}
