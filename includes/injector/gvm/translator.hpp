/*
 *  Injectors - Address Translation Management
 *
 *  Copyright (C) 2014 LINK/2012 <dma_2012@hotmail.com>
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

#if !defined(INJECTOR_GVM_HAS_TRANSLATOR)
#error Missing INJECTOR_GVM_HAS_TRANSLATOR on compiler definitions
#endif

/*
 *  This is a quick solution for address translations if you're too lazy to implement a proper address_manager::translator by yourself
 *  So, just call address_translator_manager::singleton().translate(p) from your address_manager::translator and that's it.
 *  It'll translate addresses based on 'address_translator' objects, when one gets constructed it turns into a possible translator.
 *  At the constructor of your derived 'address_translator' make the map object to have [addr_to_translate] = translated_addr;
 *  There's also the virtual method 'fallback' that will get called when the translation wasn't possible, you can do some fallback stuff here
 *      (such as return the pointer as is or output a error message)
 */

#include "../injector.hpp"
#include <list>
#include <map>
#include <algorithm>

namespace injector
{
    /*
     *  address_translator
     *      Base for an address translator
     */
    class address_translator
    {
        private:
            bool enabled;
            void add();
            void remove();

        protected:
            friend class address_translator_manager;
            std::map<memory_pointer_raw, memory_pointer_raw> map;

        public:
            address_translator() : enabled(true)
            {
                // Must have bounds filled with min ptr and max ptr to have search working properly
				map.insert(std::make_pair(raw_ptr(0x00000000u), raw_ptr(0x00000000u)));
                map.insert(std::make_pair(raw_ptr(0xffffffffu), raw_ptr(0xffffffffu)));
                add();
            }

            ~address_translator()
            {
                remove();
            }

            virtual void* fallback(void*) const
            {
                return nullptr;
            }


            // Enables or disables this translator
            void enable(bool enable_it)
            {
                if(enable_it) this->enable();
                else          this->disable();
            }

            // Enables this translator
            void enable()
            {
                this->enabled = true;
            }

            // Disables this translator
            void disable()
            {
                this->enabled = false;
            }

            // Checks if this translator is enabled
            bool is_enabled() const
            {
                return enabled;
            }
    };

    /*
     *  address_translator_manager
     *      Manages the address_translator objects
     */
    class address_translator_manager
    {
        protected:
            friend class address_manager;
            friend class address_translator;

            std::list<const address_translator*> translators;

            void add(const address_translator& t)
            {
                translators.push_front(&t);
            }

            void remove(const address_translator& t)
            {
                translators.remove(&t);
            }

         public:
            // Translates the address p
            void* translator(void* p);

            // Singleton object
            static address_translator_manager& singleton()
            {
                static address_translator_manager mgr;
                return mgr;
            }
    };



    inline void* address_translator_manager::translator(void* p_)
    {
        static const size_t max_ptr_dist = 7;

        // Tries to find an address in a translator map
        auto try_map = [](const std::map<memory_pointer_raw, memory_pointer_raw>& map, memory_pointer_raw p) -> memory_pointer_raw
        {
            memory_pointer_raw result = nullptr;

            // Find first element in the map that is greater than or equal to p
            auto it = map.lower_bound(p);
            if(it != map.end())
            {
                // If it's not exactly the address, get back one position on the table
                if(it->first != p) --it;

                auto diff = (p - it->first).as_int();       // What's the difference between p and that address?
                if(diff <= max_ptr_dist)                    // Could we live with this difference in hands?
                    result = it->second + raw_ptr(diff);    // Yes, we can!
            }

            return result;
        };


        //
        memory_pointer_raw result = nullptr;

        // Try to find translation for this pointer
        auto& mgr = address_translator_manager::singleton().translators;
        for(auto it = mgr.begin(); result == nullptr && it != mgr.end(); ++it)
        {
            auto& t = **it;
            if(t.is_enabled()) result = try_map(t.map, p_);
        }

        // If we couldn't translate the address, notify and try to fallback
        if(result.is_null())
        {
            for(auto it = mgr.begin(); result == nullptr && it != mgr.end(); ++it)
            {
                auto& t = **it;
                if(t.is_enabled())  result = t.fallback(p_);
            }
        }
    
        return result.get();
    }

    inline void address_translator::add()
    {
        address_translator_manager::singleton().add(*this);
    }

    inline void address_translator::remove()
    {
        address_translator_manager::singleton().remove(*this);
    }
}
