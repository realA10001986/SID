/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Settings & file handling
 * 
 * -------------------------------------------------------------------
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
 * be changed or removed..
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

#include "sid_global.h"

#define ARDUINOJSON_USE_LONG_LONG 0
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 0
#define ARDUINOJSON_ENABLE_ARDUINO_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STREAM 0
#define ARDUINOJSON_ENABLE_STD_STRING 0
#define ARDUINOJSON_ENABLE_NAN 0
#include <ArduinoJson.h>  // https://github.com/bblanchon/ArduinoJson
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#ifdef USE_SPIFFS
#define MYNVS SPIFFS
#include <SPIFFS.h>
#else
#define MYNVS LittleFS
#include <LittleFS.h>
#endif
#include <Update.h>

#include "sid_settings.h"
#include "sid_main.h"
#include "sid_wifi.h"

// If defined, old settings files will be used
// and converted if no new settings file is found.
// Keep this defined for a few versions/months.
//#define SETTINGS_TRANSITION
// Stage 2: Assume new settings are present, but
// still delete obsolete files.
#define SETTINGS_TRANSITION_2

#ifdef SETTINGS_TRANSITION
#undef SETTINGS_TRANSITION_2
#endif

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2500
#if ARDUINOJSON_VERSION_MAJOR >= 7
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif

// Secondary settings
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct [[gnu::packed]] {
    uint16_t brightness         = 15;
    uint8_t  irLocked           = 0;
    uint8_t  strictMode         = DEF_STRICT;
    uint8_t  SApeaks            = DEF_SA_PEAKS;
    uint8_t  irShowPosFBDisplay = DEF_IRFB;
    uint8_t  irShowCmdFBDisplay = DEF_IRCFB;
    uint8_t  showUpdAvail       = 1;
    uint8_t  updateV            = 0;
    uint8_t  updateR            = 0;
    uint8_t  SAmirror           = DEF_SA_MIRROR;
    uint8_t  carMode            = 0;
} secSettings;

// Tertiary settings (SD only)
// Do not change or insert new values, this
// struct is saved as such. Append new stuff.
static struct [[gnu::packed]] {
    uint8_t  bootMode     = BOOTM_NORMAL;
    uint8_t  idleMode     = 0;
} terSettings;

static int      secSetValidBytes = 0;
static uint32_t secSettingsHash  = 0;
static bool     haveSecSettings  = false;
static int      terSetValidBytes = 0;
static uint32_t terSettingsHash  = 0;
static bool     haveTerSettings  = false;

static uint32_t mainConfigHash = 0;
static uint32_t ipHash = 0;

static const char *cfgName    = "/sidconfig.json";  // Main config (flash)
static const char *ipCfgName  = "/sidipcfg";        // IP config (flash)
static const char *idName     = "/sidid";           // SID remote ID (flash)
static const char *irCfgName  = "/sidirkeys.json";  // IR keys (system-created) (flash/SD)
static const char *secCfgName = "/sid2cfg";         // Secondary settings (flash/SD)
static const char *terCfgName = "/sid3cfg";         // Tertiary settings (SD)

#ifdef SETTINGS_TRANSITION
static const char *ipCfgNameO = "/sidipcfg.json";   // IP config (flash)
static const char *idNameO    = "/sidid.json";      // SID remote ID (flash)
static const char *briCfgName = "/sidbricfg.json";  // Brightness config (flash/SD)
static const char *irlCfgName = "/sidirlcfg.json";  // IR lock (flash/SD)
static const char *ipaCfgName = "/sidipat.json";    // Idle pattern (SD only)
#endif
#ifdef SETTINGS_TRANSITION_2
static const char *obsFiles[] = {
    "/sidipcfg.json",  "/sidid.json",  "/sidbricfg.json", 
    "/sidirlcfg.json", "/sidipat.json", 
    NULL
};
#endif

static const char fwfn[]      = "/sidfw.bin";
static const char fwfnold[]   = "/sidfw.old";

static const char *jsonNames[NUM_IR_KEYS] = {
    "key0", "key1", "key2", "key3", "key4", 
    "key5", "key6", "key7", "key8", "key9", 
    "keySTAR", "keyHASH",
    "keyUP", "keyDOWN",
    "keyLEFT", "keyRIGHT",
    "keyOK" 
};

static const char *fsNoAvail = "File System not available";
static const char *failFileWrite = "Failed to open file for writing";
#ifdef SID_DBG
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
#endif

// If LittleFS/SPIFFS is mounted
static bool haveFS = false;

// If a SD card is found
bool haveSD = false;

// Save secondary settings on SD?
static bool configOnSD = false;

// Paranoia: No writes Flash-FS
bool FlashROMode = false;

extern bool doPeaks;
extern bool doMirror;

static bool read_settings(File configFile, int cfgReadCount);

static bool CopyTextParm(const char *json, char *setting, int setSize);
static bool CopyCheckValidNumParm(const char *json, char *text, int psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, int psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadIRKeys();

static void loadUpdAvail();

static void loadCarMode();

static bool     loadId();
static uint32_t createId();
static void     saveId();

static bool formatFlashFS(bool userSignal);

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, uint32_t *newHash = NULL);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, uint32_t oldHash = 0, uint32_t *newHash = NULL);

static bool writeFileToSD(const char *fn, uint8_t *buf, int len);
static bool writeFileToFS(const char *fn, uint8_t *buf, int len);

