/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2025 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Settings & file handling
 * 
 * -------------------------------------------------------------------
 * License: MIT NON-AI
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
#include <SPIFFS.h>
#else
#define SPIFFS LittleFS
#include <LittleFS.h>
#endif

#include "sid_settings.h"
#include "sid_main.h"

// Size of main config JSON
// Needs to be adapted when config grows
#define JSON_SIZE 2000
#if ARDUINOJSON_VERSION_MAJOR >= 7
#error "ArduinoJSON v7 not supported"
#define DECLARE_S_JSON(x,n) JsonDocument n;
#define DECLARE_D_JSON(x,n) JsonDocument n;
#else
#define DECLARE_S_JSON(x,n) StaticJsonDocument<x> n;
#define DECLARE_D_JSON(x,n) DynamicJsonDocument n(x);
#endif 

static const char *cfgName    = "/sidconfig.json";  // Main config (flash)
static const char *idName     = "/fcid.json";       // SID remote ID (flash)
static const char *ipCfgName  = "/sidipcfg.json";   // IP config (flash)
static const char *briCfgName = "/sidbricfg.json";  // Brightness config (flash/SD)
static const char *irlCfgName = "/sidirlcfg.json";  // IR lock (flash/SD)
static const char *irCfgName  = "/sidirkeys.json";  // IR keys (system-created) (flash/SD)
static const char *irUCfgName = "/sidirkeys.txt";   // IR keys (user-created) (SD only)
static const char *ipaCfgName = "/sidipat.json";    // Idle pattern (SD only)

static const char *jsonNames[NUM_IR_KEYS] = {
    "key0", "key1", "key2", "key3", "key4", 
    "key5", "key6", "key7", "key8", "key9", 
    "keySTAR", "keyHASH",
    "keyUP", "keyDOWN",
    "keyLEFT", "keyRIGHT",
    "keyOK" 
};

static const char *fsNoAvail = "File System not available";
static const char *badConfig = "Settings bad/missing/incomplete; writing new file";
static const char *failFileWrite = "Failed to open file for writing";

/* If SPIFFS/LittleFS is mounted */
static bool haveFS = false;

/* If a SD card is found */
bool haveSD = false;

/* Save secondary settings on SD? */
static bool configOnSD = false;

/* Paranoia: No writes Flash-FS  */
bool FlashROMode = false;

/* Cache */
static uint8_t  prevSavedBri = 12;
static uint16_t prevSavedIM  = 0;
static bool     prevSavedIRL = 0;

static bool read_settings(File configFile);

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault);
static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault);
static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault);
static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault);

static bool loadIRKeys();

static bool CopyIPParm(const char *json, char *text, uint8_t psize);

static bool loadId();
static uint32_t createId();
static void saveId();

static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName);
static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName);
static bool writeFileToSD(const char *fn, uint8_t *buf, int len);
static bool writeFileToFS(const char *fn, uint8_t *buf, int len);

/*
 * settings_setup()
 * 
 * Mount SPIFFS/LittleFS and SD (if available).
 * Read configuration from JSON config file
 * If config file not found, create one with default settings
 *
 */
