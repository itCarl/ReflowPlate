#ifndef LowTempProfile_h
#define LowTempProfile_h

#include <Arduino.h>
#include "BaseProfile.h"
#include "../const.h"

class LowTempProfile : public BaseProfile
{    
    private:

    protected:

    public:
        LowTempProfile() : BaseProfile()
        {
           this->displayName = "LowTemp Profile"; // 15 characters

            this->preheatTemp = 90;
            this->soakTemp = 120;
            this->reflowTemp = 165;
            this->coolTemp = IDLE_SAFETY_TEMPERATURE;

            this->preheatTime = 30 * 1000;
            this->soakTime = 75 * 1000;
            this->reflowTime = 60 * 1000;
            this->coolTime = 3 * 60 * 1000;
        }

        uint16_t getDesiredTemp() override
        {
            if (this->startTime == 0)
                return IDLE_SAFETY_TEMPERATURE;

            unsigned long elapsed = millis() - this->startTime;

            if (elapsed <= preheatTime){
                this->state = State::PREHEAT;
                return preheatTemp;

            } else if (elapsed <= preheatTime + soakTime) {
                this->state = State::SOAK;
                return soakTemp;

            } else if (elapsed <= preheatTime + soakTime + reflowTime) {
                this->state = State::REFLOW;
                return reflowTemp;

            }  else {
                this->state = State::COOL;
                return coolTemp;
            }
        }
};

#endif
