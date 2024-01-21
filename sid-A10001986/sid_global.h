/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Global definitions
 */

#ifndef _SID_GLOBAL_H
#define _SID_GLOBAL_H

// Version strings
#define SID_VERSION       "V1.13"
#define SID_VERSION_EXTRA "JAN212024"

//#define SID_DBG              // debug output on Serial

/*************************************************************************
 ***                     mDNS (Bonjour) support                        ***
 *************************************************************************/

// Supply mDNS service 
// Allows accessing the Config Portal via http://hostname.local
// <hostname> is configurable in the Config Portal
// This needs to be commented if WiFiManager provides mDNS
#define SID_MDNS

// Uncomment when using WiFiManager 2.0.17 or later
//#define WIFIMANAGER_2_0_17

// Uncomment this if WiFiManager has mDNS enabled
//#define SID_WM_HAS_MDNS          

/*************************************************************************
 ***             Configuration for peripherals/features                ***
 *************************************************************************/

// Uncomment for HomeAssistant MQTT protocol support
#define SID_HAVEMQTT

// --- end of config options

/*************************************************************************
 ***                           Miscellaneous                           ***
 *************************************************************************/

// Use SPIFFS (if defined) or LittleFS (if undefined; esp32-arduino >= 2.x)
//#define USE_SPIFFS

// External time travel lead time, as defined by TCD firmware
// If SID is connected by wire, and the option "Signal Time Travel without 5s 
// lead" is set on the TCD, the SID option "TCD signals without lead" must
// be set, too.
#define ETTO_LEAD 5000

// Uncomment to include BTTFN discover support (multicast)
#define BTTFN_MC

/*************************************************************************
 ***                  esp32-arduino version detection                  ***
 *************************************************************************/

#if defined __has_include && __has_include(<esp_arduino_version.h>)
#include <esp_arduino_version.h>
#ifdef ESP_ARDUINO_VERSION_MAJOR
    #if ESP_ARDUINO_VERSION >= ESP_ARDUINO_VERSION_VAL(2,0,8)
    #define HAVE_GETNEXTFILENAME
    #endif
#endif
#endif

/*************************************************************************
 ***                             GPIO pins                             ***
 *************************************************************************/

// IR Remote
#define IRREMOTE_PIN      27

// IR feedback
#define IR_FB_PIN         17

// Time Travel button (or TCD input trigger)
#define TT_IN_PIN         13

// I2S audio pins
#define I2S_BCLK_PIN      26
#define I2S_LRCLK_PIN     25
#define I2S_DIN_PIN       33

// SD Card pins
#define SD_CS_PIN          5
#define SPI_MOSI_PIN      23
#define SPI_MISO_PIN      19
#define SPI_SCK_PIN       18 

#endif
