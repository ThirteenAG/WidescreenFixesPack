#include "stdafx.h"

struct Screen
{
    int Width;
    int Height;
    float fWidth;
    float fHeight;
    float fAspectRatio;
} Screen;

struct ResEntry
{
    int32_t offset;
    size_t length;
};

struct ResList
{
    ResEntry r01 = { 0  , length("800x600") };
    ResEntry r02 = { 8  , length("960x600") };
    ResEntry r03 = { 16 , length("1024x640xxx") };
    ResEntry r04 = { 28 , length("1024x768xxx") };
    ResEntry r05 = { 40 , length("1152x864xxx") };
    ResEntry r06 = { 52 , length("1360x765xxx") };
    ResEntry r07 = { 64 , length("1360x768xxx") };
    ResEntry r08 = { 76 , length("1280x768xxx") };
    ResEntry r09 = { 88 , length("1280x720xxx") };
    ResEntry r10 = { 100, length("1280x960xxx") };
    ResEntry r11 = { 112, length("1280x1024xx") };
    ResEntry r12 = { 124, length("1344x840xxx") };
    ResEntry r13 = { 136, length("1440x900xxx") };
    ResEntry r14 = { 148, length("1440x1080xx") };
    ResEntry r15 = { 160, length("1400x1050xx") };
    ResEntry r16 = { 172, length("1600x1024xx") };
    ResEntry r17 = { 184, length("1600x1200xx") };
    ResEntry r18 = { 195, length("1680x1050xx") };

    constexpr ResList()
    {}
};

DWORD WINAPI Init(LPVOID bDelay)
{
    auto pattern = hook::pattern("8B 74 24 10 0F BE 06 85 C0 57");

    if (pattern.count_hint(1).empty() && !bDelay)
    {
        CreateThread(0, 0, (LPTHREAD_START_ROUTINE)&Init, (LPVOID)true, 0, NULL);
        return 0;
    }

    if (bDelay)
        while (pattern.clear().count_hint(1).empty()) { Sleep(0); };

    CIniReader iniReader("");
    Screen.Width = iniReader.ReadInteger("MAIN", "ResX", 0);
    Screen.Height = iniReader.ReadInteger("MAIN", "ResY", 0);

    if (!Screen.Width || !Screen.Height)
        std::tie(Screen.Width, Screen.Height) = GetDesktopRes();

    Screen.fWidth = static_cast<float>(Screen.Width);
    Screen.fHeight = static_cast<float>(Screen.Height);
    Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);

    static constexpr ResList mem_chunk;
    static std::vector<std::string> list;
    GetResolutionsList(list);
    struct ResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.esi = *(uint32_t*)(regs.esp + 0x10);

            if (*(uint32_t*)(regs.esi + mem_chunk.r01.offset) == from_bytes<uint32_t>(to_bytes("800x")) && *(uint32_t*)(regs.esi + mem_chunk.r02.offset) == from_bytes<uint32_t>(to_bytes("960x"))) // strings from frontend_pc.str
            {
                ResEntry* data = (ResEntry*)&mem_chunk;
                injector::MemoryFill(regs.esi, 0, mem_chunk.r18.offset + mem_chunk.r18.length, true);

                for (int32_t i = (sizeof(mem_chunk) / sizeof(ResEntry)) - 1, k = list.size() - 1; (i >= 0 && k >= 0); i--, k--)
                {
                    if (list[k].length() <= data[i].length)
                    {
                        injector::WriteMemoryRaw(regs.esi + data[i].offset, list[k].data(), list[k].length(), true);
                    }
                }
            }

            regs.eax = *(uint8_t*)(regs.esi);
        }
    }; injector::MakeInline<ResHook>(pattern.get_first(0), pattern.get_first(7)); //0x67EBB3

    return 0;
}


BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD reason, LPVOID /*lpReserved*/)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        Init(NULL);
    }
    return TRUE;
}