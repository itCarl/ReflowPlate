#ifndef Test01Profile_h
#define Test01Profile_h

#include <Arduino.h>
#include "BaseProfile.h"
#include "../const.h"

class Test01Profile : public BaseProfile
{    
    private:

    protected:

    public:
        Test01Profile() : BaseProfile()
        {
           this->displayName = "Test01 Profile"; // 15 characters

            this->preheatTemp = 120;
            this->soakTemp = 150;
            this->reflowTemp = 180;
            this->coolTemp = 50;

            this->preheatTime = 60000;
            this->soakTime = 90000;
            this->reflowTime = 60000;
            this->coolTime = 90000;
        }

        uint16_t getDesiredTemp() override
        {
            if (this->startTime == 0)
            {
                return IDLE_SAFETY_TEMPERATURE;
            }

            unsigned long elapsed = millis() - this->startTime;

            // Determine the current phase based on elapsed time
            if (elapsed <= preheatTime)
            {
                // Preheat phase: ramp up to preheatTemp
                return map(elapsed, 0, preheatTime, 25, preheatTemp);

            } else if (elapsed <= preheatTime + soakTime) {
                // Soak phase: hold at soakTemp
                return soakTemp;

            } else if (elapsed <= preheatTime + soakTime + reflowTime) {
                // Reflow phase: ramp up to reflowTemp and then down
                unsigned long phaseElapsed = elapsed - (preheatTime + soakTime);
                return map(phaseElapsed, 0, reflowTime, soakTemp, reflowTemp);

            } else if (elapsed <= preheatTime + soakTime + reflowTime + coolTime) {
                // Cool phase: ramp down to coolTemp
                unsigned long phaseElapsed = elapsed - (preheatTime + soakTime + reflowTime);
                return map(phaseElapsed, 0, coolTime, reflowTemp, coolTemp);
                
            } else {
                // Reflow complete, hold at coolTemp
                return coolTemp;
            }
        }
};

#endif
