#include <Arduino.h>
#include <WiFi.h>
// #include <DNSServer.h>
#include <WiFiClient.h>
#include <AsyncTCP.h>
#include <ElegantOTA.h>
#include <LittleFS.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <Thermistor.h>
#include <NTC_Thermistor.h>

// Custom Libs
#include "../lib/PID_Autotuner/PIDAutotuner.h"

#include "customChars.h"
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

const char* ssid = CLIENT_SSID;
const char* password = CLIENT_PASS;

// DNSServer dnsServer;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

uint8_t connectionCounter = 0;

LiquidCrystal_I2C LCD(0x27, 16, 2);
Thermistor* therm1;
#ifdef PID_AUTOTUNE
PIDAutotuner tuner;
#endif

double Setpoint = IDLE_SAFETY_TEMPERATURE;
double Input, Output;
double LastError = 0.0, Integral = 0.0;
unsigned long lastTime = 0;

// Good but not perfect
// double Kp = 6.45;
// double Ki = 0;
// double Kd = 45;

// stable amplitude
// double Kp = 38;
// double Ki = 0;
// double Kd = 0;

// near perfect
double Kp = 21.55;
double Ki = 0.070;
double Kd = 320;

BaseProfile* profiles[] = {
  // new CooldownProfile(),
  new LowTempProfile(),
  new HighTempProfile(),
  new Test01Profile(),
};

uint8_t selectedProfileIndex = 0;
BaseProfile* selectedProfile = profiles[selectedProfileIndex];
/*
 *   0 = emergency stop
 *   1 = idle
 *   2 = executing selected profile
 *   3 = Manual Temperature Control
 *  10 = settings menu
 */
uint8_t mode = 1;

bool relayActive = false;
bool PIDTuningComplete = false;

const char emptyLine[17] = "                "; // 16 spaces + null terminator

uint16_t WindowSize = 2000; //5000
unsigned long windowStartTime = 0;

unsigned long tempControlInterval = 100;
unsigned long lastTempControl = 0;

unsigned long controlInterval = 100;
unsigned long lastControlTime = 0;

unsigned long updateInterval = 2000;
unsigned long nextUpdateTime = 0;
unsigned long lastUpdateTime = 0;

unsigned long mode3StartTime = 0;
unsigned long mode3ElapsedTime = 0;

// PIDTuner tuner(1.0, 150.0, WindowSize);

uint16_t rawPotiValue = 0;
uint16_t potiValue = 0;
uint16_t oldPotiValue = 0;

uint16_t oldSetTemp = 0;
uint16_t oldCurrentTemp = 0;

const float alpha = 0.12;
float filteredTemperature = 0.0;

bool btnPrevPressed = false;
bool btnNextPressed = false;
bool btnConfirmPressed = false;

