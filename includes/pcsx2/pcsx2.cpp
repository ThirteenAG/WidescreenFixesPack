#include "pcsx2.h"
#include "ModuleList.hpp"

CEXP PCSX2Data PCSX2GetData()
{
    static auto _PCSX2GetData = (PCSX2Data(*)())nullptr;
    static bool bPatched = false;
    static PCSX2Data data;
    if (!bPatched)
    {
        if (_PCSX2GetData)
            return _PCSX2GetData();

        auto pattern = hook::pattern("C6 45 FC 08 8D 47 ? FF 70 04");
        if (pattern.empty())
        {
            ModuleList dlls;
            dlls.Enumerate(ModuleList::SearchLocation::LocalOnly);
            for (auto& e : dlls.m_moduleList)
            {
                auto fn = (PCSX2Data(*)())GetProcAddress(std::get<HMODULE>(e), "PCSX2GetData");
                if (fn != NULL)
                {
                    _PCSX2GetData = fn;
                    return _PCSX2GetData();
                }
            }
        }
        else
        {
            static auto offs = *pattern.get_first<uint8_t>(7);
            struct GetOptionsRes
            {
                void operator()(injector::reg_pack& regs)
                {
                    *(uint8_t*)(regs.ebp - 4) = 8;
                    regs.eax = regs.edi + offs;
                    data.pGSWindowOptions = *(GSWindowOptions**)(regs.ebp - 0x3C);
                }
            }; injector::MakeInline<GetOptionsRes>(pattern.get_first(0), pattern.get_first(7));

            pattern = hook::pattern("FF 35 ? ? ? ? 51 68 ? ? ? ? 50 E8 ? ? ? ? 83 C4 18 8B C8 E8 ? ? ? ? C6 45 FC 05"); //game crc
            data.pGameCRC = *pattern.get_first<uint32_t*>(2);

            auto[DW, DH] = GetDesktopRes();
            data.DesktopWidth = DW;
            data.DesktopHeight = DH;

            bPatched = true;
        }
    }

    data.GameCRC = *data.pGameCRC;
    if (data.pGSWindowOptions != nullptr)
    {
        data.Fullscreen = data.pGSWindowOptions->IsFullscreen;
        data.AspectRatioSwitch = data.pGSWindowOptions->AspectRatio;
        data.WindowWidth = data.Fullscreen ? data.DesktopWidth : data.pGSWindowOptions->WindowSize.x;
        data.WindowHeight = data.Fullscreen ? data.DesktopHeight : data.pGSWindowOptions->WindowSize.y;
        switch (data.AspectRatioSwitch)
        {
        case 1:
            data.AspectRatio = 4.0f / 3.0f;
            break;
        case 2:
            data.AspectRatio = 16.0f / 9.0f;
            break;
        default:
            data.AspectRatio = (float)data.WindowWidth / (float)data.WindowHeight;
            break;
        }
    }
    return data;
}

void WritePnach(std::wstring& filename, std::vector<MagicNumberSt>& arr, std::wstring_view gametitle, std::wstring_view comment)
{
    if (!fileExists(filename))
    {
        std::wofstream pnach;
        pnach.open(filename, std::ios_base::out);
        if (pnach.is_open())
        {
            pnach << L"gametitle=" << gametitle << std::endl;
            pnach << L"comment=" << comment << std::endl;
            pnach << std::endl;

            for each (auto addr in arr)
            {
                if (addr.ptr != nullptr)
                {
                    pnach << L"patch=1,EE," + int_to_hex((uint32_t)addr.ptr - EEMainMemoryStart) + L",word," + int_to_hex(*(uint32_t*)addr.ptrNew) << std::endl;
                }
                else
                {
                    pnach << L"patch=1,EE," + int_to_hex((uint32_t)addr.hi - EEMainMemoryStart) + L",short," + int_to_hex(HIWORD(*(uint32_t*)addr.ptrNew)) << std::endl;
                    pnach << L"patch=1,EE," + int_to_hex((uint32_t)addr.lo - EEMainMemoryStart) + L",short," + int_to_hex(LOWORD(*(uint32_t*)addr.ptrNew)) << std::endl;
                }
                pnach << std::endl;
            }
            pnach.close();
        }
    }
}