void settings_setup()
{
    const char *funcName = "settings_setup";
    bool writedefault = false;
    bool SDres = false;

    #ifdef SID_DBG
    Serial.printf("%s: Mounting flash FS... ", funcName);
    #endif

    if(SPIFFS.begin()) {

        haveFS = true;

    } else {

        #ifdef SID_DBG
        Serial.print(F("failed, formatting... "));
        #endif

        // Show the user some action
        showWaitSequence();

        SPIFFS.format();
        if(SPIFFS.begin()) haveFS = true;

        endWaitSequence();

    }

    if(haveFS) {
      
        #ifdef SID_DBG
        Serial.println(F("ok, loading settings"));
        #endif
        
        if(SPIFFS.exists(cfgName)) {
            File configFile = SPIFFS.open(cfgName, "r");
            if(configFile) {
                writedefault = read_settings(configFile);
                configFile.close();
            } else {
                writedefault = true;
            }
        } else {
            writedefault = true;
        }

        // Write new config file after mounting SD and determining FlashROMode

    } else {

        Serial.println(F("failed.\nThis is very likely a hardware problem."));
        Serial.println(F("*** Since the internal storage is unavailable, all settings/states will be stored on"));
        Serial.println(F("*** the SD card (if available)")); 
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
        Serial.println(F("ok"));
        #endif

        uint8_t cardType = SD.cardType();
       
        #ifdef SID_DBG
        const char *sdTypes[5] = { "No card", "MMC", "SD", "SDHC", "unknown (SD not usable)" };
        Serial.printf("SD card type: %s\n", sdTypes[cardType > 4 ? 4 : cardType]);
        #endif

        haveSD = ((cardType != CARD_NONE) && (cardType != CARD_UNKNOWN));

    } else {

        Serial.println(F("No SD card found"));
      
    }

    if(haveSD) {
        if(SD.exists("/SID_FLASH_RO") || !haveFS) {
            bool writedefault2 = false;
            FlashROMode = true;
            Serial.println(F("Flash-RO mode: All settings/states stored on SD. Reloading settings."));
            if(SD.exists(cfgName)) {
                File configFile = SD.open(cfgName, "r");
                if(configFile) {
                    writedefault2 = read_settings(configFile);
                    configFile.close();
                } else {
                    writedefault2 = true;
                }
            } else {
                writedefault2 = true;
            }
            if(writedefault2) {
                Serial.printf("%s: %s\n", funcName, badConfig);
                write_settings();
            }
        }
    }
   
    // Now write new config to flash FS if old one somehow bad
    // Only write this file if FlashROMode is off
    if(haveFS && writedefault && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, badConfig);
        write_settings();
    }

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

    // Load user-config's and learned IR keys
    loadIRKeys();
}

void unmount_fs()
{
    if(haveFS) {
        SPIFFS.end();
        #ifdef SID_DBG
        Serial.println(F("Unmounted Flash FS"));
        #endif
        haveFS = false;
    }
    if(haveSD) {
        SD.end();
        #ifdef SID_DBG
        Serial.println(F("Unmounted SD card"));
        #endif
        haveSD = false;
    }
}

