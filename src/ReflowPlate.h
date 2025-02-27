#pragma once
#ifndef ReflowPlate_h
#define ReflowPlate_h

#include <WiFi.h>
// #include <DNSServer.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ElegantOTA.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>

// Custom Libs
#include "../lib/PID_Autotuner/PIDAutotuner.h"
#include "RingBuffer.h"

#include "profiles/BaseProfile.h"
#include "const.h"
#include "fcn_declare.h"
#include "config.h"

// web classes
#include "web/CaptiveRequestHandler.h"

// profiles
// #include "profiles/CooldownProfile.h"
#include "profiles/LowTempProfile.h"
#include "profiles/HighTempProfile.h"
#include "profiles/Test01Profile.h"

#ifndef VERSION_CODE
    #define VERSION_CODE "unkown"
#endif

#ifndef VERSION
    #define VERSION "err"
#endif

#ifndef BUILD_TIME
    #define BUILD_TIME "unkown"
#endif

#ifndef CLIENT_SSID
    #define CLIENT_SSID "none"
#endif

#ifndef CLIENT_PASS
    #define CLIENT_PASS ""
#endif

// GLOBAL VARIABLES
// both declared and defined in header (solution from http://www.keil.com/support/docs/1868.htm)
//
//e.g. byte test = 2 becomes RP_GLOBAL byte test _INIT(2);
//     int arr[]{0,1,2} becomes RP_GLOBAL int arr[] _INIT_N(({0,1,2}));
#ifndef DEFINE_GLOBAL_VARS
    #define RP_GLOBAL extern
    #define _INIT(x)
    #define _INIT_N(x)
#else
    #define RP_GLOBAL
    #define _INIT(x) = x
    #define UNPACK( ... ) __VA_ARGS__
    #define _INIT_N(x) UNPACK x
#endif

RP_GLOBAL char clientSSID[33] _INIT(CLIENT_SSID);
RP_GLOBAL char clientPass[65] _INIT(CLIENT_PASS);

RP_GLOBAL char apSSID[33] _INIT("ReflowPlate");
// RP_GLOBAL char apPass[65]  _INIT(DEFAULT_AP_PASS);

RP_GLOBAL AsyncWebServer server _INIT_N(((80)));
RP_GLOBAL AsyncWebSocket ws _INIT_N((("/ws")));

RP_GLOBAL Preferences storage;

RP_GLOBAL LiquidCrystal_I2C LCD _INIT_N(((0x27, 16, 2)));
RP_GLOBAL Thermistor* therm1;
#ifdef PID_AUTOTUNE
RP_GLOBAL PIDAutotuner tuner;
#endif

struct TemperatureData {
    unsigned long timestamp;    // 4 bytes (on ESP32)
    uint8_t pwr;                // 1 byte  (on ESP32)
    double input;               // 8 bytes (on ESP32)
    double setpoint;            // 8 bytes (on ESP32)

    void serialize(JsonObject& obj) const {
        obj["timestamp"] = timestamp;
        obj["pwr"] = pwr;
        obj["temp"] = input;
        obj["setpoint"] = setpoint;
    }
};
RP_GLOBAL RingBuffer<TemperatureData> temperatureData _INIT_N(((150)));

RP_GLOBAL double Setpoint _INIT(IDLE_SAFETY_TEMPERATURE);
RP_GLOBAL double Input _INIT(0);
RP_GLOBAL double Output _INIT(0);
RP_GLOBAL double LastError _INIT(0.0);
RP_GLOBAL double Integral _INIT(0.0);
RP_GLOBAL unsigned long lastTime _INIT(0);

// it's okay
RP_GLOBAL double Kp _INIT(21.55);
RP_GLOBAL double Ki _INIT(0.070);
RP_GLOBAL double Kd _INIT(320);

RP_GLOBAL BaseProfile* profiles[3] _INIT_N(({
    // new CooldownProfile(),
    new LowTempProfile(),
    new HighTempProfile(),
    new Test01Profile(),
}));

RP_GLOBAL uint8_t selectedProfileIndex _INIT(0);
RP_GLOBAL BaseProfile* selectedProfile _INIT(profiles[selectedProfileIndex]);
/*
 *   0 = emergency stop
 *   1 = idle
 *   2 = executing selected profile
 *   3 = Manual Temperature Control
 *  10 = settings menu
 */
