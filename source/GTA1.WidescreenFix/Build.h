#pragma once

// The executables this fix supports are three different compiles of the game:
//
// - Classics: the Rockstar Classics / Steam GTA1 and London 1969/1961 (WINO).
// - Retail: the original GTA1 retail release.
// - London: the original London 1969 and 1961 retail releases.
//
// Retail and London pack their game structures (player, car and ped records are
// smaller), keep the raster globals one slot further from the row table and, in
// London, call every function through a jump stub. Signatures, the byte offsets
// read from them and these constants are chosen per build.
namespace GTA1Build
{
    enum class Kind { Classics, Retail, London };
    inline Kind Current = Kind::Classics;

    // Game records.
    inline int PlayerStride = 444, ViewOffset = -0xA4;
    inline int CarStride = 688, CarHeading = 144, PedHeading = 168;
    // Raster globals in front of the row table: pitch, bytes per pixel, pitch in pixels.
    inline int RasterPitch = -4, RasterBytes = -3, RasterPixels = -2;
    // Mode list entries: size and the offset of the mode pointer.
    inline int ModeEntrySize = 28, ModeEntryMode = 24;

    inline void Set(Kind kind)
    {
        Current = kind;
        if (kind == Kind::Classics) return;
        PlayerStride = 417; ViewOffset = -0xA1;
        CarStride = 664; CarHeading = 144; PedHeading = 153;
        RasterPitch = -5; RasterBytes = -4; RasterPixels = -3;
        ModeEntrySize = 25; ModeEntryMode = 21;
    }

    // The signature and an offset for the running build.
    inline const char* Sig(const char* classics, const char* retail, const char* london)
    {
        switch (Current)
        {
        case Kind::Retail: return retail;
        case Kind::London: return london;
        default: return classics;
        }
    }
    inline int Off(int classics, int retail, int london)
    {
        switch (Current)
        {
        case Kind::Retail: return retail;
        case Kind::London: return london;
        default: return classics;
        }
    }
    inline hook::pattern Find(const char* classics, const char* retail, const char* london)
    {
        return hook::pattern(Sig(classics, retail, london));
    }

    // London calls through jump stubs: the function a call or stub leads to.
    inline uint8_t* Body(void* address)
    {
        auto code = static_cast<uint8_t*>(address);
        for (int i = 0; code && i < 4 && code[0] == 0xE9; ++i)
            code = code + 5 + *reinterpret_cast<int32_t*>(code + 1);
        return code;
    }
    inline uint8_t* Target(void* call) { return Body(injector::GetBranchDestination(call).get<void>()); }
}
