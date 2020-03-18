
#ifndef DEVICE_SETTINGS

#define DEVICE_SETTINGS 1

#ifdef ARDUINO_SAMD_MKR1000
#include <WiFi101.h> // MKR 1000
#define PR_PIN    0
#define RESET_PIN 7

#define NEXT_BUTTON_PIN -1
#define PRIOR_BUTTON_PIN  -1

#endif

#ifdef ARDUINO_SAMD_MKRWIFI1010
#include <WiFiNINA.h> // MKR 1010

#define PR_PIN    0
#define RESET_PIN 7

#define NEXT_BUTTON_PIN -1
#define PRIOR_BUTTON_PIN  -1

#endif
#ifdef ARDUINO_ESP32_DEV

#include <WiFi.h> // ESP32


#define NEXT_BUTTON_PIN   25
#define PRIOR_BUTTON_PIN  27

#endif

#ifdef ARDUINO_ESP8266_ESP12 // Huzzah board

#include <ESP8266WiFi.h>

#define PR_PIN    0 /* TBD */
#define RESET_PIN 7 /* TBD */

#define NEXT_BUTTON_PIN -1
#define PRIOR_BUTTON_PIN  -1

#endif


#endif
