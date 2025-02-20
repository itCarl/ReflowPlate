#define DEFINE_GLOBAL_VARS
#include "ReflowPlate.h"
#include <Arduino.h>

ReflowPlate::ReflowPlate()
{

}

void ReflowPlate::setup()
{
    // sanity check delay - allows reprogramming if accidently doing stupid things
    // delay(1000);
    initPins();
    initLCD();

    Serial.begin(115200);
    Serial.println("I am alive :)");

    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

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
        DEBUG_PRINTLN("LittleFS Mount Failed");
        return;
    } else{
        DEBUG_PRINTLN("Little FS Mounted Successfully");
    }

    Input = therm1->readCelsius();
    Setpoint = Input-5;

    LCD.print(".");
    delay(1000);

    initConnection();
    initServer();

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

void ReflowPlate::loop()
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

    // buffer temperature data
    TemperatureData td;
    td.timestamp = millis();
    td.pwr = (uint8_t)((Output * 100) / WindowSize);
    td.input = round(Input, 1);
    td.setpoint = round(Setpoint, 1);
    temperatureData.push(td);
    notify();

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

void ReflowPlate::reset()
{
    ws.closeAll(1012);
    DEBUG_PRINTLN("ReflowPlate Reset");
    ESP.restart();
}

void ReflowPlate::initPins()
{
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
    delay(500);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(DEBUG_LED, LOW);
}

void ReflowPlate::initLCD()
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

void ReflowPlate::initConnection()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    LCD.setCursor(0, 1);
    LCD.print("WiFi");
    LCD.setCursor(4, 1);
    WiFi.begin(clientSSID, clientPass);
    uint8_t connectionCounter = 0;

    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        LCD.print(".");
        DEBUG_PRINT("Connection Failed! SSID: ");
        DEBUG_PRINTLN(clientSSID);

        if(connectionCounter >= 5) {
            clearSecondRow();
            LCD.setCursor(0, 1);
            LCD.print("starting AP");
            DEBUG_PRINTLN("Starting a Access Point...");
            WiFi.mode(WIFI_AP);
            WiFi.softAP(apSSID);
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
}

void ReflowPlate::initAP()
{

}
