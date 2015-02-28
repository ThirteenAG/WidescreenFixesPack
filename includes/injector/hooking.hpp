/*
 *  Injectors - Classes for making your hooking life easy
 *
 *  Copyright (C) 2013-2014 LINK/2012 <dma_2012@hotmail.com>
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
#include "injector.hpp"
#include <cassert>
#include <functional>
#include <memory>       // for std::shared_ptr
#include <list>

namespace injector
{
    /*
     *  scoped_base
     *      Base for any scoped hooking type
     *      !!!! NOTICE !!!! --> Any derived which implements/reimplements restore() should implement a destructor calling it
     */
    class scoped_base
    {
        public:
            virtual ~scoped_base() {}
            virtual void restore() = 0;
    };

    /*
     *  scoped_basic
     *      Base for scoped types which will need a buffer to save/restore stuff
     */
    template<size_t bufsize>    // TODO specialize bufsize=0 to be dynamic
    class scoped_basic : public scoped_base
    {
        private:
            uint8_t            buf[bufsize];// Saved content
            memory_pointer_raw addr;        // Data saved from this address
            size_t             size;        // Size saved
            bool               saved;       // Something saved?
            bool               vp;          // Virtual protect?

        public:

            static const bool  is_dynamic = false;

            // Restore the previosly saved data
            // Problems may arise if someone else hooked the same place using the same method
            virtual void restore()
            {
                #ifndef INJECTOR_SCOPED_NOSAVE_NORESTORE
                    if(this->saved)
                    {
                        WriteMemoryRaw(this->addr, this->buf, this->size, this->vp);
                        this->saved = false;
                    }
                #endif
            }

            // Save buffer at @addr with @size and virtual protect @vp
            virtual void save(memory_pointer_tr addr, size_t size, bool vp)
            {
                #ifndef INJECTOR_SCOPED_NOSAVE_NORESTORE
                    assert(size <= bufsize);            // Debug Safeness
                    this->restore();                    // Restore anything we have saved
                    this->saved = true;                 // Mark that we have data save
                    this->addr = addr.get<void>();      // Save address
                    this->size = size;                  // Save size
                    this->vp = vp;                      // Save virtual protect
                    ReadMemoryRaw(addr, buf, size, vp); // Save buffer
                #endif
            }

        public:
            // Constructor, initialises
            scoped_basic() : saved(false)
            {}

            ~scoped_basic()
            {
                this->restore();
            }

            // No copy construction, we can't do this! Sure we can move construct :)
            scoped_basic(const scoped_basic&) = delete;
            scoped_basic(scoped_basic&& rhs)
            {
                *this = std::move(rhs);
            }

            scoped_basic& operator=(const scoped_basic& rhs) = delete;
            scoped_basic& operator=(scoped_basic&& rhs)
            {
                if(this->saved = rhs.saved)
                {
                    assert(bufsize >= rhs.size);

                    this->addr = rhs.addr;
                    this->size = rhs.size;
                    this->vp = rhs.vp;
                    memcpy(buf, rhs.buf, rhs.size);

                    rhs.saved = false;
                }
                return *this;
            }
    };

    /*
     *  RAII wrapper for memory writes
     *  Can save only basic and POD types
     */
    template<size_t bufsize_ = 10>
    class scoped_write : public scoped_basic<bufsize_>
    {
        public:
            // Save buffer at @addr with @size and virtual protect @vp and then overwrite it with @value
            void write(memory_pointer_tr addr, void* value, size_t size, bool vp)
            {
                this->save(addr, size, vp);
                return WriteMemoryRaw(addr, value, size, vp);
            }

            // Save buffer at @addr with size sizeof(@value) and virtual protect @vp and then overwrite it with @value
            template<class T>
            void write(memory_pointer_tr addr, T value, bool vp = false)
            {
                this->save(addr, sizeof(T), vp);
                return WriteMemory<T>(addr, value, vp);
            }

            // Constructors, move constructors, assigment operators........
            scoped_write() = default;
            scoped_write(const scoped_write&) = delete;
            scoped_write(scoped_write&& rhs) : scoped_basic<bufsize_>(std::move(rhs)) {}
            scoped_write& operator=(const scoped_write& rhs) = delete;
            scoped_write& operator=(scoped_write&& rhs)
            { scoped_basic<bufsize_>::operator=(std::move(rhs)); return *this; }
    };

    /*
     *  RAII wrapper for filling
     */
    template<size_t bufsize_ = 5>
    class scoped_fill : public scoped_basic<bufsize_>
    {
        public:
            // Fills memory at @addr with value @value and size @size and virtual protect @vp
            void fill(memory_pointer_tr addr, uint8_t value, size_t size, bool vp)
            {
                this->save(addr, size, vp);
                return MemoryFill(addr, value, size, vp);
            }

            // Constructors, move constructors, assigment operators........
            scoped_fill() = default;
            scoped_fill(const scoped_fill&) = delete;
            scoped_fill(scoped_fill&& rhs) : scoped_basic<bufsize_>(std::move(rhs)) {}
            scoped_fill& operator=(const scoped_fill& rhs) = delete;
            scoped_fill& operator=(scoped_fill&& rhs)
            { scoped_basic<bufsize_>::operator=(std::move(rhs)); return *this; }

            scoped_fill(memory_pointer_tr addr, uint8_t value, size_t size, bool vp)
            { fill(addr, value, vp); }
    };
    
    /*
     *  RAII wrapper for nopping
     */
    template<size_t bufsize_>
    class scoped_nop : public scoped_basic<bufsize_>
    {
        public:
            // Makes NOP at @addr with value @value and size @size and virtual protect @vp
            void make_nop(memory_pointer_tr addr, size_t size = 1, bool vp = true)
            {
                this->save(addr, size, vp);
                return MakeNOP(addr, size, vp);
            }

            // Constructors, move constructors, assigment operators........
            scoped_nop() = default;
            scoped_nop(const scoped_nop&) = delete;
            scoped_nop(scoped_nop&& rhs) : scoped_basic<bufsize_>(std::move(rhs)) {}
            scoped_nop& operator=(const scoped_nop& rhs) = delete;
            scoped_nop& operator=(scoped_nop&& rhs)
            { scoped_basic<bufsize_>::operator=(std::move(rhs)); return *this; }

            scoped_nop(memory_pointer_tr addr, size_t size = 1, bool vp = true)
            { make_nop(addr, size, vp); }
    };
    
    /*
     *  RAII wrapper for MakeJMP 
     */
    class scoped_jmp : public scoped_basic<5>
    {
        public:
            // Makes NOP at @addr with value @value and size @size and virtual protect @vp
            memory_pointer_raw make_jmp(memory_pointer_tr at, memory_pointer_raw dest, bool vp = true)
            {
                this->save(at, 5, vp);
                return MakeJMP(at, dest, vp);
            }

            // Constructors, move constructors, assigment operators........
            scoped_jmp() = default;
            scoped_jmp(const scoped_jmp&) = delete;
            scoped_jmp(scoped_jmp&& rhs) : scoped_basic<5>(std::move(rhs)) {}
            scoped_jmp& operator=(const scoped_jmp& rhs) = delete;
            scoped_jmp& operator=(scoped_jmp&& rhs)
            { scoped_basic<5>::operator=(std::move(rhs)); return *this; }

            scoped_jmp(memory_pointer_tr at, memory_pointer_raw dest, bool vp = true)
            { make_jmp(at, dest, vp); }
    };
    
    /*
     *  RAII wrapper for MakeCALL 
     */
    class scoped_call : public scoped_basic<5>
    {
        public:
            // Makes NOP at @addr with value @value and size @size and virtual protect @vp
            memory_pointer_raw make_call(memory_pointer_tr at, memory_pointer_raw dest, bool vp = true)
            {
                this->save(at, 5, vp);
                return MakeCALL(at, dest, vp);
            }

            // Constructors, move constructors, assigment operators........
            scoped_call() = default;
            scoped_call(const scoped_call&) = delete;
            scoped_call(scoped_call&& rhs) : scoped_basic<5>(std::move(rhs)) {}
            scoped_call& operator=(const scoped_call& rhs) = delete;
            scoped_call& operator=(scoped_call&& rhs)
            { scoped_basic<5>::operator=(std::move(rhs)); return *this; }

            scoped_call(memory_pointer_tr at, memory_pointer_raw dest, bool vp = true)
            { make_call(at, dest, vp); }
    };


