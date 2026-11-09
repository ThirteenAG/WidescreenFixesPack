module;

#include "stdafx.h"
#include "d3d9.h"

export module ComVars;

export struct bVector3
{
    float x;
    float y;
    float z;
};

export struct bVector4
{
    float x;
    float y;
    float z;
    float w;
};

export struct bMatrix4
{
    bVector4 v0;
    bVector4 v1;
    bVector4 v2;
    bVector4 v3;
};

export enum GameFlowState : int32_t
{
    GAMEFLOW_STATE_NONE = 0x0,
    GAMEFLOW_STATE_LOADING_FRONTEND = 0x1,
    GAMEFLOW_STATE_UNLOADING_FRONTEND = 0x2,
    GAMEFLOW_STATE_IN_FRONTEND = 0x3,
    GAMEFLOW_STATE_LOADING_REGION = 0x4,
    GAMEFLOW_STATE_LOADING_TRACK = 0x5,
    GAMEFLOW_STATE_RACING = 0x6,
    GAMEFLOW_STATE_UNLOADING_TRACK = 0x7,
    GAMEFLOW_STATE_UNLOADING_REGION = 0x8,
    GAMEFLOW_STATE_EXIT_DEMO_DISC = 0x9,
};

export struct GameFlowManager
{
    char unk[36];
    GameFlowState CurrentGameFlowState;

    static inline bool (*IsPaused)() = nullptr;
};

export namespace NFSPSDialog
{
    struct Configuration
    {
        void* table;
        uint8_t custom;
        uint8_t hide;
        uint16_t reserved;
        uint32_t title;
        uint8_t buttons;
        uint8_t padding[3];
        wchar_t* body;
        wchar_t* labels[3];
        uint32_t opacity;
        uint32_t context;
    };
    static_assert(sizeof(Configuration) == 40);
    static_assert(offsetof(Configuration, body) == 16);

}

export GameRef<NFSPSDialog::Configuration*> DialogPending;
export GameRef<void*> DialogInstance;
export GameRef<uint8_t> StaticTextStatePool;
export struct Binding
    {
        int32_t type;
        int32_t device;     // the key for keyboard bindings
        int32_t index;
    };

    // Every action has a list of slots: two keyboard slots, then two for each joystick
export struct ActionEntry
    {
        const char* name;
        Binding* slots;
    };



export struct TreeEntry
    {
        int32_t id;
        int32_t parent;
        uint32_t label;
        int32_t type;
        int32_t dalId;
    };
    static_assert(sizeof(TreeEntry) == 0x14);


export struct LanguageInfo
    {
        int32_t id;
        const char* name;
        const char* code;
        const char* files[4];
        uint32_t unknown[2];
    };
    static_assert(sizeof(LanguageInfo) == 0x24);

export GameRef<ActionEntry> Actions;
export GameRef<TreeEntry> OptionsTable;
export GameRef<TreeEntry> OptionsTableEnd;
export GameRef<LanguageInfo> Languages;
export GameRef<LanguageInfo> LanguagesEnd;
export GameRef<int32_t> NumJoysticks;
export GameRef<uintptr_t> Joysticks;
export GameRef<uintptr_t> TheICEManager;
export GameRef<int32_t> GammaInteger;
export GameRef<int32_t> RealTimer;
export GameRef<int32_t> SimActive;
export GameRef<float> TimeBank;
export GameRef<int32_t> WindowWidth;
export GameRef<int32_t> WindowHeight;
export GameRef<int32_t> CurrentLanguage;
export GameRef<uint8_t> FastMem;
export GameRef<uintptr_t> OnlineActiveGameManager;
export GameRef<uint8_t*> DefaultsMainContext;
export GameRef<uint8_t*> DefaultsPauseContext;
export GameRef<void*> PackageManager;
export GameRef<uint8_t*> ActiveCharset;
export GameRef<uint32_t> RawCharset;
export GameRef<double> OriginalHorizontalFov;
export GameRef<double> OriginalVerticalFov;
export GameRef<double> OriginalBoostAdder;
export GameRef<double> OriginalBoostSubtractor;
export GameRef<bool> twkEnableTSM;

export struct RegistryName
{
    const char* name;
    int32_t id;
    int32_t region;
};
export GameRef<RegistryName> RegistryNames;
export GameRef<LanguageInfo> RegistryLanguageBegin;
export GameRef<LanguageInfo> RegistryLanguageEnd;


export int SimRate = 60;