static bool loadConfigFile(const char *fn, uint8_t *buf, int len, int& validBytes, int forcefs = 0);
static bool saveConfigFile(const char *fn, uint8_t *buf, int len, int forcefs = 0);
static uint32_t calcHash(uint8_t *buf, int len);
static bool saveSecSettings(bool useCache);
static bool saveTerSettings(bool useCache);
#ifdef SETTINGS_TRANSITION
static void removeOldFiles(const char *oldfn);
#endif

static void firmware_update();

/*
 * settings_setup()
 * 
 * Mount flash FS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 */
void settings_setup()
{
    #ifdef SID_DBG
    const char *funcName = "settings_setup";
    #endif
    bool writedefault = false;
    bool freshFS = false;
    bool SDres = false;
    int cfgReadCount = 0;

    #ifdef SID_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(MYNVS.begin()) {

        haveFS = true;

    } else {

        #ifdef SID_DBG
        Serial.print("failed, formatting... ");
        #endif

        haveFS = formatFlashFS(true);
        freshFS = true;

    }

    if(haveFS) {
      
        #ifdef SID_DBG
        Serial.printf("ok.\nFlashFS: %d total, %d used\n", MYNVS.totalBytes(), MYNVS.usedBytes());
        #endif

        #ifdef SETTINGS_TRANSITION_2
        for(int i = 0; ; i++) {
            if(!obsFiles[i]) break;
            MYNVS.remove(obsFiles[i]);
        }
        #endif
        
        if(MYNVS.exists(cfgName)) {
            File configFile = MYNVS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile, cfgReadCount);
                cfgReadCount++;
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println("failed.\n*** Mounting flash FS failed. Using SD (if available)");

    }

    // Set up SD card
    SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);

    haveSD = false;

    uint32_t sdfreq = (settings.sdFreq[0] == '0') ? 16000000 : 4000000;
    #ifdef SID_DBG
    Serial.printf("%s: SD/SPI frequency %dMHz\n", funcName, sdfreq / 1000000);
    #endif

    #ifdef SID_DBG
    Serial.printf("%s: Mounting SD... ", funcName);
    #endif

    if(!(SDres = SD.begin(SD_CS_PIN, SPI, sdfreq))) {
        #ifdef SID_DBG
        Serial.printf("Retrying at 25Mhz... ");
        #endif
        SDres = SD.begin(SD_CS_PIN, SPI, 25000000);
    }

    if(SDres) {

        #ifdef SID_DBG
        Serial.println("ok");
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef SID_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    }

    if(haveSD) {

        firmware_update();

        if(SD.exists("/SID_FLASH_RO") || !haveFS) {
            bool writedefault2 = true;
            FlashROMode = true;
            Serial.println("Flash-RO mode: All settings/states stored on SD. Reloading settings.");
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile, cfgReadCount);
                    configFile.close();
                }
            }
            if(writedefault2) {
                #ifdef SID_DBG
                Serial.printf("%s: %s\n", funcName, badConfig);
                #endif
                mainConfigHash = 0;
                write_settings();
            }
        }
        
    } else {
        Serial.println("No SD card found");
    }

    // Re-format flash FS if either VER found, or
    // our config file is missing.
    if(haveFS && !FlashROMode) {
        if(MYNVS.exists("VER") || (!freshFS && !cfgReadCount)) {
            #ifdef SID_DBG
            Serial.println("Reformatting flash FS");
            #endif
            writedefault = true;
            formatFlashFS(true);
        }
    }
   
    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        #ifdef SID_DBG
        Serial.printf("%s: %s\n", funcName, badConfig);
        #endif
        mainConfigHash = 0;
        write_settings();
    }

    #ifdef SETTINGS_TRANSITION_2
    if(haveSD) {
        for(int i = 0; ; i++) {
            if(!obsFiles[i]) break;
            SD.remove(obsFiles[i]);
        }
    }
    #endif

    // Load/create "Remote ID"
    if(!loadId()) {
        myRemID = createId();
        #ifdef SID_DBG
        Serial.printf("Created Remote ID: 0x%lx\n", myRemID);
        #endif
        saveId();
    }

    // Determine if volume/ir settings are to be stored on SD
    configOnSD = (haveSD && ((settings.CfgOnSD[0] != '0') || FlashROMode));

    // Load secondary config file
    if(loadConfigFile(secCfgName, (uint8_t *)&secSettings, sizeof(secSettings), secSetValidBytes)) {
        secSettingsHash = calcHash((uint8_t *)&secSettings, sizeof(secSettings));
        haveSecSettings = true;
    }

    // Load tertiary config file (SD only)
    if(haveSD) {
        if(loadConfigFile(terCfgName, (uint8_t *)&terSettings, sizeof(terSettings), terSetValidBytes, 1)) {
            terSettingsHash = calcHash((uint8_t *)&terSettings, sizeof(terSettings));
            haveTerSettings = true;
        }
    }

    // Load user-config's and learned IR keys
    loadIRKeys();

    // Load car mode
    if(*settings.cm_ssid) {
        loadCarMode();
    }

    loadUpdAvail();
}

