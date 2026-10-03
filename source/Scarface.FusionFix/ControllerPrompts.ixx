module;
#include "stdafx.h"
#include "RTTI.h"
#include "ButtonIcons.h"
#include <atomic>
#include <mutex>
#include <map>

export module ControllerPrompts;

namespace
{
    using Microsoft::WRL::ComPtr;
    struct Token { const char* name; int slot; int physical; std::wstring keyboard; wchar_t marker[2]{}; int secondPhysical = -1; };
    // Logical slots in Scarface's selected controller description, not XInput button numbers.
    Token tokens[] = {
        {"BUTTON_GRAPHIC_X",5,-1},{"BUTTON_GRAPHIC_CIRCLE",6,-1},{"BUTTON_GRAPHIC_TRIANGLE",7,-1},
        {"BUTTON_GRAPHIC_SQUARE",8,-1},{"BUTTON_GRAPHIC_R1",9,-1},{"BUTTON_GRAPHIC_R2",10,-1},
        {"BUTTON_GRAPHIC_R3",11,-1},{"BUTTON_GRAPHIC_L1",12,-1},{"BUTTON_GRAPHIC_L2",13,-1},
        {"BUTTON_GRAPHIC_L3",14,-1},{"BUTTON_GRAPHIC_START",15,-1},{"BUTTON_GRAPHIC_SELECT",16,-1},
        {"BUTTON_GRAPHIC_DPAD_UP",-1,4},{"BUTTON_GRAPHIC_DPAD_DOWN",-1,5},
        {"BUTTON_GRAPHIC_DPAD_LEFT",-1,6},{"BUTTON_GRAPHIC_DPAD_RIGHT",-1,7},
        {"BUTTON_GRAPHIC_LEFT_STICK",-1,0},{"BUTTON_GRAPHIC_RIGHT_STICK",-1,2},
        {"BUTTON_GRAPHIC_L",-1,0},{"BUTTON_GRAPHIC_R",-1,2},
        {"BUTTON_GRAPHIC_DPAD_HORIZONTAL",-1,6,{}, {},7},
        {"BUTTON_GRAPHIC_DPAD_VERTICAL",-1,4,{}, {},5},
        // PC/PS2 text-bible entries for the same tutorial identify these equivalents.
        {"BUTTON_GRAPHIC_2KEY",-1,7},{"BUTTON_GRAPHIC_3KEY",-1,5},
        {"BUTTON_GRAPHIC_EKEY",7,-1},{"BUTTON_GRAPHIC_VKEY",6,-1},
        {"BUTTON_GRAPHIC_ZKEY",10,-1},{"BUTTON_GRAPHIC_LSHIFT",5,-1},
        {"BUTTON_GRAPHIC_LMB",9,-1},{"BUTTON_GRAPHIC_MOUSE",-1,0},
        {"BUTTON_GRAPHIC_SNIPER_ZOOM",-1,4,{}, {},5},
        {"BUTTON_TEXT_X",5,-1},{"BUTTON_TEXT_CIRCLE",6,-1},
        {"BUTTON_TEXT_SQUARE",8,-1},{"BUTTON_TEXT_TRIANGLE",7,-1},
        {"BUTTON_TEXT_L1",12,-1},{"BUTTON_TEXT_R1",9,-1},{"BUTTON_TEXT_R2",10,-1},
        {"BUTTON_TEXT_L3_PRESS",14,-1},{"BUTTON_TEXT_LEFT_ANALOG",-1,0},
        {"BUTTON_TEXT_RIGHT_ANALOG",-1,2},{"BUTTON_TEXT_DPAD_HORIZONTAL",-1,0},
        {"BUTTON_TEXT_RIGHT",-1,7},{"BUTTON_TEXT_UP",5,-1},
        {"BUTTON_TEXT_ZKEY",10,-1},{"BUTTON_TEXT_WEAPON_DESELECT",-1,5},
        {"BUTTON_TEXT_WEAPON_SELECT",-1,6,{}, {},7},
        {"BUTTON_TEXT_SNIPER_ZOOM",-1,4,{}, {},5},
        // ControllerConfigActionMap binds Start to PauseMenuAction(0), just like Esc.
        {nullptr,15,-1,L"ESC"},
        // Context-specific variants: map marker, map zoom, and the sniper modifier.
        {nullptr,10,-1},{nullptr,-1,2},{nullptr,12,-1}
    };
    constexpr wchar_t markerBase = 0xE100;
    constexpr size_t controllerExitToken = std::size(tokens)-4;
    constexpr wchar_t glyphs[20] = {0xAB,0xAB,0xAC,0xAC,0xA4,0xA5,0xA6,0xA7,0xAF,0xB1,0xBC,0xBE,0xBD,0xB9,0xB2,0xB3,0xA3,0xA2,0xB4,0xB5};
    constexpr int pagePhysical[24] = {-1,-1,17,16,4,5,6,7,-1,-1,0,2,8,9,14,15,18,19,-1,-1,13,10,12,11};
    SafetyHookInline lookupHook, widthHook, lineWidthHook, wrapWidthHook, maskHook, drawHook, wrapDrawHook, beginHook, endHook;
    SafetyHookInline setTextHook;
    std::mutex tokenMutex;
    std::map<std::pair<uintptr_t,int>,std::wstring> expandedBible;
    const wchar_t* (__cdecl* lookupText)(const char*) = nullptr;
    bool enabled = false;
    std::atomic<bool> gamepad{false};
    std::atomic<uint64_t> lastKeyboardActivity{0};
    unsigned controller = 0;
    uintptr_t* controllerDescription = nullptr;
    using QueryButton = uint32_t(__cdecl*)(unsigned,unsigned);
    QueryButton queryButton = nullptr;
    using QueryActivity = uint64_t(__cdecl*)(unsigned);
    QueryActivity queryActivity = nullptr;
    IDirect3DDevice9* device = nullptr;
    std::array<ComPtr<IDirect3DTexture9>,20> icons;
    ComPtr<IDirect3DBaseTexture9> savedTexture;
    bool textureChanged = false;
    std::array<int,17> bindingPhysical{};
    injector::hook_back<const char* (__cdecl*)(const char*,int)> hbControlButtonLabel;