#if __cplusplus >= 201103L || _MSC_VER >= 1800  // C++11 or MSVC 2013 required for variadic templates

    /*
     *  function_hooker_manager
     *      Manages many function_hookers that points to the same address
     *      The need for this function arises because otherwise we would only be able to allow one hook per address using function_hookers
     *      This manager takes care of the amount of hooks placed in a particular address, calls the hooks and unhooks when necessary.
     */
    template<class ToManage, class Ret, class ...Args>
    class function_hooker_manager : protected scoped_call
    {
        private:
            using func_type_raw = typename ToManage::func_type_raw;
            using func_type     = typename ToManage::func_type;
            using functor_type  = typename ToManage::functor_type;
            using assoc_type    = std::list<std::pair<const ToManage*, functor_type>>;

            // Only construction is allowed... by myself ofcourse...
            function_hooker_manager() = default;
            function_hooker_manager(const function_hooker_manager&) = delete;
            function_hooker_manager(function_hooker_manager&&) = delete;

            //
            func_type_raw   original;               // Pointer to the original function we've replaced
            assoc_type      assoc;                  // Association between owners of a hook and the hook (map)
            bool            has_hooked = false;     // Is the hook already in place?

            // Find assoc iterator for the content owned by 'owned'
            typename assoc_type::iterator find_assoc(const ToManage& owner)
            {
                for(auto it = assoc.begin(); it != assoc.end(); ++it)
                    if(it->first == &owner) return it;
                return assoc.end();
            }

            // Adds a new item to the association map (or override if already in the map)
            void add(const ToManage& hooker, functor_type functor)
            {
                auto it = find_assoc(hooker);
                if(it != assoc.end())
                    it->second = std::move(functor);
                else
                    assoc.emplace_back(&hooker, std::move(functor));
            }

        public:
            // Forwards the call to all the installed hooks
            static Ret call_hooks(Args&... args)
            {
                auto& manager = *instance();

                if(manager.assoc.size() == 0) // This may be uncommon but may happen (?), no hook installed
                    return manager.original(args...);

                // Functor for the original call
                func_type original = [&manager](Args... args) -> Ret {
                    return manager.original(args...);
                };

                if(manager.assoc.size() == 1)
                {
                    // We have only one hook, just use it directly no need to go further in complexity
                    auto& functor = manager.assoc.begin()->second;
                    return functor(std::move(original), args...);
                }
                else
                {
                    // Build a serie of functors which captures the previous functor sending it to the next functor,
                    // that's what would happen if the hooks took place independent of the template staticness (AAAAAAA)
                    func_type next = std::move(original);
                    for(auto it = manager.assoc.begin(); it != manager.assoc.end(); ++it)
                    {
                        auto& functor = it->second;
                        next = [functor, next](Args... args) -> Ret
                        {
                            return functor(next, args...);
                        };
                    }
                    return next(args...);
                }
            }

        public:

            // Installs a hook associated with the function_hooker 'hooker' which would call the specified 'functor'
            // We need an auxiliar function pointer 'ptr' (to abstract calling conventions) which should forward itself to ^call_hooks
            void make_call(const ToManage& hooker, functor_type functor, memory_pointer_raw ptr)
            {
                this->add(hooker, std::move(functor));

                // Make sure we only hook this address for the manager once
                if(!this->has_hooked)
                {
                    // (the following cast is needed for __thiscall functions)
                    this->original = (func_type_raw) (void*) scoped_call::make_call(hooker.addr, ptr).get();
                    this->has_hooked = true;
                }
            }

            // Restores the state of the call we've replaced in the game code
            // All installed hooks gets uninstalled after this
            void restore()
            {
                if(this->has_hooked)
                {
                    this->has_hooked = false;
                    this->assoc.clear();
                    return scoped_call::restore();
                }
            }

            // Replaces the hook associated with 'from' to be associated with 'to'
            // After this call the 'from' object has no association in this manager
            void replace(const ToManage& from, const ToManage& to)
            {
                auto it = find_assoc(from);
                if(it != assoc.end())
                {
                    auto functor = std::move(it->second);
                    assoc.erase(it);
                    this->add(to, std::move(functor));
                }
            }

            // Removes the hook associated with the specified 'hooker'
            // If the number of hooks reaches zero after the remotion, a restore will take place
            void remove(const ToManage& hooker)
            {
                auto it = find_assoc(hooker);
                if(it != assoc.end())
                {
                    assoc.erase(it);
                    if(assoc.size() == 0) this->restore();
                }
            }

            // The instance of this specific manager
            // This work as a shared pointer to avoid the static destruction of the manager even when a static function_hooker
            // still wants to use it.
            static std::shared_ptr<function_hooker_manager> instance()
            {
                static auto fm_ptr = std::shared_ptr<function_hooker_manager>(new function_hooker_manager());
                return fm_ptr;
            }

    };


    /*
     *  function_hooker_base
     *      Base for any function_hooker, this class manages the relationship with the function hooker manager
     */
    template<uintptr_t addr1, class FuncType, class Ret, class ...Args>
    class function_hooker_base : public scoped_base
    {
        public:
            static const uintptr_t addr = addr1;

            using func_type_raw = FuncType;
            using func_type     = std::function<Ret(Args...)>;
            using functor_type  = std::function<Ret(func_type, Args&...)>;
            using manager_type  = function_hooker_manager<function_hooker_base, Ret, Args...>;

        public:
            // Constructors, move constructors, assigment operators........
            function_hooker_base(const function_hooker_base&) = delete;
            function_hooker_base& operator=(const function_hooker_base& rhs) = delete;

            function_hooker_base() : manager(manager_type::instance())
            {}

            //
            virtual ~function_hooker_base()
            {
                this->restore();
            }

            // The move constructor should do a replace in the manager
            function_hooker_base(function_hooker_base&& rhs)
                : scoped_base(std::move(rhs)), has_call(rhs.has_call),
                  manager(rhs.manager)    // (don't move the manager!, every function_hooker should own one)
            {
                manager->replace(rhs, *this);
            }

            // The move assignment operator should also do a replace in the manager
            function_hooker_base& operator=(function_hooker_base&& rhs)
            {
                scoped_base::operator=(std::move(rhs));
                manager->replace(rhs, *this);
                this->has_call = rhs.has_call;
                this->manager = rhs.manager;        // (don't move the manager! every function_hooker should own one) 
                return *this;
            }

            // Deriveds should implement a proper make_call (yeah it's virtual so derived-deriveds can do some fest)
            virtual void make_call(functor_type functor) = 0;

            // Restores the state of the call we've replaced in the game code
            virtual void restore()
            {
                this->has_call = false;
                manager->remove(*this);
            }

            // Checkers whether a hook is installed
            bool has_hooked()
            {
                return this->has_call;
            }

        private:
            bool has_call = false;                      // Has a hook installed?
            std::shared_ptr<manager_type> manager;      // **EVERY** function_hooker should have a ownership over it's manager_type
                                                        // this prevents the static destruction of the manager_type while it may be still needed.

        protected: // Forwarders to the function hooker manager

            void make_call(functor_type functor, memory_pointer_raw ptr)
            {
                this->has_call = true;
                manager->make_call(*this, std::move(functor), ptr);
            }

            static Ret call_hooks(Args&... a)
            {
                return manager_type::call_hooks(a...);
            }
    };




    /*
     *  function_hooker
     *      For standard conventions (usually __cdecl)
     */
    template<uintptr_t addr1, class Prototype>
    struct function_hooker;

    template<uintptr_t addr1, class Ret, class ...Args>
    class function_hooker<addr1, Ret(Args...)>
        : public function_hooker_base<addr1, Ret(*)(Args...), Ret, Args...>
    {
        private:
            using base = function_hooker_base<addr1, Ret(*)(Args...), Ret, Args...>;

            // The hook caller
            static Ret call(Args... a)
            {
                return base::call_hooks(a...);
            }

        public:
            // Constructors, move constructors, assigment operators........
            function_hooker() = default;
            function_hooker(const function_hooker&) = delete;
            function_hooker(function_hooker&& rhs) : base(std::move(rhs)) {}
            function_hooker& operator=(const function_hooker& rhs) = delete;
            function_hooker& operator=(function_hooker&& rhs)
            { base::operator=(std::move(rhs)); return *this; }

            // Makes the hook
            void make_call(typename base::functor_type functor)
            {
                return base::make_call(std::move(functor), raw_ptr(call));
            }
    };


    /*
     *  function_hooker_stdcall
     *      For stdcall conventions (__stdcall)
     */
    template<uintptr_t addr1, class Prototype>
    struct function_hooker_stdcall;

    template<uintptr_t addr1, class Ret, class ...Args>
    struct function_hooker_stdcall<addr1, Ret(Args...)>
        : public function_hooker_base<addr1, Ret(__stdcall*)(Args...), Ret, Args...>
    {
        private:
            using base = function_hooker_base<addr1, Ret(__stdcall*)(Args...), Ret, Args...>;

            // The hook caller
            static Ret __stdcall call(Args... a)
            {
                return base::call_hooks(a...);
            }

        public:
            // Constructors, move constructors, assigment operators........
            function_hooker_stdcall() = default;
            function_hooker_stdcall(const function_hooker_stdcall&) = delete;
            function_hooker_stdcall(function_hooker_stdcall&& rhs) : base(std::move(rhs)) {}
            function_hooker_stdcall& operator=(const function_hooker_stdcall& rhs) = delete;
            function_hooker_stdcall& operator=(function_hooker_stdcall&& rhs)
            { base::operator=(std::move(rhs)); return *this; }

            // Makes the hook
            void make_call(typename base::functor_type functor)
            {
                return base::make_call(std::move(functor), raw_ptr(call));
            }
    };


    /*
     *  function_hooker_fastcall
     *      For fastcall conventions (__fastcall)
     */
    template<uintptr_t addr1, class Prototype>
    struct function_hooker_fastcall;

    template<uintptr_t addr1, class Ret, class ...Args>
    struct function_hooker_fastcall<addr1, Ret(Args...)>
        : public function_hooker_base<addr1, Ret(__fastcall*)(Args...), Ret, Args...>
    {
        private:
            using base = function_hooker_base<addr1, Ret(__fastcall*)(Args...), Ret, Args...>;

            // The hook caller
            static Ret __fastcall call(Args... a)
            {
                return base::call_hooks(a...);
            }

        public:
            // Constructors, move constructors, assigment operators........
            function_hooker_fastcall() = default;
            function_hooker_fastcall(const function_hooker_fastcall&) = delete;
            function_hooker_fastcall(function_hooker_fastcall&& rhs) : base(std::move(rhs)) {}
            function_hooker_fastcall& operator=(const function_hooker_fastcall& rhs) = delete;
            function_hooker_fastcall& operator=(function_hooker_fastcall&& rhs)
            { base::operator=(std::move(rhs)); return *this; }

            // Makes the hook
            void make_call(typename base::functor_type functor)
            {
                return base::make_call(std::move(functor), raw_ptr(call));
            }
    };


    /*
     *  function_hooker_thiscall
     *      For thiscall conventions (__thiscall, class methods)
     */
    template<uintptr_t addr1, class Prototype>
    struct function_hooker_thiscall;

    template<uintptr_t addr1, class Ret, class ...Args>
    struct function_hooker_thiscall<addr1, Ret(Args...)>
        : public function_hooker_base<addr1, Ret(__thiscall*)(Args...), Ret, Args...>
    {
        private:
            using base = function_hooker_base<addr1, Ret(__thiscall*)(Args...), Ret, Args...>;

            // The hook caller
            static Ret __thiscall call(Args... a)
            {
                return base::call_hooks(a...);
            }

        public:
            // Constructors, move constructors, assigment operators........
            function_hooker_thiscall() = default;
            function_hooker_thiscall(const function_hooker_thiscall&) = delete;
            function_hooker_thiscall(function_hooker_thiscall&& rhs) : base(std::move(rhs)) {}
            function_hooker_thiscall& operator=(const function_hooker_thiscall& rhs) = delete;
            function_hooker_thiscall& operator=(function_hooker_thiscall&& rhs)
            { base::operator=(std::move(rhs)); return *this; }

            // Makes the hook
            void make_call(typename base::functor_type functor)
            {
                return base::make_call(std::move(functor), raw_ptr(call));
            }
    };



    /******************* HELPERS ******************/

    /*
     *  Adds a hook to be alive for the entire program lifetime
     *  That means the hook received will be alive until the program dies.
     *  Note: Parameter must be a rvalue
     */
    template<class T> inline
    T& add_static_hook(T&& hooker)
    {
        static std::list<T> a;
        return *a.emplace(a.end(), std::move(hooker));
    }

    /*
     *  Makes a hook which is alive until it gets out of scope
     *  'T' must be any function_hooker object
     */
    template<class T, class F> inline
    T make_function_hook(F functor)
    {
        T a;
        a.make_call(std::move(functor));
        return a;
    }

    /*
     *  Makes a hook which is alive for the entire lifetime of this program
     *  'T' must be any function_hooker object
     */
    template<class T, class F> inline
    T& make_static_hook(F functor)
    {
        return add_static_hook(make_function_hook<T>(std::move(functor)));
    }


    // TODO when we have access to C++14 add a make_function_hook, make_stdcall_function_hook, and so on
    // the problem behind implement it with C++11 is that lambdas cannot be generic and the first param of a hook is a functor pointing
    // to the previous call pointer

#endif

}
