#include "ReflowPlate.h"

void initStorage()
{
    if(!LittleFS.begin(true)){
        DEBUG_PRINTLN("LittleFS Mount Failed");
        return;
    } else{
        DEBUG_PRINTLN("Little FS Mounted Successfully");
    }

    storage.begin("rp", false);
    loadFromStorage();
}

void loadFromStorage()
{
    Kp = storage.getDouble("Kp", Kp);
    Ki = storage.getDouble("Kp", Ki);
    Kd = storage.getDouble("Kp", Kd);
}

void savePID()
{
    storage.putDouble("Kp", Kp);
    storage.putDouble("Ki", Ki);
    storage.putDouble("Kd", Kd);
}
