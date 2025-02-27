#include "ReflowPlate.h"
#include "fcn_declare.h"
#include "const.h"

inline unsigned long Seconds(unsigned long sec)
{
    return sec * 1000;
}

float round(float x, uint8_t decimals)
{
    float multiplier = powf(10, decimals);
    float nx = (int)(x * multiplier + 0.5f);
    return nx / multiplier;
}

double round(double x, uint8_t decimals)
{
    double multiplier = powf(10, decimals);
    double nx = (int)(x * multiplier + 0.5f);
    return nx / multiplier;
}

uint16_t roundToNearestFive(uint16_t num)
{
    return ((num + 2) / 5) * 5;
}

double computePID(double setpoint, double input, double kp, double ki, double kd)
{
    static double lastError = 0.0, integral = 0.0;
    double deltaTime = (millis() - lastTime) / 1000.0;  // Convert ms to seconds
    lastTime = millis();

    if (deltaTime <= 0) return 0; // Prevent division by zero

    double error = setpoint - input;
    integral += error * deltaTime;
    double derivative = (error - lastError) / deltaTime;

    // Prevent integral wind-up
    integral = constrain(integral, -WindowSize, WindowSize);

    lastError = error;

    return (kp * error) + (ki * integral) + (kd * derivative);
}

/*
 *  Profile Utillity
 */
BaseProfile* getPreviousProfile()
{
    return selectProfileByIndex(selectedProfileIndex == 0 ? (sizeof(profiles)/sizeof(*profiles))-1 : --selectedProfileIndex);
}

BaseProfile* getNextProfile()
{
    return selectProfileByIndex(++selectedProfileIndex % (sizeof(profiles)/sizeof(*profiles)));
}

BaseProfile* selectProfileByIndex(uint8_t index)
{
    uint8_t numProfiles = (sizeof(profiles)/sizeof(*profiles));
    if(index > numProfiles)
        return selectedProfile;

    selectedProfileIndex = index;
    selectedProfile = profiles[selectedProfileIndex];

    DEBUG_PRINTF("Selected profile at index %d with name: %s", selectedProfileIndex, selectedProfile->getDisplayName().c_str());
    return selectedProfile;
}

BaseProfile* selectProfileById(long id)
{
    uint8_t numProfiles = (sizeof(profiles) / sizeof(*profiles));

    for (uint8_t i = 0; i < numProfiles; i++) {
        if (profiles[i]->getId() == id) {
            selectedProfileIndex = i;
            selectedProfile = profiles[selectedProfileIndex];

            DEBUG_PRINTF("Selected profile with ID %ld at index %d with name: %s", id, selectedProfileIndex, selectedProfile->getDisplayName().c_str());
            return selectedProfile;
        }
    }

    DEBUG_PRINTLN("Profile with the given ID not found.");
    return selectedProfile;
}

uint16_t readPoti()
{
    uint16_t rawValue = analogRead(POTI_1);
    uint16_t normalizedValue = map(rawValue, 0, 4095, 0, 280);
    return normalizedValue;
}