void unmount_fs()
{
    if(haveFS) {
        MYNVS.end();
        #ifdef SID_DBG
        Serial.println("Unmounted Flash FS");
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef SID_DBG
        Serial.println("Unmounted SD card");
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile, int cfgReadCount)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    DECLARE_D_JSON(JSON_SIZE,json);
    
    DeserializationError error = readJSONCfgFile(json, configFile, &mainConfigHash);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("ERROR: Config file too large (%d vs %d), memory corrupted, awaiting doom.\n", jsonSize, JSON_SIZE);
    }
    
    #ifdef SID_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    #endif
    #endif

    if(!error) {

        // WiFi Configuration

        if(!cfgReadCount) {
            memset(settings.ssid, 0, sizeof(settings.ssid));
            memset(settings.pass, 0, sizeof(settings.pass));
            memset(settings.bssid, 0, sizeof(settings.bssid));
        }

        if(json["ssid"]) {
            memset(settings.ssid, 0, sizeof(settings.ssid));
            memset(settings.pass, 0, sizeof(settings.pass));
            memset(settings.bssid, 0, sizeof(settings.bssid));
            strncpy(settings.ssid, json["ssid"], sizeof(settings.ssid) - 1);
            if(json["pass"]) {
                strncpy(settings.pass, json["pass"], sizeof(settings.pass) - 1);
            }
            if(json["bssid"]) {
                strncpy(settings.bssid, json["bssid"], sizeof(settings.bssid) - 1);
            }
        } else {
            if(!cfgReadCount) {
                // Set a marker for "no ssid tag in config file", ie read from NVS.
                settings.ssid[1] = 'X';
            } else if(settings.ssid[0] || settings.ssid[1] != 'X') {
                // FlashRO: If flash-config didn't set the marker, write new file 
                // with ssid/pass from flash-config
                wd = true;
            }
        }

        wd |= CopyTextParm(json["cmsid"], settings.cm_ssid, sizeof(settings.cm_ssid));
        wd |= CopyTextParm(json["cmpwd"], settings.cm_pass, sizeof(settings.cm_pass));
        wd |= CopyTextParm(json["cmbid"], settings.cm_bssid, sizeof(settings.cm_bssid));

        wd |= CopyTextParm(json["hostName"], settings.hostName, sizeof(settings.hostName));
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);

        wd |= CopyTextParm(json["systemID"], settings.systemID, sizeof(settings.systemID));
        wd |= CopyTextParm(json["appw"], settings.appw, sizeof(settings.appw));
        wd |= CopyCheckValidNumParm(json["apch"], settings.apChnl, sizeof(settings.apChnl), 0, 11, DEF_AP_CHANNEL);
        wd |= CopyCheckValidNumParm(json["wAOD"], settings.wifiAPOffDelay, sizeof(settings.wifiAPOffDelay), 0, 99, DEF_WIFI_APOFFDELAY);

        // Settings

        wd |= CopyCheckValidNumParm(json["skipTTAnim"], settings.skipTTAnim, sizeof(settings.skipTTAnim), 0, 1, DEF_SKIP_TTANIM);
        wd |= CopyCheckValidNumParm(json["ssTimer"], settings.ssTimer, sizeof(settings.ssTimer), 0, 999, DEF_SS_TIMER);

        wd |= CopyTextParm(json["tcdIP"], settings.tcdIP, sizeof(settings.tcdIP));
        wd |= CopyCheckValidNumParm(json["useGPSS"], settings.useGPSS, sizeof(settings.useGPSS), 0, 1, DEF_USE_GPSS);
        wd |= CopyCheckValidNumParm(json["useNM"], settings.useNM, sizeof(settings.useNM), 0, 1, DEF_USE_NM);
        wd |= CopyCheckValidNumParm(json["useFPO"], settings.useFPO, sizeof(settings.useFPO), 0, 1, DEF_USE_FPO);
        wd |= CopyCheckValidNumParm(json["bttfnTT"], settings.bttfnTT, sizeof(settings.bttfnTT), 0, 1, DEF_BTTFN_TT);
        wd |= CopyCheckValidNumParm(json["ssClock"], settings.ssClock, sizeof(settings.ssClock), 0, 1, DEF_SS_CLK);
        wd |= CopyCheckValidNumParm(json["ssClkOffNM"], settings.ssClockOffNM, sizeof(settings.ssClockOffNM), 0, 1, DEF_SS_CLK_NMOFF);

        wd |= CopyCheckValidNumParm(json["TCDpresent"], settings.TCDpresent, sizeof(settings.TCDpresent), 0, 1, DEF_TCD_PRES);
        wd |= CopyCheckValidNumParm(json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);

        wd |= CopyCheckValidNumParm(json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);

        wd |= CopyCheckValidNumParm(json["disDIR"], settings.disDIR, sizeof(settings.disDIR), 0, 1, DEF_DISDIR);

        #ifdef SID_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        wd |= CopyTextParm(json["mqttServer"], settings.mqttServer, sizeof(settings.mqttServer));
        wd |= CopyCheckValidNumParm(json["mqttV"], settings.mqttVers, sizeof(settings.mqttVers), 0, 1, 0);
        wd |= CopyTextParm(json["mqttUser"], settings.mqttUser, sizeof(settings.mqttUser));
        wd |= CopyTextParm(json["mqttT"], settings.mqttTopic, sizeof(settings.mqttTopic));
        #endif

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DECLARE_D_JSON(JSON_SIZE,json);

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef SID_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    // Write this only if either set, or also present in file read earlier
    if(settings.ssid[0] || settings.ssid[1] != 'X') {
        json["ssid"] = (const char *)settings.ssid;
        json["pass"] = (const char *)settings.pass;
        json["bssid"] = (const char *)settings.bssid;
    }

    json["cmsid"] = (const char *)settings.cm_ssid;
    json["cmpwd"] = (const char *)settings.cm_pass;
    json["cmbid"] = (const char *)settings.cm_bssid;
        
    json["hostName"] = (const char *)settings.hostName;
    json["wifiConRetries"] = (const char *)settings.wifiConRetries;

    json["systemID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["apch"] = (const char *)settings.apChnl;
    json["wAOD"] = (const char *)settings.wifiAPOffDelay;

    json["skipTTAnim"] = (const char *)settings.skipTTAnim;
    json["ssTimer"] = (const char *)settings.ssTimer;
    
    json["tcdIP"] = (const char *)settings.tcdIP;
    json["useGPSS"] = (const char *)settings.useGPSS;
    json["useNM"] = (const char *)settings.useNM;
    json["useFPO"] = (const char *)settings.useFPO;
    json["bttfnTT"] = (const char *)settings.bttfnTT;
    json["ssClock"] = (const char *)settings.ssClock;
    json["ssClkOffNM"] = (const char *)settings.ssClockOffNM;

    json["TCDpresent"] = (const char *)settings.TCDpresent;
    json["noETTOLead"] = (const char *)settings.noETTOLead;
 
    json["CfgOnSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;

    json["disDIR"] = (const char *)settings.disDIR;

    #ifdef SID_HAVEMQTT
    json["useMQTT"] = (const char *)settings.useMQTT;
    json["mqttServer"] = (const char *)settings.mqttServer;
    json["mqttV"] = (const char *)settings.mqttVers;
    json["mqttUser"] = (const char *)settings.mqttUser;
    json["mqttT"] = (const char *)settings.mqttTopic;
    #endif

    writeJSONCfgFile(json, cfgName, FlashROMode, mainConfigHash, &mainConfigHash);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && MYNVS.exists(cfgName));
}