#ifdef USE_PID
double computePID(double setpoint, double input, double kp, double ki, double kd) {
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

uint16_t roundToNearestFive(uint16_t num) {
  return ((num + 2) / 5) * 5;
}

void clearFirstRow()
{
  LCD.setCursor(0, 0);
  LCD.print(emptyLine);
}

void clearSecondRow()
{
  LCD.setCursor(0, 1);
  LCD.print(emptyLine);
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

BaseProfile* getPreviousProfile()
{
  // selectedProfileIndex = --selectedProfileIndex % (sizeof(profiles)/sizeof(*profiles)); // integer wrap around "Bug" (not really but.. u know) when negative
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
  // uint16_t normalizedValue = map(rawValue, 0, 4095, 0, 100);
  uint16_t normalizedValue = map(rawValue, 0, 4095, 0, 300);
  return normalizedValue;
}

// void clearTemperature(uint8_t startPos)
// {
//   LCD.setCursor(startPos, 1);
//   LCD.print("   ");
// }

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

void notify()
{
//   StaticJsonDocument<JSON_OBJECT_SIZE(10)> json;

//   JsonObject battery = json.createNestedObject("battery");
//   battery["voltage"] = voltage;
//   battery["capacity"] = BATTERY_CAPACITY;
//   battery["percent"] = batteryLevel;
//   battery["isCharging"] = batteryIsCharging;
//   battery["nextRead"] = nextBatteryReadTime - millis();

//   json["airflow"] = sensorValue;
//   json["startTimer"] = startTimer;
//   json["endTimer"] = endTimer;
//   json["time"] = millis();

//   char data[200];
//   size_t len = serializeJson(json, data);
//   ws.textAll(data, len);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
    if (strcmp((char*)data, "update") == 0) {
      notify();
    }
  }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      DEBUG_PRINTF("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      DEBUG_PRINTF("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      DEBUG_PRINTF("WebSocket client #%u Error\n", client->id());
      break;
  }
}

String processor(const String& var)
{
  // return "no data.";
  
  // if(var == "BATTERY_VOLTAGE") {
  //   return "0.00 v";
  // }
  // else if(var == "BATTERY_LEVEL") {
  //   return "0 &#x25;";
  // }
  // else if(var == "BATTERY_CAPACITY") {
  //   return String(BATTERY_CAPACITY) + "mAh (" + String((3.7f * BATTERY_CAPACITY)/1000) + " Wh)";
  // }
  // else if(var == "AIRFLOW") {
  //   return String(sensorValue);
  // }
  // else if(var == "VERSION") {
  //   return String(BLSS_VERSION);
  // }
  // else if(var == "ESP_CHIPID") {
  //   return String(ESP.getChipId());
  // }
  // else if(var== "WIFI_MAC") {
  //   return String(WiFi.macAddress());
  // }

  return String();
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{  
  // sanity check delay - allows reprogramming if accidently blowing power
  // delay(1000);
  pinMode(RELAY, OUTPUT);
  digitalWrite(RELAY, LOW);
  
  pinMode(BTN_CONFIRM, INPUT_PULLUP);
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);

  pinMode(POTI_1, INPUT_PULLUP);

  pinMode(THERM_1, INPUT);
  
  // Debug LED
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(DEBUG_LED, LOW);

  LCD.init();
  LCD.backlight();
  Serial.begin(115200); 
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  Serial.println("I am alive :)");

  LCD.createChar(0, arrowUp);
  LCD.createChar(1, arrowDown);
  LCD.createChar(2, coolDown);
  LCD.createChar(3, hot);
  LCD.createChar(6, hotPlateLeft);
  LCD.createChar(7, hotPlateRight);

  LCD.home();
  LCD.print("Initializing");
  delay(500);

  therm1 = new NTC_Thermistor_ESP32(
    THERM_1,
    REFERENCE_RESISTANCE,
    NOMINAL_RESISTANCE,
    NOMINAL_TEMPERATURE,
    B_VALUE,
    ESP32_ADC_VREF_MV,
    ESP32_ANALOG_RESOLUTION
  );

  if(!LittleFS.begin(true)){
    Serial.println("LittleFS Mount Failed");
    return;
  } else{
      Serial.println("Little FS Mounted Successfully");
  }

  Input = therm1->readCelsius();
  Setpoint = Input-5;

  LCD.print(".");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  LCD.setCursor(0, 1);
  LCD.print("WiFi");
  LCD.setCursor(4, 1);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    LCD.print(".");
    DEBUG_PRINT("Connection Failed! SSID: ");
    DEBUG_PRINTLN(ssid);

    if(connectionCounter >= 5) {
      clearSecondRow();
      LCD.setCursor(0, 1);
      LCD.print("starting AP");
      DEBUG_PRINTLN("Starting a Access Point...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("DEV");
      // dnsServer.start(53, "*", WiFi.softAPIP());
      server.addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER); //only when requested from AP+
      
      IPAddress IP = WiFi.softAPIP();
      LCD.clear();
      LCD.home();
      LCD.println("IP-Address:");
      LCD.print(IP);
      DEBUG_PRINT("AP IP-Address:");
      DEBUG_PRINTLN(IP);
      break;
      // Serial.println("Connection Failed! Rebooting...");
      // ESP.restart();
    }
    delay(2500);
    connectionCounter++;
  }

  LCD.setCursor(13, 0);
  LCD.print(".");
  delay(1000);
  clearSecondRow();
  LCD.setCursor(0, 1);
  LCD.print("done");
  delay(3000);
  if(connectionCounter < 5) {
    LCD.clear();
    LCD.home();
    LCD.print("IP-Address:");
    LCD.setCursor(0, 1);
    LCD.print(WiFi.localIP());
    DEBUG_PRINTLN("Ready");
    DEBUG_PRINT("IP-Address: ");
    DEBUG_PRINTLN(WiFi.localIP());
    delay(5000);
  }
  LCD.clear();
  LCD.home();
  // Websocket  
  ws.onEvent(onEvent);
  server.addHandler(&ws);

  /* ================================================
  * |
  * |  Web Server
  * |
  * ================================================
  */

  // root route
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Hello, world");
    // request->send(LittleFS, "/index.html", "text/html", false, processor);
  });
  server.serveStatic("/", LittleFS, "/");
  
  // style route
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/style.css", "text/css");
  });

  // style route
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(LittleFS, "/script.js", "text/javascript");
  });

  ElegantOTA.begin(&server);
  server.begin();
  DEBUG_PRINTLN("Web Server started");