static bool read_settings(File configFile)
{
    const char *funcName = "read_settings";
    bool wd = false;
    size_t jsonSize = 0;
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    */
    
    DeserializationError error = readJSONCfgFile(json, configFile, funcName);

    #if ARDUINOJSON_VERSION_MAJOR < 7
    jsonSize = json.memoryUsage();
    if(jsonSize > JSON_SIZE) {
        Serial.printf("%s: ERROR: Config file too large (%d vs %d), memory corrupted, awaiting doom.\n", funcName, jsonSize, JSON_SIZE);
    }
    
    #ifdef SID_DBG
    if(jsonSize > JSON_SIZE - 256) {
          Serial.printf("%s: WARNING: JSON_SIZE needs to be adapted **************\n", funcName);
    }
    Serial.printf("%s: Size of document: %d (JSON_SIZE %d)\n", funcName, jsonSize, JSON_SIZE);
    #endif
    #endif

    if(!error) {

        wd |= CopyCheckValidNumParm(json["bootSA"], settings.bootSA, sizeof(settings.bootSA), 0, 1, DEF_BOOTSA);
        wd |= CopyCheckValidNumParm(json["ssTimer"], settings.ssTimer, sizeof(settings.ssTimer), 0, 999, DEF_SS_TIMER);
        
        wd |= CopyCheckValidNumParm(json["disDIR"], settings.disDIR, sizeof(settings.disDIR), 0, 1, DEF_DISDIR);

        if(json["hostName"]) {
            memset(settings.hostName, 0, sizeof(settings.hostName));
            strncpy(settings.hostName, json["hostName"], sizeof(settings.hostName) - 1);
        } else wd = true;
        if(json["systemID"]) {
            memset(settings.systemID, 0, sizeof(settings.systemID));
            strncpy(settings.systemID, json["systemID"], sizeof(settings.systemID) - 1);
        } else wd = true;

        if(json["appw"]) {
            memset(settings.appw, 0, sizeof(settings.appw));
            strncpy(settings.appw, json["appw"], sizeof(settings.appw) - 1);
        } else wd = true;
        
        wd |= CopyCheckValidNumParm(json["wifiConRetries"], settings.wifiConRetries, sizeof(settings.wifiConRetries), 1, 10, DEF_WIFI_RETRY);
        wd |= CopyCheckValidNumParm(json["wifiConTimeout"], settings.wifiConTimeout, sizeof(settings.wifiConTimeout), 7, 25, DEF_WIFI_TIMEOUT);

        wd |= CopyCheckValidNumParm(json["TCDpresent"], settings.TCDpresent, sizeof(settings.TCDpresent), 0, 1, DEF_TCD_PRES);
        wd |= CopyCheckValidNumParm(json["noETTOLead"], settings.noETTOLead, sizeof(settings.noETTOLead), 0, 1, DEF_NO_ETTO_LEAD);

        if(json["tcdIP"]) {
            memset(settings.tcdIP, 0, sizeof(settings.tcdIP));
            strncpy(settings.tcdIP, json["tcdIP"], sizeof(settings.tcdIP) - 1);
        } else wd = true;

        wd |= CopyCheckValidNumParm(json["useGPSS"], settings.useGPSS, sizeof(settings.useGPSS), 0, 1, DEF_USE_GPSS);
        wd |= CopyCheckValidNumParm(json["useNM"], settings.useNM, sizeof(settings.useNM), 0, 1, DEF_USE_NM);
        wd |= CopyCheckValidNumParm(json["useFPO"], settings.useFPO, sizeof(settings.useFPO), 0, 1, DEF_USE_FPO);
        wd |= CopyCheckValidNumParm(json["bttfnTT"], settings.bttfnTT, sizeof(settings.bttfnTT), 0, 1, DEF_BTTFN_TT);
        wd |= CopyCheckValidNumParm(json["ssClock"], settings.ssClock, sizeof(settings.ssClock), 0, 1, DEF_SS_CLK);
        wd |= CopyCheckValidNumParm(json["ssClkOffNM"], settings.ssClockOffNM, sizeof(settings.ssClockOffNM), 0, 1, DEF_SS_CLK_NMOFF);

        // strictMode is overruled by loadIdlePat later (if present)
        wd |= CopyCheckValidNumParm(json["strictMode"], settings.strictMode, sizeof(settings.strictMode), 0, 1, DEF_STRICT);
        strictMode = (settings.strictMode[0] == '1');
        wd |= CopyCheckValidNumParm(json["skipTTAnim"], settings.skipTTAnim, sizeof(settings.skipTTAnim), 0, 1, DEF_SKIP_TTANIM);
        wd |= CopyCheckValidNumParm(json["SApeaks"], settings.SApeaks, sizeof(settings.SApeaks), 0, 1, DEF_SA_PEAKS);

        #ifdef SID_HAVEMQTT
        wd |= CopyCheckValidNumParm(json["useMQTT"], settings.useMQTT, sizeof(settings.useMQTT), 0, 1, 0);
        if(json["mqttServer"]) {
            memset(settings.mqttServer, 0, sizeof(settings.mqttServer));
            strncpy(settings.mqttServer, json["mqttServer"], sizeof(settings.mqttServer) - 1);
        } else wd = true;
        if(json["mqttUser"]) {
            memset(settings.mqttUser, 0, sizeof(settings.mqttUser));
            strncpy(settings.mqttUser, json["mqttUser"], sizeof(settings.mqttUser) - 1);
        } else wd = true;
        #endif

        wd |= CopyCheckValidNumParm(json["CfgOnSD"], settings.CfgOnSD, sizeof(settings.CfgOnSD), 0, 1, DEF_CFG_ON_SD);
        //wd |= CopyCheckValidNumParm(json["sdFreq"], settings.sdFreq, sizeof(settings.sdFreq), 0, 1, DEF_SD_FREQ);

    } else {

        wd = true;

    }

    return wd;
}

