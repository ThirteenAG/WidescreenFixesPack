#pragma once
#include <stdafx.h>
#include <stdlib.h>
#include <any>
#include "mips.hpp"
#include <iostream>
#include "FileWatch.hpp"

class PCSX2MemoryDyn
{
public:
    hook::pattern pattern = {};
    size_t size = 0;
    uint32_t patternNum = 0;
    uint32_t offset = 0;

    PCSX2MemoryDyn()
    {}

    PCSX2MemoryDyn(hook::pattern p, size_t s, uint32_t n, uint32_t o) : pattern(p), size(s), patternNum(n), offset(o)
    {}
};

class PCSX2Memory
{
public:
    enum patch_place_type
    {
        PPT_ONCE_ON_LOAD = 0,
        PPT_CONTINUOUSLY = 1
    };

    enum patch_cpu_type
    {
        NO_CPU,
        CPU_EE,
        CPU_IOP,

        CPU_EE_DYN
    };

    enum patch_data_type
    {
        NO_TYPE,
        BYTE_T,
        SHORT_T,
        WORD_T,
        DOUBLE_T,
        EXTENDED_T
    };

    enum patch_instruction
    {
        NONE,
        LUI_ORI,
        LUI_ORI_MUL,

        MAKE_INLINE
    };

    PCSX2Memory(patch_place_type place_type, patch_cpu_type cpu_type, std::any addr, patch_data_type data_type, patch_instruction instr, std::any data, std::wstring comment, std::any originalValue = std::any(), bool writeToPnach = true)
        : place_type(place_type), cpu_type(cpu_type), addr(addr), data_type(data_type), instr(instr), data(data), comment(comment), originalValue(originalValue)
    {
        if (addr.type() == typeid(PCSX2MemoryDyn))
        {
            this->dynAddr = std::any_cast<PCSX2MemoryDyn>(addr);
            this->addr.reset();
        }
    }

    PCSX2Memory(std::wstring cm)
        : comment(cm), isOnlyComment(true)
    {}

    std::wstring getCpuType()
    {
        switch (cpu_type)
        {
        case PCSX2Memory::NO_CPU:
            return L"";
        case PCSX2Memory::CPU_EE:
            return L"EE";
        case PCSX2Memory::CPU_IOP:
            return L"IOP";
        default:
            return L"EE";
        }
    }

    std::wstring getDataType()
    {
        return getDataType(data_type);
    }

    std::wstring getDataType(patch_data_type t)
    {
        switch (t)
        {
        case PCSX2Memory::NO_TYPE:
            return L"";
        case PCSX2Memory::BYTE_T:
            return L"byte";
        case PCSX2Memory::SHORT_T:
            return L"short";
        case PCSX2Memory::WORD_T:
            return L"word";
        case PCSX2Memory::DOUBLE_T:
            return L"double";
        case PCSX2Memory::EXTENDED_T:
            return L"extended";
        default:
            return L"";
        }
    }

    size_t getDataSize()
    {
        return getDataSize(data_type);
    }

    size_t getDataSize(patch_data_type t)
    {
        switch (t)
        {
        case PCSX2Memory::NO_TYPE:
            return 0;
        case PCSX2Memory::BYTE_T:
            return sizeof(uint8_t);
        case PCSX2Memory::SHORT_T:
            return sizeof(uint16_t);
        case PCSX2Memory::WORD_T:
            return sizeof(uint32_t);
        case PCSX2Memory::DOUBLE_T:
            return sizeof(uint64_t);
        case PCSX2Memory::EXTENDED_T:
            return sizeof(uint32_t);
        default:
            return 0;
        }
    }

    patch_place_type place_type;
    patch_cpu_type cpu_type;
    std::any addr;
    patch_data_type data_type;
    patch_instruction instr;
    std::any data;
    std::any originalValue;
    std::wstring comment;
    bool isOnlyComment = false;
    PCSX2MemoryDyn dynAddr;
};