/*
 *  Helpers for parm copying & checking
 */

static bool CopyTextParm(const char *json, char *setting, int setSize)
{
    if(!json) return true;
    
    memset(setting, 0, setSize);
    strncpy(setting, json, setSize - 1);
    return false;
}

static bool CopyCheckValidNumParm(const char *json, char *text, int psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, int psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);
    bool ret = false;

    if(!len) {
        i = setDefault;
        ret = true;
    } else {
        for(int j = 0; j < len; j++) {
            if(text[j] < '0' || text[j] > '9') {
                i = setDefault;
                ret = true;
                break;
            }
        }
        if(!ret) {
            i = atoi(text);   
            if(i < lowerLim) {
                i = lowerLim;
                ret = true;
            } else if(i > upperLim) {
                i = upperLim;
                ret = true;
            }
        }
    }
    sprintf(text, "%d", i);
    return ret;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    bool ret = false;
    float f;

    if(!len) {
        f = setDefault;
        ret = true;
    } else {
        for(i = 0; i < len; i++) {
            if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
                f = setDefault;
                ret = true;
                break;
            }
        }
        if(!ret) {
            f = strtof(text, NULL);
            if(f < lowerLim) {
                f = lowerLim;
                ret = true;
            } else if(f > upperLim) {
                f = upperLim;
                ret = true;
            }
        }
    }
    sprintf(text, "%.1f", f);
    return ret;
}

bool evalBool(char *s)
{
    if(*s == '0') return false;
    return true;
}

static bool openCfgFileRead(const char *fn, File& f, bool SDonly = false)
{
    bool haveConfigFile = false;
    
    if(configOnSD || SDonly) {
        if(SD.exists(fn)) {
            haveConfigFile = (f = SD.open(fn, "r"));
        }
    } 
    if(!haveConfigFile && !SDonly && haveFS) {
        if(MYNVS.exists(fn)) {
            haveConfigFile = (f = MYNVS.open(fn, "r"));
        }
    }

    return haveConfigFile;
}

/*
 * Load custom IR config
 */

static bool loadIRkeysFromFile(File configFile, int index)
{
    uint32_t ir_keys[NUM_IR_KEYS];
    DECLARE_S_JSON(1024,json);
    bool ret = true;
    
    DeserializationError err = readJSONCfgFile(json, configFile);

    if(err) return false;
        
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        if(json[jsonNames[i]]) {
            ir_keys[i] = (uint32_t)strtoul(json[(const char *)jsonNames[i]], NULL, 16);
            if(!ir_keys[i]) ret = false;
            #ifdef SID_DBG
            else {
                Serial.printf("Adding IR %s - 0x%08x\n", jsonNames[i], ir_keys[i]);
            }
            #endif
        } else {
            ret = false;
        }
    }

    if(ret) {
        populateIRarray(ir_keys, index);
    }   
    
    configFile.close();

    return true;
}

static bool loadIRKeys()
{
    File configFile;

    // Load learned keys from Flash/SD
    if(openCfgFileRead(irCfgName, configFile)) {
        if(!loadIRkeysFromFile(configFile, REM_KEYS_LEARNED)) {
            #ifdef SID_DBG
            Serial.printf("%s is incomplete, deleting\n", irCfgName);
            #endif
            deleteIRKeys();
        }
    } else {
        #ifdef SID_DBG
        Serial.printf("%s does not exist\n", irCfgName);
        #endif
    }

    return true;
}

void saveIRKeys()
{
    uint32_t ir_keys[NUM_IR_KEYS];
    char buf[12];

    if(!haveFS && !configOnSD)
        return;

    copyIRarray(ir_keys, REM_KEYS_LEARNED);

    // Delete file if keys incomplete
    bool keysMissing = false;
    for(int i = 0; i < NUM_IR_KEYS; i++) {
        if(!ir_keys[i]) keysMissing = true;
    }
    if(keysMissing) {
        deleteIRKeys();
        return;
    }

    DECLARE_S_JSON(1024,json);

    for(int i = 0; i < NUM_IR_KEYS; i++) {
        sprintf(buf, "0x%08x", ir_keys[i]);
        json[(const char *)jsonNames[i]] = buf;   // no const cast, needs to be copied
    }

    writeJSONCfgFile(json, irCfgName, configOnSD);
}