    int __fastcall SetText(void* self,void*,const char* text,int index,int literal)
    {
        // FETextObject::SetText clears strings without a text-bible substitution
        // unless its literal flag is set. Only our binding labels are literal.
        for(const auto name:ScarfaceButtons::Names)
            if(text==name){literal=1;break;}
        return setTextHook.thiscall<int>(self,text,index,literal);
    }

    const char* __cdecl ControlButtonLabel(const char* format,int index)
    {
        // win32_controller.cso's physicalindex enumerates logical controls, not
        // DirectInput button numbers: X, Triangle, Square, Circle, R1/R2/R3, L1/L2/L3.
        constexpr int slots[]={5,7,8,6,9,10,11,12,13,14};
        if(queryButton && index>=0 && index<int(std::size(slots)))
        {
            const int physical=bindingPhysical[slots[index]];
            if(physical>=0 && physical<int(std::size(ScarfaceButtons::Names)))
                return ScarfaceButtons::Names[physical];
        }
        return hbControlButtonLabel.fun(format,index);
    }

    uint32_t Key(std::string_view text)
    {
        uint32_t hash=0;
        for(unsigned char c:text)hash=((65599u*hash)&0x7FFFFFFF)^(c<97?c+32:c);
        return hash|0x80000000;
    }

    const wchar_t* __fastcall Lookup(void* self,void*,int index)
    {
        const auto original = lookupHook.thiscall<const wchar_t*>(self,index);
        if (!original || index<0) return original;
        const auto keys=*reinterpret_cast<const uint32_t**>(static_cast<uint8_t*>(self)+24);
        if(keys[index]==Key("WIN32_PAUSE_CONTROLLER_EXIT"))
        {
            // Keep the localized suffix; reserve a marker so an already-open
            // page follows input and Xidi profile changes without being rebuilt.
            std::wstring result=original;
            for(size_t i=0;i+3<=result.size();++i)
                if(_wcsnicmp(result.c_str()+i,L"ESC",3)==0)
                {
                    std::scoped_lock lock(tokenMutex);
                    tokens[controllerExitToken].keyboard=result.substr(i,3);
                    result.replace(i,3,tokens[controllerExitToken].marker);
                    auto& cached=expandedBible[{reinterpret_cast<uintptr_t>(self),index}];
                    if(cached!=result)cached=std::move(result);
                    return cached.c_str();
                }
            return original;
        }
        for (auto& token : tokens)
        {
            if (!token.name || Key(token.name)!=keys[index]) continue;
            std::scoped_lock lock(tokenMutex);
            token.keyboard = original;
            return token.marker;
        }
        // Resolve embedded tokens before the game's one-level substitution. Keep markers in
        // cached FE strings so input/profile changes need no UI recreation or translated prose.
        if (!wcschr(original,L'[')) return original;
        std::wstring result=original;
        bool changed=false;
        for (size_t i=0;i<std::size(tokens)-3;++i)
        {
            auto& token=tokens[i];
            if(!token.name)continue;
            std::wstring name=L"[";
            for(auto p=token.name;*p;++p)name+=wchar_t(*p);
            name+=L']';
            if(result.find(name)==std::wstring::npos)continue;
            if(!lookupText(token.name))continue;
            size_t replacement=i;
            if(keys[index]==0x88D871B9 && strcmp(token.name,"BUTTON_GRAPHIC_LMB")==0)
                replacement=std::size(tokens)-3;
            if(strcmp(token.name,"BUTTON_GRAPHIC_SNIPER_ZOOM")==0)
            {
                if(keys[index]==0xDFED7AFE)replacement=std::size(tokens)-2;
                if(keys[index]==0xCBF8481D)replacement=std::size(tokens)-1;
            }
            std::scoped_lock lock(tokenMutex);
            if(replacement!=i)tokens[replacement].keyboard=token.keyboard;
            size_t pos=0;
            while((pos=result.find(name,pos))!=std::wstring::npos)
            {
                result.replace(pos,name.size(),tokens[replacement].marker);
                ++pos;
            }
            changed=true;
        }
        if(!changed)return original;
        std::scoped_lock lock(tokenMutex);
        auto& cached=expandedBible[{reinterpret_cast<uintptr_t>(self),index}];
        if(cached!=result)cached=std::move(result);
        return cached.c_str();
    }