class PCSX2
{
public:
    static inline hook::pattern pcsx2_crc_pattern = hook::pattern("83 3D ? ? ? ? ? 75 24");
    static inline uintptr_t EEMainMemoryStart = 0/*0x20000000*/;
    static inline uintptr_t EEMainMemoryEnd = 0/*0x21ffffff*/;
    static inline const uintptr_t IOPMainMemoryStart = 0x24000000;
    static inline const uintptr_t IOPMainMemoryEnd = 0x24211000;
    const std::filesystem::path PCSX2_ui = GetExeModulePath<std::wstring>() + L"inis\\PCSX2_ui.ini";
    const std::filesystem::path PCSX2_vm = GetExeModulePath<std::wstring>() + L"inis\\PCSX2_vm.ini";
    std::vector<PCSX2Memory> vecPatches;
    std::function<void(PCSX2& ps2)> UpdateDataCallback;

    PCSX2(std::initializer_list<uint32_t> crc_list, uint32_t customCodeBufAddr, std::function<void(PCSX2& ps2)>&& cb) : gameCRC(crc_list)
    {
        mBufAddr = customCodeBufAddr;
        pGameCRC = *pcsx2_crc_pattern.get_first<uint32_t*>(2);
        DeletePnach();
        UpdateDataCallback = std::forward<std::function<void(PCSX2& ps2)>>(cb);
    }

    void FindHostMemoryMapEEmem()
    {
        uintptr_t curAddr = 0;
        while (true)
        {
            MEMORY_BASIC_INFORMATION MemoryInf;
            if (VirtualQuery((LPCVOID)curAddr, &MemoryInf, sizeof(MemoryInf)) == 0) break;
            if (MemoryInf.AllocationProtect == PAGE_NOACCESS && MemoryInf.State == MEM_COMMIT &&
                MemoryInf.Protect == PAGE_READONLY && MemoryInf.Type == MEM_PRIVATE)
            {
                if (EEMainMemoryStart == 0)
                {
                    EEMainMemoryStart = (uintptr_t)MemoryInf.BaseAddress;
                }
            }
            else if (EEMainMemoryStart != 0 && MemoryInf.AllocationProtect == PAGE_NOACCESS && MemoryInf.State == MEM_RESERVE &&
                MemoryInf.Protect == 0 && MemoryInf.Type == MEM_PRIVATE)
            {
                EEMainMemoryEnd = (uintptr_t)MemoryInf.BaseAddress;
                return;
            }
            curAddr += MemoryInf.RegionSize;
        }
    };

    uint32_t FindMemoryBuffer()
    {
        auto test = [](uint8_t* begin, std::size_t bytes) -> bool
        {
            return std::all_of(begin, begin + bytes, [](uint8_t const byte)
            {
                return byte == 0;
            });
        };
        size_t end = EEMainMemoryStart + 0x2000000 - 0x10000;
        size_t NeededRegionSize = 10000;
        do
        {
            for (auto i = end - NeededRegionSize; i >= EEMainMemoryStart; i -= 1)
            {
                if (test((uint8_t*)i, NeededRegionSize))
                    return i - EEMainMemoryStart;
            }
            std::this_thread::yield();
        } while (true);

        return 0;
    }

    void EnableCallback()
    {
        GetPCSX2Data();
        UpdateDataCallback(*this);
        using namespace std::chrono_literals;

        static filewatch::FileWatch<std::wstring> watchui(this->PCSX2_ui.wstring(), [this](const std::wstring& path, const filewatch::Event change_type)
        {
            if (change_type == filewatch::Event::renamed_new)
            {
                std::this_thread::sleep_for(500ms);

                this->GetPCSX2Data();
                this->UpdateDataCallback(*this);
            }
        });

        if (fileExists(this->PCSX2_vm.string()))
        {
            char buf[32];
            GetPrivateProfileStringA("EmuCore", "EnableWideScreenPatches", "", buf, sizeof(buf), this->PCSX2_vm.string().c_str());
            this->IsEnableWidescreenPatchesChecked = std::string(buf) == "enabled";
        }

        static filewatch::FileWatch<std::wstring> watchvm(this->PCSX2_vm.wstring(), [this](const std::wstring& path, const filewatch::Event change_type)
        {
            if (change_type == filewatch::Event::renamed_new)
            {
                std::this_thread::sleep_for(500ms);

                if (fileExists(this->PCSX2_vm.string()))
                {
                    char buf[32];
                    GetPrivateProfileStringA("EmuCore", "EnableWideScreenPatches", "", buf, sizeof(buf), this->PCSX2_vm.string().c_str());
                    this->IsEnableWidescreenPatchesChecked = std::string(buf) == "enabled";
                }
            }
        });
    }