void deleteIRKeys()
{
    if(configOnSD) {
        SD.remove(irCfgName);
    } else if(haveFS) {
        MYNVS.remove(irCfgName);
    }
}

/*
 *  Load/save display brightness
 */

void loadBrightness()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadBrightness: extracting from secSettings");
        #endif
        sid.setBrightness(secSettings.brightness);
    } else {
        #ifdef SETTINGS_TRANSITION
        char temp[6];
        File configFile;
        if(!haveFS && !configOnSD) return;
        if(openCfgFileRead(briCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(!CopyCheckValidNumParm(json["brightness"], temp, sizeof(temp), 0, 15, 15)) {
                    sid.setBrightness((uint8_t)atoi(temp), true);
                }
            } 
            configFile.close();
            saveBrightness();
        }
        removeOldFiles(briCfgName);
        #endif
    }
}


void storeBrightness()
{
    secSettings.brightness = sid.getBrightness();
}

void saveBrightness()
{
    storeBrightness();
    saveSecSettings(true);
}

/*
 *  Load/save IR lock status
 */

void loadIRLock()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadIRLock: extracting from secSettings");
        #endif
        irLocked = !!secSettings.irLocked;
    } else {
        #ifdef SETTINGS_TRANSITION
        char temp[6];
        File configFile;
        if(!haveFS && !configOnSD) return;
        if(openCfgFileRead(irlCfgName, configFile)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(!CopyCheckValidNumParm(json["lock"], temp, sizeof(temp), 0, 1, 0)) {
                    irLocked = (atoi(temp) > 0);
                }
            } 
            configFile.close();
            saveIRLock();
        }
        removeOldFiles(irlCfgName);
        #endif
    }
}

void storeIRLock()
{
    // Used to keep secSettings up-to-date in case
    // of delayed save
    secSettings.irLocked = irLocked ? 1 : 0;
}

void saveIRLock()
{
    storeIRLock();
    saveSecSettings(true);
}

/*
 *  Load/save strictMode
 */

void loadStrict()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadStrict: extracting from secSettings");
        #endif
        strictMode = !!secSettings.strictMode;
    }
}

void saveStrict()
{
    secSettings.strictMode = strictMode ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save SAsettings
 */

void loadSASettings()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadSASettings: extracting from secSettings");
        #endif
        doPeaks = !!secSettings.SApeaks;
        doMirror = !!secSettings.SAmirror;
    }
}

void saveSASettings()
{
    secSettings.SApeaks = doPeaks ? 1 : 0;
    secSettings.SAmirror = doMirror ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save "positive IR feedback"
 */

void loadPosIRFB()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadPosIRFB: extracting from secSettings");
        #endif
        irShowPosFBDisplay = !!secSettings.irShowPosFBDisplay;
    }
}

