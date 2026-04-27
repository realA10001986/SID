/*
 * -------------------------------------------------------------------
 * CircuitSetup.us Status Indicator Display
 * (C) 2023-2026 Thomas Winischhofer (A10001986)
 * https://github.com/realA10001986/SID
 * https://sid.out-a-ti.me
 *
 * Settings handling
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

#ifndef _SID_SETTINGS_H
#define _SID_SETTINGS_H

#define MS(s) XMS(s)
#define XMS(s) #s

void settings_setup();

void unmount_fs();

void write_settings();
bool checkConfigExists();

bool evalBool(char *s);

void saveIRKeys();
void deleteIRKeys();

void loadBrightness();
void storeBrightness();
void saveBrightness();

void loadIRLock();
void storeIRLock();
void saveIRLock();

void loadStrict();
void saveStrict();

void loadSASettings();
void saveSASettings();

void loadPosIRFB();
void savePosIRFB();

void loadIRCFB();
void saveIRCFB();

void saveUpdAvail();

void loadUpdVers(int &v, int& r);
void saveUpdVers(int v, int r);

void saveAllSecCP();

void saveCarMode();

void loadIdlePat();
void storeIdlePat();
void saveIdlePat();

#define BOOTM_NORMAL 0
#define BOOTM_IGNORE BOOTM_NORMAL
#define BOOTM_SA     1
uint8_t loadBootMode();
void    storeBootMode(uint8_t bootMode);
void    saveBootMode();

bool loadIpSettings();
void writeIpSettings();
void deleteIpSettings();

void moveSettings();

// Default settings

#define DEF_HOSTNAME        "sid"
#define DEF_WIFI_RETRY      3     // 1-10; Default: 3 retries
#define DEF_AP_CHANNEL      1     // 1-13; 0 = random(1-13)
#define DEF_WIFI_APOFFDELAY 0

#define DEF_STRICT          1     // 0: Allow random diviations from movie patterns; 1: no not
#define DEF_SKIP_TTANIM     1     // 0: Don't skip tt anim; 1: do
#define DEF_SA_PEAKS        0     // 1: Show peaks in SA, 0: don't
#define DEF_SA_MIRROR       0     // 1: Show "mirrored" SA, 0: don't
#define DEF_IRFB            1     // 0: Don't show positive IR feedback on display; 1: do
#define DEF_IRCFB           1     // 0: Don't show command entry feedback; 1: do
#define DEF_SS_TIMER        0     // "Screen saver" timeout in minutes; 0 = ss off

#define DEF_TCD_IP          ""    // TCD hostname (or ip address) for BTTFN
#define DEF_USE_GPSS        0     // 0: Ignore GPS speed; 1: Use it for chase speed
#define DEF_USE_NM          0     // 0: Ignore TCD night mode; 1: Follow TCD night mode
#define DEF_USE_FPO         0     // 0: Ignore TCD fake power; 1: Follow TCD fake power
#define DEF_BTTFN_TT        1     // 0: '0' on IR remote and TT button trigger stand-alone TT; 1: They trigger BTTFN-wide TT
#define DEF_SS_CLK          0     // "Screen saver" is clock (0=off, 1=on)
#define DEF_SS_CLK_NMOFF    0     // 0: Clock dimmed in NM 1: Clock off in NM

#define DEF_TCD_PRES        0     // 0: No TCD connected, 1: connected via GPIO
#define DEF_NO_ETTO_LEAD    0     // Default: 0: TCD signals TT with ETTO_LEAD lead time; 1 without

#define DEF_CFG_ON_SD       1     // Default: Save secondary settings on SD card
#define DEF_SD_FREQ         0     // SD/SPI frequency: Default 16MHz

#define DEF_DISDIR          0     // 0: Do not disable default IR remote control; 1: do

struct Settings {
    char ssid[34]           = "";
    char pass[66]           = "";
    char bssid[18]          = "";

    char cm_ssid[14]        = "TCD-AP";
    char cm_pass[10]        = "";
    char cm_bssid[18]       = "";

    char hostName[32]       = DEF_HOSTNAME;
    char wifiConRetries[4]  = MS(DEF_WIFI_RETRY);
    char systemID[8]        = "";
    char appw[10]           = "";
    char apChnl[4]          = MS(DEF_AP_CHANNEL);
    char wifiAPOffDelay[4]  = MS(DEF_WIFI_APOFFDELAY);
    
    char skipTTAnim[2]      = MS(DEF_SKIP_TTANIM);
    char ssTimer[4]         = MS(DEF_SS_TIMER);
    
    char tcdIP[32]          = DEF_TCD_IP;
    char useGPSS[2]         = MS(DEF_USE_GPSS);
    char useNM[2]           = MS(DEF_USE_NM);
    char useFPO[2]          = MS(DEF_USE_FPO);
    char bttfnTT[2]         = MS(DEF_BTTFN_TT);
    char ssClock[2]         = MS(DEF_SS_CLK);
    char ssClockOffNM[2]    = MS(DEF_SS_CLK_NMOFF);    

    char TCDpresent[2]      = MS(DEF_TCD_PRES);
    char noETTOLead[2]      = MS(DEF_NO_ETTO_LEAD);

    char CfgOnSD[2]         = MS(DEF_CFG_ON_SD);
    char sdFreq[2]          = MS(DEF_SD_FREQ);

    char disDIR[2]          = MS(DEF_DISDIR);

#ifdef SID_HAVEMQTT  
    char useMQTT[2]         = "0";
    char mqttVers[2]        = "0"; // 0 = 3.1.1, 1 = 5.0
    char mqttServer[80]     = "";  // ip or domain [:port]  
    char mqttUser[128]      = "";  // user[:pass] (UTF8)
    char mqttTopic[128]     = "";  // topic (UTF8)       [limited to 127 bytes through WM]
#endif 

    // Kludge for CP
    char strictMode[2]      = MS(DEF_STRICT);
    char SApeaks[2]         = MS(DEF_SA_PEAKS);
    char SAmirror[2]        = MS(DEF_SA_MIRROR);
    char PIRFB[2]           = MS(DEF_IRFB);
    char PIRCFB[2]          = MS(DEF_IRCFB);
    char ecmKludge[2]       = "0";  // MUST BE 0
};

struct IPSettings {
    char ip[20]       = "";
    char gateway[20]  = "";
    char netmask[20]  = "";
    char dns[20]      = "";
};

extern struct Settings settings;
extern struct IPSettings ipsettings;

extern bool   haveSD;
extern bool   FlashROMode;

#endif
