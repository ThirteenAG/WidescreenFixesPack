module;

#include <stdafx.h>

export module DInputMapper;

// The controller mapping dialog's COM classes (diactfrm.dll next to the game), activated from the manifest resource
namespace TCDInputActivation
{
    constexpr WORD Resource = 5001;

    HANDLE Create(HMODULE module, const std::wstring& directory)
    {
        if (!module || directory.empty()) return INVALID_HANDLE_VALUE;
        wchar_t source[4096]{};
        auto length = GetModuleFileNameW(module, source, DWORD(std::size(source)));
        if (!length || length >= std::size(source)) return INVALID_HANDLE_VALUE;
        ACTCTXW description{};
        description.cbSize = sizeof(description);
        description.dwFlags = ACTCTX_FLAG_HMODULE_VALID | ACTCTX_FLAG_RESOURCE_NAME_VALID | ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID;
        description.lpSource = source;
        description.lpAssemblyDirectory = directory.c_str();
        description.lpResourceName = MAKEINTRESOURCEW(Resource);
        description.hModule = module;
        return CreateActCtxW(&description);
    }

    HANDLE ProcessContext()
    {
        static HANDLE context = INVALID_HANDLE_VALUE;
        static std::once_flag once;
        std::call_once(once, []
        {
            HMODULE module = nullptr;
            wchar_t executable[4096]{};
            auto length = GetModuleFileNameW(nullptr, executable, DWORD(std::size(executable)));
            if (!length || length >= std::size(executable) ||
                !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(&ProcessContext), &module)) return;
            context = Create(module, std::filesystem::path(executable).parent_path().wstring());
            if (context == INVALID_HANDLE_VALUE)
                OutputDebugStringA("TCNYC: controller mapping activation context unavailable.\n");
        });
        return context;
    }

    class Scope
    {
        ULONG_PTR cookie = 0;
        bool active = false;

    public:
        explicit Scope(HANDLE context) : active(context != INVALID_HANDLE_VALUE && ActivateActCtx(context, &cookie))
        {
        }
        Scope() : Scope(ProcessContext())
        {
        }
        ~Scope()
        {
            if (active) DeactivateActCtx(0, cookie);
        }
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
        bool Active() const
        {
            return active;
        }
    };
}

SafetyHookInline configureHook;
int32_t __fastcall ConfigureDevices(void* self, void*, HWND parent, void* target, void* callback, DWORD flags, void* reference)
{
    TCDInputActivation::Scope activation;
    return configureHook.unsafe_thiscall<int32_t>(self, parent, target, callback, flags, reference);
}

class DInputMapper
{
public:
    DInputMapper()
    {
        WFP::onInitEvent() += []()
        {
            // The native wrapper calls IDirectInput8::ConfigureDevices and returns
            // through one RET20 path. Scope COM redirection to that call only.
            auto pattern = hook::pattern("83 EC 44 53 55 8B 2D ? ? ? ? 56 57 8B F1 33 FF 33 DB 39 7E 14 76 20");
            configureHook = safetyhook::create_inline(pattern.get_first(), ConfigureDevices);
        };
    }
} DInputMapper;
