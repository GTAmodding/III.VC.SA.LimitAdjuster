/*
* Utility for adjusting static arrays from the game
* Copyright (c) 2014 LINK/2012 <dma_2012@hotmail.com>
* Licensed under the MIT License (http://opensource.org/licenses/MIT)
*/
#pragma once
#include "LimitAdjuster.h"
#include "utility/PointerAdjuster.hpp"

/*
    StaticArrayAdjuster
        Dynamic adjuster for arrays statically allocated in some data segment
        @T is the type of the element in the array
*/
template<class T>
class StaticArrayAdjuster : public SimpleAdjuster
{
    private:
        PointerAdjuster ptrs;                                           // Pointers to be replaced in rellocations
        std::vector<T>  our_array;                                      // Our rellocated array
        bool            has_relloc;                                     // Has a relloc happened in any moment? (i.e. we're using our_array)

    public:
        T* const     default_ptr;                                       // Default static array (in the data segment0
        const size_t default_size;                                      // Default size of the static array

        // Needs the address of the original static array and it's size (in number of elements not in bytes!!)
        StaticArrayAdjuster(uintptr_t addr, size_t size)
            : has_relloc(false), default_size(size), default_ptr(injector::memory_pointer(addr).get())
        {}

        // Adds a pointer for rellocation when necessary
        // Displacement is how far (in bytes) this pointer is far from the beggining of the array 
        void AddPointer(uintptr_t addr, int displacement = 0)
        {
            ptrs.AddPointer(addr, displacement);
        }

        // Makes sure the array has enought capacity to hold at least size elements
        StaticArrayAdjuster& EnsureHasCapacityFor(size_t size)
        {
            if(size >= GetArraySize()) GrowArray();
            return *this;
        }

        // Accesses a element from the array
        T& operator[](size_t i)
        {
            return GetArray()[i];
        }

        // Gets the current size of the array
        size_t GetArraySize()
        {
            return has_relloc? our_array.size() : default_size;
        }

        // Gets the current array pointer (either rellocated or default)
        T* GetArray()
        {
            return has_relloc? &our_array[0] : &default_ptr[0];
        }

        // Grows the array by 100%
        void GrowArray()
        {
            RellocArray(GetArraySize() * 2);
        }

        // Rellocate all pointers to have size elements
        void RellocArray(size_t size)
        {
            // Reallocate the array and relloc the pointers
            our_array.resize(size);
            ApplyPatches();

            // Move the contents of the previous array to the new array if it's the first time we're rellocating
            // Not necessary the second time because std::vector handles that for us
            if(has_relloc == false)
            {
                ptrs.ApplyReflection(&our_array[0], default_ptr, sizeof(T), size, default_size);
            }

            has_relloc = true;
        }

        // Makes a reflection of the current dynamic array in the static array (of course only until the max size of the static array)
        void ApplyReflection()
        {
            if(has_relloc)
            {
                ptrs.ApplyReflection(default_ptr, &our_array[0], sizeof(T), default_size, our_array.size());
            }
        }

    private:

        // Performs the rellocation itself
        void ApplyPatches()
        {
            ptrs.ApplyPatches(&our_array[0]);
        }
};