void savePosIRFB()
{
    secSettings.irShowPosFBDisplay = irShowPosFBDisplay ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save "command entry IR feedback"
 */

void loadIRCFB()
{
    if(haveSecSettings) {
        #ifdef SID_DBG
        Serial.println("loadIRCFB: extracting from secSettings");
        #endif
        irShowCmdFBDisplay = !!secSettings.irShowCmdFBDisplay;
    }
}

void saveIRCFB()
{
    secSettings.irShowCmdFBDisplay = irShowCmdFBDisplay ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save "show update notification at boot"
 */

void loadUpdAvail()
{
    if(haveSecSettings) {
        showUpdAvail = !!secSettings.showUpdAvail;
    }
}

void saveUpdAvail()
{
    secSettings.showUpdAvail = showUpdAvail ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save curr version
 */

void loadUpdVers(int &v, int& r)
{
    if(haveSecSettings) {
        v = secSettings.updateV;
        r = secSettings.updateR;
    } else {
        v = r = 0;
    }
}

void saveUpdVers(int v, int r)
{
    secSettings.updateV = v;
    secSettings.updateR = r;
    saveSecSettings(true);
}

// Special for CP where several settings are possibly
// changed at the same time. We don't want to write the
// file more than once.
void saveAllSecCP()
{
    secSettings.strictMode = strictMode ? 1 : 0;
    secSettings.SApeaks = doPeaks ? 1 : 0;
    secSettings.SAmirror = doMirror ? 1 : 0;
    secSettings.irShowPosFBDisplay = irShowPosFBDisplay ? 1 : 0;
    secSettings.irShowCmdFBDisplay = irShowCmdFBDisplay ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save carMode
 */

static void loadCarMode()
{
    if(haveSecSettings) {
        carMode = !!secSettings.carMode;
    }
}

void saveCarMode()
{
    secSettings.carMode = carMode ? 1 : 0;
    saveSecSettings(true);
}

/*
 *  Load/save the idle pattern (SD only)
 */

void loadIdlePat()
{
    if(!haveSD)
        return;

    if(haveTerSettings) {
        #ifdef SID_DBG
        Serial.println("loadIdlePat: extracting from terSettings");
        #endif
        if(terSettings.idleMode <= SID_MAX_IDLE_MODE) {
            idleMode = terSettings.idleMode;
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        char temp[6];
        File configFile;
        if(!haveSD) return;
        if(openCfgFileRead(ipaCfgName, configFile, true)) {
            DECLARE_S_JSON(512,json);
            if(!readJSONCfgFile(json, configFile)) {
                if(!CopyCheckValidNumParm(json["pattern"], temp, sizeof(temp), 0, 0x1f, 0)) {
                    idleMode = (uint16_t)atoi(temp);
                    strictMode = (idleMode & 0x10) ? true : false;
                    idleMode &= 0x0f;
                    if(idleMode > SID_MAX_IDLE_MODE) idleMode = 0;
                }
            } 
            configFile.close();
            saveIdlePat();
            saveStrict();
        }
        removeOldFiles(ipaCfgName);
        #endif
    }
}

void storeIdlePat()
{
    // Used to keep terSettings up-to-date in case
    // of delayed save
    terSettings.idleMode = idleMode;
}

void saveIdlePat()
{
    storeIdlePat();
    saveTerSettings(true);
}

/*
 * Load/save boot display mode (Idle, SA)
 */

uint8_t loadBootMode()
{
    if(haveSD && haveTerSettings) {
        return terSettings.bootMode;
    }

    return 0;
}

void storeBootMode(uint8_t bootMode)
{
    terSettings.bootMode = bootMode;
}

void saveBootMode()
{
    saveTerSettings(true);
}

/*
 * Load/save/delete settings for static IP configuration
 */

#ifdef SETTINGS_TRANSITION
static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0) 
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}
#endif

bool loadIpSettings()
{
    memset((void *)&ipsettings, 0, sizeof(ipsettings));

    if(!haveFS && !FlashROMode)
        return false;

    int vb = 0;
    if(loadConfigFile(ipCfgName, (uint8_t *)&ipsettings, sizeof(ipsettings), vb, -1)) {
        #ifdef SID_DBG
        Serial.println("loadIpSettings: Loaded bin settings");
        #endif
        if(*ipsettings.ip) {
            if(checkIPConfig()) {
                ipHash = calcHash((uint8_t *)&ipsettings, sizeof(ipsettings));
                return true;
            } else {
                memset((void *)&ipsettings, 0, sizeof(ipsettings));
                deleteIpSettings();
            }
        }
    } else {
        #ifdef SETTINGS_TRANSITION
        bool invalid = false;
        bool haveConfig = false;
        if( (!FlashROMode && MYNVS.exists(ipCfgNameO)) ||
            (FlashROMode && SD.exists(ipCfgNameO)) ) {
            File configFile = FlashROMode ? SD.open(ipCfgNameO, "r") : MYNVS.open(ipCfgNameO, "r");
            if(configFile) {
                DECLARE_S_JSON(512,json);
                DeserializationError error = readJSONCfgFile(json, configFile);
                if(!error) {
                    invalid |= CopyIPParm(json["IpAddress"], ipsettings.ip, sizeof(ipsettings.ip));
                    invalid |= CopyIPParm(json["Gateway"], ipsettings.gateway, sizeof(ipsettings.gateway));
                    invalid |= CopyIPParm(json["Netmask"], ipsettings.netmask, sizeof(ipsettings.netmask));
                    invalid |= CopyIPParm(json["DNS"], ipsettings.dns, sizeof(ipsettings.dns));
                    haveConfig = !invalid;
                } else {
                    invalid = true;
                }
                configFile.close();
            }
            removeOldFiles(ipCfgNameO);
        }
        if(invalid) {
            memset((void *)&ipsettings, 0, sizeof(ipsettings));
        } else {
            writeIpSettings();
        }
        return haveConfig;
        #endif
    }

    ipHash = 0;
    return false;
}

void writeIpSettings()
{
    if(!haveFS && !FlashROMode)
        return;

    if(!*ipsettings.ip)
        return;

    uint32_t nh = calcHash((uint8_t *)&ipsettings, sizeof(ipsettings));
    
    if(ipHash) {
        if(nh == ipHash) {
            #ifdef SID_DBG
            Serial.printf("writeIpSettings: Not writing, hash identical (%x)\n", ipHash);
            #endif
            return;
        }
    }

    ipHash = nh;
    
    saveConfigFile(ipCfgName, (uint8_t *)&ipsettings, sizeof(ipsettings), -1);
}

void deleteIpSettings()
{
    #ifdef SID_DBG
    Serial.println("deleteIpSettings: Deleting ip config");
    #endif

    ipHash = 0;

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
        MYNVS.remove(ipCfgName);
    }
}

/*
 *  Load/save/create remote ID
 */

static bool loadId()
{
    uint32_t buf = 0;
    int vb = 0;
    if(loadConfigFile(idName, (uint8_t *)&buf, sizeof(buf), vb, -1)) {
        #ifdef SID_DBG
        Serial.println("loadId: Loaded bin settings");
        #endif
        myRemID = buf;
        return true;
    } else {
        #ifdef SETTINGS_TRANSITION
        bool invalid = false;
        bool haveConfig = false;
        if(!haveFS && !FlashROMode) return false;
        if( (!FlashROMode && MYNVS.exists(idNameO)) ||
             (FlashROMode && SD.exists(idNameO)) ) {
            File configFile = FlashROMode ? SD.open(idNameO, "r") : MYNVS.open(idNameO, "r");
            if(configFile) {
                DECLARE_S_JSON(512, json);
                DeserializationError error = readJSONCfgFile(json, configFile);
                if(!error) {
                    myRemID = (uint32_t)json["ID"];
                    invalid = (myRemID == 0);
                    if(!invalid) saveId();
                    haveConfig = !invalid;
                }
                configFile.close();
            }
            removeOldFiles(idNameO);
        }
        return haveConfig;
        #endif
    }

    return false;
}

static uint32_t createId()
{
    return esp_random() ^ esp_random() ^ esp_random();
}

static void saveId()
{
    if(!haveFS && !FlashROMode)
        return;

    saveConfigFile(idName, (uint8_t *)&myRemID, sizeof(myRemID), -1);
}

/*
 * Various helpers
 */

static bool formatFlashFS(bool userSignal)
{
    bool ret = false;

    if(userSignal) {
        // Show the user some action
        showWaitSequence();
    } else {
        #ifdef SID_DBG
        Serial.println("Formatting flash FS");
        #endif
    }

    MYNVS.format();
    if(MYNVS.begin()) ret = true;

    if(userSignal) {
        endWaitSequence();
    }
    
    return ret;
}

/* 
 * Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 */
void moveSettings()
{
    if(!haveSD || !haveFS) 
        return;

    if(configOnSD && FlashROMode) {
        #ifdef SID_DBG
        Serial.println("moveSettings: Writing to flash prohibted (FlashROMode), aborting.");
        #endif
    }

    // Flush pending saves
    flushDelayedSave();

    configOnSD = !configOnSD;
    
    saveSecSettings(false);
    saveIRKeys();

    configOnSD = !configOnSD;

    if(configOnSD) {
        SD.remove(secCfgName);
        SD.remove(irCfgName);
    } else {
        MYNVS.remove(secCfgName);
        MYNVS.remove(irCfgName);
    }
}

/*
 * Helpers for JSON config files
 */
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, uint32_t *readHash)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("rJSON: Buffer allocation failed (%d)\n", bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef SID_DBG
    Serial.println(buf);
    #endif

    if(readHash) {
        *readHash = calcHash((uint8_t *)buf, bufSize);
    }
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}

static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, uint32_t oldHash, uint32_t *newHash)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("wJSON: Buffer allocation failed (%d)\n", bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef SID_DBG
    Serial.printf("Writing %s to %s\n", fn, useSD ? "SD" : "FS");
    Serial.println((const char *)buf);
    #endif

    if(oldHash || newHash) {
        uint32_t newH = calcHash((uint8_t *)buf, bufSize);
        
        if(newHash) *newHash = newH;
    
        if(oldHash) {
            if(oldHash == newH) {
                #ifdef SID_DBG
                Serial.printf("Not writing %s, hash identical (%x)\n", fn, oldHash);
                #endif
                free(buf);
                return true;
            }
        }
    }

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("wJSON: %s\n", failFileWrite);
    }

    return success;
}

