#ifndef CooldownProfile_h
#define CooldownProfile_h

#include <Arduino.h>
#include "BaseProfile.h"
#include "../const.h"

class CooldownProfile : public BaseProfile
{    
    private:

    protected:

    public:
        CooldownProfile() : BaseProfile()
        {
           this->displayName = "Cooldown"; // 8 characters
           this->coolTemp = 50;
        }

        uint16_t getDesiredTemp() override
        {
            if (this->startTime == 0)
                return IDLE_SAFETY_TEMPERATURE;

            this->state = State::COOL;
            return this->coolTemp;
        }

        bool isReflowComplete() override
        {
            
        }
};

#endif
