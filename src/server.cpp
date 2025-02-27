#include "ReflowPlate.h"

/* ================================================
* |
* |  Web Server
* |
* ================================================
*/

void initServer()
{
    // Websocket
    ws.onEvent(onEvent);
    server.addHandler(&ws);

    // root route
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        // request->send(200, "text/plain", "Hello, world");
        request->send(LittleFS, "/index.html", "text/html", false, processor);
    });

    server.serveStatic("/", LittleFS, "/");

    // style route
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });

    // script route
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "text/javascript");
    });

    // not found route
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });
}

void notify()
{
    JsonDocument doc;
    String out;
    // out.reserve(2048);

    JsonObject meta = doc["meta"].to<JsonObject>();
    meta["freeHeap"] = ESP.getFreeHeap();
    meta["kp"] = Kp;
    meta["ki"] = Ki;
    meta["kd"] = Kd;
    meta["sprof"] = selectedProfile->getId();
    meta["controlsLocked"] = controlsLocked;

    JsonArray temp = doc["temp"].to<JsonArray>();
    temperatureData.serializeToJson(temp);

    serializeJson(doc, out);
    ws.textAll(out);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = '\0';
        DEBUG_PRINTLN("[WS] Message incoming.");

        JsonDocument doc;
        String out;
        DeserializationError error = deserializeJson(doc, data);
        bool sendResponse = false;

        if (error) {
            DEBUG_PRINT("[WS] json parsing error: ");
            DEBUG_PRINTLN(error.c_str());
            return;
        }

        // get command
        const char* cmd = doc["cmd"];

        if (strcmp(cmd, "upt") == 0) {
            DEBUG_PRINTLN("[WS] Update command received");
            JsonObject receivedData = doc["states"];
            controlsLocked = receivedData["controlsLocked"] | false;
            Kp = receivedData["kp"] | Kp;
            Ki = receivedData["ki"] | Ki;
            Kd = receivedData["kd"] | Kd;
            saveConfig();
            sendResponse = true; // necessary for fast UI updates

        } else if (strcmp(cmd, "getCfg") == 0) {
            DEBUG_PRINTLN("[WS] getConfig command received");
            JsonObject cfg = doc["cfg"].to<JsonObject>();
            cfg["version"] = VERSION;
            cfg["versionCode"] = VERSION_CODE;
            cfg["buildTime"] = BUILD_TIME;

            JsonArray jProfiles = cfg["profiles"].to<JsonArray>();
            for (int i = 0; i < (sizeof(profiles)/sizeof(*profiles)); i++) {
                JsonObject profile = jProfiles.add<JsonObject>();
                profiles[i]->serialize(profile);
            }
            sendResponse = true;

        } else if (strcmp(cmd, "start") == 0) {
            showMessage("Confirmed!");
            mode = 2;
            selectedProfile->startReflow();

        }  else if (strcmp(cmd, "stop") == 0) {
            showMessage("Aborted!");
            mode = 1;
            showSelectedProfile();

        }  else if (strcmp(cmd, "selProf") == 0) {
            selectProfileById(doc["data"]);
            showSelectedProfile();

        } else if (strcmp(cmd, "rstCntlr") == 0) {
            ReflowPlate::instance().reset();
        }

        if(!sendResponse)
            return;

        serializeJson(doc, out);
        ws.textAll(out);
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