    bool HasIconGlyph(void* font,int physical)
    {
        const auto object=static_cast<const uint8_t*>(font);
        const int count=*reinterpret_cast<const int*>(object+52);
        const auto codes=*reinterpret_cast<const uint16_t* const*>(object+60);
        const auto data=*reinterpret_cast<const uint8_t* const*>(object+56);
        if(!codes || !data || count<=0)return false;
        const auto found=std::lower_bound(codes,codes+count,uint16_t(glyphs[physical]));
        if(found==codes+count || *found!=glyphs[physical])return false;
        const auto page=*reinterpret_cast<const uint32_t*>(data+(found-codes)*48+20);
        return (page&0x80000000) && (page&0x7FFFFFFF)<std::size(pagePhysical) &&
            pagePhysical[page&0x7FFFFFFF]==physical;
    }

    std::wstring Expand(void* font,const wchar_t* text)
    {
        if (!text) return {};
        std::wstring output;
        std::scoped_lock lock(tokenMutex);
        for (; *text; ++text)
        {
            const unsigned index = unsigned(*text)-markerBase;
            if(index>=std::size(tokens)) {output+=*text;continue;}
            const auto& token=tokens[index];
            int physical = token.slot>=0 ? bindingPhysical[token.slot] : token.physical;
            if(gamepad.load(std::memory_order_relaxed) && physical>=0 && physical<20 && icons[physical] &&
                HasIconGlyph(font,physical) && (token.secondPhysical<0 ||
                    (icons[token.secondPhysical] && HasIconGlyph(font,token.secondPhysical))))
            {
                output+=glyphs[physical];
                if(token.secondPhysical>=0){output+=L'/';output+=glyphs[token.secondPhysical];}
            }
            else output+=token.keyboard;
        }
        return output;
    }

    float __fastcall Width(void* self,void*,const wchar_t* text)
    {auto s=Expand(self,text);return widthHook.thiscall<float>(self,s.c_str());}
    float __fastcall LineWidth(void* self,void*,const wchar_t* text)
    {auto s=Expand(self,text);return lineWidthHook.thiscall<float>(self,s.c_str());}
    float __fastcall WrapWidth(void* self,void*,const wchar_t* text,float width,float scale)
    {auto s=Expand(self,text);return wrapWidthHook.thiscall<float>(self,s.c_str(),width,scale);}
    int __fastcall Mask(void* self,void*,const wchar_t* text)
    {
        // FE dialogs cache this mask when their text is assigned. Reserve the keyboard
        // pages and every supported controller page so input/profile changes can draw
        // new glyphs without recreating the dialog (or losing its selection).
        std::wstring s;
        bool hasPrompt=false;
        {
            std::scoped_lock lock(tokenMutex);
            if(text)for(auto p=text;*p;++p)
            {
                const unsigned index=unsigned(*p)-markerBase;
                if(index<std::size(tokens))
                {
                    s+=tokens[index].keyboard;
                    hasPrompt=true;
                }
                else s+=*p;
            }
        }
        if(hasPrompt)for(int physical=0;physical<int(std::size(glyphs));++physical)
            if(HasIconGlyph(self,physical))s+=glyphs[physical];
        return maskHook.thiscall<int>(self,s.c_str());
    }
    void __fastcall Draw(void* self,void*,int primitive,int page,const wchar_t* text,int color,float x,float y,float scale)
    {auto s=Expand(self,text);drawHook.thiscall<void>(self,primitive,page,s.c_str(),color,x,y,scale);}
    const wchar_t* __fastcall WrapDraw(void* self,void*,int primitive,int page,const wchar_t* text,int color,float x,float y,float width,int align,float scale)
    {
        auto s=Expand(self,text);
        wrapDrawHook.thiscall<const wchar_t*>(self,primitive,page,s.c_str(),color,x,y,width,align,scale);
        // The original returns the terminating character, never a pointer into our temporary.
        return text ? text+wcslen(text) : nullptr;
    }

