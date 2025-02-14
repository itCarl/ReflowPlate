#ifndef BaseProfile_h
#define BaseProfile_h

#include <Arduino.h>
#include "../const.h"

enum State { 
    IDLE, 
    PREHEAT, 
    SOAK, 
    REFLOW, 
    COOL, 
    COMPLETE 
};

class BaseProfile
{    
    private:

    protected:
        String displayName;
        uint16_t maxTemp = 0;
        State state = State::IDLE;

        unsigned long startTime = 0;

        uint16_t preheatTemp;
        uint16_t soakTemp;
        uint16_t reflowTemp;
        uint16_t coolTemp;

        uint32_t preheatTime;
        uint32_t soakTime;
        uint32_t reflowTime;
        uint32_t coolTime;

    public:
        BaseProfile() : displayName("Base Profile")
        {

        }

        virtual uint16_t getDesiredTemp() = 0;

        virtual void startReflow()
        {
            this->startTime = millis();
        }

        virtual bool isReflowComplete()
        {
            unsigned long totalTime = this->preheatTime + this->soakTime + this->reflowTime + this->coolTime;
            return (millis() - this->startTime) > totalTime;
        }

        unsigned long getElapsedTime()
        {
            return millis() - this->startTime;
        }

        int32_t getTimeLeft()
        {
            if (this->startTime == 0)
                return -1; // Indicates an invalid state (not started)

            if (this->getElapsedTime() <= preheatTime) {
                return (preheatTime - this->getElapsedTime()) / 1000;

            } else if (this->getElapsedTime() <= preheatTime + soakTime) {
                return ((preheatTime + soakTime) - this->getElapsedTime()) / 1000;

            } else if (this->getElapsedTime() <= preheatTime + soakTime + reflowTime) {
                return ((preheatTime + soakTime + reflowTime) - this->getElapsedTime()) / 1000;

            } else if (this->getElapsedTime() <= preheatTime + soakTime + reflowTime + coolTime) {
                return ((preheatTime + soakTime + reflowTime + coolTime) - this->getElapsedTime()) / 1000;

            } else {
                return 0; // No time left in any state
            }
        }

        virtual String showProfileState()
        {
            switch (this->state)
            {
                case State::IDLE:
                    return "idle";
                case State::PREHEAT:
                    return "preheat";
                case State::SOAK:
                    return "soak";
                case State::REFLOW:
                    return "reflow";
                case State::COOL:
                    return "cooldown";
                case State::COMPLETE:
                    return "completed";
                default:
                    return "unkown";
                    break;
            }
        }

        /*
         *      GETTER & SETTER
         */
        virtual String getDisplayName()
        {
            return this->displayName;
        }

        virtual void setDisplayName(String displayName)
        {
            this->displayName = displayName;
        }

        virtual uint16_t getMaxTemp()
        {
            return this->maxTemp;
        }
};

#endif
