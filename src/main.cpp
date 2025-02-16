#include <Arduino.h>

/*
 * Arduino IDE compatibility file.
 */

#include "ReflowPlate.h"

void setup()
{ 
    ReflowPlate::instance().setup();
}

void loop() 
{
    ReflowPlate::instance().loop();
}
