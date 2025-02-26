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

#ifdef USE_PID
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
#endif

/*
 *  Menu Utillity
 */
BaseProfile* getPreviousProfile()
{
    selectedProfileIndex = selectedProfileIndex == 0 ? (sizeof(profiles)/sizeof(*profiles))-1 : --selectedProfileIndex;
    selectedProfile = profiles[selectedProfileIndex];
    DEBUG_PRINTLN("prev");
    return selectedProfile;
}

BaseProfile* getNextProfile()
{
    selectedProfileIndex = ++selectedProfileIndex % (sizeof(profiles)/sizeof(*profiles));
    selectedProfile = profiles[selectedProfileIndex];
    DEBUG_PRINTLN("next");
    return selectedProfile;
}

uint16_t readPoti()
{
    uint16_t rawValue = analogRead(POTI_1);
    uint16_t normalizedValue = map(rawValue, 0, 4095, 0, 280);
    return normalizedValue;
}