    void GetPCSX2Data()
    {
        if (!fileExists(this->PCSX2_ui.string()))
            return;

        char buf[32];
        GetPrivateProfileStringA("GSWindow", "IsFullscreen", "", buf, sizeof(buf), PCSX2_ui.string().c_str());
        IsFullscreen = std::string(buf) == "enabled";
        GetPrivateProfileStringA("GSWindow", "WindowSize", "", buf, sizeof(buf), PCSX2_ui.string().c_str());
        auto WindowSize = std::string(buf);
        GetPrivateProfileStringA("GSWindow", "AspectRatio", "", buf, sizeof(buf), PCSX2_ui.string().c_str());
        auto AspectRatioIni = std::string(buf);

        auto splitInTwo = [](std::string val, std::string_view delim) -> std::pair<std::string, std::string>
        {
            std::string arg;
            std::string::size_type pos = val.find(delim);
            if (val.npos != pos)
            {
                arg = val.substr(pos + 1);
                val = val.substr(0, pos);
            }
            return std::make_pair(val, arg);
        };

        auto ToInt = [](std::pair<std::string, std::string> p) -> std::pair<int32_t, int32_t>
        {
            return std::make_pair(std::stoi(p.first.empty() ? "0" : p.first), std::stoi(p.second.empty() ? "0" : p.second));
        };

        std::tie(WindowWidth, WindowHeight) = ToInt(splitInTwo(WindowSize, ","));
        if (IsFullscreen || !WindowWidth || !WindowHeight)
            std::tie(WindowWidth, WindowHeight) = GetDesktopRes();

        switch (AspectRatioIni == "Stretch")
        {
        case true:
            AspectRatio = (float)WindowWidth / (float)WindowHeight;
            break;
        case false:
            auto[w, h] = ToInt(splitInTwo(AspectRatioIni, ":"));
            if (w && h)
                AspectRatio = (float)w / (float)h;
            else
                AspectRatio = 16.0f / 9.0f;
            break;
        }
    }

    void WriteMemoryLoop()
    {
        __try
        {
            while (true)
            {
                using namespace std::chrono_literals;
                std::this_thread::sleep_for(100ms);
                if (isCRCValid() && !GetWidescreenPatchesOption())
                {
                    WriteMemory();
                }
            }
        }
        __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
        { }
    }

    uintptr_t GetBuffer()
    {
        FindHostMemoryMapEEmem();

        if (!mBufAddr)
            mBufAddr = FindMemoryBuffer();
        else
            mBufAddr = (mBufAddr >= EEMainMemoryStart) ? (mBufAddr - EEMainMemoryStart) : mBufAddr;

        return mBufAddr;
    }

