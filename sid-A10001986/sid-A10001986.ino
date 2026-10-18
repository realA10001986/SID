/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 * This code was written without the use of artificial intelligence.
 *
 * License: Modified MIT NON-AI
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of the 
 * Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * Links inside the Software pointing to the original source must not 
 * be changed or removed.
 *
 * In addition, the following restrictions apply:
 * 
 * 1. The Software and any modifications made to it may not be used 
 * for the purpose of training or improving machine learning algorithms, 
 * including but not limited to artificial intelligence, natural 
 * language processing, or data mining. This condition applies to any 
 * derivatives, modifications, or updates based on the Software code. 
 * Any usage of the Software in an AI-training dataset is considered a 
 * breach of this License.
 *
 * 2. The Software may not be included in any dataset used for 
 * training or improving machine learning algorithms, including but 
 * not limited to artificial intelligence, natural language processing, 
 * or data mining.
 *
 * 3. Any person or organization found to be in violation of these 
 * restrictions will be subject to legal action and may be held liable 
 * for any damages resulting from such use.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Build instructions (for Arduino IDE)
 * 
 * - Install the Arduino IDE
 *   https://www.arduino.cc/en/software
 *    
 * - This firmware requires the "ESP32-Arduino" framework. To install this framework, 
 *   in the Arduino IDE, go to "File" > "Preferences" and add the URL   
 *   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
 *   to "Additional Boards Manager URLs". The list is comma-separated.
 *   
 * - Go to "Tools" > "Board" > "Boards Manager", then search for "esp32", and install 
 *   the latest 2.x version by Espressif Systems. Versions >=3.x are not supported.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board using a suitable USB cable.
 *   Note that NodeMCU ESP32 boards come in two flavors that differ in which serial 
 *   communications chip is used: Either SLAB CP210x USB-to-UART or CH340. Installing
 *   a driver might be required.
 *   Mac: 
 *   For the SLAB CP210x (which is on the NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   The port ("Tools -> "Port") is named /dev/cu.SLAB_USBtoUART, and the maximum
 *   upload speed ("Tools" -> "Upload Speed") can be used.
 *   The CH340 is supported out-of-the-box since Mojave. The port is named 
 *   /dev/cu.usbserial-XXXX (XXXX being some random number), and the maximum upload 
 *   speed is 460800.
 *   Windows:
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
 *   installing a driver is required:
 *   https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers?tab=downloads
 *   After installing this driver, connect your ESP32, start the Device Manager, 
 *   expand the "Ports (COM & LPT)" list and look for the port with the ESP32 name.
 *   Choose this port under "Tools" -> "Port" in Arduino IDE.
 *   For the CH340, another driver is needed. Try connecting the ESP32 and have
 *   Windows install a driver automatically; otherwise search google for a suitable
 *   driver. Note that the maximum upload speed is either 115200, or perhaps 460800.
 *
 * - Install required libraries. In the Arduino IDE, go to "Tools" -> "Manage Libraries" 
 *   and install the following libraries:
 *   - ArduinoJSON (>= 6.19): https://arduinojson.org/v6/doc/installation/
 *
 * - Download the complete firmware source code:
 *   https://github.com/realA10001986/SID/archive/refs/heads/main.zip
 *   Extract this file somewhere. Enter the "sid-A10001986" folder and 
 *   double-click on "sid-A10001986.ino". This opens the firmware in the
 *   Arduino IDE.
 *
 * - Go to "Sketch" -> "Upload" to compile and upload the firmware to your ESP32 board.
 */

/*  Changelog
 *  
 *  2026/10/18 (A10001986) [2.00]
 *    **********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below     **
 *    **********************************************************************************
 *    - Re-arrange WiFi Configuration page for a better user experience
 *    - BTTFN: "Hostname of TCD" now defaults to "timecircuits" instead of being empty.
 *      As a result, a factory-fresh SID now connects to a TCD in the network without
 *      any user configuration changes.
 *    - IR command sequence *123456ok now only deletes SID-AP password, and *123457ok
 *      deletes static IP config. Both lead to a reboot if applicable.
 *    - IR command sequence *697159ok, when entered twice, resets all settings to
 *      factory defaults.
 *    - Major version bump due to generational identity of underlying code with other 
 *      props.
 *  2026/09/20 (A10001986) [1.76]
 *    **********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below     **
 *    **********************************************************************************
 *    - Car Mode: Automatically pair SID to currently connected TCD by means of the
 *      TCD's BSSID upon first successful connection in Car Mode. This avoids connecting
 *      to other people's TCDs if in close range (eg on meet-ups).
 *    - Key "9" on IR remote refills Plutonium (requires TCD with firmware >= 3.27, and
 *      Dash Gauges), unless any of the games is running (where it is "quit").
 *    - Protect the SID from uploading a wrong firmware by accident. The filename of
 *      the firmware binary now must contain the word "sid". The check is case-
 *      insensitive.
 *    - Bonjour/mDNS: Send good-bye packet on controlled reboots
 *    - Minor fixes in connection with feedback signals
 *  2026/08/23 (A10001986) [1.75]
 *    **********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below     **
 *    **********************************************************************************
 *    - IR Learning: Remove "GO" word sequence.
 *    - IR Learning: Each key now needs to pressed twice to sort out unsuitable remotes
 *      sending alternate codes on consecutive key presses.
 *    - TCD-provided speed now only overrules idle patterns 0-3, the "Universal Backlot" 
 *      idle pattern (#4) is unaffected (and has a smoother tt sequence now).
 *    - More robust error handling in case of malformed config files
 *    - SD: More compatibility fixes
 *    - Code maintenance
 *  2026/07/17 (A10001986) [1.74]
 *    *********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below    **
 *    *********************************************************************************
 *    - Bugfix: Fix SD initialization in ESP32-Arduino to conform to specs.
 *      Sandisk Ultra 32GB cards (and possibly others that previously were
 *      not usable) are now recognized.
 *  2026/06/22 (A10001986) [1.73]
 *    *********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below    **
 *    *********************************************************************************
 *    - IR learning timeout increased to 20 seconds
 *    - MQTT: Don't queue and instead ignore (most) commands while fake-off or busy
 *  2026/04/27 (A10001986) [1.72]
 *    *********************************************************************************
 *    ** If updating from below 1.70, please see boxed note at version 1.71 below    **
 *    *********************************************************************************
 *    - Add Car Mode: In addition to existing WiFi network connection settings, you 
 *      can now pre-configure TCD-AP SSID and password in WiFi Settings and quickly
 *      switch between normal (home, iPhone...) WiFi connection and Car WiFi connection 
 *      (through TCD-AP) by commands *990 (normal) and *991 (Car) or through the
 *      Config Portal.
 *    - New "mirrored" Spectrum Analyzer mode (*64 toggles between legacy and mirrored)
 *    - Fix race condition in Fake Power logic
 *    - Code optimizations and fixes
 *  2026/03/25 (A10001986) [1.71]
 *    *********************************************************************************
 *    ** If updating from below 1.70, please install 1.70 first to have your         **
 *    ** settings converted. If 1.70 is skipped, many of your settings (static IP,   **
 *    ** brightness, movie mode, peaks in SA, IR lock) will be restored to default   **
 *    ** values. It suffices to install 1.70 and boot once; you can then immediately **
 *    ** update to a later version.                                                  **
 *    ** https://github.com/realA10001986/SID/releases/tag/V1.70                     **
 *    *********************************************************************************
 *    - MQTT: Add user-defined topic to subscribe to, and display messages for, 
 *      similar to TCD. Only the first max. 8 characters of messages (and only 
 *      a-z/A-Z and 0-9) are shown.
 *    - WiFi: Allow defining a BSSID (AP MAC address) to connect to a specific AP
 *      if multiple APs with identical SSID are available.
 *    - MQTT: Disable if server can't be resolved
 *    - WiFi: Do not power down AP long as a client is connected
 *    - Brush up Config Portal a bit
 *    - Code optimizations and fixes
 *  2026/02/15 (A10001986) [1.70]
 *    - New file format for secondary and IP settings. This version of the firmware 
 *      converts old to new.
 *    - SID will boot into either Idle or SA mode depending on what was used  
 *      before reboot/power-down (SD card required); "Boot into SA" option removed.
 *      Current mode is saved 10 seconds after activation (or upon fake-power-down
 *      or a controlled reboot such as when changing settings in Config Portal)
 *    - Add option to show positive IR feedback (=successful commands) on display.
 *      Can be changed through Config Portal or *62 (every change is saved).
 *    - Add option to show command entry IR feedback on display. Can be changed 
 *      through Config Portal or *63 (every change is saved).
 *    - Changed games IR keys: 1 new game, 5 pause, 9 quit
 *    - "Peaks in SA" setting changes through *61 are now saved.
 *    - Display MAC address (STA) on WiFi Configuration Page
 *    - Avoid falling back to stand-alone mode too early when TCD stops sending
 *      packets
 *    - Config files are now only written if actually changed which prolongs
 *      Flash life-span.
 *    - Code optimizations and fixes.
 *  [...]
 *  2023/06/28 (A10001986)
 *    - Initial version: unfinished, entirely untested
 *      Missing: Display control; pin assignments; all sequences; etc. 
 */

#include "sid_global.h"

#include <Arduino.h>
#include <Wire.h>

#include "sid_settings.h"
#include "sid_wifi.h"
#include "sid_main.h"

void setup()
{
    powerupMillis = millis();
    
    Serial.begin(115200);
    Serial.println();

    Wire.begin(-1, -1, 400000);

    main_boot();
    settings_setup();
    wifi_setup();
    main_setup();
    bttfn_loop();
}

void loop()
{    
    main_loop();
    wifi_loop();
    bttfn_loop();
}

#if defined(SID_DBG) || defined(SID_DBG_NET)
#warning "Debug output is enabled. Binary not suitable for release."
#endif