void write_settings()
{
    const char *funcName = "write_settings";
    DECLARE_D_JSON(JSON_SIZE,json);
    /*
    //StaticJsonDocument<JSON_SIZE> json;
    DynamicJsonDocument json(JSON_SIZE);
    */

    if(!haveFS && !FlashROMode) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    #ifdef SID_DBG
    Serial.printf("%s: Writing config file\n", funcName);
    #endif

    json["bootSA"] = (const char *)settings.bootSA;
    json["ssTimer"] = (const char *)settings.ssTimer;

    json["disDIR"] = (const char *)settings.disDIR;

    json["TCDpresent"] = (const char *)settings.TCDpresent;
    json["noETTOLead"] = (const char *)settings.noETTOLead;
    
    json["hostName"] = (const char *)settings.hostName;
    json["systemID"] = (const char *)settings.systemID;
    json["appw"] = (const char *)settings.appw;
    json["wifiConRetries"] = (const char *)settings.wifiConRetries;
    json["wifiConTimeout"] = (const char *)settings.wifiConTimeout;

    json["tcdIP"] = (const char *)settings.tcdIP;
    json["useGPSS"] = (const char *)settings.useGPSS;
    json["useNM"] = (const char *)settings.useNM;
    json["useFPO"] = (const char *)settings.useFPO;
    json["bttfnTT"] = (const char *)settings.bttfnTT;
    json["ssClock"] = (const char *)settings.ssClock;
    json["ssClkOffNM"] = (const char *)settings.ssClockOffNM;

    sprintf(settings.strictMode, "%d", strictMode ? 1 : 0);
    json["strictMode"] = (const char *)settings.strictMode;
    json["skipTTAnim"] = (const char *)settings.skipTTAnim;
    json["SApeaks"] = (const char *)settings.SApeaks;

    #ifdef SID_HAVEMQTT
    json["useMQTT"] = (const char *)settings.useMQTT;
    json["mqttServer"] = (const char *)settings.mqttServer;
    json["mqttUser"] = (const char *)settings.mqttUser;
    #endif
 
    json["CfgOnSD"] = (const char *)settings.CfgOnSD;
    //json["sdFreq"] = (const char *)settings.sdFreq;

    writeJSONCfgFile(json, cfgName, FlashROMode, funcName);
}

bool checkConfigExists()
{
    return FlashROMode ? SD.exists(cfgName) : (haveFS && SPIFFS.exists(cfgName));
}

/*
 *  Helpers for parm copying & checking
 */

static bool CopyCheckValidNumParm(const char *json, char *text, uint8_t psize, int lowerLim, int upperLim, int setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParm(text, lowerLim, upperLim, setDefault);
}

static bool CopyCheckValidNumParmF(const char *json, char *text, uint8_t psize, float lowerLim, float upperLim, float setDefault)
{
    if(!json) return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return checkValidNumParmF(text, lowerLim, upperLim, setDefault);
}

static bool checkValidNumParm(char *text, int lowerLim, int upperLim, int setDefault)
{
    int i, len = strlen(text);

    if(len == 0) {
        sprintf(text, "%d", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] < '0' || text[i] > '9') {
            sprintf(text, "%d", setDefault);
            return true;
        }
    }

    i = atoi(text);

    if(i < lowerLim) {
        sprintf(text, "%d", lowerLim);
        return true;
    }
    if(i > upperLim) {
        sprintf(text, "%d", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "000")
    sprintf(text, "%d", i);

    return false;
}

