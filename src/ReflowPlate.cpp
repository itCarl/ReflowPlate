#define DEFINE_GLOBAL_VARS
#include "ReflowPlate.h"
#include <Arduino.h>

ReflowPlate::ReflowPlate()
{
    //
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
    randomSeed(esp_random());   // prepare for random number generation

    therm1 = new NTC_Thermistor_ESP32(
        THERM_1,
        REFERENCE_RESISTANCE,
        NOMINAL_RESISTANCE,
        NOMINAL_TEMPERATURE,
        B_VALUE,
        ESP32_ADC_VREF_MV,
        ESP32_ANALOG_RESOLUTION
    );

    initStorage();

    Input = therm1->readCelsius();

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

    rawPotiValue = oldRawPotiValue = readPoti();
    potiValue = roundToNearestFive(rawPotiValue);
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

    if(millis() - lastTempControl > tempControlInterval)
    {
        lastTempControl = millis();
        Input = therm1->readCelsius();
        filteredTemperature = alpha * Input + (1 - alpha) * filteredTemperature;
        Input = filteredTemperature;
        Output = computePID(Setpoint, Input, Kp, Ki, Kd);
        if (Output < 0 || Output > WindowSize) {
            // DEBUG_PRINTF("PID Output out of range: %f\n", Output);
            Output = constrain(Output, 0, WindowSize);
        }
        PLOT_PRINTF("%.2f,%.2f,%d,%.2f\n", Setpoint, Input, (uint8_t)((Output * 100) / WindowSize), (double)(millis() / 1000.0));


        notfiyPwr();
    }



    if(millis() - lastControlTime > controlInterval)
    {
        rawPotiValue = readPoti();

        // Apply hysteresis
        if (abs(oldRawPotiValue - rawPotiValue) > 4) {
            oldRawPotiValue = rawPotiValue;
            potiValue = roundToNearestFive(rawPotiValue);
            lastPotiChangeTime = millis();
            potiValueChanged = true;
        }

        if(potiValueChanged && (mode == 1 || mode == 3)) {
            Setpoint = potiValue;
            showTemperatures(oldCurrentTemp, Setpoint);
            printSpaceBetween("Setting...", " ");    // ugly fix
            mode = 3;
            mode3StartTime = millis();
            potiValueChanged = false;
        }

        if(!digitalRead(BTN_PREV) && btnPrevPressed && mode != 2) {
            btnPrevPressed = false;
            if(mode != 3) showPreviousProfile();
            if(mode == 3) showSelectedProfile();
            mode = 1;
            Setpoint = IDLE_SAFETY_TEMPERATURE;
        } else if(digitalRead(BTN_PREV) && !btnPrevPressed && mode != 2) {
            btnPrevPressed = true;
        }

        if(!digitalRead(BTN_NEXT) && btnNextPressed && mode != 2) {
            btnNextPressed = false;
            if(mode != 3) showNextProfile();
            if(mode == 3) showSelectedProfile();
            mode = 1;
            Setpoint = IDLE_SAFETY_TEMPERATURE;
        } else if(digitalRead(BTN_NEXT) && !btnNextPressed && mode != 2) {
            btnNextPressed = true;
        }

        if(!digitalRead(BTN_CONFIRM) && btnConfirmPressed) {
            btnConfirmPressed = false;
            if(mode == 1) {
                mode = 2;
                showMessage("Confirmed!");
                selectedProfile->startReflow();
            } else if(mode == 2) {
                mode = 1;
                showMessage("Aborted!");
                showSelectedProfile();
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
                showSelectedProfile();
            }
        }

        if(mode == 2 && selectedProfile->isReflowComplete()) {
            mode = 1;
            showSelectedProfile();
        }

        lastControlTime = millis();
    }

    if (millis() - windowStartTime > WindowSize) {
        windowStartTime += WindowSize;
    }

    unsigned long onTime = Output;
    if ((Output > (millis() - windowStartTime)) && (mode != 1 || Setpoint >= IDLE_SAFETY_TEMPERATURE)) {
        if(!relayActive) {
            relayActive = true;
            digitalWrite(RELAY, HIGH);
            // PLOT_PRINTF("Time in Window: %lums\n", (millis() - windowStartTime));
            // PLOT_PRINTF("On Time Remaining: %lums\n", ((unsigned long)Output - (millis() - windowStartTime)));
            // PLOT_PRINTLN(String(1) + "," + String(millis()) + "," + String(Output) + "," + String(millis() - windowStartTime) + "," + String(Setpoint));
        }
    } else {
        if(relayActive) {
            relayActive = false;
            digitalWrite(RELAY, LOW);
            // PLOT_PRINTF("Time in Window: %lums\n", (millis() - windowStartTime));
            // PLOT_PRINTF("Off Time Remaining: %lums\n\n", (WindowSize - onTime) - ((millis() - windowStartTime) - onTime));
            // PLOT_PRINTLN(String(0) + "," + String(PLOT_PRINTFmillis()) + "," + String(Output) + "," + String(millis() - windowStartTime) + "," + String(Setpoint));
        }
    }

    if (millis() < nextUpdateTime) return;
    nextUpdateTime = millis() + updateInterval;
    lastUpdateTime = millis();
    // LCD.clear(); looks ugly only update the chars that actualy need to be updated

    LCD.home();
    if(mode == 1){

    } else if(mode == 2) {
        printSpaceBetween(selectedProfile->showProfileState(), String(selectedProfile->getTimeLeft()) + "s");
    } else if(mode == 3 && (millis() - lastPotiChangeTime >= potiSettleTime)) {
        printSpaceBetween("PotiControl", String(mode3ElapsedTime / 1000) + "s");
    }

    showTemperatures(Input <= 5 ? therm1->readCelsius() : Input, Setpoint);
    oldCurrentTemp = Input;
    oldSetTemp = Setpoint;

    // buffer temperature data
    TemperatureData td;
    td.timestamp = millis();
    td.input = round(Input, 1);
    td.setpoint = round(Setpoint, 1);
    temperatureData.push(td);
    notify();

    DEBUG_PRINTLN("--DEBUG--");
    DEBUG_PRINTLN(Output);
    DEBUG_PRINTLN(millis() - windowStartTime);
    showOutputPower();
    DEBUG_PRINTF("Mode: %d\n", mode);
    DEBUG_PRINTF("Relay State: %s\n", relayActive ? "On" : "Off");
    DEBUG_PRINTF("Free Heap: %d\n", ESP.getFreeHeap());
    DEBUG_PRINTLN("");

    if(!initialized) {
        initialized = true;
    }
}

void ReflowPlate::restart()
{
    ws.closeAll(1012);
    WiFi.disconnect();
    DEBUG_PRINTLN("ReflowPlate restart");
    ESP.restart();
}

void ReflowPlate::reset()
{
    DEBUG_PRINTLN("ReflowPlate reset");
    storage.clear();
    ReflowPlate::instance().restart();
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

void ReflowPlate::initConnection()
{
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    LCD.setCursor(0, 1);
    LCD.print("WiFi");
    LCD.setCursor(4, 1);
    WiFi.begin(clientSSID, clientPass);
    uint8_t connectionCounter = 0;

    while (WiFi.waitForConnectResult(1000) != WL_CONNECTED) {
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
        initAP();
    }
    LCD.clear();
    LCD.home();
}

void ReflowPlate::initAP()
{
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
