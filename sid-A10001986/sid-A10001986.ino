/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2024 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * License: MIT
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
 *   the latest version by Espressif Systems.
 *   Detailed instructions for this step:
 *   https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
 *   
 * - Go to "Tools" > "Board: ..." -> "ESP32 Arduino" and select your board model (the
 *   CircuitSetup original boards are "NodeMCU-32S")
 *   
 * - Connect your ESP32 board.
 *   Note that NodeMCU ESP32 boards come in two flavors that differ in which serial 
 *   communications chip is used: Either SLAB CP210x USB-to-UART or CH340. Installing
 *   a driver might be required.
 *   Mac: 
 *   For the SLAB CP210x (which is used by NodeMCU-boards distributed by CircuitSetup)
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
 *   - WifiManager (tablatronix, tzapu) https://github.com/tzapu/WiFiManager
 *     (Tested with 2.0.13beta, 2.0.15-rc1, 2.0.16-rc2)
 *     For versions 2.0.16-rc2 and below, in order to avoid a delay when powering up
 *     several BTTFN-connected props, change _preloadwifiscan to false in WiFiManager.h 
 *     before compiling:
 *     -boolean       _preloadwifiscan        = true;
 *     +boolean       _preloadwifiscan        = false;
 *   - ArduinoJSON >= 6.19: https://arduinojson.org/v6/doc/installation/
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
 *  2024/01/21 (A10001986)
 *    - Major cleanup, minor fixes
 *  2024/01/18 (A10001986)
 *    - Fix Wifi Menu size
 *  2024/01/15 (A10001986)
 *    - Flush outstanding delayed saves before rebooting and on fake-power-off
 *    - Remove "Restart" menu item from CP, can't let WifiManager reboot behind
 *      our back.
 *  2023/12/08 (A10001986)
 *    - Add option to trigger a BTTFN-wide TT when pressing 0 on the IR remote
 *      or pressing the TT button (instead of a stand-alone TT).
 *    - Fix wakeup vs. SS logic
 *  2023/11/22 (A10001986)
 *    - Add option to show clock when screen saver is active (requires BTTFN
 *      connection to TCD for time information)
 *    - Wake up only if "speed" is from RotEnc, not when from GPS
 *  2023/11/20 (A10001986)
 *    - Make Wakeup on GPS/RotEnc changes more immediate
 *  2023/11/19 (A10001986)
 *    - Wake up on GPS/RotEnc speed change
 *    - Soften end of time travel when going back to actual GPS/RotEnc speed in 
 *      strict mode
 *    - Oscillate slightly in GPS/RotEnc (strict and non-strict) mode
 *    - Show "wait" symbol even when ss is active or fake power is off
 *  2023/11/05 (A10001986)
 *    - Settings: (De)serialize JSON from/to buffer instead of file
 *    - Fix corrupted CfgOnSD setting
 *  2023/11/04 (A10001986)
 *    - Unmount filesystems before reboot
 *  2023/11/02 (A10001986)
 *    - Start CP earlier to reduce network traffic delay caused by that darn WiFi 
 *      scan upon CP start.
 *    * WiFiManager: Disable pre-scanning of WiFi networks when starting the CP.
 *      Scanning is now only done when accessing the "Configure WiFi" page.
 *      To do that in your own installation, set _preloadwifiscan to false
 *      in WiFiManager.h
 *  2023/10/31 (A10001986)
 *    - BTTFN: User can now enter TCD's hostname instead of IP address. If hostname
 *      is given, TCD must be on same local network. Uses multicast, not DNS.
 *    - Change default for "skip tt anim" - default is now movie-like.
 *  2023/10/29 (A10001986)
 *    - Add "Adhere strictly to movie patterns" option, which is ON by default.
 *      If set, only parts of the movie-extracted time travel sequence (with
 *      interpolations) are shown when idle (modes 0-3) and when using GPS 
 *      speed. If unset, random variations will be shown. This is less boring, 
 *      but also less authentic.
 *      This option can be changed in the CP, and by *50OK (6050). If an SD card
 *      is present, every change will be saved. If no SD card is present, only
 *      the setting from the CP will be persistent.
 *    - Change command sequence for SA peaks to *51OK (6051).
 *    - Fix saving idle mode #5
 *  2023/10/27 (A10001986)
 *    - Make time tunnel animation (flicker) optional, purists might want to
 *      disable it.
 *  2023/10/27 (A10001986) [1.0]
 *    - Fix MQTT idle sequence selection
 *  2023/10/26 (A10001986)
 *    - Add "Universal backlot in the early 2000s" idle sequence (#4)
 *    - SA: Limit height during TT accoring to final pattern
 *  2023/10/25 (A10001986)
 *    - SA: Make FFT in float instead of double and thereby speed it up
 *    - SA: Redo bar scaling; tweak frequency bands
 *    - "Disable" some lamps in tt sequence
 *  2023/10/24 (A10001986)
 *    - Switch i2c speed to 400kHz
 *    - Various fixes to (until now blindly written) Spectrum Analyzer
 *  2023/10/05 (A10001986)
 *    - Add support for "wakeup" command (BTTFN/MQTT)
 *  2023/09/30 (A10001986)
 *    - Extend remote commands to 32 bit
 *    - Fix ring buffer handling for remote commands
 *  2023/09/26 (A10001986)
 *    - Clean up options
 *    - Fix TT button scan during TT
 *    - Clear display when Config Portal settings are saved
 *  2023/09/25 (A10001986)
 *    - Add option to handle TT triggers from wired TCD without 5s lead. Respective
 *      options on TCD and external props must be set identically.
 *  2023/09/23 (A10001986)
 *    - Add remote control facility through TCD keypad (requires BTTFN connection 
 *      with TCD). Commands for SID are 6000-6999.
 *    - Changed some command sequences
 *  2023/09/13 (A10001986)
 *    - Siddly: Add progress bar in red line
 *  2023/09/11 (A10001986)
 *    - Make SA peaks an option (CP and *50OK)
 *    - Guard SPIFFS/LittleFS calls with FS check
 *  2023/09/10 (A10001986)
 *    - If specific config file not found on SD, read from FlashFS - but only
 *      if it is mounted.
 *  2023/09/09 (A10001986)
 *    - Switch to LittleFS by default
 *    - *654321OK lets SID forget learned IR remote control
 *    - Remove "Wait for TCD fake power on" option
 *    - Fix SD pin numbers
 *    - Save current idle pattern to SD for persistence
 *      (only if changed via IR, not MQTT)
 *    - If SD mount fails at 16Mhz, retry at 25Mhz
 *    - If specific config file not found on SD, read from FlashFS
 *  2023/09/08 (A10001986)
 *    - TT sequence changes
 *    - Changed brightness adjustments from left/right to up/down
 *  2023/09/07 (A10001986)
 *    - Better IR learning guidance
 *    - Add'l TT sequence changes for idle mode 3
 *    - Add some MQTT commands
 *  2023/09/06 (A10001986)
 *    - Add alternative idle modes
 *    - Add tt "sequence" for sa mode
 *  2023/09/02 (A10001986)
 *    - Handle dynamic ETTO LEAD for BTTFN-triggered time travels
 *    - Go back to stand-alone mode if BTTFN polling times-out
 *    - Rework sequences
 *    - Fixes for SA
 *  2023/09/01 (A10001986)
 *    - Add TT sequence (unfinished)
 *  2023/08/31 (A10001986)
 *    - Further fixes for games
 *    - Spectrum analyzer not changed due to hardware issues
 *  2023/08/28 (A10001986)
 *    - Fixes for Siddly, text output, i2c communication, and general.
 *    - Test code in SA.
 *  2023/08/27 (A10001986)
 *    - Adapt to TCD's WiFi name appendix option
 *    - Add "AP name appendix" setting; allows unique AP names when running multiple 
 *      SIDs in AP mode in close range. 7 characters, 0-9/a-z/A-Z/- only, will be 
 *      added to "SID-AP".
 *    - Add AP password: Allows to configure a WPA2 password for the SID's AP mode
 *      (empty or 8 characters, 0-9/a-z/A-Z/- only)
 *    - *123456OK not only clears static IP config (as before), but also clears AP mode 
 *      WiFi password.
 *  2023/08/25 (A10001986)
 *    - Remove "Wait for TCD WiFi" option - this is not required; if the TCD is acting
 *      access point, it is supposed to be in car mode, and a delay is not required.
 *      (Do not let the TCD search for a configured WiFi network and have the FC rely on 
 *      the TCD falling back to AP mode; this will take long and the FC might time-out 
 *      unless the FC has a couple of connection retrys and long timeouts configured! 
 *      Use car mode, or delete the TCD's configured WiFi network if you power up the
 *      TCD and FC at the same time.)
 *    - Some code cleanups
 *    - Restrict WiFi Retrys to 10 (like WiFiManager)
 *    - Add "Wait for fake power on" option; if set, FC only boots
 *      after it received a fake-power-on signal from the TCD
 *      (Needs "Follow fake power" option set)
 *    - Fix parm handling of FPO and NM in fc_wifi
 *  2023/08/20 (A10001986)
 *    - Fixes for siddisplay
 *  2023/08/14 (A10001986)
 *    - Add config option to disable the default IR control
 *  2023/08/01 (A10001986)
 *    - Fix/enhance ABORT_TT (BTTFN/MQTT) [like FC]
 *  2023/07/23 (A10001986)
 *    - First version of translator table according to current schematics
 *  2023/07/22 (A10001986)
 *    - BTTFN dev type
 *  2023/07/21 (A10001986)
 *    - Add LED translator skeleton, prepare display routines
 *  2023/07/09 (A10001986)
 *    - BTTFN: Add night mode and fake power support (both signalled from TCD)
 *  2023/07/07 (A10001986)
 *    - Add TCD notifications: New way of wirelessly connecting the props by WiFi (aptly named
 *      "BTTFN"), without MQTT. Needs IP address of TCD entered in CP. Either MQTT or BTTFN is 
 *      used; if the TCD is configured to use MQTT, it will not send notifications via BTTFN.
 *    - Add "screen saver": Deactivate all LEDs after a configurable number of minutes
 *      of inactivity. TT button press, IR control key, time travel deactivates screen
 *      saver. (If IR is locked, only '#' key will deactivate ss.)
 *  2023/07/03 (A10001986)
 *    - Save ir lock state (*71OK), make it persitent over reboots
 *    - IR input: Disable repeated-keys detection, this hinders proper game play
 *      Remotes that send repeated keys (when holding the button) are not usable.
 *  2023/06/29 (A10001986)
 *    - Add font and letter sequences
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