/*
 * Generic file readers/writers
 */

static bool readFile(File& myFile, uint8_t *buf, int len)
{
    if(myFile) {
        size_t bytesr = myFile.read(buf, len);
        myFile.close();
        return (bytesr == len);
    } else
        return false;
}

static bool readFileU(File& myFile, uint8_t*& buf, int& len)
{
    if(myFile) {
        len = myFile.size();
        buf = (uint8_t *)malloc(len+1);
        if(buf) {
            buf[len] = 0;
            return readFile(myFile, buf, len);
        } else {
            myFile.close();
        }
    }
    return false;
}

// Read file of unknown size from SD
static bool readFileFromSDU(const char *fn, uint8_t*& buf, int& len)
{   
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    return readFileU(myFile, buf, len);
}

// Read file of unknown size from NVS
static bool readFileFromFSU(const char *fn, uint8_t*& buf, int& len)
{   
    if(!haveFS || !MYNVS.exists(fn))
        return false;

    File myFile = MYNVS.open(fn, FILE_READ);
    return readFileU(myFile, buf, len);
}

// Read file of known size from SD
static bool readFileFromSD(const char *fn, uint8_t *buf, int len)
{   
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_READ);
    return readFile(myFile, buf, len);
}

// Read file of known size from NVS
static bool readFileFromFS(const char *fn, uint8_t *buf, int len)
{
    if(!haveFS || !MYNVS.exists(fn))
        return false;

    File myFile = MYNVS.open(fn, FILE_READ);
    return readFile(myFile, buf, len);
}

static bool writeFile(File& myFile, uint8_t *buf, int len)
{
    if(myFile) {
        size_t bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

// Write file to SD
static bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    return writeFile(myFile, buf, len);
}

// Write file to NVS
static bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    if(!haveFS)
        return false;

    File myFile = MYNVS.open(fn, FILE_WRITE);
    return writeFile(myFile, buf, len);
}

static uint8_t cfChkSum(const uint8_t *buf, int len)
{
    uint16_t s = 0;
    while(len--) {
        s += *buf++;
    }
    s = (s >> 8) + (s & 0xff);
    s += (s >> 8);
    return (uint8_t)(~s);
}

