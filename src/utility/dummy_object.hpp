/*
* Utility for a dummy class with a proper constructor / destructor
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the zlib License (http://opensource.org/licenses/zlib)
*/
#pragma once
#include <type_traits>
#include <injector/injector.hpp>

//
// using snake_case to reflect std::aligned_storage
//


// Reflects aligned_storage but has constructor and destructor for the storage
template<size_t size, uintptr_t addr_ctor = 0, uintptr_t addr_dtor = 0>
struct dummy_object
{
    typename std::aligned_storage<size, 1>::type data;

    dummy_object()
    {
        static_assert(sizeof(dummy_object) == size, "Wrong sizeof");
        if(addr_ctor) ((void(__fastcall*)(void*))(injector::lazy_ptr<addr_ctor>().get()))(this);
    }

    ~dummy_object()
    {
        if(addr_dtor) ((void(__fastcall*)(void*))(injector::lazy_ptr<addr_dtor>().get()))(this);
    }
};

// Reflects aligned_storage but has constructor and destructor for the storage, where the destructor is virtual
template<size_t size, uintptr_t addr_ctor>
struct dummy_object_vmt : public dummy_object<size, addr_ctor>
{
    ~dummy_object_vmt()
    {
        auto addr_dtor = (*(void***)(this))[0];
        if(addr_dtor) ((void(__fastcall*)(void*, int edx, int))(addr_dtor))(this, 0, false);
    }
};
