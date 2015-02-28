/*
 *  Injectors - Base Header
 *
 *  Copyright (C) 2012-2014 LINK/2012 <dma_2012@hotmail.com>
 *
 *  This software is provided 'as-is', without any express or implied
 *  warranty. In no event will the authors be held liable for any damages
 *  arising from the use of this software.
 * 
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 * 
 *     1. The origin of this software must not be misrepresented; you must not
 *     claim that you wrote the original software. If you use this software
 *     in a product, an acknowledgment in the product documentation would be
 *     appreciated but is not required.
 * 
 *     2. Altered source versions must be plainly marked as such, and must not be
 *     misrepresented as being the original software.
 * 
 *     3. This notice may not be removed or altered from any source
 *     distribution.
 *
 */
#pragma once
#include <windows.h>
#include <cstdint>
#include <cstdio>

namespace injector
{

#if 1       // GVM and Address Translator, Not very interesting for the users, so skip reading those...
    
/*
 *  game_version_manager
 *      Detects the game, the game version and the game region
 *      This assumes the executable is decrypted, so, Silent's ASI Loader is recommended.
 */
#ifndef INJECTOR_OWN_GVM 
#ifndef INJECTOR_GVM_DUMMY
class game_version_manager
{
    public:
        // Set this if you would like that MessagesBox contain PluginName as caption
        const char* PluginName;
        
    private:
        char game, region, major, minor, majorRevision, minorRevision, cracker, steam;

    public:
        game_version_manager()
        {
            #ifdef INJECTOR_GVM_PLUGIN_NAME
                PluginName = INJECTOR_GVM_PLUGIN_NAME;
            #else
                PluginName = "LODLights.asi";
            #endif
            
            this->Clear();
        }
        

        // Clear any information about game version
        void Clear()
        {
            game = region = major = minor = cracker = steam = 0;
        }
        
        // Checks if I don't know the game we are attached to
        bool IsUnknown()		{ return game == 0; }
        // Checks if this is the steam version
        bool IsSteam()			{ return steam != 0; }
        // Gets the game we are attached to (0, '3', 'V', 'S', 'I', 'E')
        char GetGame()			{ return game; }
        // Gets the region from the game we are attached to (0, 'U', 'E');
        char GetRegion()		{ return region; }
        // Get major and minor version of the game (e.g. [major = 1, minor = 0] = 1.0)
        int GetMajorVersion()	{ return major; }
        int GetMinorVersion()	{ return minor; }
        int GetMajorRevisionVersion()	{ return majorRevision; }
        int GetMinorRevisionVersion()	{ return minorRevision; }
        
        bool IsHoodlum()        { return cracker == 'H'; }
        
        // Region conditions
        bool IsUS() { return region == 'U'; }
        bool IsEU() { return region == 'E'; }

        // Game Conditions
        bool IsIII() { return game == '3'; }
        bool IsVC () { return game == 'V'; }
        bool IsSA () { return game == 'S'; }
        bool IsIV () { return game == 'I'; }
        bool IsEFLC(){ return game == 'E'; }

        // Detects game, region and version; returns false if could not detect it
        bool Detect();
        
        // Gets the game version as text, the buffer must contain at least 32 bytes of space.
        char* GetVersionText(char* buffer)
        {
            if(this->IsUnknown())
            {
                strcpy(buffer, "UNKNOWN GAME");
                return buffer;
            }

            const char* g = this->IsIII() ? "III" : this->IsVC() ? "VC" : this->IsSA() ? "SA" : this->IsIV() ? "IV" : this->IsEFLC() ? "EFLC" : "UNK";
            const char* r = this->IsUS()? "US" : this->IsEU()? "EURO" : "UNK_REGION";
            const char* s = this->IsSteam()? "Steam" : "";
            if (this->IsIII() || this->IsVC() || this->IsSA())
                sprintf(buffer, "GTA %s %d.%d %s%s", g, major, minor, r, s);
            else 
                sprintf(buffer, "GTA %s %d.%d.%d.%d %s%s", g, major, minor, majorRevision, minorRevision, r, s);
            return buffer;
        }


    public:
        // Raises a error saying that you could not detect the game version
        void RaiseCouldNotDetect()
        {
            MessageBoxA(0,
                "Could not detect the game version\nContact the mod creator!",
                PluginName, MB_ICONERROR
            );
        }

        // Raises a error saying that the exe version is incompatible (and output the exe name)
        void RaiseIncompatibleVersion()
        {
            char buf[128], v[32];
            sprintf(buf,
                "An incompatible exe version has been detected! (%s)\nContact the mod creator!",
                GetVersionText(v)
                );
            MessageBoxA(0, buf, PluginName, MB_ICONERROR);
        }
};
#else   // INJECTOR_GVM_DUMMY
class game_version_manager
{
    public:
        bool Detect() { return true; }
};
#endif  // INJECTOR_GVM_DUMMY
#endif  // INJECTOR_OWN_GVM


/*
 *  address_manager
 *      Address translator from 1.0 executables to other executables offsets
 *      Inherits from game_version_manager ;)
 */
class address_manager : public game_version_manager
{
    private:
        address_manager()
        {
            this->Detect();
        }
        
        // You could implement your translator for the address your plugin uses
        // If not implemented, the translator won't translate anything, just return the samething as before
        #ifdef INJECTOR_GVM_HAS_TRANSLATOR
                void* translator(void* p);
        #else
                void* translator(void* p) { return p; }
        #endif

    public:
        // Translates address p to the running executable pointer
        void* translate(void* p)
        {
            return translator(p);
        }
        
        
    public:
        // Address manager singleton
        static address_manager& singleton()
        {
            static address_manager m;
            return m;
        }
        
        // Static version of translate()
        static void* translate_address(void* p)
        {
            return singleton().translate(p);
        }
        
        //
        static void set_name(const char* modname)
        {
            singleton().PluginName = modname;
        }
        
    public:
        // Functors for memory translation:

        // Translates aslr translator
        struct fn_mem_translator_aslr
        {
            void* operator()(void* p) const
            {
                static uintptr_t  module = (uintptr_t)GetModuleHandle(NULL);
                return (void*)((uintptr_t)(p)-(0x400000 - module));
            }
        };

        // Translates nothing translator
        struct fn_mem_translator_nop
        {
            void* operator()(void* p) const
            { return p; }
        };
        
        // Real translator
        struct fn_mem_translator
        {
            void* operator()(void* p) const
            { return translate_address(p); }
        };
};

#endif  // #if 1


}