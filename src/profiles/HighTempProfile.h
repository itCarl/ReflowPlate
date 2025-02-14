#ifndef HighTempProfile_h
#define HighTempProfile_h

#include <Arduino.h>
#include "BaseProfile.h"
#include "../const.h"

class HighTempProfile : public BaseProfile
{    
    private:

    protected:

    public:
        HighTempProfile() : BaseProfile()
        {
           this->displayName = "HighTemp Profile"; // 15 characters

            this->preheatTemp = 150;
            this->soakTemp = 200;
            this->reflowTemp = 260;
            this->coolTemp = 70;

            this->preheatTime = 60000;
            this->soakTime = 90000;
            this->reflowTime = 60000;
            this->coolTime = 90000;
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
