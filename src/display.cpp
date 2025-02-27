#include "ReflowPlate.h"

void initLCD()
{
    LCD.init();
    LCD.backlight();

    // load custom characters
    LCD.createChar(0, arrowUp);
    LCD.createChar(1, arrowDown);
    LCD.createChar(2, coolDown);
    LCD.createChar(3, hot);
    LCD.createChar(6, hotPlateLeft);
    LCD.createChar(7, hotPlateRight);

    LCD.home();
    LCD.print("Initializing");
}

void showOutputPower()
{
    const uint8_t pwr = !relayActive ? 0 : (Output * 100) / WindowSize;
    DEBUG_PRINTF("Relay: %3d%%\n", pwr);
    LCD.setCursor(0, 1);
    LCD.printf("%3d%%", pwr);
}

void showMessage(const char* msg)
{
    clearFirstRow();
    LCD.print(msg);
    delay(1000);
}

void showSelectedProfile()
{
    clearFirstRow();
    LCD.print(selectedProfile->getDisplayName());
}

void showPreviousProfile()
{
    clearFirstRow();
    LCD.print(getPreviousProfile()->getDisplayName());
}

void showNextProfile()
{
    clearFirstRow();
    LCD.print(getNextProfile()->getDisplayName());
}

/*
 *
 * Utillitiy functions
 *
 */
void clearRow(uint8_t y)
{
    const char emptyLine[17] = "                "; // 16 spaces + null terminator
    LCD.setCursor(0, y);
    LCD.print(emptyLine);
    LCD.setCursor(0, y);
}

void clearFirstRow()
{
    clearRow(0);
}

void clearSecondRow()
{
    clearRow(1);
}

void clearRowSegment(uint8_t y, uint8_t start, uint8_t end) {
    if (start > end || end >= 16) return; // Invalid range

    LCD.setCursor(start, y);
    for (uint8_t i = start; i <= end; i++) {
        LCD.print(" ");
    }
    LCD.setCursor(start, y);
}

void printSpaceBetween(String left, String right)
{
    static String leftOld = "";
    static String rightOld = "";

    if (leftOld == left && rightOld == right) return; // No update needed
    if (leftOld != left && rightOld != right) clearFirstRow();

    uint8_t leftLength = left.length();
    uint8_t rightLength = right.length();
    uint8_t spaceBetween = 16 - (leftLength + rightLength);
    uint8_t offset = leftLength + spaceBetween;

    if (spaceBetween < 0) {
        LCD.home();
        LCD.print("Text too long");
        return;
    }

    LCD.home();
    if(leftOld != left) {
        clearRowSegment(0, 0, leftLength);
        LCD.print(left);
        leftOld = left;
    }

    if(rightOld != right) {
        clearRowSegment(0, offset, 15);
        LCD.setCursor(offset, 0);
        LCD.print(right);
        rightOld = right;
    }
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

