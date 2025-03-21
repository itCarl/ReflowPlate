#pragma once
#ifndef FCNDeclare_h
#define FCNDeclare_h

#include "ReflowPlate.h"

// utils.cpp
inline unsigned long Seconds(unsigned long sec);
float round(float x, uint8_t decimals);
double round(double x, uint8_t decimals);
uint16_t roundToNearestFive(uint16_t num);
template <typename T>
void smooth(T& filteredValue, T newValue, float alpha)
{
    filteredValue = alpha * newValue + (1 - alpha) * filteredValue;
}
double computePID(double setpoint, double input, double kp, double ki, double kd);
BaseProfile* getPreviousProfile();
BaseProfile* getNextProfile();
BaseProfile* selectProfileByIndex(uint8_t index);
BaseProfile* selectProfileById(long id);
uint16_t readPoti();

// display.cpp
void initLCD();
void showOutputPower();
void showMessage(const char* msg);
void showSelectedProfile();
void showPreviousProfile();
void showNextProfile();
void clearRow(uint8_t y);
void clearFirstRow();
void clearSecondRow();
void printSpaceBetween(String left, String right);
void showTemperature(uint8_t startPos, uint16_t temp);
void showTemperatures(uint16_t currentTemp, uint16_t setTemp);

// storage.cpp
void initStorage();
void loadConfig();
void saveConfig();

// server.cpp
void initServer();
void notify();
void notfiyPwr();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
String processor(const String& var);

#endif
