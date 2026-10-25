#pragma once

namespace SCGameplayFOV
{
    inline float Factor(std::wstring_view state, float factor)
    {
        // Scripted, aiming and gadget cameras keep their own framing.
        constexpr std::wstring_view gameplay[] =
        {
            L"s_PlayerWalking", L"s_Carry", L"s_Roll", L"s_SideRoll", L"s_QuickStep",
            L"s_PlayerJumping", L"s_PlayerBTW", L"s_PlayerBTWMove", L"s_PlayerBTWPeek",
            L"s_Rappelling", L"s_RappellingFall", L"s_Split", L"s_Fence", L"s_Ledge",
            L"s_GrabbingLedge", L"s_HoistingAboveLedge", L"s_GrabbingHandOverHand",
            L"s_HandOverHand", L"s_HandOverHandFeetUp", L"s_GrabbingNarrowLadder",
            L"s_NarrowLadder", L"s_NarrowLadderSlideDown", L"s_GrabbingPipe", L"s_Pipe",
            L"s_PipeSlideDown", L"s_GrabbingZipLine", L"s_ZipLine", L"s_GrabbingPole",
            L"s_Pole", L"s_PoleSlideDown", L"s_PlayerSwimming"
        };
        if (std::find(std::begin(gameplay), std::end(gameplay), state) == std::end(gameplay))
            return 1.0f;
        return std::isfinite(factor) ? std::clamp(factor, 0.5f, 2.0f) : 1.0f;
    }
}