#ifdef PID_AUTOTUNE
  LCD.print("PID tune");
  // tuner.startWarmup(5 * 1000);
  tuner.setTargetInputValue(150);
  tuner.setLoopInterval(tempControlInterval*1000);
  tuner.setOutputRange(0, WindowSize);
  tuner.setZNMode(PIDAutotuner::znModeBasicPID);
  tuner.setTuningCycles(5);
  digitalWrite(RELAY, HIGH);
  delay(1000);
  tuner.startTuningLoop();
#else
  LCD.print(selectedProfile->getDisplayName());
  showTemperatures(Input, mode == 1 ? IDLE_SAFETY_TEMPERATURE : readPoti());
#endif

  lastTime = millis();
  windowStartTime = millis();

  rawPotiValue = potiValue = oldPotiValue = roundToNearestFive(readPoti());
  DEBUG_PRINTLN(potiValue);

  PLOT_PRINTLN("Setpoint,Temp,OnTime%,Time");
  PLOT_PRINTF("PID Values: Kp:%.3f | Ki:%.3f | Kd:%.3f\n", Kp, Ki, Kd);
}

void loop() 
{

#ifdef PID_AUTOTUNE
  Input = therm1->readCelsius();
  // tuner.performRelayTest(Input);
  if(!PIDTuningComplete) {
    // if(!tuner.isTuningComplete()) {
    //   digitalWrite(RELAY, tuner.getOutput() > 0 ? HIGH : LOW);
    if (!tuner.isFinished()) {
      if((millis() - lastTempControl) > tempControlInterval) {
        lastTempControl = millis(); 
        double output = tuner.tunePID(Input);
        digitalWrite(RELAY, (output > (WindowSize / 2)) ? HIGH : LOW);
      }
    // if (tuner.getTu() <= 0 && tuner.getKu() <= 0) {
    //   float controlSignal = tuner.update(Input);
    //   digitalWrite(RELAY, (controlSignal > (WindowSize / 2)) ? HIGH : LOW);
    } else {
      // Get tuned values
      Kp = tuner.getKp();
      Ki = tuner.getKi();
      Kd = tuner.getKd();
      // tuner.calculatePIDParams(Kp, Ki, Kd);

      clearFirstRow();
      LCD.home();
      LCD.print("Tuning Complete");
      clearSecondRow();
      LCD.setCursor(0, 1);
      LCD.printf("%.2f|%.2f|%.2f", Kp, Ki, Kd);

      Serial.println("Tuning Complete!");
      Serial.print("Kp: "); Serial.println(Kp);
      Serial.print("Ki: "); Serial.println(Ki);
      Serial.print("Kd: "); Serial.println(Kd);

      PIDTuningComplete = true;
      digitalWrite(RELAY, LOW);      
      delay(30*1000); //30sec
      LCD.clear();
      showTemperatures(Input, mode == 1 ? IDLE_SAFETY_TEMPERATURE : readPoti()); 
      LCD.home();
      LCD.print(selectedProfile->getDisplayName());
    }
    if (millis() < nextUpdateTime || PIDTuningComplete) return;
    nextUpdateTime = millis() + updateInterval;
    lastUpdateTime = millis();
    showTemperature(POS_SET_TEMPERATURE, Input);
    LCD.print((char)223);
    LCD.print("C");

    LCD.setCursor(13 + (Input < 10) + (Input < 100), 0);
    LCD.print(Input);
    clearSecondRow();
    LCD.setCursor(0, 1);
    LCD.printf("%.2f|%.2f|%.2f", Kp, Ki, Kd);

    return; 
  }
#endif

  ElegantOTA.loop();
  // dnsServer.processNextRequest();


  if(millis() - lastTempControl > tempControlInterval)
  {
    lastTempControl = millis(); 
    Input = therm1->readCelsius();
    filteredTemperature = alpha * Input + (1 - alpha) * filteredTemperature;
    Input = filteredTemperature;
#ifdef USE_PID

    Output = computePID(Setpoint, Input, Kp, Ki, Kd);
    if (Output < 0 || Output > WindowSize) {
      // DEBUG_PRINTF("PID Output out of range: %f\n", Output);
      Output = constrain(Output, 0, WindowSize);
    }
#endif
    PLOT_PRINTF("%.2f,%.2f,%d,%.2f\n", Setpoint, Input, (uint8_t)((Output * 100) / WindowSize), (double)(millis() / 1000.0));
#ifndef USE_PID
    if(Input < Setpoint && (mode != 1 && Setpoint >= IDLE_SAFETY_TEMPERATURE)) {
      if(!relayActive) {
        relayActive = true;
        digitalWrite(RELAY, HIGH);
        LCD.setCursor(0, 1);
        LCD.print("1");
      }
    } else {
      if(relayActive) {
        relayActive = false;
        digitalWrite(RELAY, LOW);
        LCD.setCursor(0, 1);
        LCD.print("0");
      }
    }
#endif
  }

  if(millis() - lastControlTime > controlInterval)
  {
    rawPotiValue = readPoti();
    
    // Apply hysteresis
    if (abs(potiValue - rawPotiValue) > 4) {
        potiValue = roundToNearestFive(rawPotiValue);
    } else {
        rawPotiValue = potiValue;
    }

    if(potiValue != oldPotiValue && (mode == 1 || mode == 3)) {
      oldPotiValue = potiValue;
      Setpoint = potiValue;
      showTemperatures(oldCurrentTemp, Setpoint);
      mode = 3;
      mode3StartTime = millis();
    }

    if(!digitalRead(BTN_PREV) && btnPrevPressed && mode != 2) {
      btnPrevPressed = false;
      mode = 1;
      Setpoint = IDLE_SAFETY_TEMPERATURE;
      clearFirstRow();
      LCD.home();
      LCD.print(getPreviousProfile()->getDisplayName());
    } else if(digitalRead(BTN_PREV) && !btnPrevPressed && mode != 2) {
      btnPrevPressed = true;
    }

    if(!digitalRead(BTN_NEXT) && btnNextPressed && mode != 2) {
      btnNextPressed = false;
      mode = 1;
      Setpoint = IDLE_SAFETY_TEMPERATURE;
      clearFirstRow();
      LCD.home();
      LCD.print(getNextProfile()->getDisplayName());
    } else if(digitalRead(BTN_NEXT) && !btnNextPressed && mode != 2) {
      btnNextPressed = true;
    }

    if(!digitalRead(BTN_CONFIRM) && btnConfirmPressed) {
      btnConfirmPressed = false;
      if(mode == 1) {
        mode = 2;
        clearFirstRow();
        LCD.home();
        LCD.print("CONFIRMED!");
        delay(1000);
        selectedProfile->startReflow();
      } else if(mode == 2) {
        mode = 1;       
        clearFirstRow();
        LCD.home();
        LCD.print("Aborted!");
        delay(1000);
        LCD.home();
        LCD.print(selectedProfile->getDisplayName());
      }
    } else if(digitalRead(BTN_CONFIRM) && !btnConfirmPressed) {
      btnConfirmPressed = true;
    }

    if(mode == 0 || mode == 1) {
      Setpoint = IDLE_SAFETY_TEMPERATURE;
    } else if(mode == 2) {
      Setpoint = selectedProfile->getDesiredTemp();
    } else if(mode == 3) {
      mode3ElapsedTime = millis() - mode3StartTime;
      if(mode3ElapsedTime >= MAX_POTI_TIME) {
        mode = 1;
        clearFirstRow();
        LCD.home();
        LCD.print(selectedProfile->getDisplayName());
      }
    }

    if(mode == 2 && selectedProfile->isReflowComplete()) {
      mode = 1;
      clearFirstRow();
      LCD.home();
      LCD.print(selectedProfile->getDisplayName());
    }

    lastControlTime = millis(); 
  }
  

#ifdef USE_PID
  if (millis() - windowStartTime > WindowSize)
  { //time to shift the Relay Window
    windowStartTime += WindowSize;
  }

  unsigned long onTime = Output;
  if ((Output > (millis() - windowStartTime)) && (mode != 1 && Setpoint >= IDLE_SAFETY_TEMPERATURE)) {
    if(!relayActive) {
      relayActive = true;
      digitalWrite(RELAY, HIGH);
      // PLOT_PRINTF("Time in Window: %lums\n", (millis() - windowStartTime));
      // PLOT_PRINTF("On Time Remaining: %lums\n", ((unsigned long)Output - (millis() - windowStartTime)));
      // PLOT_PRINTLN(String(1) + "," + String(millis()) + "," + String(Output) + "," + String(millis() - windowStartTime) + "," + String(Setpoint));
      // LCD.setCursor(0, 1);
      // LCD.print("1");
      // LCD.printf("%3d%%", (uint8_t)((Output * 100) / WindowSize));
    }
  } else {
    if(relayActive) {
      relayActive = false;
      digitalWrite(RELAY, LOW);
      // PLOT_PRINTF("Time in Window: %lums\n", (millis() - windowStartTime));
      // PLOT_PRINTF("Off Time Remaining: %lums\n\n", (WindowSize - onTime) - ((millis() - windowStartTime) - onTime));
      // PLOT_PRINTLN(String(0) + "," + String(PLOT_PRINTFmillis()) + "," + String(Output) + "," + String(millis() - windowStartTime) + "," + String(Setpoint));
      // LCD.setCursor(0, 1);
      // LCD.print("0");
      // LCD.printf("%3d%%", 0);
    }
  }
#endif

  if (millis() < nextUpdateTime) return;
  nextUpdateTime = millis() + updateInterval;
  lastUpdateTime = millis();
  // LCD.clear(); looks ugly

  LCD.home();
  // LCD.write(0);
  // LCD.write(1);
  // LCD.write(2);  
  // LCD.write(3);
  // LCD.write(6);
  // LCD.write(7);
  if(mode == 1){

  } else if(mode == 2) {
    printSpaceBetween(selectedProfile->showProfileState(), String(selectedProfile->getTimeLeft()) + "s");
  } else if(mode == 3) {
    printSpaceBetween("PotiControl", String(mode3ElapsedTime / 1000) + "s");
  }
  
  showTemperatures(Input <= 5 ? therm1->readCelsius() : Input, Setpoint);
  oldCurrentTemp = Input;
  oldSetTemp = Setpoint;


  DEBUG_PRINTLN("--DEBUG--");
#ifdef USE_PID
  DEBUG_PRINTLN(Output);
  DEBUG_PRINTLN(millis() - windowStartTime);
  if(relayActive) {
    DEBUG_PRINTF("Relay: %3d%%\n", (uint8_t)((Output * 100) / WindowSize));
    LCD.setCursor(0, 1);
    LCD.printf("%3d%%", (uint8_t)((Output * 100) / WindowSize));
  } else {
    DEBUG_PRINTF("Relay: %3d%%\n", 0);
    LCD.setCursor(0, 1);
    LCD.printf("%3d%%", 0);
  }
#endif
  DEBUG_PRINTF("Mode: %d\n", mode);
  DEBUG_PRINTF("Relay State: %s\n", relayActive ? "On" : "Off");
  DEBUG_PRINTF("Free Heap: %d\n", ESP.getFreeHeap());
  DEBUG_PRINTLN("");
}
