/*
 *  Sample Adjuster
 *
 */
#include "LimitAdjuster.h"
 
// Sample Simple Adjuster
class Sample1 : public SimpleAdjuster
{
    public:
        const char* GetLimitName() { return GetGVM().IsSA()? "FoodLimit" : nullptr; }
        void ChangeLimit(int, const std::string& value) { injector::WriteMemory(0xF00D, std::stof(value)); }
        
// Instantiate the adjuster on the global scope
} simple_adjuster_sample;
 
 
// Sample "Complex" Adjuster
class Sample2 : public Adjuster
{
    public:
        // ID enum, those ids are local between each Adjuster, you shouldn't worry about conflicts with other adjusters
        enum
        {
            CoffeeLimit,
            FoodLimit,
            IceLimit
        };
    
        // Get the limit table that we're going to handle
        const Limit* GetLimits()
        {
            static Limit limits[] = 
            {
                // You could use DEFINE_LIMIT(CoffeeLimit) instead of {...}
                { "CoffeeLimit",    CoffeeLimit },
                { "FoodLimit",      FoodLimit   },
                { "IceLimit",       IceLimit    },
                // You could also use FINISH_LIMITS()
                { nullptr, 0 }
            };
            
            // This Sample Adjuster can handle only San Andreas game
            if(GetGVM().IsSA()) return limits;
            return nullptr;
        }
        
        // Change an specific limit
        void ChangeLimit(int id, const std::string& value)
        {
            switch(id)
            {
                case CoffeeLimit:   // Changes the cofee limit
                    injector::WriteMemory(0xC0FFEE, std::stoi(value, 0, 0));
                    break;
                    
                case FoodLimit:     // Changes the food limit
                    injector::WriteMemory(0xF00D, std::stof(value));
                    break;
                    
                case IceLimit:      // Changes the ice limit
                    injector::WriteMemory<short>(0x1CED, std::stoi(value) % 1337);
                    break;
            }
        }


 // Instantiate the adjuster on the global scope
} complex_adjuster_sample;

