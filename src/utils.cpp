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

/*
 *  LCD Utillity
 */
void clearRow(uint8_t y)
{
    const char emptyLine[17] = "                "; // 16 spaces + null terminator
    LCD.setCursor(0, y);
    LCD.print(emptyLine);
}

void clearFirstRow()
{
    clearRow(0);
}

void clearSecondRow()
{
    clearRow(1);
}

void printSpaceBetween(String left, String right)
{
    LCD.home();
    clearFirstRow();

    int leftLength = left.length();
    int rightLength = right.length();
    int spaceBetween = 16 - (leftLength + rightLength);

    if (spaceBetween < 0) {
        LCD.home();
        LCD.print("Text too long");
        return;
    }

    LCD.home();
    LCD.print(left);
    LCD.setCursor(leftLength + spaceBetween, 0);
    LCD.print(right);
}

void showTemperature(uint8_t startPos, uint16_t temp)
{
    // clearTemperature(startPos);
    uint8_t pos = startPos;
    if(temp < 500) {
        // pos += (temp < 10) + (temp < 100);
        LCD.setCursor(pos, 1);
        // LCD.print(temp);
        LCD.printf("%3d", temp);
    } else {
        LCD.setCursor(pos, 1);
        LCD.print("Err");
    }
}

void showTemperatures(uint16_t currentTemp, uint16_t setTemp)
{
    LCD.setCursor(6, 1);
    if(setTemp > currentTemp+5 && setTemp > IDLE_SAFETY_TEMPERATURE) {
        LCD.write(0);
    } else if(setTemp < currentTemp-5 && setTemp > IDLE_SAFETY_TEMPERATURE) {
        LCD.write(1);
    } else {
        LCD.print("-");
    }

    if(currentTemp != oldCurrentTemp)
        showTemperature(POS_CURRENT_TEMPERATURE, currentTemp);

    LCD.setCursor(POS_CURRENT_TEMPERATURE+3, 1);
    LCD.print("/");

    if(setTemp != oldSetTemp)
        showTemperature(POS_SET_TEMPERATURE, setTemp);

    LCD.setCursor(14, 1);
    LCD.print((char)223);
    LCD.print("C");

    DEBUG_PRINTF("%d/%d°C\n", currentTemp, setTemp);
}
