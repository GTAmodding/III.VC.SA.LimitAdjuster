/*
 *  Sample Adjuster
 *
 */
#include "LimitAdjuster.h"
 
// Sample Adjuster
class Sample : public Adjuster
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
        virtual const Limit* GetLimits()
        {
            static Limit limits[] = 
            {
                // You could use DEFINE_LIMIT(CoffeeLimit) instead of {...}
                { "CoffeeLimit",        CoffeeLimit         },
                { "FoodLimit", FoodLimit  },
                { "IceLimit",   IceLimit    },
                // You could also use FINISH_LIMITS()
                { nullptr, 0 }
            };
            
            // This Sample Adjuster can handle only San Andreas game
            auto& gmv = this->GetGVM();
            if(gvm.IsSA()) return limits;
            return nullptr;
        }
        
        // Change an specific limit
        virtual void ChangeLimit(int id, const std::string& value)
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
} adjuster_sample;