export GameRef<HWND> hWnd;
export GameRef<uint32_t> dwWindowedMode;
export GameRef<IDirect3DDevice9**> Direct3DDevice;
export GameRef<int> ResX;
export GameRef<int> ResY;
export GameRef<GameFlowManager> TheGameFlowManager;

// The resizable window: the picture keeps the game's resolution and is scaled to the window, so the aspect ratio
// follows the window
export bool bAspectFromWindow = false;

// The aspect ratio of the picture on the screen
export float ScreenAspectRatio()
{
    if (bAspectFromWindow)
    {
        RECT rect{};
        if (auto window = hWnd.get(); window && GetClientRect(window, &rect) && rect.right > 0 && rect.bottom > 0)
            return static_cast<float>(rect.right) / static_cast<float>(rect.bottom);
    }
    if (ResX <= 0 || ResY <= 0)
        return 16.0f / 9.0f;
    return static_cast<float>(ResX) / static_cast<float>(ResY);
}

export bool IsFrontEnd()
{
    return TheGameFlowManager->CurrentGameFlowState == GAMEFLOW_STATE_IN_FRONTEND;
}

export bool IsRacing()
{
    return TheGameFlowManager->CurrentGameFlowState == GAMEFLOW_STATE_RACING;
}

export namespace Sim
{
    namespace Internal
    {
        export GameRef<float> mLastFrameTime;
    }
}

export uint32_t bStringHash(const char* str)
{
    if (!str)
        return uint32_t(-1);

    uint32_t result = uint32_t(-1);
    for (; *str; ++str)
        result = uint8_t(*str) + 33 * result;
    return result;
}

template<typename... Args>
class WindowMessage : public WFP::Event<Args...>
{
public:
    using WFP::Event<Args...>::Event;
};

// Every message of the game window, before the game handles it
export __declspec(noinline) WindowMessage<HWND, UINT, WPARAM, LPARAM>& onWindowMessage()
{
    static WindowMessage<HWND, UINT, WPARAM, LPARAM> WindowMessageEvent;
    return WindowMessageEvent;
}

// Front end objects
struct FEMinNode
{
    void* __vftable;
    FEMinNode* next;
    FEMinNode* prev;
};

struct FEMinList
{
    void* __vftable;
    uint32_t numElements;
    FEMinNode* head;
    FEMinNode* tail;
};

export enum FEObjType : int32_t
{
    FE_None = 0x0,
    FE_Image = 0x1,
    FE_String = 0x2,
    FE_Model = 0x3,
    FE_List = 0x4,
    FE_Group = 0x5,
    FE_CodeList = 0x6,
    FE_Movie = 0x7,
    FE_Effect = 0x8,
    FE_ColoredImage = 0x9,
    FE_AnimImage = 0xA,
    FE_SimpleImage = 0xB,
    FE_MultiImage = 0xC,
    FE_UserMin = 0x100,
};

export struct FEObject : FEMinNode
{
    uint32_t GUID;
    uint32_t NameHash;
    char* pName;
    FEObjType Type;
    uint32_t Flags;
    uint16_t RenderContext;
    uint16_t ResourceIndex;
    uint32_t Handle;
    uint32_t UserParam;
    uint8_t* pData;
    uint32_t DataSize;
    FEMinList Responses;
    FEMinList Scripts;
    void* pCurrentScript;
    void* Cached;
};
static_assert(sizeof(FEObject) == 0x5C);

export namespace cFEng
{
    // cFEng { FEngine* mFEng; bool bWasPaused; }
    GameRef<void*> mInstance;

    // Calls the callback for every object of every loaded package, groups included
    void ForAllObjects(const std::function<void(FEObject*)>& callback)
    {
        if (!mInstance.is_initialized() || !mInstance.get())
            return;

        auto engine = *static_cast<uint8_t**>(mInstance.get());
        if (!engine)
            return;

        std::function<void(FEMinNode*)> visit = [&](FEMinNode* node)
        {
            for (int guard = 0; node && guard < 4096; node = node->next, ++guard)
            {
                auto object = static_cast<FEObject*>(node);
                callback(object);
                if (object->Type == FE_Group)
                    visit(reinterpret_cast<FEMinList*>(reinterpret_cast<uint8_t*>(object) + 0x5C)->head);
            }
        };

        // FEngine::PackList at +0xD8, FEPackage::Objects at +0x44
        auto package = reinterpret_cast<FEMinList*>(engine + 0xD8)->head;
        for (int guard = 0; package && guard < 256; package = package->next, ++guard)
            visit(reinterpret_cast<FEMinList*>(reinterpret_cast<uint8_t*>(package) + 0x44)->head);
    }
}
