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

    //
    server.onNotFound([](AsyncWebServerRequest *request){
        request->send(404, "text/plain", "Not found");
    });
}

void notify()
{
    JsonDocument doc;
    String out;
    out.reserve(2048);    // optional

    JsonObject meta = doc["meta"].to<JsonObject>();
    JsonArray temp = doc["temp"].to<JsonArray>();    
    temperatureData.serializeToJson(temp);

    // char buffer[2000];
    // size_t len = serializeJson(doc, buffer);
    // ws.textAll(buffer, len);

    serializeJson(doc, out);
    ws.textAll(out);
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) 
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        data[len] = 0;
        if (strcmp((char*)data, "update") == 0) {
            notify();
            DEBUG_PRINTLN("WS message received.");
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