static bool checkValidNumParmF(char *text, float lowerLim, float upperLim, float setDefault)
{
    int i, len = strlen(text);
    float f;

    if(len == 0) {
        sprintf(text, "%.1f", setDefault);
        return true;
    }

    for(i = 0; i < len; i++) {
        if(text[i] != '.' && text[i] != '-' && (text[i] < '0' || text[i] > '9')) {
            sprintf(text, "%.1f", setDefault);
            return true;
        }
    }

    f = atof(text);

    if(f < lowerLim) {
        sprintf(text, "%.1f", lowerLim);
        return true;
    }
    if(f > upperLim) {
        sprintf(text, "%.1f", upperLim);
        return true;
    }

    // Re-do to get rid of formatting errors (eg "00.")
    sprintf(text, "%.1f", f);

    return false;
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
        if(SPIFFS.exists(fn)) {
            haveConfigFile = (f = SPIFFS.open(fn, "r"));
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
    //StaticJsonDocument<1024> json;
    
    DeserializationError err = readJSONCfgFile(json, configFile, "loadIRkeysFromFile");
    
    if(!err) {
        size_t jsonSize = json.memoryUsage();
        int j = 0;
        if(!index) Serial.printf("%s JSON parsed, size %d\n", configFile.name(), jsonSize);
        for(int i = 0; i < NUM_IR_KEYS; i++) {
            if(json[jsonNames[i]]) {
                j++;
                ir_keys[i] = (uint32_t)strtoul(json[(const char *)jsonNames[i]], NULL, 16);
                #ifdef SID_DBG
                Serial.printf("Adding IR %s - 0x%08x\n", jsonNames[i], ir_keys[i]);
                #endif
            } else {
                if(!index) Serial.printf("IR %s missing in %s\n", jsonNames[i], configFile.name());
            }
        }
        populateIRarray(ir_keys, index);
        if(!index) Serial.printf("%d IR keys added from %s\n", j, configFile.name());
    } else {
        if(!index) Serial.printf("Error parsing %s: %s\n", configFile.name(), err.c_str());
    }
    configFile.close();

    return true;
}

static bool loadIRKeys()
{
    File configFile;

    // Load user keys from SD
    if(haveSD) {
        if(SD.exists(irUCfgName)) {
            configFile = SD.open(irUCfgName, "r");
            if(configFile) {
                loadIRkeysFromFile(configFile, 0);
            } else {
                #ifdef SID_DBG
                Serial.printf("%s not found on SD card\n", irUCfgName);
                #endif
            }
        }
    }

    // Load learned keys from Flash/SD
    if(openCfgFileRead(irCfgName, configFile)) {
        loadIRkeysFromFile(configFile, 1);
    } else {
        #ifdef SID_DBG
        Serial.printf("%s does not exist\n", irCfgName);
        #endif
    }

    return true;
}

bool saveIRKeys()
{
    DECLARE_S_JSON(1024,json);
    //StaticJsonDocument<1024> json;
    uint32_t ir_keys[NUM_IR_KEYS];
    char buf[12];

    if(!haveFS && !configOnSD)
        return true;

    copyIRarray(ir_keys, 1);

    for(int i = 0; i < NUM_IR_KEYS; i++) {
        sprintf(buf, "0x%08x", ir_keys[i]);
        json[(const char *)jsonNames[i]] = buf;   // no const cast, needs to be copied
    }

    writeJSONCfgFile(json, irCfgName, configOnSD, "saveIRKeys");
    
    return true;
}

void deleteIRKeys()
{
    if(configOnSD) {
        SD.remove(irCfgName);
    } else if(haveFS) {
        SPIFFS.remove(irCfgName);
    }
}

/*
 *  Load/save the Brightness
 */

bool loadBrightness()
{
    const char *funcName = "loadBrightness";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        #ifdef SID_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    if(openCfgFileRead(briCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["brightness"], temp, sizeof(temp), 0, 15, 15)) {
                sid.setBrightness((uint8_t)atoi(temp), true);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedBri = sid.getBrightness();

    return true;
}

void saveBrightness(bool useCache)
{
    const char *funcName = "saveBrightness";
    char buf[6];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && (prevSavedBri == sid.getBrightness())) {
        #ifdef SID_DBG
        Serial.printf("%s: Prev. saved bri identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", sid.getBrightness());
    json["brightness"] = (const char *)buf;

    if(writeJSONCfgFile(json, briCfgName, configOnSD, funcName)) {
        prevSavedBri = sid.getBrightness();
    }
}

/*
 *  Load/save the IR lock status
 */

bool loadIRLock()
{
    const char *funcName = "loadIRLock";
    char temp[6];
    File configFile;

    if(!haveFS && !configOnSD) {
        #ifdef SID_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    if(openCfgFileRead(irlCfgName, configFile)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["lock"], temp, sizeof(temp), 0, 1, 0)) {
                irLocked = (atoi(temp) > 0);
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedIRL = irLocked;

    return true;
}

void saveIRLock(bool useCache)
{
    const char *funcName = "saveIRLock";
    char buf[6];
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(useCache && (prevSavedIRL == irLocked)) {
        #ifdef SID_DBG
        Serial.printf("%s: Prev. saved irl identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveFS && !configOnSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", irLocked ? 1 : 0);
    json["lock"] = (const char *)buf;

    if(writeJSONCfgFile(json, irlCfgName, configOnSD, funcName)) {
        prevSavedIRL = irLocked;
    }
}

/*
 *  Load/save the idle pattern (SD only)
 */

bool loadIdlePat()
{
    const char *funcName = "loadIdlePat";
    char temp[6];
    File configFile;

    if(!haveSD) {
        #ifdef SID_DBG
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        #endif
        return false;
    }

    if(openCfgFileRead(ipaCfgName, configFile, true)) {
        DECLARE_S_JSON(512,json);
        //StaticJsonDocument<512> json;
        if(!readJSONCfgFile(json, configFile, funcName)) {
            if(!CopyCheckValidNumParm(json["pattern"], temp, sizeof(temp), 0, 0x1f, 0)) {
                idleMode = (uint16_t)atoi(temp);
                strictMode = (idleMode & 0x10) ? true : false;
                idleMode &= 0x0f;
                if(idleMode > SID_MAX_IDLE_MODE) idleMode = 0;
            }
        } 
        configFile.close();
    }

    // Do not write a default file, use pre-set value

    prevSavedIM = idleMode;
    if(strictMode) prevSavedIM |= 0x10;

    #ifdef SID_DBG
    Serial.printf("%s: idleMode %d, strict %d\n", funcName, idleMode, strictMode);
    #endif

    return true;
}

void saveIdlePat(bool useCache)
{
    const char *funcName = "saveIdlePat";
    char buf[6];
    uint16_t tempIM = idleMode;
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(strictMode) tempIM |= 0x10;

    if(useCache && (prevSavedIM == tempIM)) {
        #ifdef SID_DBG
        Serial.printf("%s: Prev. saved idle pattern identical, not writing\n", funcName);
        #endif
        return;
    }

    if(!haveSD) {
        Serial.printf("%s: %s\n", funcName, fsNoAvail);
        return;
    }

    sprintf(buf, "%d", tempIM);
    json["pattern"] = (const char *)buf;

    if(writeJSONCfgFile(json, ipaCfgName, true, funcName)) {
        prevSavedIM = tempIM;
    }
}

/*
 * Load/save/delete settings for static IP configuration
 */

bool loadIpSettings()
{
    bool invalid = false;
    bool haveConfig = false;

    if(!haveFS && !FlashROMode)
        return false;

    if( (!FlashROMode && SPIFFS.exists(ipCfgName)) ||
        (FlashROMode && SD.exists(ipCfgName)) ) {

        File configFile = FlashROMode ? SD.open(ipCfgName, "r") : SPIFFS.open(ipCfgName, "r");

        if(configFile) {

            DECLARE_S_JSON(512,json);
            //StaticJsonDocument<512> json;
            
            DeserializationError error = readJSONCfgFile(json, configFile, "loadIpSettings");

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

    }

    if(invalid) {

        // config file is invalid - delete it

        Serial.println(F("loadIpSettings: IP settings invalid; deleting file"));

        deleteIpSettings();

        memset(ipsettings.ip, 0, sizeof(ipsettings.ip));
        memset(ipsettings.gateway, 0, sizeof(ipsettings.gateway));
        memset(ipsettings.netmask, 0, sizeof(ipsettings.netmask));
        memset(ipsettings.dns, 0, sizeof(ipsettings.dns));

    }

    return haveConfig;
}

static bool CopyIPParm(const char *json, char *text, uint8_t psize)
{
    if(!json) return true;

    if(strlen(json) == 0) 
        return true;

    memset(text, 0, psize);
    strncpy(text, json, psize-1);
    return false;
}

void writeIpSettings()
{
    DECLARE_S_JSON(512,json);
    //StaticJsonDocument<512> json;

    if(!haveFS && !FlashROMode)
        return;

    if(strlen(ipsettings.ip) == 0)
        return;
    
    json["IpAddress"] = (const char *)ipsettings.ip;
    json["Gateway"]   = (const char *)ipsettings.gateway;
    json["Netmask"]   = (const char *)ipsettings.netmask;
    json["DNS"]       = (const char *)ipsettings.dns;

    writeJSONCfgFile(json, ipCfgName, FlashROMode, "writeIpSettings");
}

void deleteIpSettings()
{
    #ifdef SID_DBG
    Serial.println(F("deleteIpSettings: Deleting ip config"));
    #endif

    if(FlashROMode) {
        SD.remove(ipCfgName);
    } else if(haveFS) {
        SPIFFS.remove(ipCfgName);
    }
}

/*
 *  Load/save/delete remote ID
 */

static bool loadId()
{
    bool invalid = false;
    bool haveConfig = false;
  
    if(!haveFS && !FlashROMode)
        return false;
  
    if( (!FlashROMode && SPIFFS.exists(idName)) ||
         (FlashROMode && SD.exists(idName)) ) {
  
        File configFile = FlashROMode ? SD.open(idName, "r") : SPIFFS.open(idName, "r");
    
        if(configFile) {
    
            DECLARE_S_JSON(512, json);
            //StaticJsonDocument<512> json;
      
            DeserializationError error = readJSONCfgFile(json, configFile, "loadId");
      
            if(!error) {
      
                myRemID = (uint32_t)json["ID"];
        
                #ifdef FC_DBG
                Serial.printf("Loaded Remote ID: 0x%lx\n", myRemID);
                #endif
        
                invalid = (myRemID == 0);
        
                haveConfig = !invalid;
      
            } else {
      
                invalid = true;
      
            }
      
            configFile.close();
  
      }
  
    }
  
    if(invalid) {
        // config file is invalid
        Serial.println(F("loadId: ID invalid; creating new ID"));
    }
  
    return haveConfig;
}

static uint32_t createId()
{
    return esp_random() ^ esp_random() ^ esp_random();
}

static void saveId()
{
    DECLARE_S_JSON(512, json);
    //StaticJsonDocument<512> json;
  
    if(!haveFS && !FlashROMode)
        return;
  
    json["ID"] = myRemID;
  
    writeJSONCfgFile(json, idName, FlashROMode, "saveId");
}


/*
 * Various helpers
 */


/* Copy secondary settings from/to SD if user
 * changed "save to SD"-option in CP
 */
void copySettings()
{
    if(!haveSD || !haveFS)
        return;

    configOnSD = !configOnSD;

    if(configOnSD || !FlashROMode) {
        #ifdef SID_DBG
        Serial.println(F("copySettings: Copying secondary settings to other medium"));
        #endif
        saveBrightness(false);
        saveIRLock(false);
        saveIRKeys();
        // NOT idlePat/strict, is only stored on SD
    }

    configOnSD = !configOnSD;
}

/*
 * Helpers for JSON config files
 */
static DeserializationError readJSONCfgFile(JsonDocument& json, File& configFile, const char *funcName)
{
    const char *buf = NULL;
    size_t bufSize = configFile.size();
    DeserializationError ret;

    if(!(buf = (const char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return DeserializationError::NoMemory;
    }

    memset((void *)buf, 0, bufSize + 1);

    configFile.read((uint8_t *)buf, bufSize);

    #ifdef SID_DBG
    Serial.println(buf);
    #endif
    
    ret = deserializeJson(json, buf);

    free((void *)buf);

    return ret;
}


static bool writeJSONCfgFile(const JsonDocument& json, const char *fn, bool useSD, const char *funcName)
{
    char *buf;
    size_t bufSize = measureJson(json);
    bool success = false;

    if(!(buf = (char *)malloc(bufSize + 1))) {
        Serial.printf("%s: Buffer allocation failed (%d)\n", funcName, bufSize);
        return false;
    }

    memset(buf, 0, bufSize + 1);
    serializeJson(json, buf, bufSize);

    #ifdef SID_DBG
    Serial.printf("Writing %s to %s\n", fn, useSD ? "SD" : "FS");
    Serial.println((const char *)buf);
    #endif

    if(useSD) {
        success = writeFileToSD(fn, (uint8_t *)buf, (int)bufSize);
    } else {
        success = writeFileToFS(fn, (uint8_t *)buf, (int)bufSize);
    }

    free(buf);

    if(!success) {
        Serial.printf("%s: %s\n", funcName, failFileWrite);
    }

    return success;
}

/*
 * Generic file readers/writes for external
 */
static bool writeFileToSD(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveSD)
        return false;

    File myFile = SD.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}

static bool writeFileToFS(const char *fn, uint8_t *buf, int len)
{
    size_t bytesw;
    
    if(!haveFS)
        return false;

    File myFile = SPIFFS.open(fn, FILE_WRITE);
    if(myFile) {
        bytesw = myFile.write(buf, len);
        myFile.close();
        return (bytesw == len);
    } else
        return false;
}
