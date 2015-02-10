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
#include <injector/injector.hpp>    // base injection library
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

            template<class T>
            T GetUserData() const { return reinterpret_cast<T>(udata); }
        };
    
    public:

        //
        Adjuster()              { GetAdjusters().push_back(this); }
        Adjuster(std::nullptr_t){}  // Do not register adjuster constructor (may be useful?)
        virtual ~Adjuster()     { /* No need to remove for now */ }

        // Get Game Version Manager
        static injector::address_manager& GetGVM()
        {
            return injector::address_manager::singleton();
        }

        static bool IsIII() { return GetGVM().IsIII(); }
        static bool IsVC()  { return GetGVM().IsVC(); }
        static bool IsSA()  { return GetGVM().IsSA(); }


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

        /*
         *  Gets the current usage of the limit id in a string.
         *  The output string should contain only the "Usage / Max" part
         *  Returns true if could get the usage and false otherwise
         */
        virtual bool GetUsage(int id, std::string& usagebuf) { return false; }

        /*
         *  Called after all ChangeLimit's have been called, to post process data, etc
         *  Note this is called even if this ChangeLimit has not been called because there's no ini entry for it
         */
        virtual void Process() {}

    public:

        // Helper to check if string is 'unlimited'
        static bool IsUnlimited(const std::string& str)
        {
            return str.compare("unlimited") == 0;
        }


#if __cplusplus >= 201103L || _MSC_VER >= 1800  // C++11 compilant or MSVC 2013

        template<class T>
        static bool GetUsage(std::string& str, T usage, T max)
        {
			using std::to_string;
            str = to_string(usage);
            str.append(" / ").append(to_string(max));
            return true;
        }

#else	// for MSVC 2010 and MSVC 2012

		template<class T>
		static bool GetUsage(std::string& str, T usage, T max)
        {
            str = to_string(usage);
            str.append(" / ").append(to_string(max));
            return true;
        }

		// MSVC 2010 and MSVC 2012 std::to_string provides only overloads for the types (long double), (long long) and (unsigned long long) 
		// We need to get around it to accept any T
		template<class T>
		static std::string to_string(T value)
		{
			static const bool is_floating_point = std::is_floating_point<T>::value;
			static const bool is_unsigned       = std::is_unsigned<T>::value;
			static const bool is_signed         = std::is_signed<T>::value;

			static_assert(is_floating_point || is_signed || is_unsigned, "T must be either a floating point, signed, or unsigned type");

			return is_floating_point? std::to_string((long double)value) :
				           is_signed? std::to_string((long long)value)   :
						              std::to_string((unsigned long long)value);
		}

#endif

};


/*
 *  You can use the following interface for your limit adjuster if it adjusts only one limit
 */
class SimpleAdjuster : public Adjuster
{
    private:
        Limit lm[2];

    public:
    
        SimpleAdjuster() : Adjuster() {}                        // Register adjuster constructor
        SimpleAdjuster(std::nullptr_t) : Adjuster(nullptr) {}   // Do not register adjuster constructor (may be useful?)

        /*
         *  Here you should return the name of the limit this adjuster will handle
         *  Just like the old 'GetLimits' the function may return a null pointer to tell it won't handle any limit
         */
        virtual const char* GetLimitName()=0;
        
        /*
         *  Just like the ChangeLimit from the base Adjuster class, the first parameter should be ignored
         */
        virtual void ChangeLimit(int, const std::string& value)=0;

        /*
         *  Just like the GetUsage from the base Adjuster class, the first parameter should be ignored
         */
        virtual bool GetUsage(int, std::string& usagebuf) { return false; }

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
#define DEFINE_LIMIT_U(limit, udata)    CREATE_LIMIT_U(limit, udata)
#define FINISH_LIMITS()                 CREATE_NULL_LIMIT()