static bool loadConfigFile(const char *fn, uint8_t *buf, int len, int& validBytes, int forcefs)
{
    bool haveConfigFile = false;
    int fl;
    uint8_t *bbuf = NULL;

    // forcefs: > 0: SD only; = 0 either (configOnSD); < 0: Flash if !FlashROMode, SD if FlashROMode

    if(haveSD && ((!forcefs && configOnSD) || forcefs > 0 || (forcefs < 0 && FlashROMode))) {
        haveConfigFile = readFileFromSDU(fn, bbuf, fl);
    }
    if(!haveConfigFile && haveFS && (!forcefs || (forcefs < 0 && !FlashROMode))) {
        haveConfigFile = readFileFromFSU(fn, bbuf, fl);
    }
    if(haveConfigFile) {
        uint8_t chksum = cfChkSum(bbuf, fl - 1);
        if((haveConfigFile = (bbuf[fl - 1] == chksum))) {
            validBytes = bbuf[0] | (bbuf[1] << 8);
            memcpy(buf, bbuf + 2, min(len, validBytes));
            haveConfigFile = true; //(len <= validBytes);
            #ifdef SID_DBG
            Serial.printf("loadConfigFile: loaded %s: need %d, got %d bytes: ", fn, len, validBytes);
            for(int k = 0; k < len; k++) Serial.printf("%02x ", buf[k]);
            Serial.printf("chksum %02x\n", chksum);
            #endif
        } else {
            #ifdef SID_DBG
            Serial.printf("loadConfigFile: Bad checksum %02x %02x\n", chksum, bbuf[fl - 1]);
            #endif
        }
    }

    if(bbuf) free(bbuf);

    return haveConfigFile;
}

static bool saveConfigFile(const char *fn, uint8_t *buf, int len, int forcefs)
{
    uint8_t *bbuf;
    bool ret = false;

    if(!(bbuf = (uint8_t *)malloc(len + 3)))
        return false;

    bbuf[0] = len & 0xff;
    bbuf[1] = len >> 8;
    memcpy(bbuf + 2, buf, len);
    bbuf[len + 2] = cfChkSum(bbuf, len + 2);
    
    #ifdef SID_DBG
    Serial.printf("saveConfigFile: %s: ", fn);
    for(int k = 0; k < len + 3; k++) Serial.printf("0x%02x ", bbuf[k]);
    Serial.println("");
    #endif

    if((!forcefs && configOnSD) || forcefs > 0 || (forcefs < 0 && FlashROMode)) {
        ret = writeFileToSD(fn, bbuf, len + 3);
    } else if(haveFS) {
        ret = writeFileToFS(fn, bbuf, len + 3);
    }

    free(bbuf);

    return ret;
}

static uint32_t calcHash(uint8_t *buf, int len)
{
    uint32_t hash = 2166136261UL;
    for(int i = 0; i < len; i++) {
        hash = (hash ^ buf[i]) * 16777619;
    }
    return hash;
}

static bool saveSecSettings(bool useCache)
{
    uint32_t oldHash = secSettingsHash;

    secSettingsHash = calcHash((uint8_t *)&secSettings, sizeof(secSettings));
    
    if(useCache) {
        if(oldHash == secSettingsHash) {
            #ifdef SID_DBG
            Serial.printf("saveSecSettings: Data up to date, not writing (%x)\n", secSettingsHash);
            #endif
            return true;
        }
    }
    
    return saveConfigFile(secCfgName, (uint8_t *)&secSettings, sizeof(secSettings), 0);
}

static bool saveTerSettings(bool useCache)
{
    if(!haveSD)
        return false;

    uint32_t oldHash = terSettingsHash;
    
    terSettingsHash = calcHash((uint8_t *)&terSettings, sizeof(terSettings));
    
    if(useCache) {
        if(oldHash == terSettingsHash) {
            #ifdef SID_DBG
            Serial.printf("saveTerSettings: Data up to date, not writing (%x)\n", terSettingsHash);
            #endif
            return true;
        }
    }
    
    return saveConfigFile(terCfgName, (uint8_t *)&terSettings, sizeof(terSettings), 1);
}

#ifdef SETTINGS_TRANSITION
static void removeOldFiles(const char *oldfn)
{
    if(haveSD) SD.remove(oldfn);
    if(haveFS) MYNVS.remove(oldfn);
    #ifdef SID_DBG
    Serial.printf("removeOldFiles: Removing %s\n", oldfn);
    #endif
}
#endif

// Emergency firmware update from SD card
static void fw_error_blink(int n)
{
    bool leds = false;

    for(int i = 0; i < n; i++) {
        leds = !leds;
        digitalWrite(IR_FB_PIN, leds ? HIGH : LOW);
        delay(500);
    }
    digitalWrite(IR_FB_PIN, LOW);
}

static void firmware_update()
{
    const char *upderr = "Firmware update error %d\n";
    uint8_t  buf[1024];
    unsigned int lastMillis = millis();
    bool     leds = false;
    size_t   s;

    if(!SD.exists(fwfn))
        return;
    
    File myFile = SD.open(fwfn, FILE_READ);
    
    if(!myFile)
        return;

    pinMode(IR_FB_PIN, OUTPUT);
    
    if(!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Serial.printf(upderr, Update.getError());
        fw_error_blink(5);
        return;
    }

    while((s = myFile.read(buf, 1024))) {
        if(Update.write(buf, s) != s) {
            break;
        }
        if(millis() - lastMillis > 1000) {
            leds = !leds;
            digitalWrite(IR_FB_PIN, leds ? HIGH : LOW);
            lastMillis = millis();
        }
    }
    
    if(Update.hasError() || !Update.end(true)) {
        Serial.printf(upderr, Update.getError());
        fw_error_blink(5);
    } 
    myFile.close();
    // Rename/remove in any case, we don't
    // want an update loop hammer our flash
    SD.remove(fwfnold);
    SD.rename(fwfn, fwfnold);
    unmount_fs();
    delay(1000);
    fw_error_blink(0);
    esp_restart();
}    
