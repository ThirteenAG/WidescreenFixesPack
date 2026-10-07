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
    struct Token { const char* name; int slot; int physical; std::wstring keyboard; wchar_t marker[2]{}; int secondPhysical = -1; int gameplayContext = -1; };
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
        {nullptr,10,-1},{nullptr,-1,2},{nullptr,12,-1},
        // Vehicle-only PC tokens have different actions from the same keys on foot.
        {nullptr,5,-1,{}, {},-1,1},{nullptr,8,-1,{}, {},-1,1},{nullptr,14,-1,{}, {},-1,1},
        // Context is independent of the paused menu and of MainCharacter's lifetime.
        // 0 = OnFoot, 1 = InCar. Keep the original keyboard text on each variant.
        {nullptr,6,-1,{}, {},-1,1},{nullptr,12,-1,{}, {},-1,0},{nullptr,9,-1,{}, {},-1,1}
    };
    constexpr wchar_t markerBase = 0xE100;
    constexpr wchar_t gameplayMarkerBase = 0xE200;
    constexpr size_t controllerExitToken = std::size(tokens)-10;
    constexpr size_t mapMarkerToken = controllerExitToken+1, mapZoomToken = controllerExitToken+2;
    constexpr size_t sniperModifierToken = controllerExitToken+3;
    constexpr size_t accelerateToken = controllerExitToken+4, brakeToken = controllerExitToken+5;
    constexpr size_t hornToken = controllerExitToken+6;
    constexpr size_t handbrakeToken = controllerExitToken+7, lockOnToken = controllerExitToken+8;
    constexpr size_t hardTurnToken = controllerExitToken+9;
    constexpr wchar_t glyphs[20] = {0xAB,0xAB,0xAC,0xAC,0xA4,0xA5,0xA6,0xA7,0xAF,0xB1,0xBC,0xBE,0xBD,0xB9,0xB2,0xB3,0xA3,0xA2,0xB4,0xB5};
    constexpr int pagePhysical[24] = {-1,-1,17,16,4,5,6,7,-1,-1,0,2,8,9,14,15,18,19,-1,-1,13,10,12,11};
    SafetyHookInline lookupHook, widthHook, lineWidthHook, wrapWidthHook, maskHook, drawHook, wrapDrawHook, beginHook, endHook;
    SafetyHookInline setTextHook;
    SafetyHookInline glyphHook;
    SafetyHookInline lookTutorialUpdateHook;
    const char* (__cdecl* evaluateScript)(const char*,int,const char*,int,int) = nullptr;
    void* boundTutorialMap = nullptr;
    std::mutex tokenMutex;
    std::map<std::pair<uintptr_t,int>,std::wstring> expandedBible;
    const wchar_t* (__cdecl* lookupText)(const char*) = nullptr;
    bool enabled = false;
    unsigned gamepadIcons = 0;
    std::atomic<bool> gamepad{false};
    std::atomic<uint64_t> lastKeyboardActivity{0};
    unsigned controller = 0;
    uintptr_t* controllerDescription = nullptr;
    using QueryButton = uint32_t(__cdecl*)(unsigned,unsigned);
    QueryButton queryButton = nullptr;
    using QueryActivity = uint64_t(__cdecl*)(unsigned);
    QueryActivity queryActivity = nullptr;
    using QueryProfileButton = uint32_t(__cdecl*)(unsigned,unsigned,const wchar_t*);
    QueryProfileButton queryProfileButton = nullptr;
    void** mainCharacter = nullptr;
    bool modernControls = false;
    IDirect3DDevice9* device = nullptr;
    std::array<ComPtr<IDirect3DTexture9>,20> icons;
    std::array<float,20> iconHeightPerWidth{};
    ComPtr<IDirect3DBaseTexture9> savedTexture;
    bool textureChanged = false;
    int iconPage = -1;
    std::array<int,17> bindingPhysical{};
    std::array<int,17> gameplayPhysical{};
    std::array<std::array<int,17>,2> actionPhysical{};
    injector::hook_back<const char* (__cdecl*)(const char*,int)> hbControlButtonLabel;

    void __fastcall UpdateLookTutorial(void* self,void*,const void* time)
    {
        const auto state=static_cast<const uint8_t*>(self);
        // mActionMap is assigned by StartTutorial; don't read it while inactive.
        const int stage=*reinterpret_cast<const int*>(state+0x68);
        const bool active=(state[0x60]&8) && (stage==-1 || stage>0);
        auto map=active ? *reinterpret_cast<void* const*>(state+0x6C) : nullptr;
        if(!map)boundTutorialMap=nullptr;
        if(map && map!=boundTutorialMap)
        {
            // PC binds Enter to this action but omits the controller binding at
            // the initial stage (-1). Use the same native action/state transition.
            evaluateScript("HUDLookTutorialActionMap.bind(\"Joystick\", \"X\", \"LookTutorial_InputX\");",
                0,nullptr,0,-1);
            // tutorial.cso binds Start to Options on PC, but only binds the
            // Triangle exit action on consoles. Add that missing PC binding
            // through the game's own ActionMap; its Pressed(1) retains the
            // original stage, pause and opening-mission restrictions.
            evaluateScript("HUDLookTutorialActionMap.bindCmd(\"Joystick\", \"Triangle\", \"LookTutorialInput( 1 );\", \"\");",
                0,nullptr,0,-1);
            boundTutorialMap=map;
        }
        lookTutorialUpdateHook.thiscall<void>(self,time);
    }

    int __fastcall SetText(void* self,void*,const char* text,int index,int literal)
    {
        // FETextObject::SetText clears strings without a text-bible substitution
        // unless its literal flag is set. Only our binding labels are literal.
        for(const auto name:ScarfaceButtons::Names[gamepadIcons])
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
            if(physical>=0 && physical<int(std::size(ScarfaceButtons::Names[gamepadIcons])))
                return ScarfaceButtons::Names[gamepadIcons][physical];
        }
        return hbControlButtonLabel.fun(format,index);
    }

    uint32_t Key(std::string_view text)
    {
        uint32_t hash=0;
        for(unsigned char c:text)hash=((65599u*hash)&0x7FFFFFFF)^(c<97?c+32:c);
        return hash|0x80000000;
    }

    bool GameplayText(uint32_t key)
    {
        // Text-bible IDs of gameplay instructions. Dialog actions (Close, Load,
        // Create, etc.) continue to use the effective menu mapping.
        constexpr uint32_t keys[] = {
            0x96F55CA2,0x804C113A,0x918065F2,0x91826670,0x947C2E03,0x96D11874,
            0xA02D2959,0xA02D295B,0xA02D295D,0xA02D295F,0xD0E218A8,0xC4810877,
            0xE4381519,0xE4401720,0x83FB4E1A,0x83FC4EDB,0xB76262C5,0xB76262C6,
            0xB5F86DE5,0xB5F86DE6,0xBA4AF1CF,0xD50139A4,0xD50139A7,0x9470D95D,
            0xA1776B24,0xDFA0EE4C,0xDFD9A106,0xDEDA0E00,0xDAFC5700,0xDAFC5702,
            0xDAFC5703,0xDAFC5704,0xDAFC5705,0xDAFC5706,0xDAFC5707,0xDAFC5709,
            0xD11DD512,0x824F575E,0xAC803364,0xD3D8B600,0xDAE6E623,0xDC67F847,
            0xEA55AB3E,0xD50B3C3D,0xBB4799AC,0xF5EEDE2D,0xF5EEDE2F,
            0xC4810871,0xC4810874,0xE43213AD
        };
        return std::find(std::begin(keys),std::end(keys),key)!=std::end(keys);
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
        const bool gameplayText=GameplayText(keys[index]);
        for (size_t i=0;i<controllerExitToken;++i)
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
                replacement=mapMarkerToken;
            if(strcmp(token.name,"BUTTON_GRAPHIC_SNIPER_ZOOM")==0)
            {
                if(keys[index]==0xDFED7AFE)replacement=mapZoomToken;
                if(keys[index]==0xCBF8481D)replacement=sniperModifierToken;
            }
            // PC acceleration/braking tokens name keyboard arrows; the vehicle
            // action maps bind these to controller X/Square (slots 5/8).
            if((keys[index]==0xB76262C5 || keys[index]==0xDAFC5700 || keys[index]==0xDAFC5706) &&
                strcmp(token.name,"BUTTON_GRAPHIC_DPAD_UP")==0)replacement=accelerateToken;
            if((keys[index]==0xDAFC5702 || keys[index]==0xDAFC5705) &&
                strcmp(token.name,"BUTTON_GRAPHIC_DPAD_DOWN")==0)replacement=brakeToken;
            if((keys[index]==0x96D11874 || keys[index]==0xA02D295D) &&
                (strcmp(token.name,"BUTTON_GRAPHIC_CIRCLE")==0 || strcmp(token.name,"BUTTON_TEXT_CIRCLE")==0))
                replacement=hornToken;
            // The vehicle's EBrake action is Circle, although the old tutorial
            // names R1. Resolve Circle through InCar, preserving custom mappings.
            if((keys[index]==0xDAFC5704 || keys[index]==0xDAFC5709) &&
                (strcmp(token.name,"BUTTON_GRAPHIC_R1")==0 || strcmp(token.name,"BUTTON_TEXT_R1")==0))
                replacement=handbrakeToken;
            if(keys[index]==0xBB4799AC && strcmp(token.name,"BUTTON_GRAPHIC_L1")==0)
                replacement=lockOnToken;
            if(keys[index]==0x83FB4E1A && strcmp(token.name,"BUTTON_GRAPHIC_R1")==0)
                replacement=hardTurnToken;
            std::scoped_lock lock(tokenMutex);
            if(replacement!=i)tokens[replacement].keyboard=token.keyboard;
            size_t pos=0;
            while((pos=result.find(name,pos))!=std::wstring::npos)
            {
                result.replace(pos,name.size(),1,(gameplayText?gameplayMarkerBase:markerBase)+wchar_t(replacement));
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
        int tutorialSlot=-1;
        if(lookupText && (*text==markerBase || *text==markerBase+10))
        {
            // tutorial.p3d authors these as two separate substitutions, e.g.
            // [BUTTON_GRAPHIC_X]  [LOOK_TUTORIAL_START]. Match the localized
            // label rather than changing the shared X/Start tokens globally.
            std::wstring_view suffix=text+1;
            while(!suffix.empty() && iswspace(suffix.front()))suffix.remove_prefix(1);
            const auto label=lookupText(*text==markerBase ? "LOOK_TUTORIAL_START" : "LOOK_TUTORIAL_EXIT");
            if(label && suffix==label)
                tutorialSlot=*text==markerBase ? 15 : (lookTutorialUpdateHook ? 7 : -1);
        }
        const auto first=text;
        std::scoped_lock lock(tokenMutex);
        for (; *text; ++text)
        {
            const bool gameplayText=*text>=gameplayMarkerBase;
            const unsigned index = unsigned(*text)-(gameplayText?gameplayMarkerBase:markerBase);
            if(index>=std::size(tokens)) {output+=*text;continue;}
            const auto& token=tokens[index];
            const int slot=text==first && tutorialSlot>=0 ? tutorialSlot : token.slot;
            int physical = slot>=0 ? (gameplayText?gameplayPhysical:bindingPhysical)[slot] : token.physical;
            if(gameplayText && slot>=0 && token.gameplayContext>=0)
                physical=actionPhysical[token.gameplayContext][slot];
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
                const unsigned index=unsigned(*p)-(*p>=gameplayMarkerBase?gameplayMarkerBase:markerBase);
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

    int __stdcall EmitGlyph(void* primitive,const uint8_t* glyph,float x,float y,float width,float height,int color)
    {
        // TextureFont derives icon height from UV span * the original font page's
        // pixel height (699FB0/69A140). HD font pages can change that height without
        // changing the glyph width. Use our replacement texture's aspect instead.
        if(textureChanged && iconPage>=0 && glyph)
        {
            const int physical=pagePhysical[iconPage];
            const auto page=*reinterpret_cast<const uint32_t*>(glyph+20);
            if((page&0x80000000) && (page&0x7FFFFFFF)==unsigned(iconPage) &&
                *reinterpret_cast<const uint16_t*>(glyph+16)==glyphs[physical])
            {
                std::array<uint32_t,12> replacement;
                memcpy(replacement.data(),glyph,sizeof(replacement));
                // Our DDS contains one complete icon, independently of font atlas UVs.
                const std::array<float,4> uv{0.0f,0.0f,1.0f,1.0f};
                memcpy(replacement.data()+6,uv.data(),sizeof(uv));
                return glyphHook.stdcall<int>(primitive,replacement.data(),x,y,width,
                    width*iconHeightPerWidth[physical],color);
            }
        }
        return glyphHook.stdcall<int>(primitive,glyph,x,y,width,height,color);
    }

    int __fastcall Begin(void* self,void*,int page)
    {
        auto result=beginHook.thiscall<int>(self,page);
        iconPage=-1;
        if(device && gamepad.load(std::memory_order_relaxed) && page>=0 && page<int(std::size(pagePhysical)))
        {
            const int id=pagePhysical[page];
            if(id>=0 && icons[id] && HasIconGlyph(self,id) && SUCCEEDED(device->GetTexture(0,savedTexture.ReleaseAndGetAddressOf())))
            {
                textureChanged=SUCCEEDED(device->SetTexture(0,icons[id].Get()));
                if(textureChanged)iconPage=page;
            }
        }
        return result;
    }
    int __fastcall End(void* self,void*,int primitive)
    {
        // TextureFont::End tail-jumps to the rendering context's EndPrims. Its
        // primitive argument remains on the stack and must be forwarded/consumed.
        auto result=endHook.thiscall<int>(self,primitive);
        if(textureChanged && device)device->SetTexture(0,savedTexture.Get());
        savedTexture.Reset();textureChanged=false;iconPage=-1;return result;
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
            for(unsigned i=0;i<icons.size();++i)
            {
                icons[i]=ScarfaceButtons::CreateIcon(d,i,gamepadIcons);
                D3DSURFACE_DESC desc{};
                iconHeightPerWidth[i]=icons[i] && SUCCEEDED(icons[i]->GetLevelDesc(0,&desc)) && desc.Width
                    ? float(desc.Height)/float(desc.Width) : 1.0f;
            }
        }
        if(!queryButton || !queryActivity)
            if(auto module=GetModuleHandleW(L"Xidi.32.dll"))
            {
                queryButton=reinterpret_cast<QueryButton>(GetProcAddress(module,"XidiGetPhysicalButtonMask"));
                queryActivity=reinterpret_cast<QueryActivity>(GetProcAddress(module,"XidiGetLastControllerActivity"));
                queryProfileButton=reinterpret_cast<QueryProfileButton>(GetProcAddress(module,"XidiGetPhysicalButtonMaskForProfile"));
            }
        DWORD foregroundPid=0;GetWindowThreadProcessId(GetForegroundWindow(),&foregroundPid);
        if(foregroundPid!=GetCurrentProcessId())return;
        UpdateInputMode();
        bindingPhysical.fill(-1);
        gameplayPhysical.fill(-1);
        for(auto& bindings:actionPhysical)bindings.fill(-1);
        const wchar_t* gameplayProfile=nullptr;
        if(modernControls && mainCharacter && *mainCharacter)
            gameplayProfile=*reinterpret_cast<const uintptr_t*>(static_cast<const uint8_t*>(*mainCharacter)+0x2E8)
                ? L"InCar" : L"OnFoot";
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
                    const auto gameplayMask=queryProfileButton && gameplayProfile
                        ? queryProfileButton(controller,unsigned(button-1),gameplayProfile) : mask;
                    if(gameplayMask && !(gameplayMask&(gameplayMask-1)))gameplayPhysical[slot]=int(std::countr_zero(gameplayMask));
                    constexpr const wchar_t* profiles[]={L"OnFoot",L"InCar"};
                    for(size_t context=0;context<actionPhysical.size();++context)
                    {
                        const auto actionMask=modernControls && queryProfileButton
                            ? queryProfileButton(controller,unsigned(button-1),profiles[context]) : mask;
                        if(actionMask && !(actionMask&(actionMask-1)))actionPhysical[context][slot]=int(std::countr_zero(actionMask));
                    }
                }
            }
        }
    }

    void Initialize()
    {
        CIniReader iniReader("");
        const bool bControllerPrompts=iniReader.ReadInteger("MAIN","ControllerPrompts",1)!=0;
        if(!bControllerPrompts)return;
        gamepadIcons=unsigned(std::clamp(iniReader.ReadInteger("MAIN","GamepadIcons",0),0,int(std::size(ScarfaceButtons::Names))-1));
        modernControls=iniReader.ReadInteger("MAIN","ModernControlScheme",1)!=0;
        auto font=ScarfaceRTTI::FindVtable(".?AVTextureFont@pure3d@@");
        auto pattern=hook::pattern("8B 44 24 04 83 F8 FF 74 11 8B 51 1C 8B 04 82 8B 49 20 D1 E8 8D 04 41 C2 04 00"); // 0x6A41E0
        if(!font || pattern.size()!=1)return;
        auto lookup=pattern.get_first();
        pattern=hook::pattern("55 8B 6C 24 08 56 57 BF ? ? ? ? 8B F5 B9 01 00 00 00 33 C0 F3 A6"); // 0x6783A0
        if(pattern.empty())
            pattern=hook::pattern("55 8B 6C 24 08 56 57 E9 ? ? ? ? 8B F5 B9 01 00 00 00 33 C0 F3 A6"); // Scarface2: 0x6783A0, protected MOV EDI.
        if(pattern.size()!=1)return;
        lookupText=reinterpret_cast<decltype(lookupText)>(pattern.get_first());
        pattern=hook::pattern("83 3D ? ? ? ? 00 55 8B E9 74 ? 83 3D ? ? ? ? 00 74 ? 53 8B 1D"); // 0x42EC60 + 14
        if(pattern.size()==1)
            controllerDescription=*pattern.get_first<uintptr_t*>(14);
        else if(pattern.empty())
        {
            // Find the same description through logical-to-device binding lookup;
            // Scarface2 protects the original getter's entry instructions.
            pattern=hook::pattern("8B 0D ? ? ? ? 8B 49 04 8D 04 7F 03 C0 8B 94 00 ? ? ? ? 8B 14 91 03 C0 8B 80 ? ? ? ? 89 54 24 18"); // Scarface2: 0x42EC25 + 2.
            if(pattern.size()!=1)return;
            controllerDescription=*pattern.get_first<uintptr_t*>(2);
        }
        else return;
        pattern=hook::pattern("A1 ? ? ? ? 85 C0 74 50"); // MainCharacter: operand at +1.
        if(pattern.size()==1)mainCharacter=*pattern.get_first<void**>(1);
        pattern=hook::pattern("83 EC 10 8B 44 24 18 8B 48 1C 8B 50 24 53 8B 58 18 55 8B 68 20"); // 0x699DF0, font quad submission.
        if(pattern.size()!=1)return;
        glyphHook=safetyhook::create_inline(pattern.get_first(),EmitGlyph);
        for(unsigned i=0;i<std::size(tokens);++i)tokens[i].marker[0]=markerBase+wchar_t(i);
        bindingPhysical.fill(-1);
        gameplayPhysical.fill(-1);
        for(auto& bindings:actionPhysical)bindings.fill(-1);
        auto slot=[font](unsigned i){return injector::ReadMemory<void*>(font+i*sizeof(void*),true);};
        widthHook=safetyhook::create_inline(slot(13),Width);
        lineWidthHook=safetyhook::create_inline(slot(26),LineWidth);
        wrapWidthHook=safetyhook::create_inline(slot(11),WrapWidth);
        maskHook=safetyhook::create_inline(slot(19),Mask);
        drawHook=safetyhook::create_inline(slot(16),Draw);
        wrapDrawHook=safetyhook::create_inline(slot(15),WrapDraw);
        beginHook=safetyhook::create_inline(slot(14),Begin);
        endHook=safetyhook::create_inline(slot(17),End);
        if(glyphHook&&widthHook&&lineWidthHook&&wrapWidthHook&&maskHook&&drawHook&&wrapDrawHook&&beginHook&&endHook)
            lookupHook=safetyhook::create_inline(lookup,Lookup);
        enabled=bool(lookupHook);
        if(enabled)
        {
            auto tutorial=ScarfaceRTTI::FindVtable(".?AVLookTutorialHUD@@");
            pattern=hook::pattern("6A FF 6A 00 6A 00 6A 00 68 ? ? ? ? E8 ? ? ? ? 83 C4 14 C2 04 00 83 F8 01 75"); // 0x5B6380, Con::evaluate call at +13.
            if(tutorial && pattern.size()==1)
            {
                evaluateScript=reinterpret_cast<decltype(evaluateScript)>(injector::GetBranchDestination(pattern.get_first(13)).as_int());
                lookTutorialUpdateHook=safetyhook::create_inline(injector::ReadMemory<void*>(tutorial+54*sizeof(void*),true),UpdateLookTutorial);
            }
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
        glyphHook.reset();
        lookTutorialUpdateHook.reset();evaluateScript=nullptr;boundTutorialMap=nullptr;
        savedTexture.Reset();for(auto& icon:icons)icon.Reset();device=nullptr;
        queryActivity=nullptr;queryButton=nullptr;queryProfileButton=nullptr;enabled=false;
    }
}