    void WriteMemory()
    {
        mCurBufAddr = GetBuffer();

        for (auto& obj : vecPatches)
        {
            if (!obj.isOnlyComment)
            {
                if (obj.addr.has_value())
                {
                    if (*(uint32_t*)&obj.addr == 0)
                        continue;

                    if (obj.dynAddr.size > 0)
                    {
                        auto addr = *(uint32_t*)&obj.addr;
                        auto og = std::any_cast<uint64_t>(obj.originalValue);
                        bool b = false;

                        switch (obj.data_type)
                        {
                        case PCSX2Memory::BYTE_T:
                            b = *(uint8_t*)addr != *(uint8_t*)&og && *(uint8_t*)addr != (isPtr(obj.data) ? **(uint8_t**)(&obj.data) : *(uint8_t*)(&obj.data));
                            break;
                        case PCSX2Memory::SHORT_T:
                            b = *(uint16_t*)addr != *(uint16_t*)&og && *(uint16_t*)addr != (isPtr(obj.data) ? **(uint16_t**)(&obj.data) : *(uint16_t*)(&obj.data));
                            break;
                        case PCSX2Memory::WORD_T:
                        case PCSX2Memory::EXTENDED_T: //not implemented
                            b = *(uint32_t*)addr != *(uint32_t*)&og && *(uint32_t*)addr != (isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data));
                            break;
                        case PCSX2Memory::DOUBLE_T:
                            b = *(uint64_t*)addr != *(uint64_t*)&og && *(uint64_t*)addr != (isPtr(obj.data) ? **(uint64_t**)(&obj.data) : *(uint64_t*)(&obj.data));
                            break;
                        default:
                            break;
                        }

                        if (b)
                        {
                            obj.addr.reset();
                            continue;
                        }
                    }

                    if (obj.instr == PCSX2Memory::NONE)
                    {
                        auto addr = *(uint32_t*)&obj.addr;
                        if (addr < EEMainMemoryStart)
                            addr += EEMainMemoryStart;

                        switch (obj.data_type)
                        {
                        case PCSX2Memory::BYTE_T:
                            injector::WriteMemory<uint8_t>(addr, isPtr(obj.data) ? **(uint8_t**)(&obj.data) : *(uint8_t*)(&obj.data), true);
                            break;
                        case PCSX2Memory::SHORT_T:
                            injector::WriteMemory<uint16_t>(addr, isPtr(obj.data) ? **(uint16_t**)(&obj.data) : *(uint16_t*)(&obj.data), true);
                            break;
                        case PCSX2Memory::WORD_T:
                        case PCSX2Memory::EXTENDED_T: //not implemented
                            injector::WriteMemory<uint32_t>(addr, isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data), true);
                            break;
                        case PCSX2Memory::DOUBLE_T:
                            injector::WriteMemory<uint64_t>(addr, isPtr(obj.data) ? **(uint64_t**)(&obj.data) : *(uint64_t*)(&obj.data), true);
                            break;
                        default:
                            break;
                        }
                    }
                    else if (obj.instr == PCSX2Memory::LUI_ORI)
                    {
                        auto data = isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data);
                        auto hi = *(uint32_t*)&obj.addr;
                        if (hi < EEMainMemoryStart)
                            hi += EEMainMemoryStart;

                        uint32_t bytes = injector::ReadMemory<uint32_t>(hi, true);
                        uint8_t instr = (bytes >> (3 * 8)) & 0xFF;

                        if (instr == 0x08) //j
                        {
                            hi = ((bytes & 0x00FFFFFF) * 4) + EEMainMemoryStart; // jumping to address
                            bytes = injector::ReadMemory<uint32_t>(hi, true);
                            instr = (bytes >> (3 * 8)) & 0xFF;
                        }

                        if (instr >= lui1 && instr <= lui2)
                        {
                            auto originalValue = bytes;
                            if (obj.originalValue.has_value())
                                originalValue = isPtr(obj.originalValue) ? **(uint32_t**)(&obj.originalValue) : *(uint32_t*)(&obj.originalValue);

                            if (LOWORD(bytes) == HIWORD(originalValue))
                            {
                                for (uint32_t i = hi + 4; i < i + 24; i++)
                                {
                                    bytes = injector::ReadMemory<uint32_t>(i, true);
                                    instr = (bytes >> (3 * 8)) & 0xFF;

                                    if ((instr >= ori1 && instr <= ori2) && (LOWORD(bytes) == LOWORD(originalValue)))
                                    {
                                        auto lo = i;
                                        injector::WriteMemory<uint16_t>(hi, HIWORD(data), true);
                                        injector::WriteMemory<uint16_t>(lo, LOWORD(data), true);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    else if (obj.instr == PCSX2Memory::LUI_ORI_MUL)
                    {
                        for (size_t j = 0; j <= 0; j++)
                        {
                            auto data = isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data);
                            auto hi = *(uint32_t*)&obj.addr + j;
                            if (hi < EEMainMemoryStart)
                                hi += EEMainMemoryStart;

                            uint32_t bytes = injector::ReadMemory<uint32_t>(hi, true);
                            uint16_t instr = (uint16_t)(bytes >> 16);

                            if (instr == 0x3C01)
                            {
                                auto originalValue = bytes;
                                if (obj.originalValue.has_value())
                                    originalValue = isPtr(obj.originalValue) ? **(uint32_t**)(&obj.originalValue) : *(uint32_t*)(&obj.originalValue);

                                //if (LOWORD(bytes) == HIWORD(originalValue))
                                {
                                    for (uint32_t i = hi + 4; i < i + 24; i++)
                                    {
                                        bytes = injector::ReadMemory<uint32_t>(i, true);
                                        uint16_t instr = (uint16_t)(bytes >> 16);

                                        if ((instr == 0x4481 || instr == 0x3421) /* && (LOWORD(bytes) == LOWORD(originalValue))*/)
                                        {
                                            auto lo = i;

                                            union
                                            {
                                                struct
                                                {
                                                    uint16_t low;
                                                    uint16_t high;
                                                };
                                                float f;
                                            } u;

                                            u.high = injector::ReadMemory<uint16_t>(hi, true);
                                            u.low = injector::ReadMemory<uint16_t>(lo, true);

                                            float f = u.f /** *(float*)&data*/;
                                            injector::WriteMemory<uint16_t>(hi, HIWORD(data), true);
                                            injector::WriteMemory<uint16_t>(lo, LOWORD(data), true);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else if (obj.instr == PCSX2Memory::MAKE_INLINE)
                    {
                        auto addr = *(uint32_t*)&obj.addr;
                        if (addr >= EEMainMemoryStart)
                            addr -= EEMainMemoryStart;

                        std::ostringstream ss;
                        mips::j(ss, mCurBufAddr); //MakeJMP to custom code
                        injector::WriteMemory(addr + EEMainMemoryStart, *(uint32_t*)(ss.str()).c_str(), true);
                        ss.str("");
                        ss.clear();
                        auto makeInline = std::any_cast<std::function<void(std::ostringstream& buf)>>(obj.data);
                        makeInline(ss);
                        mips::j(ss, addr + 4); // MakeJMP back to original code
                        mips::nop(ss);
                        injector::WriteMemoryRaw(mCurBufAddr + EEMainMemoryStart, (void*)(ss.str()).c_str(), (size_t)ss.tellp(), true);
                        mCurBufAddr += (size_t)ss.tellp();
                    }
                }
                else if (obj.dynAddr.size > 0)
                {
                    if (obj.dynAddr.pattern.clear().size() == obj.dynAddr.size)
                    {
                        auto ptr = obj.dynAddr.pattern.get(obj.dynAddr.patternNum).get<void>(obj.dynAddr.offset);
                        obj.originalValue = *(uint64_t*)ptr;
                        obj.addr = ptr;
                    }
                }
            }
        }
    }

    void WritePnach()
    {
        mCurBufAddr = GetBuffer();

        auto filename = GetExeModulePath<std::wstring>() + L"cheats_ws\\" + int_to_hex(*pGameCRC) + L".pnach";
        std::wofstream pnach(filename, std::ios_base::out);

        pnach << L"// To recreate this file with new settings, close PCSX2, then reopen it and load the game." << std::endl;

        for (auto obj : vecPatches)
        {
            if (obj.isOnlyComment)
            {
                pnach << obj.comment << std::endl;
            }
            else if (obj.cpu_type != PCSX2Memory::NO_CPU && obj.cpu_type != PCSX2Memory::CPU_EE_DYN)
            {
                if (*(uint32_t*)&obj.addr == 0)
                    continue;

                if (obj.instr == PCSX2Memory::NONE)
                {
                    auto addr = *(uint32_t*)&obj.addr;
                    auto data = isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data);

                    auto t = pnach.tellp();
                    pnach << L"patch=" << obj.place_type << L"," << obj.getCpuType() << L"," << int_to_hex(addr) << L"," << obj.getDataType() << L"," << int_to_hex(data, obj.getDataSize());
                    pnach << std::setw(40 - (size_t)(pnach.tellp() - t)) << L"";
                    pnach << obj.comment << std::endl;

                }
                else if (obj.instr == PCSX2Memory::LUI_ORI)
                {
                    auto data = isPtr(obj.data) ? **(uint32_t**)(&obj.data) : *(uint32_t*)(&obj.data);
                    auto hi = *(uint32_t*)&obj.addr;
                    if (hi < EEMainMemoryStart)
                        hi += EEMainMemoryStart;
                    uint32_t bytes = injector::ReadMemory<uint32_t>(hi, true);
                    uint8_t instr = (bytes >> (3 * 8)) & 0xFF;

                    if (instr == 0x08) //j
                    {
                        hi = ((bytes & 0x00FFFFFF) * 4) + EEMainMemoryStart; // jumping to address
                        bytes = injector::ReadMemory<uint32_t>(hi, true);
                        instr = (bytes >> (3 * 8)) & 0xFF;
                    }

                    if (instr >= lui1 && instr <= lui2)
                    {
                        auto originalValue = bytes;
                        if (obj.originalValue.has_value())
                            originalValue = isPtr(obj.originalValue) ? **(uint32_t**)(&obj.originalValue) : *(uint32_t*)(&obj.originalValue);

                        if (LOWORD(bytes) == HIWORD(originalValue))
                        {
                            for (uint32_t i = hi + 4; i < i + 24; i++)
                            {
                                bytes = injector::ReadMemory<uint32_t>(i, true);
                                instr = (bytes >> (3 * 8)) & 0xFF;

                                if ((instr >= ori1 && instr <= ori2) && (LOWORD(bytes) == LOWORD(originalValue)))
                                {
                                    auto lo = i;
                                    auto t = pnach.tellp();
                                    pnach << L"patch=" << obj.place_type << L"," << obj.getCpuType() << L"," << int_to_hex(hi - EEMainMemoryStart) << L"," << obj.getDataType(PCSX2Memory::SHORT_T) << L"," << int_to_hex(HIWORD(data));
                                    pnach << std::setw(40 - (size_t)(pnach.tellp() - t)) << L"";
                                    pnach << obj.comment << std::endl;
                                    t = pnach.tellp();
                                    pnach << L"patch=" << obj.place_type << L"," << obj.getCpuType() << L"," << int_to_hex(lo - EEMainMemoryStart) << L"," << obj.getDataType(PCSX2Memory::SHORT_T) << L"," << int_to_hex(LOWORD(data));
                                    pnach << std::setw(40 - (size_t)(pnach.tellp() - t)) << L"";
                                    pnach << obj.comment << std::endl;
                                    pnach << std::endl;
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (obj.instr == PCSX2Memory::MAKE_INLINE)
                {
                    auto addr = *(uint32_t*)&obj.addr;

                    if (addr >= EEMainMemoryStart)
                        addr -= EEMainMemoryStart;

                    std::ostringstream ss;
                    mips::j(ss, mCurBufAddr); //MakeJMP to custom code
                    injector::WriteMemory(addr + EEMainMemoryStart, *(uint32_t*)(ss.str()).c_str(), true);
                    auto t = pnach.tellp();
                    pnach << L"patch=" << obj.place_type << L"," << obj.getCpuType() << L"," << int_to_hex(addr) << L"," << obj.getDataType() << L"," << int_to_hex(*(uint32_t*)(ss.str()).c_str());
                    pnach << std::setw(40 - (size_t)(pnach.tellp() - t)) << L"";
                    pnach << obj.comment << std::endl;
                    ss.str("");
                    ss.clear();
                    auto makeInline = std::any_cast<std::function<void(std::ostringstream& buf)>>(obj.data);
                    makeInline(ss);
                    mips::j(ss, addr + 4); // MakeJMP back to original code
                    mips::nop(ss);
                    injector::WriteMemoryRaw(mCurBufAddr + EEMainMemoryStart, (void*)(ss.str()).c_str(), (size_t)ss.tellp(), true);
                    for (size_t i = 0; i < (ss.str().size()); i += 4)
                    {
                        pnach << L"patch=" << obj.place_type << L"," << obj.getCpuType() << L"," << int_to_hex(mCurBufAddr + i) << L"," << obj.getDataType(PCSX2Memory::WORD_T) << L"," << int_to_hex(*(uint32_t*)& ss.str().at(i)) << std::endl;
                    }
                    mCurBufAddr += (size_t)ss.tellp();
                }
            }
        }
        pnach.close();
    }

    void DeletePnach()
    {
        for (auto crc : gameCRC)
        {
            auto pnach = GetExeModulePath<std::wstring>() + L"cheats_ws\\" + int_to_hex(crc) + L".pnach";
            if (fileExists(pnach))
                DeleteFile(pnach.c_str());
        }
    }

    bool isCRCValid()
    {
        return std::find(gameCRC.begin(), gameCRC.end(), *pGameCRC) != gameCRC.end();
    }

    template<typename T>
    T GV(std::initializer_list<T> list)
    {
        return *(list.begin() + GetGameVersion());
    }

    ptrdiff_t GetGameVersion()
    {
        return std::distance(gameCRC.begin(), std::find(gameCRC.begin(), gameCRC.end(), GetCurCRC()));
    }

    uint32_t GetCurCRC()
    {
        return *pGameCRC;
    }

    uint32_t GetWindowWidth()
    {
        return WindowWidth;
    }

    uint32_t GetWindowHeight()
    {
        return WindowHeight;
    }

    float GetAspectRatio()
    {
        return AspectRatio;
    }

    bool GetWidescreenPatchesOption()
    {
        return IsEnableWidescreenPatchesChecked;
    }

private:
    std::vector<uint32_t> gameCRC;
    uint32_t* pGameCRC;
    uint32_t  WindowWidth;
    uint32_t  WindowHeight;
    float     AspectRatio;
    bool      IsFullscreen;
    bool      IsEnableWidescreenPatchesChecked;

    const uint8_t lui1 = 0x3C;
    const uint8_t lui2 = 0x3F;
    const uint8_t ori1 = 0x34;
    const uint8_t ori2 = 0x37;
    const uint8_t ori3 = 0x44;

    uintptr_t mBufAddr = 0;
    uintptr_t mCurBufAddr = 0;

    bool isPtr(std::any& a)
    {
        return std::string(a.type().name()).back() == '*';
    };
};

static inline auto& EEStart = PCSX2::EEMainMemoryStart;
static inline auto& EEEnd = PCSX2::EEMainMemoryEnd;
static inline constexpr auto ONCE = PCSX2Memory::PPT_ONCE_ON_LOAD;
static inline constexpr auto CONT = PCSX2Memory::PPT_CONTINUOUSLY;
static inline constexpr auto EE = PCSX2Memory::CPU_EE;
static inline constexpr auto IOP = PCSX2Memory::CPU_IOP;
static inline constexpr auto EE_DYN = PCSX2Memory::CPU_EE_DYN;
static inline constexpr auto BYTE_T = PCSX2Memory::BYTE_T;
static inline constexpr auto SHORT_T = PCSX2Memory::SHORT_T;
static inline constexpr auto WORD_T = PCSX2Memory::WORD_T;
static inline constexpr auto DOUBLE_T = PCSX2Memory::DOUBLE_T;
static inline constexpr auto EXTENDED_T = PCSX2Memory::EXTENDED_T;
static inline constexpr auto NONE = PCSX2Memory::NONE;
static inline constexpr auto LUI_ORI = PCSX2Memory::LUI_ORI;
static inline constexpr auto LUI_ORI_MUL = PCSX2Memory::LUI_ORI_MUL;
static inline constexpr auto MAKE_INLINE = PCSX2Memory::MAKE_INLINE;
typedef std::ostringstream oss;
typedef std::function<void(oss& buf)> mips_asm;