RP_GLOBAL uint8_t mode _INIT(1);

RP_GLOBAL bool controlsLocked _INIT(false);

RP_GLOBAL bool relayActive _INIT(false);
RP_GLOBAL bool PIDTuningComplete _INIT(false);

RP_GLOBAL uint16_t WindowSize _INIT(2000); // 5000
RP_GLOBAL unsigned long windowStartTime _INIT(0);

RP_GLOBAL unsigned long tempControlInterval _INIT(100);
RP_GLOBAL unsigned long lastTempControl _INIT(0);

RP_GLOBAL unsigned long controlInterval _INIT(100);
RP_GLOBAL unsigned long lastControlTime _INIT(0);

RP_GLOBAL unsigned long updateInterval _INIT(1000);
RP_GLOBAL unsigned long nextUpdateTime _INIT(0);
RP_GLOBAL unsigned long lastUpdateTime _INIT(0);

RP_GLOBAL unsigned long mode3StartTime _INIT(0);
RP_GLOBAL unsigned long mode3ElapsedTime _INIT(0);

// PIDTuner tuner(1.0, 150.0, WindowSize);

RP_GLOBAL uint16_t rawPotiValue _INIT(0);
RP_GLOBAL uint16_t oldRawPotiValue _INIT(0);
RP_GLOBAL uint16_t potiValue _INIT(0);
RP_GLOBAL bool potiValueChanged _INIT(false);
RP_GLOBAL unsigned long potiSettleTime _INIT(850);
RP_GLOBAL unsigned long lastPotiChangeTime _INIT(0);

RP_GLOBAL uint16_t oldSetTemp _INIT(0);
RP_GLOBAL uint16_t oldCurrentTemp _INIT(0);

RP_GLOBAL const float alpha _INIT(0.12);
RP_GLOBAL float filteredTemperature _INIT(0.0);

RP_GLOBAL bool btnPrevPressed _INIT(false);
RP_GLOBAL bool btnNextPressed _INIT(false);
RP_GLOBAL bool btnConfirmPressed _INIT(false);

RP_GLOBAL uint8_t arrowUp[8] _INIT_N(({
    0x04, 0x0E, 0x15, 0x04, 0x04, 0x04, 0x04, 0x04
}));

RP_GLOBAL uint8_t arrowDown[8] _INIT_N(({
    0x04, 0x04, 0x04, 0x04, 0x04, 0x15, 0x0E, 0x04
}));

RP_GLOBAL uint8_t coolDown[8] _INIT_N(({
    0x04, 0x0A, 0x0A, 0x0A, 0x0A, 0x11, 0x11, 0x0E
}));

RP_GLOBAL uint8_t hot[8] _INIT_N(({
    0x04, 0x0A, 0x0E, 0x0E, 0x0E, 0x1F, 0x1F, 0x0E
}));

RP_GLOBAL uint8_t hotPlateLeft[8] _INIT_N(({
    0x04, 0x04, 0x02, 0x00, 0x1F, 0x08, 0x08, 0x08
}));

RP_GLOBAL uint8_t hotPlateRight[8] _INIT_N(({
    0x12, 0x09, 0x0A, 0x00, 0x1F, 0x02, 0x02, 0x02
}));

#ifdef PRINT_DEBUG
    #define DEBUG_PRINT(x) Serial.print(x)
    #define DEBUG_PRINTLN(x) Serial.println(x)
    #define DEBUG_PRINTF(x...) Serial.printf(x)
#else
    #define DEBUG_PRINT(x)
    #define DEBUG_PRINTLN(x)
    #define DEBUG_PRINTF(x...)
#endif

#ifdef PRINT_PLOT
    #define PLOT_PRINTLN(x) Serial.println(x)
    #define PLOT_PRINTF(x...) Serial.printf(x)
#else
    #define PLOT_PRINTLN(x)
    #define PLOT_PRINTF(x...)
#endif

class ReflowPlate
{
    public:
        ReflowPlate();
        static ReflowPlate& instance()
        {
            static ReflowPlate instance;
            return instance;
        }

        void setup();
        void loop();
        void reset();
        void restart();

        void initPins();
        void initConnection();
        void initAP();
};

#endif