    int __fastcall Begin(void* self,void*,int page)
    {
        auto result=beginHook.thiscall<int>(self,page);
        if(device && gamepad.load(std::memory_order_relaxed) && page>=0 && page<int(std::size(pagePhysical)))
        {
            const int id=pagePhysical[page];
            if(id>=0 && icons[id] && HasIconGlyph(self,id) && SUCCEEDED(device->GetTexture(0,savedTexture.ReleaseAndGetAddressOf())))
                textureChanged=SUCCEEDED(device->SetTexture(0,icons[id].Get()));
        }
        return result;
    }
    int __fastcall End(void* self,void*,int primitive)
    {
        // TextureFont::End tail-jumps to the rendering context's EndPrims. Its
        // primitive argument remains on the stack and must be forwarded/consumed.
        auto result=endHook.thiscall<int>(self,primitive);
        if(textureChanged && device)device->SetTexture(0,savedTexture.Get());
        savedTexture.Reset();textureChanged=false;return result;
    }

    void UpdateInputMode()
    {
        // Xidi owns both device detection and profile mappings. Its indices match
        // queryButton's indices even when the backend reads a DS4 or another non-XInput pad.
        uint64_t latestActivity=0;
        if(queryActivity)for(unsigned i=0;i<4;++i)
        {
            const auto activity=queryActivity(i);
            if(activity>latestActivity)
            {
                latestActivity=activity;
                controller=i;
            }
        }
        gamepad.store(latestActivity>lastKeyboardActivity.load(std::memory_order_relaxed),std::memory_order_relaxed);
    }

}

export namespace ControllerPrompts
{
    bool IsEnabled(){return enabled;}
    void OnMessage(HWND window,UINT message,WPARAM,LPARAM param)
    {
        if(!enabled)return;
        static bool registered=false;
        if(!registered)
        {
            RAWINPUTDEVICE keyboard{1,6,0,window};
            registered=RegisterRawInputDevices(&keyboard,1,sizeof(keyboard))!=FALSE;
        }
        if(message!=WM_INPUT || GetForegroundWindow()!=window)return;
        RAWINPUT raw{};UINT size=sizeof(raw);
        if(GetRawInputData(reinterpret_cast<HRAWINPUT>(param),RID_INPUT,&raw,&size,sizeof(RAWINPUTHEADER))==UINT(-1) || !raw.header.hDevice)return;
        bool activity=false;
        if(raw.header.dwType==RIM_TYPEKEYBOARD)activity=!(raw.data.keyboard.Flags&RI_KEY_BREAK);
        if(raw.header.dwType==RIM_TYPEMOUSE)
            activity=raw.data.mouse.lLastX||raw.data.mouse.lLastY||raw.data.mouse.usButtonFlags;
        if(activity)lastKeyboardActivity.store(GetTickCount64(),std::memory_order_relaxed);
    }

