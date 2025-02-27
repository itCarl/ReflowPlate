#include "ReflowPlate.h"

void initStorage()
{
    if(!LittleFS.begin(true)){
        DEBUG_PRINTLN("LittleFS Mount Failed");
        return;
    } else{
        DEBUG_PRINTLN("Little FS Mounted Successfully");
    }

    loadConfig();
}

void loadConfig()
{
    storage.begin("cfg", false);

    Kp = storage.getDouble("Kp", Kp);
    Ki = storage.getDouble("Ki", Ki);
    Kd = storage.getDouble("Kd", Kd);

    storage.end();
}

void saveConfig()
{
    storage.begin("cfg", false);

    storage.putDouble("Kp", Kp);
    storage.putDouble("Ki", Ki);
    storage.putDouble("Kd", Kd);

    storage.end();
}
