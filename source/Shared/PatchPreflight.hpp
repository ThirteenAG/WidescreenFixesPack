#pragma once

#include <initializer_list>

namespace wfp
{
    struct PatchSignature
    {
        const char* bytes;
        size_t minimum;
    };

    // Check the whole feature before changing any of its instructions.
    inline bool PatchPreflight(const char* failure, std::initializer_list<PatchSignature> signatures)
    {
        for (const auto& signature : signatures)
        {
            auto pattern = hook::pattern(signature.bytes);
            const auto count = pattern.size();
            if (count < (signature.minimum ? signature.minimum : 1))
            {
                OutputDebugStringA(failure);
                return false;
            }
        }
        return true;
    }

    inline bool PatchPreflight(HMODULE module, const char* failure, std::initializer_list<PatchSignature> signatures)
    {
        if (!module)
        {
            OutputDebugStringA(failure);
            return false;
        }
        for (const auto& signature : signatures)
        {
            auto pattern = hook::module_pattern(module, signature.bytes);
            const auto count = pattern.size();
            if (count < (signature.minimum ? signature.minimum : 1))
            {
                OutputDebugStringA(failure);
                return false;
            }
        }
        return true;
    }

}
