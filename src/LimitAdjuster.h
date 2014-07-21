/*
 *  Limit Adjuster Base Header
 *
 *      Each "Adjuster" should inherit from the Adjuster class or the SimpleAdjuster class
 *      An Adjuster should be instantiated at global scope and it'll be automatically added to the adjusters list.
 *      This list then will be taken and used while parsing the adjuster ini file.
 *
 *      >> You can check out an example for an Adjuster at "src/sample/sample.cpp"
 *
 *
 */
#pragma once

#include "injector/injector.hpp"    // 100% sure the derived Adjuster will use this
#include <string>                   // for std::string
#include <vector>                   // for std::vector


/*
 *  You can use the following interface for your limit adjuster if it adjusts more than one limit
 */
class Adjuster
{
    public:
        // List of Adjusters object instantiated around
        static std::vector<Adjuster*>& GetAdjusters()
        {
            static std::vector<Adjuster*> adjusters;
            return adjusters;
        }
    
        struct Limit
        {
            const char* name;       // Name of the limit (INI Key)
            int         id;         // Id for the limit, this is Adjuster specific
            void*       udata;      // User data
            
            Limit() {}
            Limit(const char* name, int id, void* udata = 0) : name(name), id(id), udata(udata)
            {}
        };
    
    public:

        //
        Adjuster()              { GetAdjusters().push_back(this); }
        virtual ~Adjuster()     { /* No need to remove for now */ }

        // Get Game Version Manager
        injector::address_manager& GetGVM()
        {
            return injector::address_manager::singleton();
        }


        // Virtual Methods

        /*
         *  Here you should return an array of limits that can be adjusted by this Adjuster.
         *  The array should have an null element at the end of it, that's, and element which Limit::name is equal to null
         *  The array should live long enought to be used
         *
         *  PS: The function can return a null pointer to tell that it won't handle any limit.
         *      This is useful when the game version is not supported by the Adjuster
         */
        virtual const Limit* GetLimits()=0;
        
        /*
         *  If an limit is found on the ini and it's present on the Adjuster::Limit array for the specific Adjuster
         *  this function gets called to change the limit. The id argument is equal to the Adjuster::Limit::id.
         *  The value argument is equal to the value part of the key-value pair on the ini file.
         */
        virtual void ChangeLimit(int id, const std::string& value)=0;
};


/*
 *  You can use the following interface for your limit adjuster if it adjusts only one limit
 */
class SimpleAdjuster : public Adjuster
{
    private:
        Limit lm[2];

    public:
    
        /*
         *  Here you should return the name of the limit this adjuster will handle
         *  Just like the old 'GetLimits' the function may return a null pointer to tell it won't handle any limit
         */
        virtual const char* GetLimitName()=0;
        
        /*
         *  Just like the ChangeLimit from the base Adjuster class, the first parameter should be ignored
         *  
         */
        virtual void ChangeLimit(int, const std::string& value)=0;




        // Wrapping the old virtual methods into the new ones
        const Limit* GetLimits()
        {
            lm[0].name = GetLimitName();
            lm[1].name = nullptr;
            return lm;
        }
};





// Some maybe helpful macro
#define CREATE_NULL_LIMIT()             Limit( nullptr, 0 )
#define CREATE_LIMIT(limit)             Limit( #limit, limit )
#define CREATE_LIMIT_U(limit, udata)    Limit( #limit, limit, udata )
#define DEFINE_LIMIT(limit)             CREATE_LIMIT(limit)
#define FINISH_LIMITS()                 CREATE_NULL_LIMIT()


