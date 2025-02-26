#pragma once
#ifndef FCNDeclare_h
#define FCNDeclare_h

// utils.cpp
inline unsigned long Seconds(unsigned long sec);
float round(float x, uint8_t decimals);
double round(double x, uint8_t decimals);
uint16_t roundToNearestFive(uint16_t num);
double computePID(double setpoint, double input, double kp, double ki, double kd);
BaseProfile* getPreviousProfile();
BaseProfile* getNextProfile();
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
void savePID();
void loadFromStorage();

// server.cpp
void initServer();
void notify();
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
String processor(const String& var);

#endif
