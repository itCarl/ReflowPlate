#ifndef const_h
#define const_h


/*
 *      GENERAL DEFINITIONS
 */
#define IDLE_SAFETY_TEMPERATURE 15
#define MAX_POTI_TIME 30 * 60 * 1000 // in ms; 5 minutes
#define PRINT_DEBUG // OR
// #define PRINT_PLOT
#define USE_PID
// #define PID_AUTOTUNE

/*
 *      GPIO definitions
 */
#define BTN_CONFIRM 26
#define BTN_NEXT 32
#define BTN_PREV 27

#define POTI_1 39

#define THERM_1 36
// #define THERM_2 35

#define RELAY 17

#define DEBUG_LED LED_BUILTIN

/*
 *      Thermister
 */
#define REFERENCE_RESISTANCE    10000
#define NOMINAL_RESISTANCE      100000
#define NOMINAL_TEMPERATURE     25
#define B_VALUE                 3950
#define ESP32_ANALOG_RESOLUTION 4095
#define ESP32_ADC_VREF_MV       3300

/*
 *  LCD
 */
#define POS_CURRENT_TEMPERATURE 7
#define POS_SET_TEMPERATURE 11




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

#endif
