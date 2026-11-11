#include "Game.hpp"
#include "Shared/Console/PatchNotice.hpp"
#include "Movie.hpp"
#include <cstring>

namespace vcsws
{
namespace
{
constexpr uint32_t words[] = {0x3C050009u, 0x24A58000u, 0x00850018u, 0x34050200u,
    0x34060003u, 0x34070001u, 0x00002012u, 0x02042021u, 0x0C000000u, 0x24840010u,
    0x0C000000u, 0x00000000u};
constexpr uint8_t masks[] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 2, 0};
constexpr std::array<console::AddressRule, 1> rules = {{
    {0x08A1A28Cu, 0x08A1A26Cu, words, masks, 12, 16, 32, 0, 0, console::AddressKind::Code}
}};
console::NativeProfile<rules> profile;
injector::hook_back<int(void*, int, int, int)> present;
alignas(64) uint32_t frame[MovieMap::Width * MovieMap::Height];
MovieMap map;
float mappedAspect;
bool available;

int Present(void* pixels, int stride, int format, int sync)
{
    // This call belongs to the movie player's ready-frame path. Its decoder
    // has finished writing a fresh 8888 buffer before the buffer is displayed.
    console::portable::Unthrottle(false);
    const float aspect = autoAspect ? console::portable::Aspect() : Drawing::settings.aspect;
    constexpr float original = float(MovieMap::Width) / MovieMap::Height;
    const uintptr_t address = reinterpret_cast<uintptr_t>(pixels);
    if (std::fabs(aspect - original) > 0.001f && pixels &&
        stride == 512 && format == 3 && address >= 0x08000000u &&
        address <= 0x10000000u - 512u * MovieMap::Height * sizeof(uint32_t))
    {
        if (mappedAspect != aspect)
        {
            map.Set(aspect);
            mappedAspect = aspect;
        }
        auto output = static_cast<uint32_t*>(pixels);
        for (int y = 0; y < MovieMap::Height; ++y)
            std::memcpy(frame + y * MovieMap::Width, output + y * stride, MovieMap::Width * sizeof(uint32_t));
        for (int y = 0; y < MovieMap::Height; ++y)
        {
            const auto source = map.y[y] < 0 ? nullptr : frame + map.y[y] * MovieMap::Width;
            for (int x = 0; x < MovieMap::Width; ++x)
                output[y * stride + x] = source && map.x[x] >= 0 ? source[map.x[x]] : 0xFF000000u;
        }
        sceKernelDcacheWritebackRange(pixels, stride * MovieMap::Height * sizeof(uint32_t));
    }
    return present.fun(pixels, stride, format, sync);
}
}

void InstallMovies()
{
    available = profile.initialize({pattern.text_addr, reinterpret_cast<const uint8_t*>(pattern.text_addr),
        pattern.text_size, console::portable::gameGP});
    if (!available)
    {
        logger.Write("Movie correction unavailable: unsupported movie presentation code; original playback preserved.\n");
        console::portable::PatchRejected(PSP_HOOK_UNSUPPORTED_INSTRUCTION);
        return;
    }
    present.fun = injector::MakeCALL(profile.get<0x08A1A28C>(), Present).get();
}
}
