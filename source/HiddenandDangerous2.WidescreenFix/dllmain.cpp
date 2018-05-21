#include "stdafx.h"

bool bSettingsApp;

struct Screen
{
    int32_t Width;
    int32_t Height;
    float fWidth;
    float fHeight;
    float fFieldOfView;
    float fAspectRatio;
    float fCustomAspectRatioHor;
    float fCustomAspectRatioVer;
    int32_t Width43;
    float fWidth43;
    float fHudScale;
    float fHudOffset;
    float fCutOffArea;
    float fFMVScale;
    float fDynamicScreenFieldOfViewScale;
    float fCustomFieldOfView;
} Screen;

struct S_matrix
{
    float a1 = 1.0f;
    float a2 = 0.0f;
    float a3 = 0.0f;

    uint32_t pad1 = 0;

    float a4 = 0.0f;
    float a5 = 1.0f;
    float a6 = 0.0f;

    uint32_t pad2 = 0;

    float a7 = 0.0f;
    float a8 = 0.0f;
    float a9 = 1.0f;

    float a10 = 0.0f;
    float a11 = 0.0f;
    float a12 = 0.0f;
    float a13 = 0.0f;
    float a14 = 1.0f;
} mat;

int32_t CaptureStackBackTrace2(int32_t FramesToSkip, int32_t nFrames, PVOID* BackTrace, PDWORD pBackTraceHash)
{
    int32_t regEBP;
    __asm mov regEBP, ebp;

    uint32_t *pFrame = (uint32_t*)regEBP;
    void* pNextInstruction;
    int32_t iFrame = 0;

    __try {
        for (; iFrame < nFrames; iFrame++)
        {
            if (*pFrame == 0x40000000) // some magic, stack trace is incomplete if function was called from exe, this sort of fixes it
                pFrame = (uint32_t*)(pFrame + 4); // seems like only works properly for first 4 callers, then breaks

            pNextInstruction = (void*)(*(pFrame + 1));

            if (pNextInstruction == nullptr)
                break;

            BackTrace[iFrame] = pNextInstruction;
            pFrame = (uint32_t*)(*pFrame);
        }
    }
    __except ((GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {}

    return iFrame;
}

std::vector<uintptr_t> S_matrix_Identity_Xrefs;
uintptr_t dword_69E690, dword_66AEDC;
void __stdcall S_matrix_Identity(S_matrix* _this)
{
    static void* stack[5];
    CaptureStackBackTrace2(0, 4, stack, NULL);

    memcpy(_this, &mat, sizeof(S_matrix));

    if (std::end(S_matrix_Identity_Xrefs) != std::find(std::begin(S_matrix_Identity_Xrefs), std::end(S_matrix_Identity_Xrefs), (uintptr_t)stack[3]))
    {
        auto self = (S_matrix*)((uintptr_t)_this - 0x150 + 0x40);
        self->a1 = Screen.fHudScale;
    }
    else
    {
        if ((uintptr_t)stack[3] == dword_69E690 && (uintptr_t)stack[2] == dword_66AEDC)
        {
            auto self = (S_matrix*)((uintptr_t)_this - 0x150 + 0x40);
            self->a1 = Screen.fHudScale;
        }
    }

    DBGONLY(spd::log()->info("{0:x}", stack[3]););
}

void InitSettings()
{
    auto pattern = hook::pattern("0F ? ? ? ? ? 81 ? 80 02 00 00"); //0x4029AB 0x4029AB
    injector::MakeNOP(pattern.count_hint(2).get(0).get<void*>(0), 6, true);
    injector::MakeNOP(pattern.count_hint(2).get(1).get<void*>(0), 6, true);
}

void Init()
{
    CIniReader iniReader("");
    bool bFixHUD = iniReader.ReadInteger("MAIN", "FixHUD", 0) != 0;
    Screen.fCustomFieldOfView = iniReader.ReadFloat("MAIN", "FOVFactor", 1.0f);
    bool bInsertKey = iniReader.ReadInteger("MAIN", "InsertKey", 1) != 0;

    auto pattern = hook::pattern("68 ? ? ? ? 8B 80 5C"); //0x423E65 0x424077
    static auto ar1 = pattern.count(2).get(0).get<float>(1);
    static auto ar2 = pattern.count(2).get(1).get<float>(1);
    pattern = hook::pattern("C7 45 EC ? ? ? ? FF D6"); //0x6DB137
    struct GetResHook
    {
        void operator()(injector::reg_pack& regs)
        {
            Screen.Width = regs.eax;
            Screen.Height = regs.edx;
            Screen.fWidth = static_cast<float>(Screen.Width);
            Screen.fHeight = static_cast<float>(Screen.Height);
            Screen.fAspectRatio = (Screen.fWidth / Screen.fHeight);
            Screen.Width43 = static_cast<uint32_t>(Screen.fHeight * (4.0f / 3.0f));
            Screen.fWidth43 = static_cast<float>(Screen.Width43);
            Screen.fHudScale = (1.0f * ((4.0f / 3.0f) / (Screen.fAspectRatio)));
            Screen.fHudOffset = (Screen.fWidth - Screen.fHeight * (4.0f / 3.0f)) / 2.0f;
            Screen.fDynamicScreenFieldOfViewScale = (0.5f / ((4.0f / 3.0f) / (Screen.fAspectRatio))) * (Screen.fCustomFieldOfView ? Screen.fCustomFieldOfView : 1.0f);

            //aspect ratio
            injector::WriteMemory<float>(ar1, Screen.fAspectRatio, true);
            injector::WriteMemory<float>(ar2, Screen.fAspectRatio, true);
        }
    }; injector::MakeInline<GetResHook>(pattern.get_first(0), pattern.get_first(7));

    //FOV and aspect ratio again
    auto mpattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "D8 0D ? ? ? ? D9 82 38 01 00 00");
    injector::WriteMemory(mpattern.count_hint(2).get(0).get<void*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);
    injector::WriteMemory(mpattern.count_hint(2).get(1).get<void*>(2), &Screen.fDynamicScreenFieldOfViewScale, true);

    /*static int test[] = {
        0x00464DE1,
        0x00464E2C,
        0x0048DB15,
        //0x0049F5EF,
        //0x00698B75,
        //0x0069E690,
        //0x006C1005,
        //0x006C29ED,
        //0x006CA8B6,
        //0x006CCCF2,
        //0x006D3F7A
    };*/

    static auto dword_464DE1 = (uintptr_t)hook::get_pattern("6A 0F B9 ? ? ? ? E8 ? ? ? ? C3", 12);
    static auto dword_464E2C = (uintptr_t)hook::get_pattern("B9 ? ? ? ? E8 ? ? ? ? 8B C8 E8 ? ? ? ? 85 C0 ? ? 8B 80 60 01");
    static auto dword_48DB15 = (uintptr_t)hook::get_pattern("FF 50 1C 83 C6 04 81 FE ? ? ? ? 7C E9 5F 5E C3", 3);
    mpattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "8D 8B 50 01 00 00 51");
    struct ARFix
    {
        void operator()(injector::reg_pack& regs)
        {
            static void* stack[5];
            CaptureStackBackTrace2(0, 4, stack, NULL);
            //logfile << std::hex << stack[3] << std::endl;

            regs.ecx = regs.ebx + 0x150;
            if ((uintptr_t)stack[3] == dword_464DE1 || (uintptr_t)stack[3] == dword_464E2C || (uintptr_t)stack[3] == dword_48DB15 ||
                (uintptr_t)stack[2] == dword_464DE1 || (uintptr_t)stack[2] == dword_464E2C || (uintptr_t)stack[2] == dword_48DB15)
                *(float*)(regs.ebx + 0x138) = Screen.fAspectRatio;
        }
    }; injector::MakeInline<ARFix>(mpattern.get_first(0), mpattern.get_first(6));

    //2D
    mpattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "78 ? A1 ? ? ? ? C2 04 00");
    static auto dword_1000988F = (uint32_t)hook::module_pattern(GetModuleHandle(L"LS3DF"), "FF 51 18 89 44 24 0C A1").get_first();
    struct BrokenHudFix
    {
        void operator()(injector::reg_pack& regs)
        {
            regs.eax = Screen.Width;

            auto callerAddr = *(uint32_t*)(regs.esp) - 3;
            //logfile << std::hex << callerAddr << std::endl;
            if (callerAddr == dword_1000988F)
                regs.eax = Screen.Width43;
        }
    }; injector::MakeInline<BrokenHudFix>(mpattern.count(2).get(0).get<void*>(0), mpattern.count(2).get(0).get<void*>(7));

    if (bFixHUD)
    {
        //hud fix
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("32 C9 E8 ? ? ? ? 68 ? ? ? ? 6A 05")); // 0x00698B75
        //S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("8B DE 8B 13 6A 01 8B CB")); // 0x0069E690 // fixes pause menu pop up, but breaks game image
        dword_69E690 = (uintptr_t)hook::get_pattern("8B DE 8B 13 6A 01 8B CB"); // so instead
        dword_66AEDC = (uintptr_t)hook::get_pattern("8B 8E F0 00 00 00 8B 06 51"); // I'll check for two callers
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("B0 01 E9 ? ? ? ? 8B 46 04")); // 0x006C1005
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("FF 50 04 8B 4C 24 20 5F 5E 5D 5B", 3)); // 0x006C14B2
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("89 45 EC 33 C0 66 A1")); // 0x006CA8B6
        //S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("84 C0 0F 84 ? ? ? ? 80 3E")); // 0x006CCCF2 - Causes black screen in  "Op. Challenger - Agent" cutscene
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("E8 ? ? ? ? 8B C8 E8 ? ? ? ? 8D 54 24 2C")); // 0x006D3F7A
        //S_matrix_Identity_Xrefs.push_back(0x0040200E); //0040200E
        //S_matrix_Identity_Xrefs.push_back(0x00464E2C); //00464E2C
        //S_matrix_Identity_Xrefs.push_back(0x0046A85A); //0046A85A
        //S_matrix_Identity_Xrefs.push_back(0x0048DB15); //0048DB15
        //S_matrix_Identity_Xrefs.push_back(0x0048DFED); //0048DFED
        //S_matrix_Identity_Xrefs.push_back(0x006C1005); //006C1005
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("8D 8E A0 B3 06 00 5E E9")); //0049F5EF //hud on load
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("8B 4F 14 85 C9 ? ? 8B 01 6A 01 FF 10 6A 10 68 40 02 00 00")); //006C2912
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("E9 ? ? ? ? 33 F6 83 C9 FF 89 77 14")); //006C29ED
        S_matrix_Identity_Xrefs.push_back((uintptr_t)hook::get_pattern("8B 0D ? ? ? ? E8 ? ? ? ? C6 00 00 C7 47 24")); //006C3F5A

        mpattern = hook::module_pattern(GetModuleHandle(L"LS3DF"), "E8 ? ? ? ? 8B 43 40 8B 4B 50 8B 53 60 89 06"); //(DWORD)GetModuleHandle(L"LS3DF") + 0x0A268
        injector::MakeCALL(mpattern.get_first(0), S_matrix_Identity, true);

        pattern = hook::pattern("88 46 72 88 46 70 5F"); //0x644B64
        struct TextHook
        {
            void operator()(injector::reg_pack& regs)
            {
                *(uint8_t*)(regs.esi + 0x72) = 1;
                *(uint8_t*)(regs.esi + 0x70) = 1;

                *(float *)(regs.esi + 0x08) *= Screen.fHudScale;
                *(float *)(regs.esi + 0x10) *= Screen.fHudScale;
                *(float *)(regs.esi + 0x18) *= Screen.fHudScale;

                *(float *)(regs.esi + 0x08) += Screen.fHudOffset;
                *(float *)(regs.esi + 0x10) += Screen.fHudOffset;
                //*(float *)(regs.esi + 0x18) += Screen.fHudOffset;
            }
        }; injector::MakeInline<TextHook>(pattern.get_first(0), pattern.get_first(6));

        static auto curs_pos_y = *(float**)hook::get_pattern("D9 05 ? ? ? ? D8 71 28 8B 44 24 04 D9 E0", 2); //0x66A120 //0x840664
        static auto curs_pos_x = curs_pos_y - 1; //0x840660
        pattern = hook::pattern("C7 44 24 28 00 00 00 00 52"); //0x66A801
        struct MouseFix
        {
            void operator()(injector::reg_pack& regs)
            {
                *(float*)(regs.esp + 0x28) = 0.0f;

                *(float*)(regs.esp + 0x38 + 0x00) = (*curs_pos_x - Screen.fHudOffset) / Screen.fWidth43;
                *(float*)(regs.esp + 0x38 + 0x04) = (*curs_pos_y / Screen.fHeight) * -0.75f;
                *(float*)(regs.esp + 0x38 + 0x08) = -1.0f;
            }
        }; injector::MakeInline<MouseFix>(pattern.get_first(0), pattern.get_first(8));

        pattern = hook::pattern("8B F1 8B 46 04 85 C0 74 5D"); //0x4AD441
        struct BloodyMoviesFix
        {
            void operator()(injector::reg_pack& regs)
            {
                regs.esi = regs.ecx;
                regs.eax = *(uint32_t*)(regs.esi + 0x04);

                *(uint32_t*)(regs.esp + 0x08) = 0;
            }
        }; injector::MakeInline<BloodyMoviesFix>(pattern.get_first(0));
    }

    if (bInsertKey)
    {
        pattern = hook::pattern("8B C7 5F F7 D8 1B C0 5E 25 ? ? ? ? C3"); //0x731C30
        struct InsertKey
        {
            void operator()(injector::reg_pack& regs)
            {
                static constexpr auto key = "1234-5678-9abc-dddf";
                if (!regs.eax)
                    regs.eax = (uint32_t)key;
            }
        }; injector::MakeInline<InsertKey>(pattern.get_first(13));
        injector::MakeRET(pattern.get_first(13 + 5));
    }
}

CEXP void InitializeASI()
{
    std::call_once(CallbackHandler::flag, []()
    {
        CallbackHandler::RegisterCallback(L"LS3DF.dll", Init);
        CallbackHandler::RegisterCallback(InitSettings, hook::pattern("0F ? ? ? ? ? 81 ? 80 02 00 00"));
    });
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {

    }
    return TRUE;
}