    void Update(IDirect3DDevice9* d)
    {
        if(!enabled||!d)return;
        if(device!=d)
        {
            for(auto& icon:icons)icon.Reset();device=d;
            for(unsigned i=0;i<icons.size();++i)icons[i]=ScarfaceButtons::CreateIcon(d,i);
        }
        if(!queryButton || !queryActivity)
            if(auto module=GetModuleHandleW(L"Xidi.32.dll"))
            {
                queryButton=reinterpret_cast<QueryButton>(GetProcAddress(module,"XidiGetPhysicalButtonMask"));
                queryActivity=reinterpret_cast<QueryActivity>(GetProcAddress(module,"XidiGetLastControllerActivity"));
            }
        DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
        if(foregroundPid!=GetCurrentProcessId())return;
        UpdateInputMode();
        bindingPhysical.fill(-1);
        if(queryButton && controllerDescription && *controllerDescription)
        {
            auto names=*reinterpret_cast<const char***>(*controllerDescription+4);
            if(names)for(int slot=5;slot<17;++slot)
            {
                const char* name=names[slot];int button=-1;
                if(name && (sscanf_s(name,"Button %d",&button)==1 || sscanf_s(name,"Button_%d",&button)==1) && button>=1 && button<=16)
                {
                    // Xidi's DirectInput object names are one-based ("Button 1"),
                    // while XidiGetPhysicalButtonMask takes a zero-based index.
                    const auto mask=queryButton(controller,unsigned(button-1));
                    // An ambiguous binding must not pretend that one particular button is required.
                    if(mask && !(mask&(mask-1)))bindingPhysical[slot]=int(std::countr_zero(mask));
                }
            }
        }
    }

    void Initialize()
    {
        CIniReader iniReader("");
        const bool bControllerPrompts=iniReader.ReadInteger("MAIN","ControllerPrompts",1)!=0;
        if(!bControllerPrompts)return;
        auto font=ScarfaceRTTI::FindVtable(".?AVTextureFont@pure3d@@");
        auto pattern=hook::pattern("8B 44 24 04 83 F8 FF 74 11 8B 51 1C 8B 04 82 8B 49 20 D1 E8 8D 04 41 C2 04 00"); // 0x6A41E0
        if(!font || pattern.size()!=1)return;
        auto lookup=pattern.get_first();
        pattern=hook::pattern("55 8B 6C 24 08 56 57 BF ? ? ? ? 8B F5 B9 01 00 00 00 33 C0 F3 A6"); // 0x6783A0
        if(pattern.size()!=1)return;
        lookupText=reinterpret_cast<decltype(lookupText)>(pattern.get_first());
        pattern=hook::pattern("83 3D ? ? ? ? 00 55 8B E9 74 ? 83 3D ? ? ? ? 00 74 ? 53 8B 1D"); // 0x42EC60 + 14
        if(pattern.size()!=1)return;
        controllerDescription=*pattern.get_first<uintptr_t*>(14);
        for(unsigned i=0;i<std::size(tokens);++i)tokens[i].marker[0]=markerBase+wchar_t(i);
        bindingPhysical.fill(-1);
        auto slot=[font](unsigned i){return injector::ReadMemory<void*>(font+i*sizeof(void*),true);};
        widthHook=safetyhook::create_inline(slot(13),Width);
        lineWidthHook=safetyhook::create_inline(slot(26),LineWidth);
        wrapWidthHook=safetyhook::create_inline(slot(11),WrapWidth);
        maskHook=safetyhook::create_inline(slot(19),Mask);
        drawHook=safetyhook::create_inline(slot(16),Draw);
        wrapDrawHook=safetyhook::create_inline(slot(15),WrapDraw);
        beginHook=safetyhook::create_inline(slot(14),Begin);
        endHook=safetyhook::create_inline(slot(17),End);
        if(widthHook&&lineWidthHook&&wrapWidthHook&&maskHook&&drawHook&&wrapDrawHook&&beginHook&&endHook)
            lookupHook=safetyhook::create_inline(lookup,Lookup);
        enabled=bool(lookupHook);
        if(enabled)
        {
            auto text=ScarfaceRTTI::FindVtable(".?AVFETextObject@@");
            pattern=hook::pattern("53 68 ? ? ? ? E8 ? ? ? ? 83 C4 08 EB 05 B8 ? ? ? ? 8B 4E 38"); // 0x5E27D4 + 6
            if(text && pattern.size()==1)
            {
                setTextHook=safetyhook::create_inline(injector::ReadMemory<void*>(text+39*sizeof(void*),true),SetText);
                if(setTextHook)
                    hbControlButtonLabel.fun=injector::MakeCALL(pattern.get_first(6),ControlButtonLabel,true).get();
            }
        }
    }
    void Shutdown()
    {
        lookupHook.reset();widthHook.reset();lineWidthHook.reset();wrapWidthHook.reset();maskHook.reset();
        drawHook.reset();wrapDrawHook.reset();beginHook.reset();endHook.reset();
        setTextHook.reset();
        savedTexture.Reset();for(auto& icon:icons)icon.Reset();device=nullptr;
        queryActivity=nullptr;queryButton=nullptr;enabled=false;
    }
}
